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

#define MSG_DEL_CANCEL  "�����R��"
#define MSG_SELECT_BOARD          "\033[7m�i ��ܬݪO �j\033[m\n" \
                                  "�п�J�ݪO�W��(���ť���۰ʷj�M)�G"
#define MSG_CLOAKED     "�����I�����ΤF�I�ݤ����... :P"
#define MSG_UNCLOAK     "�ڭn���{����F...."
#define MSG_BIG_BOY     "�ڬO�j�ӭ�! ^o^Y"
#define MSG_BIG_GIRL    "�@���j���k *^-^*"
#define MSG_LITTLE_BOY  "�ڬO���}��... =)"
#define MSG_LITTLE_GIRL "�̥i�R������! :>"
#define MSG_MAN         "����Ҩ��� (^O^)"
#define MSG_WOMAN       "�s�ڤp����!! /:>"
#define MSG_PLANT       "�Ӫ��]���ʧO��.."
#define MSG_MIME        "�q���`�S�ʧO�F�a"
#define MSG_PASSWD      "�п�J�z���K�X: "
#define MSG_POSTER      "\033[34;46m �峹��Ū "\
                        "\033[31;47m (y)\033[30m�^�H "\
			"\033[31m(=[]<>)\033[30m�����D�D "\
			"\033[31m(/?)\033[30m�j�M���D "\
			"\033[31m(aA)\033[30m�j�M�@�� "\
			"\033[31m(x)\033[30m��� "\
			"\033[31m(V)\033[30m�벼 \033[m"
#define MSG_SEPERATOR   "\
�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w"

#define MSG_CLOAKED     "�����I�����ΤF�I�ݤ����... :P"
#define MSG_UNCLOAK     "�ڭn���{����F...."

#define MSG_WORKING     "�B�z���A�еy��..."

#define MSG_CANCEL      "�����C"
#define MSG_USR_LEFT    "User �w�g���}�F"
#define MSG_NOBODY      "�ثe�L�H�W�u"

#define MSG_DEL_OK      "�R������"
#define MSG_DEL_CANCEL  "�����R��"
#define MSG_DEL_ERROR   "�R�����~"
#define MSG_DEL_NY      "�нT�w�R��(Y/N)?[N] "

#define MSG_FWD_OK      "�峹��H����!"
#define MSG_FWD_ERR1    "��H���~: system error"
#define MSG_FWD_ERR2    "��H���~: address error"

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
#define MSG_MAILER      \
"\033[34;46m �E������ \033[31;47m(R)\033[30m�^�H\033[31m(x)\033[30m��F\
\033[31m(y)\033[30m�s�զ^�H\033[31m(D)\033[30m�R��\
\033[31m(c)\033[30m���J�H��\033[31m(z)\033[30m�H�� \033[31m[G]\033[30m�~��?\033[0m"
#define MSG_SHORTULIST  "\033[7m\
�ϥΪ̥N��    �ثe���A   �x�ϥΪ̥N��    �ثe���A   �x�ϥΪ̥N��    �ثe���A  \033[0m"


#define STR_AUTHOR1     "�@��:"
#define STR_AUTHOR2     "�o�H�H:"
#define STR_POST1       "�ݪO:"
#define STR_POST2       "����:"

/* Flags to getdata input function */
#define NOECHO       0
#define DOECHO       1
#define LCECHO       2

#define YEA  1		       /* Booleans  (Yep, for true and false) */
#define NA   0

/* �n�����Y */
#define IRH 1   /* I reject him.             */
#define HRM 2   /* He reject me.             */
#define IBH 4   /* I am board friend of him. */
#define IFH 8   /* I am friend of him.       */
#define HFM 16  /* He is friend of me.       */
#define ST_FRIEND  (IBH | IFH | HFM)
#define ST_REJECT  (IRH | HRM)       

/* ��L�]�w */
#define KEY_TAB         9
#define KEY_ESC         27
#define KEY_UP          0x0101
#define KEY_DOWN        0x0102
#define KEY_RIGHT       0x0103
#define KEY_LEFT        0x0104
#define KEY_HOME        0x0201
#define KEY_INS         0x0202
#define KEY_DEL         0x0203
#define KEY_END         0x0204
#define KEY_PGUP        0x0205
#define KEY_PGDN        0x0206

#define QCAST           int (*)(const void *, const void *)
#define Ctrl(c)         (c & 037)
#define chartoupper(c)  ((c >= 'a' && c <= 'z') ? c+'A'-'a' : c)

#define LEN_AUTHOR1     5
#define LEN_AUTHOR2     7

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

#define CHE_O(c)          ((c) >> 3)
#define CHE_P(c)          ((c) & 7)
#define RTL(x)            (((x) - 3) >> 1)
#define dim(x)          (sizeof(x) / sizeof(x[0]))
#define LTR(x)            ((x) * 2 + 3)
#define CHE(a, b)         ((a) | ((b) << 3))

#define MAX_MODES 127

#ifndef MIN
#define	MIN(a,b)	(((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define	MAX(a,b)	(((a)>(b))?(a):(b))
#endif

#define char_lower(c)  ((c >= 'A' && c <= 'Z') ? c|32 : c)

#define STR_CURSOR      "��"
#define STR_UNCUR       "  "
#endif
