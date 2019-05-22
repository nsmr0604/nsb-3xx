/*
 *	cryptoLayer.h
 *	Release $Name: MATRIXSSL_1_2_4_OPEN $
 *
 *	Cryptography provider layered header.  This layer decouples
 *	the cryptography implementation from the SSL protocol implementation.
 *	Contributors adding new providers must implement all functions 
 *	externed below.
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

#ifndef _h_CRYPTO_LAYER
#define _h_CRYPTO_LAYER

#include "../matrixConfig.h"

/******************************************************************************/
/*
	Crypto may have some reliance on os layer (psMalloc in particular)
*/
#include "../os/osLayer.h"

/*
	Define the default crypto provider here
*/
#define	USE_PEERSEC_CRYPTO

#ifdef __cplusplus
extern "C" {
#endif

#define SSL_MD5_HASH_SIZE		16
#define SSL_SHA1_HASH_SIZE		20

#define SSL_MAX_MAC_SIZE		20
#define SSL_MAX_IV_SIZE			16
#define SSL_MAX_BLOCK_SIZE		16
#define SSL_MAX_SYM_KEY_SIZE	32
/*
	Enable the algorithms used for each cipher suite
*/
#ifdef USE_SSL_RSA_WITH_NULL_MD5
#define USE_RSA
#define USE_MD5_MAC
#endif

#ifdef USE_SSL_RSA_WITH_NULL_SHA
#define USE_RSA
#define USE_SHA1_MAC
#endif

#ifdef USE_SSL_RSA_WITH_RC4_128_SHA
#define USE_ARC4
#define USE_SHA1_MAC
#define USE_RSA
#endif

#ifdef USE_SSL_RSA_WITH_RC4_128_MD5
#define USE_ARC4
#define USE_MD5_MAC
#define USE_RSA
#endif

#ifdef USE_SSL_RSA_WITH_3DES_EDE_CBC_SHA
#define USE_3DES
#define USE_SHA1_MAC
#define USE_RSA
#endif

/*
	Support for optionally encrypted private key files. These are
	usually encrypted with 3DES.
*/
#ifdef USE_ENCRYPTED_PRIVATE_KEYS
#define USE_3DES
#endif

/*
	Support for client side SSL
*/
#ifdef USE_CLIENT_SIDE_SSL
#define USE_X509
#define USE_RSA_PUBLIC_ENCRYPT
#endif

/*
	Support for client authentication
*/

/*
	Misc support
*/
/* #define USE_FULL_CERT_PARSE */
/* #define USE_MD2 */
#define USE_AES

/*
	Now that we've set up the required defines, include the crypto layer header
*/
#ifdef USE_PEERSEC_CRYPTO
#include "peersec/pscrypto.h"
#endif

#ifdef USE_ARC4
extern void matrixArc4Init(sslCipherContext_t *ctx, unsigned char *key, int32 keylen);
extern int32 matrixArc4(sslCipherContext_t *ctx, unsigned char *in,
					  unsigned char *out, int32 len);
#endif /* USE_ARC4 */

#ifdef USE_3DES
extern int32 matrix3desInit(sslCipherContext_t *ctx, unsigned char *IV,
						  unsigned char *key, int32 keylen);
extern int32 matrix3desEncrypt(sslCipherContext_t *ctx, unsigned char *pt,\
							 unsigned char *ct, int32 len);
extern int32 matrix3desDecrypt(sslCipherContext_t *ctx, unsigned char *ct,
							 unsigned char *pt, int32 len);
#endif /* USE_3DES */

extern void matrixMd5Init(sslMd5Context_t *ctx);
extern void matrixMd5Update(sslMd5Context_t *ctx, const unsigned char *buf, unsigned long len);
extern int32 matrixMd5Final(sslMd5Context_t *ctx, unsigned char *hash);

#ifdef USE_MD2
/*
	MD2 is provided for compatibility with V2 and older X509 certificates,
	it is known to have security problems and should not be used for any current
	development.
*/
extern void matrixMd2Init(sslMd2Context_t *ctx);
extern int32 matrixMd2Update(sslMd2Context_t *ctx, const unsigned char *buf, unsigned long len);
extern int32 matrixMd2Final(sslMd2Context_t *ctx, unsigned char *hash);
#endif /* USE_MD2 */

extern void matrixSha1Init(sslSha1Context_t *ctx);
extern void matrixSha1Update(sslSha1Context_t *ctx, const unsigned char *buf, unsigned long len);
extern int32 matrixSha1Final(sslSha1Context_t *ctx, unsigned char *hash);

#ifdef USE_RSA
#ifdef USE_FILE_SYSTEM
extern int32 matrixRsaReadCert(char *fileName, unsigned char **out, int32 *outLen);
extern int32 matrixRsaReadPrivKey(char *fileName, char *password, sslRsaKey_t **key);
#endif /* USE_FILE_SYSTEM */

extern int32 matrixRsaReadPrivKeyMem(char *keyBuf, int32 keyBufLen, char *password, sslRsaKey_t **key);
extern int32 matrixRsaReadCertMem(char *certBuf, int32 certLen, unsigned char **out, int32 *outLen);

extern void matrixRsaFreeKey(sslRsaKey_t *key);

extern int32 matrixRsaEncryptPub(psPool_t *pool, sslRsaKey_t *key, 
							   unsigned char *in, int32 inlen,
							   unsigned char *out, int32 outlen);
extern int32 matrixRsaDecryptPriv(psPool_t *pool, sslRsaKey_t *key, 
								unsigned char *in, int32 inlen,
								unsigned char *out, int32 outlen);
extern int32 matrixRsaEncryptPriv(psPool_t *pool, sslRsaKey_t *key, 
								unsigned char *in, int32 inlen,
								unsigned char *out, int32 outlen);
extern int32 matrixRsaDecryptPub(psPool_t *pool, sslRsaKey_t *key, 
							   unsigned char *in, int32 inlen,
							   unsigned char *out, int32 outlen);


#endif /* USE_RSA */

#ifdef USE_AES
extern int32 matrixAesInit(sslCipherContext_t *ctx, unsigned char *IV,
						 unsigned char *key, int32 keylen);
extern int32 matrixAesEncrypt(sslCipherContext_t *ctx, unsigned char *pt,
							unsigned char *ct, int32 len);
extern int32 matrixAesDecrypt(sslCipherContext_t *ctx, unsigned char *ct,
							unsigned char *pt, int32 len);
#endif /* USE_AES */

/*
	Any change to these cert structs must be reflected in
	matrixSsl.h for public use
*/
typedef struct {
	char	*country;
	char	*state;
	char	*locality;
	char	*organization;
	char	*orgUnit;
	char	*commonName;
} sslDistinguishedName_t;

typedef struct {
	char	*dns;
	char	*uri;
	char	*email;
} sslSubjectAltName_t;

typedef struct sslCertInfo {
	int32						verified;
	unsigned char			*serialNumber;
	int32						serialNumberLen;
	char					*notBefore;
	char					*notAfter;
	char					*sigHash;
	int32						sigHashLen;
	sslSubjectAltName_t		subjectAltName;
	sslDistinguishedName_t	subject;
	sslDistinguishedName_t	issuer;
	struct sslCertInfo		*next;
} sslCertInfo_t;

#ifdef USE_X509

extern int32 matrixX509ParseCert(psPool_t *pool, unsigned char *certBuf, 
							   int32 certlen, sslRsaCert_t **cert);
extern void matrixX509FreeCert(sslRsaCert_t *cert);
extern int32 matrixX509ValidateCert(psPool_t *pool, sslRsaCert_t *subjectCert,
								  sslRsaCert_t *issuerCert);
extern int32 matrixX509ValidateChain(psPool_t *pool, sslRsaCert_t *chain,
								   sslRsaCert_t **subjectCert);
extern int32 matrixX509UserValidator(psPool_t *pool, 
						sslRsaCert_t *subjectCert,
						int32 (*certValidator)(sslCertInfo_t *t, void *arg),
						void *arg);
#endif /* USE_X509 */

#ifdef __cplusplus
   }
#endif

#endif /* _h_CRYPTO_LAYER */

/******************************************************************************/

