/* $Id$ */
#ifndef INCLUDE_STRUCT_H
#define INCLUDE_STRUCT_H

#define IDLEN      12             /* Length of bid/uid */
typedef struct bid_t {
    int    high;
    int    buyitnow;
    int    increment;
    char   userid[IDLEN + 1];
    time_t enddate;
    int    payby; /* 1 cash 2 check or mail 4 wire 8 credit 16 postoffice */
    int    shipping;
}bid_t;

/* �p������� */
typedef struct chicken_t {
    char    name[20];
    char    type;             /* ���� */
    unsigned char   tech[16]; /* �ޯ� */
    time_t  birthday;         /* �ͤ� */
    time_t  lastvisit;        /* �W�����U�ɶ� */
    int     oo;               /* �ɫ~ */
    int     food;             /* ���� */
    int     medicine;         /* �ī~ */
    int     weight;           /* �魫 */
    int     clean;            /* ���b */
    int     run;              /* �ӱ��� */
    int     attack;           /* �����O */
    int     book;             /* ���� */
    int     happy;            /* �ּ� */
    int     satis;            /* ���N�� */
    int     temperament;      /* ��� */
    int     tiredstrong;      /* �h�ҫ� */
    int     sick;             /* �f����� */
    int     hp;               /* ��q */
    int     hp_max;           /* ����q */
    int     mm;               /* �k�O */
    int     mm_max;           /* ���k�O */
    time_t  cbirth;           /* ��ڭp��Ϊ��ͤ� */
    int     pad[2];           /* �d�ۥH��� */
} chicken_t;

#define PASSLEN    14             /* Length of encrypted passwd field */
#define REGLEN     38             /* Length of registration data */

typedef struct userec_t {
    char    userid[IDLEN + 1];
    char    realname[20];
    char    username[24];
    char    passwd[PASSLEN];
    unsigned char   uflag;
    unsigned int    userlevel;
    unsigned short  numlogins;
    unsigned short  numposts;
    time_t  firstlogin;
    time_t  lastlogin;
    char    lasthost[16];
    int     money;
    char    remoteuser[3];           /* �O�d �ثe�S�Ψ쪺 */
    char    proverb;
    char    email[50];
    char    address[50];
    char    justify[REGLEN + 1];
    unsigned char   month;
    unsigned char   day;
    unsigned char   year;
    unsigned char   sex;
    unsigned char   state;
    unsigned char   pager;
    unsigned char   invisible;
    unsigned int    exmailbox;
    chicken_t       mychicken;
    time_t  lastsong;
    unsigned int    loginview;
    unsigned char   channel;      /* �ʺA�ݪO */
    unsigned short  vl_count;     /* ViolateLaw counter */
    unsigned short  five_win;
    unsigned short  five_lose;
    unsigned short  five_tie;
    unsigned short  chc_win;
    unsigned short  chc_lose;
    unsigned short  chc_tie;
    int     mobile;
    char    mind[4];
    char    ident[11];
    unsigned int    uflag2;
    unsigned char   signature;
    char    pad[71];
} userec_t;
/* these are flags in userec_t.uflag */
#define PAGER_FLAG      0x4     /* true if pager was OFF last session */
#define CLOAK_FLAG      0x8     /* true if cloak was ON last session */
#define FRIEND_FLAG     0x10    /* true if show friends only */
#define BRDSORT_FLAG    0x20    /* true if the boards sorted alphabetical */
#define MOVIE_FLAG      0x40    /* true if show movie */
#define COLOR_FLAG      0x80    /* true if the color mode open */
#define MIND_FLAG       0x100   /* true if mind search mode open <-Heat*/
/* these are flags in userec_t.uflag2 */
#define WATER_MASK      000003  /* water mask */
#define WATER_ORIG      0x0
#define WATER_NEW       0x1
#define WATER_OFO       0x2
#define WATERMODE(mode) ((cuser.uflag2 & WATER_MASK) == mode)
#define FAVNOHILIGHT    0x10   /* false if hilight favorite */
#define FAVNEW_FLAG     0x20   /* true if add new board into one's fav */
#define FOREIGN         0x100  /* true if a foreign */
#define LIVERIGHT       0x200  /* true if get "liveright" already */

#define BTLEN      48             /* Length of board title */

typedef struct boardheader_t {
    char    brdname[IDLEN + 1];          /* bid */
    char    title[BTLEN + 1];
    char    BM[IDLEN * 3 + 3];           /* BMs' userid, token '/' */
    unsigned int    brdattr;             /* board���ݩ� */
    char    pad[3];                      /* �S�Ψ쪺 */
    time_t  bupdate;                     /* note update time */
    char    pad2[3];                     /* �S�Ψ쪺 */
    unsigned char   bvote;               /* Vote flags */
    time_t  vtime;                       /* Vote close time */
    unsigned int    level;               /* �i�H�ݦ��O���v�� */
    int     unused;                      /* �٨S�Ψ� */
    int     gid;                         /* �ݪO���ݪ����O ID */
    void    *next[2];	                 /* �b�P�@��gid�U�@�ӬݪO �ʺA����*/
    void    *firstchild[2];	         /* �ݩ�o�ӬݪO���Ĥ@�Ӥl�ݪO */
    void    *parent;
    int     childcount;                  /* ���h�֭�child */
    int     nuser;                       /* �h�֤H�b�o�O */
    char    pad5[4];                     /* original void *u */
    time_t  endgamble;
    char    pad3[84];
} boardheader_t;

#define BRD_NOZAP       000000001         /* ���izap  */
#define BRD_NOCOUNT     000000002         /* ���C�J�έp */
#define BRD_NOTRAN      000000004         /* ����H */
#define BRD_GROUPBOARD  000000010         /* �s�ժO */
#define BRD_HIDE        000000020         /* ���êO (�ݪO�n�ͤ~�i��) */
#define BRD_POSTMASK    000000040         /* ����o��ξ\Ū */
#define BRD_ANONYMOUS   000000100         /* �ΦW�O */
#define BRD_DEFAULTANONYMOUS 000000200    /* �w�]�ΦW�O */
#define BRD_BAD		000000400         /* �H�k��i���ݪO */
#define BRD_VOTEBOARD   000001000         /* �s�p���ݪO */
#define BRD_WARNEL      000002000         /* �s�p���ݪO */
#define BRD_TOP         000004000         /* �����ݪO�s�� */
#define BRD_NORECOMMEND 000010000         /* ���i���� */
#define BRD_BLOG        000020000         /* BLOG */

#define TTLEN      64             /* Length of title */
#define FNLEN      33             /* Length of filename  */

#define FHR_REFERENCE	(1<<31)

typedef struct fileheader_t {
    char    filename[FNLEN];         /* M.9876543210.A */
    char    recommend;               /* important level */
    char    owner[IDLEN + 2];        /* uid[.] */
    char    date[6];                 /* [02/02] or space(5) */
    char    title[TTLEN + 1];
    int     money;	             /* rocker: if bit32 on ==> reference */
    unsigned char   filemode;        /* must be last field @ boards.c */
} fileheader_t;

#define FILE_LOCAL      0x1     /* local saved */
#define FILE_READ       0x1     /* already read : mail only */
#define FILE_MARKED     0x2     /* opus: 0x8 */
#define FILE_DIGEST     0x4     /* digest */
#define FILE_HOLD       0x8     /* holdmail for mail */
#define FILE_SOLVED	0x10	/* problem solved, sysop only */
#define FILE_HIDE       0x20    /* hild */
#define FILE_BM         0x40    /* BM only */
#define FILE_BOTH       0x80    /* both replay for mail and board */
#define FILE_MULTI      0x100   /* multi send for mail */
#define FILE_BID        0x200   /* for bid */

#define STRLEN     80             /* Length of most string data */


/* uhash is a userid->uid hash table -- jochang */

#define HASH_BITS 16

union xitem_t {
    struct {                    /* bbs_item */
	char    fdate[9];       /* [mm/dd/yy] */
	char    editor[13];      /* user ID */
	char    fname[31];
    } B;
    struct {                    /* gopher_item */
	char    path[81];
	char    server[48];
	int     port;
    } G;
};

typedef struct {
    char    title[63];
    union   xitem_t X;
} item_t;

typedef struct {
    item_t  *item[MAX_ITEMS];
    char    mtitle[STRLEN];
    char    *path;
    int     num, page, now, level;
} gmenu_t;

#define FAVMAX   1024		  /* Max boards of Myfavorite */
#define FAVGMAX    32             /* Max groups of Myfavorite */
#define FAVGSLEN    8		  /* Max Length of Description String */

typedef struct msgque_t {
    pid_t   pid;
    char    userid[IDLEN + 1];
    char    last_call_in[80];
} msgque_t;

typedef struct userinfo_t {
    int     uid;                  /* Used to find user name in passwd file */
    pid_t   pid;                  /* kill() to notify user of talk request */
    int     sockaddr;             /* ... */
    int     destuid;              /* talk uses this to identify who called */
    int     destuip;              /* dest index in utmpshm->uinfo[] */
    unsigned char   active;         /* When allocated this field is true */
    unsigned char   invisible;      /* Used by cloaking function in Xyz menu */
    unsigned char   sockactive;     /* Used to coordinate talk requests */
    unsigned int    userlevel;
    unsigned char   mode;           /* UL/DL, Talk Mode, Chat Mode, ... */
    unsigned char   pager;          /* pager toggle, YEA, or NA */
    unsigned char   in_chat;        /* for in_chat commands   */
    unsigned char   sig;            /* signal type */
    char    userid[IDLEN + 1];
    char    chatid[11];             /* chat id, if in chat mode */
    char    username[24];
    char    from[27];               /* machine name the user called in from */
    int     from_alias;
    char    birth;                   /* �O�_�O�ͤ� Ptt*/
    char    tty[11];                 /* tty port */
    int     friend[MAX_FRIEND];
    int     friend_online[MAX_FRIEND];/* point��u�W�n�� utmpshm����m */
			          /* �n�ͤ����cache �e���bit�O���A */
    int     reject[MAX_REJECT];
    int     idoffset;                 /* shm id�W�� offset */
    int     lock;
    int     friendtotal;              /* �n�ͤ����cache �j�p */ 
    unsigned char   msgcount;
    msgque_t        msgs[MAX_MSGS];
    time_t  uptime;
    time_t  lastact;               /* �W���ϥΪ̰ʪ��ɶ� */
    unsigned int    brc_id;
    unsigned char   lockmode;       /* ���� multi_login �����F�� */
    char    turn;                    /* for gomo */
    char    mateid[IDLEN + 1];       /* for gomo */
    unsigned short  int     five_win;
    unsigned short  int     five_lose;
    unsigned short  int     five_tie;
    unsigned short  int     chc_win;
    unsigned short  int     chc_lose;
    unsigned short  int     chc_tie;
    char    mailalert;
    char    sex;
    char    color;
    char    mind[4];
} userinfo_t;

typedef struct water_t {
    pid_t   pid;
    char    userid[IDLEN + 1];
    int     top, count;
    msgque_t        msg[MAX_REVIEW];
    userinfo_t      *uin;     // Ptt:�o�i�H���Nalive
} water_t;

typedef struct {
    fileheader_t    *header;
    char    mtitle[STRLEN];
    char    *path;
    int     num, page, now, level;
} menu_t;

typedef struct onekey_t {     /* Used to pass commands to the readmenu */
    int     key;
    int     (*fptr)();
} onekey_t;

#define ANSILINELEN (511)                /* Maximum Screen width in chars */

/* anti_crosspost */
typedef struct crosspost_t {
    int     checksum[4]; /* 0 -> 'X' cross post  1-3 -> ²�d�峹�� */
    int     times;       /* �ĴX�� */
} crosspost_t;

#define SORT_BY_ID    0
#define SORT_BY_CLASS 1
#define SORT_BY_STAT  1
#define SORT_BY_IDLE  2
#define SORT_BY_FROM  3
#define SORT_BY_FIVE  4
#define SORT_BY_SEX   5

typedef struct keeploc_t {
    char    *key;
    int     top_ln;
    int     crs_ln;
    struct  keeploc_t       *next;
} keeploc_t;

#define USHM_SIZE       (MAX_ACTIVE + 4)  /* why+4? */

/* MAX_BMs is dirty hardcode 4 in mbbsd/cache.c:is_BM_cache() */
#define MAX_BMs         4                 /* for BMcache, �@�ӬݪO�̦h�X�O�D */

typedef struct {
    /* uhash */
    char    userid[MAX_USERS][IDLEN + 1];
    int     next_in_hash[MAX_USERS];
    int     money[MAX_USERS];
    int     hash_head[1 << HASH_BITS];
    int     number;				/* # of users total */
    int     loaded;				/* .PASSWD has been loaded? */

    /* utmpshm */
    userinfo_t      uinfo[USHM_SIZE];
    userinfo_t      *sorted[2][8][USHM_SIZE];
                    /* �Ĥ@��double buffer ��currsorted���V�ثe�ϥΪ�
		       �ĤG��sort type */
    int     currsorted;
    time_t  UTMPuptime;
    int     UTMPnumber;
    char    UTMPneedsort;
    char    UTMPbusystate;

    /* brdshm */
    int     BMcache[MAX_BOARD][MAX_BMs];
    boardheader_t   bcache[MAX_BOARD];
    boardheader_t   *bsorted[2][MAX_BOARD]; /* 0: by name 1: by class */
    fileheader_t    dircache[MAX_BOARD][DIRCACHESIZE];
    time_t  busystate_b[MAX_BOARD];
    int     total[MAX_BOARD];
    int     hbfl[MAX_BOARD][MAX_FRIEND + 1];
    time_t  lastposttime[MAX_BOARD];
    time_t  Buptime;
    time_t  Btouchtime;
    int     Bnumber;
    int     Bbusystate;
    time_t  close_vote_time;

    /* pttcache */
    char    notes[MAX_MOVIE][200*11];
    char    today_is[20];
    int     n_notes[MAX_MOVIE_SECTION];      /* �@�`�����X�� �ݪO */
    int     next_refresh[MAX_MOVIE_SECTION]; /* �U�@���nrefresh�� �ݪO */
    int     max_film;
    int     max_history;
    time_t  Puptime;
    time_t  Ptouchtime;
    int     Pbusystate;

    int     GLOBALVAR[10];                   /*  mbbsd���� global variable
						 �ΥH���έp����� (�D�`�A)  */

    union {
	int     v[256];
	struct {
	    int     dymaxactive;  /* �ʺA�]�w�̤j�H�ƤW��     */
	    int     toomanyusers; /* �W�L�H�ƤW�������i���Ӽ� */
	    int     noonlineuser; /* ���W�ϥΪ̤����G�����   */
#ifdef OUTTA_TIMER
	    time_t  now;
#endif
	    int     nWelcomes;
	} e;
    } GV2;

    /* fromcache */
    char    domain[MAX_FROM][50];
    char    replace[MAX_FROM][50];
    int     top;
    int     max_user;
    time_t  max_time;
    time_t  Fuptime;
    time_t  Ftouchtime;
    int     Fbusystate;
} SHM_t;

typedef struct {
    unsigned char oldlen;                /* previous line length */
    unsigned char len;                   /* current length of line */
    unsigned char mode;                  /* status of line, as far as update */
    unsigned char smod;                  /* start of modified data */
    unsigned char emod;                  /* end of modified data */
    unsigned char sso;                   /* start stand out */
    unsigned char eso;                   /* end stand out */
    unsigned char data[ANSILINELEN + 1];
} screenline_t;

typedef struct {
    int r, c;
} rc_t;

#define BRD_ROW           10
#define BRD_COL           9

typedef int board_t[BRD_ROW][BRD_COL];

/* name.c ���B�Ϊ���Ƶ��c */
typedef struct word_t {
    char    *word;
    struct  word_t  *next;
} word_t;

typedef struct commands_t {
    int     (*cmdfunc)();
    int     level;
    char    *desc;                   /* next/key/description */
} commands_t;

typedef struct MailQueue {
    char    filepath[FNLEN];
    char    subject[STRLEN];
    time_t  mailtime;
    char    sender[IDLEN + 1];
    char    username[24];
    char    rcpt[50];
    int     method;
    char    *niamod;
} MailQueue;

enum  {MQ_TEXT, MQ_UUENCODE, MQ_JUSTIFY};

typedef struct
{ 
    time_t  chrono;
    int     recno;
} TagItem;
#endif
