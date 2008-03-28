#define STRLEN   80             /* Length of most string data */
#define BRC_STRLEN 15           /* Length of boardname */
#define BTLEN    48             /* Length of board title */
#define NAMELEN  40             /* Length of username/realname */
//#define FNLEN    33             /* Length of filename  */
//                                /* XXX Ptt ���o�̦�bug*/
#define IDLEN    12             /* Length of bid/uid */
#define PASSLEN  14             /* Length of encrypted passwd field */
#define REGLEN   38             /* Length of registration data */



typedef unsigned char	uschar;	/* length = 1 */
typedef unsigned short	ushort;	/* length = 2 */
typedef unsigned long	uslong;	/* length = 4 */
typedef unsigned int	usint;	/* length = 4 */

/* ----------------------------------------------------- */
/* .PASSWDS struct : 512 bytes                           */
/* ----------------------------------------------------- */
struct sobuserec
{
  char userid[IDLEN + 1];         /* �ϥΪ̦W��  13 bytes */
  char realname[20];              /* �u��m�W    20 bytes */
  char username[24];              /* �ʺ�        24 bytes */
  char passwd[PASSLEN];           /* �K�X        14 bytes */
  uschar uflag;                   /* �ϥΪ̿ﶵ   1 byte  */
  usint userlevel;                /* �ϥΪ��v��   4 bytes */
  ushort numlogins;               /* �W������     2 bytes */
  ushort numposts;                /* POST����     2 bytes */
  time4_t firstlogin;              /* ���U�ɶ�     4 bytes */
  time4_t lastlogin;               /* �e���W��     4 bytes */
  char lasthost[24];              /* �W���a�I    24 bytes */
  char vhost[24];                 /* �������}    24 bytes */
  char email[50];                 /* E-MAIL      50 bytes */
  char address[50];               /* �a�}        50 bytes */
  char justify[REGLEN + 1];       /* ���U���    39 bytes */
  uschar month;                   /* �X�ͤ��     1 byte  */
  uschar day;                     /* �X�ͤ��     1 byte  */
  uschar year;                    /* �X�ͦ~��     1 byte  */
  uschar sex;                     /* �ʧO         1 byte  */
  uschar state;                   /* ���A??       1 byte  */
  usint habit;                    /* �ߦn�]�w     4 bytes */
  uschar pager;                   /* �߱��C��     1 bytes */
  uschar invisible;               /* �����Ҧ�     1 bytes */
  usint exmailbox;                /* �H�c�ʼ�     4 bytes */
  usint exmailboxk;               /* �H�cK��      4 bytes */
  usint toquery;                  /* �n�_��       4 bytes */
  usint bequery;                  /* �H���       4 bytes */
  char toqid[IDLEN + 1];          /* �e���d��    13 bytes */
  char beqid[IDLEN + 1];          /* �e���Q�֬d  13 bytes */
  uslong totaltime;		  /* �W�u�`�ɼ�   8 bytes */
  usint sendmsg;                  /* �o�T������   4 bytes */
  usint receivemsg;               /* ���T������   4 bytes */
  usint goldmoney;                /* ���Ъ���     8 bytes */
  usint silvermoney;              /* �ȹ�         8 bytes */
  usint exp;                      /* �g���       8 bytes */
  time4_t dtime;                   /* �s�ڮɶ�     4 bytes */
  int scoretimes;                 /* ��������     4 bytes */
  uschar rtimes;                  /* ����U�榸�� 1 bytes */
  int award;                      /* ���g�P�_     4 bytes */
  int pagermode;                  /* �I�s������   4 bytes */
  char pagernum[7];               /* �I�s�����X   7 bytes */
  char feeling[5];                /* �߱�����     5 bytes */
  char title[20];                 /* �ٿ�(�ʸ�)  20 bytes */
  usint five_win;
  usint five_lost;
  usint five_draw;
  char pad[91];                  /* �ŵ۶񺡦�512��      */
};

typedef struct sobuserec sobuserec;

