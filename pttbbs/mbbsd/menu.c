/* $Id$ */
#include "bbs.h"

// UNREGONLY �אּ�� BASIC �ӧP�_�O�_�� guest.

#define CheckMenuPerm(x) \
    ( (x == MENU_UNREGONLY)? \
      ((!HasUserPerm(PERM_BASIC) || HasUserPerm(PERM_LOGINOK))?0:1) :\
	((x) ? HasUserPerm(x) : 1))

/* help & menu processring */
static int      refscreen = NA;
extern char    *boardprefix;
extern struct utmpfile_t *utmpshm;

static const char *title_tail_msgs[] = {
    "�ݪO",
    "�t�C",
    "��K",
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
    int llen, rlen, mlen, mpos = 0;
    int pos = 0;
    int tail_type;
    const char *mid_attr = ANSI_COLOR(33);
    int is_currboard_special = 0;
    char buf[64];


    /* prepare mid */
#ifdef DEBUG
    {
	sprintf(buf, "  current pid: %6d  ", getpid());
	mid = buf; 
	mid_attr = ANSI_COLOR(41;5);
    }
#else
    if (ISNEWMAIL(currutmp)) {
	mid = "    �A���s�H��    ";
	mid_attr = ANSI_COLOR(41;5);
    } else if ( HasUserPerm(PERM_ACCTREG) ) {
	int nreg = regform_estimate_queuesize();
	if(nreg > 100)
	{
	    sprintf(buf, "  �� %03d ���f��  ", nreg);
	    mid_attr = ANSI_COLOR(41;5);
	    mid = buf;
	}
    }
#endif

    /* prepare tail */
    if (currmode & MODE_SELECT)
	tail_type = TITLE_TAIL_SELECT;
    else if (currmode & MODE_DIGEST)
	tail_type = TITLE_TAIL_DIGEST;
    else
	tail_type = TITLE_TAIL_BOARD;

    if(currbid > 0)
    {
	assert(0<=currbid-1 && currbid-1<MAX_BOARD);
	is_currboard_special = (
		(getbcache(currbid)->brdattr & BRD_HIDE) &&
		(getbcache(currbid)->brdattr & BRD_POSTMASK));
    }

    /* now, calculate real positioning info */
    llen = strlen(title);
    mlen = strlen(mid);
    mpos = (t_columns -1 - mlen)/2;

    /* first, print left. */
    clear();
    outs(TITLE_COLOR "�i");
    outs(title);
    outs("�j");
    pos = llen + 4;

    /* print mid */
    while(pos++ < mpos)
	outc(' ');
    outs(mid_attr);
    outs(mid);
    pos += mlen;
    outs(TITLE_COLOR);

    /* try to locate right */
    rlen = strlen(currboard) + 4 + 4;
    if(currboard[0] && pos+rlen < t_columns)
    {
	// print right stuff
	while(pos++ < t_columns-rlen)
	    outc(' ');
	outs(title_tail_attrs[tail_type]);
	outs(title_tail_msgs[tail_type]);
	outs("�m");

	if (is_currboard_special)
	    outs(ANSI_COLOR(32));
	outs(currboard);
	outs(title_tail_attrs[tail_type]);
	outs("�n" ANSI_RESET "\n");
    } else {
	// just pad it.
	while(pos++ < t_columns)
	    outc(' ');
	outs(ANSI_RESET "\n");
    }

}

/* Ctrl-Z Anywhere Fast Switch, not ZG. */
static char zacmd = 0;

// ZA is waiting, hurry to the meeting stone!
int 
ZA_Waiting(void)
{
    return (zacmd != 0);
}

// Promp user our ZA bar and return for selection.
int
ZA_Select(void)
{
    int k;

    // TODO refresh status bar?
    vs_footer(VCLR_ZA_CAPTION " ���ֳt����: ",
	    " (b)�峹�C�� (c)�����ݪO (f)�ڪ��̷R (m)�H�c (u)�ϥΪ̦W��");
    k = vkey();

    if (k < ' ' || k >= 'z') return 0;
    k = tolower(k);

    if(strchr("bcfmu", k) == NULL)
	return 0;

    zacmd = k;
    return 1;
}

// The ZA processor, only invoked in menu.
void 
ZA_Enter(void)
{
    char cmd = zacmd;
    while (zacmd)
    {
	cmd = zacmd;
	zacmd = 0;

	// All ZA applets must check ZA_Waiting() at every stack of event loop.
	switch(cmd) {
	    case 'b':
		Read();
		break;
	    case 'c':
		Class();
		break;
	    case 'f':
		Favorite();
		break;
	    case 'm':
		m_read();
		break;
	    case 'u':
		t_users();
		break;
	}
	// if user exit with new ZA assignment,
	// direct enter in next loop.
    }
}

/* �ʵe�B�z */
#define FILMROW 11
static unsigned short menu_row = 12;
static unsigned short menu_column = 20;

static void
show_status(void)
{
    int i;
    struct tm      *ptime = localtime4(&now);
    char           *myweek = "�Ѥ@�G�T�|����";
    const char     *msgs[] = {"����", "���}", "�ޱ�", "����", "�n��"};

    i = ptime->tm_wday << 1;
    move(b_lines, 0);
    vbarf(ANSI_COLOR(34;46) "[%d/%d �P��%c%c %d:%02d]" 
	  ANSI_COLOR(1;33;45) "%-14s"
	  ANSI_COLOR(30;47) " �u�W" ANSI_COLOR(31) 
	  "%d" ANSI_COLOR(30) "�H, �ڬO" ANSI_COLOR(31) "%s"
	  ANSI_COLOR(30) "\t[����]" ANSI_COLOR(31) "%s ",
	  ptime->tm_mon + 1, ptime->tm_mday, myweek[i], myweek[i + 1],
	  ptime->tm_hour, ptime->tm_min, currutmp->birth ?
	  "�ͤ�n�Ыȭ�" : SHM->today_is,
	  SHM->UTMPnumber, cuser.userid,
	  msgs[currutmp->pager]);
}

/*
 * current caller of movie:
 *   board.c: movie(0);    // called when IN_CLASSROOT()
 *                         // with currstat = CLASS -> don't show movies
 *   xyz.c:   movie(999999);  // logout
 *   menu.c:  movie(cmdmode); // ...
 */
#define N_SYSMOVIE (sizeof(movie_map) / sizeof(movie_map[0]))
void
movie(int cmdmode)
{
    int i;

    // movie �e�X���O Note �O��ذϡu<�t��> �ʺA�ݪO�v(SYS) �ؿ��U���峹
    // movie_map �O�ΨӨ� cmdmode �D�X�S�w���ʺA�ݪO�Aindex �� mode_map �@�ˡC
    const int movie_map[] = {
	2, 10, 11, -1, 3, -1, 12,
	7, 9, 8, 4, 5, 6,
    };

    // don't show if stat in class or user wants to skip movies
    if (currstat == CLASS || !(cuser.uflag & MOVIE_FLAG))
	return;
    // also prevent SHM busy status
    if (SHM->Pbusystate || SHM->last_film <= 0)
	return;

    if (cmdmode < N_SYSMOVIE &&
	    0 < movie_map[cmdmode] && movie_map[cmdmode] <= SHM->last_film) {
	i = movie_map[cmdmode];
    } else if (cmdmode == 999999) {	/* Goodbye my friend */
	i = 0;
    } else {
	i = N_SYSMOVIE + (int)(((float)SHM->last_film - N_SYSMOVIE + 1) * (random() / (RAND_MAX + 1.0)));
    }

    move(1, 0);
    clrtoln(1 + FILMROW);	/* �M���W���� */
#ifdef LARGETERM_CENTER_MENU
    out_lines(SHM->notes[i], 11, (t_columns - 80)/2);	/* �u�L11��N�n */
#else
    out_lines(SHM->notes[i], 11, 0);	/* �u�L11��N�n */
#endif
    outs(reset_color);
}

typedef struct {
    int     (*cmdfunc)();
    int     level;
    char    *desc;                   /* next/key/description */
} commands_t;

static int
show_menu(int moviemode, const commands_t * p)
{
    register int    n = 0;
    register char  *s;

    movie(moviemode);

    // seems not everyone likes the menu in center.
#ifdef LARGETERM_CENTER_MENU
    // update menu column [fixed const because most items are designed in this way)
    menu_column = (t_columns-40)/2;
    menu_row = 12 + (t_lines-24)/2;
#endif 

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


enum {
    M_ADMIN = 0, M_AMUSE, M_CHC, M_JCEE, M_MAIL, M_MMENU, M_NMENU,
    M_PMENU, M_PSALE, M_SREG, M_TMENU, M_UMENU, M_XMENU, M_XMAX
};

static const int mode_map[] = {
    ADMIN, AMUSE, CHC, JCEE, MAIL, MMENU, NMENU,
    PMENU, PSALE, SREG, TMENU, UMENU, XMENU,
};

static void
domenu(int cmdmode, const char *cmdtitle, int cmd, const commands_t cmdtable[])
{
    int             lastcmdptr, moviemode;
    int             n, pos, total, i;
    int             err;

    moviemode = cmdmode;
    assert(cmdmode < M_XMAX);
    cmdmode = mode_map[cmdmode];

    setutmpmode(cmdmode);

    showtitle(cmdtitle, BBSName);

    total = show_menu(moviemode, cmdtable);

    show_status();
    lastcmdptr = pos = 0;

    do {
	i = -1;
	switch (cmd) {
	case Ctrl('Z'):
	    ZA_Select(); // we'll have za loop later.
	    refscreen = YEA;
	    i = lastcmdptr;
	    break;
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
		(cmdmode == MMENU || cmdmode == TMENU || cmdmode == XMENU)) {
		if (cmd == 's')
		    ReadSelect();
		else
		    Read();
		refscreen = YEA;
		i = lastcmdptr;
		currstat = cmdmode;
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

	// end of all commands
	if (ZA_Waiting())
	{
	    ZA_Enter();
	    refscreen = 1;
	    currstat = cmdmode;
	}

	if (i > total || !CheckMenuPerm(cmdtable[i].level))
	    continue;

	if (refscreen) {
	    showtitle(cmdtitle, BBSName);
	    show_menu(moviemode, cmdtable);
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
    {m_user, PERM_SYSOP,              "UUser          �ϥΪ̸��"},
    {search_user_bypwd, PERM_ACCOUNTS|PERM_POLICE_MAN,
                                      "SSearch User   �S��j�M�ϥΪ�"},
    {search_user_bybakpwd,PERM_ACCOUNTS,"OOld User data �d�\\�ƥ��ϥΪ̸��"},
    {m_board, PERM_SYSOP|PERM_BOARD,  "BBoard         �]�w�ݪO"},
    {m_register, PERM_ACCOUNTS|PERM_ACCTREG,
                                      "RRegister      �f�ֵ��U���"},
    {x_file, PERM_SYSOP|PERM_VIEWSYSOP,
                                      "XXfile         �s��t���ɮ�"},
    {give_money, PERM_SYSOP|PERM_VIEWSYSOP,
                                      "GGivemoney     ���]��"},
    {m_loginmsg, PERM_SYSOP,          "MMessage Login �i�����y"},
    {NULL, 0, NULL}
};

/* mail menu */
static const commands_t maillist[] = {
    {m_new, PERM_READMAIL,      "RNew           �\\Ū�s�i�l��"},
    {m_read, PERM_READMAIL,     "RRead          �h�\\��Ū�H���"},
    {m_send, PERM_LOGINOK,      "RSend          �����H�H"},
    {mail_list, PERM_LOGINOK,   "RMail List     �s�ձH�H"},
    {x_love, PERM_LOGINOK,      "PPaper         ���Ѳ��;�"},
// #ifdef USE_MAIL_AUTO_FORWARD
    {setforward, PERM_LOGINOK,  "FForward       " ANSI_COLOR(1;32) 
				"�]�w�H�c�۰���H" ANSI_RESET},
// #endif // USE_MAIL_AUTO_FORWARD
    {m_sysop, 0,                "YYes, sir!     �g�H������"},
    {m_internet, PERM_INTERNET, "RInternet      �H�H�쯸�~"},
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
    // PERM_PAGE - ���y���n PERM_LOGIN �F
    // �S�D�z�i�H talk ������y�C
    {t_talk, PERM_LOGINOK,  "TTalk          ��H���"},
    // PERM_CHAT �D login �]���A�|���H�Φ��n�O�H�C
    {t_chat, PERM_LOGINOK,  "CChat          ��a���{����h"},
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
    {t_fix_aloha,PERM_LOGINOK,"XXFixALOHA     �ץ��W���q��"},
    {t_special,PERM_LOGINOK,  "SSpecial       ��L�S�O�W��"},
    {NULL, 0, NULL}
};

void Customize(); // user.c
int u_customize()
{
    Customize();
    return 0;
}

int u_fixgoodpost(void); // assess.c
/* User menu */
static const commands_t userlist[] = {
    {u_customize, PERM_BASIC,       "UUCustomize    �ӤH�Ƴ]�w"},
    {u_info, PERM_LOGINOK,    	    "IInfo          �]�w�ӤH��ƻP�K�X"},
    {calendar, PERM_LOGINOK,        "CCalendar      �ӤH��ƾ�"},
    {u_loginview, PERM_BASIC,       "LLogin View    ��ܶi���e��"},
    {u_editplan, PERM_LOGINOK,      "QQueryEdit     �s��W����"},
    {u_editsig, PERM_LOGINOK,       "SSignature     �s��ñ�W��"},
#if HAVE_FREECLOAK
    {u_cloak, PERM_LOGINOK,         "KKCloak        �����N"},
#else
    {u_cloak, PERM_CLOAK,           "KKCloak        �����N"},
#endif
    {u_register, MENU_UNREGONLY,    "RRegister      ��g�m���U�ӽг�n"},
#ifdef ASSESS
    {u_cancelbadpost, PERM_LOGINOK, "BBye BadPost   �ӽЧR���H��"},
    {u_fixgoodpost, PERM_LOGINOK,   "FFix GoodPost  �״_�u��"},
#endif // ASSESS
    {u_list, PERM_SYSOP,            "XUsers         �C�X���U�W��"},
#ifdef MERGEBBS
//    {m_sob, PERM_LOGUSER|PERM_SYSOP,             "SSOB Import    �F�y�ܨ��N"},
    {m_sob, PERM_BASIC,             "SSOB Import    �F�y�ܨ��N"},
#endif
    {NULL, 0, NULL}
};

#ifdef DEBUG
int _debug_check_keyinput();
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

static const commands_t      cmdlist[] = {
    {admin, PERM_SYSOP|PERM_ACCOUNTS|PERM_BOARD|PERM_VIEWSYSOP|PERM_ACCTREG|PERM_POLICE_MAN, 
				"00Admin       �i �t�κ��@�� �j"},
    {Announce,	0,		"AAnnounce     �i ��ؤ��G�� �j"},
#ifdef DEBUG
    {Favorite,	0,		"FFavorite     �i �ڪ��̤��R �j"},
#else
    {Favorite,	0,		"FFavorite     �i �� �� �̷R �j"},
#endif
    {Class,	0,		"CClass        �i ���հQ�װ� �j"},
    {Mail, 	PERM_BASIC,	"MMail         �i �p�H�H��� �j"},
    {Talk, 	0,		"TTalk         �i �𶢲�Ѱ� �j"},
    {User, 	PERM_BASIC,	"UUser         �i �ӤH�]�w�� �j"},
    {Xyz, 	0,		"XXyz          �i �t�θ�T�� �j"},
    {Play_Play, PERM_LOGINOK, 	"PPlay         �i �T�ֻP�� �j"},
    {Name_Menu, PERM_LOGINOK,	"NNamelist     �i �s�S�O�W�� �j"},
#ifdef DEBUG
    {Goodbye, 	0, 		"GGoodbye      �A���A���A���A��"},
#else
    {Goodbye, 	0, 		"GGoodbye         ���}�A�A���K "},
#endif
    {NULL, 	0, 		NULL}
};

int main_menu(void) {
    domenu(M_MMENU, "�D�\\���", (ISNEWMAIL(currutmp) ? 'M' : 'C'), cmdlist);
    return 0;
}

static int p_money() {
    domenu(M_PSALE, BBSMNAME2 "�q�c��", '0', moneylist);
    return 0;
};

// static int forsearch();
static int playground();
static int chessroom();

/* Ptt Play menu */
static const commands_t playlist[] = {
    {note, PERM_LOGINOK,     "NNote        �i ���y���O �j"},
    /* // useless.
    {forsearch,PERM_LOGINOK, "SSearchEngine�i" ANSI_COLOR(1;35) " " 
	BBSMNAME2 "�j�M�� " ANSI_RESET "�j"},
	*/
    {topsong,PERM_LOGINOK,   "TTop Songs   �i" ANSI_COLOR(1;32) " �I�q�Ʀ�] " ANSI_RESET "�j"},
    {p_money,PERM_LOGINOK,   "PPay         �i" ANSI_COLOR(1;31) " "
	BBSMNAME2 "�q�c�� " ANSI_RESET "�j"},
    {chicken_main,PERM_LOGINOK, "CChicken     "
     "�i" ANSI_COLOR(1;34) " " BBSMNAME2 "�i���� " ANSI_RESET "�j"},
    {playground,PERM_LOGINOK, "AAmusement   �i" ANSI_COLOR(1;33) " "
	BBSMNAME2 "�C�ֳ� " ANSI_RESET "�j"},
    {chessroom, PERM_LOGINOK, "BBChess      �i" ANSI_COLOR(1;34) " "
	BBSMNAME2 "�Ѱ|   " ANSI_RESET "�j"},
    {NULL, 0, NULL}
};

static const commands_t chesslist[] = {
    {chc_main,         PERM_LOGINOK, "11CChessFight   �i" ANSI_COLOR(1;33) " �H���ܧ� " ANSI_RESET "�j"},
    {chc_personal,     PERM_LOGINOK, "22CChessSelf    �i" ANSI_COLOR(1;34) " �H�ѥ��� " ANSI_RESET "�j"},
    {chc_watch,        PERM_LOGINOK, "33CChessWatch   �i" ANSI_COLOR(1;35) " �H���[�� " ANSI_RESET "�j"},
    {gomoku_main,      PERM_LOGINOK, "44GomokuFight   �i" ANSI_COLOR(1;33) "���l���ܧ�" ANSI_RESET "�j"},
    {gomoku_personal,  PERM_LOGINOK, "55GomokuSelf    �i" ANSI_COLOR(1;34) "���l�ѥ���" ANSI_RESET "�j"},
    {gomoku_watch,     PERM_LOGINOK, "66GomokuWatch   �i" ANSI_COLOR(1;35) "���l���[��" ANSI_RESET "�j"},
    {gochess_main,     PERM_LOGINOK, "77GoChessFight  �i" ANSI_COLOR(1;33) " ����ܧ� " ANSI_RESET "�j"},
    {gochess_personal, PERM_LOGINOK, "88GoChessSelf   �i" ANSI_COLOR(1;34) " ��ѥ��� " ANSI_RESET "�j"},
    {gochess_watch,    PERM_LOGINOK, "99GoChessWatch  �i" ANSI_COLOR(1;35) " ����[�� " ANSI_RESET "�j"},
    {NULL, 0, NULL}
};

static int chessroom() {
    domenu(M_CHC, BBSMNAME2 "�Ѱ|", '1', chesslist);
    return 0;
}

static const commands_t plist[] = {

/*    {p_ticket_main, PERM_LOGINOK,"00Pre         �i �`�ξ� �j"},
      {alive, PERM_LOGINOK,        "00Alive       �i  �q����  �j"},
*/
    {ticket_main, PERM_LOGINOK,  "11Gamble      �i " BBSMNAME2 "��� �j"},
    {guess_main, PERM_LOGINOK,   "22Guess number�i  �q�Ʀr  �j"},
    {othello_main, PERM_LOGINOK, "33Othello     �i  �¥մ�  �j"},
//    {dice_main, PERM_LOGINOK,    "44Dice        �i ����l   �j"},
    {vice_main, PERM_LOGINOK,    "44Vice        �i �o����� �j"},
    {g_card_jack, PERM_LOGINOK,  "55Jack        �i  �³ǧJ  �j"},
    {g_ten_helf, PERM_LOGINOK,   "66Tenhalf     �i  �Q�I�b  �j"},
    {card_99, PERM_LOGINOK,      "77Nine        �i  �E�Q�E  �j"},
    {NULL, 0, NULL}
};

static int playground() {
    domenu(M_AMUSE, BBSMNAME2 "�C�ֳ�",'1',plist);
    return 0;
}

static const commands_t slist[] = {
    /*
    // x_dict: useless
    {x_dict,0,                   "11Dictionary  "
     "�i" ANSI_COLOR(1;33) " ����j�r�� " ANSI_RESET "�j"},
     */
    {x_mrtmap, 0,                "22MRTmap      "
	 "�i" ANSI_COLOR(1;34) "  ���B�a��  " ANSI_RESET "�j"},
    {NULL, 0, NULL}
};

/* // nothing to search...
static int forsearch() {
    domenu(M_SREG, BBSMNAME2 "�j�M��", '1', slist);
    return 0;
}
*/

/* main menu */

int
admin(void)
{
    domenu(M_ADMIN, "�t�κ��@", 'X', adminlist);
    return 0;
}

int
Mail(void)
{
    domenu(M_MAIL, "�q�l�l��", 'R', maillist);
    return 0;
}

int
Talk(void)
{
    domenu(M_TMENU, "��ѻ���", 'U', talklist);
    return 0;
}

int
User(void)
{
    domenu(M_UMENU, "�ӤH�]�w", 'U', userlist);
    return 0;
}

int
Xyz(void)
{
    if (strcmp(cuser.userid, "piaip") == 0)
	x_file();
    else
    domenu(M_XMENU, "�u��{��", 'M', xyzlist);
    return 0;
}

int
Play_Play(void)
{
    domenu(M_PMENU, "�����C�ֳ�", 'A', playlist);
    return 0;
}

int
Name_Menu(void)
{
    domenu(M_NMENU, "�զ⮣��", 'O', namelist);
    return 0;
}
 
