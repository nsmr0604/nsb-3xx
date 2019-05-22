#include <config.h>
#include <common.h>
#include <command.h>
#include <malloc.h>


/**
 *  Borrowed from common/cmd_nand.c
 */
static inline int str2long(char *p, ulong * num)
{
	char *endptr;

	*num = simple_strtoul(p, &endptr, 16);
	return (*p != '\0' && *endptr == '\0') ? 1 : 0;
}

static int do_g2_ecdsa(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	int rc;
	unsigned int plain, sig, pubk;
	unsigned int plainsz, sigsz, pubksz;

	if (argc != 6 && argc != 8) {
		printf("commands arguments wrong \n--\n");
		printf("ecdsa verify data_addr data_len sig_key sig_len key_addr key_len\n");
		printf("ecdsa otpkey data_addr data_len sig_key sig_len \n");

		return 1;
	}

	str2long(argv[2], &plain);
	str2long(argv[3], &plainsz);
	str2long(argv[4], &sig);
	str2long(argv[5], &sigsz);

	rc = 1;
	if (argc == 6 && strcmp(argv[1], "otpkey") == 0) {
		rc = do_g2_ecdsa_verify_by_otp(plain, plainsz, sig, sigsz);
	} else if (argc == 8 && strcmp(argv[1], "verify") == 0) {

		str2long(argv[6], &pubk);
		str2long(argv[7], &pubksz);

		rc = do_g2_ecdsa_verify(plain, plainsz, sig, sigsz, pubk,
					pubksz);
	}

	if (rc != 0) {
		printf("Verified failed %d\n", rc);
	}
	return rc;
}



U_BOOT_CMD(ecdsa, 8, 0, do_g2_ecdsa,
	   "ecdsa signataure check",
	   "---\n"
	   "ecdsa verify data_addr data_len sig_key sig_len key_addr key_len\n"
	   "ecdsa otpkey data_addr data_len sig_key sig_len \n"
	  );
