#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "fnv_hash.h"

#include "ansi.h"
#include "libbbsutil.h"

#define CHAR_LOWER(c)  ((c >= 'A' && c <= 'Z') ? c|32 : c)
/* ----------------------------------------------------- */
/* �r���ഫ�ˬd���                                      */
/* ----------------------------------------------------- */
/**
 * �N�r�� s �ର�p�g�s�^ t
 * @param t allocated char array
 * @param s
 */
void
str_lower(char *t, const char *s)
{
    register unsigned char ch;

    do {
	ch = *s++;
	*t++ = CHAR_LOWER(ch);
    } while (ch);
}

/**
 * �����r�� buf ��ݦh�l���ťաC
 * @param buf
 */
void
trim(char *buf)
{				/* remove trailing space */
    char           *p = buf;

    while (*p)
	p++;
    while (--p >= buf) {
	if (*p == ' ')
	    *p = '\0';
	else
	    break;
    }
}

/**
 * ���� src �� '\n' �ç令 '\0'
 * @param src
 */
void chomp(char *src)
{
    while(*src){
	if (*src == '\n')
	    *src = 0;
	else
	    src++;
    }
}

int
strip_blank(char *cbuf, char *buf)
{
    for (; *buf; buf++)
	if (*buf != ' ')
	    *cbuf++ = *buf;
    *cbuf = 0;
    return 0;
}

static const char EscapeFlag[] = {
    /*  0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 10 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0, 0, 0, 0, 0,
    /* 20 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 30 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, /* 0~9 ;= */
    /* 40 */ 0, 2, 2, 2, 2, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, /* ABCDHIJK */
    /* 50 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 60 */ 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 2, 2, 0, 0, /* fhlm */
    /* 70 */ 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* su */
    /* 80 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 90 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* A0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* B0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* C0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* D0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* E0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* F0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
/**
 * �ھ� mode �� strip �r�� src�A�ç⵲�G�s�� dst
 * @param dst
 * @param src
 * @param mode enum {STRIP_ALL = 0, ONLY_COLOR, NO_RELOAD};
 *             STRIP_ALL:  �����Y��
 *             ONLY_COLOR: �Y���Ҧ����C��L���� (ESC[*m)
 *             NO_RELOAD: �� strip (?)
 * @return strip �᪺����
 */
int
strip_ansi(char *dst, const char *src, enum STRIP_FLAG mode)
{
    register int    count = 0;
#define isEscapeParam(X) (EscapeFlag[(int)(X)] & 1)
#define isEscapeCommand(X) (EscapeFlag[(int)(X)] & 2)

    for(; *src; ++src)
	if( *src != ESC_CHR ){
	    if( dst )
		*dst++ = *src;
	    ++count;
	}else{
	    const char* p = src + 1;
	    if( *p != '[' ){
		++src;
		if(*src=='\0') break;
		continue;
	    }
	    while(isEscapeParam(*++p));
	    if( (mode == NO_RELOAD && isEscapeCommand(*p)) ||
		(mode == ONLY_COLOR && *p == 'm' )){
		register int len = p - src + 1;
		if( dst ){
		    strncpy(dst, src, len);
		    dst += len;
		}
		count += len;
	    }
	    src = p;
	    if(*src=='\0') break;
	}
    if( dst )
	*dst = 0;
    return count;
}

int  
strlen_noansi(const char *s)
{
    // XXX this is almost identical to
    // strip_ansi(NULL, s, STRIP_ALL)
    register int count = 0, mode = 0;

    if (!s || !*s)
	return 0;

    for (; *s; ++s)
    {
	// 0 - no ansi, 1 - [, 2 - param+cmd
	switch (mode)
	{
	    case 0:
		if (*s == ESC_CHR)
		    mode = 1;
		else 
		    count ++;
		break;

	    case 1:
		if (*s == '[')
		    mode = 2;
		else
		    mode = 0; // unknown command
		break;

	    case 2:
		if (isEscapeParam(*s))
		    continue;
		else if (isEscapeCommand(*s))
		    mode = 0;
		else
		    mode = 0;
		break;
	}
    }
    return count;
}

void
strip_nonebig5(unsigned char *str, int maxlen)
{
  int i;
  int len=0;
  for(i=0;i<maxlen && str[i];i++) {
    if(32<=str[i] && str[i]<128)
      str[len++]=str[i];
    else if(str[i]==255) {
      if(i+1<maxlen)
	if(251<=str[i+1] && str[i+1]<=254) {
	  i++;
	  if(i+1<maxlen && str[i+1])
	    i++;
	}
      continue;
    } else if(str[i]&0x80) {
      if(i+1<maxlen)
	if((0x40<=str[i+1] && str[i+1]<=0x7e) ||
	   (0xa1<=str[i+1] && str[i+1]<=0xfe)) {
	  str[len++]=str[i];
	  str[len++]=str[i+1];
	  i++;
	}
    }
  }
  if(len<maxlen)
    str[len]='\0';
}

int DBCS_RemoveIntrEscape(unsigned char *buf, int *len)
{
    register int isInAnsi = 0, isInDBCS = 0;
    int l = 0, i = 0, oldl, iansi = 0;

    if (len) l = *len; else l = strlen((const char*)buf);
    oldl = l;

    for (i = 0; i < l; i++)
    {
	if (buf[i] == ESC_CHR && !isInAnsi)
	{
	    // new escape
	    isInAnsi = 1;
	    iansi = i;
	    continue;
	} 

	// character
	if (isInAnsi)
	{
	    // closing ANSI section?
	    switch (isInAnsi)
	    {
	    case 1: // normal ANSI
		if (buf[i] == '[')
		    isInAnsi = 2;
		else
		    isInAnsi = 0; // unknown command
		break;

	    case 2:
		if (isEscapeParam(buf[i]))
		    break;
		else 
		    isInAnsi = 0;
		break;
	    }
	    if (isInAnsi == 0 && isInDBCS && i+1 < l)
	    {
		// interupting ANSI closed, let's modify the string
		int sz = i + 1 - iansi; // size to move
		memmove(buf+iansi, buf+i+1, l-i-1);
		l -= sz;
		i = iansi-1; // for the ++ in loop
	    }
	} else if (isInDBCS) {
	    // not ANSI but in DBCS. finished one char.
	    isInDBCS = 0;
	} else if (buf[i] >= 0x80) {
	    // DBCS lead.
	    isInDBCS = 1;
	} else {
	    // normal character.
	}
    }

    if(len) *len = l;
    return (oldl != l) ? 1 : 0;
}

/* ----------------------------------------------------- */
/* �r���ˬd��ơG�^��B�Ʀr�B�ɦW�BE-mail address        */
/* ----------------------------------------------------- */

int
invalid_pname(const char *str)
{
    const char           *p1, *p2, *p3;

    p1 = str;
    while (*p1) {
	if (!(p2 = strchr(p1, '/')))
	    p2 = str + strlen(str);
	if (p1 + 1 > p2 || p1 + strspn(p1, ".") == p2) /* �����\�� / �}�Y, �άO // �����u�� . */
	    return 1;
	for (p3 = p1; p3 < p2; p3++)
	    if (!isalnum(*p3) && !strchr("@[]-._", *p3)) /* �u���\ alnum �γo�ǲŸ� */
		return 1;
	p1 = p2 + (*p2 ? 1 : 0);
    }
    return 0;
}

/*
 * return	1	if /^[0-9]+$/
 * 		0	else, �t�Ŧr��
 */
int is_number(const char *p)
{
    if (*p == '\0')
	return 0;

    for(; *p; p++) {
	if (*p < '0' || '9' < *p)
	    return 0;
    }
    return 1;
}

unsigned
StringHash(const char *s)
{
    return fnv1a_32_strcase(s, FNV1_32_INIT);
}

/* qp_encode() modified from mutt-1.5.7/rfc2047.c q_encoder() */
const char MimeSpecials[] = "@.,;:<>[]\\\"()?/= \t";
char * qp_encode (char *s, size_t slen, const char *d, const char *tocode)
{
    char hex[] = "0123456789ABCDEF";
    char *s0 = s;

    memcpy (s, "=?", 2), s += 2;
    memcpy (s, tocode, strlen (tocode)), s += strlen (tocode);
    memcpy (s, "?Q?", 3), s += 3;
    assert(s-s0+3<slen);

    while (*d != '\0' && s-s0+6<slen)
    {
	unsigned char c = *d++;
	if (c == ' ')
	    *s++ = '_';
	else if (c >= 0x7f || c < 0x20 || c == '_' ||  strchr (MimeSpecials, c))
	{ 
	    *s++ = '=';
	    *s++ = hex[(c & 0xf0) >> 4];
	    *s++ = hex[c & 0x0f];
	}
	else
	    *s++ = c;
    }
    memcpy (s, "?=", 2), s += 2;
    *s='\0';
    return s0;
}

