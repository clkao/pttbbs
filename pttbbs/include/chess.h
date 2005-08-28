/* $Id$ */

#ifndef INCLUDE_CHESS_H
#define INCLUDE_CHESS_H

#define CHESS_DRAWING_TIME_ROW   128
#define CHESS_DRAWING_TURN_ROW   129
#define CHESS_DRAWING_WARN_ROW   130
#define CHESS_DRAWING_STEP_ROW   131
#define CHESS_DRAWING_HELP_ROW   132

#define CHESS_PHOTO_LINE      15
#define CHESS_PHOTO_COLUMN    (256 + 25)

struct ChessBroadcastList;
struct ChessActions;
struct ChessConstants;

#define const
#define static
#define private

typedef struct {
    char    userid[IDLEN + 1];
    int	    win;
    int     lose;
    int     tie;
    unsigned short rating;
    unsigned short orig_rating; // ��l rating, �]���C���}�l�����@��, rating �ȴN�]���F
} ChessUser;

private typedef struct {
    int   used;
    int   size;
    void *body;
} ChessHistory;

/*   �����[��
 *
 * ���H��ԮɡA���賣�|���@�� broadcast_list �� linked-list�A�����ۨC�U�@
 * �B�ѡA�����N�o�ӰT���ᵹ���ǤH�]sock�^�C
 * �C��@���[�Ѫ̥[�J�]�[�ѥi�H�q����ζ¤誺�[�I�i��^�A�䤤�@�誺�U�Ѫ�
 * �� broadcast_list �N�|�h�@���O���A����N�|�N�U���Φ�����U���C�@�B��
 * �ǵ� broadcast_list ���Ҧ��ݭn���H�A�F���[�Ѫ��ĪG�C
 */
private typedef struct ChessBroadcastListNode {
    int    sock;
    struct ChessBroadcastListNode *next;
} ChessBroadcastListNode;

private typedef struct ChessBroadcastList {
    struct ChessBroadcastListNode head; /* dummy node */
} ChessBroadcastList;


typedef struct {
    int     limit_hand;
    int     limit_time;
    int     free_time;
    enum {
	CHESS_TIMEMODE_MULTIHAND, /* ���ɭ��B */
	CHESS_TIMEMODE_COUNTING   /* Ū�� */
    } time_mode;
} ChessTimeLimit;

typedef enum {
    CHESS_MODE_VERSUS,   /* �﫳 */
    CHESS_MODE_WATCH,    /* �[�� */
    CHESS_MODE_PERSONAL, /* ���� */
    CHESS_MODE_REPLAY    /* ���� */
} ChessGameMode;

typedef enum {
    CHESS_RESULT_CONTINUE,
    CHESS_RESULT_WIN,
    CHESS_RESULT_LOST,
    CHESS_RESULT_TIE,
    CHESS_RESULT_END   /* watching or replaying */
} ChessGameResult;

typedef struct ChessInfo {
    private const static
	struct ChessActions* actions; /* vtable */
    private const static
	struct ChessConstants* constants;

    rc_t cursor;

    /* �p�ɥ�, [0] = mine, [1] = his */
    int	    lefttime[2];
    int     lefthand[2]; /* ���ɭ��B�ɥ�, = 0 ���ۥѮɶ��ΫD���ɭ��B�Ҧ� */
    const ChessTimeLimit* timelimit;

    const ChessGameMode mode;
    const ChessUser user1;
    const ChessUser user2;
    const char myturn;   /* �ڤ��C�� */

    char       turn;
    char       ipass, hepass;
    char       warnmsg[64];
    char       last_movestr[36];
    char      *photo;

    void      *board;
    void      *tag;

    private int    sock;
    private ChessHistory history;
    private ChessBroadcastList broadcast_list;
    private ChessGameResult (*play_func[2])(struct ChessInfo* info);

    private int  current_step;  /* used by watch and replay */
    private char step_tmp[0];
} ChessInfo;

#undef const
#undef static
#undef private

typedef struct ChessActions {
    /* initial */
    void (*init_user)   (const userinfo_t* uinfo, ChessUser* user);
    void (*init_user_rec)(const userec_t* uinfo, ChessUser* user);
    void (*init_board)  (void* board);

    /* playing */
    void (*drawline)    (const ChessInfo* info, int line);
    void (*movecur)     (int r, int c);
    void (*prepare_play)(ChessInfo* info);
    int  (*select)      (ChessInfo* info, rc_t location,
	    ChessGameResult* result);
    void (*prepare_step)(ChessInfo* info, const void* step);
    ChessGameResult (*apply_step)  (void* board, const void* step);
    void (*drawstep)    (ChessInfo* info, const void* step);

    /* ending */
    void (*gameend)    (ChessInfo* info, ChessGameResult result);
    void (*genlog)     (ChessInfo* info, FILE* fp, ChessGameResult result);
} ChessActions;

typedef struct ChessConstants {
    int   step_entry_size;
    int   traditional_timeout;
    int   board_height;
    int   board_width;
    const char *chess_name;
    const char *photo_file_name;
    const char *log_board;
    const char *turn_color[2];
    const char *turn_str[2];
} ChessConstants;

typedef enum {
    CHESS_STEP_NORMAL, CHESS_STEP_PASS,
    CHESS_STEP_DROP, CHESS_STEP_FAILURE,
    CHESS_STEP_NOP, /* for wake up */
    CHESS_STEP_UNDO, /* undo request */
    CHESS_STEP_UNDO_ACC,  /* accept */
    CHESS_STEP_UNDO_REJ,  /* reject */
} ChessStepType;


int ChessTimeCountDown(ChessInfo* info, int who, int length);
void ChessStepMade(ChessInfo* info, int who);

ChessStepType ChessStepReceive(ChessInfo* info, void* step);
int ChessStepSendOpposite(ChessInfo* info, const void* step);
void ChessStepBroadcast(ChessInfo* info, const void* step);
int ChessStepSend(ChessInfo* info, const void* step);

void ChessHistoryAppend(ChessInfo* info, void* step);
const void* ChessHistoryRetrieve(ChessInfo* info, int n);

void ChessPlay(ChessInfo* info);
int ChessStartGame(char func_char, int sig, const char* title);
int ChessWatchGame(void (*play)(int, ChessGameMode),
	int game, const char* title);
int ChessReplayGame(const char* fname);

ChessInfo* NewChessInfo(const ChessActions* actions,
	const ChessConstants* constants, int sock, ChessGameMode mode);
void DeleteChessInfo(ChessInfo* info);

void ChessEstablishRequest(int sock);
void ChessAcceptingRequest(int sock);
void ChessShowRequest(void);

void ChessDrawLine(const ChessInfo* info, int line);
void ChessDrawExtraInfo(const ChessInfo* info, int line);

#endif /* INCLUDE_CHESS_H */
