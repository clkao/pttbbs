/*
 * �ϥΤ�k�j�P�p�U: innbbsd �� �b SUBJECT �q news Ū�i�ӫ�, �I�s
 * str_decode_M3(SUBJECT) �N��F
 */

/*
 * bsd ���U�ϥέn�sĶ�ɭn�[  -I/usr/local/include -L/usr/local/lib -liconv
 * �æw�� libiconv, �Y�u���S��iconv�N�⩳�U�� #define USE_ICONV 1 �R�F
 */

/*-------------------------------------------------------*/
/* lib/str_decode.c	( NTHU CS MapleBBS Ver 3.00 )	 */
/*-------------------------------------------------------*/
/* target : included C for QP/BASE64 decoding		 */
/* create : 95/03/29				 	 */
/* update : 97/03/29				 	 */
/*-------------------------------------------------------*/


/* ----------------------------------------------------- */
/* QP code : "0123456789ABCDEF"				 */
/* ----------------------------------------------------- */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h> /* isspace() */

#define USE_ICONV 1
/*
 * bsd ���U�ϥέn�sĶ�ɭn�[  -I/usr/local/include -L/usr/local/lib -liconv
 * �Y�u���S��iconv�N��W���o�� #define �R�F
 */

#ifdef USE_ICONV
#include <iconv.h>
#endif

static int 
qp_code(int x)
{
    if (x >= '0' && x <= '9')
	return x - '0';
    if (x >= 'a' && x <= 'f')
	return x - 'a' + 10;
    if (x >= 'A' && x <= 'F')
	return x - 'A' + 10;
    return -1;
}


/* ------------------------------------------------------------------ */
/* BASE64 :							      */
/* "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" */
/* ------------------------------------------------------------------ */


static int 
base64_code(int x)
{
    if (x >= 'A' && x <= 'Z')
	return x - 'A';
    if (x >= 'a' && x <= 'z')
	return x - 'a' + 26;
    if (x >= '0' && x <= '9')
	return x - '0' + 52;
    if (x == '+')
	return 62;
    if (x == '/')
	return 63;
    return -1;
}


/* ----------------------------------------------------- */
/* judge & decode QP / BASE64				 */
/* ----------------------------------------------------- */

inline int 
isreturn(unsigned char c)
{
    return c == '\r' || c == '\n';
}

#if 0 /* in glibc */
inline int 
isspace(unsigned char c)
{
    return c == ' ' || c == '\t' || isreturn(c);
}
#endif

/* static inline */
int 
mmdecode(unsigned char *src, unsigned char encode, unsigned char *dst)
{
    /* Thor.980901: src�Mdst�i�ۦP, ��src �@�w��?��\0���� */
    /* Thor.980901: �`�N, decode�X�����G���|�ۤv�[�W \0 */
    unsigned char  *t = dst;
    int             pattern = 0, bits = 0;
    encode |= 0x20;		/* Thor: to lower */
    switch (encode) {
    case 'q':			/* Thor: quoted-printable */
	while (*src && *src != '?') {	/* Thor: delimiter *//* Thor.980901:
					 * 0 ��O delimiter */
	    if (*src == '=') {
		int             x = *++src, y = x ? *++src : 0;
		if (isreturn(x))
		    continue;
		if ((x = qp_code(x)) < 0 || (y = qp_code(y)) < 0)
		    return -1;
		*t++ = (x << 4) + y, src++;
	    } else if (*src == '_')
		*t++ = ' ', src++;
#if 0
	    else if (!*src)	/* Thor: no delimiter is not successful */
		return -1;
#endif
	    else		/* Thor: *src != '=' '_' */
		*t++ = *src++;
	}
	return t - dst;
    case 'b':			/* Thor: base 64 */
	while (*src && *src != '?') {	/* Thor: delimiter */
	    /*
	     * Thor.980901: 0�]�� *//* Thor: pattern & bits are cleared
	     * outside
	     */
	    int             x;
#if 0
	    if (!*src)
		return -1;	/* Thor: no delimiter is not successful */
#endif
	    x = base64_code(*src++);
	    if (x < 0)
		continue;	/* Thor: ignore everything not in the
				 * base64,=,.. */
	    pattern = (pattern << 6) | x;
	    bits += 6;		/* Thor: 1 code gains 6 bits */
	    if (bits >= 8) {	/* Thor: enough to form a byte */
		bits -= 8;
		*t++ = (pattern >> bits) & 0xff;
	    }
	}
	return t - dst;
    }
    return -1;
}

#ifdef USE_ICONV
size_t 
str_iconv(
	  const char *fromcode,	/* charset of source string */
	  const char *tocode,	/* charset of destination string */
	  char *src,		/* source string */
	  size_t srclen,		/* source string length */
	  char *dst,		/* destination string */
	  size_t dstlen)
{				/* destination string length */
    /*
     * �o�Ө禡�|�N�@�Ӧr�� (src) �q charset=fromcode �ন charset=tocode,
     * srclen �O src ������, dst �O��X��buffer, dstlen �h���w�F dst ���j�p,
     * �̫�|�� '\0', �ҥH�n�d�@��byte��'\0'. �p�G�J�� src �����D�r�����r,
     * �άO src ���������㪺 byte, ���|�屼.
     */
    iconv_t         iconv_descriptor;
    size_t          iconv_ret, dstlen_old;

    dstlen--;			/* keep space for '\0' */

    dstlen_old = dstlen;

    /* Open a descriptor for iconv */
    iconv_descriptor = iconv_open(tocode, fromcode);

    if (iconv_descriptor == ((iconv_t) (-1))) {	/* if open fail */
	strncpy(dst, src, dstlen);
	return dstlen;
    }
    /* Start translation */
    while (srclen > 0 && dstlen > 0) {
	iconv_ret = iconv(iconv_descriptor, (const char **)&src, &srclen,
			  &dst, &dstlen);
	if (iconv_ret != 0) {
	    switch (errno) {
		/* invalid multibyte happened */
	    case EILSEQ:
		/* forward that byte */
		*dst = *src;
		src++;
		srclen--;
		dst++;
		dstlen--;
		break;
		/* incomplete multibyte happened */
	    case EINVAL:
		/* forward that byte (maybe wrong) */
		*dst = *src;
		src++;
		srclen--;
		dst++;
		dstlen--;
		break;
		/* dst no rooms */
	    case E2BIG:
		/* break out the while loop */
		srclen = 0;
		break;
	    }
	}
    }
    *dst = '\0';
    /* close descriptor of iconv */
    iconv_close(iconv_descriptor);

    return (dstlen_old - dstlen);
}
#endif


void 
str_decode_M3(unsigned char *str)
{
    int             adj;
    int             i;
    unsigned char  *src, *dst;
    unsigned char   buf[512];
    unsigned char   charset[512], dst1[512];


    src = str;
    dst = buf;
    adj = 0;

    while (*src && (dst - buf) < sizeof(buf) - 1) {
	if (*src != '=') {	/* Thor: not coded */
	    unsigned char  *tmp = src;
	    while (adj && *tmp && isspace(*tmp))
		tmp++;
	    if (adj && *tmp == '=') {	/* Thor: jump over space */
		adj = 0;
		src = tmp;
	    } else
		*dst++ = *src++;
	    /* continue; *//* Thor: take out */
	} else {		/* Thor: *src == '=' */
	    unsigned char  *tmp = src + 1;
	    if (*tmp == '?') {	/* Thor: =? coded */
		/* "=?%s?Q?" for QP, "=?%s?B?" for BASE64 */
		tmp++;
		i = 0;
		while (*tmp && *tmp != '?') {
		    if (i + 1 < sizeof(charset)) {
			charset[i] = *tmp;
			charset[i + 1] = '\0';
			i++;
		    }
		    tmp++;
		}
		if (*tmp && tmp[1] && tmp[2] == '?') {	/* Thor: *tmp == '?' */
#ifdef USE_ICONV
		    int             i = mmdecode(tmp + 3, tmp[1], dst1);
		    i = str_iconv(charset, "big5", dst1, i, dst,
				  sizeof(buf) - ((int)(dst - buf)));
#else
		    int             i = mmdecode(tmp + 3, tmp[1], dst);
#endif
		    if (i >= 0) {
			tmp += 3;	/* Thor: decode's src */
#if 0
			while (*tmp++ != '?');	/* Thor: no ? end, mmdecode
						 * -1 */
#endif
			while (*tmp && *tmp++ != '?');	/* Thor: no ? end,
							 * mmdecode -1 */
			/* Thor.980901: 0 �]�� decode ���� */
			if (*tmp == '=')
			    tmp++;
			src = tmp;	/* Thor: decode over */
			dst += i;
			adj = 1;/* Thor: adjcent */
		    }
		}
	    }
	    while (src != tmp)	/* Thor: not coded */
		*dst++ = *src++;
	}
    }
    *dst = 0;
    strcpy(str, buf);
}
