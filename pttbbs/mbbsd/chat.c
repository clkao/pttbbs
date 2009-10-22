/* $Id$ */
#include "bbs.h"

#define CHAT_STOP_LINE	(t_lines-3)
#define CHAT_INIT_LINE	(2)

#ifndef EXP_CCW_CHAT

///////////////////////////////////////////////////////////////////////////
// CHAT helpers

static void
chat_prepare_newline(int *p_line)
{
    assert(p_line);
    move(*p_line, 0);

    if (*p_line < CHAT_STOP_LINE - 1)
    {
	// simply append
	(*p_line)++;
    }
    else 
    {
	// TODO after resize, we may need to scroll more than once.
	// scroll screen buffer
	region_scroll_up(CHAT_INIT_LINE, CHAT_STOP_LINE - CHAT_INIT_LINE);
	move(*p_line-1, 0);
    }
    clrtoeol();
}

static void
chat_clear_main_window(int *p_line)
{
    int i;
    for (i = CHAT_INIT_LINE; i < CHAT_STOP_LINE; i++)
    {
	move(i, 0);
	SOLVE_ANSI_CACHE();
	clrtoeol();
    }
    if (p_line)
	*p_line = CHAT_INIT_LINE;
}

static void
chat_draw_separate_lines()
{
    move(CHAT_INIT_LINE-1, 0);
    vpad(t_columns-1, "�w");
    move(CHAT_STOP_LINE, 0);
    vpad(t_columns-1, "�w");
}

///////////////////////////////////////////////////////////////////////////
// CHAT specific

static int      chatline;
static FILE    *flog;

static void
chat_print_line(const char *str)
{
    if (*str == '>' && !PERM_HIDE(currutmp))
	return;

    chat_prepare_newline(&chatline);
    outs(str);
    outs("\n��");

    if (flog)
	fprintf(flog, "%s\n", str);
}

static void
chat_clear(char *unused GCC_UNUSED)
{
    chat_clear_main_window(&chatline);

    // render new prompt
    move(chatline = 2, 0);
    outs("��");
}

static void
chat_footer()
{
    vs_footer("�i�ͤѫǡj",
	    " (PgUp/PgDn)�^�U��ѰO�� (Ctrl-Z)�ֳt����\t(Ctrl-C)���}��ѫ�");
}

static void
chat_prompt_footer(const char *myid)
{
    move(b_lines - 1, 0);
    clrtobot();
    outs(myid);
    outc(':');
    chat_footer();
}

static int
chat_send(int fd, const char *buf)
{
    int             len;
    char            genbuf[200];

    len = snprintf(genbuf, sizeof(genbuf), "%s\n", buf);
    return (send(fd, genbuf, len, 0) == len);
}

struct ChatBuf {
    char buf[128];
    int bufstart;
};

static int
chat_recv(struct ChatBuf *cb, int fd, char *chatroom, char *chatid, size_t chatid_size)
{
    int             c, len;
    char           *bptr;

    len = sizeof(cb->buf) - cb->bufstart - 1;
    if ((c = recv(fd, cb->buf + cb->bufstart, len, 0)) <= 0)
	return -1;
    c += cb->bufstart;

    bptr = cb->buf;
    while (c > 0) {
	len = strlen(bptr) + 1;
	if (len > c && (unsigned)len < (sizeof(cb->buf)/ 2) )
	    break;

	if (*bptr == '/') {
	    switch (bptr[1]) {
	    case 'c':
		chat_clear(NULL);
		break;
	    case 'n':
		strlcpy(chatid, bptr + 2, chatid_size);
		chat_prompt_footer(chatid);
		break;
	    case 'r':
		strlcpy(chatroom, bptr + 2, IDLEN+1);
		break;
	    case 't':
		move(0, 0);
		clrtoeol();
		prints(ANSI_COLOR(1;37;46) " �ͤѫ� [%-12s] " ANSI_COLOR(45) " ���D: %-48s" ANSI_RESET,
		       chatroom, bptr + 2);
	    }
	} else
	    chat_print_line(bptr);

	c -= len;
	bptr += len;
    }

    if (c > 0) {
	memmove(cb->buf, bptr, sizeof(cb->buf)-(bptr-cb->buf));
	cb->bufstart = len - 1;
    } else
	cb->bufstart = 0;
    return 0;
}

static void
chathelp(const char *cmd, const char *desc)
{
    char            buf[STRLEN];

    snprintf(buf, sizeof(buf), "  %-20s- %s", cmd, desc);
    chat_print_line(buf);
}

static void
chat_help(char *arg)
{
    if (strstr(arg, " op")) {
	chat_print_line("�ͤѫǺ޲z���M�Ϋ��O");
	chathelp("[/f]lag [+-][ls]", "�]�w��w�B���K���A");
	chathelp("[/i]nvite <id>", "�ܽ� <id> �[�J�ͤѫ�");
	chathelp("[/k]ick <id>", "�N <id> ��X�ͤѫ�");
	chathelp("[/o]p <id>", "�N Op ���v�O�ಾ�� <id>");
	chathelp("[/t]opic <text>", "���Ӹ��D");
	chathelp("[/w]all", "�s�� (�����M��)");
	chathelp(" /ban <userid>", "�ڵ� <userid> �A���i�J����ѫ� (�[�J�¦W��)");
	chathelp(" /unban <userid>", "�� <userid> ���X�¦W��");
    } else {
	chathelp(" /help op", "�ͤѫǺ޲z���M�Ϋ��O");
	chathelp("[//]help", "MUD-like ����ʵ�");
	chathelp("[/a]ct <msg>", "���@�Ӱʧ@");
	chathelp("[/b]ye [msg]", "�D�O");
	chathelp("[/c]lear", "�M���ù�");
	chathelp("[/j]oin <room>", "�إߩΥ[�J�ͤѫ�");
	chathelp("[/l]ist [room]", "�C�X�ͤѫǨϥΪ�");
	chathelp("[/m]sg <id> <msg>", "�� <id> ��������");
	chathelp("[/n]ick <id>", "�N�ͤѥN������ <id>");
	chathelp("[/p]ager", "�����I�s��");
	chathelp("[/r]oom ", "�C�X�@��ͤѫ�");
	chathelp("[/w]ho", "�C�X���ͤѫǨϥΪ�");
	chathelp(" /whoin <room>", "�C�X�ͤѫ�<room> ���ϥΪ�");
	chathelp(" /ignore <userid>", "�������w�ϥΪ̪��T��");
	chathelp(" /unignore <userid>", "��������w�ϥΪ̪��T��");
    }
}

static void
chat_date(char *unused GCC_UNUSED)
{
    char            genbuf[200];

    snprintf(genbuf, sizeof(genbuf),
	     "�� " BBSNAME "�зǮɶ�: %s", Cdate(&now));
    chat_print_line(genbuf);
}

static void
chat_pager(char *unused GCC_UNUSED)
{
    char            genbuf[200];

    char           *msgs[PAGER_MODES] = {
	/* Ref: please match PAGER* in modes.h */
	"����", "���}", "�ޱ�", "����", "�n��"
    };

    snprintf(genbuf, sizeof(genbuf), "�� �z���I�s��:[%s]",
	     msgs[currutmp->pager = (currutmp->pager + 1) % PAGER_MODES]);
    chat_print_line(genbuf);
}

static void
chat_query(char *arg)
{
    char           *uid;
    int             tuid;
    userec_t        xuser;
    char *strtok_pos;

    chat_print_line("");
    strtok_r(arg, str_space, &strtok_pos);
    if ((uid = strtok_r(NULL, str_space, &strtok_pos)) && (tuid = getuser(uid, &xuser))) {
	char            buf[ANSILINELEN], *ptr;
	FILE           *fp;

	snprintf(buf, sizeof(buf), 
		"%s(%s) " STR_LOGINDAYS " %d " STR_LOGINDAYS_QTY "�A�o��L %d �g�峹",
		 xuser.userid, xuser.nickname,
		 xuser.numlogindays, xuser.numposts);
	chat_print_line(buf);

	snprintf(buf, sizeof(buf),
		 "�̪�(%s)�q[%s]�W��", 
		 Cdate(xuser.lastseen ? &xuser.lastseen : &xuser.lastlogin),
		(xuser.lasthost[0] ? xuser.lasthost : "(����)"));
	chat_print_line(buf);

	sethomefile(buf, xuser.userid, fn_plans);
	if ((fp = fopen(buf, "r"))) {
	    tuid = 0;
	    while (tuid++ < MAX_QUERYLINES && fgets(buf, sizeof(buf), fp)) {
		if ((ptr = strchr(buf, '\n')))
		    ptr[0] = '\0';
		chat_print_line(buf);
	    }
	    fclose(fp);
	}
    } else
	chat_print_line(err_uid);
}

typedef struct chat_command_t {
    char           *cmdname;	/* Chatroom command length */
    void            (*cmdfunc) (char *);	/* Pointer to function */
}               chat_command_t;

static const chat_command_t chat_cmdtbl[] = {
    {"help", chat_help},
    {"clear", chat_clear},
    {"date", chat_date},
    {"pager", chat_pager},
    {"query", chat_query},
    {NULL, NULL}
};

static int
chat_cmd_match(const char *buf, const char *str)
{
    while (*str && *buf && !isspace((int)*buf))
	if (tolower(*buf++) != *str++)
	    return 0;
    return 1;
}

static int
chat_cmd(char *buf, int fd GCC_UNUSED)
{
    int             i;

    if (*buf++ != '/')
	return 0;

    for (i = 0; chat_cmdtbl[i].cmdname; i++) {
	if (chat_cmd_match(buf, chat_cmdtbl[i].cmdname)) {
	    chat_cmdtbl[i].cmdfunc(buf);
	    return 1;
	}
    }
    return 0;
}

typedef struct {
    struct ChatBuf *chatbuf;
    int    cfd;
    char  *chatroom;
    char  *chatid;
    int	  *chatting;
    char  *logfpath;
} ChatCbParam;

static int
_vgetcb_peek(int key, VGET_RUNTIME *prt GCC_UNUSED, void *instance)
{
    ChatCbParam *p = (ChatCbParam*) instance;
    assert(p);

    switch (key) {
	case I_OTHERDATA: // incoming
	    // XXX why 9? I don't know... just copied from old code.
	    if (chat_recv(p->chatbuf, p->cfd, p->chatroom, p->chatid, 9) == -1) {
		chat_send(p->cfd, "/b");
		*(p->chatting) = 0;
		return VGETCB_ABORT;
	    }
	    return VGETCB_NEXT;

	case Ctrl('C'):
	    chat_send(p->cfd, "/b");
	    *(p->chatting) = 0;
	    return VGETCB_ABORT;

	case Ctrl('I'):
	    {
		VREFSCR scr = vscr_save();
		add_io(0, 0);
		t_idle();
		vscr_restore(scr);
		add_io(p->cfd, 0);
	    }
	    return VGETCB_NEXT;

	case KEY_PGUP:
	case KEY_PGDN:
	    if (flog)
	    {
		VREFSCR scr = vscr_save();
		add_io(0, 0);

		fflush(flog);
		more(p->logfpath, YEA);

		vscr_restore(scr);
		add_io(p->cfd, 0);
	    }
	    return VGETCB_NEXT;

	    // Support ZA because chat is mostly independent and secure.
	case Ctrl('Z'):
	    {
		int za = 0;
		VREFCUR cur = vcur_save();
		add_io(0, 0);
		za = ZA_Select();
		chat_footer();
		vcur_restore(cur);
		add_io(p->cfd, 0);
		if (za)
		    return VGETCB_ABORT;
		return VGETCB_NEXT;
	    }
    }
    return VGETCB_NONE;
}

int
do_chat(int cfd)
{
    char chatroom[IDLEN+1] = "";/* Chat-Room Name */
    char inbuf[80], chatid[20] = "", *ptr = "";
    char hasnewmail = 0;
    char fpath[PATHLEN];
    int  chatting = YEA;
    const int chatid_len = 10;

    struct ChatBuf  chatbuf;
    ChatCbParam	    vgetparam = {0};

    memset(&chatbuf, 0, sizeof(chatbuf));
    while (1) {
	getdata(b_lines - 1, 0, "�п�J�Q�ϥΪ���Ѽʺ١G", chatid, 9, DOECHO);
	if(!chatid[0])
	    strlcpy(chatid, cuser.userid, sizeof(chatid));
	chatid[8] = '\0';
	/*
	 * �s�榡:    /! UserID ChatID Password
	 */
	snprintf(inbuf, sizeof(inbuf), "/! %s %s %s",
		cuser.userid, chatid, cuser.passwd);
	chat_send(cfd, inbuf);
	if (recv(cfd, inbuf, 3, 0) != 3) {
	    close(cfd);
           vmsg("�t�ο��~�C");
	    return 0;
	}
	if (!strcmp(inbuf, CHAT_LOGIN_OK))
	    break;
	else if (!strcmp(inbuf, CHAT_LOGIN_EXISTS))
	    ptr = "�o�ӥN���w�g���H�ΤF";
	else if (!strcmp(inbuf, CHAT_LOGIN_INVALID))
	    ptr = "�o�ӥN���O���~��";
	else if (!strcmp(inbuf, CHAT_LOGIN_BOGUS))
	    ptr = "�ФŬ��������i�J��ѫ� !!";

	move(b_lines - 2, 0);
	outs(ptr);
	clrtoeol();
	bell();
    }

    add_io(cfd, 0);

    setutmpmode(CHATING);
    currutmp->in_chat = YEA;
    strlcpy(currutmp->chatid, chatid, sizeof(currutmp->chatid));

    clear();
    chat_draw_separate_lines();
#define QHELP_STR " /h �d�߫��O  /b ���} "
    move(CHAT_STOP_LINE, (t_columns - sizeof(QHELP_STR))/2*2 );
    outs(QHELP_STR);
    chat_prompt_footer(chatid);
    memset(inbuf, 0, sizeof(inbuf));
    chatline = CHAT_INIT_LINE;

    setuserfile(fpath, "chat_XXXXXX");
    flog = fdopen(mkstemp(fpath), "w");

    // set up vgetstring callback parameter
    VGET_CALLBACKS vge = { _vgetcb_peek };

    vgetparam.chatbuf = &chatbuf;
    vgetparam.cfd = cfd;
    vgetparam.chatid = chatid;
    vgetparam.chatroom = chatroom;
    vgetparam.chatting = &chatting;
    vgetparam.logfpath = fpath;

    while (chatting) {
	if (ZA_Waiting())
	{
	    // process ZA
	    VREFSCR scr = vscr_save();
	    add_io(0, 0);
	    ZA_Enter();
	    vscr_restore(scr);
	    add_io(cfd, 0);
	}
	chat_prompt_footer(chatid);
	move(b_lines-1, chatid_len);

	// chatid_len = 10, quote(:) occupies 1, so 79-11=68
	vgetstring(inbuf, 68, VGET_TRANSPARENT, "", &vge, &vgetparam);

	// quick check for end flag or exit command.
	if (!chatting)
	    break;

	if (strncasecmp(inbuf, "/b", 2) == 0)
	{
	    // cases: /b, /bye, "/b "
	    // !cases: /ban
	    if (tolower(inbuf[2]) != 'a')
		break;
	}

	// quick continue for empty input
	if (!*inbuf)
	    continue;

#ifdef EXP_ANTIFLOOD
	{
	    // prevent flooding */
	    static time4_t lasttime = 0;
	    static int flood = 0;

	    syncnow();
	    if (now - lasttime < 3 )
	    {
		// 3 ���~�b���O���檺 ((25-5)/2)
		if( ++flood > 10 ){
		    // flush all input!
		    drop_input();
		    while (wait_input(1, 0))
		    {
			if (num_in_buf())
			    drop_input();
			else
			    tty_read((unsigned char*)inbuf, sizeof(inbuf));
		    }
		    drop_input();
		    vmsg("�ФŤj�q�ŶK�γy���~�O�����ĪG�C");
		    // log?
		    sleep(2);
		    continue;
		}
	    } else {
		lasttime = now;
		flood = 0;
	    }
	}
#endif // anti-flood

	// send message to server if possible.
	if (!chat_cmd(inbuf, cfd))
	    chatting = chat_send(cfd, inbuf);

	// print mail message if possible.
	if (ISNEWMAIL(currutmp))
	{
	    if (!hasnewmail)
	    {
		chat_print_line("�� �z����Ū���s�H��C");
		hasnewmail = 1;
	    }
	} else {
	    if (hasnewmail)
		hasnewmail = 0;
	}
    }

    close(cfd);
    add_io(0, 0);
    currutmp->in_chat = currutmp->chatid[0] = 0;

    if (flog) {
	char            ans[4];

	fclose(flog);
	more(fpath, NA);
	getdata(b_lines - 1, 0, "�M��(C) ���ܳƧѿ�(M) (C/M)?[C]",
		ans, sizeof(ans), LCECHO);
	if (*ans == 'm') {
	    if (mail_log2id(cuser.userid, "�|ĳ�O��",
		    fpath, "[��.��.��]", 0, 1) < 0)
		vmsg("�Ƨѿ��x�s���ѡC");
	}
	unlink(fpath);
    }
    return 0;
}

#endif // !EXP_CCW_CHAT

int
t_chat(void)
{
    static time4_t lastEnter = 0;
    int    fd;

    if (!HasUserPerm(PERM_CHAT))
	return -1;

    if(HasUserPerm(PERM_VIOLATELAW))
    {
       vmsg("�Х�ú�@��~��ϥβ�ѫ�!");
       return -1;
    }

#ifdef CHAT_GAPMINS
    if ((now - lastEnter)/60 < CHAT_GAPMINS)
    {
       vmsg("�z�~�����}��ѫǡA�̭����b��z���C�еy��A�աC");
       return 0;
    }
#endif

#ifdef CHAT_REGDAYS
    if ((now - cuser.firstlogin)/DAY_SECONDS < CHAT_REGDAYS)
    {
	int i = CHAT_REGDAYS - (now-cuser.firstlogin)/DAY_SECONDS +1;
	vmsgf("�z�٤�����`�� (�A�� %d �ѧa)", i);
	return 0;
    }
#endif

    // start to create connection.
    syncnow();
    move(b_lines, 0); clrtoeol();
    outs(" ���b�P��ѫǳs�u... ");
    refresh();
    fd = toconnect(XCHATD_ADDR);
    move(b_lines-1, 0); clrtobot();
    if (fd < 0) {
	outs(" ��ѫǥ��b��z���A�еy�ԦA�աC");
	system("bin/xchatd");
	pressanykey();
	return -1;
    }

    // mark for entering
    syncnow();
    lastEnter = now;

#ifdef EXP_CCW_CHAT
    return ccw_chat(fd);
#else
    return do_chat(fd);
#endif
}

