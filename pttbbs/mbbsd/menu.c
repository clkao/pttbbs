/* $Id$ */
#include "bbs.h"

#define CheckMenuPerm(x) ( (x) ? HasUserPerm(x) : 1)

/* help & menu processring */
static int      refscreen = NA;
extern char    *boardprefix;
extern struct utmpfile_t *utmpshm;
extern char     board_hidden_status;


static const char *title_tail_msgs[] = {
    "�ݪO",
    "��K",
    "�t�C",
};
static const char *title_tail_attrs[] = {
    ANSI_COLOR(37),
    ANSI_COLOR(32),
    ANSI_COLOR(36),
};
enum {
    TITLE_TAIL_BOARD = 0,
    TITLE_TAIL_SELECT,
    TITLE_TAIL_DIGEST,
};

void
showtitle(const char *title, const char *mid)
{
    /* we have to...
     * - display title in left, cannot truncate.
     * - display mid message, cannot truncate
     * - display tail (board info), if possible.
     */
    int llen = -1, rlen = -1, mlen = -1, mpos = 0;
    int pos = 0;
    int tail_type = TITLE_TAIL_BOARD;
    const char *mid_attr = ANSI_COLOR(33);

    static char     lastboard[16] = {0};
    char buf[64];

    if (currmode & MODE_SELECT)
       tail_type = TITLE_TAIL_SELECT;
    else if (currmode & MODE_DIGEST)
	tail_type = TITLE_TAIL_DIGEST;

    /* check if board was changed. */
    if (strcmp(currboard, lastboard) != 0 && currboard[0]) {
	int bid = getbnum(currboard);
	if(bid > 0)
	{
	    board_hidden_status = ((getbcache(bid)->brdattr & BRD_HIDE) &&
				   (getbcache(bid)->brdattr & BRD_POSTMASK));
	    strncpy(lastboard, currboard, sizeof(lastboard));
	}
    }

    /* next, determine if title was overrided. */
#ifdef DEBUG
    {
	sprintf(buf, "  current pid: %6d  ", getpid());
	mid = buf; 
	mid_attr = ANSI_COLOR(41;5);
	mlen = strlen(mid);
    }
#else
    if (currutmp->mailalert) {
	mid = "   �l�t�ӫ��a�o   ";
	mid_attr = ANSI_COLOR(41;5);
	mlen = strlen(mid);
    } else if ( HasUserPerm(PERM_ACCTREG) ) {
	int nreg = dashs((char *)fn_register) / 163;
	if(nreg > 100)
	{
	    sprintf(buf, "  �� %03d ���f��  ", nreg);
	    mid_attr = ANSI_COLOR(41;5);
	    mid = buf;
	    mlen = strlen(mid);
	}
    }
#endif
    /* now, calculate real positioning info */
    if(llen < 0) llen = strlen(title);
    if(mlen < 0) mlen = strlen(mid);
    mpos = (t_columns - mlen)/2;

    /* first, print left. */
    clear();
    outs(TITLE_COLOR "�i");
    outs(title);
    outs("�j");
    pos = llen + 4;
    /* prepare for mid */
    while(pos < mpos)
	outc(' '), pos++;
    outs(mid_attr);
    outs(mid), pos+=mlen;
    outs(TITLE_COLOR);
    /* try to locate right */
    rlen = strlen(currboard) + 4 + 4;
    if(currboard[0] && pos+rlen <= t_columns)
    {
	// print right stuff
	while(++pos < t_columns-rlen)
	    outc(' ');
	outs(title_tail_attrs[tail_type]);
	outs(title_tail_msgs[tail_type]);
	outs("�m");
	if (board_hidden_status)
	    outs(ANSI_COLOR(32));
	outs(currboard);
	outs(title_tail_attrs[tail_type]);
	outs("�n" ANSI_RESET "\n");
    } else {
	// just pad it.
	while(++pos < t_columns)
	    outc(' ');
	outs(ANSI_RESET "\n");
    }

}

/* �ʵe�B�z */
#define FILMROW 11
static const unsigned char menu_row = 12;
static const unsigned char menu_column = 20;

static void
show_status(void)
{
    int i;
    struct tm      *ptime = localtime4(&now);
    char            mystatus[160];
    char           *myweek = "�Ѥ@�G�T�|����";
    const char     *msgs[] = {"����", "���}", "�ޱ�", "����", "�n��"};

    i = ptime->tm_wday << 1;
    snprintf(mystatus, sizeof(mystatus),
	     ANSI_COLOR(34;46) "[%d/%d �P��%c%c %d:%02d]" 
	     ANSI_COLOR(1;33;45) "%-14s"
	     ANSI_COLOR(30;47) " �ثe�{�̦�" ANSI_COLOR(31) 
	     "%d" ANSI_COLOR(30) "�H, �ڬO" ANSI_COLOR(31) "%-12s"
	     ANSI_COLOR(30) ,
	     ptime->tm_mon + 1, ptime->tm_mday, myweek[i], myweek[i + 1],
	     ptime->tm_hour, ptime->tm_min, currutmp->birth ?
	     "�ͤ�n�Ыȭ�" : SHM->today_is,
	     SHM->UTMPnumber, cuser.userid);
    outmsg(mystatus);
    i = strlen(mystatus) - (3*7+25);
    sprintf(mystatus, "[����]" ANSI_COLOR(31) "%s " ANSI_RESET,
	msgs[currutmp->pager]);
    outslr("", i, mystatus, strlen(msgs[currutmp->pager]) + 7);
}

void
movie(int i)
{
    static short    history[MAX_HISTORY];
    int             j;

    if ((currstat != CLASS) && (cuser.uflag & MOVIE_FLAG) &&
	!SHM->Pbusystate && SHM->max_film > 0) {
	if (currstat == PSALE) {
	    i = PSALE;
	    reload_money();
	} else {
	    do {
		if (!i)
		    i = 1 + (int)(((float)SHM->max_film *
				   random()) / (RAND_MAX + 1.0));

		for (j = SHM->max_history; j >= 0; j--)
		    if (i == history[j]) {
			i = 0;
			break;
		    }
	    } while (i == 0);
	}

	memmove(history, &history[1], SHM->max_history * sizeof(short));
	history[SHM->max_history] = j = i;

	if (i == 999)		/* Goodbye my friend */
	    i = 0;

	move(1, 0);
	clrtoline(1 + FILMROW);	/* �M���W���� */
	out_lines(SHM->notes[i], 11);	/* �u�L11��N�n */
	outs(reset_color);
    }
    show_status();
    refresh();
}

static int
show_menu(const commands_t * p)
{
    register int    n = 0;
    register char  *s;

    movie(currstat);

    move(menu_row, 0);
    while ((s = p[n].desc)) {
	if (CheckMenuPerm(p[n].level)) {
	    prints("%*s  (" ANSI_COLOR(1;36) "%c" ANSI_COLOR(0) ")%s\n", menu_column, "", s[1],
		   s+2);
	}
	n++;
    }
    return n - 1;
}

void
domenu(int cmdmode, const char *cmdtitle, int cmd, const commands_t cmdtable[])
{
    int             lastcmdptr;
    int             n, pos, total, i;
    int             err;

    // XXX do we really need this ?
    static char     cmd0[MODE_MAX];

    if (cmd0[cmdmode])
	cmd = cmd0[cmdmode];

    setutmpmode(cmdmode);

    showtitle(cmdtitle, BBSName);

    total = show_menu(cmdtable);

    show_status();
    lastcmdptr = pos = 0;

    do {
	i = -1;
	switch (cmd) {
	case Ctrl('I'):
	    t_idle();
	    refscreen = YEA;
	    i = lastcmdptr;
	    break;
	case Ctrl('N'):
	    New();
	    refscreen = YEA;
	    i = lastcmdptr;
	    break;
	case Ctrl('A'):
	    if (mail_man() == FULLUPDATE)
		refscreen = YEA;
	    i = lastcmdptr;
	    break;
	case KEY_DOWN:
	    i = lastcmdptr;
	case KEY_HOME:
	case KEY_PGUP:
	    do {
		if (++i > total)
		    i = 0;
	    } while (!CheckMenuPerm(cmdtable[i].level));
	    break;
	case KEY_END:
	case KEY_PGDN:
	    i = total;
	    break;
	case KEY_UP:
	    i = lastcmdptr;
	    do {
		if (--i < 0)
		    i = total;
	    } while (!CheckMenuPerm(cmdtable[i].level));
	    break;
	case KEY_LEFT:
	case 'e':
	case 'E':
	    if (cmdmode == MMENU)
		cmd = 'G';
	    else if ((cmdmode == MAIL) && chkmailbox())
		cmd = 'R';
	    else
		return;
	default:
	    if ((cmd == 's' || cmd == 'r') &&
	    (currstat == MMENU || currstat == TMENU || currstat == XMENU)) {
		if (cmd == 's')
		    ReadSelect();
		else
		    Read();
		refscreen = YEA;
		i = lastcmdptr;
		break;
	    }
	    if (cmd == '\n' || cmd == '\r' || cmd == KEY_RIGHT) {
		move(b_lines, 0);
		clrtoeol();

		currstat = XMODE;

		if ((err = (*cmdtable[lastcmdptr].cmdfunc) ()) == QUIT)
		    return;
		currutmp->mode = currstat = cmdmode;

		if (err == XEASY) {
		    refresh();
		    safe_sleep(1);
		} else if (err != XEASY + 1 || err == FULLUPDATE)
		    refscreen = YEA;

		if (err != -1)
		    cmd = cmdtable[lastcmdptr].desc[0];
		else
		    cmd = cmdtable[lastcmdptr].desc[1];
		cmd0[cmdmode] = cmdtable[lastcmdptr].desc[0];
	    }
	    if (cmd >= 'a' && cmd <= 'z')
		cmd &= ~0x20;
	    while (++i <= total)
		if (cmdtable[i].desc[1] == cmd)
		    break;

	    if (!CheckMenuPerm(cmdtable[i].level)) {
		for (i = 0; cmdtable[i].desc; i++)
		    if (CheckMenuPerm(cmdtable[i].level))
			break;
		if (!cmdtable[i].desc)
		    return;
	    }

	    if (cmd == 'H' && i > total){
		/* TODO: Add menu help */
	    }
	}

	if (i > total || !CheckMenuPerm(cmdtable[i].level))
	    continue;

	if (refscreen) {
	    showtitle(cmdtitle, BBSName);

	    show_menu(cmdtable);

	    show_status();
	    refscreen = NA;
	}
	cursor_clear(menu_row + pos, menu_column);
	n = pos = -1;
	while (++n <= (lastcmdptr = i))
	    if (CheckMenuPerm(cmdtable[n].level))
		pos++;

	cursor_show(menu_row + pos, menu_column);
    } while (((cmd = igetch()) != EOF) || refscreen);

    abort_bbs(0);
}
/* INDENT OFF */

/* administrator's maintain menu */
static const commands_t adminlist[] = {
    {m_user, PERM_ACCOUNTS,           "UUser          �ϥΪ̸��"},
    {search_user_bypwd, PERM_SYSOP,   "SSearch User   �S��j�M�ϥΪ�"},
    {search_user_bybakpwd,PERM_SYSOP, "OOld User data �d�\\�ƥ��ϥΪ̸��"},
    {m_board, PERM_SYSOP,             "BBoard         �]�w�ݪO"},
    {m_register, PERM_ACCOUNTS|PERM_ACCTREG,
                                      "RRegister      �f�ֵ��U���"},
    {cat_register, PERM_SYSOP,        "CCatregister   �L�k�f�֮ɥΪ�"},
    {x_file, PERM_SYSOP|PERM_VIEWSYSOP,
                                      "XXfile         �s��t���ɮ�"},
    {give_money, PERM_SYSOP|PERM_VIEWSYSOP,
                                      "GGivemoney     ���]��"},
    {m_loginmsg, PERM_SYSOP,          "MMessage Login �i�����y"},
#ifdef  HAVE_MAILCLEAN
    {m_mclean, PERM_SYSOP,            "MMail Clean    �M�z�ϥΪ̭ӤH�H�c"},
#endif
#ifdef  HAVE_REPORT
    {m_trace, PERM_SYSOP,             "TTrace         �]�w�O�_�O��������T"},
#endif
    {NULL, 0, NULL}
};

/* mail menu */
static const commands_t maillist[] = {
    {m_new, PERM_READMAIL,      "RNew           �\\Ū�s�i�l��"},
    {m_read, PERM_READMAIL,     "RRead          �h�\\��Ū�H���"},
    {m_send, PERM_LOGINOK,      "RSend          �����H�H"},
    {x_love, PERM_LOGINOK,      "PPaper         " ANSI_COLOR(1;32) "���Ѳ��;�" ANSI_RESET " "},
    {mail_list, PERM_LOGINOK,   "RMail List     �s�ձH�H"},
    {setforward, PERM_LOGINOK, "FForward       " ANSI_COLOR(32) "�]�w�H�c�۰���H" ANSI_RESET},
    {m_sysop, 0,                "YYes, sir!     �ԴA����"},
    {m_internet, PERM_INTERNET, "RInternet      �H�H�� Internet"},
    {mail_mbox, PERM_INTERNET,  "RZip UserHome  ��Ҧ��p�H��ƥ��]�^�h"},
    {built_mail_index, PERM_LOGINOK, "SSavemail      ���ثH�c����"},
    {mail_all, PERM_SYSOP,      "RAll           �H�H���Ҧ��ϥΪ�"},
    {NULL, 0, NULL}
};

/* Talk menu */
static const commands_t talklist[] = {
    {t_users, 0,            "UUsers         ������Ѥ�U"},
    {t_pager, PERM_BASIC,   "PPager         �����I�s��"},
    {t_idle, 0,             "IIdle          �o�b"},
    {t_query, 0,            "QQuery         �d�ߺ���"},
    {t_qchicken, 0,         "WWatch Pet     �d���d��"},
    {t_talk, PERM_PAGE,     "TTalk          ��H���"},
    {t_chat, PERM_CHAT,     "CChat          ��a���{����h"},
#ifdef PLAY_ANGEL
    {t_changeangel, PERM_LOGINOK, "UAChange Angel �󴫤p�Ѩ�"},
    {t_angelmsg, PERM_ANGEL, "LLeave message �d�����p�D�H"},
#endif
    {t_display, 0,          "DDisplay       ��ܤW�X�����T"},
    {NULL, 0, NULL}
};

/* name menu */
static int t_aloha() {
    friend_edit(FRIEND_ALOHA);
    return 0;
}

static int t_special() {
    friend_edit(FRIEND_SPECIAL);
    return 0;
}

static const commands_t namelist[] = {
    {t_override, PERM_LOGINOK,"OOverRide      �n�ͦW��"},
    {t_reject, PERM_LOGINOK,  "BBlack         �a�H�W��"},
    {t_aloha,PERM_LOGINOK,    "AALOHA         �W���q���W��"},
#ifdef POSTNOTIFY
    {t_post,PERM_LOGINOK,     "NNewPost       �s�峹�q���W��"},
#endif
    {t_special,PERM_LOGINOK,  "SSpecial       ��L�S�O�W��"},
    {NULL, 0, NULL}
};

void Customize(); // user.c
int u_customize()
{
    Customize();
    return 0;
}

/* User menu */
static const commands_t userlist[] = {
    {u_info, PERM_LOGINOK,    	    "IInfo          �]�w�ӤH��ƻP�K�X"},
    {u_customize, PERM_LOGINOK,     "IUCustomize    �ӤH�Ƴ]�w"},
    {calendar, PERM_LOGINOK,        "CCalendar      �ӤH��ƾ�"},
    {u_editcalendar, PERM_LOGINOK,  "CDEditCalendar �s��ӤH��ƾ�"},
    {u_loginview, PERM_LOGINOK,     "LLogin View    ��ܶi���e��"},
#ifdef  HAVE_SUICIDE
    {u_kill, PERM_BASIC,            "IKill          �۱��I�I"},
#endif
    {u_editplan, PERM_LOGINOK,      "QQueryEdit     �s��W����"},
    {u_editsig, PERM_LOGINOK,       "SSignature     �s��ñ�W��"},
#if HAVE_FREECLOAK
    {u_cloak, PERM_LOGINOK,         "KKCloak        �����N"},
#else
    {u_cloak, PERM_CLOAK,           "KKCloak        �����N"},
#endif
    {u_register, PERM_BASIC,        "RRegister      ��g�m���U�ӽг�n"},
    {u_list, PERM_SYSOP,            "XUsers         �C�X���U�W��"},
#ifdef MERGEBBS
//    {m_sob, PERM_LOGUSER|PERM_SYSOP,             "SSOB Import    �F�y�ܨ��N"},
    {m_sob, PERM_BASIC,             "SSOB Import    �F�y�ܨ��N"},
#endif
    {NULL, 0, NULL}
};

#ifdef DEBUG
int _debug_check_keyinput();
int _debug_testregcode();
int _debug_reportstruct()
{
    clear();
    prints("boardheader_t:\t%d\n", sizeof(boardheader_t));
    prints("fileheader_t:\t%d\n", sizeof(fileheader_t));
    prints("userinfo_t:\t%d\n", sizeof(userinfo_t));
    prints("screenline_t:\t%d\n", sizeof(screenline_t));
    prints("SHM_t:\t%d\n", sizeof(SHM_t));
    prints("bid_t:\t%d\n", sizeof(bid_t));
    prints("userec_t:\t%d\n", sizeof(userec_t));
    pressanykey();
    return 0;
}

#endif

/* XYZ tool menu */
static const commands_t xyzlist[] = {
#ifndef DEBUG
    /* All these are useless in debug mode. */
#ifdef  HAVE_LICENSE
    {x_gpl, 0,       "LLicense       GNU �ϥΰ���"},
#endif
#ifdef HAVE_INFO
    {x_program, 0,   "PProgram       ���{���������P���v�ŧi"},
#endif
    {x_boardman,0,   "MMan Boards    �m�ݪO��ذϱƦ�]�n"},
//  {x_boards,0,     "HHot Boards    �m�ݪO�H��Ʀ�]�n"},
    {x_history, 0,   "HHistory       �m�ڭ̪������n"},
    {x_note, 0,      "NNote          �m�Ĳ��W���y���O�n"},
    {x_login,0,      "SSystem        �m�t�έ��n���i�n"},
    {x_week, 0,      "WWeek          �m���g���Q�j�������D�n"},
    {x_issue, 0,     "IIssue         �m����Q�j�������D�n"},
    {x_today, 0,     "TToday         �m����W�u�H���έp�n"},
    {x_yesterday, 0, "YYesterday     �m�Q��W�u�H���έp�n"},
    {x_user100 ,0,   "UUsers         �m�ϥΪ̦ʤj�Ʀ�]�n"},
#else
    {_debug_check_keyinput, 0, 
	    	     "MMKeycode      �ˬd���䱱��X�u��"},
    {_debug_testregcode, 0, 
	    	     "RRegcode       �ˬd���U�X����"},
    {_debug_reportstruct, 0, 
	    	     "RReportStruct  ���i�U�ص��c���j�p"},
#endif

    {p_sysinfo, 0,   "XXinfo         �m�d�ݨt�θ�T�n"},
    {NULL, 0, NULL}
};

/* Ptt money menu */
static const commands_t moneylist[] = {
    {p_give, 0,         "00Give        ����L�H��"},
    {save_violatelaw, 0,"11ViolateLaw  ú�@��"},
#if !HAVE_FREECLOAK
    {p_cloak, 0,        "22Cloak       ���� ����/�{��   $19  /��"},
#endif
    {p_from, 0,         "33From        �Ȯɭק�G�m     $49  /��"},
    {ordersong,0,       "44OSong       �ڮ�ʺA�I�q��   $200 /��"},
    {p_exmail, 0,       "55Exmail      �ʶR�H�c         $1000/��"},
    {NULL, 0, NULL}
};

static int p_money() {
    domenu(PSALE, "��tt�q�c��", '0', moneylist);
    return 0;
};

#if 0
const static commands_t jceelist[] = {
    {x_90,PERM_LOGINOK,	     "0090 JCEE     �i90�Ǧ~�פj���p�۬d�]�t�Ρj"},
    {x_89,PERM_LOGINOK,	     "1189 JCEE     �i89�Ǧ~�פj���p�۬d�]�t�Ρj"},
    {x_88,PERM_LOGINOK,      "2288 JCEE     �i88�Ǧ~�פj���p�۬d�]�t�Ρj"},
    {x_87,PERM_LOGINOK,      "3387 JCEE     �i87�Ǧ~�פj���p�۬d�]�t�Ρj"},
    {x_86,PERM_LOGINOK,      "4486 JCEE     �i86�Ǧ~�פj���p�۬d�]�t�Ρj"},
    {NULL, 0, NULL}
};

static int m_jcee() {
    domenu(JCEE, "��tt�d�]�t��", '0', jceelist);
    return 0;
}
#endif

static int forsearch();
static int playground();
static int chessroom();

/* Ptt Play menu */
static const commands_t playlist[] = {
#if 0
#if HAVE_JCEE
    {m_jcee, PERM_LOGINOK,   "JJCEE        �i �j���p�Ҭd�]�t�� �j"},
#endif
#endif
    {note, PERM_LOGINOK,     "NNote        �i ���y���O �j"},
/* XXX �a���F, �γ\�i�H���� weather.today/weather.tomorrow ���ϥ��S�N�q */
/* {x_weather,0 ,           "WWeather     �i ��H�w�� �j"}, */
/* XXX �a���F */
/*    {x_stock,0 ,             "SStock       �i �ѥ��污 �j"},*/
    {forsearch,PERM_LOGINOK, "SSearchEngine�i" ANSI_COLOR(1;35) " ��tt�j�M�� " ANSI_RESET "�j"},
    {topsong,PERM_LOGINOK,   "TTop Songs   �i" ANSI_COLOR(1;32) "�ڮ��I�q�Ʀ�]" ANSI_RESET "�j"},
    {p_money,PERM_LOGINOK,   "PPay         �i" ANSI_COLOR(1;31) " ��tt�q�c�� " ANSI_RESET "�j"},
    {chicken_main,PERM_LOGINOK, "CChicken     "
     "�i" ANSI_COLOR(1;34) " ��tt�i���� " ANSI_RESET "�j"},
    {playground,PERM_LOGINOK, "AAmusement   �i" ANSI_COLOR(1;33) " ��tt�C�ֳ� " ANSI_RESET "�j"},
    {chessroom, PERM_LOGINOK, "BBhineseChess�i" ANSI_COLOR(1;34) " ��tt�Ѱ| " ANSI_RESET "�j"},
    {NULL, 0, NULL}
};

static const commands_t chesslist[] = {
    {chc_main,     PERM_LOGINOK, "11CChessFight   �i" ANSI_COLOR(1;33) " �H���ܧ� " ANSI_RESET "�j"},
    {chc_personal, PERM_LOGINOK, "22CChessSelf    �i" ANSI_COLOR(1;34) " �H�ѥ��� " ANSI_RESET "�j"},
    {chc_watch,    PERM_LOGINOK, "33CChessWatch   �i" ANSI_COLOR(1;35) " �H���[�� " ANSI_RESET "�j"},
    {gomoku_main,  PERM_LOGINOK, "44GomokuFight   �i" ANSI_COLOR(1;33) "���l���ܧ�" ANSI_RESET "�j"},
    {gomoku_personal, PERM_LOGINOK, "55GomokuSelf    �i" ANSI_COLOR(1;34) "���l�ѥ���" ANSI_RESET "�j"},
    {gomoku_watch, PERM_LOGINOK, "66GomokuWatch   �i" ANSI_COLOR(1;35) "���l���[��" ANSI_RESET "�j"},
    {GoBot,        PERM_LOGINOK, "77GoBot         �i" ANSI_COLOR(1;36) " ��ѥ��� " ANSI_RESET "�j"},
    {NULL, 0, NULL}
};

static int chessroom() {
    domenu(CHC, "��tt�Ѱ|", '1', chesslist);
    return 0;
}

static const commands_t plist[] = {

/*    {p_ticket_main, PERM_LOGINOK,"00Pre         �i �`�ξ� �j"},
      {alive, PERM_LOGINOK,        "00Alive       �i  �q����  �j"},
*/
    {ticket_main, PERM_LOGINOK,  "11Gamble      �i ��tt��� �j"},
    {guess_main, PERM_LOGINOK,   "22Guess number�i �q�Ʀr   �j"},
    {othello_main, PERM_LOGINOK, "33Othello     �i �¥մ�   �j"},
//    {dice_main, PERM_LOGINOK,    "44Dice        �i ����l   �j"},
    {vice_main, PERM_LOGINOK,    "44Vice        �i �o����� �j"},
    {g_card_jack, PERM_LOGINOK,  "55Jack        �i �³ǧJ �j"},
    {g_ten_helf, PERM_LOGINOK,   "66Tenhalf     �i �Q�I�b �j"},
    {card_99, PERM_LOGINOK,      "77Nine        �i �E�Q�E �j"},
    {NULL, 0, NULL}
};

static int playground() {
    domenu(AMUSE, "��tt�C�ֳ�",'1',plist);
    return 0;
}

static const commands_t slist[] = {
    {x_dict,0,                   "11Dictionary  "
     "�i" ANSI_COLOR(1;33) " ����j�r�� " ANSI_RESET "�j"},
    {x_mrtmap, 0,                "22MRTmap      "
	 "�i" ANSI_COLOR(1;34) "  ���B�a��  " ANSI_RESET "�j"},
    {NULL, 0, NULL}
};

static int forsearch() {
    domenu(SREG, "��tt�j�M��", '1', slist);
    return 0;
}

/* main menu */

int
admin(void)
{
    domenu(ADMIN, "�t�κ��@", 'X', adminlist);
    return 0;
}

int
Mail(void)
{
    domenu(MAIL, "�q�l�l��", 'R', maillist);
    return 0;
}

int
Talk(void)
{
    domenu(TMENU, "��ѻ���", 'U', talklist);
    return 0;
}

int
User(void)
{
    domenu(UMENU, "�ӤH�]�w", 'I', userlist);
    return 0;
}

int
Xyz(void)
{
    domenu(XMENU, "�u��{��", 'M', xyzlist);
    return 0;
}

int
Play_Play(void)
{
    domenu(PMENU, "�����C�ֳ�", 'A', playlist);
    return 0;
}

int
Name_Menu(void)
{
    domenu(NMENU, "�զ⮣��", 'O', namelist);
    return 0;
}
 
