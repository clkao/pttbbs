/* $Id: var.c,v 1.3 2002/05/14 15:08:48 ptt Exp $ */
#include <stdio.h>
#include <sys/types.h>
#include "config.h"
#include "pttstruct.h"
#include "common.h"

char *str_permid[] = {
    "���v�O",                   /* PERM_BASIC */
    "�i�J��ѫ�",                 /* PERM_CHAT */
    "��H���",                   /* PERM_PAGE */
    "�o��峹",                   /* PERM_POST */
    "���U�{�ǻ{��",               /* PERM_LOGINOK */
    "�H��L�W��",                 /* PERM_MAILLIMIT */
    "�����N",                     /* PERM_CLOAK */
    "�ݨ��Ԫ�",                   /* PERM_SEECLOAK */
    "�ä[�O�d�b��",               /* PERM_XEMPT */
    "���������N",                 /* PERM_DENYPOST */
    "�O�D",                       /* PERM_BM */
    "�b���`��",                   /* PERM_ACCOUNTS */
    "��ѫ��`��",                 /* PERM_CHATCLOAK */
    "�ݪO�`��",                   /* PERM_BOARD */
    "����",                       /* PERM_SYSOP */
    "BBSADM",                     /* PERM_POSTMARK */
    "���C�J�Ʀ�]",               /* PERM_NOTOP */
    "�H�k�q�r��",                 /* PERM_VIOLATELAW */
    "���������~���H",             /* PERM_ */
    "�S�Q��",                     /* PERM_ */
    "��ı����",                   /* PERM_VIEWSYSOP */
    "�[��ϥΪ̦���",             /* PERM_LOGUSER */
    "��ذ��`��z�v",             /* PERM_Announce */
    "������",                     /* PERM_RELATION */
    "�S�Ȳ�",                     /* PERM_SMG */
    "�{����",                     /* PERM_PRG */
    "���ʲ�",                     /* PERM_ACTION */
    "���u��",                     /* PERM_PAINT */
    "�ߪk��",                     /* PERM_LAW */
    "�p�ժ�",                     /* PERM_SYSSUBOP */
    "�@�ťD��",                   /* PERM_LSYSOP */
    "�ޢ���"                      /* PERM_PTT */  
};

char *str_permboard[] = {
    "���i Zap",                   /* BRD_NOZAP */
    "���C�J�έp",                 /* BRD_NOCOUNT */
    "����H",                     /* BRD_NOTRAN */
    "�s�ժ�",                     /* BRD_GROUP */
    "���ê�",                     /* BRD_HIDE */
    "����(���ݳ]�w)",        /* BRD_POSTMASK */
    "�ΦW��",                     /* BRD_ANONYMOUS */
    "�w�]�ΦW��",                 /* BRD_DEFAULTANONYMOUS */
    "�H�k��i���ݪ�",             /* BRD_BAD */
    "�s�p�M�άݪ�",               /* BRD_VOTEBOARD */
    "�S�Q��",
    "�S�Q��", 
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��", 
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��", 
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��", 
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��",
    "�S�Q��", 
};

int usernum;
pid_t currpid;                  /* current process ID */
unsigned int currstat;
int currmode = 0;
int curredit = 0;
int showansi = 1;
time_t login_start_time;
userec_t cuser;                   /* current user structure */
userec_t xuser;                   /* lookup user structure */
char quote_file[80] = "\0";
char quote_user[80] = "\0";
time_t paste_time;
char paste_title[STRLEN];
char paste_path[256];
int paste_level;
char currtitle[TTLEN + 1] = "\0";
char vetitle[TTLEN + 1] = "\0";
char currowner[IDLEN + 2] = "\0";
char currauthor[IDLEN + 2] = "\0";
char currfile[FNLEN];           /* current file name @ bbs.c mail.c */
unsigned char currfmode;               /* current file mode */
char currboard[IDLEN + 2];
int currbid;
unsigned int currbrdattr;
char currBM[IDLEN * 3 + 10];
char reset_color[4] = "\033[m";
char margs[64] = "\0";           /*  main argv list*/
crosspost_t postrecord;           /* anti cross post */

/* global string variables */
/* filename */

char *fn_passwd = FN_PASSWD;
char *fn_board = FN_BOARD;
char *fn_note_ans = FN_NOTE_ANS;
char *fn_register = "register.new";
char *fn_plans = "plans";
char *fn_writelog = "writelog";
char *fn_talklog = "talklog";
char *fn_overrides = FN_OVERRIDES;
char *fn_reject = FN_REJECT;
char *fn_canvote = FN_CANVOTE;
char *fn_notes = "notes";
char *fn_water = FN_WATER;
char *fn_visable = FN_VISABLE;
char *fn_mandex = "/.Names";
char *fn_proverb = "proverb";

/* are descript in userec.loginview */

char *loginview_file[NUMVIEWFILE][2] = {
    {FN_NOTE_ANS       ,"�Ĳ��W���y���O"},
    {FN_TOPSONG        ,"�I�q�Ʀ�]"    },
    {"etc/topusr"      ,"�Q�j�Ʀ�]"    },
    {"etc/topusr100"   ,"�ʤj�Ʀ�]"   },
    {"etc/birth.today" ,"����جP"     },
    {"etc/weather.tmp" ,"�Ѯ�ֳ�"     },
    {"etc/stock.tmp"   ,"�ѥ��ֳ�"     },
    {"etc/day"         ,"����Q�j���D"  },
    {"etc/week"        ,"�@�g���Q�j���D"},
    {"etc/today"       ,"���ѤW���H��"  },
    {"etc/yesterday"   ,"�Q��W���H��"  },
    {"etc/history"     ,"���v�W������"  },
    {"etc/topboardman" ,"��ذϱƦ�]"  },
    {"etc/topboard.tmp","�ݪO�H��Ʀ�]"}
};

/* message */
char *msg_seperator = MSG_SEPERATOR;
char *msg_mailer = MSG_MAILER;
char *msg_shortulist = MSG_SHORTULIST;

char *msg_cancel = MSG_CANCEL;
char *msg_usr_left = MSG_USR_LEFT;
char *msg_nobody = MSG_NOBODY;

char *msg_sure_ny = MSG_SURE_NY;
char *msg_sure_yn = MSG_SURE_YN;

char *msg_bid = MSG_BID;
char *msg_uid = MSG_UID;

char *msg_del_ok = MSG_DEL_OK;
char *msg_del_ny = MSG_DEL_NY;

char *msg_fwd_ok = MSG_FWD_OK;
char *msg_fwd_err1 = MSG_FWD_ERR1;
char *msg_fwd_err2 = MSG_FWD_ERR2;

char *err_board_update = ERR_BOARD_UPDATE;
char *err_bid = ERR_BID;
char *err_uid = ERR_UID;
char *err_filename = ERR_FILENAME;

char *str_mail_address = "." BBSUSER "@" MYHOSTNAME;
char *str_new = "new";
char *str_reply = "Re: ";
char *str_space = " \t\n\r";
char *str_sysop = "SYSOP";
char *str_author1 = STR_AUTHOR1;
char *str_author2 = STR_AUTHOR2;
char *str_post1 = STR_POST1;
char *str_post2 = STR_POST2;
char *BBSName = BBSNAME;

/* #define MAX_MODES 78 */
/* MAX_MODES is defined in common.h */

char *ModeTypeTable[MAX_MODES] = {
    "�o�b",                       /* IDLE */
    "�D���",                     /* MMENU */
    "�t�κ��@",                   /* ADMIN */
    "�l����",                   /* MAIL */
    "��Ϳ��",                   /* TMENU */
    "�ϥΪ̿��",                 /* UMENU */
    "XYZ ���",                   /* XMENU */
    "�����ݪO",                   /* CLASS */
    "Play���",                   /* PMENU */
    "�s�S�O�W��",                 /* NMENU */
    "��tt�q�c��",                 /* PSALE */
    "�o��峹",                   /* POSTING */
    "�ݪO�C��",                   /* READBRD */
    "�\\Ū�峹",                  /* READING */
    "�s�峹�C��",                 /* READNEW */
    "��ܬݪO",                   /* SELECT */
    "Ū�H",                       /* RMAIL */
    "�g�H",                       /* SMAIL */
    "��ѫ�",                     /* CHATING */
    "��L",                       /* XMODE */
    "�M��n��",                   /* FRIEND */
    "�W�u�ϥΪ�",                 /* LAUSERS */
    "�ϥΪ̦W��",                 /* LUSERS */
    "�l�ܯ���",                   /* MONITOR */
    "�I�s",                       /* PAGE */
    "�d��",                       /* TQUERY */
    "���",                       /* TALK  */
    "�s�W����",                   /* EDITPLAN */
    "�sñ�W��",                   /* EDITSIG */
    "�벼��",                     /* VOTING */
    "�]�w���",                   /* XINFO */
    "�H������",                   /* MSYSOP */
    "�L�L�L",                     /* WWW */
    "���j�ѤG",                   /* BIG2 */
    "�^��",                       /* REPLY */
    "�Q���y����",                 /* HIT */
    "���y�ǳƤ�",                 /* DBACK */
    "���O��",                     /* NOTE */
    "�s��峹",                   /* EDITING */
    "�o�t�γq�i",                 /* MAILALL */
    "�N���",                     /* MJ */
    "�q���ܤ�",                   /* P_FRIEND */
    "�W���~��",                   /* LOGIN */
    "�d�r��",                     /* DICT */
    "�����P",                     /* BRIDGE */
    "���ɮ�",                     /* ARCHIE */
    "���a��",                     /* GOPHER */
    "��News",                     /* NEWS */
    "���Ѳ��;�",                 /* LOVE */
    "�s�y���U��",  		  /* EDITEXP */
    "�ӽ�IP��}",		  /* IPREG */
    "���޿줽��", 		  /* NetAdm */
    "������~�{",  		  /* DRINK */
    "�p���",                     /* CAL */
    "�s�y�y�k��",		  /* PROVERB */
    "���G��",                     /* ANNOUNCE */
    "��y����",                   /* EDNOTE */
    "�^�~½Ķ��",   		  /* CDICT */
    "�˵��ۤv���~",		  /* LOBJ */
    "�I�q",                       /* OSONG */
    "���b���p��",                 /* CHICKEN */
    "���m��",                     /* TICKET */
    "�q�Ʀr",                     /* GUESSNUM */ 
    "�C�ֳ�",			  /* AMUSE */
    "�¥մ�",			  /* OTHELLO */
    "����l",                     /* DICE*/
    "�o�����",                   /* VICE */  
    "�G�G��ing",                  /* BBCALL */
    "ú�@��",                     /* CROSSPOST */
    "���l��",                     /* M_FIVE */
    "21�Iing",                    /* JACK_CARD */
    "10�I�bing",                  /* TENHALF */ 
    "�W�ŤE�Q�E",                 /* CARD_99 */
    "�����d��",                   /* RAIL_WAY */
    "�j�M���",                    /* SREG */
    "�U�H��",                      /* CHC */
    "�U�t�X",			   /* DARK */
    "NBA�j�q��"                    /* TMPJACK */
    "��tt�d�]�t��",                 /* JCEE */
    "���s�峹"                    /* REEDIT */
};
