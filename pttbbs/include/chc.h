/* �H��
 * ���H��ԮɡA���賣�|���@�� chc_act_list �� linked-list�A�����ۨC�U�@�B
 * �ѡA�����N�o�ӰT���ᵹ���ǤH�]socket�^�C
 * �@�}�l��M�N�O���@�ӡA�C��@���[�Ѫ̥[�J�]�[�ѥi�H�q����ζ¤誺�[�I
 * �i��^�A�䤤�@�誺�U�Ѫ̪� act_list �N�|�h�@���O���A����N�|�N�U���Φ�
 * ����U���C�@�B�Ѷǵ� act_list ���Ҧ��ݭn���H�A�F���[�Ѫ��ĪG�C
 */

#define SIDE_ROW           7
#define TURN_ROW           8
#define STEP_ROW           9
#define TIME_ROW          10
#define WARN_ROW          12
#define MYWIN_ROW         17
#define HISWIN_ROW        18

#define CHC_VERSUS		1	/* ���H */
#define CHC_WATCH		2	/* �[�� */
#define CHC_PERSONAL		4	/* ���� */
#define CHC_WATCH_PERSONAL	8	/* �[�H���� */

#define CHE_O(c)          ((c) >> 3)
#define CHE_P(c)          ((c) & 7)
#define RTL(x)            (((x) - 3) >> 1)
#define dim(x)          (sizeof(x) / sizeof(x[0]))
#define LTR(x)            ((x) * 2 + 3)
#define CHE(a, b)         ((a) | ((b) << 3))

#define BLACK_COLOR       "\033[1;36m"
#define RED_COLOR         "\033[1;31m"
#define BLACK_REVERSE     "\033[1;37;46m"
#define RED_REVERSE       "\033[1;37;41m"
#define TURN_COLOR        "\033[1;33m"

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

