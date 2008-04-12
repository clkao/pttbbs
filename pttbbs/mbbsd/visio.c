/* $Id$ */
#include "bbs.h"

/*
 * visio.c
 * High-level virtual screen input output control
 *
 * This is not the original visio.c from maple3.
 * m3 visio = (ptt) visio+screen.
 * This visio contains only high level UI element/widgets.
 * In fact the only APIs from m3 are vmsg/vget...
 *
 * To add API here, please...
 * (1) name the API in prefix of 'v'.
 * (2) use only screen.c APIs.
 * (3) take care of wide screen.
 * (4) utilize the colos in visio.h, and name asa VCLR_* (visio color)
 */

// ---- DEFINITION ---------------------------------------------------
#define MAX_COL		(t_columns-1)
#define SAFE_MAX_COL	(MAX_COL-1)

// ---- UTILITIES ----------------------------------------------------
inline void
outnc(int n, unsigned char c)
{
    while (n-- > 0)
	outc(c);
}

inline void
nblank(int n)
{
    outnc(n, ' ');
}

// ---- HIGH LEVEL API -----------------------------------------------

/**
 * vbarf(s, ...): �榡�ƿ�X���k������r�� (MAX_COL)
 *
 * s �� \t �᪺���e�|����k�ݡC
 */
void
vbarf(const char *s, ...)
{
    char msg[512], *s2;
    va_list ap;
    va_start(ap, s);
    vsnprintf(msg, sizeof(msg), s, ap);
    va_end(ap);

    s2 = strchr(msg, '\t');
    if (s2) *s2++ = 0;
    else s2 = "";

    return vbar(msg, s2);
}

/**
 * vbar(l, r): ���k����e���ù� (MAX_COL)
 *
 * �`�N r ���᭱���|�ɪťաC
 * l r ���i�H�t ANSI ����X�C 
 * �`�N: �ثe����@�۰ʻ{�w��Фw�b��}�Y�C
 */
void
vbar(const char *l, const char *r)
{
    // TODO strlen_noansi �]�⦸... ��� l �i�H�� output ���C
    int szl = strlen_noansi(l),
	szr = strlen_noansi(r);
    // assume we are already in (y, 0)
    clrtoeol();
    outs(l);
    szl = MAX_COL - szl;
    if (szl > szr)
    {
	nblank(szl - szr);
	szl = szr;
    }
    if (szl == szr)
	outs(r);
    else if (szl > 0)
	nblank(szl);
    outs(ANSI_RESET);
}

/**
 * vfooter(caption, msg): �b�ù������L�X�榡�ƪ� caption msg 
 *
 * msg ���Y�� () �h�|�ܦ�A \t �᪺��r�|�a�k�C
 * �̫᭱�|�۰ʯd�@�Ӫť� (�H�קK�۰ʰ�������r�����D)�C
 */
void 
vfooter(const char *caption, const char *msg)
{
    int i = 0;
    move(b_lines, 0); clrtoeol();

    if (caption)
    {
	outs(VCLR_FOOTER_CAPTION);
	outs(caption); i+= strlen(caption);
    }

    if (!msg) msg = "";
    outs(VCLR_FOOTER);

    while (*msg && i < SAFE_MAX_COL)
    {
	if (*msg == '(')
	    outs(VCLR_FOOTER_QUOTE);
	else if (*msg == '\t')
	{
	    // if we don't have enough space, ignore whole.
	    int l = strlen(++msg);
	    if (i + l > SAFE_MAX_COL) break;
	    l = SAFE_MAX_COL - l - i;
	    nblank(l); 
	    i += l;
	    continue;
	}
	outc(*msg); i++;
	if (*msg == ')')
	    outs(VCLR_FOOTER);
	msg ++;
    }
    nblank(SAFE_MAX_COL-i);
    outc(' ');
    outs(ANSI_RESET);
}

