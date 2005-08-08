/* �H��
 * ���H��ԮɡA���賣�|���@�� chc_act_list �� linked-list�A�����ۨC�U�@�B
 * �ѡA�����N�o�ӰT���ᵹ���ǤH�]socket�^�C
 * �@�}�l��M�N�O���@�ӡA�C��@���[�Ѫ̥[�J�]�[�ѥi�H�q����ζ¤誺�[�I
 * �i��^�A�䤤�@�誺�U�Ѫ̪� act_list �N�|�h�@���O���A����N�|�N�U���Φ�
 * ����U���C�@�B�Ѷǵ� act_list ���Ҧ��ݭn���H�A�F���[�Ѫ��ĪG�C
 */

#define SIDE_ROW           7
#define REAL_TURN_ROW      8
#define STEP_ROW           9
#define REAL_TIME_ROW1    10
#define REAL_TIME_ROW2    11
#define REAL_WARN_ROW     13
#define MYWIN_ROW         17
#define HISWIN_ROW        18

#define PHOTO_TURN_ROW    19
#define PHOTO_TIME_ROW1   20
#define PHOTO_TIME_ROW2   21
#define PHOTO_WARN_ROW    22

/* virtual lines */
#define TURN_ROW         128
#define TIME_ROW         129
#define WARN_ROW         130

#define CHC_VERSUS		1	/* ���H */
#define CHC_WATCH		2	/* �[�� */
#define CHC_PERSONAL		4	/* ���� */
#define CHC_WATCH_PERSONAL	8	/* �[�H���� */

#define CHE_O(c)          ((c) >> 3)
#define CHE_P(c)          ((c) & 7)
#define RTL(myturn, x)    ((myturn)==BLK?BRD_ROW-1-((x)-3)/2:((x)-3)/2)
#define CTL(myturn, x)    ((myturn)==BLK?BRD_COL-1-(x):(x))
#define dim(x)          (sizeof(x) / sizeof(x[0]))
#define LTR(myturn, x)    ((((myturn)==BLK?BRD_ROW-1-(x):(x)) * 2) + 3)
#define CHE(a, b)         ((a) | ((b) << 3))

#define BLACK_COLOR       ANSI_COLOR(1;36)
#define RED_COLOR         ANSI_COLOR(1;31)
#define BLACK_REVERSE     ANSI_COLOR(1;37;46)
#define RED_REVERSE       ANSI_COLOR(1;37;41)
#define TURN_COLOR        ANSI_COLOR(1;33)

typedef struct chcusr_t{
    char    userid[IDLEN + 1];
    int	    win;
    int     lose;
    int     tie;
    unsigned short rating;
    unsigned short orig_rating; // ��l rating, �]���C���}�l�����@��, rating �ȴN�]���F
} chcusr_t;

#define CHC_ACT_BOARD	0x1	/* set if transfered board to this sock */

typedef struct chc_act_list{
    int     sock;
    struct chc_act_list *next;
} chc_act_list;

#define BRD_ROW           10
#define BRD_COL           9

typedef int board_t[BRD_ROW][BRD_COL];
typedef int (*board_p)[BRD_COL];

