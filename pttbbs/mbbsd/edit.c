/* $Id$ */
/* edit.c, �ΨӴ��� bbs�W����r�s�边, �Y ve.
 * �{�b�o�@�ӬO�c�d�L������, �����í�w, �Τ���h�� cpu, ���O�i�H�٤U�\�h
 * ���O���� (�H Ptt����, �b�E�d�H�W�����ɭ�, ���i�٤U 50MB ���O����)
 * �p�G�z�{���u�� cpu���O����v�ä��X�G�z�����D, �z�i�H�Ҽ{��ϥέץ��e��
 * ���� (Revision 782)
 *
 * �쥻 ve �����k�O, �]���C�@��̤j�i�H��J WRAPMARGIN �Ӧr, ��O�N���C�@
 * ��O�d�F WRAPMARGIN �o��j���Ŷ� (�� 512 bytes) . ���O��ڤW, ���b�ץ�
 * �����̤p���Ҷq�W, �ڭ̥u���n�ϱo��ЩҦb�o�@����� WRAPMARGIN �o��j,
 * ��L�C�@���ꤣ���n�o��h���Ŷ�. ��O�o�� patch�N�b�C����Цb�涡����
 * ���ɭ�, �N�쥻������O�����Y�p, �A�N�s���쪺���歫�s�[�j, �H�F���̤p��
 * �O����ζq.
 * �H�W�����o�Ӱʧ@�b adjustline() ������, adjustline()�t�~�]�A�ץ��ƭ�
 * global pointer, �H�קK dangling pointer . 
 * �t�~�Y�w�q DEBUG, �b textline_t ���c���N�[�J mlength, ��ܸӦ��ڦ���
 * �O����j�p. �H��K���յ��G.
 * �o�Ӫ������G�٦��a��S���ץ��n, �i��ɭP segmentation fault .
 */
#include "bbs.h"
typedef struct textline_t {
    struct textline_t *prev;
    struct textline_t *next;
    short           len;
#ifdef DEBUG
    short           mlength;
#endif
    char            data[1];
}               textline_t;

#define KEEP_EDITING    -2
#define BACKUP_LIMIT    100

enum {
    NOBODY, MANAGER, SYSOP
};

typedef struct editor_internal_t {
    textline_t *firstline;
    textline_t *lastline;
    textline_t *currline;
    textline_t *blockline;
    textline_t *top_of_win;
    textline_t *deleted_lines;

    short currpnt, currln, totaln;
    short curr_window_line;
    short edit_margin;
    short blockln;
    short blockpnt;
    char insert_c;

    char phone_mode;
    char phone_mode0;
    char insert_character	:1;
    char my_ansimode		:1;
    char ifuseanony		:1;
    char redraw_everything	:1;
    char raw_mode		:1;
    char line_dirty		:1;
    char indent_mode		:1;

    struct editor_internal_t *prev;

} editor_internal_t;
// ?? } __attribute__ ((packed))

//static editor_internal_t *edit_buf_head = NULL;
static editor_internal_t *curr_buf = NULL;

static char editline[WRAPMARGIN + 2];

static const char fp_bak[] = "bak";


static char *BIG5[13] = {
  "�A�F�G�B�N�C�H�I�E�T�]�^��������",
  "�b�c�d�e�f�g�h�i�j�k�l�m�n�o�p�� ",
  "���󡷡������������������������",
  "�ˡ\\�[�¡ġX�����y���@�������A�B",
  "�ϡСѡҡԡӡסݡڡܡء١ա֡��",
  "�ۡ����������ޡߡ���",
  "����������������",
  "�i�j�u�v�y�z�q�r�m�n�e�f�a�b�_�`",
  "�g�h�c�d�k�l�s�t�o�p�w�x�{�|",
  "���������Ρ������I����K�L�ơ�",
  "�\\�]�^�_�`�a�b�c�d�e�f�g�h�i�j�k",
  "�l�m�n�o�p�q�r�s�G�K�N�S�U�X�Z�[",
  "��������������������"
};

static char *BIG_mode[13] = {
  "���I",
  "�϶�",
  "�аO",
  "�нu",
  "�Ƥ@",
  "�ƤG",
  "�b�Y",
  "�A�@",
  "�A�G",
  "��L",
  "�Ƥ@",
  "�ƤG",
  "�Ʀr"
};

static char *table[8] = {
  "�x�w�|�r�}�u�q�t�z�s�{",
  "����������������������",
  "���w������������������",
  "�x�����������������",
  "�x�w���r���u�q�t�~�s��",
  "�������䢣������~�ޢ�",
  "���w������������������",
  "�x�����������������"
};

static char *table_mode[6] = {
  "����",
  "�s��",
  "�q",
  "��",
  "��",
  "��"
};


/* �O����޲z�P�s��B�z */
static void
indigestion(int i)
{
    fprintf(stderr, "�Y������ %d\n", i);
}

void init_edit_buffer(editor_internal_t *buf)
{
    buf->firstline = NULL;
    buf->lastline = NULL;
    buf->currline = NULL;
    buf->blockline = NULL;
    buf->top_of_win = NULL;
    buf->deleted_lines = NULL;

    buf->insert_character = 1;
    buf->redraw_everything = 1;
    buf->indent_mode = 0;
    buf->line_dirty = 0;
    buf->currpnt = 0;
    buf->totaln = 0;
    buf->my_ansimode = 0;
    buf->phone_mode = 0;
    buf->phone_mode0 = 0;
    buf->blockln = -1;
    buf->insert_c = ' ';
}

static void enter_edit_buffer(void)
{
    editor_internal_t *p = curr_buf;
    curr_buf = (editor_internal_t *)malloc(sizeof(editor_internal_t));
    curr_buf->prev = p;
}

static void exit_edit_buffer(void)
{
    editor_internal_t *p = curr_buf;
    curr_buf = p->prev;
    free(p);
}

/* Thor: ansi �y���ഫ  for color �s��Ҧ� */
static int
ansi2n(int ansix, textline_t * line)
{
    register char  *data, *tmp;
    register char   ch;

    data = tmp = line->data;

    while (*tmp) {
	if (*tmp == KEY_ESC) {
	    while ((ch = *tmp) && !isalpha((int)ch))
		tmp++;
	    if (ch)
		tmp++;
	    continue;
	}
	if (ansix <= 0)
	    break;
	tmp++;
	ansix--;
    }
    return tmp - data;
}

static short
n2ansi(short nx, textline_t * line)
{
    register short  ansix = 0;
    register char  *tmp, *nxp;
    register char   ch;

    tmp = nxp = line->data;
    nxp += nx;

    while (*tmp) {
	if (*tmp == KEY_ESC) {
	    while ((ch = *tmp) && !isalpha((int)ch))
		tmp++;
	    if (ch)
		tmp++;
	    continue;
	}
	if (tmp >= nxp)
	    break;
	tmp++;
	ansix++;
    }
    return ansix;
}

/* �ù��B�z�G���U�T���B��ܽs�褺�e */
static void
edit_msg()
{
    const char *edit_mode[2] = {"���N", "���J"};
    register int n = curr_buf->currpnt;
    int i;

    if (curr_buf->my_ansimode)		/* Thor: �@ ansi �s�� */
	n = n2ansi(n, curr_buf->currline);
    n++;
    if(curr_buf->phone_mode) {
	move(b_lines - 1, 0);
	clrtoeol();
	if(curr_buf->phone_mode < 20) {
	    prints("\033[1;46m�i%s��J�j ", BIG_mode[curr_buf->phone_mode - 1]);
	    for (i = 0;i < 16;i++)
		if (i < strlen(BIG5[curr_buf->phone_mode - 1]) / 2)
		    prints("\033[37m%c\033[34m%2.2s", 
			    i + 'A', BIG5[curr_buf->phone_mode - 1] + i * 2);
		else
		    outs("   ");
	    outs("\033[37m   `-=���� Z��� \033[m");
	}
	else {
	    prints("\033[1;46m�i���ø�s�j /=%s *=%s��   ",
		    table_mode[(curr_buf->phone_mode - 20) / 4], 
		    table_mode[(curr_buf->phone_mode - 20) % 4 + 2]);
	    for (i = 0;i < 11;i++)
		prints("\033[37m%c\033[34m%2.2s", i ? i + '/' : '.', 
			table[curr_buf->phone_mode - 20] + i * 2);
	    outs("\033[37m          Z���X \033[m");
	}
    }
    move(b_lines, 0);
    clrtoeol();
    prints("\033[%sm �s��峹 \033[31;47m (^Z)\033[30m���� "
	    "\033[31;47m(^P)\033[30m�Ÿ� "
	    "\033[31;47m(^G)\033[30m���J�Ϥ�w \033[31m(^X,^Q)"
	    "\033[30m���}��%s�x%c%c%c%c�� %3d:%3d \033[m",
	    "37;44",
	    edit_mode[curr_buf->insert_character ? 1 : 0],
	    curr_buf->my_ansimode ? 'A' : 'a',
	    curr_buf->indent_mode ? 'I' : 'i',
	    curr_buf->phone_mode ? 'P' : 'p',
	    curr_buf->raw_mode ? 'R' : 'r',
	    curr_buf->currln + 1, n);
}

static textline_t *
back_line(textline_t * pos, int num)
{
    while (num-- > 0) {
	register textline_t *item;

	if (pos && (item = pos->prev)) {
	    pos = item;
	    curr_buf->currln--;
	}
    }
    return pos;
}

static textline_t *
forward_line(textline_t * pos, int num)
{
    while (num-- > 0) {
	register textline_t *item;

	if (pos && (item = pos->next)) {
	    pos = item;
	    curr_buf->currln++;
	}
    }
    return pos;
}

static int
getlineno()
{
    int             cnt = 0;
    textline_t     *p = curr_buf->currline;

    while (p && (p != curr_buf->top_of_win)) {
	cnt++;
	p = p->prev;
    }
    return cnt;
}

static char    *
killsp(char *s)
{
    while (*s == ' ')
	s++;
    return s;
}

static textline_t *
alloc_line(short length)
{
    register textline_t *p;

    if ((p = (textline_t *) malloc(length + sizeof(textline_t)))) {
	memset(p, 0, length + sizeof(textline_t));
#ifdef DEBUG
	p->mlength = length;
#endif
	return p;
    }
    indigestion(13);
    abort_bbs(0);
    return NULL;
}

/* append p after line in list. keeps up with last line */
static void
append(textline_t * p, textline_t * line)
{
    register textline_t *n;

    if ((p->next = n = line->next))
	n->prev = p;
    else
	curr_buf->lastline = p;
    line->next = p;
    p->prev = line;
}

/*
 * delete_line deletes 'line' from the list, and maintains the lastline, and
 * firstline pointers.
 */

static void
delete_line(textline_t * line)
{
    register textline_t *p = line->prev;
    register textline_t *n = line->next;

    if (!p && !n) {
	line->data[0] = line->len = 0;
	return;
    }
    if (n)
	n->prev = p;
    else
	curr_buf->lastline = p;
    if (p)
	p->next = n;
    else
	curr_buf->firstline = n;
    strcat(line->data, "\n");
    line->prev = curr_buf->deleted_lines;
    curr_buf->deleted_lines = line;
    curr_buf->totaln--;
}

static int
ask(char *prompt)
{
    int             ch;

    move(0, 0);
    clrtoeol();
    standout();
    outs(prompt);
    standend();
    ch = igetch();
    move(0, 0);
    clrtoeol();
    return (ch);
}

static int
indent_spcs()
{
    textline_t     *p;
    int             spcs;

    if (!curr_buf->indent_mode)
	return 0;

    for (p = curr_buf->currline; p; p = p->prev) {
	for (spcs = 0; p->data[spcs] == ' '; ++spcs);
	if (p->data[spcs])
	    return spcs;
    }
    return 0;
}

static textline_t *
adjustline(textline_t *oldp, short len)
{
    /* adjustline(oldp, len);
     * �ΨӱN oldp ���쪺���@��, ���s�ץ��� len�o���.
     * �b�o��@�@���F�⦸�� memcpy() , �Ĥ@���q heap ���� stack ,
     * ���ӰO���� free() ��, �S���s�b stack�W malloc() �@��,
     * �M��A�����^��.
     * �D�n�O�� sbrk() �[��쪺���G, �o�ˤl�~�u�����Y��O����ζq.
     * �Ԩ� /usr/share/doc/papers/malloc.ascii.gz (in FreeBSD)
     */
    char tmpl[sizeof(textline_t) + WRAPMARGIN];
    textline_t *newp;
    memcpy(tmpl, oldp, oldp->len + sizeof(textline_t));
    free(oldp);

    newp = alloc_line(len);
    memcpy(newp, tmpl, len + sizeof(textline_t));
#ifdef DEBUG
    newp->mlength = len;
#endif
    if( oldp == curr_buf->firstline ) curr_buf->firstline = newp;
    if( oldp == curr_buf->lastline )  curr_buf->lastline  = newp;
    if( oldp == curr_buf->currline )  curr_buf->currline  = newp;
    if( oldp == curr_buf->blockline ) curr_buf->blockline = newp;
    if( oldp == curr_buf->top_of_win) curr_buf->top_of_win= newp;
    if( newp->prev != NULL ) newp->prev->next = newp;
    if( newp->next != NULL ) newp->next->prev = newp;
    //    vmsg("adjust %x to %x, length: %d", (int)oldp, (int)newp, len);
    return newp;
}

/* split 'line' right before the character pos */
static textline_t *
split(textline_t * line, int pos)
{
    if (pos <= line->len) {
	register textline_t *p = alloc_line(WRAPMARGIN);
	register char  *ptr;
	int             spcs = indent_spcs();

	curr_buf->totaln++;

	p->len = line->len - pos + spcs;
	line->len = pos;

	memset(p->data, ' ', spcs);
	p->data[spcs] = 0;
	strcat(p->data, (ptr = line->data + pos));
	ptr[0] = '\0';
	append(p, line);
	line = adjustline(line, line->len);
	if (line == curr_buf->currline && pos <= curr_buf->currpnt) {
	    curr_buf->currline = p;
	    if (pos == curr_buf->currpnt)
		curr_buf->currpnt = spcs;
	    else
		curr_buf->currpnt -= pos;
	    curr_buf->curr_window_line++;
	    curr_buf->currln++;
	}
	curr_buf->redraw_everything = YEA;
    }
    return line;
}

static void
insert_char(int ch)
{
    register textline_t *p = curr_buf->currline;
    register int    i = p->len;
    register char  *s;
    int             wordwrap = YEA;

    if (curr_buf->currpnt > i) {
	indigestion(1);
	return;
    }
    if (curr_buf->currpnt < i && !curr_buf->insert_character) {
	p->data[curr_buf->currpnt++] = ch;
	/* Thor: ansi �s��, �i�Hoverwrite, ���\�� ansi code */
	if (curr_buf->my_ansimode)
	    curr_buf->currpnt = ansi2n(n2ansi(curr_buf->currpnt, p), p);
    } else {
	while (i >= curr_buf->currpnt) {
	    p->data[i + 1] = p->data[i];
	    i--;
	}
	p->data[curr_buf->currpnt++] = ch;
	i = ++(p->len);
    }
    if (i < WRAPMARGIN)
	return;
    s = p->data + (i - 1);
    while (s != p->data && *s == ' ')
	s--;
    while (s != p->data && *s != ' ')
	s--;
    if (s == p->data) {
	wordwrap = NA;
	s = p->data + (i - 2);
    }
    p = split(p, (s - p->data) + 1);
    p = p->next;
    i = p->len;
    if (wordwrap && i >= 1) {
	if (p->data[i - 1] != ' ') {
	    p->data[i] = ' ';
	    p->data[i + 1] = '\0';
	    p->len++;
	}
    }
}
static void
insert_dchar(const char *dchar) 
{
    insert_char(*dchar);
    insert_char(*(dchar+1));
}

static void
insert_string(const char *str)
{
    int             ch;

    while ((ch = *str++)) {
	if (isprint2(ch) || ch == '\033')
	    insert_char(ch);
	else if (ch == '\t') {
	    do {
		insert_char(' ');
	    } while (curr_buf->currpnt & 0x7);
	} else if (ch == '\n')
	    split(curr_buf->currline, curr_buf->currpnt);
    }
}

static int
undelete_line()
{
    textline_t     *p = curr_buf->deleted_lines;
    editor_internal_t   tmp;

    if (!curr_buf->deleted_lines)
	return 0;

    tmp.currline = curr_buf->currline;
    tmp.top_of_win = curr_buf->top_of_win;
    tmp.currpnt = curr_buf->currpnt;
    tmp.currln = curr_buf->currln;
    tmp.curr_window_line = curr_buf->curr_window_line;
    tmp.indent_mode = curr_buf->indent_mode;

    curr_buf->indent_mode = 0;
    insert_string(curr_buf->deleted_lines->data);
    curr_buf->indent_mode = tmp.indent_mode;
    curr_buf->deleted_lines = curr_buf->deleted_lines->prev;
    free(p);

    curr_buf->currline = tmp.currline;
    curr_buf->top_of_win = tmp.top_of_win;
    curr_buf->currpnt = tmp.currpnt;
    curr_buf->currln = tmp.currln;
    curr_buf->curr_window_line = tmp.curr_window_line;
    return 0;
}

/*
 * 1) lines were joined and one was deleted 2) lines could not be joined 3)
 * next line is empty returns false if: 1) Some of the joined line wrapped
 */
static int
join(textline_t * line)
{
    register textline_t *n;
    register int    ovfl;

    if (!(n = line->next))
	return YEA;
    if (!*killsp(n->data))
	return YEA;

    ovfl = line->len + n->len - WRAPMARGIN;
    if (ovfl < 0) {
	strcat(line->data, n->data);
	line->len += n->len;
	delete_line(n);
	return YEA;
    } else {
	register char  *s;

	s = n->data + n->len - ovfl - 1;
	while (s != n->data && *s == ' ')
	    s--;
	while (s != n->data && *s != ' ')
	    s--;
	if (s == n->data)
	    return YEA;
	split(n, (s - n->data) + 1);
	if (line->len + n->len >= WRAPMARGIN) {
	    indigestion(0);
	    return YEA;
	}
	join(line);
	n = line->next;
	ovfl = n->len - 1;
	if (ovfl >= 0 && ovfl < WRAPMARGIN - 2) {
	    s = &(n->data[ovfl]);
	    if (*s != ' ') {
		strcpy(s, " ");
		n->len++;
	    }
	}
	return NA;
    }
}

static void
delete_char()
{
    register int    len;

    if ((len = curr_buf->currline->len)) {
	register int    i;
	register char  *s;

	if (curr_buf->currpnt >= len) {
	    indigestion(1);
	    return;
	}
	for (i = curr_buf->currpnt, s = curr_buf->currline->data + i; i != len; i++, s++)
	    s[0] = s[1];
	curr_buf->currline->len--;
    }
}

static void
load_file(FILE * fp) /* NOTE it will fclose(fp) */
{
    int indent_mode0 = curr_buf->indent_mode;

    assert(fp);
    curr_buf->indent_mode = 0;
    while (fgets(editline, WRAPMARGIN + 2, fp))
	insert_string(editline);
    fclose(fp);
    curr_buf->indent_mode = indent_mode0;
}

/* �Ȧs�� */
char           *
ask_tmpbuf(int y)
{
    static char     fp_buf[10] = "buf.0";
    static char     msg[] = "�п�ܼȦs�� (0-9)[0]: ";

    msg[19] = fp_buf[4];
    do {
	if (!getdata(y, 0, msg, fp_buf + 4, 4, DOECHO))
	    fp_buf[4] = msg[19];
    } while (fp_buf[4] < '0' || fp_buf[4] > '9');
    return fp_buf;
}

static void
read_tmpbuf(int n)
{
    FILE           *fp;
    char            fp_tmpbuf[80];
    char            tmpfname[] = "buf.0";
    char           *tmpf;
    char            ans[4] = "y";

    if (0 <= n && n <= 9) {
	tmpfname[4] = '0' + n;
	tmpf = tmpfname;
    } else {
	tmpf = ask_tmpbuf(3);
	n = tmpf[4] - '0';
    }

    setuserfile(fp_tmpbuf, tmpf);
    if (n != 0 && n != 5 && more(fp_tmpbuf, NA) != -1)
	getdata(b_lines - 1, 0, "�T�wŪ�J��(Y/N)?[Y]", ans, sizeof(ans), LCECHO);
    if (*ans != 'n' && (fp = fopen(fp_tmpbuf, "r"))) {
	load_file(fp);
	while (curr_buf->curr_window_line >= b_lines) {
	    curr_buf->curr_window_line--;
	    curr_buf->top_of_win = curr_buf->top_of_win->next;
	}
    }
}

static void
write_tmpbuf()
{
    FILE           *fp;
    char            fp_tmpbuf[80], ans[4];
    textline_t     *p;

    setuserfile(fp_tmpbuf, ask_tmpbuf(3));
    if (dashf(fp_tmpbuf)) {
	more(fp_tmpbuf, NA);
	getdata(b_lines - 1, 0, "�Ȧs�ɤw����� (A)���[ (W)�мg (Q)�����H[A] ",
		ans, sizeof(ans), LCECHO);

	if (ans[0] == 'q')
	    return;
    }
    if ((fp = fopen(fp_tmpbuf, (ans[0] == 'w' ? "w" : "a+")))) {
	for (p = curr_buf->firstline; p; p = p->next) {
	    if (p->next || p->data[0])
		fprintf(fp, "%s\n", p->data);
	}
	fclose(fp);
    }
}

static void
erase_tmpbuf()
{
    char            fp_tmpbuf[80];
    char            ans[4] = "n";

    setuserfile(fp_tmpbuf, ask_tmpbuf(3));
    if (more(fp_tmpbuf, NA) != -1)
	getdata(b_lines - 1, 0, "�T�w�R����(Y/N)?[N]",
		ans, sizeof(ans), LCECHO);
    if (*ans == 'y')
	unlink(fp_tmpbuf);
}

/**
 * �s�边�۰ʳƥ�
 */
void
auto_backup()
{
    if (curr_buf == NULL)
	return;

    if (curr_buf->currline) {
	FILE           *fp;
	textline_t     *p, *v;
	char            bakfile[64];
	int             count = 0;

	setuserfile(bakfile, fp_bak);
	if ((fp = fopen(bakfile, "w"))) {
	    for (p = curr_buf->firstline; p != NULL && count < 512; p = v, count++) {
		v = p->next;
		fprintf(fp, "%s\n", p->data);
		free(p);
	    }
	    fclose(fp);
	}
	curr_buf->currline = NULL;
    }
}

/**
 * ���^�s�边�ƥ�
 */
void
restore_backup()
{
    char            bakfile[80], buf[80];

    setuserfile(bakfile, fp_bak);
    if (dashf(bakfile)) {
	stand_title("�s�边�۰ʴ_��");
	getdata(1, 0, "�z���@�g�峹�|�������A(S)�g�J�Ȧs�� (Q)��F�H[S] ",
		buf, 4, LCECHO);
	if (buf[0] != 'q') {
	    setuserfile(buf, ask_tmpbuf(3));
	    Rename(bakfile, buf);
	} else
	    unlink(bakfile);
    }
}

/* �ޥΤ峹 */
static int
garbage_line(char *str)
{
    int             qlevel = 0;

    while (*str == ':' || *str == '>') {
	if (*(++str) == ' ')
	    str++;
	if (qlevel++ >= 1)
	    return 1;
    }
    while (*str == ' ' || *str == '\t')
	str++;
    if (qlevel >= 1) {
	if (!strncmp(str, "�� ", 3) || !strncmp(str, "==>", 3) ||
	    strstr(str, ") ����:\n"))
	    return 1;
    }
    return (*str == '\n');
}

static void
do_quote()
{
    int             op;
    char            buf[256];

    getdata(b_lines - 1, 0, "�аݭn�ޥέ���(Y/N/All/Repost)�H[Y] ",
	    buf, 3, LCECHO);
    op = buf[0];

    if (op != 'n') {
	FILE           *inf;

	if ((inf = fopen(quote_file, "r"))) {
	    char           *ptr;
	    int             indent_mode0 = curr_buf->indent_mode;

	    fgets(buf, 256, inf);
	    if ((ptr = strrchr(buf, ')')))
		ptr[1] = '\0';
	    else if ((ptr = strrchr(buf, '\n')))
		ptr[0] = '\0';

	    if ((ptr = strchr(buf, ':'))) {
		char           *str;

		while (*(++ptr) == ' ');

		/* ����o�ϡA���o author's address */
		if ((curredit & EDIT_BOTH) && (str = strchr(quote_user, '.'))) {
		    strcpy(++str, ptr);
		    str = strchr(str, ' ');
		    assert(str);
		    str[0] = '\0';
		}
	    } else
		ptr = quote_user;

	    curr_buf->indent_mode = 0;
	    insert_string("�� �ޭz�m");
	    insert_string(ptr);
	    insert_string("�n���ʨ��G\n");

	    if (op != 'a')	/* �h�� header */
		while (fgets(buf, 256, inf) && buf[0] != '\n');

	    if (op == 'a')
		while (fgets(buf, 256, inf)) {
		    insert_char(':');
		    insert_char(' ');
		    insert_string(Ptt_prints(buf, STRIP_ALL));
		}
	    else if (op == 'r')
		while (fgets(buf, 256, inf))
		    insert_string(Ptt_prints(buf, NO_RELOAD));
	    else {
		if (curredit & EDIT_LIST)	/* �h�� mail list �� header */
		    while (fgets(buf, 256, inf) && (!strncmp(buf, "�� ", 3)));
		while (fgets(buf, 256, inf)) {
		    if (!strcmp(buf, "--\n"))
			break;
		    if (!garbage_line(buf)) {
			insert_char(':');
			insert_char(' ');
			insert_string(Ptt_prints(buf, STRIP_ALL));
		    }
		}
	    }
	    curr_buf->indent_mode = indent_mode0;
	    fclose(inf);
	}
    }
}

/* �f�d user �ި����ϥ� */
static int
check_quote()
{
    register textline_t *p = curr_buf->firstline;
    register char  *str;
    int             post_line;
    int             included_line;

    post_line = included_line = 0;
    while (p) {
	if (!strcmp(str = p->data, "--"))
	    break;
	if (str[1] == ' ' && ((str[0] == ':') || (str[0] == '>')))
	    included_line++;
	else {
	    while (*str == ' ' || *str == '\t')
		str++;
	    if (*str)
		post_line++;
	}
	p = p->next;
    }

    if ((included_line >> 2) > post_line) {
	move(4, 0);
	outs("���g�峹���ި���ҶW�L 80%�A�бz���ǷL���ץ��G\n\n"
	     "\033[1;33m1) �W�[�@�Ǥ峹 ��  2) �R�������n���ި�\033[m");
	{
	    char            ans[4];

	    getdata(12, 12, "(E)�~��s�� (W)�j��g�J�H[E] ",
		    ans, sizeof(ans), LCECHO);
	    if (ans[0] == 'w')
		return 0;
	}
	return 1;
    }
    return 0;
}

/* �ɮ׳B�z�GŪ�ɡB�s�ɡB���D�Bñ�W�� */
static void
read_file(char *fpath)
{
    FILE           *fp;

    if ((fp = fopen(fpath, "r")) == NULL) {
	if ((fp = fopen(fpath, "w+"))) {
	    fclose(fp);
	    return;
	}
	indigestion(4);
	abort_bbs(0);
    }
    load_file(fp);
}

void
write_header(FILE * fp,  int ifuseanony)
{

    if (curredit & EDIT_MAIL || curredit & EDIT_LIST) {
	fprintf(fp, "%s %s (%s)\n", str_author1, cuser.userid,
		cuser.username
	);
    } else {
	char           *ptr;
	struct {
	    char            author[IDLEN + 1];
	    char            board[IDLEN + 1];
	    char            title[66];
	    time_t          date;	/* last post's date */
	    int             number;	/* post number */
	}               postlog;

	memset(&postlog, 0, sizeof(postlog));
	strlcpy(postlog.author, cuser.userid, sizeof(postlog.author));
	ifuseanony = 0;
#ifdef HAVE_ANONYMOUS
	if (currbrdattr & BRD_ANONYMOUS) {
	    int             defanony = (currbrdattr & BRD_DEFAULTANONYMOUS);
	    if (defanony)
		getdata(3, 0, "�п�J�A�Q�Ϊ�ID�A�]�i������[Enter]�A"
		 "�άO��[r]�ίu�W�G", real_name, sizeof(real_name), DOECHO);
	    else
		getdata(3, 0, "�п�J�A�Q�Ϊ�ID�A�]�i������[Enter]�ϥέ�ID�G",
			real_name, sizeof(real_name), DOECHO);
	    if (!real_name[0] && defanony) {
		strlcpy(real_name, "Anonymous", sizeof(real_name));
		strlcpy(postlog.author, real_name, sizeof(postlog.author));
		ifuseanony = 1;
	    } else {
		if (!strcmp("r", real_name) || (!defanony && !real_name[0]))
		    strlcpy(postlog.author, cuser.userid, sizeof(postlog.author));
		else {
		    snprintf(postlog.author, sizeof(postlog.author),
			     "%s.", real_name);
		    ifuseanony = 1;
		}
	    }
	}
#endif
	strlcpy(postlog.board, currboard, sizeof(postlog.board));
	ptr = save_title;
	if (!strncmp(ptr, str_reply, 4))
	    ptr += 4;
	strncpy(postlog.title, ptr, 65);
	postlog.date = now;
	postlog.number = 1;
	append_record(".post", (fileheader_t *) & postlog, sizeof(postlog));
#ifdef HAVE_ANONYMOUS
	if (currbrdattr & BRD_ANONYMOUS) {
	    int             defanony = (currbrdattr & BRD_DEFAULTANONYMOUS);

	    fprintf(fp, "%s %s (%s) %s %s\n", str_author1, postlog.author,
		    (((!strcmp(real_name, "r") && defanony) ||
		      (!real_name[0] && (!defanony))) ? cuser.username :
		     "�q�q�ڬO�� ? ^o^"),
		    local_article ? str_post2 : str_post1, currboard);
	} else {
	    fprintf(fp, "%s %s (%s) %s %s\n", str_author1, cuser.userid,
		    cuser.username,
		    local_article ? str_post2 : str_post1, currboard);
	}
#else				/* HAVE_ANONYMOUS */
	fprintf(fp, "%s %s (%s) %s %s\n", str_author1, cuser.userid,
		cuser.username,
		local_article ? str_post2 : str_post1, currboard);
#endif				/* HAVE_ANONYMOUS */

    }
    save_title[72] = '\0';
    fprintf(fp, "���D: %s\n�ɶ�: %s\n", save_title, ctime(&now));
}

void
addsignature(FILE * fp, int ifuseanony)
{
    FILE           *fs;
    int             i, num;
    char            buf[WRAPMARGIN + 1];
    char            fpath[STRLEN];

    static char     msg[] = "�п��ñ�W�� (1-9, 0=���[ X=�H��)[X]: ";
    char            ch;

    if (!strcmp(cuser.userid, STR_GUEST)) {
	fprintf(fp, "\n--\n�� �o�H�� :" BBSNAME "(" MYHOSTNAME
		") \n�� From: %s\n", fromhost);
	return;
    }
    if (ifuseanony) {
	num = showsignature(fpath, &i);
	if (num){
	    msg[34] = ch = isdigit(cuser.signature) ? cuser.signature : 'X';
	    getdata(0, 0, msg, buf, 4, DOECHO);

	    if (!buf[0])
		buf[0] = ch;

	    if (isdigit((int)buf[0]))
		ch = buf[0];
	    else
		ch = '1' + rand() % num;
	    cuser.signature = buf[0];

	    if (ch != '0') {
		fpath[i] = ch;
		if ((fs = fopen(fpath, "r"))) {
		    fputs("\n--\n", fp);
		    for (i = 0; i < MAX_SIGLINES &&
			    fgets(buf, sizeof(buf), fs); i++)
			fputs(buf, fp);
		    fclose(fs);
		}
	    }
	}
    }
#ifdef HAVE_ORIGIN
#ifdef HAVE_ANONYMOUS
    if (ifuseanony)
	fprintf(fp, "\n--\n�� �o�H��: " BBSNAME "(" MYHOSTNAME
		") \n�� From: %s\n", "�ʦW�ѨϪ��a");
    else {
	char            temp[33];

	strncpy(temp, fromhost, 31);
	temp[32] = '\0';
	fprintf(fp, "\n--\n�� �o�H��: " BBSNAME "(" MYHOSTNAME
		") \n�� From: %s\n", temp);
    }
#else
    strncpy(temp, fromhost, 15);
    fprintf(fp, "\n--\n�� �o�H��: " BBSNAME "(" MYHOSTNAME
	    ") \n�� From: %s\n", temp);
#endif
#endif
}

static int
write_file(char *fpath, int saveheader, int *islocal)
{
    struct tm      *ptime;
    FILE           *fp = NULL;
    textline_t     *p, *v;
    char            ans[TTLEN], *msg;
    int             aborted = 0, line = 0, checksum[3], sum = 0, po = 1;

    stand_title("�ɮ׳B�z");
    if (currstat == SMAIL)
	msg = "[S]�x�s (A)��� (T)����D (E)�~�� (R/W/D)Ū�g�R�Ȧs�ɡH";
    else if (local_article)
	msg = "[L]�����H�� (S)�x�s (A)��� (T)����D (E)�~�� "
	    "(R/W/D)Ū�g�R�Ȧs�ɡH";
    else
	msg = "[S]�x�s (L)�����H�� (A)��� (T)����D (E)�~�� "
	    "(R/W/D)Ū�g�R�Ȧs�ɡH";
    getdata(1, 0, msg, ans, 2, LCECHO);

    switch (ans[0]) {
    case 'a':
	outs("�峹\033[1m �S�� \033[m�s�J");
	aborted = -1;
	break;
    case 'r':
	read_tmpbuf(-1);
    case 'e':
	return KEEP_EDITING;
    case 'w':
	write_tmpbuf();
	return KEEP_EDITING;
    case 'd':
	erase_tmpbuf();
	return KEEP_EDITING;
    case 't':
	move(3, 0);
	prints("�¼��D�G%s", save_title);
	strlcpy(ans, save_title, sizeof(ans));
	if (getdata_buf(4, 0, "�s���D�G", ans, sizeof(ans), DOECHO))
	    strlcpy(save_title, ans, sizeof(save_title));
	return KEEP_EDITING;
    case 's':
	if (!HAS_PERM(PERM_LOGINOK)) {
	    local_article = 1;
	    move(2, 0);
	    outs("�z�|���q�L�����T�{�A�u�� Local Save�C\n");
	    pressanykey();
	} else
	    local_article = 0;
	break;
    case 'l':
	local_article = 1;
    }

    if (!aborted) {

	if (saveheader && !(curredit & EDIT_MAIL) && check_quote())
	    return KEEP_EDITING;

	if (!(*fpath))
	    setuserfile(fpath, "ve_XXXXXX");
	if ((fp = fopen(fpath, "w")) == NULL) {
	    indigestion(5);
	    abort_bbs(0);
	}
	if (saveheader)
	    write_header(fp, curr_buf->ifuseanony);
    }
    for (p = curr_buf->firstline; p; p = v) {
	v = p->next;
	if (!aborted) {
	    assert(fp);
	    msg = p->data;
	    if (v || msg[0]) {
		trim(msg);

		line++;
		if (currstat == POSTING && po ) {
		    saveheader = str_checksum(msg);
		    if (saveheader) {
			if (postrecord.last_bid != currbid &&
			    postrecord.checksum[po] == saveheader) {
			    po++;
			    if (po > 3) {
				postrecord.times++;
				postrecord.last_bid = currbid;
				po = 0;
			    }
			} else
			    po = 1;
			if (currstat == POSTING && line >= curr_buf->totaln / 2 &&
			    sum < 3) {
			    checksum[sum++] = saveheader;
			}
		    }
		}
		fprintf(fp, "%s\n", msg);
	    }
	}
	free(p);
    }
    curr_buf->currline = NULL;

    if (postrecord.times > MAX_CROSSNUM-1 && hbflcheck(currbid, currutmp->uid))
	anticrosspost();

    if (po && sum == 3) {
	memcpy(&postrecord.checksum[1], checksum, sizeof(int) * 3);
        if(postrecord.last_bid != currbid)
	           postrecord.times = 0;
    }
    if (!aborted) {
	if (islocal)
	    *islocal = local_article;
	if (currstat == POSTING || currstat == SMAIL)
	    addsignature(fp, curr_buf->ifuseanony);
	else if (currstat == REEDIT
#ifndef ALL_REEDIT_LOG
		 && strcmp(currboard, "SYSOP") == 0
#endif
	    ) {
	    ptime = localtime(&now);
	    fprintf(fp,
		    "�� �s��: %-15s �Ӧ�: %-20s (%02d/%02d %02d:%02d)\n",
		    cuser.userid, fromhost,
		    ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour, ptime->tm_min);
	}
	fclose(fp);

	if (local_article && (currstat == POSTING))
	    return 0;
	return 0;
    }
    return aborted;
}


static void
display_buffer()
{
    register textline_t *p;
    register int    i;
    int             inblock;
    char            buf[WRAPMARGIN + 2];
    int             min, max;

    if (curr_buf->currpnt > curr_buf->blockpnt) {
	min = curr_buf->blockpnt;
	max = curr_buf->currpnt;
    } else {
	min = curr_buf->currpnt;
	max = curr_buf->blockpnt;
    }

    for (p = curr_buf->top_of_win, i = 0; i < b_lines; i++) {
	move(i, 0);
	clrtoeol();
	if (curr_buf->blockln >= 0 &&
	((curr_buf->blockln <= curr_buf->currln && curr_buf->blockln <= (curr_buf->currln - curr_buf->curr_window_line + i) &&
	  (curr_buf->currln - curr_buf->curr_window_line + i) <= curr_buf->currln) ||
	 (curr_buf->currln <= (curr_buf->currln - curr_buf->curr_window_line + i) &&
	  (curr_buf->currln - curr_buf->curr_window_line + i) <= curr_buf->blockln))) {
	    outs("\033[7m");
	    inblock = 1;
	} else
	    inblock = 0;
	if (p) {
	    if (curr_buf->my_ansimode)
		if (curr_buf->currln == curr_buf->blockln && p == curr_buf->currline && max > min) {
		    outs("\033[m");
		    strncpy(buf, p->data, min);
		    buf[min] = 0;
		    outs(buf);
		    outs("\033[7m");
		    strncpy(buf, p->data + min, max - min);
		    buf[max - min] = 0;
		    outs(buf);
		    outs("\033[m");
		    outs(p->data + max);
		} else
		    outs(p->data);
	    else if (curr_buf->currln == curr_buf->blockln && p == curr_buf->currline && max > min) {
		outs("\033[m");
		strncpy(buf, p->data, min);
		buf[min] = 0;
		edit_outs(buf);
		outs("\033[7m");
		strncpy(buf, p->data + min, max - min);
		buf[max - min] = 0;
		edit_outs(buf);
		outs("\033[m");
		edit_outs(p->data + max);
	    } else
		edit_outs((curr_buf->edit_margin < p->len) ? &p->data[curr_buf->edit_margin] : "");
	    p = p->next;
	    if (inblock)
		outs("\033[m");
	} else
	    outc('~');
    }
    edit_msg();
}

static void
goto_line(int lino)
{
    char            buf[10];

    if (lino > 0 ||
	(getdata(b_lines - 1, 0, "���ܲĴX��:", buf, sizeof(buf), DOECHO) &&
	 sscanf(buf, "%d", &lino) && lino > 0)) {
	textline_t     *p;

	p = curr_buf->firstline;
	curr_buf->currln = lino - 1;

	while (--lino && p->next)
	    p = p->next;

	if (p)
	    curr_buf->currline = p;
	else {
	    curr_buf->currln = curr_buf->totaln;
	    curr_buf->currline = curr_buf->lastline;
	}
	curr_buf->currpnt = 0;
	if (curr_buf->currln < 11) {
	    curr_buf->top_of_win = curr_buf->firstline;
	    curr_buf->curr_window_line = curr_buf->currln;
	} else {
	    int             i;

	    curr_buf->curr_window_line = 11;
	    for (i = curr_buf->curr_window_line; i; i--)
		p = p->prev;
	    curr_buf->top_of_win = p;
	}
    }
    curr_buf->redraw_everything = YEA;
}

/*
 * mode: 0: prompt 1: forward -1: backward
 */
static void
search_str(int mode)
{
    static char     str[65];
    typedef char   *(*FPTR) ();
    static FPTR     fptr;
    char            ans[4] = "n";

    if (!mode) {
	if (getdata_buf(b_lines - 1, 0, "[�j�M]����r:",
			str, sizeof(str), DOECHO))
	    if (*str) {
		if (getdata(b_lines - 1, 0, "�Ϥ��j�p�g(Y/N/Q)? [N] ",
			    ans, sizeof(ans), LCECHO) && *ans == 'y')
		    fptr = strstr;
		else
		    fptr = strcasestr;
	    }
    }
    if (*str && *ans != 'q') {
	textline_t     *p;
	char           *pos = NULL;
	int             lino;

	if (mode >= 0) {
	    for (lino = curr_buf->currln, p = curr_buf->currline; p; p = p->next, lino++)
		if ((pos = fptr(p->data + (lino == curr_buf->currln ? curr_buf->currpnt + 1 : 0),
				str)) && (lino != curr_buf->currln ||
					  pos - p->data != curr_buf->currpnt))
		    break;
	} else {
	    for (lino = curr_buf->currln, p = curr_buf->currline; p; p = p->prev, lino--)
		if ((pos = fptr(p->data, str)) &&
		    (lino != curr_buf->currln || pos - p->data != curr_buf->currpnt))
		    break;
	}
	if (pos) {
	    curr_buf->currline = p;
	    curr_buf->currln = lino;
	    curr_buf->currpnt = pos - p->data;
	    if (lino < 11) {
		curr_buf->top_of_win = curr_buf->firstline;
		curr_buf->curr_window_line = curr_buf->currln;
	    } else {
		int             i;

		curr_buf->curr_window_line = 11;
		for (i = curr_buf->curr_window_line; i; i--)
		    p = p->prev;
		curr_buf->top_of_win = p;
	    }
	    curr_buf->redraw_everything = YEA;
	}
    }
    if (!mode)
	curr_buf->redraw_everything = YEA;
}

static void
match_paren()
{
    char           *parens = "()[]{}";
    int             type;
    int             parenum = 0;
    char           *ptype;
    textline_t     *p;
    int             lino;
    int             c, i = 0;

    if (!(ptype = strchr(parens, curr_buf->currline->data[curr_buf->currpnt])))
	return;

    type = (ptype - parens) / 2;
    parenum = ((ptype - parens) % 2) ? -1 : 1;

    /* FIXME CRASH */
    if (parenum > 0) {
	for (lino = curr_buf->currln, p = curr_buf->currline; p; p = p->next, lino++) {
	    int len = strlen(p->data);
	    for (i = (lino == curr_buf->currln) ? curr_buf->currpnt + 1 : 0; i < len; i++) {
		if (p->data[i] == '/' && p->data[++i] == '*') {
		    ++i;
		    while (1) {
			while (i < len &&
			       !(p->data[i] == '*' && p->data[i + 1] == '/')) {
			    i++;
			}
			if (i >= len && p->next) {
			    p = p->next;
			    len = strlen(p->data);
			    ++lino;
			    i = 0;
			} else
			    break;
		    }
		} else if ((c = p->data[i]) == '\'' || c == '"') {
		    while (1) {
			while (i < len - 1) {
			    if (p->data[++i] == '\\' && (size_t)i < len - 2)
				++i;
			    else if (p->data[i] == c)
				goto end_quote;
			}
			if ((size_t)i >= len - 1 && p->next) {
			    p = p->next;
			    len = strlen(p->data);
			    ++lino;
			    i = -1;
			} else
			    break;
		    }
	    end_quote:
		    ;
		} else if ((ptype = strchr(parens, p->data[i])) &&
			   (ptype - parens) / 2 == type) {
		    if (!(parenum += ((ptype - parens) % 2) ? -1 : 1))
			goto p_outscan;
		}
	    }
	}
    } else {
	for (lino = curr_buf->currln, p = curr_buf->currline; p; p = p->prev, lino--) {
	    int len = strlen(p->data);
	    for (i = ((lino == curr_buf->currln) ?  curr_buf->currpnt - 1 : len - 1); i >= 0; i--) {
		if (p->data[i] == '/' && p->data[--i] == '*' && i > 0) {
		    --i;
		    while (1) {
			while (i > 0 &&
				!(p->data[i] == '*' && p->data[i - 1] == '/')) {
			    i--;
			}
			if (i <= 0 && p->prev) {
			    p = p->prev;
			    len = strlen(p->data);
			    --lino;
			    i = len - 1;
			} else
			    break;
		    }
		} else if ((c = p->data[i]) == '\'' || c == '"') {
		    while (1) {
			while (i > 0)
			    if (i > 1 && p->data[i - 2] == '\\')
				i -= 2;
			    else if ((p->data[--i]) == c)
				goto begin_quote;
			if (i <= 0 && p->prev) {
			    p = p->prev;
			    len = strlen(p->data);
			    --lino;
			    i = len;
			} else
			    break;
		    }
begin_quote:
		    ;
		} else if ((ptype = strchr(parens, p->data[i])) &&
			(ptype - parens) / 2 == type) {
		    if (!(parenum += ((ptype - parens) % 2) ? -1 : 1))
			goto p_outscan;
		}
	    }
	}
    }
p_outscan:
    if (!parenum) {
	int             top = curr_buf->currln - curr_buf->curr_window_line;
	int             bottom = curr_buf->currln - curr_buf->curr_window_line + b_lines - 1;

	curr_buf->currpnt = i;
	curr_buf->currline = p;
	curr_buf->curr_window_line += lino - curr_buf->currln;
	curr_buf->currln = lino;

	if (lino < top || lino > bottom) {
	    if (lino < 11) {
		curr_buf->top_of_win = curr_buf->firstline;
		curr_buf->curr_window_line = curr_buf->currln;
	    } else {
		int             i;

		curr_buf->curr_window_line = 11;
		for (i = curr_buf->curr_window_line; i; i--)
		    p = p->prev;
		curr_buf->top_of_win = p;
	    }
	    curr_buf->redraw_everything = YEA;
	}
    }
}

static void
block_del(int hide)
{
    if (curr_buf->blockln < 0) {
	curr_buf->blockln = curr_buf->currln;
	curr_buf->blockpnt = curr_buf->currpnt;
	curr_buf->blockline = curr_buf->currline;
    } else {
	char            fp_tmpbuf[80];
	FILE           *fp;
	textline_t     *begin, *end, *p;
	char            tmpfname[10] = "buf.0";
	char            ans[6] = "w+n";

	move(b_lines - 1, 0);
	clrtoeol();
	if (hide == 1)
	    tmpfname[4] = 'q';
	else if (!hide && !getdata(b_lines - 1, 0, "��϶����ܼȦs�� "
				   "(0:Cut, 5:Copy, 6-9, q: Cancel)[0] ",
				   tmpfname + 4, 4, LCECHO))
	    tmpfname[4] = '0';
	if (tmpfname[4] < '0' || tmpfname[4] > '9')
	    tmpfname[4] = 'q';
	if ('1' <= tmpfname[4] && tmpfname[4] <= '9') {
	    setuserfile(fp_tmpbuf, tmpfname);
	    if (tmpfname[4] != '5' && dashf(fp_tmpbuf)) {
		more(fp_tmpbuf, NA);
		getdata(b_lines - 1, 0, "�Ȧs�ɤw����� (A)���[ (W)�мg "
			"(Q)�����H[W] ", ans, 2, LCECHO);
		if (*ans == 'q')
		    tmpfname[4] = 'q';
		else if (*ans != 'a')
		    *ans = 'w';
	    }
	    if (tmpfname[4] != '5') {
		getdata(b_lines - 1, 0, "�R���϶�(Y/N)?[N] ",
			ans + 2, 4, LCECHO);
		if (ans[2] != 'y')
		    ans[2] = 'n';
	    }
	} else if (hide != 3)
	    ans[2] = 'y';

	tmpfname[5] = ans[1] = ans[3] = 0;
	if (tmpfname[4] != 'q') {
	    if (curr_buf->currln >= curr_buf->blockln) {
		begin = curr_buf->blockline;
		end = curr_buf->currline;
		if (ans[2] == 'y' && !(begin == end && curr_buf->currpnt != curr_buf->blockpnt)) {
		    curr_buf->curr_window_line -= (curr_buf->currln - curr_buf->blockln);
		    if (curr_buf->curr_window_line < 0) {
			curr_buf->curr_window_line = 0;
			if (end->next)
			    (curr_buf->top_of_win = end->next)->prev = begin->prev;
			else
			    curr_buf->top_of_win = (curr_buf->lastline = begin->prev);
		    }
		    curr_buf->currln -= (curr_buf->currln - curr_buf->blockln);
		}
	    } else {
		begin = curr_buf->currline;
		end = curr_buf->blockline;
	    }
	    if (ans[2] == 'y' && !(begin == end && curr_buf->currpnt != curr_buf->blockpnt)) {
		if (begin->prev)
		    begin->prev->next = end->next;
		else if (end->next)
		    curr_buf->top_of_win = curr_buf->firstline = end->next;
		else {
		    curr_buf->currline = curr_buf->top_of_win = curr_buf->firstline =
			curr_buf->lastline = alloc_line(WRAPMARGIN);
		    curr_buf->currln = curr_buf->curr_window_line = curr_buf->edit_margin = 0;
		}

		if (end->next)
		    (curr_buf->currline = end->next)->prev = begin->prev;
		else if (begin->prev) {
		    curr_buf->currline = (curr_buf->lastline = begin->prev);
		    curr_buf->currln--;
		    if (curr_buf->curr_window_line > 0)
			curr_buf->curr_window_line--;
		}
	    }
	    setuserfile(fp_tmpbuf, tmpfname);
	    if ((fp = fopen(fp_tmpbuf, ans))) {
		if (begin == end && curr_buf->currpnt != curr_buf->blockpnt) {
		    char            buf[WRAPMARGIN + 2];

		    if (curr_buf->currpnt > curr_buf->blockpnt) {
			strlcpy(buf, begin->data + curr_buf->blockpnt, sizeof(buf));
			buf[curr_buf->currpnt - curr_buf->blockpnt] = 0;
		    } else {
			strlcpy(buf, begin->data + curr_buf->currpnt, sizeof(buf));
			buf[curr_buf->blockpnt - curr_buf->currpnt] = 0;
		    }
		    fputs(buf, fp);
		} else {
		    for (p = begin; p != end; p = p->next)
			fprintf(fp, "%s\n", p->data);
		    fprintf(fp, "%s\n", end->data);
		}
		fclose(fp);
	    }
	    if (ans[2] == 'y') {
		if (begin == end && curr_buf->currpnt != curr_buf->blockpnt) {
		    int             min, max;

		    if (curr_buf->currpnt > curr_buf->blockpnt) {
			min = curr_buf->blockpnt;
			max = curr_buf->currpnt;
		    } else {
			min = curr_buf->currpnt;
			max = curr_buf->blockpnt;
		    }
		    strlcpy(begin->data + min, begin->data + max, sizeof(begin->data) - min);
		    begin->len -= max - min;
		    curr_buf->currpnt = min;
		} else {
		    for (p = begin; p != end; curr_buf->totaln--)
			free((p = p->next)->prev);
		    free(end);
		    curr_buf->totaln--;
		    curr_buf->currpnt = 0;
		}
	    }
	}
	curr_buf->blockln = -1;
	curr_buf->redraw_everything = YEA;
    }
}

static void
block_shift_left()
{
    textline_t     *begin, *end, *p;

    if (curr_buf->currln >= curr_buf->blockln) {
	begin = curr_buf->blockline;
	end = curr_buf->currline;
    } else {
	begin = curr_buf->currline;
	end = curr_buf->blockline;
    }
    p = begin;
    while (1) {
	if (p->len) {
	    strcpy(p->data, p->data + 1);
	    --p->len;
	}
	if (p == end)
	    break;
	else
	    p = p->next;
    }
    if (curr_buf->currpnt > curr_buf->currline->len)
	curr_buf->currpnt = curr_buf->currline->len;
    curr_buf->redraw_everything = YEA;
}

static void
block_shift_right()
{
    textline_t     *begin, *end, *p;

    if (curr_buf->currln >= curr_buf->blockln) {
	begin = curr_buf->blockline;
	end = curr_buf->currline;
    } else {
	begin = curr_buf->currline;
	end = curr_buf->blockline;
    }
    p = begin;
    while (1) {
	if (p->len < WRAPMARGIN) {
	    int             i = p->len + 1;

	    while (i--)
		p->data[i + 1] = p->data[i];
	    p->data[0] = curr_buf->insert_character ? ' ' : curr_buf->insert_c;
	    ++p->len;
	}
	if (p == end)
	    break;
	else
	    p = p->next;
    }
    if (curr_buf->currpnt > curr_buf->currline->len)
	curr_buf->currpnt = curr_buf->currline->len;
    curr_buf->redraw_everything = YEA;
}

static void
transform_to_color(char *line)
{
    while (line[0] && line[1])
	if (line[0] == '*' && line[1] == '[') {
	    line[0] = KEY_ESC;
	    line += 2;
	} else
	    ++line;
}

static void
block_color()
{
    textline_t     *begin, *end, *p;

    if (curr_buf->currln >= curr_buf->blockln) {
	begin = curr_buf->blockline;
	end = curr_buf->currline;
    } else {
	begin = curr_buf->currline;
	end = curr_buf->blockline;
    }
    p = begin;
    while (1) {
	transform_to_color(p->data);
	if (p == end)
	    break;
	else
	    p = p->next;
    }
    block_del(1);
}

static char* 
phone_char(char c)
{

    if (curr_buf->phone_mode > 0 && curr_buf->phone_mode < 20) {
	if (tolower(c)<'a'||(tolower(c)-'a') >= strlen(BIG5[curr_buf->phone_mode - 1]) / 2)
	    return 0;
	return BIG5[curr_buf->phone_mode - 1] + (tolower(c) - 'a') * 2;
    }
    else if (curr_buf->phone_mode >= 20) {
	if (c == '.') c = '/';

	if (c < '/' || c > '9')
	    return 0;

	return table[curr_buf->phone_mode - 20] + (c - '/') * 2;
    }
    return 0;
}


/* �s��B�z�G�D�{���B��L�B�z */
int
vedit(char *fpath, int saveheader, int *islocal)
{
    FILE           *fp1;
    char            last = 0, *pstr;	/* the last key you press */
    int             ch, ret;
    int             lastindent = -1;
    int             last_margin;
    int             mode0 = currutmp->mode;
    int             destuid0 = currutmp->destuid;
    int             money = 0;
    int             interval = 0;
    time_t          th = now;
    int             count = 0, tin = 0;
    textline_t     *oldcurrline;

    currutmp->mode = EDITING;
    currutmp->destuid = currstat;

    enter_edit_buffer();
    init_edit_buffer(curr_buf);

    oldcurrline = curr_buf->currline = curr_buf->top_of_win =
	curr_buf->firstline = curr_buf->lastline = alloc_line(WRAPMARGIN);

    if (*fpath)
	read_file(fpath);

    if (*quote_file) {
	do_quote();
	*quote_file = '\0';
	if (quote_file[79] == 'L')
	    local_article = 1;
    }
    curr_buf->currline = curr_buf->firstline;
    curr_buf->currpnt = curr_buf->currln = curr_buf->curr_window_line = curr_buf->edit_margin = last_margin = 0;

    while (1) {
	if (curr_buf->redraw_everything || curr_buf->blockln >= 0) {
	    display_buffer();
	    curr_buf->redraw_everything = NA;
	}
	if( oldcurrline != curr_buf->currline ){
	    oldcurrline = adjustline(oldcurrline, oldcurrline->len);
	    oldcurrline = curr_buf->currline = adjustline(curr_buf->currline, WRAPMARGIN);
	}
	if (curr_buf->my_ansimode)
	    ch = n2ansi(curr_buf->currpnt, curr_buf->currline);
	else
	    ch = curr_buf->currpnt - curr_buf->edit_margin;
	move(curr_buf->curr_window_line, ch);
	// XXX dont strcmp, just do it ?
	if (!curr_buf->line_dirty && strcmp(editline, curr_buf->currline->data))
	    strcpy(editline, curr_buf->currline->data);
	ch = igetch();
	/* jochang debug */
	if ((interval = (now - th))) {
	    th = now;
	    if ((char)ch != last) {
		money++;
		last = (char)ch;
	    }
	}
	if (interval && interval == tin)
          {  // Ptt : +- 1 ��]��
	    count++;
            if(count>60) 
            {
             money = 0;
             count = 0;
/*
             log_file("etc/illegal_money",  LOG_CREAT | LOG_VF,
             "\033[1;33;46m%s \033[37;45m �ξ����H�o��峹 \033[37m %s\033[m\n",
             cuser.userid, ctime(&now));
             post_violatelaw(cuser.userid, "Ptt�t��ĵ��", 
                 "�ξ����H�o��峹", "�j������");
             abort_bbs(0);
*/
            }
          }
	else if(interval){
	    count = 0;
	    tin = interval;
	}
	if (curr_buf->raw_mode) {
	    switch (ch) {
	    case Ctrl('S'):
	    case Ctrl('Q'):
	    case Ctrl('T'):
		continue;
	    }
	}
	if (curr_buf->phone_mode) {
	    switch (ch) {
		case 'z':
		case 'Z':
		    if (curr_buf->phone_mode < 20)
			curr_buf->phone_mode = curr_buf->phone_mode0 = 20;
		    else
			curr_buf->phone_mode = curr_buf->phone_mode0 = 2;
		    curr_buf->line_dirty = 1;
		    curr_buf->redraw_everything = YEA;
		    continue;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		    if (curr_buf->phone_mode < 20) {
			curr_buf->phone_mode = curr_buf->phone_mode0 = ch - '0' + 1;
			curr_buf->line_dirty = 1;
			curr_buf->redraw_everything = YEA;
			continue;
		    }
		    break;
		case '-':
		    if (curr_buf->phone_mode < 20) {
			curr_buf->phone_mode = curr_buf->phone_mode0 = 11;
			curr_buf->line_dirty = 1;
			curr_buf->redraw_everything = YEA;
			continue;
		    }
		    break;
		case '=':
		    if (curr_buf->phone_mode < 20) {
			curr_buf->phone_mode = curr_buf->phone_mode0 = 12;
			curr_buf->line_dirty = 1;
			curr_buf->redraw_everything = YEA;
			continue;
		    }
		    break;
		case '`':
		    if (curr_buf->phone_mode < 20) {
			curr_buf->phone_mode = curr_buf->phone_mode0 = 13;
			curr_buf->line_dirty = 1;
			curr_buf->redraw_everything = YEA;
			continue;
		    }
		    break;
		case '/':
		    if (curr_buf->phone_mode >= 20) {
			curr_buf->phone_mode += 4;
			if (curr_buf->phone_mode > 27)
			    curr_buf->phone_mode -= 8;
			curr_buf->line_dirty = 1;
			curr_buf->redraw_everything = YEA;
			continue;
		    }
		    break;
		case '*':
		    if (curr_buf->phone_mode >= 20) {
			curr_buf->phone_mode++;
			if ((curr_buf->phone_mode - 21) % 4 == 3)
			    curr_buf->phone_mode -= 4;
			curr_buf->line_dirty = 1;
			curr_buf->redraw_everything = YEA;
			continue;
		    }
		    break;
	    }
	}

	if (ch < 0x100 && isprint2(ch)) {
            if(curr_buf->phone_mode && (pstr=phone_char(ch)))
	   	insert_dchar(pstr); 
	    else
		insert_char(ch);
	    lastindent = -1;
	    curr_buf->line_dirty = 1;
	} else {
	    if (ch == KEY_UP || ch == KEY_DOWN ){
		if (lastindent == -1)
		    lastindent = curr_buf->currpnt;
	    } else
		lastindent = -1;
	    if (ch == KEY_ESC)
		switch (KEY_ESC_arg) {
		case ',':
		    ch = Ctrl(']');
		    break;
		case '.':
		    ch = Ctrl('T');
		    break;
		case 'v':
		    ch = KEY_PGUP;
		    break;
		case 'a':
		case 'A':
		    ch = Ctrl('V');
		    break;
		case 'X':
		    ch = Ctrl('X');
		    break;
		case 'q':
		    ch = Ctrl('Q');
		    break;
		case 'o':
		    ch = Ctrl('O');
		    break;
		case '-':
		    ch = Ctrl('_');
		    break;
		case 's':
		    ch = Ctrl('S');
		    break;
		}

	    switch (ch) {
	    case Ctrl('X'):	/* Save and exit */
		ret = write_file(fpath, saveheader, islocal);
		if (ret != KEEP_EDITING) {
		    currutmp->mode = mode0;
		    currutmp->destuid = destuid0;

		    exit_edit_buffer();
		    if (!ret)
			return money;
		    else
			return ret;
		}
		curr_buf->line_dirty = 1;
		curr_buf->redraw_everything = YEA;
		break;
	    case Ctrl('W'):
		if (curr_buf->blockln >= 0)
		    block_del(2);
		curr_buf->line_dirty = 1;
		break;
	    case Ctrl('Q'):	/* Quit without saving */
		ch = ask("���������x�s (Y/N)? [N]: ");
		if (ch == 'y' || ch == 'Y') {
		    currutmp->mode = mode0;
		    currutmp->destuid = destuid0;
		    exit_edit_buffer();
		    return -1;
		}
		curr_buf->line_dirty = 1;
		curr_buf->redraw_everything = YEA;
		break;
	    case Ctrl('C'):
		ch = curr_buf->insert_character;
		curr_buf->insert_character = curr_buf->redraw_everything = YEA;
		if (!curr_buf->my_ansimode)
		    insert_string(reset_color);
		else {
		    char            ans[4];
		    move(b_lines - 2, 55);
		    outs("\033[1;33;40mB\033[41mR\033[42mG\033[43mY\033[44mL"
			 "\033[45mP\033[46mC\033[47mW\033[m");
		    if (getdata(b_lines - 1, 0,
			      "�п�J  �G��/�e��/�I��[���`�զr�©�][0wb]�G",
				ans, sizeof(ans), LCECHO)) {
			char            t[] = "BRGYLPCW";
			char            color[15];
			char           *tmp, *apos = ans;
			int             fg, bg;

			strcpy(color, "\033[");
			if (isdigit((int)*apos)) {
			    sprintf(color,"%s%c", color, *(apos++)); 
			    if (*apos)
				strcat(color, ";");
			}
			if (*apos) {
			    if ((tmp = strchr(t, toupper(*(apos++)))))
				fg = tmp - t + 30;
			    else
				fg = 37;
			    sprintf(color, "%s%d", color, fg);
			}
			if (*apos) {
			    if ((tmp = strchr(t, toupper(*(apos++)))))
				bg = tmp - t + 40;
			    else
				bg = 40;
			    sprintf(color, "%s;%d", color, bg);
			}
			strcat(color, "m");
			insert_string(color);
		    } else
			insert_string(reset_color);
		}
		curr_buf->insert_character = ch;
		curr_buf->line_dirty = 1;
		break;
	    case KEY_ESC:
		curr_buf->line_dirty = 0;
		switch (KEY_ESC_arg) {
		case 'U':
		    t_users();
		    curr_buf->redraw_everything = YEA;
		    curr_buf->line_dirty = 1;
		    break;
		case 'i':
		    t_idle();
		    curr_buf->redraw_everything = YEA;
		    curr_buf->line_dirty = 1;
		    break;
		case 'n':
		    search_str(1);
		    break;
		case 'p':
		    search_str(-1);
		    break;
		case 'L':
		case 'J':
		    goto_line(0);
		    break;
		case ']':
		    match_paren();
		    break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		    read_tmpbuf(KEY_ESC_arg - '0');
		    curr_buf->redraw_everything = YEA;
		    break;
		case 'l':	/* block delete */
		case ' ':
		    block_del(0);
		    curr_buf->line_dirty = 1;
		    break;
		case 'u':
		    if (curr_buf->blockln >= 0)
			block_del(1);
		    curr_buf->line_dirty = 1;
		    break;
		case 'c':
		    if (curr_buf->blockln >= 0)
			block_del(3);
		    curr_buf->line_dirty = 1;
		    break;
		case 'y':
		    undelete_line();
		    break;
		case 'R':
		    curr_buf->raw_mode ^= 1;
		    curr_buf->line_dirty = 1;
		    break;
		case 'I':
		    curr_buf->indent_mode ^= 1;
		    curr_buf->line_dirty = 1;
		    break;
		case 'j':
		    if (curr_buf->blockln >= 0)
			block_shift_left();
		    else if (curr_buf->currline->len) {
			int             currpnt0 = curr_buf->currpnt;
			curr_buf->currpnt = 0;
			delete_char();
			curr_buf->currpnt = (currpnt0 <= curr_buf->currline->len) ? currpnt0 :
			    currpnt0 - 1;
			if (curr_buf->my_ansimode)
			    curr_buf->currpnt = ansi2n(n2ansi(curr_buf->currpnt, curr_buf->currline),
					     curr_buf->currline);
		    }
		    curr_buf->line_dirty = 1;
		    break;
		case 'k':
		    if (curr_buf->blockln >= 0)
			block_shift_right();
		    else {
			int             currpnt0 = curr_buf->currpnt;

			curr_buf->currpnt = 0;
			insert_char(' ');
			curr_buf->currpnt = currpnt0;
		    }
		    curr_buf->line_dirty = 1;
		    break;
		case 'f':
		    while (curr_buf->currpnt < curr_buf->currline->len &&
			   isalnum((int)curr_buf->currline->data[++curr_buf->currpnt]));
		    while (curr_buf->currpnt < curr_buf->currline->len &&
			   isspace((int)curr_buf->currline->data[++curr_buf->currpnt]));
		    curr_buf->line_dirty = 1;
		    break;
		case 'b':
		    while (curr_buf->currpnt && isalnum((int)curr_buf->currline->data[--curr_buf->currpnt]));
		    while (curr_buf->currpnt && isspace((int)curr_buf->currline->data[--curr_buf->currpnt]));
		    curr_buf->line_dirty = 1;
		    break;
		case 'd':
		    while (curr_buf->currpnt < curr_buf->currline->len) {
			delete_char();
			if (!isalnum((int)curr_buf->currline->data[curr_buf->currpnt]))
			    break;
		    }
		    while (curr_buf->currpnt < curr_buf->currline->len) {
			delete_char();
			if (!isspace((int)curr_buf->currline->data[curr_buf->currpnt]))
			    break;
		    }
		    curr_buf->line_dirty = 1;
		    break;
		default:
		    curr_buf->line_dirty = 1;
		}
		break;
	    case Ctrl('_'):
		if (strcmp(editline, curr_buf->currline->data)) {
		    char            buf[WRAPMARGIN];

		    strlcpy(buf, curr_buf->currline->data, sizeof(buf));
		    strcpy(curr_buf->currline->data, editline);
		    strcpy(editline, buf);
		    curr_buf->currline->len = strlen(curr_buf->currline->data);
		    curr_buf->currpnt = 0;
		    curr_buf->line_dirty = 1;
		}
		break;
	    case Ctrl('S'):
		search_str(0);
		break;
	    case Ctrl('U'):
		insert_char('\033');
		curr_buf->line_dirty = 1;
		break;
	    case Ctrl('V'):	/* Toggle ANSI color */
		curr_buf->my_ansimode ^= 1;
		if (curr_buf->my_ansimode && curr_buf->blockln >= 0)
		    block_color();
		clear();
		curr_buf->redraw_everything = YEA;
		curr_buf->line_dirty = 1;
		break;
	    case Ctrl('I'):
		do {
		    insert_char(' ');
		} while (curr_buf->currpnt & 0x7);
		curr_buf->line_dirty = 1;
		break;
	    case '\r':
	    case '\n':
#ifdef MAX_EDIT_LINE
		if( curr_buf->totaln == MAX_EDIT_LINE ){
		    outs("MAX_EDIT_LINE exceed");
		    break;
		}
#endif
		split(curr_buf->currline, curr_buf->currpnt);
		oldcurrline = curr_buf->currline;
		curr_buf->line_dirty = 0;
		break;
	    case Ctrl('G'):
		{
		    unsigned int    currstat0 = currstat;
		    setutmpmode(EDITEXP);
		    a_menu("�s�軲�U��", "etc/editexp",
			   (HAS_PERM(PERM_SYSOP) ? SYSOP : NOBODY));
		    currstat = currstat0;
		}
		if (trans_buffer[0]) {
		    if ((fp1 = fopen(trans_buffer, "r"))) {
			int             indent_mode0 = curr_buf->indent_mode;

			curr_buf->indent_mode = 0;
			while (fgets(editline, WRAPMARGIN + 2, fp1)) {
			    if (!strncmp(editline, "�@��:", 5) ||
				!strncmp(editline, "���D:", 5) ||
				!strncmp(editline, "�ɶ�:", 5))
				continue;
			    insert_string(editline);
			}
			fclose(fp1);
			curr_buf->indent_mode = indent_mode0;
			while (curr_buf->curr_window_line >= b_lines) {
			    curr_buf->curr_window_line--;
			    curr_buf->top_of_win = curr_buf->top_of_win->next;
			}
		    }
		}
		curr_buf->redraw_everything = YEA;
		curr_buf->line_dirty = 1;
		break;
            case Ctrl('P'):
                if (curr_buf->phone_mode)
                   curr_buf->phone_mode = 0;
		else {
		    if (curr_buf->phone_mode0)
			curr_buf->phone_mode = curr_buf->phone_mode0;
		    else
			curr_buf->phone_mode = curr_buf->phone_mode0 = 2;
		}
                curr_buf->redraw_everything = YEA;
		curr_buf->line_dirty = 1;
		break;

	    case Ctrl('Z'):	/* Help */
		more("etc/ve.hlp", YEA);
		curr_buf->redraw_everything = YEA;
		curr_buf->line_dirty = 1;
		break;
	    case Ctrl('L'):
		clear();
		curr_buf->redraw_everything = YEA;
		curr_buf->line_dirty = 1;
		break;
	    case KEY_LEFT:
		if (curr_buf->currpnt) {
		    if (curr_buf->my_ansimode)
			curr_buf->currpnt = n2ansi(curr_buf->currpnt, curr_buf->currline);
		    curr_buf->currpnt--;
		    if (curr_buf->my_ansimode)
			curr_buf->currpnt = ansi2n(curr_buf->currpnt, curr_buf->currline);
		    curr_buf->line_dirty = 1;
		} else if (curr_buf->currline->prev) {
		    curr_buf->curr_window_line--;
		    curr_buf->currln--;
		    curr_buf->currline = curr_buf->currline->prev;
		    curr_buf->currpnt = curr_buf->currline->len;
		    curr_buf->line_dirty = 0;
		}
		break;
	    case KEY_RIGHT:
		if (curr_buf->currline->len != curr_buf->currpnt) {
		    if (curr_buf->my_ansimode)
			curr_buf->currpnt = n2ansi(curr_buf->currpnt, curr_buf->currline);
		    curr_buf->currpnt++;
		    if (curr_buf->my_ansimode)
			curr_buf->currpnt = ansi2n(curr_buf->currpnt, curr_buf->currline);
		    curr_buf->line_dirty = 1;
		} else if (curr_buf->currline->next) {
		    curr_buf->currpnt = 0;
		    curr_buf->curr_window_line++;
		    curr_buf->currln++;
		    curr_buf->currline = curr_buf->currline->next;
		    curr_buf->line_dirty = 0;
		}
		break;
	    case KEY_UP:
		if (curr_buf->currline->prev) {
		    if (curr_buf->my_ansimode)
			ch = n2ansi(curr_buf->currpnt, curr_buf->currline);
		    curr_buf->curr_window_line--;
		    curr_buf->currln--;
		    curr_buf->currline = curr_buf->currline->prev;
		    if (curr_buf->my_ansimode)
			curr_buf->currpnt = ansi2n(ch, curr_buf->currline);
		    else
			curr_buf->currpnt = (curr_buf->currline->len > lastindent) ? lastindent :
			    curr_buf->currline->len;
		    curr_buf->line_dirty = 0;
		}
		break;
	    case KEY_DOWN:
		if (curr_buf->currline->next) {
		    if (curr_buf->my_ansimode)
			ch = n2ansi(curr_buf->currpnt, curr_buf->currline);
		    curr_buf->currline = curr_buf->currline->next;
		    curr_buf->curr_window_line++;
		    curr_buf->currln++;
		    if (curr_buf->my_ansimode)
			curr_buf->currpnt = ansi2n(ch, curr_buf->currline);
		    else
			curr_buf->currpnt = (curr_buf->currline->len > lastindent) ? lastindent :
			    curr_buf->currline->len;
		    curr_buf->line_dirty = 0;
		}
		break;

	    case Ctrl('B'):
	    case KEY_PGUP: {
		short tmp = curr_buf->currln;
	   	curr_buf->top_of_win = back_line(curr_buf->top_of_win, 22);
	  	curr_buf->currln = tmp;
	 	curr_buf->currline = back_line(curr_buf->currline, 22);
		curr_buf->curr_window_line = getlineno();
		if (curr_buf->currpnt > curr_buf->currline->len)
		    curr_buf->currpnt = curr_buf->currline->len;
		curr_buf->redraw_everything = YEA;
		curr_buf->line_dirty = 0;
	 	break;
	    }

	    case Ctrl('F'):
	    case KEY_PGDN: {
		short tmp = curr_buf->currln;
		curr_buf->top_of_win = forward_line(curr_buf->top_of_win, 22);
		curr_buf->currln = tmp;
		curr_buf->currline = forward_line(curr_buf->currline, 22);
		curr_buf->curr_window_line = getlineno();
		if (curr_buf->currpnt > curr_buf->currline->len)
		    curr_buf->currpnt = curr_buf->currline->len;
		curr_buf->redraw_everything = YEA;
		curr_buf->line_dirty = 0;
		break;
	    }

	    case KEY_END:
	    case Ctrl('E'):
		curr_buf->currpnt = curr_buf->currline->len;
		curr_buf->line_dirty = 1;
		break;
	    case Ctrl(']'):	/* start of file */
		curr_buf->currline = curr_buf->top_of_win = curr_buf->firstline;
		curr_buf->currpnt = curr_buf->currln = curr_buf->curr_window_line = 0;
		curr_buf->redraw_everything = YEA;
		curr_buf->line_dirty = 0;
		break;
	    case Ctrl('T'):	/* tail of file */
		curr_buf->top_of_win = back_line(curr_buf->lastline, 23);
		curr_buf->currline = curr_buf->lastline;
		curr_buf->curr_window_line = getlineno();
		curr_buf->currln = curr_buf->totaln;
		curr_buf->redraw_everything = YEA;
		curr_buf->currpnt = 0;
		curr_buf->line_dirty = 0;
		break;
	    case KEY_HOME:
	    case Ctrl('A'):
		curr_buf->currpnt = 0;
		curr_buf->line_dirty = 1;
		break;
	    case KEY_INS:	/* Toggle insert/overwrite */
	    case Ctrl('O'):
		if (curr_buf->blockln >= 0 && curr_buf->insert_character) {
		    char            ans[4];

		    getdata(b_lines - 1, 0,
			    "�϶��L�եk�����J�r��(�w�]���ťզr��)",
			    ans, sizeof(ans), LCECHO);
		    curr_buf->insert_c = ans[0] ? ans[0] : ' ';
		}
		curr_buf->insert_character ^= 1;
		curr_buf->line_dirty = 1;
		break;
	    case Ctrl('H'):
	    case '\177':	/* backspace */
		curr_buf->line_dirty = 1;
		if (curr_buf->my_ansimode) {
		    curr_buf->my_ansimode = 0;
		    clear();
		    curr_buf->redraw_everything = YEA;
		} else {
		    if (curr_buf->currpnt == 0) {
			textline_t     *p;

			if (!curr_buf->currline->prev)
			    break;
			curr_buf->line_dirty = 0;
			curr_buf->curr_window_line--;
			curr_buf->currln--;
			curr_buf->currline = curr_buf->currline->prev;
			curr_buf->currline = adjustline(curr_buf->currline, WRAPMARGIN);

			// FIXME dirty fix. would this cause memory leak?
			oldcurrline = curr_buf->currline;

			curr_buf->currpnt = curr_buf->currline->len;
			curr_buf->redraw_everything = YEA;
			if (*killsp(curr_buf->currline->next->data) == '\0') {
			    delete_line(curr_buf->currline->next);
			    break;
			}
			p = curr_buf->currline;
			while (!join(p)) {
			    p = p->next;
			    if (p == NULL) {
				indigestion(2);
				abort_bbs(0);
			    }
			}
			break;
		    }
		    curr_buf->currpnt--;
		    delete_char();
		}
		break;
	    case Ctrl('D'):
	    case KEY_DEL:	/* delete current character */
		curr_buf->line_dirty = 1;
		if (curr_buf->currline->len == curr_buf->currpnt) {
		    textline_t     *p = curr_buf->currline;

		    while (!join(p)) {
			p = p->next;
			if (p == NULL) {
			    indigestion(2);
			    abort_bbs(0);
			}
		    }
		    curr_buf->line_dirty = 0;
		    curr_buf->redraw_everything = YEA;
		} else {
		    delete_char();
		    if (curr_buf->my_ansimode)
			curr_buf->currpnt = ansi2n(n2ansi(curr_buf->currpnt, curr_buf->currline), curr_buf->currline);
		}
		break;
	    case Ctrl('Y'):	/* delete current line */
		curr_buf->currline->len = curr_buf->currpnt = 0;
	    case Ctrl('K'):	/* delete to end of line */
		if (curr_buf->currline->len == 0) {
		    textline_t     *p = curr_buf->currline->next;
		    if (!p) {
			p = curr_buf->currline->prev;
			if (!p)
			    break;
			if (curr_buf->curr_window_line > 0) {
			    curr_buf->curr_window_line--;
			    curr_buf->currln--;
			}
		    }
		    if (curr_buf->currline == curr_buf->top_of_win)
			curr_buf->top_of_win = p;
		    delete_line(curr_buf->currline);
		    curr_buf->currline = p;
		    curr_buf->redraw_everything = YEA;
		    curr_buf->line_dirty = 0;
		    oldcurrline = curr_buf->currline = adjustline(curr_buf->currline, WRAPMARGIN);
		    break;
		}
		if (curr_buf->currline->len == curr_buf->currpnt) {
		    textline_t     *p = curr_buf->currline;

		    while (!join(p)) {
			p = p->next;
			if (p == NULL) {
			    indigestion(2);
			    abort_bbs(0);
			}
		    }
		    curr_buf->redraw_everything = YEA;
		    curr_buf->line_dirty = 0;
		    break;
		}
		curr_buf->currline->len = curr_buf->currpnt;
		curr_buf->currline->data[curr_buf->currpnt] = '\0';
		curr_buf->line_dirty = 1;
		break;
	    }
	    if (curr_buf->currln < 0)
		curr_buf->currln = 0;
	    if (curr_buf->curr_window_line < 0) {
		curr_buf->curr_window_line = 0;
		if (!curr_buf->top_of_win->prev)
		    indigestion(6);
		else {
		    curr_buf->top_of_win = curr_buf->top_of_win->prev;
		    rscroll();
		}
	    }
	    if (curr_buf->curr_window_line == b_lines ||
                (curr_buf->phone_mode && curr_buf->curr_window_line == b_lines - 1)) {
                if(curr_buf->phone_mode)
                   curr_buf->curr_window_line = t_lines - 3;
                else
                   curr_buf->curr_window_line = t_lines - 2;

		if (!curr_buf->top_of_win->next)
		    indigestion(7);
		else {
		    curr_buf->top_of_win = curr_buf->top_of_win->next;
                    if(curr_buf->phone_mode)
                      move(b_lines-1, 0);
                    else
		      move(b_lines, 0);
		    clrtoeol();
		    scroll();
		}
	    }
	}
	if (curr_buf->currpnt < t_columns - 1)
	    curr_buf->edit_margin = 0;
	else
	    curr_buf->edit_margin = curr_buf->currpnt / (t_columns - 8) * (t_columns - 8);

	if (!curr_buf->redraw_everything) {
	    if (curr_buf->edit_margin != last_margin) {
		last_margin = curr_buf->edit_margin;
		curr_buf->redraw_everything = YEA;
	    } else {
		move(curr_buf->curr_window_line, 0);
		clrtoeol();
		if (curr_buf->my_ansimode)
		    outs(curr_buf->currline->data);
		else
		    edit_outs(&curr_buf->currline->data[curr_buf->edit_margin]);
		edit_msg();
	    }
	}
    }

    exit_edit_buffer();
}
