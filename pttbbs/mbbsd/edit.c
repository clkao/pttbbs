/* $Id$ */
/* edit.c, �ΨӴ��� bbs�W����r�s�边, �Y ve.
 * �{�b�o�@�ӬO�c�d�L������, �����í�w, �Τ���h�� cpu, ���O�i�H�٤U�\�h
 * ���O���� (�H Ptt����, �b�E�d�H�W�����ɭ�, ���i�٤U 50MB ���O����)
 * �p�G�z�{���u�� cpu���O����v�ä��X�G�z�����D, �z�i�H�Ҽ{��ϥέץ��e��
 * ���� (Revision 1.27)
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

static textline_t *firstline = NULL;
static textline_t *lastline = NULL;
static textline_t *currline = NULL;
static textline_t *blockline = NULL;
static textline_t *top_of_win = NULL;
static textline_t *deleted_lines = NULL;

static char     line[WRAPMARGIN + 2];
static int      ifuseanony = 0;
static int      currpnt, currln, totaln;
static int      curr_window_line;
static int      redraw_everything;
static int      insert_character;
static int      my_ansimode;
static int      raw_mode;
static int      edit_margin;
static int      blockln = -1;
static int      blockpnt;
static int      line_dirty;
static int      indent_mode;
static int      insert_c = ' ';

static char     fp_bak[] = "bak";

/* �O����޲z�P�s��B�z */
static void
indigestion(int i)
{
    fprintf(stderr, "�Y������ %d\n", i);
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
	    while ((ch = *tmp) && !isalpha(ch))
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

static int
n2ansi(int nx, textline_t * line)
{
    register int    ansix = 0;
    register char  *tmp, *nxp;
    register char   ch;

    tmp = nxp = line->data;
    nxp += nx;

    while (*tmp) {
	if (*tmp == KEY_ESC) {
	    while ((ch = *tmp) && !isalpha(ch))
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
    char    *edit_mode[2] = {"���N", "���J"};
    register int    n = currpnt;

    if (my_ansimode)		/* Thor: �@ ansi �s�� */
	n = n2ansi(n, currline);
    n++;
    move(b_lines, 0);
    clrtoeol();
    prints("\033[%sm �s��峹 \033[31;47m (Ctrl-Z)\033[30m���U���� "
	   "\033[31;47m(^G)\033[30m���J�Ϥ�w \033[31m(^X,^Q)"
	   "\033[30m���}��%s�x%c%c%c%c�� %3d:%3d  \033[m",
	   "37;44",
	   edit_mode[insert_character],
	   my_ansimode ? 'A' : 'a', indent_mode ? 'I' : 'i',
	   'P', raw_mode ? 'R' : 'r',
	   currln + 1, n);
}

static textline_t *
back_line(textline_t * pos, int num)
{
    while (num-- > 0) {
	register textline_t *item;

	if (pos && (item = pos->prev)) {
	    pos = item;
	    currln--;
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
	    currln++;
	}
    }
    return pos;
}

static int
getlineno()
{
    int             cnt = 0;
    textline_t     *p = currline;

    while (p && (p != top_of_win)) {
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
	lastline = p;
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
	lastline = p;
    if (p)
	p->next = n;
    else
	firstline = n;
    strcat(line->data, "\n");
    line->prev = deleted_lines;
    deleted_lines = line;
    totaln--;
}

static int
ask(char *prompt)
{
    int             ch;

    move(0, 0);
    clrtoeol();
    standout();
    prints("%s", prompt);
    standend();
    ch = igetkey();
    move(0, 0);
    clrtoeol();
    return (ch);
}

static int
indent_spcs()
{
    textline_t     *p;
    int             spcs;

    if (!indent_mode)
	return 0;

    for (p = currline; p; p = p->prev) {
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
    textline_t tmpl[sizeof(textline_t) + WRAPMARGIN];
    textline_t *newp;
    memcpy(tmpl, oldp, oldp->len + sizeof(textline_t));
    free(oldp);

    newp = alloc_line(len);
    memcpy(newp, tmpl, len + sizeof(textline_t));
#ifdef DEBUG
    newp->mlength = len;
#endif
    if( oldp == firstline ) firstline = newp;
    if( oldp == lastline )  lastline  = newp;
    if( oldp == currline )  currline  = newp;
    if( oldp == blockline ) blockline = newp;
    if( oldp == top_of_win) top_of_win= newp;
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

	totaln++;

	p->len = line->len - pos + spcs;
	line->len = pos;

	memset(p->data, ' ', spcs);
	p->data[spcs] = 0;
	strcat(p->data, (ptr = line->data + pos));
	ptr[0] = '\0';
	append(p, line);
	line = adjustline(line, line->len);
	if (line == currline && pos <= currpnt) {
	    currline = p;
	    if (pos == currpnt)
		currpnt = spcs;
	    else
		currpnt -= pos;
	    curr_window_line++;
	    currln++;
	}
	redraw_everything = YEA;
    }
    return line;
}

static void
insert_char(int ch)
{
    register textline_t *p = currline;
    register int    i = p->len;
    register char  *s;
    int             wordwrap = YEA;

    if (currpnt > i) {
	indigestion(1);
	return;
    }
    if (currpnt < i && !insert_character) {
	p->data[currpnt++] = ch;
	/* Thor: ansi �s��, �i�Hoverwrite, ���\�� ansi code */
	if (my_ansimode)
	    currpnt = ansi2n(n2ansi(currpnt, p), p);
    } else {
	while (i >= currpnt) {
	    p->data[i + 1] = p->data[i];
	    i--;
	}
	p->data[currpnt++] = ch;
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
insert_string(char *str)
{
    int             ch;

    while ((ch = *str++)) {
	if (isprint2(ch) || ch == '\033')
	    insert_char(ch);
	else if (ch == '\t') {
	    do {
		insert_char(' ');
	    } while (currpnt & 0x7);
	} else if (ch == '\n')
	    split(currline, currpnt);
    }
}

static int
undelete_line()
{
    textline_t     *p = deleted_lines;
    textline_t     *currline0 = currline;
    textline_t     *top_of_win0 = top_of_win;
    int             currpnt0 = currpnt;
    int             currln0 = currln;
    int             curr_window_line0 = curr_window_line;
    int             indent_mode0 = indent_mode;

    if (!deleted_lines)
	return 0;

    indent_mode = 0;
    insert_string(deleted_lines->data);
    indent_mode = indent_mode0;
    deleted_lines = deleted_lines->prev;
    free(p);

    currline = currline0;
    top_of_win = top_of_win0;
    currpnt = currpnt0;
    currln = currln0;
    curr_window_line = curr_window_line0;
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

    if ((len = currline->len)) {
	register int    i;
	register char  *s;

	if (currpnt >= len) {
	    indigestion(1);
	    return;
	}
	for (i = currpnt, s = currline->data + i; i != len; i++, s++)
	    s[0] = s[1];
	currline->len--;
    }
}

static void
load_file(FILE * fp) /* NOTE it will fclose(fp) */
{
    int             indent_mode0 = indent_mode;

    assert(fp);
    indent_mode = 0;
    while (fgets(line, WRAPMARGIN + 2, fp))
	insert_string(line);
    fclose(fp);
    indent_mode = indent_mode0;
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
	while (curr_window_line >= b_lines) {
	    curr_window_line--;
	    top_of_win = top_of_win->next;
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
	for (p = firstline; p; p = p->next) {
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

/* �s�边�۰ʳƥ� */
void
auto_backup()
{
    if (currline) {
	FILE           *fp;
	textline_t     *p, *v;
	char            bakfile[64];
	int             count = 0;

	setuserfile(bakfile, fp_bak);
	if ((fp = fopen(bakfile, "w"))) {
	    for (p = firstline; p != NULL && count < 512; p = v, count++) {
		v = p->next;
		fprintf(fp, "%s\n", p->data);
		free(p);
	    }
	    fclose(fp);
	}
	currline = NULL;
    }
}

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
	    int             indent_mode0 = indent_mode;

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

	    indent_mode = 0;
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
	    indent_mode = indent_mode0;
	    fclose(inf);
	}
    }
}

/* �f�d user �ި����ϥ� */
static int
check_quote()
{
    register textline_t *p = firstline;
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
write_header(FILE * fp)
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
		    snprintf(postlog.author, sizeof(postlog.author),
			     "%s", cuser.userid);
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
    if (!ifuseanony) {
	num = showsignature(fpath, &i);
	if (num){
	    msg[34] = ch = isdigit(cuser.signature) ? cuser.signature : 'X';
	    getdata(0, 0, msg, buf, 4, DOECHO);

	    if (!buf[0])
		buf[0] = ch;

	    if (isdigit(buf[0]))
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
	safe_sleep(1);
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
	    prints("�z�|���q�L�����T�{�A�u�� Local Save�C\n");
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

	if (!*fpath) {
	    sethomepath(fpath, cuser.userid);
	    strcpy(fpath, tempnam(fpath, "ve_"));
	}
	if ((fp = fopen(fpath, "w")) == NULL) {
	    indigestion(5);
	    abort_bbs(0);
	}
	if (saveheader)
	    write_header(fp);
    }
    for (p = firstline; p; p = v) {
	v = p->next;
	if (!aborted) {
	    assert(fp);
	    msg = p->data;
	    if (v || msg[0]) {
		trim(msg);

		line++;
		if (currstat == POSTING && po) {
		    saveheader = str_checksum(msg);
		    if (saveheader) {
			if (postrecord.checksum[po] == saveheader) {
			    po++;
			    if (po > 3) {
				postrecord.times++;
				po = 0;
			    }
			} else
			    po = 1;
			if (currstat == POSTING && line >= totaln / 2 &&
			    sum < 3) {
			    checksum[sum++] = saveheader;
			}
		    }
		}
#ifdef SUPPORT_GB
		if (current_font_type == TYPE_GB) {
		    fprintf(fp, "%s\n", hc_convert_str(msg, HC_GBtoBIG, HC_DO_SINGLE));
		} else
#endif
		    fprintf(fp, "%s\n", msg);
	    }
	}
	free(p);
    }
    currline = NULL;

    if (postrecord.times > MAX_CROSSNUM - 1)
	anticrosspost();

    if (po && sum == 3) {
	memcpy(&postrecord.checksum[1], checksum, sizeof(int) * 3);
	postrecord.times = 0;
    }
    if (!aborted) {
	if (islocal)
	    *islocal = (local_article == 1);
	if (currstat == POSTING || currstat == SMAIL)
	    addsignature(fp, ifuseanony);
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

    if (currpnt > blockpnt) {
	min = blockpnt;
	max = currpnt;
    } else {
	min = currpnt;
	max = blockpnt;
    }

    for (p = top_of_win, i = 0; i < b_lines; i++) {
	move(i, 0);
	clrtoeol();
	if (blockln >= 0 &&
	((blockln <= currln && blockln <= (currln - curr_window_line + i) &&
	  (currln - curr_window_line + i) <= currln) ||
	 (currln <= (currln - curr_window_line + i) &&
	  (currln - curr_window_line + i) <= blockln))) {
	    outs("\033[7m");
	    inblock = 1;
	} else
	    inblock = 0;
	if (p) {
	    if (my_ansimode)
		if (currln == blockln && p == currline && max > min) {
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
	    else if (currln == blockln && p == currline && max > min) {
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
		edit_outs((edit_margin < p->len) ? &p->data[edit_margin] : "");
	    p = p->next;
	    if (inblock)
		outs("\033[m");
	} else
	    outch('~');
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

	p = firstline;
	currln = lino - 1;

	while (--lino && p->next)
	    p = p->next;

	if (p)
	    currline = p;
	else {
	    currln = totaln;
	    currline = lastline;
	}
	currpnt = 0;
	if (currln < 11) {
	    top_of_win = firstline;
	    curr_window_line = currln;
	} else {
	    int             i;

	    curr_window_line = 11;
	    for (i = curr_window_line; i; i--)
		p = p->prev;
	    top_of_win = p;
	}
    }
    redraw_everything = YEA;
}

char           *
strcasestr(const char *big, const char *little)
{
    char           *ans = (char *)big;
    int             len = strlen(little);
    char           *endptr = (char *)big + strlen(big) - len;

    while (ans <= endptr)
	if (!strncasecmp(ans, little, len))
	    return ans;
	else
	    ans++;
    return 0;
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
	    for (lino = currln, p = currline; p; p = p->next, lino++)
		if ((pos = fptr(p->data + (lino == currln ? currpnt + 1 : 0),
				str)) && (lino != currln ||
					  pos - p->data != currpnt))
		    break;
	} else {
	    for (lino = currln, p = currline; p; p = p->prev, lino--)
		if ((pos = fptr(p->data, str)) &&
		    (lino != currln || pos - p->data != currpnt))
		    break;
	}
	if (pos) {
	    currline = p;
	    currln = lino;
	    currpnt = pos - p->data;
	    if (lino < 11) {
		top_of_win = firstline;
		curr_window_line = currln;
	    } else {
		int             i;

		curr_window_line = 11;
		for (i = curr_window_line; i; i--)
		    p = p->prev;
		top_of_win = p;
	    }
	    redraw_everything = YEA;
	}
    }
    if (!mode)
	redraw_everything = YEA;
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

    if (!(ptype = strchr(parens, currline->data[currpnt])))
	return;

    type = (ptype - parens) / 2;
    parenum += ((ptype - parens) % 2) ? -1 : 1;

    /* FIXME �Ӧh strlen() */
    /* FIXME CRASH */
    if (parenum > 0) {
	for (lino = currln, p = currline; p; p = p->next, lino++) {
	    for (i = (lino == currln) ? currpnt + 1 : 0;
		 (size_t)i < strlen(p->data); i++)
		if (p->data[i] == '/' && p->data[++i] == '*') {
		    ++i;
		    while (1) {
			while ((size_t)i < strlen(p->data) - 1 &&
			       !(p->data[i] == '*' && p->data[i + 1] == '/'))
			    i++;
			if ((size_t)i >= strlen(p->data) - 1 && p->next) {
			    p = p->next;
			    ++lino;
			    i = 0;
			} else
			    break;
		    }
		} else if ((c = p->data[i]) == '\'' || c == '"') {
		    while (1) {
			while (i < (int)(strlen(p->data) - 1))
			    if (p->data[++i] == '\\' && (size_t)i < strlen(p->data) - 2)
				++i;
			    else if (p->data[i] == c)
				goto end_quote;
			if ((size_t)i >= strlen(p->data) - 1 && p->next) {
			    p = p->next;
			    ++lino;
			    i = -1;
			} else
			    break;
		    }
	    end_quote:
		    ;
		} else if ((ptype = strchr(parens, p->data[i])) &&
			   (ptype - parens) / 2 == type)
		    if (!(parenum += ((ptype - parens) % 2) ? -1 : 1))
			goto p_outscan;
	}
    } else {
	for (lino = currln, p = currline; p; p = p->prev, lino--)
	    for (i = ((lino == currln) ?
		      currpnt - 1 :
		      (int)strlen(p->data) - 1);
		 i >= 0; i--)
		if (p->data[i] == '/' && p->data[--i] == '*' && i > 0) {
		    --i;
		    while (1) {
			while (i > 0 &&
			       !(p->data[i] == '*' && p->data[i - 1] == '/'))
			    i--;
			if (i <= 0 && p->prev) {
			    p = p->prev;
			    --lino;
			    i = strlen(p->data) - 1;
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
			    --lino;
			    i = strlen(p->data);
			} else
			    break;
		    }
	    begin_quote:
		    ;
		} else if ((ptype = strchr(parens, p->data[i])) &&
			   (ptype - parens) / 2 == type)
		    if (!(parenum += ((ptype - parens) % 2) ? -1 : 1))
			goto p_outscan;
    }
p_outscan:
    if (!parenum) {
	int             top = currln - curr_window_line;
	int             bottom = currln - curr_window_line + b_lines - 1;

	currpnt = i;
	currline = p;
	curr_window_line += lino - currln;
	currln = lino;

	if (lino < top || lino > bottom) {
	    if (lino < 11) {
		top_of_win = firstline;
		curr_window_line = currln;
	    } else {
		int             i;

		curr_window_line = 11;
		for (i = curr_window_line; i; i--)
		    p = p->prev;
		top_of_win = p;
	    }
	    redraw_everything = YEA;
	}
    }
}

static void
block_del(int hide)
{
    if (blockln < 0) {
	blockln = currln;
	blockpnt = currpnt;
	blockline = currline;
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
	    if (currln >= blockln) {
		begin = blockline;
		end = currline;
		if (ans[2] == 'y' && !(begin == end && currpnt != blockpnt)) {
		    curr_window_line -= (currln - blockln);
		    if (curr_window_line < 0) {
			curr_window_line = 0;
			if (end->next)
			    (top_of_win = end->next)->prev = begin->prev;
			else
			    top_of_win = (lastline = begin->prev);
		    }
		    currln -= (currln - blockln);
		}
	    } else {
		begin = currline;
		end = blockline;
	    }
	    if (ans[2] == 'y' && !(begin == end && currpnt != blockpnt)) {
		if (begin->prev)
		    begin->prev->next = end->next;
		else if (end->next)
		    top_of_win = firstline = end->next;
		else {
		    currline = top_of_win = firstline =
			lastline = alloc_line(WRAPMARGIN);
		    currln = curr_window_line = edit_margin = 0;
		}

		if (end->next)
		    (currline = end->next)->prev = begin->prev;
		else if (begin->prev) {
		    currline = (lastline = begin->prev);
		    currln--;
		    if (curr_window_line > 0)
			curr_window_line--;
		}
	    }
	    setuserfile(fp_tmpbuf, tmpfname);
	    if ((fp = fopen(fp_tmpbuf, ans))) {
		if (begin == end && currpnt != blockpnt) {
		    char            buf[WRAPMARGIN + 2];

		    if (currpnt > blockpnt) {
			strlcpy(buf, begin->data + blockpnt, sizeof(buf));
			buf[currpnt - blockpnt] = 0;
		    } else {
			strlcpy(buf, begin->data + currpnt, sizeof(buf));
			buf[blockpnt - currpnt] = 0;
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
		if (begin == end && currpnt != blockpnt) {
		    int             min, max;

		    if (currpnt > blockpnt) {
			min = blockpnt;
			max = currpnt;
		    } else {
			min = currpnt;
			max = blockpnt;
		    }
		    strlcpy(begin->data + min, begin->data + max, sizeof(begin->data) - min);
		    begin->len -= max - min;
		    currpnt = min;
		} else {
		    for (p = begin; p != end; totaln--)
			free((p = p->next)->prev);
		    free(end);
		    totaln--;
		    currpnt = 0;
		}
	    }
	}
	blockln = -1;
	redraw_everything = YEA;
    }
}

static void
block_shift_left()
{
    textline_t     *begin, *end, *p;

    if (currln >= blockln) {
	begin = blockline;
	end = currline;
    } else {
	begin = currline;
	end = blockline;
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
    if (currpnt > currline->len)
	currpnt = currline->len;
    redraw_everything = YEA;
}

static void
block_shift_right()
{
    textline_t     *begin, *end, *p;

    if (currln >= blockln) {
	begin = blockline;
	end = currline;
    } else {
	begin = currline;
	end = blockline;
    }
    p = begin;
    while (1) {
	if (p->len < WRAPMARGIN) {
	    int             i = p->len + 1;

	    while (i--)
		p->data[i + 1] = p->data[i];
	    p->data[0] = insert_character ? ' ' : insert_c;
	    ++p->len;
	}
	if (p == end)
	    break;
	else
	    p = p->next;
    }
    if (currpnt > currline->len)
	currpnt = currline->len;
    redraw_everything = YEA;
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

    if (currln >= blockln) {
	begin = blockline;
	end = currline;
    } else {
	begin = currline;
	end = blockline;
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

/* �s��B�z�G�D�{���B��L�B�z */
int
vedit(char *fpath, int saveheader, int *islocal)
{
    FILE           *fp1;
    char            last = 0;	/* the last key you press */
    int             ch, foo;
    int             lastindent = -1;
    int             last_margin;
    int             mode0 = currutmp->mode;
    int             destuid0 = currutmp->destuid;
    unsigned int    money = 0;
    int             interval = 0;
    time_t          th = now;

    textline_t     *firstline0 = firstline;
    textline_t     *lastline0 = lastline;
    textline_t     *currline0 = currline;
    textline_t     *blockline0 = blockline;
    textline_t     *top_of_win0 = top_of_win;
    textline_t     *oldcurrline;
    int             local_article0 = local_article;
    int             currpnt0 = currpnt;
    int             currln0 = currln;
    int             totaln0 = totaln;
    int             curr_window_line0 = curr_window_line;
    int             insert_character0 = insert_character;
    int             my_ansimode0 = my_ansimode;
    int             edit_margin0 = edit_margin;
    int             blockln0 = blockln, count = 0, tin = 0;

    currutmp->mode = EDITING;
    currutmp->destuid = currstat;
    insert_character = redraw_everything = 1;
    blockln = -1;

    line_dirty = currpnt = totaln = my_ansimode = 0;
    oldcurrline = currline = top_of_win =
	firstline = lastline = alloc_line(WRAPMARGIN);

    if (*fpath)
	read_file(fpath);

    if (*quote_file) {
	do_quote();
	*quote_file = '\0';
	if (quote_file[79] == 'L')
	    local_article = 1;
    }
    currline = firstline;
    currpnt = currln = curr_window_line = edit_margin = last_margin = 0;

    while (1) {
	if (redraw_everything || blockln >= 0) {
	    display_buffer();
	    redraw_everything = NA;
	}
	if( oldcurrline != currline ){
	    oldcurrline = adjustline(oldcurrline, oldcurrline->len);
	    oldcurrline = currline = adjustline(currline, WRAPMARGIN);
	}
	if (my_ansimode)
	    ch = n2ansi(currpnt, currline);
	else
	    ch = currpnt - edit_margin;
	move(curr_window_line, ch);
	if (!line_dirty && strcmp(line, currline->data))
	    strcpy(line, currline->data);
	ch = igetkey();
	/* jochang debug */
	if ((interval = (now - th))) {
	    th = now;
	    if ((char)ch != last) {
		money++;
		last = (char)ch;
	    }
	}
	if (interval && interval == tin)
	    count++;
	else {
	    count = 0;
	    tin = interval;
	}
	/* �s��240��interval�@�� , �����O�b�İ] 
	if (count >= 240) {
	    char buf[200];
	    snprintf(buf, sizeof(buf), "\033[1;33;46m%s\033[37m�b\033[37;45m%s\n"
		    "\033[37m�O�H�k�ȿ� , %s\033[m", cuser.userid,
		    currboard, ctime(&now));
	    log_file("etc/illegal_money", buf, 1);
	    money = 0;
	    post_violatelaw(cuser.userid, "Ptt �t��ĵ��", "�H�k�ȿ�", "�������k�ұo");
	    mail_violatelaw(cuser.userid, "Ptt �t��ĵ��", "�H�k�ȿ�", "�������k�ұo");
	    demoney(10000);
	    abort_bbs(0);
	} */
	if (raw_mode)
	    switch (ch) {
	    case Ctrl('S'):
	    case Ctrl('Q'):
	    case Ctrl('T'):
		continue;
		break;
	    }
	if (ch < 0x100 && isprint2(ch)) {
	    insert_char(ch);
	    lastindent = -1;
	    line_dirty = 1;
	} else {
	    if (ch == Ctrl('P') || ch == KEY_UP || ch == KEY_DOWN ||
		ch == Ctrl('N')) {
		if (lastindent == -1)
		    lastindent = currpnt;
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
		foo = write_file(fpath, saveheader, islocal);
		if (foo != KEEP_EDITING) {
		    currutmp->mode = mode0;
		    currutmp->destuid = destuid0;
		    firstline = firstline0;
		    lastline = lastline0;
		    currline = currline0;
		    blockline = blockline0;
		    top_of_win = top_of_win0;
		    local_article = local_article0;
		    currpnt = currpnt0;
		    currln = currln0;
		    totaln = totaln0;
		    curr_window_line = curr_window_line0;
		    insert_character = insert_character0;
		    my_ansimode = my_ansimode0;
		    edit_margin = edit_margin0;
		    blockln = blockln0;
		    if (!foo)
			return money;
		    else
			return foo;
		}
		line_dirty = 1;
		redraw_everything = YEA;
		break;
	    case Ctrl('W'):
		if (blockln >= 0)
		    block_del(2);
		line_dirty = 1;
		break;
	    case Ctrl('Q'):	/* Quit without saving */
		ch = ask("���������x�s (Y/N)? [N]: ");
		if (ch == 'y' || ch == 'Y') {
		    currutmp->mode = mode0;
		    currutmp->destuid = destuid0;
		    firstline = firstline0;
		    lastline = lastline0;
		    currline = currline0;
		    blockline = blockline0;
		    top_of_win = top_of_win0;
		    local_article = local_article0;
		    currpnt = currpnt0;
		    currln = currln0;
		    totaln = totaln0;
		    curr_window_line = curr_window_line0;
		    insert_character = insert_character0;
		    my_ansimode = my_ansimode0;
		    edit_margin = edit_margin0;
		    blockln = blockln0;
		    return -1;
		}
		line_dirty = 1;
		redraw_everything = YEA;
		break;
	    case Ctrl('C'):
		ch = insert_character;
		insert_character = redraw_everything = YEA;
		if (!my_ansimode)
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

			strlcpy(color, "\033[", sizeof(color));
			if (isdigit(*apos)) {
			    snprintf(color, sizeof(color),
				     "%s%c", color, *(apos++));
			    if (*apos)
				snprintf(color, sizeof(color), "%s;", color);
			}
			if (*apos) {
			    if ((tmp = strchr(t, toupper(*(apos++)))))
				fg = tmp - t + 30;
			    else
				fg = 37;
			    snprintf(color, sizeof(color), "%s%d", color, fg);
			}
			if (*apos) {
			    if ((tmp = strchr(t, toupper(*(apos++)))))
				bg = tmp - t + 40;
			    else
				bg = 40;
			    snprintf(color, sizeof(color), "%s;%d", color, bg);
			}
			snprintf(color, sizeof(color), "%sm", color);
			insert_string(color);
		    } else
			insert_string(reset_color);
		}
		insert_character = ch;
		line_dirty = 1;
		break;
	    case KEY_ESC:
		line_dirty = 0;
		switch (KEY_ESC_arg) {
		case 'U':
		    t_users();
		    redraw_everything = YEA;
		    line_dirty = 1;
		    break;
		case 'i':
		    t_idle();
		    redraw_everything = YEA;
		    line_dirty = 1;
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
		    redraw_everything = YEA;
		    break;
		case 'l':	/* block delete */
		case ' ':
		    block_del(0);
		    line_dirty = 1;
		    break;
		case 'u':
		    if (blockln >= 0)
			block_del(1);
		    line_dirty = 1;
		    break;
		case 'c':
		    if (blockln >= 0)
			block_del(3);
		    line_dirty = 1;
		    break;
		case 'y':
		    undelete_line();
		    break;
		case 'R':
		    raw_mode ^= 1;
		    line_dirty = 1;
		    break;
		case 'I':
		    indent_mode ^= 1;
		    line_dirty = 1;
		    break;
		case 'j':
		    if (blockln >= 0)
			block_shift_left();
		    else if (currline->len) {
			int             currpnt0 = currpnt;
			currpnt = 0;
			delete_char();
			currpnt = (currpnt0 <= currline->len) ? currpnt0 :
			    currpnt0 - 1;
			if (my_ansimode)
			    currpnt = ansi2n(n2ansi(currpnt, currline),
					     currline);
		    }
		    line_dirty = 1;
		    break;
		case 'k':
		    if (blockln >= 0)
			block_shift_right();
		    else {
			int             currpnt0 = currpnt;

			currpnt = 0;
			insert_char(' ');
			currpnt = currpnt0;
		    }
		    line_dirty = 1;
		    break;
		case 'f':
		    while (currpnt < currline->len &&
			   isalnum(currline->data[++currpnt]));
		    while (currpnt < currline->len &&
			   isspace(currline->data[++currpnt]));
		    line_dirty = 1;
		    break;
		case 'b':
		    while (currpnt && isalnum(currline->data[--currpnt]));
		    while (currpnt && isspace(currline->data[--currpnt]));
		    line_dirty = 1;
		    break;
		case 'd':
		    while (currpnt < currline->len) {
			delete_char();
			if (!isalnum(currline->data[currpnt]))
			    break;
		    }
		    while (currpnt < currline->len) {
			delete_char();
			if (!isspace(currline->data[currpnt]))
			    break;
		    }
		    line_dirty = 1;
		    break;
		default:
		    line_dirty = 1;
		}
		break;
	    case Ctrl('_'):
		if (strcmp(line, currline->data)) {
		    char            buf[WRAPMARGIN];

		    strlcpy(buf, currline->data, sizeof(buf));
		    strcpy(currline->data, line);
		    strcpy(line, buf);
		    currline->len = strlen(currline->data);
		    currpnt = 0;
		    line_dirty = 1;
		}
		break;
	    case Ctrl('S'):
		search_str(0);
		break;
	    case Ctrl('U'):
		insert_char('\033');
		line_dirty = 1;
		break;
	    case Ctrl('V'):	/* Toggle ANSI color */
		my_ansimode ^= 1;
		if (my_ansimode && blockln >= 0)
		    block_color();
		clear();
		redraw_everything = YEA;
		line_dirty = 1;
		break;
	    case Ctrl('I'):
		do {
		    insert_char(' ');
		} while (currpnt & 0x7);
		line_dirty = 1;
		break;
	    case '\r':
	    case '\n':
#ifdef MAX_EDIT_LINE
		if( totaln == MAX_EDIT_LINE ){
		    vmsg("MAX_EDIT_LINE exceed");
		    break;
		}
#endif
		split(currline, currpnt);
		oldcurrline = currline;
		line_dirty = 0;
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
			int             indent_mode0 = indent_mode;

			indent_mode = 0;
			while (fgets(line, WRAPMARGIN + 2, fp1)) {
			    if (!strncmp(line, "�@��:", 5) ||
				!strncmp(line, "���D:", 5) ||
				!strncmp(line, "�ɶ�:", 5))
				continue;
			    insert_string(line);
			}
			fclose(fp1);
			indent_mode = indent_mode0;
			while (curr_window_line >= b_lines) {
			    curr_window_line--;
			    top_of_win = top_of_win->next;
			}
		    }
		}
		redraw_everything = YEA;
		line_dirty = 1;
		break;
	    case Ctrl('Z'):	/* Help */
		more("etc/ve.hlp", YEA);
		redraw_everything = YEA;
		line_dirty = 1;
		break;
	    case Ctrl('L'):
		clear();
		redraw_everything = YEA;
		line_dirty = 1;
		break;
	    case KEY_LEFT:
		if (currpnt) {
		    if (my_ansimode)
			currpnt = n2ansi(currpnt, currline);
		    currpnt--;
		    if (my_ansimode)
			currpnt = ansi2n(currpnt, currline);
		    line_dirty = 1;
		} else if (currline->prev) {
		    curr_window_line--;
		    currln--;
		    currline = currline->prev;
		    currpnt = currline->len;
		    line_dirty = 0;
		}
		break;
	    case KEY_RIGHT:
		if (currline->len != currpnt) {
		    if (my_ansimode)
			currpnt = n2ansi(currpnt, currline);
		    currpnt++;
		    if (my_ansimode)
			currpnt = ansi2n(currpnt, currline);
		    line_dirty = 1;
		} else if (currline->next) {
		    currpnt = 0;
		    curr_window_line++;
		    currln++;
		    currline = currline->next;
		    line_dirty = 0;
		}
		break;
	    case KEY_UP:
	    case Ctrl('P'):
		if (currline->prev) {
		    if (my_ansimode)
			ch = n2ansi(currpnt, currline);
		    curr_window_line--;
		    currln--;
		    currline = currline->prev;
		    if (my_ansimode)
			currpnt = ansi2n(ch, currline);
		    else
			currpnt = (currline->len > lastindent) ? lastindent :
			    currline->len;
		    line_dirty = 0;
		}
		break;
	    case KEY_DOWN:
	    case Ctrl('N'):
		if (currline->next) {
		    if (my_ansimode)
			ch = n2ansi(currpnt, currline);
		    currline = currline->next;
		    curr_window_line++;
		    currln++;
		    if (my_ansimode)
			currpnt = ansi2n(ch, currline);
		    else
			currpnt = (currline->len > lastindent) ? lastindent :
			    currline->len;
		    line_dirty = 0;
		}
		break;
	    case Ctrl('B'):
	    case KEY_PGUP:
		redraw_everything = currln;
		top_of_win = back_line(top_of_win, 22);
		currln = redraw_everything;
		currline = back_line(currline, 22);
		curr_window_line = getlineno();
		if (currpnt > currline->len)
		    currpnt = currline->len;
		redraw_everything = YEA;
		line_dirty = 0;
		break;
	    case KEY_PGDN:
	    case Ctrl('F'):
		redraw_everything = currln;
		top_of_win = forward_line(top_of_win, 22);
		currln = redraw_everything;
		currline = forward_line(currline, 22);
		curr_window_line = getlineno();
		if (currpnt > currline->len)
		    currpnt = currline->len;
		redraw_everything = YEA;
		line_dirty = 0;
		break;
	    case KEY_END:
	    case Ctrl('E'):
		currpnt = currline->len;
		line_dirty = 1;
		break;
	    case Ctrl(']'):	/* start of file */
		currline = top_of_win = firstline;
		currpnt = currln = curr_window_line = 0;
		redraw_everything = YEA;
		line_dirty = 0;
		break;
	    case Ctrl('T'):	/* tail of file */
		top_of_win = back_line(lastline, 23);
		currline = lastline;
		curr_window_line = getlineno();
		currln = totaln;
		redraw_everything = YEA;
		currpnt = 0;
		line_dirty = 0;
		break;
	    case KEY_HOME:
	    case Ctrl('A'):
		currpnt = 0;
		line_dirty = 1;
		break;
	    case KEY_INS:	/* Toggle insert/overwrite */
	    case Ctrl('O'):
		if (blockln >= 0 && insert_character) {
		    char            ans[4];

		    getdata(b_lines - 1, 0,
			    "�϶��L�եk�����J�r��(�w�]���ťզr��)",
			    ans, sizeof(ans), LCECHO);
		    insert_c = (*ans) ? *ans : ' ';
		}
		insert_character ^= 1;
		line_dirty = 1;
		break;
	    case Ctrl('H'):
	    case '\177':	/* backspace */
		line_dirty = 1;
		if (my_ansimode) {
		    my_ansimode = 0;
		    clear();
		    redraw_everything = YEA;
		} else {
		    if (currpnt == 0) {
			textline_t     *p;

			if (!currline->prev)
			    break;
			line_dirty = 0;
			curr_window_line--;
			currln--;
			currline = currline->prev;
			currline = adjustline(currline, WRAPMARGIN);

			// dirty fix. would this cause memory leak?
			oldcurrline = currline;

			currpnt = currline->len;
			redraw_everything = YEA;
			if (*killsp(currline->next->data) == '\0') {
			    delete_line(currline->next);
			    break;
			}
			p = currline;
			while (!join(p)) {
			    p = p->next;
			    if (p == NULL) {
				indigestion(2);
				abort_bbs(0);
			    }
			}
			break;
		    }
		    currpnt--;
		    delete_char();
		}
		break;
	    case Ctrl('D'):
	    case KEY_DEL:	/* delete current character */
		line_dirty = 1;
		if (currline->len == currpnt) {
		    textline_t     *p = currline;

		    while (!join(p)) {
			p = p->next;
			if (p == NULL) {
			    indigestion(2);
			    abort_bbs(0);
			}
		    }
		    line_dirty = 0;
		    redraw_everything = YEA;
		} else {
		    delete_char();
		    if (my_ansimode)
			currpnt = ansi2n(n2ansi(currpnt, currline), currline);
		}
		break;
	    case Ctrl('Y'):	/* delete current line */
		currline->len = currpnt = 0;
	    case Ctrl('K'):	/* delete to end of line */
		if (currline->len == 0) {
		    textline_t     *p = currline->next;
		    if (!p) {
			p = currline->prev;
			if (!p)
			    break;
			if (curr_window_line > 0) {
			    curr_window_line--;
			    currln--;
			}
		    }
		    if (currline == top_of_win)
			top_of_win = p;
		    delete_line(currline);
		    currline = p;
		    redraw_everything = YEA;
		    line_dirty = 0;
		    oldcurrline = currline = adjustline(currline, WRAPMARGIN);
		    break;
		}
		if (currline->len == currpnt) {
		    textline_t     *p = currline;

		    while (!join(p)) {
			p = p->next;
			if (p == NULL) {
			    indigestion(2);
			    abort_bbs(0);
			}
		    }
		    redraw_everything = YEA;
		    line_dirty = 0;
		    break;
		}
		currline->len = currpnt;
		currline->data[currpnt] = '\0';
		line_dirty = 1;
		break;
	    }
	    if (currln < 0)
		currln = 0;
	    if (curr_window_line < 0) {
		curr_window_line = 0;
		if (!top_of_win->prev)
		    indigestion(6);
		else {
		    top_of_win = top_of_win->prev;
		    rscroll();
		}
	    }
	    if (curr_window_line == b_lines) {
		curr_window_line = t_lines - 2;
		if (!top_of_win->next)
		    indigestion(7);
		else {
		    top_of_win = top_of_win->next;
		    move(b_lines, 0);
		    clrtoeol();
		    scroll();
		}
	    }
	}
	if (currpnt < t_columns - 1)
	    edit_margin = 0;
	else
	    edit_margin = currpnt / (t_columns - 8) * (t_columns - 8);

	if (!redraw_everything) {
	    if (edit_margin != last_margin) {
		last_margin = edit_margin;
		redraw_everything = YEA;
	    } else {
		move(curr_window_line, 0);
		clrtoeol();
		if (my_ansimode)
		    outs(currline->data);
		else
		    edit_outs(&currline->data[edit_margin]);
		edit_msg();
	    }
	}
    }
}

