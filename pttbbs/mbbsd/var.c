/* $Id$ */
#define INCLUDE_VAR_H
#include "bbs.h"

char           * const str_permid[] = {
    "基本權力",			/* PERM_BASIC */
    "進入聊天室",		/* PERM_CHAT */
    "找人聊天",			/* PERM_PAGE */
    "發表文章",			/* PERM_POST */
    "註冊程序認證",		/* PERM_LOGINOK */
    "信件無上限",		/* PERM_MAILLIMIT */
    "隱身術",			/* PERM_CLOAK */
    "看見忍者",			/* PERM_SEECLOAK */
    "永久保留帳號",		/* PERM_XEMPT */
    "站長隱身術",		/* PERM_DENYPOST */
    "板主",			/* PERM_BM */
    "帳號總管",			/* PERM_ACCOUNTS */
    "聊天室總管",		/* PERM_CHATCLOAK */
    "看板總管",			/* PERM_BOARD */
    "站長",			/* PERM_SYSOP */
    "BBSADM",			/* PERM_POSTMARK */
    "不列入排行榜",		/* PERM_NOTOP */
    "違法通緝中",		/* PERM_VIOLATELAW */
#ifdef PLAY_ANGEL
    "可擔任小天使",		/* PERM_ANGEL */
#else
    "未使用",
#endif
    "不允許\認證碼註冊",	/* PERM_NOREGCODE */
    "視覺站長",			/* PERM_VIEWSYSOP */
    "觀察使用者行蹤",		/* PERM_LOGUSER */
    "精華區總整理權",		/* PERM_Announce */
    "公關組",			/* PERM_RELATION */
    "特務組",			/* PERM_SMG */
    "程式組",			/* PERM_PRG */
    "活動組",			/* PERM_ACTION */
    "美工組",			/* PERM_PAINT */
    "立法組",			/* PERM_LAW */
    "小組長",			/* PERM_SYSSUBOP */
    "一級主管",			/* PERM_LSYSOP */
    "Ｐｔｔ"			/* PERM_PTT */
};

char           * const str_permboard[] = {
    "不可 Zap",			/* BRD_NOZAP */
    "不列入統計",		/* BRD_NOCOUNT */
    "不轉信",			/* BRD_NOTRAN */
    "群組板",			/* BRD_GROUPBOARD */
    "隱藏板",			/* BRD_HIDE */
    "限制(不需設定)",		/* BRD_POSTMASK */
    "匿名板",			/* BRD_ANONYMOUS */
    "預設匿名板",		/* BRD_DEFAULTANONYMOUS */
    "違法改進中看板",		/* BRD_BAD */
    "連署專用看板",		/* BRD_VOTEBOARD */
    "已警告要廢除",		/* BRD_WARNEL */
    "熱門看板群組",		/* BRD_TOP */
    "不可推薦",                 /* BRD_NORECOMMEND */
    "布落格",			/* BRD_BLOG */
    "板主設定列入記錄",		/* BRD_BMCOUNT */
    "沒想到",
    "沒想到",
    "沒想到",
    "沒想到",
    "沒想到",
    "沒想到",
    "沒想到",
    "沒想到",
    "沒想到",
    "沒想到",
    "沒想到",
    "沒想到",
    "沒想到",
    "沒想到",
    "沒想到",
    "沒想到",
    "沒想到",
};

int             usernum;
int             currmode = 0;
int             curredit = 0;
int             paste_level;
int             currbid;
char            quote_file[80] = "\0";
char            quote_user[80] = "\0";
char            paste_title[STRLEN];
char            paste_path[256];
char            currtitle[TTLEN + 1] = "\0";
char            vetitle[TTLEN + 1] = "\0";
char            currauthor[IDLEN + 2] = "\0";
char           *currboard = "\0";
char            currBM[IDLEN * 3 + 10];
const char            reset_color[4] = "\033[m";
char            margs[64] = "\0";	/* main argv list */
pid_t           currpid;	/* current process ID */
time_t          login_start_time;
time_t          start_time;
time_t          paste_time;
userec_t        cuser;		/* current user structure */
userec_t        xuser;		/* lookup user structure */
crosspost_t     postrecord;	/* anti cross post */
unsigned int    currbrdattr;
unsigned int    currstat;
unsigned char   currfmode;	/* current file mode */

/* global string variables */
/* filename */

char           *fn_passwd = FN_PASSWD;
char           *fn_board = FN_BOARD;
char           *fn_note_ans = FN_NOTE_ANS;
char           *fn_register = "register.new";
char           *fn_plans = "plans";
char           *fn_writelog = "writelog";
char           *fn_talklog = "talklog";
char           *fn_overrides = FN_OVERRIDES;
char           *fn_reject = FN_REJECT;
char           *fn_canvote = FN_CANVOTE;
char           *fn_notes = "notes";
char           *fn_water = FN_WATER;
char           *fn_visable = FN_VISABLE;
char           *fn_mandex = "/.Names";
char           *fn_proverb = "proverb";

/* are descript in userec.loginview */

char           * const loginview_file[NUMVIEWFILE][2] = {
    {FN_NOTE_ANS, "酸甜苦辣流言板"},
    {FN_TOPSONG, "點歌排行榜"},
    {"etc/topusr", "十大排行榜"},
    {"etc/topusr100", "百大排行榜"},
    {"etc/birth.today", "今日壽星"},
    {"etc/weather.tmp", "天氣快報"},
    {"etc/stock.tmp", "股市快報"},
    {"etc/day", "今日十大話題"},
    {"etc/week", "一週五十大話題"},
    {"etc/today", "今天上站人次"},
    {"etc/yesterday", "昨日上站人次"},
    {"etc/history", "歷史上的今天"},
    {"etc/topboardman", "精華區排行榜"},
    {"etc/topboard.tmp", "看板人氣排行榜"}
};

/* message */
char           * const msg_seperator = MSG_SEPERATOR;
char           * const msg_mailer = MSG_MAILER;
char           * const msg_shortulist = MSG_SHORTULIST;

char           * const msg_cancel = MSG_CANCEL;
char           * const msg_usr_left = MSG_USR_LEFT;
char           * const msg_nobody = MSG_NOBODY;

char           * const msg_sure_ny = MSG_SURE_NY;
char           * const msg_sure_yn = MSG_SURE_YN;

char           * const msg_bid = MSG_BID;
char           * const msg_uid = MSG_UID;

char           * const msg_del_ok = MSG_DEL_OK;
char           * const msg_del_ny = MSG_DEL_NY;

char           * const msg_fwd_ok = MSG_FWD_OK;
char           * const msg_fwd_err1 = MSG_FWD_ERR1;
char           * const msg_fwd_err2 = MSG_FWD_ERR2;

char           * const err_board_update = ERR_BOARD_UPDATE;
char           * const err_bid = ERR_BID;
char           * const err_uid = ERR_UID;
char           * const err_filename = ERR_FILENAME;

char           * const str_mail_address = "." BBSUSER "@" MYHOSTNAME;
char           * const str_new = "new";
char           * const str_reply = "Re: ";
char           * const str_space = " \t\n\r";
char           * const str_sysop = "SYSOP";
char           * const str_author1 = STR_AUTHOR1;
char           * const str_author2 = STR_AUTHOR2;
char           * const str_post1 = STR_POST1;
char           * const str_post2 = STR_POST2;
char           * const BBSName = BBSNAME;

/* #define MAX_MODES 78 */
/* MAX_MODES is defined in common.h */

char           * const ModeTypeTable[MAX_MODES] = {
    "發呆",			/* IDLE */
    "主選單",			/* MMENU */
    "系統維護",			/* ADMIN */
    "郵件選單",			/* MAIL */
    "交談選單",			/* TMENU */
    "使用者選單",		/* UMENU */
    "XYZ 選單",			/* XMENU */
    "分類看板",			/* CLASS */
    "Play選單",			/* PMENU */
    "編特別名單",		/* NMENU */
    "Ｐtt量販店",		/* PSALE */
    "發表文章",			/* POSTING */
    "看板列表",			/* READBRD */
    "閱\讀文章",		/* READING */
    "新文章列表",		/* READNEW */
    "選擇看板",			/* SELECT */
    "讀信",			/* RMAIL */
    "寫信",			/* SMAIL */
    "聊天室",			/* CHATING */
    "其他",			/* XMODE */
    "尋找好友",			/* FRIEND */
    "上線使用者",		/* LAUSERS */
    "使用者名單",		/* LUSERS */
    "追蹤站友",			/* MONITOR */
    "呼叫",			/* PAGE */
    "查詢",			/* TQUERY */
    "交談",			/* TALK  */
    "編名片檔",			/* EDITPLAN */
    "編簽名檔",			/* EDITSIG */
    "投票中",			/* VOTING */
    "設定資料",			/* XINFO */
    "寄給站長",			/* MSYSOP */
    "汪汪汪",			/* WWW */
    "打大老二",			/* BIG2 */
    "回應",			/* REPLY */
    "被水球打中",		/* HIT */
    "水球準備中",		/* DBACK */
    "筆記本",			/* NOTE */
    "編輯文章",			/* EDITING */
    "發系統通告",		/* MAILALL */
    "摸兩圈",			/* MJ */
    "電腦擇友",			/* P_FRIEND */
    "上站途中",			/* LOGIN */
    "查字典",			/* DICT */
    "打橋牌",			/* BRIDGE */
    "找檔案",			/* ARCHIE */
    "打地鼠",			/* GOPHER */
    "看News",			/* NEWS */
    "情書產生器",		/* LOVE */
    "編輯輔助器",		/* EDITEXP */
    "申請IP位址",		/* IPREG */
    "網管辦公中",		/* NetAdm */
    "虛擬實業坊",		/* DRINK */
    "計算機",			/* CAL */
    "編輯座右銘",		/* PROVERB */
    "公佈欄",			/* ANNOUNCE */
    "刻流言板",			/* EDNOTE */
    "英漢翻譯機",		/* CDICT */
    "檢視自己物品",		/* LOBJ */
    "點歌",			/* OSONG */
    "正在玩小雞",		/* CHICKEN */
    "玩彩券",			/* TICKET */
    "猜數字",			/* GUESSNUM */
    "遊樂場",			/* AMUSE */
    "黑白棋",			/* OTHELLO */
    "玩骰子",			/* DICE */
    "發票對獎",			/* VICE */
    "逼逼摳ing",		/* BBCALL */
    "繳罰單",			/* CROSSPOST */
    "五子棋",			/* M_FIVE */
    "21點ing",			/* JACK_CARD */
    "10點半ing",		/* TENHALF */
    "超級九十九",		/* CARD_99 */
    "火車查詢",			/* RAIL_WAY */
    "搜尋選單",			/* SREG */
    "下象棋",			/* CHC */
    "下暗琪",			/* DARK */
    "NBA大猜測"			/* TMPJACK */
    "Ｐtt查榜系統",		/* JCEE */
    "重編文章"			/* REEDIT */
    "部落格",                   /* BLOGGING */
    "", /* for future usage */
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    ""
};

/* term.c */
int             b_lines = 23; // bottom line of screen
int             t_lines = 24; // term lines
int             p_lines = 20;
int             t_columns = 80;
char           * const strtstandout = "\33[7m";
int             strtstandoutlen = 4;
char           * const endstandout = "\33[m";
int             endstandoutlen = 3;
char           * const clearbuf = "\33[H\33[J";
int             clearbuflen = 6;
char           *cleolbuf = "\33[K";
int             cleolbuflen = 3;
char           *scrollrev = "\33M";
int             scrollrevlen = 2;
int             automargins = 1;

/* io.c */
time_t          now;
int             KEY_ESC_arg;
int             watermode = -1;
int             wmofo = NOTREPLYING;
/*
 * WATERMODE(WATER_ORIG) | WATERMODE(WATER_NEW):
 * ????????????????????
 * Ptt 水球回顧   (FIXME: guessed by scw)
 * watermode = -1 沒在回水球
 *           = 0   在回上一顆水球  (Ctrl-R)
 *           > 0   在回前 n 顆水球 (Ctrl-R Ctrl-R)
 * 
 * WATERMODE(WATER_OFO)  by in2
 * wmofo     = NOTREPLYING     沒在回水球
 *           = REPLYING        正在回水球
 *           = RECVINREPLYING  回水球間又接到水球
 *
 * wmofo     >=0  時收到水球將只顯示, 不會到water[]裡,
 *                待回完水球的時候一次寫入.
 */


/* cache.c */
int             numboards = -1;
int            *GLOBALVAR;
SHM_t          *SHM;
boardheader_t  *bcache;
userinfo_t     *currutmp;

/* board.c */
int             class_bid = 0;

/* brc.c */
int             brc_num;
time_t          brc_list[BRC_MAXNUM];

/* read.c */
int             TagNum;			/* tag's number */
TagItem         TagList[MAXTAGS];	/* ascending list */
int		TagBoard = -1;		/* TagBoard = 0 : user's mailbox
					   TagBoard > 0 : bid where last taged*/
char            currdirect[64];

/* edit.c */
char            save_title[STRLEN];

/* bbs.c */
time_t          board_visit_time;
char            real_name[IDLEN + 1];
int             local_article;

/* mbbsd.c */
char            fromhost[STRLEN] = "\0";
char            water_usies = 0;
FILE           *fp_writelog = NULL;
water_t         water[6], *swater[6], *water_which = &water[0];

/* announce.c */
char            trans_buffer[256];

/* chc_play.c */
rc_t            chc_from, chc_to, chc_select, chc_cursor;
int             chc_lefttime;
int             chc_my, chc_turn, chc_selected, chc_firststep;
char            chc_warnmsg[64], *chc_mateid;

/* screen.c */
#define scr_lns         t_lines
#define scr_cols        ANSILINELEN
screenline_t   *big_picture = NULL;
char            roll;

/* gomo.c */
char            ku[BRDSIZ][BRDSIZ];
unsigned char  *pat, *adv;
unsigned char  * const pat_gomoku /* [1954] */ =
 /* 0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 16 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x44\x55\xcc\x00\x00\x00\x00"
 /* 32 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x33\x00\x44\x00\x33\x00\x00\x00"
 /* 48 */ "\x00\x22\x00\x55\x00\x22\x00\x00\x00\x44\x33\x66\x55\xcc\x33\x66"
 /* 64 */ "\x55\xcc\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00"
 /* 80 */ "\x55\x00\x55\x00\x05\x00\x55\x02\x46\x00\xaa\x00\x00\x55\x00\x55"
 /* 96 */ "\x00\x05\x00\x55\x00\x05\x00\x55\x00\x00\x44\xcc\x44\xcc\x05\xbb"
 /* 112 */ "\x44\xcc\x05\xbb\x44\xcc\x05\xbb\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 128 */ "\x00\x00\x33\x00\x00\x00\x44\x00\x00\x00\x00\x00\x33\x00\x44\x00"
 /* 144 */ "\x33\x22\x66\x00\x55\x55\xcc\x00\x33\x00\x00\x00\x00\x22\x00\x55"
 /* 160 */ "\x00\x22\x00\x55\x00\x02\x00\x05\x00\x22\x00\x00\x33\x44\x33\x66"
 /* 176 */ "\x55\xcc\x33\x66\x55\xcc\x33\x46\x05\xbb\x33\x66\x55\xcc\x00\x00"
 /* 192 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x44\x00\x00\x00"
 /* 208 */ "\x33\x00\x00\x22\x55\x22\x55\x02\x05\x22\x55\x02\x46\x22\xaa\x55"
 /* 224 */ "\xcc\x22\x55\x02\x46\x22\xaa\x00\x22\x55\x22\x55\x02\x05\x22\x55"
 /* 240 */ "\x02\x05\x22\x55\x02\x05\x22\x55\x02\x05\x22\x55\x02\x44\x66\xcc"
 /* 256 */ "\x66\xcc\x46\xbb\x66\xcc\x46\xbb\x66\xcc\x46\xbb\x66\xcc\x46\xbb"
 /* 272 */ "\x66\xcc\x46\xbb\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x33\x00"
 /* 288 */ "\x00\x00\x44\x00\x00\x00\x33\x00\x22\x22\x66\x00\x00\x00\x00\x00"
 /* 304 */ "\x03\x00\x44\x00\x33\x22\x66\x00\x55\x55\xcc\x00\x33\x22\x66\x00"
 /* 320 */ "\x55\x55\xcc\x00\x03\x00\x00\x00\x00\x02\x00\x55\x00\x02\x00\x55"
 /* 336 */ "\x00\x02\x00\x05\x00\x02\x00\x55\x00\x02\x02\x46\x00\x02\x00\x55"
 /* 352 */ "\x55\x05\x55\x46\xaa\xcc\x55\x46\xaa\xcc\x55\x06\x5a\xbb\x55\x46"
 /* 368 */ "\xaa\xcc\x55\x06\x5a\xbb\x55\x46\xaa\xcc\x00\x00\x00\x00\x00\x00"
 /* 384 */ "\x00\x00\x00\x00\x03\x00\x00\x00\x44\x00\x00\x00\x33\x00\x22\x22"
 /* 400 */ "\x66\x00\x00\x00\x55\x00\x55\x55\x05\x55\x05\x55\x05\x55\x05\x55"
 /* 416 */ "\x46\x55\x5a\xaa\xcc\x55\x05\x55\x46\x55\x5a\xaa\xcc\x55\x05\x55"
 /* 432 */ "\x06\x55\x0a\x55\x55\x05\x55\x05\x55\x05\x55\x05\x55\x05\x55\x05"
 /* 448 */ "\x55\x05\x55\x05\x55\x05\x55\x05\x55\x46\x55\x05\x55\x5a\x55\x5a"
 /* 464 */ "\xaa\xcc\xcc\xbb\xcc\xbb\xcc\xbb\xcc\xbb\xcc\xbb\xcc\xbb\xcc\xbb"
 /* 480 */ "\xcc\xbb\xcc\xbb\xcc\xbb\xcc\xbb\xcc\xbb\xcc\xbb\xcc\xbb\xcc\xbb"
 /* 496 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x33\x00\x00\x00\x44\x00"
 /* 512 */ "\x00\x00\x33\x00\x22\x22\x66\x00\x00\x00\x55\x00\x55\x55\xcc\x00"
 /* 528 */ "\x00\x00\x00\x00\x33\x00\x44\x00\x33\x22\x66\x00\x55\x55\xcc\x00"
 /* 544 */ "\x33\x22\x66\x00\x55\x55\xcc\x00\x33\x02\x46\x00\x05\x05\xbb\x00"
 /* 560 */ "\x33\x00\x00\x00\x00\x22\x00\x55\x00\x22\x00\x55\x00\x02\x00\x05"
 /* 576 */ "\x00\x22\x00\x55\x00\x02\x02\x46\x00\x22\x00\xaa\x00\x55\x55\xcc"
 /* 592 */ "\x00\x22\x00\x00\x33\x44\x33\x66\x55\xcc\x33\x66\x55\xcc\x33\x46"
 /* 608 */ "\x05\xbb\x33\x66\x55\xcc\x33\x46\x05\xbb\x33\x66\x55\xcc\x33\x46"
 /* 624 */ "\x05\xbb\x33\x66\x55\xcc\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 640 */ "\x03\x00\x00\x00\x44\x00\x00\x00\x33\x00\x22\x22\x66\x00\x00\x00"
 /* 656 */ "\x55\x00\x55\x55\xcc\x00\x00\x00\x33\x00\x00\x22\x55\x22\x55\x02"
 /* 672 */ "\x05\x22\x55\x02\x46\x22\xaa\x55\xcc\x22\x55\x02\x46\x22\xaa\x55"
 /* 688 */ "\xcc\x22\x55\x02\x06\x22\x5a\x05\xbb\x22\x55\x02\x46\x22\xaa\x00"
 /* 704 */ "\x22\x55\x22\x55\x02\x05\x22\x55\x02\x05\x22\x55\x02\x05\x22\x55"
 /* 720 */ "\x02\x05\x22\x55\x02\x46\x22\x55\x02\x5a\x22\xaa\x55\xcc\x22\x55"
 /* 736 */ "\x02\x05\x22\x55\x02\x44\x66\xcc\x66\xcc\x46\xbb\x66\xcc\x46\xbb"
 /* 752 */ "\x66\xcc\x46\xbb\x66\xcc\x46\xbb\x66\xcc\x46\xbb\x66\xcc\x46\xbb"
 /* 768 */ "\x66\xcc\x46\xbb\x66\xcc\x46\xbb\x66\xcc\x46\xbb\x00\x00\x00\x00"
 /* 784 */ "\x00\x00\x00\x00\x00\x00\x33\x00\x00\x00\x44\x00\x00\x00\x33\x00"
 /* 800 */ "\x22\x22\x66\x00\x00\x00\x55\x00\x55\x55\xcc\x00\x00\x00\x33\x00"
 /* 816 */ "\x22\x22\x66\x00\x00\x00\x00\x00\x03\x00\x44\x00\x33\x22\x66\x00"
 /* 832 */ "\x55\x55\xcc\x00\x33\x22\x66\x00\x55\x55\xcc\x00\x03\x02\x46\x00"
 /* 848 */ "\x05\x05\xbb\x00\x33\x22\x66\x00\x55\x55\xcc\x00\x03\x00\x00\x00"
 /* 864 */ "\x00\x02\x00\x55\x00\x02\x00\x55\x00\x02\x00\x05\x00\x02\x00\x55"
 /* 880 */ "\x00\x02\x02\x46\x00\x02\x00\xaa\x00\x55\x55\xcc\x00\x02\x00\x55"
 /* 896 */ "\x00\x02\x02\x46\x00\x02\x00\x55\x55\x05\x55\x46\xaa\xcc\x55\x46"
 /* 912 */ "\xaa\xcc\x55\x06\x5a\xbb\x55\x46\xaa\xcc\x55\x06\x5a\xbb\x55\x46"
 /* 928 */ "\xaa\xcc\x55\x06\x5a\xbb\x55\x46\xaa\xcc\x55\x06\x5a\xbb\x55\x46"
 /* 944 */ "\xaa\xcc\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00"
 /* 960 */ "\x44\x00\x00\x00\x33\x00\x22\x22\x66\x00\x00\x00\x55\x00\x55\x55"
 /* 976 */ "\xcc\x00\x00\x00\x33\x00\x22\x22\x66\x00\x00\x00\x55\x00\x55\x55"
 /* 992 */ "\x05\x55\x05\x55\x05\x55\x05\x55\x46\x55\x5a\xaa\xcc\x55\x05\x55"
 /* 1008 */ "\x46\x55\x5a\xaa\xcc\x55\x05\x55\x06\x55\x0a\x5a\xbb\x55\x05\x55"
 /* 1024 */ "\x46\x55\x5a\xaa\xcc\x55\x05\x55\x06\x55\x0a\x55\x55\x05\x55\x05"
 /* 1040 */ "\x55\x05\x55\x05\x55\x05\x55\x05\x55\x05\x55\x05\x55\x05\x55\x05"
 /* 1056 */ "\x55\x46\x55\x05\x55\x5a\x55\x5a\xaa\xcc\x55\x05\x55\x05\x55\x05"
 /* 1072 */ "\x55\x46\x55\x05\x55\x5a\x55\x5a\xaa\xcc\xcc\xbb\xcc\xbb\xcc\xbb"
 /* 1088 */ "\xcc\xbb\xcc\xbb\xcc\xbb\xcc\xbb\xcc\xbb\xcc\xbb\xcc\xbb\xcc\xbb"
 /* 1104 */ "\xcc\xbb\xcc\xbb\xcc\xbb\xcc\xbb\xcc\xbb\xcc\xbb\xcc\xbb\xcc\xbb"
 /* 1120 */ "\xcc\xbb\xcc\xbb\xcc\xbb\xcc\xbb\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 1136 */ "\x00\x00\x33\x00\x00\x00\x44\x00\x00\x00\x33\x00\x22\x22\x66\x00"
 /* 1152 */ "\x00\x00\x55\x00\x55\x55\xcc\x00\x00\x00\x33\x00\x22\x22\x66\x00"
 /* 1168 */ "\x00\x00\x55\x00\x55\x55\xcc\x00\x00\x00\x00\x00\x33\x00\x44\x00"
 /* 1184 */ "\x33\x22\x66\x00\x55\x55\xcc\x00\x33\x22\x66\x00\x55\x55\xcc\x00"
 /* 1200 */ "\x33\x02\x46\x00\x05\x05\xbb\x00\x33\x22\x66\x00\x55\x55\xcc\x00"
 /* 1216 */ "\x33\x02\x46\x00\x05\x05\xbb\x00\x33\x00\x00\x00\x00\x22\x00\x55"
 /* 1232 */ "\x00\x22\x00\x55\x00\x02\x00\x05\x00\x22\x00\x55\x00\x02\x02\x46"
 /* 1248 */ "\x00\x22\x00\xaa\x00\x55\x55\xcc\x00\x22\x00\x55\x00\x02\x02\x46"
 /* 1264 */ "\x00\x22\x00\xaa\x00\x55\x55\xcc\x00\x22\x00\x00\x03\x44\x33\x66"
 /* 1280 */ "\x55\xcc\x33\x66\x55\xcc\x03\x46\x05\xbb\x33\x66\x55\xcc\x03\x46"
 /* 1296 */ "\x05\xbb\x33\x66\x55\xcc\x03\x46\x05\xbb\x33\x66\x55\xcc\x03\x46"
 /* 1312 */ "\x05\xbb\x33\x66\x55\xcc\x03\x46\x05\xbb\x33\x66\x55\xcc\x00\x00"
 /* 1328 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x44\x00\x00\x00"
 /* 1344 */ "\x33\x00\x22\x22\x66\x00\x00\x00\x55\x00\x55\x55\xcc\x00\x00\x00"
 /* 1360 */ "\x33\x00\x22\x22\x66\x00\x00\x00\x55\x00\x55\x55\xcc\x00\x00\x00"
 /* 1376 */ "\x03\x00\x00\x02\x55\x02\x55\x02\x05\x02\x55\x02\x46\x02\xaa\x55"
 /* 1392 */ "\xcc\x02\x55\x02\x46\x02\xaa\x55\xcc\x02\x55\x02\x06\x02\x5a\x05"
 /* 1408 */ "\xbb\x02\x55\x02\x46\x02\xaa\x55\xcc\x02\x55\x02\x06\x02\x5a\x05"
 /* 1424 */ "\xbb\x02\x55\x02\x46\x02\xaa\x00\x02\x55\x02\x55\x02\x05\x02\x55"
 /* 1440 */ "\x02\x05\x02\x55\x02\x05\x02\x55\x02\x05\x02\x55\x02\x46\x02\x55"
 /* 1456 */ "\x02\x5a\x02\xaa\x55\xcc\x02\x55\x02\x05\x02\x55\x02\x46\x02\x55"
 /* 1472 */ "\x02\x5a\x02\xaa\x55\xcc\x02\x55\x02\x05\x02\x55\x02\x05\x46\xcc"
 /* 1488 */ "\x46\xcc\x06\xbb\x46\xcc\x06\xbb\x46\xcc\x06\xbb\x46\xcc\x06\xbb"
 /* 1504 */ "\x46\xcc\x06\xbb\x46\xcc\x06\xbb\x46\xcc\x06\xbb\x46\xcc\x06\xbb"
 /* 1520 */ "\x46\xcc\x06\xbb\x46\xcc\x06\xbb\x46\xcc\x06\xbb\x46\xcc\x06\xbb"
 /* 1536 */ "\x46\xcc\x06\xbb\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x33\x00"
 /* 1552 */ "\x00\x00\x44\x00\x00\x00\x33\x00\x22\x22\x66\x00\x00\x00\x55\x00"
 /* 1568 */ "\x55\x55\xcc\x00\x00\x00\x33\x00\x22\x22\x66\x00\x00\x00\x55\x00"
 /* 1584 */ "\x55\x55\xcc\x00\x00\x00\x33\x00\x02\x02\x46\x00\x00\x00\x00\x00"
 /* 1600 */ "\x03\x00\x44\x00\x33\x22\x66\x00\x55\x55\xcc\x00\x33\x22\x66\x00"
 /* 1616 */ "\x55\x55\xcc\x00\x03\x02\x46\x00\x05\x05\xbb\x00\x33\x22\x66\x00"
 /* 1632 */ "\x55\x55\xcc\x00\x03\x02\x46\x00\x05\x05\xbb\x00\x33\x22\x66\x00"
 /* 1648 */ "\x55\x55\xcc\x00\x03\x00\x00\x00\x00\x02\x00\x55\x00\x02\x00\x55"
 /* 1664 */ "\x00\x02\x00\x05\x00\x02\x00\x55\x00\x02\x02\x46\x00\x02\x00\xaa"
 /* 1680 */ "\x00\x55\x55\xcc\x00\x02\x00\x55\x00\x02\x02\x46\x00\x02\x00\xaa"
 /* 1696 */ "\x00\x55\x55\xcc\x00\x02\x00\x55\x00\x02\x02\x06\x00\x02\x00\x05"
 /* 1712 */ "\x05\x05\x05\x46\x5a\xcc\x05\x46\x5a\xcc\x05\x06\x0a\xbb\x05\x46"
 /* 1728 */ "\x5a\xcc\x05\x06\x0a\xbb\x05\x46\x5a\xcc\x05\x06\x0a\xbb\x05\x46"
 /* 1744 */ "\x5a\xcc\x05\x06\x0a\xbb\x05\x46\x5a\xcc\x05\x06\x0a\xbb\x05\x46"
 /* 1760 */ "\x5a\xcc\x05\x06\x0a\xbb\x05\x46\x5a\xcc\x00\x00\x00\x00\x00\x00"
 /* 1776 */ "\x00\x00\x00\x00\x03\x00\x00\x00\x44\x00\x00\x00\x33\x00\x22\x22"
 /* 1792 */ "\x66\x00\x00\x00\x55\x00\x55\x55\xcc\x00\x00\x00\x33\x00\x22\x22"
 /* 1808 */ "\x66\x00\x00\x00\x55\x00\x55\x55\xcc\x00\x00\x00\x03\x00\x02\x02"
 /* 1824 */ "\x46\x00\x00\x00\x05\x00\x05\x05\x05\x05\x05\x05\x05\x05\x05\x05"
 /* 1840 */ "\x46\x05\x5a\x5a\xcc\x05\x05\x05\x46\x05\x5a\x5a\xcc\x05\x05\x05"
 /* 1856 */ "\x06\x05\x0a\x0a\xbb\x05\x05\x05\x46\x05\x5a\x5a\xcc\x05\x05\x05"
 /* 1872 */ "\x06\x05\x0a\x0a\xbb\x05\x05\x05\x46\x05\x5a\x5a\xcc\x05\x05\x05"
 /* 1888 */ "\x06\x05\x0a\x05\x05\x05\x05\x05\x05\x05\x05\x05\x05\x05\x05\x05"
 /* 1904 */ "\x05\x05\x05\x05\x05\x05\x05\x05\x05\x46\x05\x05\x05\x5a\x05\x5a"
 /* 1920 */ "\x5a\xcc\x05\x05\x05\x05\x05\x05\x05\x46\x05\x05\x05\x5a\x05\x5a"
 /* 1936 */ "\x5a\xcc\x05\x05\x05\x05\x05\x05\x05\x06\x05\x05\x05\x0a\x05\x0a"
 /* 1952 */ "\x0a";

unsigned char  * const adv_gomoku /* [978] */ =
 /* 0 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 16 */ "\x00\x00\x00\x00\xa0\x00\xa0\x00\x04\x00\x04\x00\x00\xd0\x00\xd0"
 /* 32 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 48 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 64 */ "\x00\x70\x00\x00\x00\x00\xa0\x00\xa1\x00\x00\x00\xa0\x00\x04\x00"
 /* 80 */ "\x04\x00\x00\x00\x04\x00\xd0\xd0\x00\xd0\x00\xd0\x00\xd0\x00\x00"
 /* 96 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x70\x08\x08\x00\x08\x00\x08\x00"
 /* 112 */ "\x08\x00\x08\x00\x40\x40\x00\x40\x00\x40\x00\x40\x00\x40\x00\x00"
 /* 128 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x70"
 /* 144 */ "\x00\x00\x00\x70\x21\x00\x00\x00\x00\x00\xa1\x00\x00\x00\xa1\x00"
 /* 160 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 176 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 192 */ "\x00\x00\x00\x00\x00\x00\x70\x21\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 208 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 224 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 240 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x70\x00\x00"
 /* 256 */ "\x00\x70\x21\x00\x00\x00\x00\x00\x00\x00\xa0\x00\xa1\x00\x00\x00"
 /* 272 */ "\xa1\x00\x00\x00\xa0\x00\x00\x00\xa0\x00\x04\x00\x04\x00\x00\x00"
 /* 288 */ "\x04\x00\x00\x00\x04\x00\x00\x00\x04\x00\xd0\xd0\x00\xd0\x00\xd0"
 /* 304 */ "\x00\xd0\x00\xd0\x00\xd0\x00\xd0\x00\xd0\x00\x00\x00\x00\x00\x00"
 /* 320 */ "\x00\x00\x00\x00\x70\x21\x00\x00\x00\x00\x00\x00\x70\x08\x08\x00"
 /* 336 */ "\x08\x00\x08\x00\x08\x00\x08\x00\x08\x00\x08\x00\x08\x00\x08\x00"
 /* 352 */ "\x40\x40\x00\x40\x00\x40\x00\x40\x00\x40\x00\x40\x00\x40\x00\x40"
 /* 368 */ "\x00\x40\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 384 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x70\x00\x00\x00\x70"
 /* 400 */ "\x21\x00\x00\x00\x00\x00\x00\x70\x21\x00\x00\x00\x00\x00\xa1\x00"
 /* 416 */ "\x00\x00\xa1\x00\x00\x00\x00\x00\x00\x00\xa1\x00\x00\x00\x00\x00"
 /* 432 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 448 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 464 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 480 */ "\x00\x00\x70\x21\x00\x00\x00\x00\x00\x00\x70\x21\x00\x00\x00\x00"
 /* 496 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 512 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 528 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 544 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 560 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x70\x00\x00\x00\x70\x21\x00"
 /* 576 */ "\x00\x00\x00\x00\x00\x70\x21\x00\x00\x00\x00\x00\x00\x00\xa0\x00"
 /* 592 */ "\xa1\x00\x00\x00\xa1\x00\x00\x00\xa0\x00\x00\x00\xa1\x00\x00\x00"
 /* 608 */ "\xa0\x00\x00\x00\xa0\x00\x04\x00\x04\x00\x00\x00\x04\x00\x00\x00"
 /* 624 */ "\x04\x00\x00\x00\x04\x00\x00\x00\x04\x00\x00\x00\x04\x00\x00\xd0"
 /* 640 */ "\x00\xd0\x00\x00\x00\xd0\x00\x00\x00\xd0\x00\x00\x00\xd0\x00\x00"
 /* 656 */ "\x00\xd0\x00\x00\x00\xd0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 672 */ "\x70\x21\x00\x00\x00\x00\x00\x00\x70\x21\x00\x00\x00\x00\x00\x00"
 /* 688 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 704 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 720 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 736 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 752 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 768 */ "\x00\x00\x00\x00\x00\x00\x00\x70\x00\x00\x00\x70\x21\x00\x00\x00"
 /* 784 */ "\x00\x00\x00\x70\x21\x00\x00\x00\x00\x00\x00\x70\x00\x00\x00\x00"
 /* 800 */ "\x00\x00\xa1\x00\x00\x00\xa1\x00\x00\x00\x00\x00\x00\x00\xa1\x00"
 /* 816 */ "\x00\x00\x00\x00\x00\x00\xa1\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 832 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 848 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 864 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 880 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x70\x21"
 /* 896 */ "\x00\x00\x00\x00\x00\x00\x70\x21\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 912 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 928 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 944 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 960 */ "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
 /* 976 */ "\x00";

/* name.c */
word_t         *toplev;

#ifndef _BBS_UTIL_C_
/* menu.c */
const commands_t      cmdlist[] = {
    {admin, PERM_SYSOP | PERM_VIEWSYSOP, "00Admin       【 系統維護區 】"},
    {Announce, 0, "AAnnounce     【 精華公佈欄 】"},
    {Boards, 0, "FFavorite     【 我 的 最愛 】"},
    {root_board, 0, "CClass        【 分組討論區 】"},
    {Mail, PERM_BASIC, "MMail         【 私人信件區 】"},
    {Talk, 0, "TTalk         【 休閒聊天區 】"},
    {User, 0, "UUser         【 個人設定區 】"},
    {Xyz, 0, "XXyz          【 系統工具區 】"},
    {Play_Play, PERM_BASIC, "PPlay         【 娛樂/休閒生活】"},
    {Name_Menu, PERM_LOGINOK, "NNamelist     【 編特別名單 】"},
    {Goodbye, 0, "GGoodbye       離開，再見……"},
    {NULL, 0, NULL}
};
#endif

/* friend.c */
/* Ptt 各種特別名單的檔名 */
char           *friend_file[8] = {
    FN_OVERRIDES,
    FN_REJECT,
    "alohaed",
    "postlist",
    "", /* may point to other filename */
    FN_CANVOTE,
    FN_WATER,
    FN_VISABLE
};

#ifdef NOKILLWATERBALL
char    reentrant_write_request = 0;
#endif

#ifdef PTTBBS_UTIL
    #ifdef OUTTA_TIMER
	#define COMMON_TIME (SHM->GV2.e.now)
    #else
	#define COMMON_TIME (time(0))
    #endif
#else
    #define COMMON_TIME (now)
#endif
