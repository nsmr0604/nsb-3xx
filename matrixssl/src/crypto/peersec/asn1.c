/*
 *	asn1.c
 *	Release $Name: MATRIXSSL_1_2_4_OPEN $
 *
 *	DER/BER coding
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

#include "../cryptoLayer.h"

static int32 asnParseLength(unsigned char **p, int32 size, int32 *valLen);
static int32 getBig(psPool_t *pool, unsigned char **pp, int32 len, mp_int *big);
static int32 getSerialNum(psPool_t *pool, unsigned char **pp, int32 len, 
						unsigned char **sn, int32 *snLen);
static int32 getInteger(unsigned char **pp, int32 len, int32 *val);
static int32 getSequence(unsigned char **pp, int32 len, int32 *outLen);
static int32 getSet(unsigned char **pp, int32 len, int32 *outLen);
static int32 getExplicitVersion(unsigned char **pp, int32 len, int32 expVal,
							  int32 *outLen);
static int32 getAlgorithmIdentifier(unsigned char **pp, int32 len, int32 *oi,
								  int32 isPubKey);
static int32 getValidity(psPool_t *pool, unsigned char **pp, int32 len, 
					   char **notBefore, char **notAfter);
static int32 getSignature(psPool_t *pool, unsigned char **pp, int32 len,
						unsigned char **sig, int32 *sigLen);
static int32 getImplicitBitString(psPool_t *pool, unsigned char **pp, int32 len, 
						int32 impVal, unsigned char **bitString, int32 *bitLen);
static int32 getPubKey(psPool_t *pool, unsigned char **pp, int32 len, sslRsaKey_t *pubKey);

#ifdef USE_X509
static int32 getExplicitExtensions(psPool_t *pool, unsigned char **pp, 
								 int32 len, int32 expVal,
								 v3extensions_t *extensions);
static int32 getDNAttributes(psPool_t *pool, unsigned char **pp, int32 len,
						   DNattributes_t *attribs);
#endif /* USE_X509 */

#define ATTRIB_COUNTRY_NAME		6
#define ATTRIB_LOCALITY			7
#define ATTRIB_ORGANIZATION		10
#define ATTRIB_ORG_UNIT			11
#define ATTRIB_DN_QUALIFIER		46
#define ATTRIB_STATE_PROVINCE	8
#define ATTRIB_COMMON_NAME		3

#define IMPLICIT_ISSUER_ID		1
#define IMPLICIT_SUBJECT_ID		2
#define EXPLICIT_EXTENSION		3

#define EXT_CERT_POLICIES		146
#define EXT_BASIC_CONSTRAINTS	133
#define EXT_AUTH_KEY_ID			149
#define	EXT_KEY_USAGE			129
#define EXT_ALT_SUBJECT_NAME	131
#define EXT_ALT_ISSUER_NAME		132
#define EXT_SUBJ_KEY_ID			128

#define OID_SHA1				88
#define OID_MD2					646
#define OID_MD5					649

/******************************************************************************/
/*
	Parse a a private key structure in DER formatted ASN.1
	Per ftp://ftp.rsasecurity.com/pub/pkcs/pkcs-1/pkcs-1v2-1.pdf
	RSAPrivateKey ::= SEQUENCE {
		version Version,
		modulus INTEGER, -- n
		publicExponent INTEGER, -- e
		privateExponent INTEGER, -- d
		prime1 INTEGER, -- p
		prime2 INTEGER, -- q
		exponent1 INTEGER, -- d mod (p-1)
		exponent2 INTEGER, -- d mod (q-1)
		coefficient INTEGER, -- (inverse of q) mod p
		otherPrimeInfos OtherPrimeInfos OPTIONAL
	}
	Version ::= INTEGER { two-prime(0), multi(1) }
	  (CONSTRAINED BY {-- version must be multi if otherPrimeInfos present --})

	Which should look something like this in hex (pipe character 
	is used as a delimiter):
	ftp://ftp.rsa.com/pub/pkcs/ascii/layman.asc
	30	Tag in binary: 00|1|10000 -> UNIVERSAL | CONSTRUCTED | SEQUENCE (16)
	82	Length in binary: 1 | 0000010 -> LONG LENGTH | LENGTH BYTES (2)
	04 A4	Length Bytes (1188)
	02	Tag in binary: 00|0|00010 -> UNIVERSAL | PRIMITIVE | INTEGER (2)
	01	Length in binary: 0|0000001 -> SHORT LENGTH | LENGTH (1)
	00	INTEGER value (0) - RSAPrivateKey.version
	02	Tag in binary: 00|0|00010 -> UNIVERSAL | PRIMITIVE | INTEGER (2)
	82	Length in binary: 1 | 0000010 -> LONG LENGTH | LENGTH BYTES (2)
	01 01	Length Bytes (257)
	[]	257 Bytes of data - RSAPrivateKey.modulus (2048 bit key)
	02	Tag in binary: 00|0|00010 -> UNIVERSAL | PRIMITIVE | INTEGER (2)
	03	Length in binary: 0|0000011 -> SHORT LENGTH | LENGTH (3)
	01 00 01	INTEGER value (65537) - RSAPrivateKey.publicExponent
	...

	OtherPrimeInfos is not supported in this routine, and an error will be
	returned if they are present
*/

int32 psAsnParsePrivateKey(psPool_t *pool, unsigned char **pp, int32 size, sslRsaKey_t *key)
{
	unsigned char	*p, *end, *seq;
	int32			version, seqlen;

	key->optimized = 0;
	p = *pp;
	end = p + size;

	if (getSequence(&p, size, &seqlen) < 0) {
		matrixStrDebugMsg("ASN sequence parse error\n", NULL);
		return -1;
	}
	seq = p;
	if (getInteger(&p, (int32)(end - p), &version) < 0 || version != 0 ||
		getBig(pool, &p, (int32)(end - p), &(key->N)) < 0 ||
		getBig(pool, &p, (int32)(end - p), &(key->e)) < 0 ||
		getBig(pool, &p, (int32)(end - p), &(key->d)) < 0 ||
		getBig(pool, &p, (int32)(end - p), &(key->p)) < 0 ||
		getBig(pool, &p, (int32)(end - p), &(key->q)) < 0 ||
		getBig(pool, &p, (int32)(end - p), &(key->dP)) < 0 ||
		getBig(pool, &p, (int32)(end - p), &(key->dQ)) < 0 ||
		getBig(pool, &p, (int32)(end - p), &(key->qP)) < 0 ||
		(int32)(p - seq) != seqlen) {
		matrixStrDebugMsg("ASN key extract parse error\n", NULL);
		return -1;
	}
	if (mp_shrink(&key->e) != MP_OKAY) { goto done; }
	if (mp_shrink(&key->d) != MP_OKAY) { goto done; }
	if (mp_shrink(&key->N) != MP_OKAY) { goto done; }
	if (mp_shrink(&key->p) != MP_OKAY) { goto done; }
	if (mp_shrink(&key->q) != MP_OKAY) { goto done; }

/*
	Compute the optimization members.  The qP is stored in the file but
	does not match the optimization algorithms used by mpi, so
	recompute it here along with adding pQ.
*/	
	if (mp_invmod(pool, &key->q, &key->p, &key->qP) != MP_OKAY) {
		goto done;
	}
	if (mp_mulmod(pool, &key->qP, &key->q, &key->N, &key->qP) != MP_OKAY) {
		goto done;
	}
	if (mp_invmod(pool, &key->p, &key->q, &key->pQ) != MP_OKAY) { 
		goto done;
	}
	if (mp_mulmod(pool, &key->pQ, &key->p, &key->N, &key->pQ)) { 
		goto done;
	}
	
	if (mp_shrink(&key->dQ) != MP_OKAY) { goto done; }
	if (mp_shrink(&key->dP) != MP_OKAY) { goto done; }
	if (mp_shrink(&key->qP) != MP_OKAY) { goto done; }
	if (mp_shrink(&key->pQ) != MP_OKAY) { goto done; }
/*
	If we made it here, the key is ready for optimized decryption
*/
	key->optimized = 1;

done:
	*pp = p;
/*
	Set the key length of the key
*/
	key->size = mp_unsigned_bin_size(&key->N);
	return 0;
}

#ifdef USE_X509
/******************************************************************************/
/*
	Parse an X509 ASN.1 certificate stream
	http://www.faqs.org/rfcs/rfc2459.html section 4.1
*/
int32 matrixX509ParseCert(psPool_t *pool, unsigned char *pp, int32 size, 
						sslRsaCert_t **outcert)
{
	sslRsaCert_t		*cert;
	sslMd5Context_t		md5Ctx;
	sslSha1Context_t	sha1Ctx;
	unsigned char		*p, *end, *certStart, *certEnd;
	int32				certLen, len;
#ifdef USE_MD2
	sslMd2Context_t		md2Ctx;
#endif /* USE_MD2 */

/*
	Allocate the cert structure right away.  User MUST always call
	matrixX509FreeCert regardless of whether this function succeeds.
	memset is important because the test for NULL is what is used
	to determine what to free
*/
	*outcert = cert = psMalloc(pool, sizeof(sslRsaCert_t));
	if (cert == NULL) {
		return -8; /* SSL_MEM_ERROR */
	}
	memset(cert, '\0', sizeof(sslRsaCert_t));

	p = pp;
	end = p + size;
/*
	Certificate  ::=  SEQUENCE  {
		tbsCertificate		TBSCertificate,
		signatureAlgorithm	AlgorithmIdentifier,
		signatureValue		BIT STRING	}
*/
	if (getSequence(&p, (int32)(end - p), &len) < 0) {
		matrixStrDebugMsg("Initial cert parse error\n", NULL);
		return -1;
	}
	certStart = p;
/*	
	TBSCertificate  ::=  SEQUENCE  {
		version			[0]		EXPLICIT Version DEFAULT v1,
		serialNumber			CertificateSerialNumber,
		signature				AlgorithmIdentifier,
		issuer					Name,
		validity				Validity,
		subject					Name,
		subjectPublicKeyInfo	SubjectPublicKeyInfo,
		issuerUniqueID	[1]		IMPLICIT UniqueIdentifier OPTIONAL,
							-- If present, version shall be v2 or v3
		subjectUniqueID	[2]	IMPLICIT UniqueIdentifier OPTIONAL,
							-- If present, version shall be v2 or v3
		extensions		[3]	EXPLICIT Extensions OPTIONAL
							-- If present, version shall be v3	}
*/
	if (getSequence(&p, (int32)(end - p), &len) < 0) {
		matrixStrDebugMsg("ASN sequence parse error\n", NULL);
		return -1;
	}
	certEnd = p + len;
	certLen = (int32)(certEnd - certStart);

/*
	Version  ::=  INTEGER  {  v1(0), v2(1), v3(2)  }
*/
	if (getExplicitVersion(&p, (int32)(end - p), 0, &cert->version) < 0) {
		matrixStrDebugMsg("ASN version parse error\n", NULL);
		return -1;
	}
	if (cert->version != 2) {
		matrixIntDebugMsg("Warning: non-v3 certificate version: %d\n",
			cert->version);
	}
/*
	CertificateSerialNumber  ::=  INTEGER
*/
	if (getSerialNum(pool, &p, (int32)(end - p), &cert->serialNumber,
			&cert->serialNumberLen) < 0) {
		matrixStrDebugMsg("ASN serial number parse error\n", NULL);
		return -1;
	}
/*
	AlgorithmIdentifier  ::=  SEQUENCE  {
		algorithm				OBJECT IDENTIFIER,
		parameters				ANY DEFINED BY algorithm OPTIONAL }
*/
	if (getAlgorithmIdentifier(&p, (int32)(end - p),
			&cert->certAlgorithm, 0) < 0) {
		return -1;
	}
/*
	Name ::= CHOICE {
		RDNSequence }

	RDNSequence ::= SEQUENCE OF RelativeDistinguishedName

	RelativeDistinguishedName ::= SET OF AttributeTypeAndValue

	AttributeTypeAndValue ::= SEQUENCE {
		type	AttributeType,
		value	AttributeValue }

	AttributeType ::= OBJECT IDENTIFIER

	AttributeValue ::= ANY DEFINED BY AttributeType
*/
	if (getDNAttributes(pool, &p, (int32)(end - p), &cert->issuer) < 0) {
		return -1;
	}
/*
	Validity ::= SEQUENCE {
		notBefore	Time,
		notAfter	Time	}
*/
	if (getValidity(pool, &p, (int32)(end - p), &cert->notBefore,
			&cert->notAfter) < 0) {
		return -1;
	}
/*
	Subject DN
*/
	if (getDNAttributes(pool, &p, (int32)(end - p), &cert->subject) < 0) {
		return -1;
	}
/*
	SubjectPublicKeyInfo  ::=  SEQUENCE  {
		algorithm			AlgorithmIdentifier,
		subjectPublicKey	BIT STRING	}
*/
	if (getSequence(&p, (int32)(end - p), &len) < 0) {
		return -1;
	}
	if (getAlgorithmIdentifier(&p, (int32)(end - p),
			&cert->pubKeyAlgorithm, 1) < 0) {
		return -1;
	}
	if (getPubKey(pool, &p, (int32)(end - p), &cert->publicKey) < 0) {
		return -1;
	}
/*
	As the next three values are optional, we can do a specific test here
*/
	if (*p != (ASN_SEQUENCE | ASN_CONSTRUCTED)) {
		if (getImplicitBitString(pool, &p, (int32)(end - p), IMPLICIT_ISSUER_ID,
				&cert->uniqueUserId, &cert->uniqueUserIdLen) < 0 ||
			getImplicitBitString(pool, &p, (int32)(end - p), IMPLICIT_SUBJECT_ID,
				&cert->uniqueSubjectId, &cert->uniqueSubjectIdLen) < 0 ||
			getExplicitExtensions(pool, &p, (int32)(end - p), EXPLICIT_EXTENSION,
				&cert->extensions) < 0) {
			return -1;
		}
	}
/*
	This is the end of the cert.  Do a check here to be certain
*/
	if (certEnd != p) {
		return -1;
	}
/*
	Certificate signature info
*/
	if (getAlgorithmIdentifier(&p, (int32)(end - p),
			&cert->sigAlgorithm, 0) < 0) {
		return -1;
	}
/*
	Signature algorithm must match that specified in TBS cert
*/
	if (cert->certAlgorithm != cert->sigAlgorithm) {
		matrixStrDebugMsg("Parse error: mismatched signature type\n", NULL);
		return -1; 
	}
/*
	Compute the hash of the cert here for CA validation
*/
	if (cert->certAlgorithm == OID_RSA_MD5) {
		matrixMd5Init(&md5Ctx);
		matrixMd5Update(&md5Ctx, certStart, certLen);
		matrixMd5Final(&md5Ctx, cert->sigHash);
	} else if (cert->certAlgorithm == OID_RSA_SHA1) {
		matrixSha1Init(&sha1Ctx);
		matrixSha1Update(&sha1Ctx, certStart, certLen);
		matrixSha1Final(&sha1Ctx, cert->sigHash);
	}
#ifdef USE_MD2
	else if (cert->certAlgorithm == OID_RSA_MD2) {
		matrixMd2Init(&md2Ctx);
		matrixMd2Update(&md2Ctx, certStart, certLen);
		matrixMd2Final(&md2Ctx, cert->sigHash);
	}
#endif /* USE_MD2 */

	if (getSignature(pool, &p, (int32)(end - p), &cert->signature,
			&cert->signatureLen) < 0) {
		return -1;
	}

	if (p != end) {
		matrixStrDebugMsg("Warning: Cert parse did not reach end of bufffer\n",
			NULL);
	}
	return (int32)(p - pp);
}

/******************************************************************************/
/*
	User must call after all calls to matrixX509ParseCert
	(we violate the coding standard a bit here for clarity)
*/
void matrixX509FreeCert(sslRsaCert_t *cert)
{
	sslRsaCert_t	*curr, *next;

	curr = cert;
	while (curr) {
		if (curr->issuer.country)		psFree(curr->issuer.country);
		if (curr->issuer.state)			psFree(curr->issuer.state);
		if (curr->issuer.locality)		psFree(curr->issuer.locality);
		if (curr->issuer.organization)	psFree(curr->issuer.organization);
		if (curr->issuer.orgUnit)		psFree(curr->issuer.orgUnit);
		if (curr->issuer.commonName)	psFree(curr->issuer.commonName);
		if (curr->subject.country)		psFree(curr->subject.country);
		if (curr->subject.state)		psFree(curr->subject.state);
		if (curr->subject.locality)		psFree(curr->subject.locality);
		if (curr->subject.organization)	psFree(curr->subject.organization);
		if (curr->subject.orgUnit)		psFree(curr->subject.orgUnit);
		if (curr->subject.commonName)	psFree(curr->subject.commonName);
		if (curr->serialNumber)			psFree(curr->serialNumber);
		if (curr->notBefore)			psFree(curr->notBefore);
		if (curr->notAfter)				psFree(curr->notAfter);
		if (curr->publicKey.N.dp)		mp_clear(&(curr->publicKey.N));
		if (curr->publicKey.e.dp)		mp_clear(&(curr->publicKey.e));
		if (curr->signature)			psFree(curr->signature);
		if (curr->uniqueUserId)			psFree(curr->uniqueUserId);
		if (curr->uniqueSubjectId)		psFree(curr->uniqueSubjectId);
		if (curr->extensions.san.dns)	psFree(curr->extensions.san.dns);
		if (curr->extensions.san.uri)	psFree(curr->extensions.san.uri);
		if (curr->extensions.san.email)	psFree(curr->extensions.san.email);
#ifdef USE_FULL_CERT_PARSE
		if (curr->extensions.sk.id)		psFree(curr->extensions.sk.id);
		if (curr->extensions.ak.keyId)	psFree(curr->extensions.ak.keyId);
		if (curr->extensions.ak.attribs.commonName)
						psFree(curr->extensions.ak.attribs.commonName);
		if (curr->extensions.ak.attribs.country)
						psFree(curr->extensions.ak.attribs.country);
		if (curr->extensions.ak.attribs.state)
						psFree(curr->extensions.ak.attribs.state);
		if (curr->extensions.ak.attribs.locality)
						psFree(curr->extensions.ak.attribs.locality);
		if (curr->extensions.ak.attribs.organization)
						psFree(curr->extensions.ak.attribs.organization);
		if (curr->extensions.ak.attribs.orgUnit)
						psFree(curr->extensions.ak.attribs.orgUnit);
#endif /* SSL_FULL_CERT_PARSE */
		next = curr->next;
		psFree(curr);
		curr = next;
	}
}	

/******************************************************************************/
/*
	Do the signature validation for a subject certificate against a
	known CA certificate
*/
int32 psAsnConfirmSignature(char *sigHash, unsigned char *sigOut, int32 sigLen)
{
	unsigned char	*end, *p = sigOut;
	unsigned char	hash[SSL_SHA1_HASH_SIZE];
	int32			len, oi;

	end = p + sigLen;
/*
	DigestInfo ::= SEQUENCE {
		digestAlgorithm DigestAlgorithmIdentifier,
		digest Digest }

	DigestAlgorithmIdentifier ::= AlgorithmIdentifier

	Digest ::= OCTET STRING
*/
	if (getSequence(&p, (int32)(end - p), &len) < 0) {
		return -1;
	}

/*
	Could be MD5 or SHA1
 */
	if (getAlgorithmIdentifier(&p, (int32)(end - p), &oi, 0) < 0) {
		return -1;
	}
	if ((*p++ != ASN_OCTET_STRING) ||
			asnParseLength(&p, (int32)(end - p), &len) < 0 || (end - p) <  len) {
		return -1;
	}
	memcpy(hash, p, len);
	if (oi == OID_MD5 || oi == OID_MD2) {
		if (len != SSL_MD5_HASH_SIZE) {
			return -1;
		}
	} else if (oi == OID_SHA1) {
		if (len != SSL_SHA1_HASH_SIZE) {
			return -1;
		}
	} else {
		return -1;
	}
/*
	hash should match sigHash
*/
	if (memcmp(hash, sigHash, len) != 0) {
		return -1;
	}
	return 0;
}

/******************************************************************************/
/*
	Implementations of this specification MUST be prepared to receive
	the following standard attribute types in issuer names:
	country, organization, organizational-unit, distinguished name qualifier,
	state or province name, and common name 
*/
static int32 getDNAttributes(psPool_t *pool, unsigned char **pp, int32 len, 
						   DNattributes_t *attribs)
{
	sslSha1Context_t	hash;
	unsigned char		*p = *pp;
	unsigned char		*dnEnd, *dnStart;
	int32				llen, setlen, arcLen, id, stringType;
	char				*stringOut;

	dnStart = p;
	if (getSequence(&p, len, &llen) < 0) {
		return -1;
	}
	dnEnd = p + llen;

	matrixSha1Init(&hash);
	while (p < dnEnd) {
		if (getSet(&p, (int32)(dnEnd - p), &setlen) < 0) {
			return -1;
		}
		if (getSequence(&p, (int32)(dnEnd - p), &llen) < 0) {
			return -1;
		}
		if (dnEnd <= p || (*(p++) != ASN_OID) ||
				asnParseLength(&p, (int32)(dnEnd - p), &arcLen) < 0 || 
				(dnEnd - p) < arcLen) {
			return -1;
		}
/*
		id-at   OBJECT IDENTIFIER       ::=     {joint-iso-ccitt(2) ds(5) 4}
		id-at-commonName		OBJECT IDENTIFIER		::=		{id-at 3}
		id-at-countryName		OBJECT IDENTIFIER		::=		{id-at 6}
		id-at-localityName		OBJECT IDENTIFIER		::=		{id-at 7}
		id-at-stateOrProvinceName		OBJECT IDENTIFIER	::=	{id-at 8}
		id-at-organizationName			OBJECT IDENTIFIER	::=	{id-at 10}
		id-at-organizationalUnitName	OBJECT IDENTIFIER	::=	{id-at 11}
*/
		*pp = p;
/*
		FUTURE: Currently skipping OIDs not of type {joint-iso-ccitt(2) ds(5) 4}
		However, we could be dealing with an OID we MUST support per RFC.
		domainComponent is one such example.
*/
		if (dnEnd - p < 2) {
			return -1;
		}
		if ((*p++ != 85) || (*p++ != 4) ) {
			p = *pp;
/*
			Move past the OID and string type, get data size, and skip it.
			NOTE: Have had problems parsing older certs in this area.
*/
			if (dnEnd - p < arcLen + 1) {
				return -1;
			}
			p += arcLen + 1;
			if (asnParseLength(&p, (int32)(dnEnd - p), &llen) < 0 || 
					dnEnd - p < llen) {
				return -1;
			}
			p = p + llen;
			continue;
		}
/*
		Next are the id of the attribute type and the ASN string type
*/
		if (arcLen != 3 || dnEnd - p < 2) {
			return -1;
		}
		id = (int32)*p++;
/*
		Done with OID parsing
*/
		stringType = (int32)*p++;

		asnParseLength(&p, (int32)(dnEnd - p), &llen);
		if (dnEnd - p < llen) {
			return -1;
		}
		switch (stringType) {
			case ASN_PRINTABLESTRING:
			case ASN_UTF8STRING:
			case ASN_T61STRING:
				stringOut = psMalloc(pool, llen + 1);
				if (stringOut == NULL) {
					return -8; /* SSL_MEM_ERROR */
				}
				memcpy(stringOut, p, llen);
				stringOut[llen] = '\0';
				p = p + llen;
				break;
			default:
				matrixStrDebugMsg("Parsing untested DN attrib type\n", NULL);
				return -1;
		}

		switch (id) {
			case ATTRIB_COUNTRY_NAME:
				if (attribs->country) {
					psFree(attribs->country);
				}
				attribs->country = stringOut;
				break;
			case ATTRIB_STATE_PROVINCE:
				if (attribs->state) {
					psFree(attribs->state);
				}
				attribs->state = stringOut;
				break;
			case ATTRIB_LOCALITY:
				if (attribs->locality) {
					psFree(attribs->locality);
				}
				attribs->locality = stringOut;
				break;
			case ATTRIB_ORGANIZATION:
				if (attribs->organization) {
					psFree(attribs->organization);
				}
				attribs->organization = stringOut;
				break;
			case ATTRIB_ORG_UNIT:
				if (attribs->orgUnit) {
					psFree(attribs->orgUnit);
				}
				attribs->orgUnit = stringOut;
				break;
			case ATTRIB_COMMON_NAME:
				if (attribs->commonName) {
					psFree(attribs->commonName);
				}
				attribs->commonName = stringOut;
				break;
/*
			Not a MUST support
*/
			default:
				psFree(stringOut);
				return -1;
		}
/*
		Hash up the DN.  Nice for validation later
*/
		if (stringOut != NULL) {
			matrixSha1Update(&hash, (unsigned char*)stringOut, llen);
		}
	}
	matrixSha1Final(&hash, (unsigned char*)attribs->hash);
	*pp = p;
	return 0;
}

/******************************************************************************/
/*
	X509v3 extensions
*/
static int32 getExplicitExtensions(psPool_t *pool, unsigned char **pp, 
								 int32 inlen, int32 expVal,
								 v3extensions_t *extensions)
{
	unsigned char	*p = *pp, *end;
	unsigned char	*extEnd;
	int32			len, oid, tmpLen, critical;

	end = p + inlen;
	if (inlen < 1) {
		return -1;
	}
/*
	Not treating this as an error because it is optional.
*/
	if (*p != (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | expVal)) {
		return 0;
	}
	p++;
	if (asnParseLength(&p, (int32)(end - p), &len) < 0 || (end - p) < len) {
		return -1;
	}
/*
	Extensions  ::=  SEQUENCE SIZE (1..MAX) OF Extension

	Extension  ::=  SEQUENCE {
		extnID		OBJECT IDENTIFIER,
		extnValue	OCTET STRING	}
*/
	if (getSequence(&p, (int32)(end - p), &len) < 0) {
		return -1;
	}
	extEnd = p + len;
	while ((p != extEnd) && *p == (ASN_SEQUENCE | ASN_CONSTRUCTED)) {
		if (getSequence(&p, (int32)(extEnd - p), &len) < 0) {
			return -1;
		}
/*
		Conforming CAs MUST support key identifiers, basic constraints,
		key usage, and certificate policies extensions
	
		id-ce-authorityKeyIdentifier	OBJECT IDENTIFIER ::=	{ id-ce 35 }
		id-ce-basicConstraints			OBJECT IDENTIFIER ::=	{ id-ce 19 } 133
		id-ce-keyUsage					OBJECT IDENTIFIER ::=	{ id-ce 15 }
		id-ce-certificatePolicies		OBJECT IDENTIFIER ::=	{ id-ce 32 }
		id-ce-subjectAltName			OBJECT IDENTIFIER ::=	{ id-ce 17 }  131
*/
		if (extEnd - p < 1 || *p++ != ASN_OID) {
			return -1;
		}
		oid = 0;
		if (asnParseLength(&p, (int32)(extEnd - p), &len) < 0 || 
				(extEnd - p) < len) {
			return -1;
		}
		while (len-- > 0) {
			oid += (int32)*p++;
		}
/*
		Possible boolean value here for 'critical' id.  It's a failure if a
		critical extension is found that is not supported
*/
		critical = 0;
		if (*p == ASN_BOOLEAN) {
			p++;
			if (*p++ != 1) {
				matrixStrDebugMsg("Error parsing cert extension\n", NULL);
				return -1;
			}
			if (*p++ > 0) {
				critical = 1;
			}
		}
		if (extEnd - p < 1 || (*p++ != ASN_OCTET_STRING) ||
				asnParseLength(&p, (int32)(extEnd - p), &len) < 0 || 
				extEnd - p < len) {
			matrixStrDebugMsg("Expecting OCTET STRING in ext parse\n", NULL);
			return -1;
		}

		switch (oid) {
/*
			 BasicConstraints ::= SEQUENCE {
				cA						BOOLEAN DEFAULT FALSE,
				pathLenConstraint		INTEGER (0..MAX) OPTIONAL }
*/
			case EXT_BASIC_CONSTRAINTS:
				if (getSequence(&p, (int32)(extEnd - p), &len) < 0) {
					return -1;
				}
/*
				"This goes against PKIX guidelines but some CAs do it and some
				software requires this to avoid interpreting an end user
				certificate as a CA."
					- OpenSSL certificate configuration doc

				basicConstraints=CA:FALSE
*/
				if (len == 0) {
					break;
				}
				if (extEnd - p < 3) {
					return -1;
				}
				if (*p++ != ASN_BOOLEAN) {
					return -1;
				}
				if (*p++ != 1) {
					return -1;
				}
				extensions->bc.ca = *p++;
/*
				Now need to check if there is a path constraint. Only makes
				sense if cA is true.  If it's missing, there is no limit to
				the cert path
*/
				if (*p == ASN_INTEGER) {
					if (getInteger(&p, (int32)(extEnd - p),
							&(extensions->bc.pathLenConstraint)) < 0) {
						return -1;
					}
				} else {
					extensions->bc.pathLenConstraint = -1;
				}
				break;
			case EXT_ALT_SUBJECT_NAME:
				if (getSequence(&p, (int32)(extEnd - p), &len) < 0) {
					return -1;
				}
/*
				Looking only for DNS, URI, and email here to support
				FQDN for Web clients

				FUTURE:  Support all subject alt name members
				GeneralName ::= CHOICE {
					otherName						[0]		OtherName,
					rfc822Name						[1]		IA5String,
					dNSName							[2]		IA5String,
					x400Address						[3]		ORAddress,
					directoryName					[4]		Name,
					ediPartyName					[5]		EDIPartyName,
					uniformResourceIdentifier		[6]		IA5String,
					iPAddress						[7]		OCTET STRING,
					registeredID					[8]		OBJECT IDENTIFIER }
*/
				while (len > 0) {
					if (*p == (ASN_CONTEXT_SPECIFIC | ASN_PRIMITIVE | 2)) {
						p++;
						tmpLen = *p++;
						if (extEnd - p < tmpLen) {
							return -1;
						}
						extensions->san.dns = psMalloc(pool, tmpLen + 1);
						if (extensions->san.dns == NULL) {
							return -8; /* SSL_MEM_ERROR */
						}
						memset(extensions->san.dns, 0x0, tmpLen + 1);
						memcpy(extensions->san.dns, p, tmpLen);
					} else if (*p == (ASN_CONTEXT_SPECIFIC | ASN_PRIMITIVE | 6)) {
						p++;
						tmpLen = *p++;
						if (extEnd - p < tmpLen) {
							return -1;
						}
						extensions->san.uri = psMalloc(pool, tmpLen + 1);
						if (extensions->san.uri == NULL) {
							return -8; /* SSL_MEM_ERROR */
						}
						memset(extensions->san.uri, 0x0, tmpLen + 1);
						memcpy(extensions->san.uri, p, tmpLen);
					} else if (*p == (ASN_CONTEXT_SPECIFIC | ASN_PRIMITIVE | 1)) {
						p++;
						tmpLen = *p++;
						if (extEnd - p < tmpLen) {
							return -1;
						}
						extensions->san.email = psMalloc(pool, tmpLen + 1);
						if (extensions->san.email == NULL) {
							return -8; /* SSL_MEM_ERROR */
						}
						memset(extensions->san.email, 0x0, tmpLen + 1);
						memcpy(extensions->san.email, p, tmpLen);
					} else {
						matrixStrDebugMsg("Unsupported subjectAltName type.n",
							NULL);
						p++;
						tmpLen = *p++;
						if (extEnd - p < tmpLen) {
							return -1;
						}
					}
					p = p + tmpLen;
					len -= tmpLen + 2; /* the magic 2 is the type and length */
				}
				break;
#ifdef USE_FULL_CERT_PARSE
			case EXT_AUTH_KEY_ID:
/*
				AuthorityKeyIdentifier ::= SEQUENCE {
				keyIdentifier				[0] KeyIdentifier			OPTIONAL,
				authorityCertIssuer			[1] GeneralNames			OPTIONAL,
				authorityCertSerialNumber	[2] CertificateSerialNumber	OPTIONAL }

				KeyIdentifier ::= OCTET STRING
*/
				if (getSequence(&p, (int32)(extEnd - p), &len) < 0 || len < 1) {
					return -1;
				}
/*
				All memebers are optional
*/
				if (*p == (ASN_CONTEXT_SPECIFIC | ASN_PRIMITIVE | 0)) {
					p++;
					if (asnParseLength(&p, (int32)(extEnd - p), 
							&extensions->ak.keyLen) < 0 ||
							extEnd - p < extensions->ak.keyLen) {
						return -1;
					}
					extensions->ak.keyId = psMalloc(pool, extensions->ak.keyLen);
					if (extensions->ak.keyId == NULL) {
						return -8; /* SSL_MEM_ERROR */
					}
					memcpy(extensions->ak.keyId, p, extensions->ak.keyLen);
					p = p + extensions->ak.keyLen;
				}
				if (*p == (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | 1)) {
					p++;
					if (asnParseLength(&p, (int32)(extEnd - p), &len) < 0 ||
							len < 1 || extEnd - p < len) {
						return -1;
					}
					if ((*p ^ ASN_CONTEXT_SPECIFIC ^ ASN_CONSTRUCTED) != 4) {
/*
						FUTURE: support other name types
						We are just dealing with DN formats here
*/
						matrixIntDebugMsg("Error auth key-id name type: %d\n",
							*p ^ ASN_CONTEXT_SPECIFIC ^ ASN_CONSTRUCTED);
						return -1;
					}
					p++;
					if (asnParseLength(&p, (int32)(extEnd - p), &len) < 0 || 
							extEnd - p < len) {
						return -1;
					}
					if (getDNAttributes(pool, &p, (int32)(extEnd - p),
							&(extensions->ak.attribs)) < 0) {
						return -1;
					}
				}
				if (*p == (ASN_CONTEXT_SPECIFIC | ASN_PRIMITIVE | 2)) {
/*
					Standard getInteger doesn't like CONTEXT_SPECIFIC tag
*/
					*p &= ASN_INTEGER;
					if (getInteger(&p, (int32)(extEnd - p), 
							&(extensions->ak.serialNum)) < 0) {
						return -1;
					}
				}
				break;

			case EXT_KEY_USAGE:
/*
				KeyUsage ::= BIT STRING {
					digitalSignature		(0),
					nonRepudiation			(1),
					keyEncipherment			(2),
					dataEncipherment		(3),
					keyAgreement			(4),
					keyCertSign				(5),

					cRLSign					(6),
					encipherOnly			(7),
					decipherOnly			(8) }
*/
				if (*p++ != ASN_BIT_STRING) {
					return -1;
				}
				if (asnParseLength(&p, (int32)(extEnd - p), &len) < 0 || 
						extEnd - p < len) {
					return -1;
				}
				if (len != 2) {
					return -1;
				}
/*
				Assure all unused bits are 0 and store away
*/
				extensions->keyUsage = (*(p + 1)) & ~((1 << *p) -1);
				p = p + len;
				break;
			case EXT_SUBJ_KEY_ID:
/*
				The value of the subject key identifier MUST be the value
				placed in the key identifier field of the Auth Key Identifier
				extension of certificates issued by the subject of
				this certificate.
*/
				if (*p++ != ASN_OCTET_STRING || asnParseLength(&p,
						(int32)(extEnd - p), &(extensions->sk.len)) < 0 ||
						extEnd - p < extensions->sk.len) {
					return -1;
				}
				extensions->sk.id = psMalloc(pool, extensions->sk.len);
				if (extensions->sk.id == NULL) {
					return -8; /* SSL_MEM_ERROR */
				}
				memcpy(extensions->sk.id, p, extensions->sk.len);
				p = p + extensions->sk.len;
				break;
#endif /* USE_FULL_CERT_PARSE */
/*
			Unsupported or skipping because USE_FULL_CERT_PARSE is undefined
*/
			default:
				if (critical) {
					matrixStrDebugMsg("Unknown critical ext encountered\n",
						NULL);
					return -1;
				}
				p++;
				if (asnParseLength(&p, (int32)(extEnd - p), &len) < 0 ||
						extEnd - p < len) {
					return -1;
				}
				p = p + len;
				break;
		}
	}
	*pp = p;
	return 0;
}
#endif /* USE_X509 */

/******************************************************************************/
/*
	On success, p will be updated to point to first character of value and
	valLen will contain number of bytes in value
	Return:
		0			Success
		< 0			Error
*/
static int32 asnParseLength(unsigned char **p, int32 size, int32 *valLen)
{
	unsigned char	*c, *end;
	int32			len, olen;

	c = *p;
	end = c + size;
	if (end - c < 1) {
		return -1;
	}
/*
	If the length byte has high bit only set, it's an indefinite length
	We don't support this!
*/
	if (*c == 0x80) {
		*valLen = -1;
		matrixStrDebugMsg("Unsupported: ASN indefinite length\n", NULL);
		return -1;
	}
/*
	If the high bit is set, the lower 7 bits represent the number of 
	bytes that follow and represent length
	If the high bit is not set, the lower 7 represent the actual length
*/
	len = *c & 0x7F;
	if (*(c++) & 0x80) {
/*
		Make sure there aren't more than 4 bytes of length specifier,
		and that we have that many bytes left in the buffer
*/
		if (len > sizeof(int32) || len == 0x7f || (end - c) < len) {
			return -1;
		}
		olen = 0;
		while (len-- > 0) {
			olen = (olen << 8) | *c;
			c++;
		}
		if (olen < 0 || olen > INT_MAX) {
			return -1;
		}
		len = olen;
	}
	*p = c;
	*valLen = len;
	return 0;
}

/******************************************************************************/
/*
	Callback to extract a big int32 (stream of bytes) from the DER stream
*/
static int32 getBig(psPool_t *pool, unsigned char **pp, int32 len, mp_int *big)
{
	unsigned char	*p = *pp;
	int32			vlen;

	if (len < 1 || *(p++) != ASN_INTEGER ||
			asnParseLength(&p, len - 1, &vlen) < 0) {
		matrixStrDebugMsg("ASN getBig failed\n", NULL);
		return -1;
	}
	mp_init(pool, big);
	if (mp_read_unsigned_bin(big, p, vlen) != 0) {
		mp_clear(big);
		matrixStrDebugMsg("ASN getBig failed\n", NULL);
		return -1;
	}
	p += vlen;
	*pp = p;
	return 0;
}

/******************************************************************************/
/*
	Although a certificate serial number is encoded as an integer type, that
	doesn't prevent it from being abused as containing a variable length
	binary value.  Get it here.
*/	
static int32 getSerialNum(psPool_t *pool, unsigned char **pp, int32 len, 
						unsigned char **sn, int32 *snLen)
{
	unsigned char	*p = *pp;
	int32			vlen;

	if (len < 1 || *(p++) != ASN_INTEGER ||
			asnParseLength(&p, len - 1, &vlen) < 0) {
		matrixStrDebugMsg("ASN getSerialNumber failed\n", NULL);
		return -1;
	}
	*snLen = vlen;
	*sn = psMalloc(pool, vlen);
	if (*sn == NULL) {
		return -8; /* SSL_MEM_ERROR */
	}
	memcpy(*sn, p, vlen);
	p += vlen;
	*pp = p;
	return 0;
}

/******************************************************************************/
/*
	Callback to extract a sequence length from the DER stream
	Verifies that 'len' bytes are >= 'seqlen'
	Move pp to the first character in the sequence
*/
static int32 getSequence(unsigned char **pp, int32 len, int32 *seqlen)
{
	unsigned char	*p = *pp;

	if (len < 1 || *(p++) != (ASN_SEQUENCE | ASN_CONSTRUCTED) || 
			asnParseLength(&p, len - 1, seqlen) < 0 || len < *seqlen) {
		return -1;
	}
	*pp = p;
	return 0;
}

/******************************************************************************/
/*
	Extract a set length from the DER stream
*/
static int32 getSet(unsigned char **pp, int32 len, int32 *setlen)
{
	unsigned char	*p = *pp;

	if (len < 1 || *(p++) != (ASN_SET | ASN_CONSTRUCTED) || 
			asnParseLength(&p, len - 1, setlen) < 0 || len < *setlen) {
		return -1;
	}
	*pp = p;
	return 0;
}

/******************************************************************************/
/*
	Explicit value encoding has an additional tag layer
*/
static int32 getExplicitVersion(unsigned char **pp, int32 len, int32 expVal, int32 *val)
{
	unsigned char	*p = *pp;
	int32			exLen;

	if (len < 1) {
		return -1;
	}
/*
	This is an optional value, so don't error if not present.  The default
	value is version 1
*/	
	if (*p != (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | expVal)) {
		*val = 0;
		return 0;
	}
	p++;
	if (asnParseLength(&p, len - 1, &exLen) < 0 || (len - 1) < exLen) {
		return -1;
	}
	if (getInteger(&p, exLen, val) < 0) {
		return -1;
	}
	*pp = p;
	return 0;
}

/******************************************************************************/
/*
	Implementation specific OID parser for suppported RSA algorithms
*/
static int32 getAlgorithmIdentifier(unsigned char **pp, int32 len, int32 *oi,
								  int32 isPubKey)
{
	unsigned char	*p = *pp, *end;
	int32			arcLen, llen;

	end = p + len;
	if (len < 1 || getSequence(&p, len, &llen) < 0) {
		return -1;
	}
	if (end - p < 1) {
		return -1;
	}
	if (*(p++) != ASN_OID || asnParseLength(&p, (int32)(end - p), &arcLen) < 0 ||
			llen < arcLen) {
		return -1;
	}
/*
	List of expected (currently supported) OIDs
	algorithm				  summed	length		hex
	sha1						 88		05		2b0e03021a
	md2							646		08		2a864886f70d0202
	md5							649		08		2a864886f70d0205
	rsaEncryption				645		09		2a864886f70d010101
	md2WithRSAEncryption:		646		09		2a864886f70d010102
	md5WithRSAEncryption		648		09		2a864886f70d010104
	sha-1WithRSAEncryption		649		09		2a864886f70d010105

	Yes, the summing isn't ideal (as can be seen with the duplicate 649),
	but the specific implementation makes this ok.
*/
	if (end - p < 2) {
		return -1;
	}
	if (isPubKey && (*p != 0x2a) && (*(p + 1) != 0x86)) {
/*
		Expecting DSA here if not RSA, but OID doesn't always match
*/
		matrixStrDebugMsg("Unsupported algorithm identifier\n", NULL);
		return -1;
	}
	*oi = 0;
	while (arcLen-- > 0) {
		*oi += (int32)*p++;
	}
/*
	Each of these cases might have a trailing NULL parameter.  Skip it
*/
	if (*p != ASN_NULL) {
		*pp = p;
		return 0;
	}
	if (end - p < 2) {
		return -1;
	}
	*pp = p + 2;
	return 0;
}

/******************************************************************************/
/*
	Implementation specific date parser.
	Does not actually verify the date
*/
static int32 getValidity(psPool_t *pool, unsigned char **pp, int32 len,
					   char **notBefore, char **notAfter)
{
	unsigned char	*p = *pp, *end;
	int32			seqLen, timeLen;

	end = p + len;
	if (len < 1 || *(p++) != (ASN_SEQUENCE | ASN_CONSTRUCTED) || 
			asnParseLength(&p, len - 1, &seqLen) < 0 || (end - p) < seqLen) {
		return -1;
	}
/*
	Have notBefore and notAfter times in UTCTime or GeneralizedTime formats
*/
	if ((end - p) < 1 || ((*p != ASN_UTCTIME) && (*p != ASN_GENERALIZEDTIME))) {
		return -1;
	}
	p++;
/*
	Allocate them as null terminated strings
*/
	if (asnParseLength(&p, seqLen, &timeLen) < 0 || (end - p) < timeLen) {
		return -1;
	}
	*notBefore = psMalloc(pool, timeLen + 1);
	if (*notBefore == NULL) {
		return -8; /* SSL_MEM_ERROR */
	}
	memcpy(*notBefore, p, timeLen);
	(*notBefore)[timeLen] = '\0';
	p = p + timeLen;
	if ((end - p) < 1 || ((*p != ASN_UTCTIME) && (*p != ASN_GENERALIZEDTIME))) {
		return -1;
	}
	p++;
	if (asnParseLength(&p, seqLen - timeLen, &timeLen) < 0 || 
			(end - p) < timeLen) {
		return -1;
	}
	*notAfter = psMalloc(pool, timeLen + 1);
	if (*notAfter == NULL) {
		return -8; /* SSL_MEM_ERROR */
	}
	memcpy(*notAfter, p, timeLen);
	(*notAfter)[timeLen] = '\0';
	p = p + timeLen;

	*pp = p;
	return 0;
}

/******************************************************************************/
/*
	Get the BIT STRING key and plug into RSA structure
*/
static int32 getPubKey(psPool_t *pool, unsigned char **pp, int32 len, 
					 sslRsaKey_t *pubKey)
{
	unsigned char	*p = *pp;
	int32			pubKeyLen, ignore_bits, seqLen;

	if (len < 1 || (*(p++) != ASN_BIT_STRING) ||
			asnParseLength(&p, len - 1, &pubKeyLen) < 0 || 
			(len - 1) < pubKeyLen) {
		return -1;
	}
	
	ignore_bits = *p++;
/*
	We assume this is always zero
*/
	sslAssert(ignore_bits == 0);

	if (getSequence(&p, pubKeyLen, &seqLen) < 0 ||
			getBig(pool, &p, seqLen, &pubKey->N) < 0 ||
			getBig(pool, &p, seqLen, &pubKey->e) < 0) {
		return -1;
	}
	pubKey->size = mp_unsigned_bin_size(&pubKey->N);

	*pp = p;
	return 0;
}

/******************************************************************************/
/*
	Currently just returning the raw BIT STRING and size in bytes
*/
static int32 getSignature(psPool_t *pool, unsigned char **pp, int32 len,
						unsigned char **sig, int32 *sigLen)
{
	unsigned char	*p = *pp, *end;
	int32			ignore_bits, llen;
	
	end = p + len;
	if (len < 1 || (*(p++) != ASN_BIT_STRING) ||
			asnParseLength(&p, len - 1, &llen) < 0 || (end - p) < llen) {
		return -1;
	}
	ignore_bits = *p++;
/*
	We assume this is always 0.
*/
	sslAssert(ignore_bits == 0);
/*
	Length included the ignore_bits byte
*/
	*sigLen = llen - 1;
	*sig = psMalloc(pool, *sigLen);
	if (*sig == NULL) {
		return -8; /* SSL_MEM_ERROR */
	}
	memcpy(*sig, p, *sigLen);
	*pp = p + *sigLen;
	return 0;
}

/******************************************************************************/
/*
	Could be optional.  If the tag doesn't contain the value from the left
	of the IMPLICIT keyword we don't have a match and we don't incr the pointer.
*/
static int32 getImplicitBitString(psPool_t *pool, unsigned char **pp, int32 len, 
						int32 impVal, unsigned char **bitString, int32 *bitLen)
{
	unsigned char	*p = *pp;
	int32			ignore_bits;

	if (len < 1) {
		return -1;
	}
/*
	We don't treat this case as an error, because of the optional nature.
*/	
	if (*p != (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | impVal)) {
		return 0;
	}

	p++;
	if (asnParseLength(&p, len, bitLen) < 0) {
		return -1;
	}
	ignore_bits = *p++;
	sslAssert(ignore_bits == 0);

	*bitString = psMalloc(pool, *bitLen);
	if (*bitString == NULL) {
		return -8; /* SSL_MEM_ERROR */
	}
	memcpy(*bitString, p, *bitLen);
	*pp = p + *bitLen;
	return 0;
}

/******************************************************************************/
/*
	Get an integer
*/
static int32 getInteger(unsigned char **pp, int32 len, int32 *val)
{
	unsigned char	*p = *pp, *end;
	uint32			ui;
	int32			vlen;

	end = p + len;
	if (len < 1 || *(p++) != ASN_INTEGER ||
			asnParseLength(&p, len - 1, &vlen) < 0) {
		matrixStrDebugMsg("ASN getInteger failed\n", NULL);
		return -1;
	}
/*
	This check prevents us from having a big positive integer where the 
	high bit is set because it will be encoded as 5 bytes (with leading 
	blank byte).  If that is required, a getUnsigned routine should be used
*/
	if (vlen > sizeof(int32) || end - p < vlen) {
		matrixStrDebugMsg("ASN getInteger failed\n", NULL);
		return -1;
	}
	ui = 0;
/*
	If high bit is set, it's a negative integer, so perform the two's compliment
	Otherwise do a standard big endian read (most likely case for RSA)
*/
	if (*p & 0x80) {
		while (vlen-- > 0) {
			ui = (ui << 8) | (*p ^ 0xFF);
			p++;
		}
		vlen = (int32)ui;
		vlen++;
		vlen = -vlen;
		*val = vlen;
	} else {
		while (vlen-- > 0) {
			ui = (ui << 8) | *p;
			p++;
		}
		*val = (int32)ui;
	}
	*pp = p;
	return 0;
}

/******************************************************************************/
