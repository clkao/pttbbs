/* $Id$ */
#include "bbs.h"

/*
 * visio.c
 * High-Level virtual screen input output control
 *
 * Author: piaip, 2008
 *
 * This is not the original visio.c from maple3.
 * We just borrowed its file name and then 
 * re-implemented everything :)
 *
 * m3 visio = (ptt) visio+screen/term.
 *
 * This visio contains only high level UI element/widgets.
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
#define VBUFLEN		(ANSILINELEN)

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

// this is a special strlen to speed up processing.
// warning: x MUST be #define x "msg".
// otherwise you need to use real strlen.
#define MACROSTRLEN(x) (sizeof(x)-1)

// ---- VSOREF API --------------------------------------------------

/**
 * vscr_save(): �Ǧ^�ثe�e�����ƥ�����C
 */
VSOREF
vscr_save(void)
{
    // TODO optimize memory allocation someday.
    screen_backup_t *o = (screen_backup_t*)malloc(sizeof(screen_backup_t));
    assert(o);
    scr_dump(o);
    return o;
}

/**
 * vscr_restore(obj): �ϥΨçR���e�����ƥ�����C
 */
void
vscr_restore(VSOREF obj)
{
    screen_backup_t *o = (screen_backup_t*)o;
    if (o)
    {
	scr_restore(o);
	memset(o, 0, sizeof(screen_backup_t));
	free(o);
    }
}

/**
 * vcur_save(): �Ǧ^�ثe��Ъ��ƥ�����C
 */
VSOREF
vcur_save(void)
{
    // XXX ���i�� new object �F�A pointer ���j
    int y, x;
    getyx(&y, &x);
    y = ((unsigned short)y << 16) | (unsigned short)x;
    return (VSOREF)NULL + y;
}

/**
 * vcur_restore(obj): �ϥΨçR����Ъ��ƥ�����C
 */
void
vcur_restore(VSOREF o)
{
    int y, x;
    y = (unsigned int)(o - NULL);
    x = (unsigned short)(y & 0xFFFF);
    y = (unsigned short)(y >> 16);
    move(y, x);
}

// ---- LOW LEVEL API -----------------------------------------------

/**
 * vpad(n, pattern): �� n �Ӧr�� (�ϥΪ��榡�� pattern)
 *
 * @param n �n�񺡪��r���� (�L�k�񺡮ɷ|�ϥΪťն��)
 * @param pattern ��R�Ϊ��r��
 */
inline void
vpad(int n, const char *pattern)
{
    int len = strlen(pattern);
    // assert(len > 0);

    while (n >= len)
    {
	outs(pattern); 
	n -= len;
    }
    if (n) nblank(n);
}

// ---- HIGH LEVEL API -----------------------------------------------

/**
 * vshowmsg(s): �b�����L�X���w�T���γ�ª��Ȱ��T��
 *
 * @param s ���w�T���C NULL: ���N���~��C s: �Y�� \t �h�᭱�r��a�k (�Y�L�h��ܥ��N��)
 */
void
vshowmsg(const char *msg)
{
    int w = SAFE_MAX_COL;
    move(b_lines, 0); clrtoeol();

    if (!msg)
    {
	// print default message in middle
	outs(VCLR_PAUSE_PAD);
	outc(' '); // initial one space

	// VMSG_PAUSE MUST BE A #define STRING.
	w -= MACROSTRLEN(VMSG_PAUSE); // strlen_noansi(VMSG_PAUSE);
	w--; // initial space
	vpad(w/2, VMSG_PAUSE_PAD);
	outs(VCLR_PAUSE);
	outs(VMSG_PAUSE);
	outs(VCLR_PAUSE_PAD);
	vpad(w - w/2, VMSG_PAUSE_PAD);
    } else {
	// print in left, with floating (if \t exists)
	char *pfloat = strchr(msg, '\t');
	int  szfloat = 0;
	int  nmsg = 0;

	// print prefix
	w -= MACROSTRLEN(VMSG_MSG_PREFIX); // strlen_noansi(VMSG_MSG_PREFIX);
	outs(VCLR_MSG);
	outs(VMSG_MSG_PREFIX);

	// if have float, calculate float size
	if (pfloat) {
	    nmsg = pfloat - msg;
	    w -= strlen_noansi(msg) -1;	    // -1 for \t
	    pfloat ++; // skip \t
	    szfloat = strlen_noansi(pfloat);
	} else {
	    pfloat = VMSG_MSG_FLOAT;
	    szfloat = MACROSTRLEN(VMSG_MSG_FLOAT); // strlen_noansi()
	    w -= strlen_noansi(msg) + szfloat;
	}

	// calculate if we can display float
	if (w < 0)
	{
	    w += szfloat;
	    szfloat = 0;
	}

	// print msg body
	if (nmsg)
	    outns(msg, nmsg);
	else
	    outs(msg);

	// print padding for floats
	if (w > 0)
	    nblank(w);

	// able to print float?
	if (szfloat) 
	{
	    outs(VCLR_MSG_FLOAT);
	    outs(pfloat);
	}
    }

    // safe blank
    outs(" " ANSI_RESET);
}

/**
 * vans(s): �b�����L�X�T���P�p��J��A�öǦ^�ϥΪ̪���J(�ର�p�g)�C
 *
 * @param s ���w�T���A�� vshowmsg
 */
int
vans(const char *msg)
{
    char buf[3];

    // move(b_lines, 0); clrtoeol();
    vget(b_lines, 0, msg, buf, sizeof(buf), LCECHO);
    return (unsigned char)buf[0];
}

/**
 * vansf(s, ...): �b�����L�X�T���P�p��J��A�öǦ^�ϥΪ̪���J(�ର�p�g)�C
 *
 * @param s ���w�T���A�� vshowmsg
 */
int 
vansf(const char *fmt, ...)
{
    char   msg[VBUFLEN];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    return vans(msg);
}

/**
 * vmsg(s): �b�����L�X���w�T���γ�ª��Ȱ��T���A�öǦ^�ϥΪ̪�����C
 *
 * @param s ���w�T���A�� vshowmsg
 */
int
vmsg(const char *msg)
{
    int i = 0;

    vshowmsg(msg);

    // wait for key
    do {
	i = igetch();
    } while( i == 0 );

    // clear message bar
    move(b_lines, 0);
    clrtoeol();

    return i;
}


/**
 * vmsgf(s, ...): �榡�ƿ�X�Ȱ��T���éI�s vmsg)�C
 *
 * @param s �榡�ƪ��T��
 */
int
vmsgf(const char *fmt,...)
{
    char   msg[VBUFLEN];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    return vmsg(msg);
}

/**
 * vbarf(s, ...): �榡�ƿ�X���k������r�� (MAX_COL)
 *
 * @param s �榡�Ʀr�� (\t �᪺���e�|����k��)
 */
void
vbarf(const char *s, ...)
{
    char msg[VBUFLEN], *s2;
    va_list ap;
    va_start(ap, s);
    vsnprintf(msg, sizeof(msg), s, ap);
    va_end(ap);

    s2 = strchr(msg, '\t');
    if (s2) *s2++ = 0;
    else s2 = "";

    return vbarlr(msg, s2);
}

/**
 * vbarlr(l, r): ���k����e���ù� (MAX_COL)
 * �`�N: �ثe����@�۰ʻ{�w��Фw�b��}�Y�C
 *
 * @param l �a��������r�� (�i�t ANSI �X)
 * @param r �a�k������r�� (�i�t ANSI �X�A�᭱���|�ɪť�)
 */
void
vbarlr(const char *l, const char *r)
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

// ---- THEMED FORMATTING OUTPUT -------------------------------------

/**
 * vs_header(title, mid, right): �M�ſù��ÿ�X������D (MAX_COL)
 *
 * @param title: �a�����D�n���D�A���|�Q���_�C
 * @param mid: �m�������A�i�Q�����C
 * @param right: �a�k�������A�Ŷ����~�|��ܡC
 */
void 
vs_header(const char *title, const char *mid, const char *right)
{
    int w = MAX_COL;
    int szmid   = mid   ? strlen(mid) : 0;
    int szright = right ? strlen(right) : 0;

    clear();
    outs(VCLR_HEADER);

    if (title)
    {
	outs(VMSG_HDR_PREFIX);
	outs(title);
	outs(VMSG_HDR_POSTFIX);
	w -= MACROSTRLEN(VMSG_HDR_PREFIX) + MACROSTRLEN(VMSG_HDR_POSTFIX);
	w -= strlen(title);
    }

    // determine if we can display right message, and 
    // if if we need to truncate mid.
    if (szmid + szright > w)
	szright = 0;

    if (szmid >= w)
	szmid = w;
    else {
	int l = (MAX_COL-szmid)/2;
	l -= (MAX_COL-w);
	if (l > 0)
	    nblank(l), w -= l;
    }

    if (szmid) {
	outs(VCLR_HEADER_MID);
	outns(mid, szmid);
	outs(VCLR_HEADER);
    }
    nblank(w - szmid);

    if (szright) {
	outs(VCLR_HEADER_RIGHT);
	outs(right);
    }
    outs(ANSI_RESET "\n");
}

/**
 * vs_hdr(title): �M�ſù��ÿ�X²�������D
 *
 * @param title
 */
void
vs_hdr(const char *title)
{
    clear();
    outs(VCLR_HDR VMSG_HDR_PREFIX);
    outs(title);
    outs(VMSG_HDR_POSTFIX ANSI_RESET "\n");
}

/**
 * vs_footer(caption, msg): �b�ù������L�X�榡�ƪ� caption msg (���i�t ANSI �X)
 *
 * @param caption ���䪺�����r��
 * @param msg �T���r��, \t ���r�a�k�B�̫᭱�|�۰ʯd�@�ӪťաC
 */
void 
vs_footer(const char *caption, const char *msg)
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
	{
	    outs(VCLR_FOOTER_QUOTE);
	}
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

