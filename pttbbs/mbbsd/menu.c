/* $Id: menu.c,v 1.2 2002/03/17 06:06:26 in2 Exp $ */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <string.h>
#include "config.h"
#include "pttstruct.h"
#include "common.h"
#include "perm.h"
#include "modes.h"
#include "proto.h"
extern int usernum;
extern int talkrequest;
extern char *fn_register;
extern char currboard[];        /* name of currently selected board */
extern int currmode;
extern unsigned int currstat;
extern char reset_color[];
extern userinfo_t *currutmp;
extern char *BBSName;
extern int b_lines;             /* Screen bottom line number: t_lines-1 */

/* help & menu processring */
static int refscreen = NA;
extern char *boardprefix;
extern struct utmpfile_t *utmpshm;

int egetch() {
    int rval;

    while(1) {
        rval = igetkey();
        if(talkrequest) {
            talkreply();
            refscreen = YEA;
            return rval;
        }
        if(rval != Ctrl('L'))
            return rval;
        redoscr();
    }
}

extern userec_t cuser;
extern char *fn_board;
extern char board_hidden_status;

void showtitle(char *title, char *mid) {
    char    buf[40], numreg[50];
    int     nreg, spc = 0, pad, bid;
    boardheader_t   bh;
    static  char    lastboard[16] = {0};

    spc = strlen(mid);
    if(title[0] == 0)
        title++;
    else if(currutmp->mailalert) {
        mid = "\033[41;5m   �l�t�ӫ��a�o   " TITLE_COLOR;
        spc = 22;
    } else if(HAS_PERM(PERM_SYSOP) && (nreg = dashs(fn_register)/163)) {
	sprintf(numreg, "\033[41;5m  ��%03d/%03d���f��  " TITLE_COLOR,
		nreg,
		(int)dashs("register.new.tmp") / 163);
        mid = numreg;
        spc = 22;
    }
    spc = 66 - strlen(title) - spc - strlen(currboard);
    if(spc < 0)
        spc = 0;
    pad = 1 - (spc & 1);
    memset(buf, ' ', spc >>= 1);
    buf[spc] = '\0';
    
    clear();
    prints(TITLE_COLOR "�i%s�j%s\033[33m%s%s%s\033[3%s�m",
           title, buf, mid, buf, " " + pad,
           currmode & MODE_SELECT ? "6m�t�C" : currmode & MODE_ETC ? "5m��L" :
           currmode & MODE_DIGEST ? "2m��K" : "7m�ݪO");

    if( strcmp(currboard, lastboard) ){ /* change board */
	if( currboard[0] != 0 &&
	    (bid = getbnum(currboard)) > 0 &&
	    (get_record(fn_board, &bh, sizeof(bh), bid) != -1) ){
	    board_hidden_status = ((bh.brdattr & BRD_HIDE) &&
				   (bh.brdattr & BRD_POSTMASK));
	    strncpy(lastboard, currboard, sizeof(lastboard));
	}
    }
    
    if( board_hidden_status )
	prints("\033[32m%s", currboard);
    else
	prints("%s", currboard);
    prints("\033[3%dm�n\033[0m\n", currmode & MODE_SELECT ? 6 :
	   currmode & MODE_ETC ? 5 : currmode & MODE_DIGEST ? 2 : 7);
}

/* �ʵe�B�z */
#define FILMROW 11
static unsigned char menu_row = 12;
static unsigned char menu_column = 20;
static char mystatus[160];

static int u_movie() {
    cuser.uflag ^= MOVIE_FLAG;
    return 0;
}

void movie(int i) {
    extern struct pttcache_t *ptt;
    static short history[MAX_HISTORY];
    static char myweek[] = "�Ѥ@�G�T�|����";
    const char *msgs[] = {"����", "���}", "�ޱ�", "����","�n��"};
    time_t now = time(NULL);
    struct tm *ptime = localtime(&now);

    if((currstat != CLASS) && (cuser.uflag & MOVIE_FLAG) &&
       !ptt->busystate && ptt->max_film > 0) {
        if(currstat == PSALE) {
            i = PSALE;
            reload_money();
        } else {
            do {
                if(!i)
                    i = 1 + (int)(((float)ptt->max_film *
                                   rand()) / (RAND_MAX + 1.0));

                for(now = ptt->max_history; now >= 0; now--)
                    if(i == history[now]) {
                        i = 0;
                        break;
                    }
            } while(i == 0);
        }

        memcpy(history, &history[1], ptt->max_history * sizeof(short));
        history[ptt->max_history] = now = i;

        if(i == 999)       /* Goodbye my friend */
            i = 0;

        move(1, 0);
        clrtoline(1 + FILMROW); /* �M���W���� */
        Jaky_outs(ptt->notes[i], 11); /* �u�L11��N�n */
        outs(reset_color);
    }
    i = ptime->tm_wday << 1;
    sprintf(mystatus, "\033[34;46m[%d/%d �P��%c%c %d:%02d]\033[1;33;45m%-14s"
            "\033[30;47m �ثe�{�̦� \033[31m%d\033[30m�H, �ڬO\033[31m%-12s"
            "\033[30m[����]\033[31m%s\033[0m",
            ptime->tm_mon + 1, ptime->tm_mday, myweek[i], myweek[i + 1],
            ptime->tm_hour, ptime->tm_min, currutmp->birth ?
            "�ͤ�n�Ыȭ�" : ptt->today_is,
            utmpshm->number, cuser.userid, msgs[currutmp->pager]);
    outmsg(mystatus);
    refresh();
}

static int show_menu(commands_t *p) {
    register int n = 0;
    register char *s;
    const char *state[4]={"�Υ\\��", "�w�h��", "�۩w��", "SHUTUP"};
    char buf[80];

    movie(currstat);

    move(menu_row, 0);
    while((s = p[n].desc)) {
        if(HAS_PERM(p[n].level)) {
            sprintf(buf, s + 2, state[cuser.proverb % 4]);
            prints("%*s  (\033[1;36m%c\033[0m)%s\n", menu_column, "", s[1],
                   buf);
        }
        n++;
    }
    return n - 1;
}

void domenu(int cmdmode, char *cmdtitle, int cmd, commands_t cmdtable[]) {
    int lastcmdptr;
    int n, pos, total, i;
    int err;
    int chkmailbox();
    static char cmd0[LOGIN];

    if(cmd0[cmdmode])
        cmd = cmd0[cmdmode];

    setutmpmode(cmdmode);

    showtitle(cmdtitle, BBSName);

    total = show_menu(cmdtable);

    outmsg(mystatus);
    lastcmdptr = pos = 0;

    do {
        i = -1;
        switch(cmd) {
        case Ctrl('C'):
            cal();
            i = lastcmdptr;
            refscreen = YEA;
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
            if(mail_man() == FULLUPDATE)
                refscreen = YEA;
            i = lastcmdptr;
            break;
        case KEY_DOWN:
            i = lastcmdptr;
        case KEY_HOME:
        case KEY_PGUP:
            do {
                if(++i > total)
                    i = 0;
            } while(!HAS_PERM(cmdtable[i].level));
            break;
        case KEY_END:
        case KEY_PGDN:
            i = total;
            break;
        case KEY_UP:
            i = lastcmdptr;
            do {
                if(--i < 0)
                    i = total;
            } while(!HAS_PERM(cmdtable[i].level));
            break;
        case KEY_LEFT:
        case 'e':
        case 'E':
            if(cmdmode == MMENU)
                cmd = 'G';
            else if((cmdmode == MAIL) && chkmailbox())
                cmd = 'R';
            else
                return;
        default:
            if((cmd == 's'  || cmd == 'r') &&
               (currstat == MMENU || currstat == TMENU || currstat == XMENU)) {
                if(cmd == 's')
                    ReadSelect();
                else
                    Read();
                refscreen = YEA;
                i = lastcmdptr;
                break;
            }

            if(cmd == '\n' || cmd == '\r' || cmd == KEY_RIGHT) {
                move(b_lines, 0);
                clrtoeol();

                currstat = XMODE;

                if((err = (*cmdtable[lastcmdptr].cmdfunc) ()) == QUIT)
                    return;
                currutmp->mode = currstat = cmdmode;

                if(err == XEASY) {
                    refresh();
                    safe_sleep(1);
                } else if(err != XEASY + 1 || err == FULLUPDATE)
                    refscreen = YEA;

                if(err != -1)
                    cmd = cmdtable[lastcmdptr].desc[0];
                else
                    cmd = cmdtable[lastcmdptr].desc[1];
                cmd0[cmdmode] = cmdtable[lastcmdptr].desc[0];
            }

            if(cmd >= 'a' && cmd <= 'z')
                cmd &= ~0x20;
            while(++i <= total)
                if(cmdtable[i].desc[1] == cmd)
                    break;
        }

        if(i > total || !HAS_PERM(cmdtable[i].level))
            continue;

        if(refscreen) {
            showtitle(cmdtitle, BBSName);

            show_menu(cmdtable);

            outmsg(mystatus);
            refscreen = NA;
        }
        cursor_clear(menu_row + pos, menu_column);
        n = pos = -1;
        while(++n <= (lastcmdptr = i))
            if(HAS_PERM(cmdtable[n].level))
                pos++;

        cursor_show(menu_row + pos, menu_column);
    } while(((cmd = egetch()) != EOF) || refscreen);

    abort_bbs(0);
}
/* INDENT OFF */

/* administrator's maintain menu */
static commands_t adminlist[] = {
    {m_user, PERM_ACCOUNTS,           "UUser          �ϥΪ̸��"},
    {search_user_bypwd, PERM_SYSOP,   "SSearch User   �S��j�M�ϥΪ�"},
    {search_user_bybakpwd,PERM_SYSOP, "OOld User data �d�\\�ƥ��ϥΪ̸��"},
    {m_board, PERM_SYSOP,             "BBoard         �]�w�ݪO"},
    {m_register, PERM_SYSOP,          "RRegister      �f�ֵ��U���"},
    {cat_register, PERM_SYSOP,        "CCatregister   �L�k�f�֮ɥΪ�"},
    {x_file, PERM_SYSOP|PERM_VIEWSYSOP,     "XXfile         �s��t���ɮ�"},
    {give_money, PERM_SYSOP|PERM_VIEWSYSOP, "GGivemoney     ���]��"},
#ifdef  HAVE_MAILCLEAN
    {m_mclean, PERM_SYSOP,            "MMail Clean    �M�z�ϥΪ̭ӤH�H�c"},
#endif
#ifdef  HAVE_REPORT
    {m_trace, PERM_SYSOP,             "TTrace         �]�w�O�_�O��������T"},
#endif
    {NULL, 0, NULL}
};

/* mail menu */
static commands_t maillist[] = {
    {m_new, PERM_READMAIL,      "RNew           �\\Ū�s�i�l��"},
    {m_read, PERM_READMAIL,     "RRead          �h�\\��Ū�H���"},
    {m_send, PERM_BASIC,        "RSend          �����H�H"},
    {main_bbcall, PERM_LOGINOK, "BBBcall        \033[1;31m�q�ܯ���\033[m"},
    {x_love, PERM_LOGINOK,      "PPaper         \033[1;32m���Ѳ��;�\033[m "},
    {mail_list, PERM_BASIC,     "RMail List     �s�ձH�H"},
    {setforward, PERM_LOGINOK,"FForward       \033[32m�]�w�H�c�۰���H\033[m"},
    {m_sysop, 0,                "YYes, sir!     �ԴA����"},
    {m_internet, PERM_INTERNET, "RInternet      �H�H�� Internet"},
    {mail_mbox, PERM_INTERNET,  "RZip UserHome  ��Ҧ��p�H��ƥ��]�^�h"},
    {built_mail_index, PERM_LOGINOK, "SSavemail      ��H��Ϧ^��"},
    {mail_all, PERM_SYSOP,      "RAll           �H�H���Ҧ��ϥΪ�"},
    {NULL, 0, NULL}
};

/* Talk menu */
static commands_t talklist[] = {
    {t_users, 0,            "UUsers         ������Ѥ�U"},
    {t_pager, PERM_BASIC,   "PPager         �����I�s��"},
    {t_idle, 0,             "IIdle          �o�b"},
    {t_query, 0,            "QQuery         �d�ߺ���"},
    {t_qchicken, 0,         "WWatch Pet     �d���d��"},
    {t_talk, PERM_PAGE,     "TTalk          ��H���"},
    {t_chat, PERM_CHAT,     "CChat          ��a���{����h"},
#ifdef HAVE_MUD
    {x_mud, 0,              "VVrChat        \033[1;32m������~��Ѽs��\033[m"},
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

static commands_t namelist[] = {
    {t_override, PERM_LOGINOK,"OOverRide      �n�ͦW��"},
    {t_reject, PERM_LOGINOK,  "BBlack         �a�H�W��"},
    {t_aloha,PERM_LOGINOK,    "AALOHA         �W���q���W��"},
#ifdef POSTNOTIFY
    {t_post,PERM_LOGINOK,     "NNewPost       �s�峹�q���W��"},
#endif
    {t_special,PERM_LOGINOK,  "SSpecial       ��L�S�O�W��"},
    {NULL, 0, NULL}
};

/* User menu */
static commands_t userlist[] = {
    {u_info, PERM_LOGINOK,          "IInfo          �]�w�ӤH��ƻP�K�X"},
    {calendar, PERM_LOGINOK,          "CCalendar      �ӤH��ƾ�"},
    {u_editcalendar, PERM_LOGINOK,    "CCalendarEdit  �s��ӤH��ƾ�"},
    {u_loginview, PERM_LOGINOK,     "LLogin View    ��ܶi���e��"},
    {u_ansi, 0, "AANSI          ���� ANSI \033[36m�m\033[35m��\033[37m/"
     "\033[30;47m��\033[1;37m��\033[m�ҥ�"},
    {u_movie, 0,                    "MMovie         �����ʵe�ҥ�"},
#ifdef  HAVE_SUICIDE
    {u_kill, PERM_BASIC,            "IKill          �۱��I�I"},
#endif
    {u_editplan, PERM_LOGINOK,      "QQueryEdit     �s��W����"},
    {u_editsig, PERM_LOGINOK,       "SSignature     �s��ñ�W��"},
#if HAVE_FREECLOAK
    {u_cloak, PERM_LOGINOK,           "CCloak         �����N"},
#else
    {u_cloak, PERM_CLOAK,           "CCloak         �����N"},
#endif
    {u_register, PERM_BASIC,        "RRegister      ��g�m���U�ӽг�n"},
    {u_list, PERM_SYSOP,            "UUsers         �C�X���U�W��"},
    {NULL, 0, NULL}
};

/* XYZ tool menu */
static commands_t xyzlist[] = {
#ifdef  HAVE_LICENSE
    {x_gpl, 0,       "LLicense       GNU �ϥΰ���"},
#endif
#ifdef HAVE_INFO
    {x_program, 0,   "PProgram       ���{���������P���v�ŧi"},
#endif
    {x_boardman,0,   "MMan Boards    �m�ݪ���ذϱƦ�]�n"},
//  {x_boards,0,     "HHot Boards    �m�ݪ��H��Ʀ�]�n"},
    {x_history, 0,   "HHistory       �m�ڭ̪������n"},
    {x_note, 0,      "NNote          �m�Ĳ��W���y�����n"},
    {x_login,0,      "SSystem        �m�t�έ��n���i�n"},
    {x_week, 0,      "WWeek          �m���g���Q�j�������D�n"},
    {x_issue, 0,     "IIssue         �m����Q�j�������D�n"},
    {x_today, 0,     "TToday         �m����W�u�H���έp�n"},
    {x_yesterday, 0, "YYesterday     �m�Q��W�u�H���έp�n"},
    {x_user100 ,0,   "UUsers         �m�ϥΪ̦ʤj�Ʀ�]�n"},
    {x_birth, 0,     "BBirthday      �m����جP�j�[�n"},
    {p_sysinfo, 0,   "XXload         �m�d�ݨt�έt���n"},
    {NULL, 0, NULL}
};

/* Ptt money menu */
static commands_t moneylist[] = {
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

static commands_t jceelist[] = {
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

static int forsearch();
static int playground();

/* Ptt Play menu */
static commands_t playlist[] = {
#if HAVE_JCEE
    {m_jcee, PERM_LOGINOK,   "JJCEE        �i �j���p�Ҭd�]�t�� �j"},
#endif
    {note, PERM_LOGINOK,     "NNote        �i ���y���� �j"},
    {x_weather,0 ,           "WWeather     �i ��H�w�� �j"},
    {x_stock,0 ,             "SStock       �i �ѥ��污 �j"},
#ifdef HAVE_BIG2
    {x_big2, 0,              "BBig2        �i �����j�ѤG �j"},
#endif
#ifdef HAVE_MJ
    {x_mj, PERM_LOGINOK,     "QQkmj        �i �������±N �j"},
#endif
#ifdef  HAVE_BRIDGE
    {x_bridge, PERM_LOGINOK, "OOkBridge    �i ���P�v�� �j"},
#endif
#ifdef HAVE_GOPHER
    {x_gopher, PERM_LOGINOK, "GGopher      �i �a����Ʈw �j"},
#endif
#ifdef HAVE_TIN
    {x_tin, PERM_LOGINOK,    "NNEWS        �i ���ڷs�D �j"},
#endif
#ifdef BBSDOORS
    {x_bbsnet, PERM_LOGINOK, "BBBSNet      �i ��L BBS�� �j"},
#endif
#ifdef HAVE_WWW
    {x_www, PERM_LOGINOK,    "WWWW Browser �i �L�L�L �j"},
#endif
    {forsearch,PERM_LOGINOK, "SSearchEngine�i\033[1;35m ��tt�j�M�� \033[m�j"},
    {topsong,PERM_LOGINOK,   "TTop Songs   �i\033[1;32m�ڮ��I�q�Ʀ�]\033[m�j"},
    {p_money,PERM_LOGINOK,   "PPay         �i\033[1;31m ��tt�q�c�� \033[m�j"},
    {chicken_main,PERM_LOGINOK, "CChicken     "
     "�i\033[1;34m ��tt�i���� \033[m�j"},
    {playground,PERM_LOGINOK, "AAmusement   �i\033[1;33m ��tt�C�ֳ� \033[m�j"},
    {NULL, 0, NULL}
};

static commands_t plist[] = {

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

static commands_t slist[] = {
    {x_dict,0,                   "11Dictionary  "
     "�i\033[1;33m ����j�r�� \033[m�j"},
    {main_railway, PERM_LOGINOK,  "33Railway     "
     "�i\033[1;32m ������d�� \033[m�j"},
    {NULL, 0, NULL}
};

static int forsearch() {
    domenu(SREG, "��tt�j�M��", '1', slist);
    return 0;
}

/* main menu */

static int admin() {
    domenu(ADMIN, "�t�κ��@", 'X', adminlist);
    return 0;
}

static int Mail() {
    domenu(MAIL, "�q�l�l��", 'R', maillist);
    return 0;
}

static int Talk() {
    domenu(TMENU, "��ѻ���", 'U', talklist);
    return 0;
}

static int User() {
    domenu(UMENU, "�ӤH�]�w", 'A', userlist);
    return 0;
}

static int Xyz() {
    domenu(XMENU, "�u��{��", 'M', xyzlist);
    return 0;
}

static int Play_Play() {
    domenu(PMENU, "�����C�ֳ�", 'A', playlist);
    return 0;
}

static int Name_Menu() {
    domenu(NMENU, "�զ⮣��", 'O', namelist);
    return 0;
}

commands_t cmdlist[] = {
    {admin,PERM_SYSOP|PERM_VIEWSYSOP, "00Admin       �i �t�κ��@�� �j"},
    {Announce, 0,                     "AAnnounce     �i ��ؤ��G�� �j"},
    {Boards, 0,                       "FFavorite     �i �� �� �̷R �j"},
    {root_board, 0,                   "CClass        �i ���հQ�װ� �j"},
    {Mail, PERM_BASIC,                "MMail         �i �p�H�H��� �j"},
    {Talk, 0,                         "TTalk         �i �𶢲�Ѱ� �j"},
    {User, 0,                         "UUser         �i �ӤH�]�w�� �j"},
    {Xyz, 0,                          "XXyz          �i �t�Τu��� �j"},
    {Play_Play,0,                     "PPlay         �i �C�ֳ�/�j�Ǭd�]�j"},
    {Name_Menu,PERM_LOGINOK,          "NNamelist     �i �s�S�O�W�� �j"},
    {Goodbye, 0,                      "GGoodbye       ���}�A�A���K�K"},
    {NULL, 0, NULL}
};
