/* $Id$ */
#include "bbs.h"
#include "chess.h"

#define assert_not_reached() assert(!"Should never be here!!!")

#define CHESS_HISTORY_INITIAL_BUFFER_SIZE 300
#define CHESS_HISTORY_BUFFER_INCREMENT     50

static ChessInfo * CurrentPlayingGameInfo;

/* XXX: This is a BAD way to pass information.
 *      Fix this by handling chess request ourselves.
 */
static ChessTimeLimit * _current_time_limit;

#define CHESS_HISTORY_ENTRY(INFO,N) \
    ((INFO)->history.body + (N) * (INFO)->constants->step_entry_size)
static void
ChessHistoryInit(ChessHistory* history, int entry_size)
{
    history->size = CHESS_HISTORY_INITIAL_BUFFER_SIZE;
    history->used = 0;
    history->body =
	calloc(CHESS_HISTORY_INITIAL_BUFFER_SIZE,
		entry_size);
}

const void*
ChessHistoryRetrieve(ChessInfo* info, int n)
{
    assert(n >= 0 && n < info->history.used);
    return CHESS_HISTORY_ENTRY(info, n);
}

void
ChessHistoryAppend(ChessInfo* info, void* step)
{
    if (info->history.used == info->history.size)
	info->history.body = realloc(info->history.body,
		(info->history.size += CHESS_HISTORY_BUFFER_INCREMENT)
		* info->constants->step_entry_size);

    memmove(CHESS_HISTORY_ENTRY(info, info->history.used),
	    step, info->constants->step_entry_size);
    ++(info->history.used);
}

static void
ChessBroadcastListInit(ChessBroadcastList* list)
{
    list->head.next = NULL;
}

static void
ChessBroadcastListClear(ChessBroadcastList* list)
{
    ChessBroadcastListNode* p = list->head.next;
    while (p) {
	ChessBroadcastListNode* t = p->next;
	close(p->sock);
	free(p);
	p = t;
    }
}

static ChessBroadcastListNode*
ChessBroadcastListInsert(ChessBroadcastList* list)
{
    ChessBroadcastListNode* p =
	(ChessBroadcastListNode*) malloc(sizeof(ChessBroadcastListNode));

    p->next = list->head.next;
    list->head.next = p;
    return p;
}

static void
ChessDrawHelpLine(const ChessInfo* info)
{
    const static char* const HelpStr[] =
    {
	/* CHESS_MODE_VERSUS, �﫳 */
	ANSI_COLOR(1;33;42) " �U�� "
	ANSI_COLOR(;31;47) " (��������)" ANSI_COLOR(30) " ����   "
	ANSI_COLOR(31) "(�ť���/ENTER)" ANSI_COLOR(30) " �U�l "
	ANSI_COLOR(31) "(q)" ANSI_COLOR(30) " �{�� "
	ANSI_COLOR(31) "(p)" ANSI_COLOR(30) " ��� "
	ANSI_RESET,

	/* CHESS_MODE_WATCH, �[�� */
	ANSI_COLOR(1;33;42) " �[�� "
	ANSI_COLOR(;31;47) " (����)" ANSI_COLOR(30) " �e��@�B   "
	ANSI_COLOR(31) "(����)" ANSI_COLOR(30) " �e��Q�B  "
	ANSI_COLOR(31) "(PGUP/PGDN)" ANSI_COLOR(30) " �̪�/�ثe�L��  "
	ANSI_COLOR(31) "(q)" ANSI_COLOR(30) " ���} "
	ANSI_RESET,

	/* CHESS_MODE_PERSONAL, ���� */
	ANSI_COLOR(1;33;42) " ���� "
	ANSI_COLOR(;31;47) " (��������)" ANSI_COLOR(30) " ����   "
	ANSI_COLOR(31) "(�ť���/ENTER)" ANSI_COLOR(30) " �U�l "
	ANSI_COLOR(31) "(q)" ANSI_COLOR(30) " ���} "
	ANSI_RESET,

	/* CHESS_MODE_REPLAY, ���� */
	ANSI_COLOR(1;33;42) " ���� "
	ANSI_COLOR(;31;47) " (����)" ANSI_COLOR(30) " �e��@�B   "
	ANSI_COLOR(31) "(����)" ANSI_COLOR(30) " �e��Q�B  "
	ANSI_COLOR(31) "(PGUP/PGDN)" ANSI_COLOR(30) " �̪�/�ثe�L��  "
	ANSI_COLOR(31) "(q)" ANSI_COLOR(30) " ���} "
	ANSI_RESET,
    };

    mouts(b_lines, 0, HelpStr[info->mode]);
}

static void
ChessRedraw(const ChessInfo* info)
{
    int i;
    for (i = 0; i < b_lines; ++i)
	info->actions->drawline(info, i);
    ChessDrawHelpLine(info);
}

inline static int
ChessTimeCountDownCalc(ChessInfo* info, int who, int length)
{
    info->lefttime[who] -= length;

    if (!info->timelimit) /* traditional mode, only left time is considered */
	return info->lefttime[who] < 0;

    if (info->lefttime[who] < 0) { /* only allowed when in free time */
	if (info->lefthand[who])
	    return 1;
	info->lefttime[who] = info->timelimit->limit_time;
	info->lefthand[who] = info->timelimit->limit_hand;
	return 0;
    }

    return 0;
}

int
ChessTimeCountDown(ChessInfo* info, int who, int length)
{
    int result = ChessTimeCountDownCalc(info, who, length);
    info->actions->drawline(info, CHESS_DRAWING_TIME_ROW);
    return result;
}

void
ChessStepMade(ChessInfo* info, int who)
{
    if (!info->timelimit)
	info->lefttime[who] = info->constants->traditional_timeout;
    else if (
	    (info->lefthand[who] && (--(info->lefthand[who]) == 0))
	    ||
	    (info->lefthand[who] == 0 && info->lefttime[who] <= 0)
	    ) {
	info->lefthand[who] = info->timelimit->limit_hand;
	info->lefttime[who] = info->timelimit->limit_time;
    }
}

/*
 * Start of the network communication function.
 */
inline static ChessStepType
ChessRecvMove(ChessInfo* info, int sock, void *step)
{
    if (read(sock, step, info->constants->step_entry_size)
	    != info->constants->step_entry_size)
	return CHESS_STEP_FAILURE;
    return *(ChessStepType*) step;
}

inline static int
ChessSendMove(ChessInfo* info, int sock, const void *step)
{
    if (write(sock, step, info->constants->step_entry_size)
	    != info->constants->step_entry_size)
	return 0;
    return 1;
}

ChessStepType
ChessStepReceive(ChessInfo* info, void* step)
{
    ChessStepType result = ChessRecvMove(info, info->sock, step);

    if (result != CHESS_STEP_FAILURE) {
	/* automatical routing */
	ChessStepBroadcast(info, step);

	/* and logging */
	ChessHistoryAppend(info, step);
    }

    return result;
}

int
ChessStepSendOpposite(ChessInfo* info, const void* step)
{
    void (*orig_handler)(int);
    int    result = 1;
    
    orig_handler = Signal(SIGPIPE, SIG_IGN);

    if (!ChessSendMove(info, info->sock, step))
	result = 0;

    Signal(SIGPIPE, orig_handler);
    return result;
}

void
ChessStepBroadcast(ChessInfo* info, const void *step)
{
    ChessBroadcastListNode *p = &(info->broadcast_list.head);
    void (*orig_handler)(int);
    
    orig_handler = Signal(SIGPIPE, SIG_IGN);

    while(p->next){
	if (!ChessSendMove(info, p->next->sock, step)) {
	    /* remove viewer */
	    ChessBroadcastListNode *tmp = p->next->next;
	    free(p->next);
	    p->next = tmp;
	} else
	    p = p->next;
    }

    Signal(SIGPIPE, orig_handler);
}

int
ChessStepSend(ChessInfo* info, const void* step)
{
    /* send to opposite... */
    if (!ChessStepSendOpposite(info, step))
	return 0;

    /* and watchers */
    ChessStepBroadcast(info, step);

    return 1;
}

int
ChessMessageSend(ChessInfo* info, ChessStepType type)
{
    return ChessStepSend(info, &type);
}

inline static void
ChessReplayUntil(ChessInfo* info, int n)
{
    const void* step;

    while (info->current_step < n - 1) {
	info->actions->apply_step(info->board,
		ChessHistoryRetrieve(info, info->current_step));
	++(info->current_step);
    }

    /* spcial for last one to maintian information correct */
    step = ChessHistoryRetrieve(info, info->current_step);
    info->actions->prepare_step(info, step);
    info->actions->apply_step(info->board, step);
    ++(info->current_step);
}

static ChessGameResult
ChessPlayFuncMy(ChessInfo* info)
{
    int last_time = now;
    int endturn = 0;
    ChessGameResult game_result = CHESS_RESULT_CONTINUE;
    int ch;

    info->ipass = 0;
    bell();

    while (!endturn) {
	ChessStepType result;

	info->actions->drawline(info, CHESS_DRAWING_TIME_ROW);
	info->actions->movecur(info->cursor.r, info->cursor.c);
	oflush();

	ch = igetch();
	if (ChessTimeCountDown(info, 0, now - last_time))
	    ch = 'q';
	last_time = now;

	switch (ch) {
	    case I_OTHERDATA:
		result = ChessStepReceive(info, &info->step_tmp);

		if (result == CHESS_STEP_FAILURE ||
			result == CHESS_STEP_DROP) {
		    game_result = CHESS_RESULT_WIN;
		    endturn = 1;
		} else if (result == CHESS_STEP_PASS && info->ipass) {
		    game_result = CHESS_RESULT_TIE;
		    endturn = 1;
		}
		break;

	    case KEY_UP:
		--(info->cursor.r);
		if (info->cursor.r < 0)
		    info->cursor.r = info->constants->board_height - 1;
		break;

	    case KEY_DOWN:
		++(info->cursor.r);
		if (info->cursor.r >= info->constants->board_height)
		    info->cursor.r = 0;
		break;

	    case KEY_LEFT:
		--(info->cursor.c);
		if (info->cursor.c < 0)
		    info->cursor.c = info->constants->board_width - 1;
		break;

	    case KEY_RIGHT:
		++(info->cursor.c);
		if (info->cursor.c >= info->constants->board_width)
		    info->cursor.c = 0;
		break;

	    case 'q':
		game_result = CHESS_RESULT_LOST;
		endturn = 1;
		break;

	    case 'p':
		info->ipass = 1;
		ChessMessageSend(info, CHESS_STEP_PASS);
		strlcpy(info->warnmsg,
			ANSI_COLOR(1;33) "�n�D�M��!" ANSI_RESET,
			sizeof(info->warnmsg));
		info->actions->drawline(info, CHESS_DRAWING_WARN_ROW);
		bell();
		break;

	    case '\r':
	    case '\n':
	    case ' ':
		endturn = info->actions->select(info, info->cursor, &game_result);
		break;
	}
    }
    ChessTimeCountDown(info, 0, now - last_time);
    ChessStepMade(info, 0);
    info->actions->drawline(info, CHESS_DRAWING_TIME_ROW);
    info->actions->drawline(info, CHESS_DRAWING_STEP_ROW);
    return game_result;
}

static ChessGameResult
ChessPlayFuncHis(ChessInfo* info)
{
    int last_time = now;
    int endturn = 0;
    ChessGameResult game_result = CHESS_RESULT_CONTINUE;

    while (!endturn) {
	ChessStepType result;

	if (ChessTimeCountDown(info, 1, now - last_time)) {
	    info->lefttime[1] = 0;

	    /* to make him break out igetch() */
	    ChessMessageSend(info, CHESS_STEP_NOP);
	}
	last_time = now;

	info->actions->drawline(info, CHESS_DRAWING_TIME_ROW);
	move(1, 0);
	oflush();

	switch (igetch()) {
	    case 'q':
		game_result = CHESS_RESULT_LOST;
		endturn = 1;
		break;

	    case 'p':
		if (info->hepass) {
		    ChessMessageSend(info, CHESS_STEP_PASS);
		    game_result = CHESS_RESULT_TIE;
		    endturn = 1;
		}
		break;

	    case I_OTHERDATA:
		result = ChessStepReceive(info, &info->step_tmp);

		if (result == CHESS_STEP_FAILURE ||
			result == CHESS_STEP_DROP) {
		    game_result = CHESS_RESULT_WIN;
		    endturn = 1;
		} else if (result == CHESS_STEP_PASS) {
		    info->hepass = 1;
		    strlcpy(info->warnmsg,
			    ANSI_COLOR(1;33) "�n�D�M��!" ANSI_RESET,
			    sizeof(info->warnmsg));
		    info->actions->drawline(info, CHESS_DRAWING_WARN_ROW);
		} else {
		    info->actions->prepare_step(info, &info->step_tmp);
		    if (info->actions->apply_step(info->board, &info->step_tmp))
			game_result = CHESS_RESULT_LOST;
		    endturn = 1;
		    info->hepass = 0;
		    ChessStepMade(info, 1);
		    info->actions->drawstep(info, &info->step_tmp);
		}
	}
    }
    ChessTimeCountDown(info, 1, now - last_time);
    info->actions->drawline(info, CHESS_DRAWING_TIME_ROW);
    info->actions->drawline(info, CHESS_DRAWING_STEP_ROW);
    return game_result;
}

static ChessGameResult
ChessPlayFuncWatch(ChessInfo* info)
{
    int end_watch = 0;

    while (!end_watch) {
	info->actions->prepare_play(info);
	if (info->sock == -1)
	    strlcpy(info->warnmsg, ANSI_COLOR(1;33) "�ѧ��w����" ANSI_RESET,
		    sizeof(info->warnmsg));

	info->actions->drawline(info, CHESS_DRAWING_WARN_ROW);
	move(1, 0);

	switch (igetch()) {
	    case I_OTHERDATA: /* new step */
		if (ChessStepReceive(info, &info->step_tmp) == CHESS_STEP_FAILURE) {
		    add_io(0, 0);
		    info->sock = -1;
		    break;
		}

		if (info->current_step == info->history.used - 1) {
		    /* was watching up-to-date board */
		    info->actions->prepare_step(info, &info->step_tmp);
		    info->actions->apply_step(info->board, &info->step_tmp);
		    info->actions->drawstep(info, &info->step_tmp);
		    ++(info->current_step);
		}
		break;

	    case KEY_LEFT: /* ���e�@�B */
		if (info->current_step == 0)
		    bell();
		else {
		    /* TODO: implement without re-apply all steps */
		    int current = info->current_step;

		    info->actions->init_board(info->board);
		    info->current_step = 0;

		    if (current > 1)
			ChessReplayUntil(info, current - 1);
		    ChessRedraw(info);
		}
		break;

	    case KEY_RIGHT: /* ����@�B */
		if (info->current_step == info->history.used)
		    bell();
		else {
		    const void* step =
			ChessHistoryRetrieve(info, info->current_step);
		    info->actions->prepare_step(info, step);
		    info->actions->apply_step(info->board, step);
		    info->actions->drawstep(info, step);
		    ++(info->current_step);
		}
		break;

	    case KEY_UP: /* ���e�Q�B */
		if (info->current_step == 0)
		    bell();
		else {
		    /* TODO: implement without re-apply all steps */
		    int current = info->current_step;

		    info->actions->init_board(info->board);
		    info->current_step = 0;

		    if (current > 10)
			ChessReplayUntil(info, current - 10);

		    ChessRedraw(info);
		}
		break;

	    case KEY_DOWN: /* ����Q�B */
		if (info->current_step == info->history.used)
		    bell();
		else {
		    ChessReplayUntil(info,
			    MIN(info->current_step + 10, info->history.used));
		    ChessRedraw(info);
		}
		break;

	    case KEY_PGUP: /* �_�l�L�� */
		if (info->current_step == 0)
		    bell();
		else {
		    info->actions->init_board(info->board);
		    info->current_step = 0;
		    ChessRedraw(info);
		}
		break;

	    case KEY_PGDN: /* �̷s�L�� */
		if (info->current_step == info->history.used)
		    bell();
		else {
		    ChessReplayUntil(info, info->history.used);
		    ChessRedraw(info);
		}
		break;

	    case 'q':
		end_watch = 1;
	}
    }

    return CHESS_RESULT_END;
}

static void
ChessWatchRequest(int sig)
{
    int sock = establish_talk_connection(&SHM->uinfo[currutmp->destuip]);
    ChessBroadcastListNode* node;

    if (sock < 0)
	return;
    
    node = ChessBroadcastListInsert(&CurrentPlayingGameInfo->broadcast_list);
    node->sock = sock;

#define SEND(X) write(sock, &(X), sizeof(X))
    SEND(CurrentPlayingGameInfo->myturn);
    SEND(CurrentPlayingGameInfo->turn);

    if (!CurrentPlayingGameInfo->timelimit)
	write(sock, "T", 1);
    else {
	write(sock, "L", 1);
	SEND(*(CurrentPlayingGameInfo->timelimit));
    }

    SEND(CurrentPlayingGameInfo->history.used);
    write(sock, CurrentPlayingGameInfo->history.body,
	    CurrentPlayingGameInfo->constants->step_entry_size
	    * CurrentPlayingGameInfo->history.used);
#undef SEND
}

static void
ChessReceiveWatchInfo(ChessInfo* info)
{
    char time_mode;
#define RECV(X) read(info->sock, &(X), sizeof(X))
    RECV(info->myturn);
    RECV(info->turn);
    
    RECV(time_mode);
    if (time_mode == 'L') {
	info->timelimit = (ChessTimeLimit*) malloc(sizeof(ChessTimeLimit));
	RECV(*(info->timelimit));
    }

    RECV(info->history.used);
    for (info->history.size = CHESS_HISTORY_INITIAL_BUFFER_SIZE;
	    info->history.size < info->history.used;
	    info->history.size += CHESS_HISTORY_BUFFER_INCREMENT);
    info->history.body =
	calloc(info->history.size, info->constants->step_entry_size);
    read(info->sock, info->history.body,
	    info->history.used * info->constants->step_entry_size);
#undef RECV
}

static void
ChessGenLogGlobal(ChessInfo* info, ChessGameResult result)
{
    fileheader_t log_header;
    FILE        *fp;
    char         buf[256];
    int          bid;

    if ((bid = getbnum(info->constants->log_board)) == 0)
	return;

    setbpath(buf, info->constants->log_board);
    stampfile(buf, &log_header);

    fp = fopen(buf, "w");
    if (fp != NULL) {
	info->actions->genlog(info, fp, result);
	fclose(fp);

	strlcpy(log_header.owner, "[���о����H]", sizeof(log_header.owner));
	snprintf(log_header.title, sizeof(log_header.title),
		ANSI_COLOR(37;41) "����" ANSI_RESET " %s VS %s",
		info->user1.userid, info->user2.userid);

	setbdir(buf, info->constants->log_board);
	append_record(buf, &log_header, sizeof(log_header));

	setbtotal(bid);
    }
}

static void
ChessGenLogUser(ChessInfo* info, ChessGameResult result)
{
    fileheader_t log_header;
    FILE        *fp;
    char         buf[256];

    sethomepath(buf, cuser.userid);
    stampfile(buf, &log_header);

    fp = fopen(buf, "w");
    if (fp != NULL) {
	info->actions->genlog(info, fp, result);
	fclose(fp);

	strlcpy(log_header.owner, "[���e�~��]", sizeof(log_header.owner));
	if(info->myturn == 0)
	    sprintf(log_header.title, "%s V.S. %s",
		    info->user1.userid, info->user2.userid);
	else
	    sprintf(log_header.title, "%s V.S. %s",
		    info->user2.userid, info->user1.userid);
	log_header.filemode = 0;

	sethomedir(buf, cuser.userid);
	append_record_forward(buf, &log_header, sizeof(log_header),
		cuser.userid);

	mailalert(cuser.userid);
    }
}

static void
ChessGenLog(ChessInfo* info, ChessGameResult result)
{
    if (info->mode == CHESS_MODE_VERSUS && info->myturn == 0 &&
	info->constants->log_board) {
	ChessGenLogGlobal(info, result);
    }

    if (getans("�O�_�N���бH�^�H�c�H[N/y]") == 'y')
	ChessGenLogUser(info, result);
}

void
ChessPlay(ChessInfo* info)
{
    ChessGameResult game_result;
    void          (*old_handler)(int);
    const char*     game_result_str = 0;

    if (info == NULL)
	return;

    /* XXX */
    if (!info->timelimit) {
	info->timelimit = _current_time_limit;
	_current_time_limit = NULL;
    }

    CurrentPlayingGameInfo = info;
    old_handler = Signal(SIGUSR1, &ChessWatchRequest);

    if (info->mode == CHESS_MODE_WATCH) {
	int i;
	for (i = 0; i < info->history.used; ++i)
	    info->actions->apply_step(info->board,
		    ChessHistoryRetrieve(info, i));
	info->current_step = info->history.used;
    }

    /* playing initialization */
    ChessRedraw(info);
    info->turn = 1;
    info->lefttime[0] = info->lefttime[1] = info->timelimit ?
	info->timelimit->free_time : info->constants->traditional_timeout;
    info->lefthand[0] = info->lefthand[1] = 0;

    /* main loop */
    add_io(info->sock, 0);
    for (game_result = CHESS_RESULT_CONTINUE;
	 game_result == CHESS_RESULT_CONTINUE;
	 info->turn ^= 1) {
	info->actions->prepare_play(info);
	info->actions->drawline(info, CHESS_DRAWING_TURN_ROW);
	info->actions->drawline(info, CHESS_DRAWING_WARN_ROW);
	game_result = info->play_func[(int) info->turn](info);
    }
    add_io(0, 0);

    if (info->sock)
	close(info->sock);

    /* end processing */
    if (info->mode == CHESS_MODE_VERSUS) {
	switch (game_result) {
	    case CHESS_RESULT_WIN:
		game_result_str = "���{��F!";
		break;

	    case CHESS_RESULT_LOST:
		game_result_str = "�A�{��F!";
		break;

	    case CHESS_RESULT_TIE:
		game_result_str = "�M��";
		break;

	    default:
		assert_not_reached();
	}
    } else if (info->mode == CHESS_MODE_WATCH)
	game_result_str = "�����[��";
    else if (info->mode == CHESS_MODE_PERSONAL)
	game_result_str = "��������";
    else if (info->mode == CHESS_MODE_REPLAY)
	game_result_str = "��������";

    if (game_result_str) {
	strlcpy(info->warnmsg, game_result_str, sizeof(info->warnmsg));
	info->actions->drawline(info, CHESS_DRAWING_WARN_ROW);
    }

    info->actions->gameend(info, game_result);
    ChessGenLog(info, game_result);

    Signal(SIGUSR1, old_handler);
    CurrentPlayingGameInfo = NULL;
}

static userinfo_t*
ChessSearchUser(int sig, const char* title)
{
    char            uident[16];
    userinfo_t	   *uin;

    stand_title(title);
    CompleteOnlineUser(msg_uid, uident);
    if (uident[0] == '\0')
	return NULL;

    if ((uin = search_ulist_userid(uident)) == NULL)
	return NULL;

    if (sig >= 0)
	uin->sig = sig;
    return uin;
}

int
ChessStartGame(char func_char, int sig, const char* title)
{
    userinfo_t     *uin;
    char buf[4];
    
    if ((uin = ChessSearchUser(sig, title)) == NULL)
	return -1;
    uin->turn = 1;
    currutmp->turn = 0;
    strlcpy(uin->mateid, currutmp->userid, sizeof(uin->mateid));
    strlcpy(currutmp->mateid, uin->userid, sizeof(currutmp->mateid));

    stand_title(title);
    buf[0] = 0;
    getdata(2, 0, "�ϥζǲμҦ� (T), ���ɭ��B�Ҧ� (L) �άO Ū��Ҧ� (C)? (T/l/c)",
	    buf, 3, DOECHO);

    if (buf[0] == 'l' || buf[0] == 'L' ||
	buf[0] == 'c' || buf[0] == 'C') {

	_current_time_limit = (ChessTimeLimit*) malloc(sizeof(ChessTimeLimit));
	if (buf[0] == 'l' || buf[0] == 'L')
	    _current_time_limit->time_mode = CHESS_TIMEMODE_MULTIHAND;
	else
	    _current_time_limit->time_mode = CHESS_TIMEMODE_COUNTING;

	do {
	    getdata_str(3, 0, "�г]�w���� (�ۥѮɶ�) �H���������:",
		    buf, 3, DOECHO, "30");
	    _current_time_limit->free_time = atoi(buf);
	} while (_current_time_limit->free_time < 0 || _current_time_limit->free_time > 90);
	_current_time_limit->free_time *= 60; /* minute -> second */

	if (_current_time_limit->time_mode == CHESS_TIMEMODE_MULTIHAND) {
	    char display_buf[128];

	    do {
		getdata_str(4, 0, "�г]�w�B��, �H���������:",
			buf, 3, DOECHO, "5");
		_current_time_limit->limit_time = atoi(buf);
	    } while (_current_time_limit->limit_time < 0 || _current_time_limit->limit_time > 30);
	    _current_time_limit->limit_time *= 60; /* minute -> second */

	    snprintf(display_buf, sizeof(display_buf),
		    "�г]�w���B (�C %d �����ݨ��X�B):",
		    _current_time_limit->limit_time / 60);
	    do {
		getdata_str(5, 0, display_buf, buf, 3, DOECHO, "10");
		_current_time_limit->limit_hand = atoi(buf);
	    } while (_current_time_limit->limit_hand < 1);
	} else {
	    _current_time_limit->limit_hand = 1;

	    do {
		getdata_str(4, 0, "�г]�wŪ��, �H�����",
			buf, 3, DOECHO, "60");
		_current_time_limit->limit_time = atoi(buf);
	    } while (_current_time_limit->limit_time < 0);
	}
    } else
	_current_time_limit = NULL;

    my_talk(uin, friend_stat(currutmp, uin), func_char);
    return 0;
}

int
ChessWatchGame(void (*play)(int, ChessGameMode), int game, const char* title)
{
    int 	    sock, msgsock;
    userinfo_t     *uin;

    if ((uin = ChessSearchUser(-1, title)) == NULL)
	return -1;

    if (uin->uid == currutmp->uid || uin->mode != game)
	return -1;

    if (getans("�O�_�i���[��? [N/y]") != 'y')
	return 0;

    if ((sock = make_connection_to_somebody(uin, 10)) < 0) {
	vmsg("�L�k�إ߳s�u");
	return -1;
    }
#if defined(Solaris) && __OS_MAJOR_VERSION__ == 5 && __OS_MINOR_VERSION__ < 7
    msgsock = accept(sock, (struct sockaddr *) 0, 0);
#else
    msgsock = accept(sock, (struct sockaddr *) 0, (socklen_t *) 0);
#endif
    close(sock);
    if (msgsock < 0)
	return -1;

    strlcpy(currutmp->mateid, uin->userid, sizeof(currutmp->mateid));
    play(msgsock, CHESS_MODE_WATCH);
    close(msgsock);
    return 0;
}

static void
ChessInitUser(ChessInfo* info)
{
    char	      userid[2][IDLEN + 1];
    const userinfo_t* uinfo;

    if (info->mode == CHESS_MODE_PERSONAL) {
	strlcpy(userid[0], cuser.userid, sizeof(userid[0]));
	strlcpy(userid[1], cuser.userid, sizeof(userid[1]));
    }
    else if (info->mode == CHESS_MODE_WATCH) {
	userinfo_t *uinfo = search_ulist_userid(currutmp->mateid);
	strlcpy(userid[0], uinfo->userid, sizeof(userid[0]));
	strlcpy(userid[1], uinfo->mateid, sizeof(userid[1]));
    }
    else if (info->mode == CHESS_MODE_VERSUS) {
	strlcpy(userid[0], cuser.userid, sizeof(userid[0]));
	strlcpy(userid[1], currutmp->mateid, sizeof(userid[1]));
    } else
	assert_not_reached();

    uinfo = search_ulist_userid(userid[0]);
    info->actions->init_user(uinfo, &info->user1);
    uinfo = search_ulist_userid(userid[1]);
    info->actions->init_user(uinfo, &info->user2);
}

static char*
ChessPhotoInitial(ChessInfo* info)
{
    char genbuf[256];
    int line;
    FILE* fp;
    static const char * const blank_photo[6] = {
	"�z�w�w�w�w�w�w�{",
	"�x ��         �x",
	"�x    ��      �x",
	"�x       ��   �x",
	"�x          ���x",
	"�|�w�w�w�w�w�w�}" 
    };
    char country[5], level[11];
    userec_t xuser;
    char* photo;

    if (info->mode == CHESS_MODE_REPLAY)
	return NULL;

    sethomefile(genbuf, info->user1.userid, info->constants->photo_file_name);
    if (!dashf(genbuf)) {
	sethomefile(genbuf, info->user2.userid, info->constants->photo_file_name);
	if (!dashf(genbuf))
	    return NULL;
    }

    photo = (char*) calloc(
	    CHESS_PHOTO_LINE * CHESS_PHOTO_COLUMN, sizeof(char));

    /* simulate photo as two dimensional array  */
#define PHOTO(X) (photo + (X) * CHESS_PHOTO_COLUMN)

    getuser(info->user1.userid, &xuser);
    sethomefile(genbuf, info->user1.userid, info->constants->photo_file_name);
    fp = fopen(genbuf, "r");

    if (fp == NULL) {
	strcpy(country, "�L");
	level[0] = 0;
    } else {
	int i, j;
	for (line = 1; line < 8; ++line)
	    fgets(genbuf, sizeof(genbuf), fp);

	fgets(genbuf, sizeof(genbuf), fp);
	chomp(genbuf);
	strip_ansi(genbuf + 11, genbuf + 11,
		STRIP_ALL);        /* country name may have color */
	for (i = 11, j = 0; genbuf[i] && j < 4; ++i)
	    if (genbuf[i] != ' ')  /* and spaces */
		country[j++] = genbuf[i];
	country[j] = 0; /* two chinese words */

	fgets(genbuf, sizeof(genbuf), fp);
	chomp(genbuf);
	strlcpy(level, genbuf + 11, 11); /* five chinese words*/
	rewind(fp);
    }

    for (line = 0; line < 6; ++line) {
	if (fp != NULL) {
	    if (fgets(genbuf, sizeof(genbuf), fp)) {
		chomp(genbuf);
		sprintf(PHOTO(line), "%s  ", genbuf);
	    } else
		strcpy(PHOTO(line), "                  ");
	} else
	    strcpy(PHOTO(line), blank_photo[line]);

	switch (line) {
	    case 0: sprintf(genbuf, "<�N��> %s", xuser.userid);      break;
	    case 1: sprintf(genbuf, "<�ʺ�> %.16s", xuser.nickname); break;
	    case 2: sprintf(genbuf, "<�W��> %d", xuser.numlogins);   break;
	    case 3: sprintf(genbuf, "<�峹> %d", xuser.numposts);    break;
	    case 4: sprintf(genbuf, "<¾��> %-4s %s", country, level);  break;
	    case 5: sprintf(genbuf, "<�ӷ�> %.16s", xuser.lasthost); break;
	    default: genbuf[0] = 0;
	}
	strcat(PHOTO(line), genbuf);
    }
    if (fp != NULL)
	fclose(fp);

    sprintf(PHOTO(6), "      %s%2.2s��" ANSI_RESET,
	    info->constants->turn_color[(int) info->myturn],
	    info->constants->turn_str[(int) info->myturn]);
    strcpy(PHOTO(7), "           ��.��           ");
    sprintf(PHOTO(8), "                               %s%2.2s��" ANSI_RESET,
	    info->constants->turn_color[info->myturn ^ 1],
	    info->constants->turn_str[info->myturn ^ 1]);

    getuser(info->user2.userid, &xuser);
    sethomefile(genbuf, info->user2.userid, "photo_cchess");
    fp = fopen(genbuf, "r");

    if (fp == NULL) {
	strcpy(country, "�L");
	level[0] = 0;
    } else {
	int i, j;
	for (line = 1; line < 8; ++line)
	    fgets(genbuf, sizeof(genbuf), fp);

	fgets(genbuf, sizeof(genbuf), fp);
	chomp(genbuf);
	strip_ansi(genbuf + 11, genbuf + 11,
		STRIP_ALL);        /* country name may have color */
	for (i = 11, j = 0; genbuf[i] && j < 4; ++i)
	    if (genbuf[i] != ' ')  /* and spaces */
		country[j++] = genbuf[i];
	country[j] = 0; /* two chinese words */

	fgets(genbuf, sizeof(genbuf), fp);
	chomp(genbuf);
	strlcpy(level, genbuf + 11, 11); /* five chinese words*/
	rewind(fp);
    }

    for (line = 9; line < 15; ++line) {
	move(line, 37);
	switch (line - 9) {
	    case 0: sprintf(PHOTO(line), "<�N��> %-16.16s ", xuser.userid);   break;
	    case 1: sprintf(PHOTO(line), "<�ʺ�> %-16.16s ", xuser.nickname); break;
	    case 2: sprintf(PHOTO(line), "<�W��> %-16d ", xuser.numlogins);   break;
	    case 3: sprintf(PHOTO(line), "<�峹> %-16d ", xuser.numposts);    break;
	    case 4: sprintf(PHOTO(line), "<¾��> %-4s %-10s  ", country, level); break;
	    case 5: sprintf(PHOTO(line), "<�ӷ�> %-16.16s ", xuser.lasthost); break;
	}

	if (fp != NULL) {
	    if (fgets(genbuf, 200, fp)) {
		chomp(genbuf);
		strcat(PHOTO(line), genbuf);
	    } else
		strcat(PHOTO(line), "                ");
	} else
	    strcat(PHOTO(line), blank_photo[line - 9]);
    }
    if (fp != NULL)
	fclose(fp);
#undef PHOTO

    return photo;
}

static void
ChessInitPlayFunc(ChessInfo* info)
{
    switch (info->mode) {
	case CHESS_MODE_VERSUS:
	    info->play_func[(int) info->myturn] = &ChessPlayFuncMy;
	    info->play_func[info->myturn ^ 1]   = &ChessPlayFuncHis;
	    break;

	case CHESS_MODE_WATCH:
	    info->play_func[0] = info->play_func[1] = &ChessPlayFuncWatch;
	    break;

	case CHESS_MODE_PERSONAL:
	    info->play_func[0] = info->play_func[1] = &ChessPlayFuncMy;
	    break;

	case CHESS_MODE_REPLAY:
	    /* TODO: not implemented yet */
	    assert_not_reached();
	    break;
    }
}

ChessInfo*
NewChessInfo(const ChessActions* actions, const ChessConstants* constants,
	int sock, ChessGameMode mode)
{
    /* allocate memory for the structure and extra space for temporary
     * steping information storage (step_tmp[0]). */
    ChessInfo* info =
	(ChessInfo*) calloc(1, sizeof(ChessInfo) + constants->step_entry_size);

    /* compiler don't know it's actually const... */
    info->actions   = (ChessActions*)   actions;
    info->constants = (ChessConstants*) constants;
    info->mode      = mode;
    info->sock      = sock;

    if (mode == CHESS_MODE_VERSUS)
	info->myturn = currutmp->turn;
    else if (mode == CHESS_MODE_PERSONAL)
	info->myturn = 1;
    else if (mode == CHESS_MODE_WATCH)
	ChessReceiveWatchInfo(info);

    ChessInitUser(info);

    info->photo = ChessPhotoInitial(info);

    if (mode != CHESS_MODE_WATCH)
	ChessHistoryInit(&info->history, constants->step_entry_size);

    ChessBroadcastListInit(&info->broadcast_list);
    ChessInitPlayFunc(info);

    return info;
}

void
DeleteChessInfo(ChessInfo* info)
{
#define NULL_OR_FREE(X) if (X) free(X); else (void) 0
    NULL_OR_FREE(info->timelimit);
    NULL_OR_FREE(info->photo);
    NULL_OR_FREE(info->history.body);

    ChessBroadcastListClear(&info->broadcast_list);
#undef NULL_OR_FREE
}

void
ChessEstablishRequest(int sock)
{
    /* XXX */
    if (!_current_time_limit)
	write(sock, "T", 1); /* traditional */
    else {
	write(sock, "L", 1); /* limited */
	write(sock, _current_time_limit, sizeof(ChessTimeLimit));
    }
}

void
ChessAcceptingRequest(int sock)
{
    /* XXX */
    char mode;
    read(sock, &mode, 1);
    if (mode == 'T')
	_current_time_limit = NULL;
    else {
	_current_time_limit = (ChessTimeLimit*) malloc(sizeof(ChessTimeLimit));
	read(sock, _current_time_limit, sizeof(ChessTimeLimit));
    }
}

void
ChessShowRequest(void)
{
    /* XXX */
    if (!_current_time_limit)
	mouts(10, 5, "�ϥζǲέp�ɤ覡, ��B���ɤ�����");
    else if (_current_time_limit->time_mode == CHESS_TIMEMODE_MULTIHAND) {
	mouts(10, 5, "�ϥέ��ɭ��B�W�h:");
	move(12, 8);
	prints("���� (�ۥѮɶ�): %2d �� %02d ��",
		_current_time_limit->free_time / 60,
		_current_time_limit->free_time % 60);
	move(13, 8);
	prints("���ɨB��:        %2d �� %02d �� / %2d ��",
		_current_time_limit->limit_time / 60,
		_current_time_limit->limit_time % 60,
		_current_time_limit->limit_hand);
    } else if (_current_time_limit->time_mode == CHESS_TIMEMODE_COUNTING) {
	mouts(10, 5, "�ϥ�Ū��W�h:");
	move(12, 8);
	prints("���� (�ۥѮɶ�): %2d �� %02d ��",
		_current_time_limit->free_time / 60,
		_current_time_limit->free_time % 60);
	move(13, 8);
	prints("Ū��ɶ�:   �C�� %2d ��", _current_time_limit->limit_time);
    }
}

