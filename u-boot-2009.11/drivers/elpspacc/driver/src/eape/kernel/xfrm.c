#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/ratelimit.h>

#include <net/protocol.h>
#include <net/xfrm.h>
#include <net/esp.h>

#include "eape.h"

#include "cs75xx.h"

/*
 * Newer Linux versions have something like this defined automatically, so
 * remove this once we target them.
 */
#if (defined CONFIG_IPV6 || defined CONFIG_IPV6_MODULE)
#  define IPV6_ENABLED 1
#else
#  define IPV6_ENABLED 0
#endif

struct elpxfrm {
   bool disabled;
   int family;

   const struct xfrm_type *xfrm;
   union elpxfrm_proto {
      const struct net_protocol   *v4;
      const struct inet6_protocol *v6;
   } proto;
};

struct elpxfrm_state {
   eape_device *eape;
   int handle;
};

struct elpxfrm_data {
   struct sk_buff *skb;
   pdu_ddt src, dst;

   dma_addr_t dma_hdr;
   size_t dma_hdr_len;

   int sg_nents, dma_nents;
   struct scatterlist sg[];
};

/* Determine the next header for both IPv4 and IPv6. */
static int ip_next_header(struct sk_buff *skb)
{
   switch (ip_hdr(skb)->version) {
   case 4:
      return ip_hdr(skb)->protocol;
#if IPV6_ENABLED
   case 6:
      return ipv6_hdr(skb)->nexthdr;
#endif
   default:
      WARN_ON(1);
      return -EINVAL;
   }
}

static int elpxfrm_init_ddts(struct elpxfrm_data *data)
{
   struct scatterlist *sg_entry;
   int i, rc;

   rc = DMA_MAP_SG(NULL, data->sg, data->sg_nents, DMA_BIDIRECTIONAL);
   if (rc <= 0)
      return -ENOMEM;
   data->dma_nents = rc;

   rc = pdu_ddt_init(&data->src, 0x80000000UL | (data->dma_nents+1));
   if (rc < 0) {
      rc = pdu_error_code(rc);
      goto err_unmap_sg;
   }

   rc = pdu_ddt_init(&data->dst, 0x80000000UL | (data->dma_nents+1));
   if (rc < 0) {
      rc = pdu_error_code(rc);
      goto err_free_src;
   }

   data->dma_hdr_len = data->skb->data - skb_network_header(data->skb);
   data->dma_hdr = DMA_MAP_SINGLE(NULL, skb_network_header(data->skb),
                                  data->dma_hdr_len, DMA_BIDIRECTIONAL);
   if (dma_mapping_error(NULL, data->dma_hdr)) {
      rc = -ENOMEM;
      goto err_free_dst;
   }

   pdu_ddt_add(&data->src, data->dma_hdr, data->dma_hdr_len);
   pdu_ddt_add(&data->dst, data->dma_hdr, data->dma_hdr_len);

   for_each_sg(data->sg, sg_entry, data->dma_nents, i) {
      pdu_ddt_add(&data->src, sg_dma_address(sg_entry), sg_dma_len(sg_entry));
      pdu_ddt_add(&data->dst, sg_dma_address(sg_entry), sg_dma_len(sg_entry));
   }

   DMA_SYNC_SINGLE_FOR_DEVICE(NULL, data->dma_hdr, data->dma_hdr_len,
                                                   DMA_BIDIRECTIONAL);
   DMA_SYNC_SG_FOR_DEVICE(NULL, data->sg, data->sg_nents, DMA_BIDIRECTIONAL);

   return 0;
err_free_dst:
   pdu_ddt_free(&data->dst);
err_free_src:
   pdu_ddt_free(&data->src);
err_unmap_sg:
   DMA_UNMAP_SG(NULL, data->sg, data->sg_nents, DMA_BIDIRECTIONAL);
   return rc;
}

static void elpxfrm_free_ddts(struct elpxfrm_data *data, bool dma_sync)
{
   pdu_ddt_free(&data->dst);
   pdu_ddt_free(&data->src);

   if (dma_sync) {
      DMA_SYNC_SINGLE_FOR_CPU(NULL, data->dma_hdr, data->dma_hdr_len,
                                                   DMA_BIDIRECTIONAL);
      DMA_SYNC_SG_FOR_CPU(NULL, data->sg, data->sg_nents, DMA_BIDIRECTIONAL);
   }

   DMA_UNMAP_SINGLE(NULL, data->dma_hdr, data->dma_hdr_len, DMA_BIDIRECTIONAL);
   DMA_UNMAP_SG(NULL, data->sg, data->sg_nents, DMA_BIDIRECTIONAL);
}

static struct elpxfrm_data *
elpxfrm_init_data(struct sk_buff *skb, struct xfrm_state *x)
{
   struct elpxfrm_data *data;
   struct sk_buff *trailer;
   int nents, rc;

   nents = skb_cow_data(skb, x->props.trailer_len, &trailer);
   if (nents < 0)
      return ERR_PTR(nents);
   pskb_put(skb, trailer, x->props.trailer_len);

   data = kmalloc(sizeof *data + nents * sizeof data->sg[0], GFP_ATOMIC);
   if (!data)
      return ERR_PTR(-ENOMEM);
   data->skb = skb;

   sg_init_table(data->sg, nents);
   rc = skb_to_sgvec(skb, data->sg, 0, skb->len);
   if (rc < 0) {
      kfree(data);
      return ERR_PTR(rc);
   }
   data->sg_nents = rc;

   return data;
}

static void elpxfrm_output_done(void *ea, void *_data,
                                uint32_t payload_len, uint32_t ret_code,
                                uint32_t sw_id)
{
   struct elpxfrm_data *data = _data;
   struct sk_buff *skb = data->skb;
   int err = -EIO;
//   printk_ratelimited("outbound done: %zu\n", ret_code);
   /* We set SEQ_ROLL in the SA for now. */
   if (ret_code == EAPE_OK || ret_code == EAPE_SEQ_ROLL) {
      skb_push(skb, -skb_network_offset(skb));
      pskb_trim(skb, payload_len);
      err = 0;
   } else {
      pr_warn_ratelimited("outbound error %u\n", (unsigned)ret_code);
   }

   elpxfrm_free_ddts(data, err >= 0);
   kfree(data);

   if (err < -1) {
      err = NET_XMIT_CN;
   }

   xfrm_output_resume(skb, err);
}

/*
 * Outbound SKB layout:
 *
 *                     --header_len--             --trailer_len--
 * | IP | [ext hdrs] | reserved space | payload | reserved  space |
 * ^                 ^                ^         ^
 * network_header    |               data      tail
 *           transport_header
 */

static int elpxfrm_output(struct xfrm_state *x, struct sk_buff *skb)
{
   struct elpxfrm_state *state = x->data;
   struct elpxfrm_data *data;
   int rc;

   data = elpxfrm_init_data(skb, x);
   if (IS_ERR(data))
      return PTR_ERR(data);

   rc = elpxfrm_init_ddts(data);
   if (rc < 0)
      goto err_free_data;

   /* Fix up the source DDT to skip the ESP header room. */
   data->src.virt[1] = skb_transport_header(skb) - skb_network_header(skb);

   rc = eape_go(state->eape, state->handle, 1, &data->src, &data->dst,  elpxfrm_output_done, data);
   //printk_ratelimited("outbound: %d\n", rc);
   if (rc >= 0) {
      return -EINPROGRESS;
   } else if (rc == EAPE_ERR) {
      /* Fatal error that indicates a problem in the SDK. */
      WARN(1, "fatal processing error\n");
      rc = -EIO;
      goto err_free_ddts;
   }
   printk_ratelimited("output: drop packet %d\n", rc);

   rc = NET_XMIT_CN;
//   rc = NET_XMIT_DROP;
err_free_ddts:
   elpxfrm_free_ddts(data, false);
err_free_data:
   kfree(data);
   return rc;
}

static void elpxfrm_input_done(void *ea, void *_data,
                                uint32_t payload_len, uint32_t ret_code,
                                uint32_t sw_id)
{
   struct elpxfrm_data *data = _data;
   struct sk_buff *skb = data->skb;
   int err = -EIO;

   elpxfrm_free_ddts(data, 1);
   if (ret_code == EAPE_OK) {
      skb_push(skb, -skb_network_offset(skb));
      pskb_trim(skb, payload_len);
      __skb_pull(skb, skb_transport_offset(skb));
      skb->transport_header = skb->network_header;

       err = ip_next_header(skb);
   } else {
      pr_warn_ratelimited("inbound error %u\n", (unsigned)ret_code);
   }

   if (err < -1) {
      err = -EBUSY;
   }

   kfree(data);

   xfrm_input_resume(skb, err);
}

/*
 * Inbound SKB layout:
 *
 * | IP | [ext hdrs] | ESP/AH hdr | payload | [ESP trailer] |
 * ^                 ^            ^                         ^
 * network_header    |           data                      tail
 *           transport_header
 *
 */

static int elpxfrm_input(struct xfrm_state *x, struct sk_buff *skb)
{
   struct elpxfrm_state *state = x->data;
   struct elpxfrm_data *data;
   int rc;

   data = elpxfrm_init_data(skb, x);
   if (IS_ERR(data))
      return PTR_ERR(data);

   rc = elpxfrm_init_ddts(data);
   if (rc < 0)
      goto err_free_data;

   rc = eape_go(state->eape, state->handle, 0, &data->src, &data->dst, elpxfrm_input_done, data);
   //printk_ratelimited("inbound: %d\n", rc);

   if (rc >= 0) {
      return -EINPROGRESS;
   } else if (rc == EAPE_ERR) {
      /* Fatal error that indicates a problem in the SDK. */
      WARN(1, "fatal processing error\n");
      rc = -EIO;
      goto err_free_ddts;
   }
   printk_ratelimited("input: dropped packet %d\n", rc);

   rc = -EBUSY;
err_free_ddts:
   elpxfrm_free_ddts(data, false);
err_free_data:
   kfree(data);
   return rc;
}

static uint32_t elpxfrm_mac_alg(struct xfrm_algo_auth *aalg)
{
   unsigned icv_truncbits = aalg->alg_trunc_len;
   unsigned alg_id;

   if (!strcmp(aalg->alg_name, "digest_null"))
      return 0;
   else if (!strcmp(aalg->alg_name, "hmac(md5)"))
      alg_id = EAPE_SA_ALG_AUTH_MD5_96;
   else if (!strcmp(aalg->alg_name, "hmac(sha1)"))
      alg_id = EAPE_SA_ALG_AUTH_SHA1_96;
   else if (!strcmp(aalg->alg_name, "hmac(sha256)"))
      alg_id = EAPE_SA_ALG_AUTH_SHA256;
   else
      return -1;

   /* XXX Handle XCBC and CMAC. */

   /* Ensure that the truncated ICV length is valid. */
   if (alg_id == EAPE_SA_ALG_AUTH_MD5_96 && icv_truncbits != 96)
      return -1;
   if (alg_id == EAPE_SA_ALG_AUTH_SHA1_96 && icv_truncbits != 96)
      return -1;
   if (alg_id == EAPE_SA_ALG_AUTH_SHA256 && icv_truncbits != 128)
      return -1;

   return alg_id;
}

static uint32_t elpxfrm_enc_alg(struct xfrm_algo *ealg)
{
   unsigned alg_id;

   if (strcmp(ealg->alg_name, "ecb(cipher_null)") == 0)
      return 0;
   else if (strcmp(ealg->alg_name, "cbc(des)") == 0)
      alg_id = EAPE_SA_ALG_CIPH_DES_CBC;
   else if (strcmp(ealg->alg_name, "cbc(des3_ede)") == 0)
      alg_id = EAPE_SA_ALG_CIPH_3DES_CBC;
   else if (strcmp(ealg->alg_name, "cbc(aes)") == 0)
      alg_id = EAPE_SA_ALG_CIPH_AES128_CBC;
   else if (strcmp(ealg->alg_name, "rfc3686(ctr(aes))") == 0)
      alg_id = EAPE_SA_ALG_CIPH_AES128_CTR;
   else
      return -1;

   /* XXX Where does GMAC fit in here? */

   switch (ealg->alg_key_len) {
   case 56:
   case 128:
      break;
   case 168:
      break;
   case 192:
      alg_id += 1;
      break;
   case 256:
      alg_id += 2;
      break;
   default:
      return -1;
   }

   return alg_id;
}

static int elpxfrm_init_state(struct xfrm_state *x, bool ipv6)
{
   unsigned icv_truncbytes = 0, block_align;
   struct elpxfrm_state *state;
   eape_sa sa = {0};
   int rc;

   state = kmalloc(sizeof *state, GFP_KERNEL);
   if (!state)
      return -ENOMEM;

   *state = (struct elpxfrm_state) {
      .eape = eape_get_device(),
   };

   rc = eape_open(state->eape);
   if (rc < 0) {
      rc = pdu_error_code(rc);
      goto err_free_state;
   }
   state->handle = rc;

   /* Only transport and tunnel modes are supported by HW. */
   if (x->props.mode != XFRM_MODE_TRANSPORT  && x->props.mode != XFRM_MODE_TUNNEL) {
      return -EINVAL;
   }

   /* GCM/CCM not supported yet. */
   if (x->aead) {
      return -EINVAL;
   }

   /* Auth algorithm will be set for both AH and ESP. */
   if (x->aalg) {
      uint32_t mac;

      icv_truncbytes = x->aalg->alg_trunc_len/8;

      mac = elpxfrm_mac_alg(x->aalg);
      if (mac == -1 || x->aalg->alg_key_len/8 > sizeof sa.mackey)
         return -EINVAL;

      memcpy(sa.mackey, x->aalg->alg_key, x->aalg->alg_key_len/8);
      sa.flags |= EAPE_SA_FLAGS_HDR_TYPE; /* This field is badly named. */
      sa.alg |= mac;

      /* Set params for AH; will be overwritten for ESP below. */
      x->props.header_len  = sizeof (struct ip_auth_hdr) + icv_truncbytes;
      x->props.trailer_len = 0;
   }

   /* Encryption algorithm will only be set for ESP. */
   if (x->ealg) {
      struct xfrm_algo_desc *desc = xfrm_ealg_get_byname(x->ealg->alg_name, 0);
      uint32_t enc;

      enc = elpxfrm_enc_alg(x->ealg);
      if (enc == -1)
         return -EINVAL;

      block_align = ALIGN(desc->uinfo.encr.blockbits/8, 4);
      x->props.header_len = sizeof (struct ip_esp_hdr);
      x->props.trailer_len = block_align + icv_truncbytes + 1;

      /* XXX CTR needs some work. */
      if (enc != 0)
         x->props.header_len += desc->uinfo.encr.blockbits/8;

      memcpy(sa.ckey, x->ealg->alg_key, x->ealg->alg_key_len/8);
      sa.flags &= ~EAPE_SA_FLAGS_HDR_TYPE;
      sa.alg |= enc;
   }

   if (x->encap) {
      struct xfrm_encap_tmpl *encap = x->encap;
      switch (encap->encap_type) {
      default:
         goto err_free_handle;
      case UDP_ENCAP_ESPINUDP:
         x->props.header_len += sizeof(struct udphdr);
         break;
      case UDP_ENCAP_ESPINUDP_NON_IKE:
         x->props.header_len += sizeof(struct udphdr) + 2 * sizeof(u32);
         break;
      }
   }

   sa.spi = x->id.spi;
   sa.flags |= EAPE_SA_FLAGS_SEQ_ROLL;
   sa.flags |= EAPE_SA_FLAGS_ACTIVE;

   if (ipv6) {
      sa.flags |= EAPE_SA_FLAGS_IPV6;
   }

   rc = eape_build_sa(state->eape, state->handle, &sa);
   if (rc < 0) {
      rc = -ENODEV;
      goto err_free_handle;
   }

   x->data = state;
   return 0;
err_free_handle:
   eape_close(state->eape, state->handle);
err_free_state:
   kfree(state);
   return rc;
}

static void elpxfrm_destroy(struct xfrm_state *x)
{
   struct elpxfrm_state *state = x->data;

   if (state) {
      x->data = NULL;

      eape_close(state->eape, state->handle);
      kfree(state);
   }
}

static u32 elpxfrm_get_mtu(struct xfrm_state *x, int mtu)
{
   return mtu - x->props.header_len - x->props.trailer_len;
}

static int elpxfrm4_init_state(struct xfrm_state *x)
{
   return elpxfrm_init_state(x, false);
}

static void elpah4_err(struct sk_buff *skb, u32 info)
{
}

static void elpesp4_err(struct sk_buff *skb, u32 info)
{
}

#if IPV6_ENABLED
static int elpxfrm6_init_state(struct xfrm_state *x)
{
   return elpxfrm_init_state(x, true);
}

static void elpah6_err(struct sk_buff *skb, struct inet6_skb_parm *opt,
                       u8 type, u8 code, int offset, __be32 info)
{
}

static void elpesp6_err(struct sk_buff *skb, struct inet6_skb_parm *opt,
                        u8 type, u8 code, int offset, __be32 info)
{
}
#endif

enum {
   ELPXFRM_AH4,
   ELPXFRM_ESP4,
   ELPXFRM_AH6,
   ELPXFRM_ESP6,
};

static struct elpxfrm xfrms[] = {
   [ELPXFRM_AH4] = {
      .family = AF_INET,
      .xfrm = &(const struct xfrm_type){
         .description = "AH4",
         .owner       = THIS_MODULE,
         .proto       = IPPROTO_AH,
         .init_state  = elpxfrm4_init_state,
         .destructor  = elpxfrm_destroy,
         .output      = elpxfrm_output,
         .input       = elpxfrm_input,
      },
      .proto.v4 = &(const struct net_protocol) {
         .handler     = xfrm4_rcv,
         .err_handler = elpah4_err,
         .no_policy   = 1,
      },
   },
   [ELPXFRM_ESP4] = {
      .family = AF_INET,
      .xfrm = &(const struct xfrm_type){
         .description = "ESP4",
         .owner       = THIS_MODULE,
         .proto       = IPPROTO_ESP,
         .init_state  = elpxfrm4_init_state,
         .destructor  = elpxfrm_destroy,
         .output      = elpxfrm_output,
         .input       = elpxfrm_input,
         .get_mtu     = elpxfrm_get_mtu,
      },
      .proto.v4 = &(const struct net_protocol) {
         .handler     = xfrm4_rcv,
         .err_handler = elpesp4_err,
         .no_policy   = 1,
      },
   },
#if IPV6_ENABLED
   [ELPXFRM_AH6] = {
      .family = AF_INET6,
      .xfrm = &(const struct xfrm_type){
         .description = "AH6",
         .owner       = THIS_MODULE,
         .proto       = IPPROTO_AH,
         .init_state  = elpxfrm6_init_state,
         .destructor  = elpxfrm_destroy,
         .output      = elpxfrm_output,
         .input       = elpxfrm_input,
         .hdr_offset  = xfrm6_find_1stfragopt,
      },
      .proto.v6 = &(const struct inet6_protocol) {
         .handler     = xfrm6_rcv,
         .err_handler = elpah6_err,
         .flags       = INET6_PROTO_NOPOLICY,
      },
   },
   [ELPXFRM_ESP6] = {
      .family = AF_INET6,
      .xfrm = &(const struct xfrm_type){
         .description = "ESP6",
         .owner       = THIS_MODULE,
         .proto       = IPPROTO_ESP,
         .init_state  = elpxfrm6_init_state,
         .destructor  = elpxfrm_destroy,
         .output      = elpxfrm_output,
         .input       = elpxfrm_input,
         .get_mtu     = elpxfrm_get_mtu,
         .hdr_offset  = xfrm6_find_1stfragopt,
      },
      .proto.v6 = &(const struct inet6_protocol) {
         .handler     = xfrm6_rcv,
         .err_handler = elpesp6_err,
         .flags       = INET6_PROTO_NOPOLICY,
      },
   },
#endif
};

module_param_named(no_ah4,  xfrms[ELPXFRM_AH4].disabled,  bool, S_IRUGO);
module_param_named(no_esp4, xfrms[ELPXFRM_ESP4].disabled, bool, S_IRUGO);
#if IPV6_ENABLED
module_param_named(no_ah6,  xfrms[ELPXFRM_AH6].disabled,  bool, S_IRUGO);
module_param_named(no_esp6, xfrms[ELPXFRM_ESP6].disabled, bool, S_IRUGO);
#endif

static int __init elpxfrm_register(const struct elpxfrm *x)
{
   int rc;

   rc = xfrm_register_type(x->xfrm, x->family);
   if (rc < 0) {
      pr_warn("failed to register %s xfrm.\n", x->xfrm->description);
      return rc;
   }

   switch (x->family) {
   case AF_INET:
      rc = inet_add_protocol(x->proto.v4, x->xfrm->proto);
      break;
#if IPV6_ENABLED
   case AF_INET6:
      rc = inet6_add_protocol(x->proto.v6, x->xfrm->proto);
      break;
#endif
   default:
      BUG();
   }

   if (rc < 0) {
      pr_warn("failed to register %s protocol.\n", x->xfrm->description);
      return rc;
   }

   return 0;
}

static void __exit
elpxfrm_unregister(const struct elpxfrm *x)
{
   switch (x->family) {
   case AF_INET:
      inet_del_protocol(x->proto.v4, x->xfrm->proto);
      break;
#if IPV6_ENABLED
   case AF_INET6:
      inet6_del_protocol(x->proto.v6, x->xfrm->proto);
      break;
#endif
   }

   xfrm_unregister_type(x->xfrm, x->family);
}

static int __init elpxfrm_init(void)
{
   int ret = -EAGAIN, rc;
   size_t i;

   for (i = 0; i < ARRAY_SIZE(xfrms); i++) {
      if (xfrms[i].disabled)
         continue;

      rc = elpxfrm_register(&xfrms[i]);
      if (rc < 0) {
         xfrms[i].disabled = true;
         continue;
      }

      ret = 0;
   }

   return ret;
}
module_init(elpxfrm_init);

static void __exit elpxfrm_exit(void)
{
   size_t i;

   for (i = 0; i < ARRAY_SIZE(xfrms); i++) {
      if (xfrms[i].disabled)
         continue;

      elpxfrm_unregister(&xfrms[i]);
   }
}
module_exit(elpxfrm_exit);

MODULE_AUTHOR("Elliptic Technologies Inc.");
MODULE_DESCRIPTION("ESP/AH hardware offload driver.");
MODULE_LICENSE("GPL");
