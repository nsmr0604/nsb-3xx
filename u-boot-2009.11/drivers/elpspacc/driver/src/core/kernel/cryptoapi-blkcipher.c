#include <linux/version.h>
#include <linux/module.h>
#include <linux/dmapool.h>
#include <crypto/aead.h>
#include <crypto/aes.h>
#include <crypto/algapi.h>
#include <crypto/authenc.h>
#include <linux/rtnetlink.h>
#include "elpspaccdrv.h"
#include "cryptoapi.h"


static int spacc_blkcipher_setkey(struct crypto_tfm *tfm, const u8 *key, unsigned int keylen)
{
   struct spacc_crypto_ctx *ctx  = crypto_tfm_ctx(tfm);
   const struct spacc_alg  *salg = spacc_tfm_alg(tfm);
   struct spacc_priv       *priv = dev_get_drvdata(salg->dev);
   int err;

   // are keylens valid?
   ctx->ctx_valid = false;

#if 0
   // TODO
   if (!spacc_keylen_ok(salg, keylen)) {
      dev_warn(salg->dev, "Key size of %u is not valid\n", keylen);
      return -EINVAL;
   }
#endif

   err = spacc_write_context(&priv->spacc, ctx->handle, SPACC_CRYPTO_OPERATION, key, keylen, NULL, 0);
   if (err) {
      dev_warn(salg->dev, "Could not write ciphering context: %d\n", err);
      return -EINVAL;
   }

   // set expand key
   spacc_set_key_exp(&priv->spacc, ctx->handle);
   ctx->ctx_valid = true;

   return 0;
}

static void spacc_blkcipher_cb(void *spacc, void *ctx)
{
	atomic_inc(ctx);
}

static int spacc_blkcipher_process(struct blkcipher_desc *desc,
				   struct scatterlist *dst, struct scatterlist *src,
				   unsigned int nbytes,
				   int encrypt)
{
	struct crypto_blkcipher *cipher = desc->tfm;
	struct spacc_crypto_ctx *ctx = crypto_blkcipher_ctx(cipher);
	struct spacc_priv *priv = dev_get_drvdata(ctx->dev);
	pdu_ddt src_pdu, dst_pdu;
	int src_nents = -1, dst_nents = -1;
	int err = 0;

	if (ctx->handle < 0 || !ctx->ctx_valid) {
		return -EINVAL;
	}

	if (dst != src) {
		err = spacc_sg_to_ddt(ctx->dev, src, nbytes,
				      &src_pdu, DMA_TO_DEVICE);
		if (err < 0) {
			dev_dbg(ctx->dev, "%s: spacc_sg_to_ddt/src: %d\n",
				__func__, err);
			goto out_unmap;
		}
		src_nents = err;
		err = spacc_sg_to_ddt(ctx->dev, dst, nbytes,
				      &dst_pdu, DMA_FROM_DEVICE);
		if (err < 0) {
			dev_dbg(ctx->dev, "%s: spacc_sg_to_ddt/dst: %d\n",
				__func__, err);
			goto out_unmap;
		}
		dst_nents = err;
	} else {
		err = spacc_sg_to_ddt(ctx->dev, src, nbytes,
				      &src_pdu, DMA_BIDIRECTIONAL);
		if (err < 0) {
			dev_dbg(ctx->dev, "%s: spacc_sg_to_ddt/src: %d\n",
				__func__, err);
			goto out_unmap;
		}
		src_nents = err;
	}
	err = spacc_set_operation(&priv->spacc, ctx->handle,
				  encrypt ? OP_ENCRYPT : OP_DECRYPT,
				  0, 0, 0, 0, 0);
	if (err < 0) {
		dev_dbg(ctx->dev, "%s: spacc_set_operation: %d\n",
			__func__, err);
		err = -EINVAL;
		goto out_unmap;
	}
	atomic_set(&ctx->sync_completion, 0);
	wmb();
	err = spacc_packet_enqueue_ddt(&priv->spacc, ctx->handle,
				       &src_pdu, src == dst ? &src_pdu : &dst_pdu, nbytes,
				       0, 0, 0, 0, SPACC_SW_CTRL_PRIO_HI);
	if (err < 0) {
		dev_dbg(ctx->dev, "%s: spacc_packet_enqueue_ddt: %d\n",
			__func__, err);
		err = -EINVAL;
		goto out_unmap;
	}
	get_cpu();
	tasklet_disable(&priv->pop_jobs);
	while (atomic_read(&ctx->sync_completion) == 0) {
		rmb();
		spacc_pop_jobs((unsigned long)priv);
	}
	tasklet_enable(&priv->pop_jobs);
	put_cpu();
	err = 0;

out_unmap:
	if (dst != src) {
		if (src_nents >= 0) {
			dma_unmap_sg(NULL, src, src_nents, DMA_TO_DEVICE);
			pdu_ddt_free(&src_pdu);
		}
		if (dst_nents >= 0) {
			dma_unmap_sg(NULL, dst, dst_nents, DMA_FROM_DEVICE);
			pdu_ddt_free(&dst_pdu);
		}
	} else {
		if (src_nents >= 0) {
			dma_unmap_sg(NULL, src, src_nents, DMA_BIDIRECTIONAL);
			pdu_ddt_free(&src_pdu);
		}
	}
	return err;
}

static int spacc_blkcipher_encrypt(struct blkcipher_desc *desc,
				   struct scatterlist *dst, struct scatterlist *src,
				   unsigned int nbytes)
{
	return spacc_blkcipher_process(desc, dst, src, nbytes, 1);
}

static int spacc_blkcipher_decrypt(struct blkcipher_desc *desc,
				   struct scatterlist *dst, struct scatterlist *src,
				   unsigned int nbytes)
{
	return spacc_blkcipher_process(desc, dst, src, nbytes, 0);

}

static int spacc_blkcipher_cra_init(struct crypto_tfm *tfm)
{
   struct spacc_crypto_ctx *ctx  = crypto_tfm_ctx(tfm);
   const struct spacc_alg  *salg = spacc_tfm_alg(tfm);
   struct spacc_priv       *priv = dev_get_drvdata(salg->dev);
   int handle;

   // increase reference
   ctx->dev = get_device(salg->dev);

   // open spacc handle
   handle = spacc_open(&priv->spacc, salg->mode->id, CRYPTO_MODE_NULL,
		       0, 0, spacc_blkcipher_cb, &ctx->sync_completion);
   if (handle < 0) {
      dev_dbg(salg->dev, "failed to open SPAcc context\n");
      put_device(ctx->dev);
      return -1;
   }

   ctx->handle = handle;
   ctx->mode   = salg->mode->id;

   return 0;
}

static void spacc_blkcipher_cra_exit(struct crypto_tfm *tfm)
{
   struct spacc_crypto_ctx *ctx = crypto_tfm_ctx(tfm);
   struct spacc_priv *priv = dev_get_drvdata(ctx->dev);

   // close spacc handle
   if (ctx->handle >= 0) {
      spacc_close(&priv->spacc, ctx->handle);
      ctx->handle = -1;
      put_device(ctx->dev);
   }
}


const struct crypto_alg spacc_blkcipher_template __devinitconst = {
   .cra_blkcipher = {
      .setkey      = spacc_blkcipher_setkey,
      .encrypt     = spacc_blkcipher_encrypt,
      .decrypt     = spacc_blkcipher_decrypt,
      .min_keysize = 1,
      .max_keysize = 256,
   },

   .cra_priority = 1301,
   .cra_module   = THIS_MODULE,
   .cra_init     = spacc_blkcipher_cra_init,
   .cra_exit     = spacc_blkcipher_cra_exit,
   .cra_ctxsize  = sizeof (struct spacc_crypto_ctx),
   .cra_type     = &crypto_blkcipher_type,
   .cra_flags    = CRYPTO_ALG_TYPE_BLKCIPHER
                 | CRYPTO_ALG_NEED_FALLBACK
#if LINUX_VERSION_CODE != KERNEL_VERSION (2,6,36)
                 | CRYPTO_ALG_KERN_DRIVER_ONLY
#endif
};


int __init spacc_blkcipher_module_init(void)
{
   return 0;
}

void spacc_blkcipher_module_exit(void)
{
}
