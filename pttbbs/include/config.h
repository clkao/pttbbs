/* $Id$ */
#ifndef INCLUDE_CONFIG_H
#define INCLUDE_CONFIG_H

#include <syslog.h>
#include "../pttbbs.conf"

#define BBSPROG         BBSHOME "/bin/mbbsd"         /* �D�{�� */
#define BAN_FILE        "BAN"                        /* �����q�i�� */
#define LOAD_FILE       "/proc/loadavg"              /* for Linux */

/* �t�ΦW(�l���)�A��ĳ�O�W�L 3 �Ӧr���C �Ԩ� sample/pttbbs.conf */
#ifndef BBSMNAME
#define BBSMNAME	"Ptt"
#endif

/* �t�ΦW(����)�A��ĳ��n 4 �Ӧr���C �Ԩ� sample/pttbbs.conf */
#ifndef BBSMNAME2
#define BBSMNAME2	"��tt"
#endif

/* �����W�A��ĳ��n 3 �Ӧr���C �Ԩ� sample/pttbbs.conf */
#ifndef MONEYNAME
#define MONEYNAME	"Ptt"
#endif

/* AID ��ܪ����x�W�١C �Y IP �Ӫ��Хt��w�q�C */
#ifndef AID_HOSTNAME
#define AID_HOSTNAME	MYHOSTNAME
#endif

/* �D�D�t�� */

#ifndef TITLE_COLOR 
#define TITLE_COLOR       ANSI_COLOR(0;1;37;46)	/* �D�e���W����D�C */
#endif

#ifndef HLP_CATEGORY_COLOR
#define HLP_CATEGORY_COLOR	ANSI_COLOR(0;1;32)  /* ������椺������ */
#endif
#ifndef HLP_DESCRIPTION_COLOR
#define HLP_DESCRIPTION_COLOR	ANSI_COLOR(0)	    /* ������椺������ */
#endif
#ifndef HLP_KEYLIST_COLOR
#define HLP_KEYLIST_COLOR	ANSI_COLOR(0;1;36)  /* ������椺���䶵 */
#endif

/* �t�αb�������]�w */ 
#ifndef BBSUSER
#define BBSUSER "bbs"
#endif

#ifndef BBSUID
#define BBSUID (9999)
#endif

#ifndef BBSGID
#define BBSGID (99)
#endif

/* Default Board Names */
#ifndef BN_BUGREPORT
#define BN_BUGREPORT "SYSOP"
#endif

#ifndef BN_SYSOP
#define BN_SYSOP "SYSOP"
#endif

#ifndef BN_ID_PROBLEM
#define BN_ID_PROBLEM "SYSOP"
#endif

#ifndef BN_LAW
#define BN_LAW  BBSMNAME "Law"
#endif

#ifndef BN_NEWBIE
#define BN_NEWBIE BBSMNAME "Newhand"
#endif

#ifndef BN_TEST
#define BN_TEST "Test"
#endif

#ifndef BN_NOTE
#define BN_NOTE "Note"
#endif

#ifndef BN_SECURITY
#define BN_SECURITY "Security"
#endif

#ifndef BN_RECORD
#define BN_RECORD "Record"
#endif

#ifndef BN_FOREIGN
#define BN_FOREIGN BBSMNAME "Foreign"
#endif

#ifndef BN_DELETED
#define BN_DELETED "deleted"
#endif

#ifndef BN_JUNK
#define BN_JUNK "junk"
#endif 

#ifndef BN_UNANONYMOUS
#define BN_UNANONYMOUS "UnAnonymous"
#endif

#ifndef RECYCLE_BIN_NAME
#define RECYCLE_BIN_NAME "�귽�^����" // "�U����"
#endif

#ifndef RECYCLE_BIN_OWNER
#define RECYCLE_BIN_OWNER "[" RECYCLE_BIN_NAME "]"
#endif

#ifndef TIME_CAPSULE_NAME
#define TIME_CAPSULE_NAME "Magical Index" // "Time Capsule"
#endif

/* Environment */
#ifndef RELAY_SERVER_IP                     /* �H���~�H�� mail server */
#define RELAY_SERVER_IP    "127.0.0.1"
#endif

#ifndef MAX_USERS                           /* �̰����U�H�� */
#define MAX_USERS          (150000)
#endif

#ifndef MAX_ACTIVE
#define MAX_ACTIVE        (1024)         /* �̦h�P�ɤW���H�� */
#endif

#ifndef MAX_GUEST
#define MAX_GUEST         (100)          /* �̦h guest �W���H�� */
#endif

#ifndef MAX_CPULOAD
#define MAX_CPULOAD       (70)           /* CPU �̰�load */
#endif

#ifndef MAX_LANG
#define MAX_LANG          (1)			 /* �̦h�ϥλy�� */
#endif

#ifndef MAX_STRING
#define MAX_STRING        (8000)         /* �t�γ̦h�ϥΦr�� */
#endif

#ifndef MAX_POST_MONEY                      /* �o��峹�Z�O���W�� */
#define MAX_POST_MONEY     (100)
#endif

#ifndef MAX_CHICKEN_MONEY                   /* �i������Q�W�� */
#define MAX_CHICKEN_MONEY  (100)
#endif

#ifndef MAX_GUEST_LIFE                      /* �̪����{�ҨϥΪ̫O�d�ɶ�(��) */
#define MAX_GUEST_LIFE     (3 * 24 * 60 * 60)
#endif

#ifndef MAX_EDIT_LINE
#define MAX_EDIT_LINE 2048                  /* �峹�̪��s����� */
#endif 

#ifndef MAX_EDIT_LINE_LARGE
#define MAX_EDIT_LINE_LARGE (32000)
#endif

#ifndef MAX_LIFE                            /* �̪��ϥΪ̫O�d�ɶ�(��) */
#define MAX_LIFE           (120 * 24 * 60 * 60)
#endif

#ifndef KEEP_DAYS_REGGED                    /* �w���U�ϥΪ̫O�d�h�[ */
#define KEEP_DAYS_REGGED    (120)
#endif

#ifndef KEEP_DAYS_UNREGGED                    /* �����U�ϥΪ̫O�d�h�[ */
#define KEEP_DAYS_UNREGGED    (15)
#endif

#ifndef MAX_FROM
#define MAX_FROM           (300)            /* �̦h�G�m�� */
#endif

#ifndef DEBUGSLEEP_SECONDS
#define DEBUGSLEEP_SECONDS (3600)	    /* debug ���ݮɶ� */
#endif

#ifndef THREAD_SEARCH_RANGE
#define THREAD_SEARCH_RANGE (500)
#endif

#ifndef MEM_CHECK
#define MEM_CHECK 0x98761234
#endif

#ifndef FOREIGN_REG_DAY                     /* �~�y�ϥΪ̸եΤ���W�� */
#define FOREIGN_REG_DAY   30
#endif

#ifndef FORCE_PROCESS_REGISTER_FORM
#define FORCE_PROCESS_REGISTER_FORM 0
#endif

#ifndef SYSLOG_FACILITY
#define SYSLOG_FACILITY   LOG_LOCAL0
#endif

#ifndef TAR_PATH
#define TAR_PATH "tar"
#endif

#ifndef MUTT_PATH
#define MUTT_PATH "mutt"
#endif

#ifndef HBFLexpire
#define HBFLexpire        (432000)       /* 5 days */
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN        (256)
#endif

#ifndef PATHLEN
#define PATHLEN           (256)
#endif

#ifndef MAX_BOARD
#define MAX_BOARD         (8192)         /* �̤j�}�O�Ӽ� */
#endif

#ifndef MAX_EXKEEPMAIL
#define MAX_EXKEEPMAIL    (1000)         /* �̦h�H�c�[�j�h�֫� */
#endif

#ifndef OVERLOADBLOCKFDS
#define OVERLOADBLOCKFDS  (0)            /* �W����|�O�d�o��h�� fd */
#endif

#ifndef HOTBOARDCACHE
#define HOTBOARDCACHE     (0)            /* �����ݪO�֨� */
#endif

#ifndef INNTIMEZONE
#define INNTIMEZONE       "+0000 (UTC)"
#endif

#ifndef ADD_EXMAILBOX
#define ADD_EXMAILBOX     0              /* �ذe�H�c */
#endif

#ifndef HASH_BITS
#define HASH_BITS         16             /* userid->uid hashing bits */
#endif

#ifndef VICE_MIN
#define VICE_MIN	(1)	    /* �̤p�o�����B */
#endif // VICE_MIN

#ifndef BADPOST_CLEAR_DURATION
#define BADPOST_CLEAR_DURATION	(180)	// ���H��ɶ�����
#endif

/* (deprecated) more.c ���峹���ƤW��(lines/22), +4 for safe */
#define MAX_PAGES         (MAX_EDIT_LINE / 22 + 4)

/* �H�U�٥���z */
#define MAX_FRIEND        (256)          /* ���J cache ���̦h�B�ͼƥ� */
#define MAX_REJECT        (32)           /* ���J cache ���̦h�a�H�ƥ� */
#define MAX_MSGS          (10)           /* ���y(���T)�ԭ@�W�� */
#define MAX_ADBANNER      (500)          /* �̦h�ʺA�ݪO�� */
#define MAX_ADBANNER_SECTION (10)	 /* �̦h�ʺA�ݪO���O */
#define MAX_ADBANNER_HEIGHT  (11)	 /* �̤j�ʺA�ݪO���e���� */
#define MAX_ITEMS         (1000)         /* �@�ӥؿ��̦h���X�� */
#define MAX_HISTORY       (12)           /* �ʺA�ݪO�O�� 12 �����v�O�� */
#define MAX_CROSSNUM      (9) 	         /* �̦hcrosspost���� */
#define MAX_QUERYLINES    (16)           /* ��� Query/Plan �T���̤j��� */
#define MAX_LOGIN_INFO    (128)          /* �̦h�W�u�q���H�� */
#define MAX_POST_INFO     (32)           /* �̦h�s�峹�q���H�� */
#define MAX_NAMELIST      (128)          /* �̦h��L�S�O�W��H�� */
#define MAX_KEEPMAIL      (200)          /* �̦h�O�d�X�� MAIL�H */
#define MAX_NOTE          (20)           /* �̦h�O�d�X�g�d���H */
#define MAX_SIGLINES      (6)            /* ñ�W�ɤޤJ�̤j��� */
#define MAX_CROSSNUM      (9) 	         /* �̦hcrosspost���� */
#define MAX_REVIEW        (7)		 /* �̦h���y�^�U */
#define NUMVIEWFILE       (15)           /* �i���e���̦h�� */
#define MAX_SWAPUSED      (0.7)          /* SWAP�̰��ϥβv */
#define LOGINATTEMPTS     (3)            /* �̤j�i�����~���� */
#define WHERE                            /* �O�_���G�m�\�� */
#undef  LOG_BOARD  			 /* �ݪO�O�_log */


#define LOGINASNEW              /* �ĥΤW���ӽбb����� */
#define NO_WATER_POST           /* ����BlahBlah����� */
#define USE_BSMTP               /* �ϥ�opus��BSMTP �H���H? */
#define HAVE_ANONYMOUS          /* ���� Anonymous �O */
#define INTERNET_EMAIL          /* �䴩 InterNet Email �\��(�t Forward) */
#define HAVE_ORIGIN             /* ��� author �Ӧۦ�B */
#undef  HAVE_INFO               /* ��ܵ{���������� */
#undef  HAVE_LICENSE            /* ��� GNU ���v�e�� */
#undef  HAVE_REPORT             /* �t�ΰl�ܳ��i */
#undef  NEWUSER_LIMIT           /* �s��W�����T�ѭ��� */
#undef  HAVE_X_BOARDS

#define SHOWUID                 /* �ݨ��ϥΪ� UID */
#define SHOWBOARD               /* �ݨ��ϥΪ̬ݪO */
#define SHOWPID                 /* �ݨ��ϥΪ� PID */

#define DOTIMEOUT
#ifdef  DOTIMEOUT
#define IDLE_TIMEOUT    (43200) /* �@�뱡�p�� timeout (12hr) */
#define SHOW_IDLE_TIME          /* ��ܶ��m�ɶ� */
#endif

#define SEM_ENTER      -1      /* enter semaphore */
#define SEM_LEAVE      1       /* leave semaphore */
#define SEM_RESET      0       /* reset semaphore */

#define SHM_KEY         1228

#define PASSWDSEM_KEY   2010	/* semaphore key */

// #define NEW_CHATPORT    3838
#define	XCHATD_ADDR	":3838"

#define MAX_ROOM         16              /* �̦h���X���]�[�H */

#define EXIT_LOGOUT     0
#define EXIT_LOSTCONN   -1
#define EXIT_CLIERROR   -2
#define EXIT_TIMEDOUT   -3
#define EXIT_KICK       -4

#define CHAT_LOGIN_OK       "OK"
#define CHAT_LOGIN_EXISTS   "EX"
#define CHAT_LOGIN_INVALID  "IN"
#define CHAT_LOGIN_BOGUS    "BG"
#define BADCIDCHARS " *"        /* Chat Room ���T�Ω� nick ���r�� */

#define BN_ALLPOST "ALLPOST"
#define BN_ALLHIDPOST "ALLHIDPOST"

#define MAXTAGS	255
#define WRAPMARGIN (511)

#ifdef USE_MASKED_FROMHOST
#define FROMHOST    fromhost_masked
#else
#define FROMHOST    fromhost
#endif

#ifndef DEFAULT_FOLDER_CREATE_PERM
#define DEFAULT_FOLDER_CREATE_PERM (0755)
#endif
#ifndef DEFAULT_FILE_CREATE_PERM
#define DEFAULT_FILE_CREATE_PERM   (0644)
#endif

#endif
