#include <config.h>
#include <common.h>
#include <command.h>
#include <malloc.h>

#include <elpspacc.h>

#define SPACC_ADDR 0xF6600000


static int spacc_hash(spacc_device * spacc,
			 int mode,
			 PDU_DMA_ADDR_T src_phys, unsigned long src_len,
			 PDU_DMA_ADDR_T dst_phys, unsigned long dst_len,
			 const unsigned char *hmackey, unsigned hmackeysize)
{
	int num;
	int handle, rc;
	pdu_ddt src, dst;
	unsigned long offset, pdu_len;

	pdu_ddt_init(&src, 1 + (src_len / SPACC_MAX_PARTICLE_SIZE));

	for (offset = 0; offset < src_len; offset += SPACC_MAX_PARTICLE_SIZE) {
		if (offset + SPACC_MAX_PARTICLE_SIZE < src_len) {
			pdu_len = SPACC_MAX_PARTICLE_SIZE;
		} else {
			pdu_len = src_len - offset;
		}

		pdu_ddt_add(&src, src_phys + offset, pdu_len);
	}

	pdu_ddt_init(&dst, 1);
	pdu_ddt_add(&dst, dst_phys, dst_len);

	handle = spacc_open(spacc, CRYPTO_MODE_NULL, mode, 0, 0, NULL, NULL);

	spacc_write_context(spacc, handle, SPACC_HASH_OPERATION, hmackey,
			    hmackeysize, NULL, 0);

	spacc_set_operation(spacc, handle, OP_ENCRYPT, 0, 0, 0, 0, 0);

	spacc_packet_enqueue_ddt(spacc, handle, &src, &dst, src_len, 0, src_len,
				 0, 0, SPACC_SW_CTRL_PRIO_HI);

	while ((rc = spacc_pop_packets(spacc, &num)) == CRYPTO_INPROGRESS) {
		udelay(10);
	}
	rc = spacc_packet_dequeue(spacc, handle);

	spacc_close(spacc, handle);
	pdu_ddt_free(&dst);
	pdu_ddt_free(&src);
	return rc;
}

static spacc_device *get_spacc_dev(spacc_device * spacc, unsigned long baseaddr)
{
	int err;
	pdu_info info;

	pdu_get_version((void *)baseaddr, &info);

	err = spacc_init(baseaddr, spacc, &info);
	if (err != CRYPTO_OK) {
		return NULL;
	}

	return spacc;
}


static int spacc_cipher_ex(spacc_device *spacc,
   PDU_DMA_ADDR_T src_phys, PDU_DMA_ADDR_T dst_phys, unsigned long src_len,
   int mode, int encrypt,
   const unsigned char *key, unsigned keylen,
   const unsigned char *iv,  unsigned ivlen)
{
	pdu_ddt src, dst;
	int handle, rc, num;

	// create DDT for message
	pdu_ddt_init(&src, 1);	// allocate a single DDT entry
	pdu_ddt_add(&src, src_phys, src_len);	// add our source data to the source ddt

	pdu_ddt_init(&dst, 1);
	pdu_ddt_add(&dst, dst_phys, src_len);	// add the destination data to the destination ddt

	// 1.  Allocate a handle
	// here we aren't hashing so we set the hash mode to CRYPTO_MODE_NULL
	handle = spacc_open(spacc, mode, CRYPTO_MODE_NULL, 0, 0,
			NULL, NULL);

	// 2.  Set context
	// we are setting the CIPHER context with SPACC_CRYPTO_OPERATION
	spacc_write_context(spacc, handle, SPACC_CRYPTO_OPERATION, key, keylen,
			    iv, ivlen);

	// 3.  Set operation
	spacc_set_operation(spacc, handle, encrypt ? OP_ENCRYPT : OP_DECRYPT, 0,
			    0, 0, 0, 0);

	// 4.  set key exp bit if we are decrypting
	if (!encrypt) {
		spacc_set_key_exp(spacc, handle);
	}
	// start the job
	// here we set the pre_aad_sz/post_aad_sz to zero since we are treating
	// the entire source as plaintext
	spacc_packet_enqueue_ddt(spacc, handle, &src, &dst, src_len, 0, 0, 0, 0,
				 SPACC_SW_CTRL_PRIO_HI);


	while ((rc = spacc_pop_packets(spacc, &num)) == CRYPTO_INPROGRESS) {
		udelay(10);
	}

	// wait for the job to complete and allow the user to abort if need be
	rc = spacc_packet_dequeue(spacc, handle);

	pdu_ddt_free(&src);
	pdu_ddt_free(&dst);
	spacc_close(spacc, handle);
	return rc;
}

/**
 *  Borrowed from common/cmd_nand.c
 */
static inline int str2long(char *p, ulong * num)
{
	char *endptr;

	*num = simple_strtoul(p, &endptr, 16);
	return (*p != '\0' && *endptr == '\0') ? 1 : 0;
}

static void dump_results( const unsigned char *value, size_t len)
{
	int x;
	for (x = 0; x < len; x++) {
		printf("%02X ", value[x]);
		if ((x + 1) % 8 == 0) {
			printf("\n");
		}
	}
	printf("\n");
}

struct hash_algs{
	const char *name;
	const int mode_value;
	int result_bytes;
};

static int do_g2crypto_hash( int argc, char *argv[])
{
	spacc_device spacc;
	int rc;
	unsigned char buf[32];
	unsigned long plain_addr, plain_len;

	struct hash_algs support_algs[] = {
		{"md5", CRYPTO_MODE_HASH_MD5, 16},
		{"sha1", CRYPTO_MODE_HASH_SHA1, 20},
		{"sha256", CRYPTO_MODE_HASH_SHA256, 32},
	};

	struct hash_algs *first = support_algs;
	struct hash_algs *last = support_algs + ARRAY_SIZE(support_algs);
	for (; first != last; ++first) {
		if (strcmp(argv[0], first->name) == 0) {
			break;
		}
	}

	if (first == last)
		return 1;

	if (get_spacc_dev(&spacc, SPACC_ADDR) == NULL) {
		return 1;
	}

	if (!str2long(argv[1], &plain_addr))
		return 1;

	if (!str2long(argv[2], &plain_len))
		return 1;

	memset( buf, 0, sizeof(buf));
	rc = spacc_hash(&spacc, first->mode_value,
			plain_addr, plain_len, (PDU_DMA_ADDR_T) buf,
			sizeof(buf), NULL, 0);

	spacc_fini(&spacc);

	if (rc < 0) {
		printf("hash failed\n");
		return 1;
	}

	dump_results(buf, first->result_bytes);
	return 0;
}

static int do_g2crypto_crypttest( int argc, char *argv[])
{
	spacc_device spacc;
	unsigned char buf[3][32], key[16], iv[16];
	int err, x;
	int cnt;

	for (cnt = 0; cnt < 1000; ++cnt) {
		printf( "cnt = %d \n", cnt);


		if (get_spacc_dev(&spacc, SPACC_ADDR) == NULL) {
			return 1;
		}

		memset(buf, 0, sizeof(buf));

		/* make up a message */
		for (x = 0; x < 32; x++) {
			buf[0][x] = x;
		}
		for (x = 0; x < 16; x++) {
			key[x] = x;
			iv[x] = x;
		}

		/* encrypt in AES-128-CBC mode */
		err = spacc_cipher_ex(&spacc, &buf[0][0], &buf[1][0], 32,
				      CRYPTO_MODE_AES_CBC, 1, key, 16, iv, 16);

		/* decrypt in AES-128-CBC mode */
		err = spacc_cipher_ex(&spacc, &buf[1][0], &buf[3][0], 32,
				      CRYPTO_MODE_AES_CBC, 0, key, 16, iv, 16);

		spacc_fini(&spacc);

		if (memcmp(&buf[0][0], &buf[3][0], 32) == 0 &&
		    memcmp(&buf[0][0], &buf[1][0], 32) != 0) {
			printf("AES-128-CBC Correct \n");
		} else {
			printf("AES-128-CBC Failed\n");
			break;
		}
	}

	return 0;
}


static int do_g2crypto_hashtest(int argc, char *argv[])
{
	int rc;
	unsigned char buf[16];
	unsigned char msg[] = "hello world";
	int cnt;
	char md5[] = {
		0x5e, 0xb6, 0x3b, 0xbb, 0xe0, 0x1e, 0xee, 0xd0,
		0x93, 0xcb, 0x22, 0xbb, 0x8f, 0x5a, 0xcd, 0xc3
	};

	spacc_device spacc;

	for (cnt = 0; cnt < 1000; ++cnt) {
		printf("cnt = %d\n", cnt);

		if (get_spacc_dev(&spacc, 0xF6600000) == NULL) {
			return 1;
		}

		memset(buf, 0, sizeof buf);
		spacc_hash(&spacc, CRYPTO_MODE_HASH_MD5,
			   (PDU_DMA_ADDR_T) msg, strlen(msg),
			   (PDU_DMA_ADDR_T) buf, sizeof(buf), 0, 0);

		rc = memcmp(buf, md5, sizeof(md5));
		if (rc != 0) {
			printf("md5 hash failed\n");
#ifdef DEBUG
			dump_results(buf, sizeof(buf));
#endif
			return 1;
		} else {
			printf("md5 hash correct\n");
		}

		spacc_fini(&spacc);

	}

	return 0;
}


struct g2crypto_cmd {
	const char *cmd;
	int argc;
	int (*cmd_process) (int argc, char *argv[]);
};

static int do_g2crypto(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	struct g2crypto_cmd cmds[] = {
		{"hash", 5, do_g2crypto_hash},
		{"hashtest", 2, do_g2crypto_hashtest},
		{"crypttest", 2, do_g2crypto_crypttest},
	};

	struct g2crypto_cmd *first = cmds;
	struct g2crypto_cmd *last = cmds + ARRAY_SIZE(cmds);;

	for (; first != last; ++first) {
		if (strcmp(first->cmd, argv[1]) == 0 && argc == first->argc)
			return first->cmd_process(argc - 2, argv + 2);
	}

	cmd_usage(cmdtp);
	return 1;
}

U_BOOT_CMD(g2crypto, 5, 1, do_g2crypto,
	   "hash/decrypt by hardware",
	   "--\n"
	   "hashtest\n"
	   "crypttest\n"
	   "hash algo plain plain_addr\n"
	   "\n"
	   "hash algo: md5 sha1 sha256 sha512\n"
	  );

