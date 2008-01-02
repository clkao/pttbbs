/* $Id$ */
#ifndef INCLUDE_COMMON_H
#define INCLUDE_COMMON_H

#define STR_GUEST                 "guest"
#define DEFAULT_BOARD   str_sysop

#define FN_PASSWD       BBSHOME "/.PASSWDS"      /* User records */
#define FN_USSONG       "ussong"        /* �I�q�έp */
#define FN_POST_NOTE    "post.note"     /* po�峹�Ƨѿ� */
#define FN_POST_BID     "post.bid"
#define FN_MONEY        "etc/money"
#define FN_OVERRIDES    "overrides"
#define FN_REJECT       "reject"
#define FN_WATER        "water"
#define FN_CANVOTE      "can_vote"
#define FN_VISABLE      "visable"
#define FN_USIES        "usies"         /* BBS log */
#define FN_DIR		".DIR"
#define FN_BOARD        ".BRD"       /* board list */
#define FN_USEBOARD     "usboard"       /* �ݪO�έp */
#define FN_NOTE_ANS     "note.ans"
#define FN_TOPSONG      "etc/topsong"
#define FN_OVERRIDES    "overrides"
#define FN_TICKET       "ticket"
#define FN_TICKET_END   "ticket.end"
#define FN_TICKET_LOCK  "ticket.end.lock"
#define FN_TICKET_ITEMS "ticket.items"
#define FN_TICKET_RECORD "ticket.data"
#define FN_TICKET_USER    "ticket.user"  
#define FN_TICKET_OUTCOME "ticket.outcome"
#define FN_TICKET_BRDLIST "boardlist"
#define FN_BRDLISTHELP	"etc/boardlist.help"
#define FN_BOARDHELP	"etc/board.help"

#define MSG_DEL_CANCEL  "�����R��"
#define MSG_BIG_BOY     "�ڬO�j�ӭ�! ^o^Y"
#define MSG_BIG_GIRL    "�@���j���k *^-^*"
#define MSG_LITTLE_BOY  "�ڬO���}��... =)"
#define MSG_LITTLE_GIRL "�̥i�R������! :>"
#define MSG_MAN         "����Ҩ��� (^O^)"
#define MSG_WOMAN       "�s�ڤp����!! /:>"
#define MSG_PLANT       "�Ӫ��]���ʧO��.."
#define MSG_MIME        "�q���`�S�ʧO�F�a"

#define MSG_CLOAKED     "�����I�����ΤF�I�ݤ����... :P"
#define MSG_UNCLOAK     "�ڭn���{����F...."

#define MSG_WORKING     "�B�z���A�еy��..."

#define MSG_CANCEL      "�����C"
#define MSG_USR_LEFT    "�ϥΪ̤w�g���}�F"
#define MSG_NOBODY      "�ثe�L�H�W�u"

#define MSG_DEL_OK      "�R������"
#define MSG_DEL_CANCEL  "�����R��"
#define MSG_DEL_ERROR   "�R�����~"
#define MSG_DEL_NY      "�нT�w�R��(Y/N)?[N] "

#define MSG_FWD_OK      "�峹��H����!"
#define MSG_FWD_ERR1    "��H���~: �t�ο��~ system error"
#define MSG_FWD_ERR2    "��H���~: ��}���~ address error"

#define MSG_SURE_NY     "�бz�T�w(Y/N)�H[N] "
#define MSG_SURE_YN     "�бz�T�w(Y/N)�H[Y] "

#define MSG_BID         "�п�J�ݪO�W�١G"
#define MSG_UID         "�п�J�ϥΪ̥N���G"
#define MSG_PASSWD      "�п�J�z���K�X: "

#define MSG_BIG_BOY     "�ڬO�j�ӭ�! ^o^Y"
#define MSG_BIG_GIRL    "�@���j���k *^-^*"
#define MSG_LITTLE_BOY  "�ڬO���}��... =)"
#define MSG_LITTLE_GIRL "�̥i�R������! :>"
#define MSG_MAN         "����Ҩ��� (^O^)"
#define MSG_WOMAN       "�s�ڤp����!! /:>"
#define MSG_PLANT       "�Ӫ��]���ʧO��.."
#define MSG_MIME        "�q���`�S�ʧO�F�a"

#define ERR_BOARD_OPEN  ".BOARD �}�ҿ��~"
#define ERR_BOARD_UPDATE        ".BOARD ��s���~"
#define ERR_PASSWD_OPEN ".PASSWDS �}�ҿ��~"

#define ERR_BID         "�A�d���F�աI�S���o�ӪO��I"
#define ERR_UID         "�o�̨S���o�ӤH�աI"
#define ERR_PASSWD      "�K�X�����I�A���S���_�ΤH�a���W�r�ڡH"
#define ERR_FILENAME    "�ɦW���X�k�I"

#define STR_AUTHOR1     "�@��:"
#define STR_AUTHOR2     "�o�H�H:"
#define STR_POST1       "�ݪO:"
#define STR_POST2       "����:"

/* AIDS */
#define AID_DISPLAYNAME	"�峹�N�X(AID)"
/* end of AIDS */

/* LONG MESSAGES */
#define MSG_SELECT_BOARD ANSI_COLOR(7) "�i ��ܬݪO �j" ANSI_RESET "\n" \
			"�п�J�ݪO�W��(���ť���۰ʷj�M)�G"

#define MSG_POSTER_LEN (78)
#define MSG_POSTER      ANSI_COLOR(34;46) " �峹��Ū "\
	ANSI_COLOR(31;47) " (y)"	ANSI_COLOR(30) "�^�H"\
	ANSI_COLOR(31) "(X%)" 	ANSI_COLOR(30) "����"\
	ANSI_COLOR(31) "(x)" 	ANSI_COLOR(30) "��� "\
	ANSI_COLOR(31) "(=[]<>)" 	ANSI_COLOR(30) "�����D�D "\
	ANSI_COLOR(31) "(/?a)" 	ANSI_COLOR(30) "�j�M���D/�@��  "\
	ANSI_COLOR(31) "(V)" 	ANSI_COLOR(30) "�벼 "\
	""
#define MSG_MAILER_LEN (78)
#define MSG_MAILER      \
	ANSI_COLOR(34;46) " �E������ " \
	ANSI_COLOR(31;47) " (R)"	ANSI_COLOR(30) "�^�H" \
	ANSI_COLOR(31) "(x)"	ANSI_COLOR(30) "��H" \
	ANSI_COLOR(31) "(y)" 	ANSI_COLOR(30) "�^�s�իH " \
	ANSI_COLOR(31) "(D)" 	ANSI_COLOR(30) "�R�� " \
	ANSI_COLOR(31) "(c)" 	ANSI_COLOR(30) "���J�H��" \
	ANSI_COLOR(31) "(z)" 	ANSI_COLOR(30) "�H�� " \
	ANSI_COLOR(31) "��[q]" 	ANSI_COLOR(30) "���} " \
	""

#define MSG_SEPERATOR   "\
�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w"

/* Flags to getdata input function */
#define NOECHO       0
#define DOECHO       1
#define LCECHO       2
#define NUMECHO	     4

#define YEA  1		       /* Booleans  (Yep, for true and false) */
#define NA   0

#define EQUSTR 0	/* for strcmp */

/* �n�����Y */
#define IRH 1   /* I reject him.		*/
#define HRM 2   /* He reject me.		*/
#define IBH 4   /* I am board friend of him.	*/
#define IFH 8   /* He is one of my friends.	*/
#define HFM 16  /* I am one of his friends.	*/
#define ST_FRIEND  (IBH | IFH | HFM)
#define ST_REJECT  (IRH | HRM)       

/* ��L�]�w */
#define KEY_TAB         9
#define KEY_ESC         27
#define KEY_UP          0x0101
#define KEY_DOWN        0x0102
#define KEY_RIGHT       0x0103
#define KEY_LEFT        0x0104
#define KEY_STAB        0x0105	/* shift-tab */
#define KEY_HOME        0x0201
#define KEY_INS         0x0202
#define KEY_DEL         0x0203
#define KEY_END         0x0204
#define KEY_PGUP        0x0205
#define KEY_PGDN        0x0206
#define KEY_F1		0x0301
#define KEY_F2		0x0302
#define KEY_F3		0x0303
#define KEY_F4		0x0304
#define KEY_F5		0x0305
#define KEY_F6		0x0306
#define KEY_F7		0x0307
#define KEY_F8		0x0308
#define KEY_F9		0x0309
#define KEY_F10		0x030A
#define KEY_F11		0x030B
#define KEY_F12		0x030C
#define KEY_UNKNOWN	0x0FFF	/* unknown sequence */

#define QCAST           int (*)(const void *, const void *)
#define Ctrl(c)         (c & 037)
#define chartoupper(c)  ((c >= 'a' && c <= 'z') ? c+'A'-'a' : c)

#define LEN_AUTHOR1     5
#define LEN_AUTHOR2     7

/* the title of article will be truncate to PROPER_TITLE_LEN */
#define PROPER_TITLE_LEN	42


/* ----------------------------------------------------- */
/* ���y�Ҧ� ��ɩw�q                                     */
/* ----------------------------------------------------- */
#define WB_OFO_USER_TOP		7
#define WB_OFO_USER_BOTTOM	11
#define WB_OFO_USER_HEIGHT	((WB_OFO_MSG_BOTTOM) - (WB_OFO_MSG_TOP) + 1)
#define WB_OFO_USER_LEFT	28
#define WB_OFO_MSG_TOP		15
#define WB_OFO_MSG_BOTTOM	22
#define WB_OFO_MSG_LEFT		4


/* ----------------------------------------------------- */
/* �s�զW��Ҧ�   Ptt                                    */
/* ----------------------------------------------------- */
#define FRIEND_OVERRIDE 0
#define FRIEND_REJECT   1
#define FRIEND_ALOHA    2
#define FRIEND_POST     3         
#define FRIEND_SPECIAL  4
#define FRIEND_CANVOTE  5
#define BOARD_WATER     6
#define BOARD_VISABLE   7 

#define LOCK_THIS   1    // lock�o�u���୫�ƪ�
#define LOCK_MULTI  2    // lock�Ҧ��u���୫�ƪ�   

#define I_TIMEOUT   (-2)       /* Used for the getchar routine select call */
#define I_OTHERDATA (-333)     /* interface, (-3) will conflict with chinese */

#define MAX_MODES	(127)
#define MAX_RECOMMENDS  (100)

#ifndef MIN
#define	MIN(a,b)	(((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define	MAX(a,b)	(((a)>(b))?(a):(b))
#endif

#define toSTR(x)	__toSTR(x)
#define __toSTR(x)	#x

#define char_lower(c)  ((c >= 'A' && c <= 'Z') ? c|32 : c)

#define STR_CURSOR      "��"
#define STR_UNCUR       "  "

#define NOTREPLYING     -1
#define REPLYING        0
#define RECVINREPLYING  1

/* ----------------------------------------------------- */
/* �s�边�ﶵ                                            */
/* ----------------------------------------------------- */
#define EDITFLAG_TEXTONLY   (0x00000001)
#define EDITFLAG_UPLOAD	    (0x00000002)
#define EDITFLAG_ALLOWLARGE (0x00000004)

/* ----------------------------------------------------- */
/* Grayout Levels                                        */
/* ----------------------------------------------------- */
#define GRAYOUT_COLORBOLD (-2)
#define GRAYOUT_BOLD (-1)
#define GRAYOUT_DARK (0)
#define GRAYOUT_NORM (1)
#define GRAYOUT_COLORNORM (+2)

/* ----------------------------------------------------- */
/* Macros                                                */
/* ----------------------------------------------------- */

#if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 96)
    #define __builtin_expect(exp,c) (exp)

#endif

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

#ifndef SHM_HUGETLB
#define SHM_HUGETLB	04000	/* segment is mapped via hugetlb */
#endif

#endif
