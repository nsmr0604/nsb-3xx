/*
 *	rsa.c
 *	Release $Name: MATRIXSSL_1_2_4_OPEN $
 *
 *	RSA key and cert reading, RSA padding and RSA math wrappers
 */
/*
 *	Copyright (c) PeerSec Networks, 2002-2005. All Rights Reserved.
 *	The latest version of this code is available at http://www.matrixssl.org
 *
 *	This software is open source; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This General Public License does NOT permit incorporating this software 
 *	into proprietary programs.  If you are unable to comply with the GPL, a 
 *	commercial license for this software may be purchased from PeerSec Networks
 *	at http://www.peersec.com
 *	
 *	This program is distributed in WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	See the GNU General Public License for more details.
 *	
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *	http://www.gnu.org/copyleft/gpl.html
 */
/******************************************************************************/

#ifdef VXWORKS
#include <vxWorks.h>
#endif /* VXWORKS */

#ifndef WINCE
	#include <sys/stat.h>
	#include <signal.h>
#endif /* WINCE */

#include "../cryptoLayer.h"

/******************************************************************************/

static int32 ssl_rsa_crypt(psPool_t *pool, 
						const unsigned char *in, uint32 inlen,
						unsigned char *out, uint32 *outlen, 
						sslRsaKey_t *key, int32 type);

static int32 sslUnpadRSA(unsigned char *in, int32 inlen, unsigned char *out, 
						int32 outlen, int32 decryptType);
static int32 sslPadRSA(unsigned char *in, int32 inlen, unsigned char *out,
						int32 outlen, int32 cryptType);

#ifdef USE_X509
static int32 matrixX509ValidateCertInternal(psPool_t *pool, 
						sslRsaCert_t *subjectCert, 
						sslRsaCert_t *issuerCert, int32 chain);
#endif /* USE_X509 */

#ifdef USE_3DES
static int32 hexToBinary(unsigned char *hex, unsigned char *bin, int32 binlen);
static void generate3DESKey(unsigned char *pass, int32 passlen, 
							unsigned char *salt, unsigned char *key);
#endif /* USE_3DES */

#ifdef USE_RSA_BLINDING
static int32 tim_mp_exptmod(psPool_t *pool, 
						mp_int *c, mp_int *e, mp_int *d, mp_int *n, mp_int *m);
#else
#define tim_mp_exptmod(p, c, e, d, n, m) mp_exptmod(p, c, d, n, m)
#endif

/******************************************************************************/

#define RSA_PUBLIC		0x01
#define RSA_PRIVATE		0x02

#ifdef USE_3DES
static const char encryptHeader[] = "DEK-Info: DES-EDE3-CBC,";
#endif

#ifdef USE_FILE_SYSTEM
/******************************************************************************/
/*
	Return the file contents given a file name in a single allocated buffer.
	Not a good routine to use generally with the fixed mem stuff.
*/
static int32 getFileBin(char *fileName, unsigned char **bin, int32 *binLen)
{
	FILE	*fp;
	struct	stat	fstat;
	size_t	tmp = 0;

	*binLen = 0;
	*bin = NULL;

	if (fileName == NULL) {
		return -1;
	}

	if ((stat(fileName, &fstat) != 0) || (fp = fopen(fileName, "rb")) == NULL) {
		return -7; /* FILE_NOT_FOUND */
	}

	*bin = psMalloc(NULL, fstat.st_size);
	if (*bin == NULL) {
		return -8; /* SSL_MEM_ERROR */
	}
	while (((tmp = fread(*bin + *binLen, sizeof(char), 512, fp)) > 0) &&
			(*binLen < fstat.st_size)) { 
		*binLen += (int32)tmp;
	}
	fclose(fp);
	return 0;
}

/******************************************************************************/
/*
 *	Public API to return a binary buffer from a cert.  Suitable to send
 *	over the wire.  Caller must free 'out' if this function returns success (0)
 *	Parse .pem files according to http://www.faqs.org/rfcs/rfc1421.html
 */
int32 matrixRsaReadCert(char *fileName, unsigned char **out, int32 *outLen)
{
	int32			certBufLen, rc;
	unsigned char	*certBuf;

	if (fileName == NULL) {
		return 0;
	}
	if ((rc = getFileBin(fileName, &certBuf, &certBufLen)) < 0) {
		return rc;
	}
	
	rc = matrixRsaReadCertMem((char*)certBuf, certBufLen, out, outLen);
	psFree(certBuf);

	return rc;
}

/******************************************************************************/
/*
 *	Public API to return an RSA key from a PEM private key file
 *	http://www.faqs.org/rfcs/rfc1421.html
 *
 *	If password is provided, we only deal with 3des cbc encryption
 *	Function allocates key on success.  User must free.
 */
int32 matrixRsaReadPrivKey(char *fileName, char *password, sslRsaKey_t **key)
{
	unsigned char	*keyBuf;
	int32			keyBufLen, rc;

	if (fileName == NULL) {
		return 0;
	}
	if ((rc = getFileBin(fileName, &keyBuf, &keyBufLen)) < 0) {
		return rc;
	}
	
	rc = matrixRsaReadPrivKeyMem((char*)keyBuf, keyBufLen, password, key);
	psFree(keyBuf);
	return rc;
}
#endif /* USE_FILE_SYSTEM */

/******************************************************************************/
/*
 *	In memory version of matrixRsaReadCert.
 */
int32 matrixRsaReadCertMem(char *certBuf, int32 certLen, unsigned char **out,
						 int32 *outLen)
{
	char	*start, *end;

	if (certBuf == NULL) {
		return 0;
	}

	if (((start = strstr(certBuf, "-----BEGIN")) != NULL) && 
		((start = strstr(certBuf, "CERTIFICATE-----")) != NULL) &&
		(end = strstr(start, "-----END")) != NULL) {
		start += strlen("CERTIFICATE-----");
		certLen = (int32)(end - start);
	} else {
		return -1;
	}
	*out = psMalloc(NULL, certLen);
	if (*out == NULL) {
		return -8;  /* SSL_MEM_ERROR */
	}
	memset(*out, '\0', certLen);
	*outLen = certLen;

	if (ps_base64_decode((unsigned char*)start, certLen, *out, outLen) != 0) {
		psFree(*out);
		matrixStrDebugMsg("Unable to base64 decode certificate\n", NULL);
		return -1;
	}
	return 0;
}

/******************************************************************************/
/*
 *	In memory version of matrixRsaReadPrivKeyMem.
 */
int32 matrixRsaReadPrivKeyMem(char *keyBuf, int32 keyBufLen, char *password, 
		sslRsaKey_t **key)
{
	unsigned char	*DERout, *asnp;
	char			*start, *end;
	int32			DERlen, ret, PEMlen = 0;
#ifdef USE_3DES
	sslCipherContext_t	ctx;
	unsigned char	passKey[SSL_DES3_KEY_LEN];
	unsigned char	cipherIV[SSL_DES3_IV_LEN];
	int32				tmp;
#endif /* USE_3DES */

	if (keyBuf == NULL) {
		return 0;
	}
	start = end = NULL;

/*
 *	Check header and encryption parameters.
 */
	if ((start = strstr(keyBuf, "-----BEGIN RSA PRIVATE KEY-----")) == NULL) {
		matrixStrDebugMsg("Error parsing private key buffer\n", NULL);
		return -1;
	}
	start += strlen("-----BEGIN RSA PRIVATE KEY-----");
	while (*start == '\r' || *start == '\n') {
		start++;
	}

/*
	FUTURE - We should do this in reverse order.  If we see the ENCRYPTED
	tag, we should check / prompt for password.
*/
	if (password) {
#ifdef USE_3DES
		if ((strstr(keyBuf, "Proc-Type:") == NULL) || 
			(strstr(keyBuf, "4,ENCRYPTED") == NULL)) {
			matrixStrDebugMsg("Unrecognized private key file encoding\n", NULL);
			return -1;
		}
		if ((start = strstr(keyBuf, encryptHeader)) == NULL) {
			matrixStrDebugMsg("Unrecognized private key file encoding\n", NULL);
			return -1;
		}
		start += strlen(encryptHeader);
		/* SECURITY - we assume here that header points to at least 16 bytes of data */
		tmp = hexToBinary((unsigned char*)start, cipherIV, SSL_DES3_IV_LEN);
		if (tmp < 0) {
			matrixStrDebugMsg("Invalid private key file salt\n", NULL);
			return -1;
		}
		start += tmp;
		generate3DESKey((unsigned char*)password, (int32)strlen(password),
			cipherIV, (unsigned char*)passKey);
#else  /* !USE_3DES */
/*
 *		The private key is encrypted, but 3DES support has been turned off
 */
		matrixStrDebugMsg("3DES has been disabled for private key decrypt\n", NULL);
		return -1;  
#endif /* USE_3DES */
	}
	ret = 0;
	if ((end = strstr(keyBuf, "-----END RSA PRIVATE KEY-----")) == NULL) {
		matrixStrDebugMsg("Error parsing private key buffer\n", NULL);
		return -1;
	}
	PEMlen = (int32)(end - start);

/*
	Take the raw input and do a base64 decode
 */
	DERout = psMalloc(NULL, PEMlen);
	if (DERout == NULL) {
		return -8; /* SSL_MEM_ERROR */
	}
	DERlen = PEMlen;
	if (ps_base64_decode((unsigned char*)start, PEMlen, DERout, &DERlen) != 0) {
		psFree(DERout);
		matrixStrDebugMsg("Unable to base64 decode private key\n", NULL);
		return -1;
	}

#ifdef USE_3DES
/*
 *	Decode
 */
	if (password) {
		matrix3desInit(&ctx, cipherIV, passKey, SSL_DES3_KEY_LEN);
		matrix3desDecrypt(&ctx, DERout, DERout, DERlen);
	}
#endif /* USE_3DES */

/*
	Now have the DER stream to extract from in asnp
 */
	*key = psMalloc(NULL, sizeof(sslRsaKey_t));
	if (*key == NULL) {
		psFree(DERout);
		return -8; /* SSL_MEM_ERROR */
	}
	memset(*key, 0x0, sizeof(sslRsaKey_t));

	asnp = DERout;
	if (psAsnParsePrivateKey(NULL, &asnp, DERlen, *key) < 0) {
		matrixRsaFreeKey(*key);
		*key = NULL;
		memset(DERout, 0x0, PEMlen);
		psFree(DERout);
		if (password) {
			matrixStrDebugMsg(
				"Unable to ASN parse private key; password may be incorrect.\n",
				NULL);
		} else {
			matrixStrDebugMsg("Unable to ASN parse private key.\n", NULL);
		}
		return -1;
	}
	memset(DERout, 0x0, PEMlen);
	psFree(DERout);
	return 0;
}

#ifdef USE_3DES
/******************************************************************************/
/*
	Convert an ASCII hex representation to a binary buffer.
	Decode enough data out of 'hex' buffer to produce 'binlen' bytes in 'bin'
	Two digits of ASCII hex map to the high and low nybbles (in that order),
	so this function assumes that 'hex' points to 2x 'binlen' bytes of data.
	Return the number of bytes processed from hex (2x binlen) or < 0 on error.
*/
static int32 hexToBinary(unsigned char *hex, unsigned char *bin, int32 binlen)
{
	unsigned char	*end, c, highOrder;

	highOrder = 1;
	for (end = hex + binlen * 2; hex < end; hex++) {
		c = *hex;
		if ('0' <= c && c <='9') {
			c -= '0';
		} else if ('a' <= c && c <='f') {
			c -= ('a' - 10);
		} else if ('A' <= c && c <='F') {
			c -= ('A' - 10);
		} else {
			return -1;
		}
		if (highOrder++ & 0x1) {
			*bin = c << 4;
		} else {
			*bin |= c;
			bin++;
		}
	}
	return binlen * 2;
}

/******************************************************************************/
/*
	Generate a 3DES key given a password and salt value.
	We use PKCS#5 2.0 PBKDF1 key derivation format with MD5 and count == 1 per:
	http://www.rsasecurity.com/rsalabs/pkcs/pkcs-5/index.html

	This key is compatible with the algorithm used by OpenSSL to encrypt keys
	generated with 'openssl genrsa'.  If other encryption formats are used
	(for example PBKDF2), or an iteration count > 0 is used, they are not 
	compatible with this simple implementation.  OpenSSL provides many options
	for converting key formats to the one used here.

	A 3DES key is 24 bytes long, to generate it with this algorithm,
	we md5 hash the password and salt for the first 16 bytes.  We then 
	hash these first 16 bytes with the password and salt again, generating 
	another 16 bytes.  We take the first 16 bytes and 8 of the second 16 to 
	form the 24 byte key.

	salt is assumed to point to 8 bytes of data
	key is assumed to point to 24 bytes of data
*/
static void generate3DESKey(unsigned char *pass, int32 passlen, unsigned char *salt, 
					unsigned char *key)
{
	sslMd5Context_t		state;
	unsigned char		md5[SSL_MD5_HASH_SIZE];

	matrixMd5Init(&state);
	matrixMd5Update(&state, pass, passlen);
	matrixMd5Update(&state, salt, SSL_DES3_IV_LEN);
	matrixMd5Final(&state, md5);
	memcpy(key, md5, SSL_MD5_HASH_SIZE);

	matrixMd5Init(&state);
	matrixMd5Update(&state, md5, SSL_MD5_HASH_SIZE);
	matrixMd5Update(&state, pass, passlen);
	matrixMd5Update(&state, salt, SSL_DES3_IV_LEN);
	matrixMd5Final(&state, md5);
	memcpy(key + SSL_MD5_HASH_SIZE, md5, SSL_DES3_KEY_LEN - SSL_MD5_HASH_SIZE);
}
#endif /* USE_3DES */


/******************************************************************************/

static int32 ssl_rsa_crypt(psPool_t *pool, 
						const unsigned char *in, uint32 inlen,
						unsigned char *out, uint32 *outlen, 
						sslRsaKey_t *key, int32 type)
{
	mp_int			tmp, tmpa, tmpb;
	unsigned long	x;
	int32			res;

	if (in == NULL || out == NULL || outlen == NULL || key == NULL) {
		return -1;
	}

/*
	init and copy into tmp
 */
	if (_mp_init_multi(pool, &tmp, &tmpa, &tmpb, NULL, NULL, NULL, NULL, NULL)
		!= MP_OKAY) {
		matrixStrDebugMsg("ssl_rsa_crypt error: mp_init_multi\n", NULL);
		goto error;
	}
	if (mp_read_unsigned_bin(&tmp, (unsigned char *)in, (int32)inlen) != 
			MP_OKAY) {
		matrixStrDebugMsg("ssl_rsa_crypt error: mp_read_unsigned_bin\n", NULL);
		goto error; 
	}
/*
	sanity check on the input
 */
	if (mp_cmp(&key->N, &tmp) == MP_LT) {
		res = -1;
		goto done;
	}
	if (type == RSA_PRIVATE) {
		if (key->optimized) {
			if (tim_mp_exptmod(pool, &tmp, &key->e, &key->dP, &key->p, &tmpa) != MP_OKAY) {
				matrixStrDebugMsg("decrypt error: mp_exptmod dP, p\n", NULL);
				goto error;
			}
			if (tim_mp_exptmod(pool, &tmp, &key->e, &key->dQ, &key->q, &tmpb) != MP_OKAY) { 
				matrixStrDebugMsg("decrypt error: mp_exptmod dQ, q\n", NULL);
				goto error;
			}
			if (mp_mul(pool, &tmpa, &key->qP, &tmpa) != MP_OKAY) {
				matrixStrDebugMsg("decrypt error: mp_mul qP \n", NULL);
				goto error;
			}
			if (mp_mul(pool, &tmpb, &key->pQ, &tmpb) != MP_OKAY) {
				matrixStrDebugMsg("decrypt error: mp_mul pQ\n", NULL);
				goto error;
			}
			if (mp_addmod(pool, &tmpa, &tmpb, &key->N, &tmp) != MP_OKAY) {
				matrixStrDebugMsg("decrypt error: mp_addmod N\n", NULL);
				goto error;
			}
		} else {
			if (tim_mp_exptmod(pool, &tmp, &key->e, &key->d, &key->N, &tmp) != MP_OKAY) {
				matrixStrDebugMsg("ssl_rsa_crypt error: mp_exptmod\n", NULL);
				goto error;
			}
		}
	} else if (type == RSA_PUBLIC) {
		if (mp_exptmod(pool, &tmp, &key->e, &key->N, &tmp) != MP_OKAY) {
			matrixStrDebugMsg("ssl_rsa_crypt error: mp_exptmod\n", NULL);
			goto error;
		}
	} else {
		matrixStrDebugMsg("ssl_rsa_crypt error: invalid type param\n", NULL);
		goto error;
	}
/*
	read it back
 */
	x = (unsigned long)mp_unsigned_bin_size(&tmp);
	if (x > *outlen) {
		res = -1;
		matrixStrDebugMsg("ssl_rsa_crypt error: mp_unsigned_bin_size\n", NULL);
		goto done;
	}
/*
	We want the encrypted value to always be the key size.  Pad with 0x0
*/
	while (x < (unsigned long)key->size) {
		*out++ = 0x0;
		x++;
	}

	*outlen = x;
/*
	convert it
 */
	if (mp_to_unsigned_bin(pool, &tmp, out) != MP_OKAY) {
		matrixStrDebugMsg("ssl_rsa_crypt error: mp_to_unsigned_bin\n", NULL);
		goto error;
	}
/*
	clean up and return
 */
	res = 0;
	goto done;
error:
	res = -1;
done:
	_mp_clear_multi(&tmp, &tmpa, &tmpb, NULL, NULL, NULL, NULL, NULL);
	return res;
}

/******************************************************************************/
/*
	Pad a value to be encrypted by RSA, according to PKCS#1 v1.5
	http://www.rsasecurity.com/rsalabs/pkcs/pkcs-1/
	When encrypting a value with RSA, the value is first padded to be 
	equal to the public key size using the following method:
		00 <id> <data> 00 <value to be encrypted>
	- id denotes a public or private key operation
	- if id is private, data is however many non-zero bytes it takes to pad the
		value to the key length (randomLen = keyLen - 3 - valueLen).
	- if id is public, data is FF for the same length as described above
	- There must be at least 8 bytes of data.
*/
static int32 sslPadRSA(unsigned char *in, int32 inlen, unsigned char *out,
			int32 outlen, int32 cryptType)
{
	unsigned char	*c;
	int32			randomLen;
	
	randomLen = outlen - 3 - inlen;
	if (randomLen < 8) {
		matrixIntDebugMsg("RSA encryption data too large: %d\n", inlen);
		return -1;
	}
	c = out;
	*c = 0x00;
	c++;
	*c = cryptType;
	c++;
	if (cryptType == RSA_PUBLIC) {
		while (randomLen-- > 0) {
			*c++ = 0xFF;
		}
	} else {
		if (sslGetEntropy(c, randomLen) < 0) {
			matrixStrDebugMsg("Error gathering RSA pad entropy\n", NULL);
			return -1;
		}
/*
		SECURITY:  Read through the random data and change all 0x0 to 0x01.
		This is necessary to ensure the Unpad on decryption doesn't falsely
		stop walking the random number on 0x0
*/
		while (randomLen-- > 0) {
			if (*c == 0x0) {
				*c = 0x01;
			}
			c++;
		}
	}
	*c = 0x00;
	c++;
	memcpy(c, in, inlen);
	
	return outlen;
}

#ifdef USE_RSA_PRIVATE_ENCRYPT
/******************************************************************************/
/*
	RSA private encryption.  
	Called by client SSL for client authentication in CertificateVerify
	The outlen param must be set to the strength of the key:  key->size
*/
int32 matrixRsaEncryptPriv(psPool_t *pool, sslRsaKey_t *key, 
						 unsigned char *in, int32 inlen,
						 unsigned char *out, int32 outlen)
{
	int32	size;

	size = key->size;
	if (outlen < size) {
		return -1;
	}
	if (sslPadRSA(in, inlen, out, size, RSA_PUBLIC) < 0) {
		return -1;
	}
	if (ssl_rsa_crypt(pool, out, size, out, &outlen, key, RSA_PRIVATE) < 0 ||
			outlen != size) {
		return -1;
	}
	return size;
}
#endif /* USE_RSA_PRIVATE_ENCRYPT */

#ifdef USE_RSA_PUBLIC_ENCRYPT
/******************************************************************************/
/*
	RSA public encryption.
	Always called by SSL client for server auth in ClientKeyExchange
	The outlen param must be set to the strength of the key:  key->size
*/
int32 matrixRsaEncryptPub(psPool_t *pool, sslRsaKey_t *key, 
						unsigned char *in, int32 inlen,
						unsigned char *out, int32 outlen)
{
	int32	size;

	size = key->size;
	if (outlen < size) {
		return -1;
	}
	if (sslPadRSA(in, inlen, out, size, RSA_PRIVATE) < 0) {
		return -1;
	}
	if (ssl_rsa_crypt(pool, out, size, out, &outlen, key, RSA_PUBLIC) < 0 ||
			outlen != size) {
		return -1;
	}
	return size;
}

#else  /* USE_RSA_PUBLIC_ENCRYPT - Keeps the cipher suite definition clean */
int32 matrixRsaEncryptPub(psPool_t *pool, sslRsaKey_t *key, 
						unsigned char *in, int32 inlen,
						unsigned char *out, int32 outlen)
{
	if (inlen > outlen) {
		return -1;
	}
	memcpy(out, in, inlen);
	return inlen;
}
#endif /* USE_RSA_PUBLIC_ENCRYPT */

/******************************************************************************/
/*
	Unpad a value decrypted by RSA, according to PKCS#1 v1.5
	http://www.rsasecurity.com/rsalabs/pkcs/pkcs-1/
	
	When decrypted, the data will look like the pad, including the inital
	byte (00).  Form:
		00 <decryptType> <random data (min 8 bytes)> 00 <value to be encrypted>

	We don't worry about v2 rollback issues because we don't support v2
*/
static int32 sslUnpadRSA(unsigned char *in, int32 inlen, unsigned char *out, 
					   int32 outlen, int32 decryptType)
{
	unsigned char	*c, *end;

	if (inlen < outlen + 10) {
		return -1;
	}
	c = in;
	end = in + inlen;
/*
	Verify the first byte (block type) is correct.
*/
	if (*c++ != 0x00 || *c != decryptType) {
		return -1;
	}
	c++;
/*
	Skip over the random, non-zero bytes used as padding
*/
	while (c < end && *c != 0x0) {
		if (decryptType == RSA_PUBLIC) {
			if (*c != 0xFF) {
				return -1;
			}
		}
		c++;
	}
	c++;
/*
	The length of the remaining data should be equal to what was expected
	Combined with the initial length check, there must be >= 8 bytes of pad
	ftp://ftp.rsa.com/pub/pdfs/bulletn7.pdf
*/
	if (end - c != outlen) {
		return -1;
	}
/*
	Copy the value bytes to the out buffer
*/
	while (c < end) {
		*out = *c;
		out++; c++;
	}
	return outlen;
}

/******************************************************************************/
/*
	Always called by the server to decrypt the ClientKeyExchange
*/
int32 matrixRsaDecryptPriv(psPool_t *pool, sslRsaKey_t *key, 
						 unsigned char *in, int32 inlen,
						 unsigned char *out, int32 outlen)
{
	int32		ptLen;

	if (inlen != key->size) {
		return -1;
	}
	ptLen = inlen;
	if (ssl_rsa_crypt(pool, in, inlen, in, &ptLen, key, RSA_PRIVATE) < 0 || 
			ptLen != inlen) {
		return -1;
	}
	ptLen = sslUnpadRSA(in, inlen, out, outlen, RSA_PRIVATE);
	memset(in, 0x0, inlen);
	return ptLen;
}

/*
	Called by the server if authenticating client in CertificateVerify
*/
int32 matrixRsaDecryptPub(psPool_t *pool, sslRsaKey_t *key, 
						unsigned char *in, int32 inlen,
						unsigned char *out,	int32 outlen)
{
	int32		ptLen;

	if (inlen != key->size) {
		return -1;
	}
	ptLen = inlen;
	if (ssl_rsa_crypt(pool, in, inlen, in, &ptLen, key, RSA_PUBLIC) < 0 ||
			ptLen != inlen) {
		return -1;
	}
	ptLen = sslUnpadRSA(in, inlen, out, outlen, RSA_PUBLIC);
	return 0;
}


#ifdef USE_X509
/******************************************************************************/
/*
	Walk through the certificate chain and validate it.  Return the final
	member of the chain as the subjectCert that can then be validated against
	the CAs
*/
int32 matrixX509ValidateChain(psPool_t *pool, sslRsaCert_t *chain, 
							sslRsaCert_t **subjectCert)
{
	sslRsaCert_t	*ic;

	*subjectCert = chain;
	while ((*subjectCert)->next != NULL) {
		ic = (*subjectCert)->next;
		if (matrixX509ValidateCertInternal(pool, *subjectCert, ic, 1) < 0) {
			return -1;
		}
		*subjectCert = (*subjectCert)->next;
	}
	return 0;
}

/******************************************************************************/
/*
	A signature validation for certificates.  -1 return is an error.  The success
	of the validation is returned in the 'valid' param of the subjectCert.
	1 if the issuerCert	signed the subject cert. -1 if not
*/
int32 matrixX509ValidateCert(psPool_t *pool, sslRsaCert_t *subjectCert, 
						   sslRsaCert_t *issuerCert)
{
	return matrixX509ValidateCertInternal(pool, subjectCert, issuerCert, 0);
}

static int32 matrixX509ValidateCertInternal(psPool_t *pool, sslRsaCert_t *subjectCert, 
						   sslRsaCert_t *issuerCert, int32 chain)
{
	sslRsaCert_t	*ic;
	unsigned char	sigOut[10 + SSL_SHA1_HASH_SIZE + 5];	/* See below */
	int32			sigLen;

	subjectCert->valid = -1;
/*
	Supporting a one level chain or a self-signed cert.  If the issuer
	is NULL, the self-signed test is done.
*/
	if (issuerCert == NULL) {
		matrixStrDebugMsg("Warning:  No CA to validate cert with\n", NULL);
		matrixStrDebugMsg("\tPerforming self-signed CA test\n", NULL);
		ic = subjectCert;
	} else {
		ic = issuerCert;
	}
/*
	Path confirmation.	If this is a chain verification, do not allow
	any holes in the path.  Error out if issuer does not have CA permissions
	or if hashes do not match anywhere along the way.
*/
	while (ic) {
		if (subjectCert != ic) {
/*
			Certificate authority constraint only available in version 3 certs
*/
			if ((ic->version > 1) && (ic->extensions.bc.ca <= 0)) {
				if (chain) {
					return -1;
				}
				ic = ic->next;
				continue;
			}
/*
			Use sha1 hash of issuer fields computed at parse time to compare
*/
			if (memcmp(subjectCert->issuer.hash, ic->subject.hash,
					SSL_SHA1_HASH_SIZE) != 0) {
				if (chain) {
					return -1;
				}
				ic = ic->next;
				continue;
			}
		}
/*
		Signature confirmation
		The sigLen is the ASN.1 size in bytes for encoding the hash.
		The magic 10 is comprised of the SEQUENCE and ALGORITHM ID overhead.
		The magic 8 and 5 are the OID lengths of the corresponding algorithm.
		NOTE: if sigLen is modified, above sigOut static size must be changed
*/
		if (subjectCert->sigAlgorithm ==  OID_RSA_MD5 ||
				subjectCert->sigAlgorithm == OID_RSA_MD2) {
			sigLen = 10 + SSL_MD5_HASH_SIZE + 8;	/* See above */
		} else if (subjectCert->sigAlgorithm == OID_RSA_SHA1) {
			sigLen = 10 + SSL_SHA1_HASH_SIZE + 5;	/* See above */
		} else {
			matrixStrDebugMsg("Unsupported signature algorithm\n", NULL);
			return -1;
		}
		sslAssert(sigLen <= sizeof(sigOut));
		matrixRsaDecryptPub(pool, &(ic->publicKey), subjectCert->signature,
			subjectCert->signatureLen, sigOut, sigLen);
/*
		If this is a chain test, fail on any gaps in the chain
*/
		if (psAsnConfirmSignature(subjectCert->sigHash, sigOut, sigLen) < 0) {
			if (chain) {
				return -1;
			}
			ic = ic->next;
			continue;
		}
/*
		Fall through to here only if passed signature check.
*/
		subjectCert->valid = 1;
		break;
	}
	return 0;
}

/******************************************************************************/
/*
	Calls a user defined callback to allow for manual validation of the
	certificate.
*/
int32 matrixX509UserValidator(psPool_t *pool, sslRsaCert_t *subjectCert,
			int32 (*certValidator)(sslCertInfo_t *t, void *arg), void *arg)
{
	sslCertInfo_t	*cert, *current, *next;
	int32			rc;

	if (certValidator == NULL) {
		return 0;
	}
/*
	Pass the entire certificate chain to the user callback.
*/
	current = cert = psMalloc(pool, sizeof(sslCertInfo_t));
	if (current == NULL) {
		return -8; /* SSL_MEM_ERROR */
	}
	memset(cert, 0x0, sizeof(sslCertInfo_t));
	while (subjectCert) {
		
		current->issuer.commonName = subjectCert->issuer.commonName;
		current->issuer.country = subjectCert->issuer.country;
		current->issuer.locality = subjectCert->issuer.locality;
		current->issuer.organization = subjectCert->issuer.organization;
		current->issuer.orgUnit = subjectCert->issuer.orgUnit;
		current->issuer.state = subjectCert->issuer.state;

		current->subject.commonName = subjectCert->subject.commonName;
		current->subject.country = subjectCert->subject.country;
		current->subject.locality = subjectCert->subject.locality;
		current->subject.organization = subjectCert->subject.organization;
		current->subject.orgUnit = subjectCert->subject.orgUnit;
		current->subject.state = subjectCert->subject.state;

		current->serialNumber = subjectCert->serialNumber;
		current->serialNumberLen = subjectCert->serialNumberLen;
		current->verified = subjectCert->valid;
		current->notBefore = subjectCert->notBefore;
		current->notAfter = subjectCert->notAfter;

		current->subjectAltName.dns = (char*)subjectCert->extensions.san.dns;
		current->subjectAltName.uri = (char*)subjectCert->extensions.san.uri;
		current->subjectAltName.email = (char*)subjectCert->extensions.san.email;
	
		if (subjectCert->certAlgorithm == OID_RSA_MD5 ||
				subjectCert->certAlgorithm == OID_RSA_MD2) {
			current->sigHashLen = SSL_MD5_HASH_SIZE;
		} else if (subjectCert->certAlgorithm == OID_RSA_SHA1) {
			current->sigHashLen = SSL_SHA1_HASH_SIZE;
		}
		current->sigHash = (char*)subjectCert->sigHash;
		if (subjectCert->next) {
			next = psMalloc(pool, sizeof(sslCertInfo_t));
			if (next == NULL) {
				while (cert) {
					next = cert->next;
					psFree(cert);
					cert = next;
				}
				return -8; /* SSL_MEM_ERROR */
			}
			memset(next, 0x0, sizeof(sslCertInfo_t));
			current->next = next;
			current = next;
		}
		subjectCert = subjectCert->next;
	}
/*
	The user callback
*/
	rc = certValidator(cert, arg);
/*
	Free the chain
*/
	while (cert) {
		next = cert->next;
		psFree(cert);
		cert = next;
	}
	return rc;
}
#endif /* USE_X509 */

/******************************************************************************/
/*
 *	Free an RSA key.  mp_clear will zero the memory of each element and free it.
 */

void matrixRsaFreeKey(sslRsaKey_t *key)
{
	mp_clear(&(key->N));
	mp_clear(&(key->e));
	mp_clear(&(key->d));
	mp_clear(&(key->p));
	mp_clear(&(key->q));
	mp_clear(&(key->dP));
	mp_clear(&(key->dQ));
	mp_clear(&(key->qP));
	mp_clear(&(key->pQ));
	psFree(key);
}

#ifdef USE_RSA_BLINDING

static int32 tim_mp_exptmod(psPool_t *pool, 
						  mp_int *c, mp_int *e, mp_int *d, mp_int *n, mp_int *m)
{
	int32			err;
	mp_int			r, tmp, tmp2;
	unsigned char	*rtmp;
	unsigned long	rlen;
/*
	pick random r
 */
	rlen = mp_unsigned_bin_size(n);

	rtmp = psMalloc(pool, rlen);
	if (rtmp == NULL) {
		return -8; /* SSL_MEM_ERROR */
	}
	sslGetEntropy(rtmp, rlen);

	if ((err = _mp_init_multi(pool, &r, &tmp, &tmp2, NULL, NULL, NULL, NULL,
			NULL)) != MP_OKAY) {
		psFree(rtmp);
		return -1;
	}
/*
	read in r
 */
	if ((err = mp_read_unsigned_bin(&r, rtmp, rlen)) != MP_OKAY) {
		goto __ERR;
	}
/*
	compute tmp = r^e
 */
	if ((err = mp_exptmod(pool, &r, e, n, &tmp)) != MP_OKAY) {
		goto __ERR;
	}
/*
	multiply C into the mix
 */
	if ((err = mp_mulmod(pool, c, &tmp, n, &tmp)) != MP_OKAY) {
		goto __ERR;
	}
/*
	raise to d
 */
	if ((err = mp_exptmod(pool, &tmp, d, n, &tmp)) != MP_OKAY) {
		goto __ERR;
	}
/*
	invert r and multiply
 */
	if ((err = mp_invmod(pool, &r, n, &tmp2)) != MP_OKAY) {
		goto __ERR;
	}
/*
	multiply and we are totally set
 */
	if ((err = mp_mulmod(pool, &tmp, &tmp2, n, m)) != MP_OKAY) {
		goto __ERR;
	}

__ERR:  _mp_clear_multi(&r, &tmp, &tmp2, NULL, NULL, NULL, NULL, NULL);
	psFree(rtmp);
	return err;
}
#endif /* USE_RSA_BLINDING */

/******************************************************************************/


