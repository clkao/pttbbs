/* $Id$ */
#ifndef INCLUDE_STRUCT_H
#define INCLUDE_STRUCT_H


#define IDLEN      12             /* Length of bid/uid */

/* �v�и�T */
#define SALE_COMMENTED 0x1
typedef struct bid_t {
    int     high;	/* �ثe�̰��� */
    int     buyitnow;	/* �����ʶR�� */
    int     usermax;	/* �۰��v�г̰��� */
    int     increment;	/* �X���W�B */
    char    userid[IDLEN + 1];	/* �̰��X���� */
    time4_t enddate;	/* ���Ф�� */
    char    payby;	/* �I�ڤ覡 */
	/* 1 cash 2 check or mail 4 wire 8 credit 16 postoffice */
    char    flag;	/* �ݩ� (�O�_�w����) */
    char    pad[2];
    int     shipping;	/* �B�O */
} bid_t;

/* �p������� */
typedef struct chicken_t {
    char    name[20];
    char    type;             /* ���� */
    unsigned char   tech[16]; /* �ޯ� */
    time4_t birthday;         /* �ͤ� */
    time4_t lastvisit;        /* �W�����U�ɶ� */
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
    time4_t cbirth;           /* ��ڭp��Ϊ��ͤ� */
    int     pad[2];           /* �d�ۥH��� */
} chicken_t;

#define PASSLEN    14             /* Length of encrypted passwd field */
#define REGLEN     38             /* Length of registration data */

#define PASSWD_VERSION	2275

typedef struct userec_t {
    unsigned int    version;	/* version number of this sturcture, we
    				 * use revision number of project to denote.*/

    char    userid[IDLEN + 1];	/* ID */
    char    realname[20];	/* �u��m�W */
    char    nickname[24];	/* �ʺ� */
    char    passwd[PASSLEN];	/* �K�X */
    unsigned int    uflag;	/* �ߺD1 */
    unsigned int    uflag2;	/* �ߺD2 */
    unsigned int    userlevel;	/* �v�� */
    unsigned int    numlogins;	/* �W������ */
    unsigned int    numposts;	/* �峹�g�� */
    time4_t firstlogin;		/* ���U�ɶ� */
    time4_t lastlogin;		/* �̪�W���ɶ� */
    char    lasthost[16];	/* �W���W���ӷ� */
    int     money;		/* Ptt�� */
    char    remoteuser[3];	/* �O�d �ثe�S�Ψ쪺 */
    char    proverb;		/* �y�k�� (unused) */
    char    email[50];		/* Email */
    char    address[50];	/* ��} */
    char    justify[REGLEN + 1];    /* �f�ָ�� */
    unsigned char   month;	/* �ͤ� �� */
    unsigned char   day;	/* �ͤ� �� */
    unsigned char   year;	/* �ͤ� �~ */
    unsigned char   sex;	/* �ʧO */
    unsigned char   state;	/* TODO unknown (unused ?) */
    unsigned char   pager;	/* �I�s�����A */
    unsigned char   invisible;	/* ���Ϊ��A */
    unsigned int    exmailbox;	/* �ʶR�H�c�� TODO short �N���F */
    chicken_t       mychicken;	/* �d�� */
    time4_t lastsong;		/* �W���I�q�ɶ� */
    unsigned int    loginview;	/* �i���e�� */
    unsigned char   channel;	/* TODO unused */
    unsigned short  vl_count;	/* �H�k�O�� ViolateLaw counter */
    unsigned short  five_win;	/* ���l�Ѿ��Z �� */
    unsigned short  five_lose;	/* ���l�Ѿ��Z �� */
    unsigned short  five_tie;	/* ���l�Ѿ��Z �M */
    unsigned short  chc_win;	/* �H�Ѿ��Z �� */
    unsigned short  chc_lose;	/* �H�Ѿ��Z �� */
    unsigned short  chc_tie;	/* �H�Ѿ��Z �M */
    int     mobile;		/* ������X */
    char    mind[4];		/* �߱� not a null-terminate string */
    char    pad0[11];
    unsigned char   signature;	/* �D��ñ�W�� */

    unsigned char   goodpost;	/* �������n�峹�� */
    unsigned char   badpost;	/* �������a�峹�� */
    unsigned char   goodsale;	/* �v�� �n������  */
    unsigned char   badsale;	/* �v�� �a������  */
    char    myangel[IDLEN+1];	/* �ڪ��p�Ѩ� */
    unsigned short  chess_elo_rating;	/* �H�ѵ��Ť� */
    unsigned int    withme;	/* �ڷQ��H�U�ѡA���.... */
    char    pad[34];
} userec_t;
/* these are flags in userec_t.uflag */
#define PAGER_FLAG      0x4     /* true if pager was OFF last session */
#define CLOAK_FLAG      0x8     /* true if cloak was ON last session */
#define FRIEND_FLAG     0x10    /* true if show friends only */
#define BRDSORT_FLAG    0x20    /* true if the boards sorted alphabetical */
#define MOVIE_FLAG      0x40    /* true if show movie */

/* useless flag */
//#define COLOR_FLAG      0x80    /* true if the color mode open */
//#define MIND_FLAG       0x100   /* true if mind search mode open <-Heat*/

#define DBCSAWARE_FLAG	0x200	/* true if DBCS-aware enabled. */
/* please keep this even if you don't have DBCSAWARE features turned on */

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
#define REJ_OUTTAMAIL   0x400 /* true if don't accept outside mails */
#define REJECT_OUTTAMAIL (cuser.uflag2 & REJ_OUTTAMAIL)

/* flags in userec_t.withme */
#define WITHME_ALLFLAG	0x55555555
#define WITHME_TALK	0x00000001
#define WITHME_NOTALK	0x00000002
#define WITHME_FIVE	0x00000004
#define WITHME_NOFIVE	0x00000008
#define WITHME_PAT	0x00000010
#define WITHME_NOPAT	0x00000020
#define WITHME_CHESS	0x00000040
#define WITHME_NOCHESS	0x00000080
#define WITHME_DARK	0x00000100
#define WITHME_NODARK	0x00000200
#define WITHME_GO	0x00000400
#define WITHME_NOGO	0x00000800

#ifdef PLAY_ANGEL
#define REJ_QUESTION    0x800 /* true if don't want to be angel for a while */
#define REJECT_QUESTION (cuser.uflag2 & REJ_QUESTION)
#define ANGEL_MASK      0x3000
#define ANGEL_R_MAEL    0x1000 /* true if reject male */
#define ANGEL_R_FEMAEL  0x2000 /* true if reject female */
#define ANGEL_STATUS()  ((cuser.uflag2 & ANGEL_MASK) >> 12)
#define ANGEL_SET(X)    (cuser.uflag2 = (cuser.uflag2 & ~ANGEL_MASK) | \
                          (((X) & 3) << 12))
#endif

#define BTLEN      48             /* Length of board title */

/* TODO �ʺA��s����줣���Ӹ�n�g�J�ɮת��V�b�@�_,
 * �ܤ֥έ� struct �]�_�Ӥ��� */
typedef struct boardheader_t {
    char    brdname[IDLEN + 1];          /* bid */
    char    title[BTLEN + 1];
    char    BM[IDLEN * 3 + 3];           /* BMs' userid, token '/' */
    unsigned int    brdattr;             /* board���ݩ� */
    char    chesscountry;
    unsigned char   vote_limit_posts;    /* �s�p : �峹�g�ƤU�� */
    unsigned char   vote_limit_logins;   /* �s�p : �n�J���ƤU�� */
    unsigned char   vote_limit_regtime;  /* �s�p : ���U�ɶ����� */
    time4_t bupdate;                     /* note update time */
    unsigned char   post_limit_posts;    /* �o��峹 : �峹�g�ƤU�� */
    unsigned char   post_limit_logins;   /* �o��峹 : �n�J���ƤU�� */
    unsigned char   post_limit_regtime;  /* �o��峹 : ���U�ɶ����� */
    unsigned char   bvote;               /* ���|�� Vote �� */
    time4_t vtime;                       /* Vote close time */
    unsigned int    level;               /* �i�H�ݦ��O���v�� */
    time4_t perm_reload;                 /* �̫�]�w�ݪO���ɶ� */
    int     gid;                         /* �ݪO���ݪ����O ID */
    int     next[2];	                 /* �b�P�@��gid�U�@�ӬݪO �ʺA����*/
    int     firstchild[2];	         /* �ݩ�o�ӬݪO���Ĥ@�Ӥl�ݪO */
    int     parent;
    int     childcount;                  /* ���h�֭�child */
    int     nuser;                       /* �h�֤H�b�o�O */
    int     postexpire;                  /* postexpire */
    time4_t endgamble;
    char    posttype[33];
    char    posttype_f;
    unsigned char fastrecommend_pause;	/* �ֳt�s�����j */
    char    pad3[49];
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
#define BRD_BMCOUNT	000040000	  /* �O�D�]�w�C�J�O�� */
#define BRD_SYMBOLIC	000100000	  /* symbolic link to board */
#define BRD_NOBOO       000200000         /* ���i�N */
#define BRD_LOCALSAVE   000400000         /* �w�] Local Save */
#define BRD_RESTRICTEDPOST 001000000      /* �O�ͤ~��o�� */
#define BRD_GUESTPOST   002000000         /* guest�� post */
#define BRD_COOLDOWN    004000000         /* �N�R */
#define BRD_CPLOG       010000000         /* �۰ʯd����O�� */
#define BRD_NOFASTRECMD 020000000         /* �T��ֳt���� */

#define BRD_LINK_TARGET(x)	((x)->postexpire)
#define GROUPOP()               (currmode & MODE_GROUPOP)

#ifdef CHESSCOUNTRY
#define CHESSCODE_NONE   0
#define CHESSCODE_FIVE   1
#define CHESSCODE_CCHESS 2
#define CHESSCODE_GO     3
#define CHESSCODE_MAX    3
#endif /* defined(CHESSCOUNTRY) */


#define TTLEN      64             /* Length of title */
#define FNLEN      33             /* Length of filename  */

typedef struct fileheader_t {
    char    filename[FNLEN];         /* M.9876543210.A */
    char    recommend;               /* important level */
    char    owner[IDLEN + 2];        /* uid[.] */
    char    date[6];                 /* [02/02] or space(5) */
    char    title[TTLEN + 1];
    union {
	/* TODO: MOVE money to outside multi!!!!!! */
	int money;
	int anon_uid;
	/* different order to match alignment */
	struct {
	    unsigned char posts;    /* money & 0xff */
	    unsigned char logins;   /* money & 0xff00 */
	    unsigned char regtime;   /* money & 0xff0000 */
	    unsigned char pad[1];   /* money & 0xffff0000 */
	} vote_limits;
	struct {
	    /* is this ordering correct? */
	    unsigned int ref:31;
	    unsigned int flag:1;
	} refer;
    }	    multi;		    /* rocker: if bit32 on ==> reference */
    /* XXX dirty, split into flag and money if money of each file is less than 16bit? */
    unsigned char   filemode;        /* must be last field @ boards.c */
} fileheader_t;

#define FILE_LOCAL      0x1     /* local saved */
#define FILE_READ       0x1     /* already read : mail only */
#define FILE_MARKED     0x2     /* opus: 0x8 */
#define FILE_DIGEST     0x4     /* digest */
#define FILE_BOTTOM     0x8     /* push_bottom */
#define FILE_SOLVED	0x10	/* problem solved, sysop/BM only */
#define FILE_HIDE       0x20    /* hide,	in announce */
#define FILE_BID        0x20    /* bid,		in non-announce */
#define FILE_BM         0x40    /* BM only,	in announce */
#define FILE_VOTE       0x40    /* for vote,	in non-announce */
#define FILE_ANONYMOUS  0x80   /* anonymous file */
/* TODO filemode is unsigned, IS THIS MULTI CORRECT? DANGEROUS!!! */
#define FILE_MULTI      0x100   /* multi send for mail */

#define STRLEN     80             /* Length of most string data */


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

/* values of msgque_t::msgmode */
#define MSGMODE_TALK      0
#define MSGMODE_WRITE     1
#ifdef PLAY_ANGEL
#define MSGMODE_FROMANGEL 2
#define MSGMODE_TOANGEL   3
#endif

typedef struct msgque_t {
    pid_t   pid;
    char    userid[IDLEN + 1];
    char    last_call_in[76];
    int     msgmode;
} msgque_t;

/* user data in shm */
/* use GAP to detect and avoid data overflow and overriding */
typedef struct userinfo_t {
    int     uid;                  /* Used to find user name in passwd file */
    pid_t   pid;                  /* kill() to notify user of talk request */
    int     sockaddr;             /* ... */

    /* user data */
    unsigned int    userlevel;
    char    userid[IDLEN + 1];
    char    nickname[24];
    char    from[27];               /* machine name the user called in from */
    int     from_alias;
    char    sex;
    unsigned char goodpost;
    unsigned char badpost;
    unsigned char goodsale;
    unsigned char badsale;
    unsigned char angel;

    /* friends */
    int     friendtotal;              /* �n�ͤ����cache �j�p */ 
    short   nFriends;                /* �U�� friend[] �u�Ψ�e�X��,
                                        �Ψ� bsearch */
    int     friend[MAX_FRIEND];
    char    gap_1[4];
    int     friend_online[MAX_FRIEND];/* point��u�W�n�� utmpshm����m */
			          /* �n�ͤ����cache �e���bit�O���A */
    char    gap_2[4];
    int     reject[MAX_REJECT];
    char    gap_3[4];

    /* messages */
    char    msgcount;
    msgque_t        msgs[MAX_MSGS];
    char    gap_4[sizeof(msgque_t)];   /* avoid msgs racing and overflow */

    /* user status */
    char    birth;                   /* �O�_�O�ͤ� Ptt*/
    unsigned char   active;         /* When allocated this field is true */
    unsigned char   invisible;      /* Used by cloaking function in Xyz menu */
    unsigned char   mode;           /* UL/DL, Talk Mode, Chat Mode, ... */
    unsigned char   pager;          /* pager toggle, YEA, or NA */
    time4_t lastact;               /* �W���ϥΪ̰ʪ��ɶ� */
    char    mailalert;
    char    mind[4];

    /* chatroom/talk/games calling */
    unsigned char   sig;            /* signal type */
    int     destuid;              /* talk uses this to identify who called */
    int     destuip;              /* dest index in utmpshm->uinfo[] */
    unsigned char   sockactive;     /* Used to coordinate talk requests */

    /* chat */
    unsigned char   in_chat;        /* for in_chat commands   */
    char    chatid[11];             /* chat id, if in chat mode */

    /* games */
    unsigned char   lockmode;       /* ���� multi_login �����F�� */
    char    turn;                    /* �C�������� */
    char    mateid[IDLEN + 1];       /* �C����⪺ id */
    char    color;                   /* �t�� �C�� */

    /* game record */
    unsigned short  int     five_win;
    unsigned short  int     five_lose;
    unsigned short  int     five_tie;
    unsigned short  int     chc_win;
    unsigned short  int     chc_lose;
    unsigned short  int     chc_tie;
    unsigned short  int     chess_elo_rating;

    /* misc */
    unsigned int    withme;
    unsigned int    brc_id;


#ifdef NOKILLWATERBALL
    time4_t wbtime;
#endif
} userinfo_t;

typedef struct water_t {
    pid_t   pid;
    char    userid[IDLEN + 1];
    int     top, count;
    msgque_t        msg[MAX_REVIEW];
    userinfo_t      *uin;     // Ptt:�o�i�H���Nalive
} water_t;

typedef struct {
    int row, col;
    int y, x;
    void *raw_memory;
} screen_backup_t;

typedef struct {
    fileheader_t    *header;
    char    mtitle[STRLEN];
    const char    *path;
    int     num, page, now, level;
} menu_t;

/* Used to pass commands to the readmenu.
 * direct mapping, indexed by ascii code. */
#define onekey_size ((int) 'z')
typedef int (* onekey_t)();

#define ANSILINELEN (511)                /* Maximum Screen width in chars */

/* anti_crosspost */
typedef struct crosspost_t {
    int     checksum[4]; /* 0 -> 'X' cross post  1-3 -> �ˬd�峹�� */
    short   times;       /* �ĴX�� */
    short   last_bid;    /* crossport to which board */
} crosspost_t;

#define SORT_BY_ID    0
#define SORT_BY_CLASS 1
#define SORT_BY_STAT  1
#define SORT_BY_IDLE  2
#define SORT_BY_FROM  3
#define SORT_BY_FIVE  4
#define SORT_BY_SEX   5

typedef struct keeploc_t {
    unsigned int hashkey;
    int     top_ln;
    int     crs_ln;
} keeploc_t;

#define VALID_USHM_ENTRY(X) ((X) >= 0 && (X) < USHM_SIZE)
#define USHM_SIZE       ((MAX_ACTIVE)*10/9)
/* USHM_SIZE �� MAX_ACTIVE �j�O���F�����ˬd�H�ƤW����, �S�P�ɽĶi��
 * �|�y���� shm �Ŧ쪺�L�a�j��. 
 * �S, �] USHM ���� hash, �Ŷ��y�j�ɮĲv���n. */

/* MAX_BMs is dirty hardcode 4 in mbbsd/cache.c:is_BM_cache() */
#define MAX_BMs         4                 /* for BMcache, �@�ӬݪO�̦h�X�O�D */

#define SHM_VERSION 2582
typedef struct {
    int     version;
    /* uhash */
    /* uhash is a userid->uid hash table -- jochang */
    char    userid[MAX_USERS][IDLEN + 1];
    char    gap_1[IDLEN+1];
    int     next_in_hash[MAX_USERS];
    char    gap_2[sizeof(int)];
    int     money[MAX_USERS];
    char    gap_3[sizeof(int)];
#ifdef USE_COOLDOWN
    time4_t cooldowntime[MAX_USERS];
#endif
    char    gap_4[sizeof(int)];
    int     hash_head[1 << HASH_BITS];
    char    gap_5[sizeof(int)];
    int     number;				/* # of users total */
    int     loaded;				/* .PASSWD has been loaded? */

    /* utmpshm */
    userinfo_t      uinfo[USHM_SIZE];
    char    gap_6[sizeof(userinfo_t)];
    int             sorted[2][8][USHM_SIZE];
                    /* �Ĥ@��double buffer ��currsorted���V�ثe�ϥΪ�
		       �ĤG��sort type */
    char    gap_7[sizeof(int)];
    int     currsorted;
    time4_t UTMPuptime;
    int     UTMPnumber;
    char    UTMPneedsort;
    char    UTMPbusystate;

    /* brdshm */
    char    gap_8[sizeof(int)];
    int     BMcache[MAX_BOARD][MAX_BMs];
    char    gap_9[sizeof(int)];
    boardheader_t   bcache[MAX_BOARD];
    char    gap_10[sizeof(int)];
    int     bsorted[2][MAX_BOARD]; /* 0: by name 1: by class */ /* ���Y�s���O bid-1 */
    char    gap_11[sizeof(int)];
#if HOTBOARDCACHE
    unsigned char    nHOTs;
    int              HBcache[HOTBOARDCACHE];
#endif
    char    gap_12[sizeof(int)];
    time4_t busystate_b[MAX_BOARD];
    char    gap_13[sizeof(int)];
    int     total[MAX_BOARD];
    char    gap_14[sizeof(int)];
    unsigned char  n_bottom[MAX_BOARD]; /* number of bottom */
    char    gap_15[sizeof(int)];
    int     hbfl[MAX_BOARD][MAX_FRIEND + 1]; /* hidden board friend list, 0: load time, 1-MAX_FRIEND: uid */
    char    gap_16[sizeof(int)];
    time4_t lastposttime[MAX_BOARD];
    char    gap_17[sizeof(int)];
    time4_t Buptime;
    time4_t Btouchtime;
    int     Bnumber;
    int     Bbusystate;
    time4_t close_vote_time;

    /* pttcache */
    char    notes[MAX_MOVIE][200*11];
    char    gap_18[sizeof(int)];
    char    today_is[20];
    int     n_notes[MAX_MOVIE_SECTION];      /* �@�`�����X�� �ݪO */
    char    gap_19[sizeof(int)];
    int     next_refresh[MAX_MOVIE_SECTION]; /* �U�@���nrefresh�� �ݪO */
    char    gap_20[sizeof(int)];
    msgque_t loginmsg;  /* �i�����y */
    int     max_film;
    int     max_history;
    time4_t Puptime;
    time4_t Ptouchtime;
    int     Pbusystate;

    /* SHM ���������ܼ�, �i�� shmctl �]�w�����. �ѰʺA�վ�δ��ըϥ� */
    union {
	int     v[512];
	struct {
	    int     dymaxactive;  /* �ʺA�]�w�̤j�H�ƤW��     */
	    int     toomanyusers; /* �W�L�H�ƤW�������i���Ӽ� */
	    int     noonlineuser; /* ���W�ϥΪ̤����G�����   */
#ifdef OUTTA_TIMER
	    time4_t now;
#endif
	    int     nWelcomes;

	    /* �`�N, ���O�� align sizeof(int) */
	} e;
    } GV2;
    /* statistic */
    int     statistic[STAT_MAX];

    /* �G�m fromcache */
    unsigned int    home_ip[MAX_FROM];
    unsigned int    home_mask[MAX_FROM];
    char            home_desc[MAX_FROM][32];
    int        	    home_num;

    int     max_user;
    time4_t max_time;
    time4_t Fuptime;
    time4_t Ftouchtime;
    int     Fbusystate;

#ifdef I18N    
    /* i18n(internationlization) */
    char	*i18nstrptr[MAX_LANG][MAX_STRING];
    char	i18nstrbody[22 * MAX_LANG * MAX_STRING]; 
    	/* Based on the statistis, we found the lengh of one string
    	   is 22 bytes approximately.
    	*/
#endif    
} SHM_t;

#ifdef SHMALIGNEDSIZE
#   define SHMSIZE (sizeof(SHM_t)/(SHMALIGNEDSIZE)+1)*SHMALIGNEDSIZE
#else
#   define SHMSIZE (sizeof(SHM_t))
#endif

typedef struct {
    unsigned short oldlen;                /* previous line length */
    unsigned short len;                   /* current length of line */
    unsigned short smod;                  /* start of modified data */
    unsigned short emod;                  /* end of modified data */
    unsigned short sso;                   /* start stand out */
    unsigned short eso;                   /* end stand out */
    unsigned char mode;                  /* status of line, as far as update */
    /* data ���ݬO�̫�@�����, see screen_backup() */
    unsigned char data[ANSILINELEN + 1];
} screenline_t;

typedef struct {
    int r, c;
} rc_t;

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
    time4_t mailtime;
    char    sender[IDLEN + 1];
    char    username[24];
    char    rcpt[50];
    int     method;
    char    *niamod;
} MailQueue;

enum  {MQ_TEXT, MQ_UUENCODE, MQ_JUSTIFY};

typedef struct
{ 
    time4_t chrono;
    int     recno;
} TagItem;

/*
 * signature information
 */
typedef struct
{
    int total;		/* total sig files found */
    int max;		/* max number of available sig */
    int show_start;	/* by which page to start display */
    int show_max;	/* max covered range in last display */
} SigInfo;

/* type in gomo.c, structure passing through socket */
typedef struct {
    char            x;
    char            y;
} Horder_t;

#ifdef OUTTACACHE
typedef struct {
    int     index; // �b SHM->uinfo[index]
    int     uid;   // �קK�b cache server �W���P�B, �A�T�{��.
    int     friendstat;
    int     rfriendstat;
} ocfs_t;
#endif

// kcwu: for bug tracking
/* not used right now */
enum {
    F_VER,
    F_EDIT,
    F_MORE,
    F_WRITE_REQUEST,
    F_TALK_REQUEST,
    F_WATER,
    F_USERLIST,
    F_GEM,
};


#endif
