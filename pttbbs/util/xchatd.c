/* $Id$ */
#include "bbs.h"
#include "xchatd.h"
#include <sys/wait.h>
#include <netdb.h>

#define SERVER_USAGE
#undef  MONITOR                 /* �ʷ� chatroom ���ʥH�ѨM�ȯ� */
#undef  DEBUG                   /* �{���������� */

#ifdef  DEBUG
#define MONITOR
#endif

/* self-test:
 * random test, �H�����ͦU�� client input, �ت�������� server
 * crash �����p, �]�� client �å����� server �ǹL�Ӫ� data.
 * server �]���|Ū�� bbs ��ڪ��ɮש� SHM.
 * �ھ� gcov(1) �� self-test coverage �� 90%
 *
 * �p�����:
 * define SELFTEST & SELFTESTER, ������H�N�[�Ѽ�(argc>1)�N�|�] test child.
 * test server �ȶi�� 100 ��.
 *
 * Hint:
 * �t�X valgrind �M�� memory related bug.
 */
//#define SELFTEST
//#define SELFTESTER

#ifdef SELFTEST
// �t�} port
#undef NEW_CHATPORT
#define NEW_CHATPORT 12333
// only test 100 secs
#undef CHAT_INTERVAL
#define CHAT_INTERVAL 100
#endif

#define CHAT_PIDFILE    "log/chat.pid"
#define CHAT_LOGFILE    "log/chat.log"
#define CHAT_INTERVAL   (60 * 30)
#define SOCK_QLEN       1


/* name of the main room (always exists) */

#define MAIN_NAME       "main"
#define MAIN_TOPIC      "�i���i�^��Ѧ�"

#define ROOM_LOCKED     1
#define ROOM_SECRET     2
#define ROOM_OPENTOPIC  4
#define ROOM_HANDUP     8
#define ROOM_ALL        (NULL)


#define LOCKED(room)    (room->rflag & ROOM_LOCKED)
#define SECRET(room)    (room->rflag & ROOM_SECRET)
#define OPENTOPIC(room) (room->rflag & ROOM_OPENTOPIC)
#define RHANDUP(room)    (room->rflag & ROOM_HANDUP)

#define RESTRICTED(usr) (usr->uflag == 0)       /* guest */
#define CHATSYSOP(usr)  (usr->uflag & ( PERM_SYSOP | PERM_CHATROOM))
/* Thor: SYSOP �P CHATROOM���O chat�`�� */
#define PERM_ROOMOP     PERM_CHAT       /* Thor: �� PERM_CHAT�� PERM_ROOMOP */
#define PERM_HANDUP     PERM_BM		/* �� PERM_BM �����S���|��L */
#define PERM_SAY        PERM_NOTOP	/* �� PERM_NOTOP �����S���o���v */

/* �i�J�ɻݲM��              */
/* Thor: ROOMOP���ж��޲z�� */
#define ROOMOP(usr)  (usr->uflag & ( PERM_ROOMOP | PERM_SYSOP | PERM_CHATROOM))
#define CLOAK(usr)      (usr->uflag & PERM_CLOAK)
#define HANDUP(usr)  (usr->uflag & PERM_HANDUP) 
#define SAY(usr)      (usr->uflag & PERM_SAY)
/* Thor: ��ѫ������N */

#define CHATID_LEN  (8)
#define TOPIC_LEN   (48)

// �T����m (WOW style is good!)
#define COLOR_PRIVATEMSG    ANSI_COLOR(1;35)
#define COLOR_ANNOUNCE	    ANSI_COLOR(1;33)

/* ----------------------------------------------------- */
/* ChatRoom data structure                               */
/* ----------------------------------------------------- */

typedef struct ChatRoom ChatRoom;
typedef struct ChatUser ChatUser;
typedef struct UserList UserList;
typedef struct ChatCmd ChatCmd;
typedef struct ChatAction ChatAction;

struct ChatUser
{
    struct ChatUser *unext;
    ChatRoom *room;
    UserList *ignore;
    int sock;                     /* user socket */
    int userno;			  /* userno �O PASSWD �Ӫ� unum, �X�G�O�ߤ@�� */
    int uflag;
    int isize;                    /* current size of ibuf */
    uint32_t numlogindays;	  /* login counter */
    uint32_t numposts;		  /* post number */
    char userid[IDLEN+1];         /* real userid */
    char lasthost[IPV4LEN+1];     /* host address */
    char nickname[24];		  /* BBS nickname */
    char chatid[CHATID_LEN + 1];  /* chat id */
    char ibuf[80];                /* buffer for non-blocking receiving */
};


struct ChatRoom
{
    struct ChatRoom *next, *prev;
    char name[IDLEN + 1];
    char topic[TOPIC_LEN + 1];    /* Let the room op to define room topic */
    int rflag;                    /* ROOM_LOCKED, ROOM_SECRET, ROOM_OPENTOPIC */
    int occupants;                /* number of users in room */
    UserList *invite;
    UserList *ban;
};


struct UserList
{
    struct UserList *next;
    int userno;
    char userid[IDLEN + 1];
};


struct ChatCmd
{
    char *cmdstr;
    void (*cmdfunc) ();
    int exact;
};


static ChatRoom mainroom;
static ChatUser *mainuser;
static fd_set mainfds;
static int maxfds;              /* number of sockets to select on */
static int totaluser;           /* current number of connections */
static char chatbuf[256];       /* general purpose buffer */
static int common_client_command;

static char msg_not_op[] = "�� �z���O�o����ѫǪ� Op";
static char msg_no_such_id[] = "�� �ثe�S���H�ϥ� [%s] �o�Ӳ�ѥN��";
static char msg_no_such_uid[] = "�� �ثe�S�� [%s] �o�ӨϥΪ� ID";
static char msg_not_here[] = "�� [%s] ���b�o����ѫ�";

time4_t boot_time;

#define FUZZY_USER      ((ChatUser *) -1)

#undef cuser
typedef struct userec_t ACCT;

/* ----------------------------------------------------- */
/* string utilities					 */
/* ----------------------------------------------------- */
void 
chat_safe_trim(char *s, int size)
{
    int len;
    if (!s || !*s)
	return;
    len = strlen(s);
    if (len >= size)
	s[size-1] = 0;
    DBCS_safe_trim(s);
}

/* ----------------------------------------------------- */
/* acct_load for check acct                              */
/* ----------------------------------------------------- */

int
acct_load(ACCT *acct, char *userid)
{
    int id;
#ifdef SELFTEST
    memset(acct, 0, sizeof(ACCT));
    acct->userlevel |= PERM_BASIC|PERM_CHAT;
    if(random()%4==0) acct->userlevel |= PERM_CHATROOM;
    if(random()%8==0) acct->userlevel |= PERM_SYSOP;
    return atoi(userid);
#endif
    if((id=searchuser(userid, NULL))<0)
	return -1;
    return get_record(FN_PASSWD, acct, sizeof(ACCT), id);  
}

/* ----------------------------------------------------- */
/* usr_fpath for check acct                              */
/* ----------------------------------------------------- */
char *str_home_file = "home/%c/%s/%s";

void
usr_fpath(char *buf, char *userid, char *fname)
{
    sprintf(buf, str_home_file, userid[0], userid, fname);
}

int
chkpasswd(const char *passwd, const char *test)
{
    char *pw;
    char pwbuf[PASSLEN];

    strlcpy(pwbuf, test, PASSLEN);
    pw = fcrypt(pwbuf, passwd);
    return (!strncmp(pw, passwd, PASSLEN));
}

/* ----------------------------------------------------- */
/* operation log and debug information                   */
/* ----------------------------------------------------- */


static int flog;                /* log file descriptor */

#ifdef CHAT_MSG_LOGFILE
static int mlog;		/* main room logs */
#endif

static void
logit(char *key, char *msg)
{
    time4_t now;
    struct tm *p;
    char buf[512];

    now = (time4_t)time(NULL);
    p = localtime4(&now);
    snprintf(buf, sizeof(buf), "%02d/%02d %02d:%02d:%02d %-13s%s\n",
	    p->tm_mon + 1, p->tm_mday,
	    p->tm_hour, p->tm_min, p->tm_sec, key, msg);
    write(flog, buf, strlen(buf));
}


static void
log_init()
{
    flog = open(CHAT_LOGFILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    logit("START", "chat daemon");
#ifdef CHAT_MSG_LOGFILE
    {
	const char *dtstr = Cdate(&boot_time);
	const char *prefix = "\n- xchatd restart: ";
	mlog = open(CHAT_MSG_LOGFILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
	write(mlog, prefix, strlen(prefix));
	write(mlog, dtstr, strlen(dtstr));
	write(mlog, "\n", 1);
    }
#endif
}


static void
log_close()
{
    close(flog);
#ifdef CHAT_MSG_LOGFILE
    close(mlog);
#endif
}


#ifdef  DEBUG
static void
debug_user()
{
    register ChatUser *user;
    int i;
    char buf[80];

    i = 0;
    for (user = mainuser; user; user = user->unext)
    {
	snprintf(buf, sizeof(buf), "%d) %s %s", ++i, user->userid, user->chatid);
	logit("DEBUG_U", buf);
    }
}


static void
debug_room()
{
    register ChatRoom *room;
    int i;
    char buf[80];

    i = 0;
    room = &mainroom;

    do
    {
	snprintf(buf, sizeof(buf), "%d) %s %d", ++i, room->name, room->occupants);
	logit("DEBUG_R", buf);
    } while (room = room->next);
}
#endif                          /* DEBUG */


/* ----------------------------------------------------- */
/* string routines                                       */
/* ----------------------------------------------------- */


static int valid_chatid(register char *id) {
    register int ch, len;
    
    for(len = 0; (ch = *id); id++) {
	/* Thor: check for endless */
	if(ch == '/' || ch == '*' || ch == ':')
	    return 0;
	if(++len > 8)
	    return 0;
    }
    return len;
}

/* Case Independent strcmp : 1 ==> euqal */


static int
str_equal(const char *s1, const char *s2)
{
    return strcasecmp(s1, s2)==0;
}


/* ----------------------------------------------------- */
/* match strings' similarity case-insensitively          */
/* ----------------------------------------------------- */
/* str_match(keyword, string)                            */
/* ----------------------------------------------------- */
/* 0 : equal            ("foo", "foo")                   */
/* -1 : mismatch        ("abc", "xyz")                   */
/* ow : similar         ("goo", "good")                  */
/* ----------------------------------------------------- */


static int
str_match(const char *s1, const char *s2)
{
    register int c1, c2;

    for (;;)
    {                             /* Thor: check for endless */
	c2 = *s2;
	c1 = *s1;
	if (!c1)
	{
	    return c2;
	}

	if (c1 >= 'A' && c1 <= 'Z')
	    c1 |= 32;

	if (c2 >= 'A' && c2 <= 'Z')
	    c2 |= 32;

	if (c1 != c2)
	    return -1;

	s1++;
	s2++;
    }
}


/* ----------------------------------------------------- */
/* search user/room by its ID                            */
/* ----------------------------------------------------- */


static ChatUser *
cuser_by_userid(char *userid)
{
    register ChatUser *cu;

    for (cu = mainuser; cu; cu = cu->unext)
    {
	if (str_equal(userid, cu->userid))
	    break;
    }
    return cu;
}


static ChatUser *
cuser_by_chatid(char *chatid)
{
    register ChatUser *cu;

    for (cu = mainuser; cu; cu = cu->unext)
    {
	if (str_equal(chatid, cu->chatid))
	    break;
    }
    return cu;
}


static ChatUser *
fuzzy_cuser_by_chatid(char *chatid)
{
    register ChatUser *cu, *xuser;
    int mode;
    int count=0;

    xuser = NULL;

    for (cu = mainuser; cu; cu = cu->unext)
    {
	mode = str_match(chatid, cu->chatid);
	if (mode == 0)
	    return cu;

	if (mode > 0) {
	    xuser = cu;
	    count++;
	}
    }
    if(count>1) return FUZZY_USER;
    return xuser;
}


static ChatRoom *croom_by_roomid(char *roomid) {
    ChatRoom *room;
    
    for(room=&mainroom; room; room=room->next)
	if(str_equal(roomid, room->name))
	    break;
    return room;
}


/* ----------------------------------------------------- */
/* UserList routines                                     */
/* ----------------------------------------------------- */


static void
list_free(UserList *list)
{
    UserList *tmp;

    while (list)
    {
	tmp = list->next;

	free(list);
	list = tmp;
    }
}


static int
list_add_id(UserList **list, const char *id)
{
    UserList *node;
    int uid = 0;
    if (!id[0])
	return 0;

    uid = searchuser(id, NULL);
    if (uid < 1 || uid >= MAX_USERS)
	return 0;

    if((node = (UserList *) malloc(sizeof(UserList)))) {
	/* Thor: ����Ŷ����� */
	strlcpy(node->userid, id, sizeof(node->userid));
	node->userno = uid;
	node->next = *list;
	*list = node;
	return 1;
    }
    return 0;
}

static void
list_add(UserList **list, ChatUser *user)
{
    UserList *node;

    if((node = (UserList *) malloc(sizeof(UserList)))) {
	/* Thor: ����Ŷ����� */
	strcpy(node->userid, user->userid);
	node->userno = user->userno;
	node->next = *list;
	*list = node;
    }
}


static int
list_delete(UserList **list, char *userid)
{
    UserList *node;

    while((node = *list)) {
	if (str_equal(node->userid, userid))
	{
	    *list = node->next;
	    free(node);
	    return 1;
	}
	list = &node->next;         /* Thor: list�n��۫e�i */
    }

    return 0;
}


static int
list_belong(UserList *list, int userno)
{
    while (list)
    {
	if (userno == list->userno)
	    return 1;
	list = list->next;
    }
    return 0;
}

static int
list_belong_id(UserList *list, const char *userid)
{
    while (list)
    {
	if (strcasecmp(list->userid, userid) == 0)
	    return 1;
	list = list->next;
    }
    return 0;
}


/* ------------------------------------------------------ */
/* non-blocking socket routines : send message to users   */
/* ------------------------------------------------------ */


static void
Xdo_send(int nfds, fd_set *wset, char *msg)
{
    struct timeval zerotv;   /* timeval for selecting */
    int sr;

    /* Thor: for future reservation bug */

    zerotv.tv_sec = 0;
    zerotv.tv_usec = 16384;  /* Ptt: �令16384 �קK������for loop�Ycpu time
				16384 ���C��64�� */
#ifdef SELFTEST
    zerotv.tv_usec = 0;
#endif

    sr = select(nfds + 1, NULL, wset, NULL, &zerotv);

    /* FIXME �Y select() timeout, �Φ��� write ready �����S��. �h�i��|�|�� msg? */
    if (sr > 0)
    {
	register int len;

	len = strlen(msg) + 1;
	while (nfds >= 0)
	{
	    if (FD_ISSET(nfds, wset))
	    {
		send(nfds, msg, len, 0);/* Thor: �p�Gbuffer���F, ���| block */
		if (--sr <= 0)
		    return;
	    }
	    nfds--;
	}
    }
}


static void
send_to_room(ChatRoom *room, char *msg, int userno, int number)
{
    ChatUser *cu;
    fd_set wset, *wptr;
    int sock, max;

    if (number != MSG_MESSAGE && number)
	return;

    FD_ZERO(wptr = &wset);
    max = -1;

    for (cu = mainuser; cu; cu = cu->unext)
    {
	if (room == cu->room || room == ROOM_ALL)
	{
	    if (!userno || !list_belong(cu->ignore, userno))
	    {
		sock = cu->sock;
		FD_SET(sock, wptr);
		if (max < sock)
		    max = sock;
	    }
	}
    }

    if (max < 0)
	return;

#ifdef CHAT_MSG_LOGFILE
    if (room && room->name && 
	strcmp(room->name, MAIN_NAME) == 0)
    {
	time4_t now = time(0);
	const char *dtstr = Cdate_mdHM(&now);
	write(mlog, dtstr, strlen(dtstr));
	write(mlog, " ", 1);
	write(mlog, msg, strlen(msg));
	write(mlog, "\n", 1);
    }
#endif

    Xdo_send(max, wptr, msg);
}


static void
send_to_user(ChatUser *user, char *msg, int userno, int number)
{
    // BBS clients does not take non MSG_MESSAGE
    if (number && number != MSG_MESSAGE)
	return;

    if (!userno || !list_belong(user->ignore, userno))
    {
	fd_set wset, *wptr;
	int sock;

	sock = user->sock;
	FD_ZERO(wptr = &wset);
	FD_SET(sock, wptr);

	Xdo_send(sock, wptr, msg);
    }
}

/* ----------------------------------------------------- */

static void
room_changed(ChatRoom *room)
{
    if (!room)
	return;

    snprintf(chatbuf, sizeof(chatbuf), "= %s %d %d %s", room->name, room->occupants, room->rflag, room->topic);
    send_to_room(ROOM_ALL, chatbuf, 0, MSG_ROOMNOTIFY);
}

static void
user_changed(ChatUser *cu)
{
    if (!cu)
	return;

    snprintf(chatbuf, sizeof(chatbuf), "= %s %s %s %s", cu->userid, cu->chatid, cu->room->name, cu->lasthost);
    if (ROOMOP(cu))
	strcat(chatbuf, " Op");
    send_to_room(cu->room, chatbuf, 0, MSG_USERNOTIFY);
}

static void
exit_room(ChatUser *user, int mode, char *msg)
{
    ChatRoom *room;

    if(user->room == NULL)
	return;

    room = user->room;
    user->room = NULL;
    user->uflag &= ~PERM_ROOMOP;

    room->occupants--;

    if (room->occupants > 0)
    {
	char *chatid;

	chatid = user->chatid;
	switch (mode)
	{
	    case EXIT_LOGOUT:

		sprintf(chatbuf, "�� %s (%s) ���}�F ...", chatid, user->userid);
		if (msg && *msg)
		{
		    strcat(chatbuf, ": ");
		    strncat(chatbuf, msg, 80);
		}
		break;

	    case EXIT_LOSTCONN:

		sprintf(chatbuf, "�� %s (%s) ���F�_�u�������o", chatid, user->userid);
		break;

	    case EXIT_KICK:

		sprintf(chatbuf, "�� �����I%s (%s) �Q��X�h�F", chatid, user->userid);
		break;
	}
	if (!CLOAK(user))         /* Thor: ��ѫ������N */
	    send_to_room(room, chatbuf, user->userno, MSG_MESSAGE);

	if (list_belong(room->invite, user->userno)) {
	    list_delete(&(room->invite), user->userid);
	}

	sprintf(chatbuf, "- %s", user->userid);
	send_to_room(room, chatbuf, user->userno, MSG_USERNOTIFY);
	room_changed(room);

	return;
    }

    /* Now, room->occupants==0 */
    if (room != &mainroom)
    {                           /* Thor: �H�Ƭ�0��,���Omainroom�~free */
	register ChatRoom *next;

	sprintf(chatbuf, "- %s", room->name);
	send_to_room(ROOM_ALL, chatbuf, 0, MSG_ROOMNOTIFY);

	room->prev->next = room->next;
	if((next = room->next))
	    next->prev = room->prev;
	list_free(room->invite);

	free(room);
    }
}


/* ----------------------------------------------------- */
/* chat commands                                         */
/* ----------------------------------------------------- */

/* ----------------------------------------------------- */
/* (.ACCT) �ϥΪ̱b�� (account) subroutines              */
/* ----------------------------------------------------- */

static void
chat_topic(ChatUser *cu, char *msg)
{
    ChatRoom *room;
    char *topic;

    if (!ROOMOP(cu) && !OPENTOPIC(cu->room))
    {
	send_to_user(cu, msg_not_op, 0, MSG_MESSAGE);
	return;
    }

    if (*msg == '\0')
    {
	send_to_user(cu, "�� �Ы��w���D", 0, MSG_MESSAGE);
	return;
    }

    room = cu->room;
    assert(room);
    topic = room->topic;
    strlcpy(topic, msg, sizeof(room->topic));
    DBCS_safe_trim(topic);

    sprintf(chatbuf, "/t%s", topic);
    send_to_room(room, chatbuf, 0, 0);

    room_changed(room);

    sprintf(chatbuf, "�� %s �N���D�אּ " ANSI_COLOR(1;32) "%s" ANSI_RESET, cu->chatid, topic);
    if (!CLOAK(cu))               /* Thor: ��ѫ������N */
	send_to_room(room, chatbuf, 0, MSG_MESSAGE);
}


static void
chat_version(ChatUser *cu, char *msg)
{
    sprintf(chatbuf, "%d %d", XCHAT_VERSION_MAJOR, XCHAT_VERSION_MINOR);
    send_to_user(cu, chatbuf, 0, MSG_VERSION);
}

static void
chat_xinfo(ChatUser *cu, char *msg)
{
    // report system information
    time4_t uptime = time(0) - boot_time;
    int dd =  uptime / DAY_SECONDS, 
	hh = (uptime % DAY_SECONDS) / 3600,
	mm = (uptime % 3600) / 60;
    sprintf(chatbuf, "�� �t�θ�T: XCHAT ���� %d.%02d - " __DATE__ 
	    "�A�w���� %d �� %d �p�� %d ��",
	    XCHAT_VERSION_MAJOR, XCHAT_VERSION_MINOR, dd, hh, mm);
    send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
}

static void
chat_nick(ChatUser *cu, char *msg)
{
    char *chatid;
    ChatUser *xuser;

    chatid = nextword(&msg);
    chat_safe_trim(chatid, sizeof(cu->chatid));

    if (!valid_chatid(chatid))
    {
	send_to_user(cu, "�� �o�Ӳ�ѥN���O�����T��", 0, MSG_MESSAGE);
	return;
    }

    xuser = cuser_by_chatid(chatid);
    if (xuser != NULL && xuser != cu)
    {
	send_to_user(cu, "�� �w�g���H�������n�o", 0, MSG_MESSAGE);
	return;
    }

    snprintf(chatbuf, sizeof(chatbuf), "�� %s �N��ѥN���אּ " ANSI_COLOR(1;33) "%s" ANSI_RESET, cu->chatid, chatid);
    if (!CLOAK(cu))               /* Thor: ��ѫ������N */
	send_to_room(cu->room, chatbuf, cu->userno, MSG_MESSAGE);

    strlcpy(cu->chatid, chatid, sizeof(cu->chatid));
    user_changed(cu);

    snprintf(chatbuf, sizeof(chatbuf), "/n%s", chatid);
    send_to_user(cu, chatbuf, 0, 0);
}

static void
chat_list_rooms(ChatUser *cuser, char *msg)
{
    ChatRoom *cr;

    if (RESTRICTED(cuser))
    {
	send_to_user(cuser, "�� �z�S���v���C�X�{������ѫ�", 0, MSG_MESSAGE);
	return;
    }

    if (common_client_command)
	send_to_user(cuser, "", 0, MSG_ROOMLISTSTART);
    else
	send_to_user(cuser, ANSI_COLOR(7) " ��ѫǦW��  �x�H�Ƣx���D "
		// 48-4 spaces for max topic length
		"                                        "  
		ANSI_RESET, 0, MSG_MESSAGE);

    for(cr = &mainroom; cr; cr = cr->next) {
	if (!SECRET(cr) || CHATSYSOP(cuser) || (cr == cuser->room && ROOMOP(cuser)))
	{
	    if (common_client_command)
	    {
		snprintf(chatbuf, sizeof(chatbuf), "%s %d %d %s", cr->name, cr->occupants, cr->rflag, cr->topic);
		send_to_user(cuser, chatbuf, 0, MSG_ROOMLIST);
	    }
	    else
	    {
		snprintf(chatbuf, sizeof(chatbuf), " %-12s�x%4d�x%s", cr->name, cr->occupants, cr->topic);
		if (LOCKED(cr))
		    strcat(chatbuf, " [���]");
		if (SECRET(cr))
		    strcat(chatbuf, " [���K]");
		if (OPENTOPIC(cr))
		    strcat(chatbuf, " [���D]");
		send_to_user(cuser, chatbuf, 0, MSG_MESSAGE);
	    }

	}
    }

    if (common_client_command)
	send_to_user(cuser, "", 0, MSG_ROOMLISTEND);
}


static void
chat_do_user_list(ChatUser *cu, char *msg, ChatRoom *theroom)
{
    ChatRoom *myroom, *room;
    ChatUser *user;

    int start, stop, curr = 0;
    start = atoi(nextword(&msg));
    stop = atoi(nextword(&msg));

    myroom = cu->room;

#ifdef DEBUG
    logit(cu->chatid, "do user list");
#endif

    if (common_client_command)
	send_to_user(cu, "", 0, MSG_USERLISTSTART);
    else
	send_to_user(cu, ANSI_COLOR(7) " ��ѥN���x�ϥΪ̥N��  �x��ѫ�      " ANSI_RESET, 0, MSG_MESSAGE);

    for (user = mainuser; user; user = user->unext)
    {

	room = user->room;
	if ((theroom != ROOM_ALL) && (theroom != room))
	    continue;

	if (myroom != room)
	{
	    if (RESTRICTED(cu) ||
		(room && SECRET(room) && !CHATSYSOP(cu)))
		continue;
	}

	if (CLOAK(user))            /* Thor: �����N */
	    continue;


	curr++;
	if (start && curr < start)
	    continue;
	else if (stop && (curr > stop))
	    break;

	if (common_client_command)
	{
	    if (!room)
		continue;               /* Xshadow: �٨S�i�J����ж����N���C�X */

	    snprintf(chatbuf, sizeof(chatbuf), "%s %s %s %s", user->chatid, user->userid, room->name, user->lasthost);
	    if (ROOMOP(user))
		strcat(chatbuf, " Op");
	}
	else
	{
	    snprintf(chatbuf, sizeof(chatbuf), " %-8s�x%-12s�x%s", user->chatid, user->userid, room ? room->name : "[�b���f�r��]");
	    if (ROOMOP(user))
		strcat(chatbuf, " [Op]");
	}

#ifdef  DEBUG
	logit("list_U", chatbuf);
#endif

	send_to_user(cu, chatbuf, 0, common_client_command ? MSG_USERLIST : MSG_MESSAGE);
    }
    if (common_client_command)
	send_to_user(cu, "", 0, MSG_USERLISTEND);
}

static void
chat_list_by_room(ChatUser *cu, char *msg)
{
    ChatRoom *whichroom;
    char *roomstr;

    roomstr = nextword(&msg);
    if (*roomstr == '\0')
	whichroom = cu->room;
    else
    {
	if ((whichroom = croom_by_roomid(roomstr)) == NULL)
	{
	    snprintf(chatbuf, sizeof(chatbuf), "�� �S�� [%s] �o�Ӳ�ѫ�", roomstr);
	    send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
	    return;
	}

	if (whichroom != cu->room && SECRET(whichroom) && !CHATSYSOP(cu))
	{                           /* Thor: �n���n���P�@room��SECRET���i�H�C?
				     * Xshadow: �ڧ令�P�@ room �N�i�H�C */
	    send_to_user(cu, "�� �L�k�C�X�b���K��ѫǪ��ϥΪ�", 0, MSG_MESSAGE);
	    return;
	}
    }
    chat_do_user_list(cu, msg, whichroom);
}


static void
chat_list_users(ChatUser *cu, char *msg)
{
    chat_do_user_list(cu, msg, ROOM_ALL);
}

static void
chat_chatroom(ChatUser *cu, char *msg)
{
    if (common_client_command)
	send_to_user(cu, "��������] 4 21", 0, MSG_CHATROOM);
}

static void
chat_map_chatids(ChatUser *cu, ChatRoom *whichroom) /* Thor: �٨S���@���P���� */
{
    int c;
    ChatRoom *myroom, *room;
    ChatUser *user;

    myroom = whichroom;
    send_to_user(cu,
		 ANSI_COLOR(7) " ��ѥN�� �ϥΪ̥N��  �x ��ѥN�� �ϥΪ̥N��  �x ��ѥN�� �ϥΪ̥N�� " ANSI_RESET, 0, MSG_MESSAGE);

    c = 0;

    for (user = mainuser; user; user = user->unext)
    {
	room = user->room;
	if (whichroom != ROOM_ALL && whichroom != room)
	    continue;
	if (myroom != room)
	{
	    if (RESTRICTED(cu) ||     /* Thor: �n��check room �O���O�Ū� */
		(room && SECRET(room) && !CHATSYSOP(cu)))
		continue;
	}
	if (CLOAK(user))            /* Thor:�����N */
	    continue;
	sprintf(chatbuf + (c * 24), " %-8s%c%-12s%s",
		user->chatid, ROOMOP(user) ? '*' : ' ',
		user->userid, (c < 2 ? "�x" : "  "));
	if (++c == 3)
	{
	    send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
	    c = 0;
	}
    }
    if (c > 0)
	send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
}


static void
chat_map_chatids_thisroom(ChatUser *cu, char *msg)
{
    chat_map_chatids(cu, cu->room);
}


static void
chat_setroom(ChatUser *cu, char *msg)
{
    char *modestr;
    ChatRoom *room;
    char *chatid;
    int sign;
    int flag;
    char *fstr = NULL;

    if (!ROOMOP(cu))
    {
	send_to_user(cu, msg_not_op, 0, MSG_MESSAGE);
	return;
    }

    modestr = nextword(&msg);
    sign = 1;
    if (*modestr == '+')
	modestr++;
    else if (*modestr == '-')
    {
	modestr++;
	sign = 0;
    }
    if (*modestr == '\0')
    {
	send_to_user(cu,
		     "�� �Ы��w���A: {[+(�]�w)][-(����)]}{[l(���)][s(���K)][t(�}����D)}", 0, MSG_MESSAGE);
	return;
    }

    room = cu->room;
    chatid = cu->chatid;

    while (*modestr)
    {
	flag = 0;
	switch (*modestr)
	{
	case 'l':
	case 'L':
	    flag = ROOM_LOCKED;
	    fstr = "���";
	    break;

	case 's':
	case 'S':
	    flag = ROOM_SECRET;
	    fstr = "���K";
	    break;

	case 't':
	case 'T':
	    flag = ROOM_OPENTOPIC;
	    fstr = "�}����D";
	    break;
	case 'h':
	case 'H':
	    flag = ROOM_HANDUP;
	    fstr = "�|��o��";
	    break;

	default:
	    sprintf(chatbuf, "�� ���A���~�G[%c]", *modestr);
	    send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
	}

	/* Thor: check room �O���O�Ū�, ���Ӥ��O�Ū� */
	if (flag && (room->rflag & flag) != sign * flag)
	{
	    room->rflag ^= flag;
	    sprintf(chatbuf, "�� ����ѫǳQ %s %s [%s] ���A",
		    chatid, sign ? "�]�w��" : "����", fstr);
	    if (!CLOAK(cu))           /* Thor: ��ѫ������N */
		send_to_room(room, chatbuf, 0, MSG_MESSAGE);
	}
	modestr++;
    }
    room_changed(room);
}

static void
chat_private(ChatUser *cu, char *msg)
{
    char *recipient;
    ChatUser *xuser;
    int userno;

    userno = 0;
    recipient = nextword(&msg);
    xuser = (ChatUser *) fuzzy_cuser_by_chatid(recipient);
    if (xuser == NULL)
    {                             /* Thor.0724: �� userid�]�i�Ǯ����� */
	xuser = cuser_by_userid(recipient);
    }

    if (xuser == NULL)
    {
	sprintf(chatbuf, msg_no_such_id, recipient);
    }
    else if (xuser == FUZZY_USER)
    {                             /* ambiguous */
	strcpy(chatbuf, "�� �Ы�����ѥN��");
    }
    else if (*msg)
    {
	userno = cu->userno;

	// XXX valid size: ����J�ɭn�b NICK: /m MYNICK MSG
	// �ҥH��J�� MSG �̤j�� 80-NICKLEN(8)*2-5
	// prefix ���r��n�p�ߤ���L��

	sprintf(chatbuf, COLOR_PRIVATEMSG "*%s (%s)*" ANSI_RESET " ", cu->chatid, cu->userid);
	strlcat(chatbuf, msg, sizeof(chatbuf));
	send_to_user(xuser, chatbuf, userno, MSG_MESSAGE);

	sprintf(chatbuf, COLOR_PRIVATEMSG "%s" ANSI_RESET "> ", xuser->chatid);
	strlcat(chatbuf, msg, sizeof(chatbuf));
    }
    else
    {
	sprintf(chatbuf, "�� �z�Q�� %s ������ܩO�H", xuser->chatid);
    }
    send_to_user(cu, chatbuf, userno, MSG_MESSAGE);       /* Thor: userno �n�令 0
							   * ��? */
}

static void
chat_query(ChatUser *cu, char *msg)
{
    char *recipient;
    ChatUser *xuser;

    recipient = nextword(&msg);
    xuser = (ChatUser *) fuzzy_cuser_by_chatid(recipient);
    if (xuser == NULL)
	xuser = cuser_by_userid(recipient);
    if (xuser == NULL)
	sprintf(chatbuf, msg_no_such_id, recipient);
    else if (xuser == FUZZY_USER)
	strcpy(chatbuf, "�� �вM��������誺��ѥN��"); // ambiguous
    else 
    {
	snprintf(chatbuf, sizeof(chatbuf),
		"�� ��Ѽʺ�: %s �A" BBSMNAME " ID: %s (%s)",
		xuser->chatid, xuser->userid, xuser->nickname);
	send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
	snprintf(chatbuf, sizeof(chatbuf),
		"   " STR_LOGINDAYS " %d " STR_LOGINDAYS_QTY "�A"
		"�o��L %d �g�峹�A�̪�q %s �W��",
		xuser->numlogindays, xuser->numposts, xuser->lasthost);
    }
    send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
}


static void
chat_cloak(ChatUser *cu, char *msg)
{
    if (CHATSYSOP(cu))
    {
	cu->uflag ^= PERM_CLOAK;
	sprintf(chatbuf, "�� %s", CLOAK(cu) ? MSG_CLOAKED : MSG_UNCLOAK);
	send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
    }
}



/* ----------------------------------------------------- */


static void
arrive_room(ChatUser *cuser, ChatRoom *room)
{
    char *rname;

    /* Xshadow: �����e���ۤv, �ϥ����ж��N�|���s build user list */
    snprintf(chatbuf, sizeof(chatbuf), "+ %s %s %s %s", cuser->userid, cuser->chatid, room->name, cuser->lasthost);
    if (ROOMOP(cuser))
	strcat(chatbuf, " Op");
    send_to_room(room, chatbuf, 0, MSG_USERNOTIFY);

    cuser->room = room;
    room->occupants++;
    rname = room->name;

    room_changed(room);

    sprintf(chatbuf, "/r%s", rname);
    send_to_user(cuser, chatbuf, 0, 0);
    sprintf(chatbuf, "/t%s", room->topic);
    send_to_user(cuser, chatbuf, 0, 0);

    sprintf(chatbuf, "�� " ANSI_COLOR(1;32) "%s (%s)" ANSI_RESET " �i�J "
	    ANSI_COLOR(1;33) "[%s]" ANSI_RESET " �]�[",
	    cuser->chatid, cuser->userid, rname);
    if (!CLOAK(cuser))            /* Thor: ��ѫ������N */
	send_to_room(room, chatbuf, cuser->userno, MSG_MESSAGE);
}


static int
enter_room(ChatUser *cuser, char *rname, char *msg)
{
    ChatRoom *room;
    int create;

    create = 0;

    // truncate names
    chat_safe_trim(rname, sizeof(room->name));

    room = croom_by_roomid(rname);
    if (room == NULL)
    {
	/* new room */

#ifdef  MONITOR
	logit(cuser->userid, "create new room");
#endif

	room = (ChatRoom *) malloc(sizeof(ChatRoom));
	if (room == NULL)
	{
	    send_to_user(cuser, "�� �L�k�A�s�P�]�[�F", 0, MSG_MESSAGE);
	    return 0;
	}

	memset(room, 0, sizeof(ChatRoom));
	strlcpy(room->name, rname, sizeof(room->name));
	strcpy(room->topic, "�o�O�@�ӷs�Ѧa");

	snprintf(chatbuf, sizeof(chatbuf), "+ %s 1 0 %s", room->name, room->topic);
	send_to_room(ROOM_ALL, chatbuf, 0, MSG_ROOMNOTIFY);

	if (mainroom.next != NULL)
	    mainroom.next->prev = room;
	room->next = mainroom.next;
	mainroom.next = room;
	room->prev = &mainroom;

	create = 1;
    }
    else
    {
	if (cuser->room == room)
	{
	    sprintf(chatbuf, "�� �z���ӴN�b [%s] ��ѫ��o :)", rname);
	    send_to_user(cuser, chatbuf, 0, MSG_MESSAGE);
	    return 0;
	}

	if (!CHATSYSOP(cuser) && 
	    (list_belong(room->ban, cuser->userno) ||
	     (LOCKED(room) && !list_belong(room->invite, cuser->userno))))
	{
	    send_to_user(cuser, "�� �����c���A�D�в��J", 0, MSG_MESSAGE);
	    return 0;
	}
    }

    exit_room(cuser, EXIT_LOGOUT, msg);
    arrive_room(cuser, room);

    if (create)
	cuser->uflag |= PERM_ROOMOP;

    return 0;
}


static void
logout_user(ChatUser *cuser)
{
    int sock;
    ChatUser *xuser, *prev;

    sock = cuser->sock;
    shutdown(sock, 2);
    close(sock);

    FD_CLR(sock, &mainfds);

#if 0   /* Thor: �]�\���t�o�@�� */
    if (sock >= maxfds)
	maxfds = sock - 1;
#endif

    list_free(cuser->ignore);

    xuser = mainuser;
    if (xuser == cuser)
    {
	mainuser = cuser->unext;
    }
    else
    {
	do
	{
	    prev = xuser;
	    xuser = xuser->unext;
	    if (xuser == cuser)
	    {
		prev->unext = cuser->unext;
		break;
	    }
	} while (xuser);
    }

#ifdef DEBUG
    sprintf(chatbuf, "%p", cuser);
    logit("free cuser", chatbuf);
#endif

    free(cuser);

    totaluser--;
}


static void
print_user_counts(ChatUser *cuser)
{
    ChatRoom *room;
    int num, userc, suserc, roomc, number;

    userc = suserc = roomc = 0;

    for(room = &mainroom; room; room = room->next) {
	num = room->occupants;
	if (SECRET(room))
	{
	    suserc += num;
	    if (CHATSYSOP(cuser))
		roomc++;
	}
	else
	{
	    userc += num;
	    roomc++;
	}
    }

    number = MSG_MESSAGE;

    // welcome message here.
    sprintf(chatbuf, "�� �w����{�i�����ѫǡj�A�ثe�}�F " 
	    ANSI_COLOR(1;31) "%d" ANSI_RESET " ���]�[�C", roomc);
    send_to_user(cuser, chatbuf, 0, number);

    sprintf(chatbuf, "�� �u�W�� " ANSI_COLOR(1;36) "%d" ANSI_RESET " �H", userc);
    if (suserc)
	sprintf(chatbuf + strlen(chatbuf), " [%d �H�b���K��ѫ�]", suserc);
    send_to_user(cuser, chatbuf, 0, number);
}


static int
login_user(ChatUser *cu, char *msg)
{
    int utent;

    char *passwd;
    char *userid;
    char *chatid;

    // deprecated: we don't lookup real host now.
#if 0
    struct sockaddr_in from;
    unsigned int fromlen;
    struct hostent *hp;
#endif

    ACCT acct;
    int level;

    /*
     * Thor.0819: SECURED_CHATROOM : /! userid chatid passwd , userno
     * el �bcheck��passwd����o
     */
    /* Xshadow.0915: common client support : /-! userid chatid password */

    /* �ǰѼơGuserlevel, userid, chatid */

    /* client/server �����̾� userid �� .PASSWDS �P�_ userlevel */

    userid = nextword(&msg);
    chatid = nextword(&msg);

    chat_safe_trim(chatid, sizeof(cu->chatid));

#ifdef  DEBUG
    logit("ENTER", userid);
#endif
    /* Thor.0730: parse space before passwd */
    passwd = msg;

    /* Thor.0813: ���L�@�Ů�Y�i, �]���ϥ��p�Gchatid���Ů�, �K�X�]���� */
    /* �N��K�X��, �]���|����:p */
    /* �i�O�p�G�K�X�Ĥ@�Ӧr�O�Ů�, �����Ӧh�Ů�|�i����... */
    if (*passwd == ' ')
	passwd++;

    /* Thor.0729: load acct */
    if (!*userid || (acct_load(&acct, userid) < 0))
    {

#ifdef  DEBUG
	logit("noexist", chatid);
#endif

	send_to_user(cu, CHAT_LOGIN_INVALID, 0, 0);

	return -1;
    }
    else if(strncmp(passwd, acct.passwd, PASSLEN) &&
	    !chkpasswd(acct.passwd, passwd))
    {
#ifdef  DEBUG
	logit("fake", chatid);
#endif

	send_to_user(cu, CHAT_LOGIN_INVALID, 0, 0);
	return -1;
    }

    /* Thor.0729: if ok, read level.  */
    level = acct.userlevel;
    /* Thor.0819: read userno for client/server bbs */
#ifdef SELFTEST
    utent = atoi(userid)+1;
#else
    utent = searchuser(acct.userid, NULL);
#endif
    assert(utent);

    if (!valid_chatid(chatid))
    {
#ifdef  DEBUG
	logit("enter", chatid);
#endif

	send_to_user(cu, CHAT_LOGIN_INVALID, 0, 0);
	return 0;
    }

    if (cuser_by_chatid(chatid) != NULL)
    {
	/* chatid in use */

#ifdef  DEBUG
	logit("enter", "duplicate");
#endif

	send_to_user(cu, CHAT_LOGIN_EXISTS, 0, 0);
	return 0;
    }

    cu->userno = utent;
    cu->uflag = level & ~(PERM_ROOMOP | PERM_CLOAK | PERM_HANDUP | PERM_SAY);
    /* Thor: �i�ӥ��M��ROOMOP(�PPERM_CHAT), CLOAK */
    strcpy(cu->userid, userid);
    strlcpy(cu->chatid, chatid, sizeof(cu->chatid));
    // fill user information from acct
    strlcpy(cu->nickname, acct.nickname, sizeof(cu->nickname));
    cu->numposts = acct.numposts;
    cu->numlogindays = acct.numlogindays;
    strlcpy(cu->lasthost, acct.lasthost, sizeof(cu->lasthost));

    // deprecated: let's use BBS lasthost
#if 0
    /* Xshadow: ���o client ���ӷ� */
    fromlen = sizeof(from);
    if (!getpeername(cu->sock, (struct sockaddr *) & from, &fromlen))
    {
	if ((hp = gethostbyaddr((char *) &from.sin_addr, sizeof(struct in_addr), from.sin_family)))
	    strlcpy(cu->lasthost, hp->h_name, sizeof(cu->lasthost));
	else
	    strlcpy(cu->lasthost, (char *) inet_ntoa(from.sin_addr), sizeof(cu->lasthost));
    }
    else
	strcpy(cu->lasthost, "[�~�Ӫ�]");
#endif

    send_to_user(cu, CHAT_LOGIN_OK, 0, 0);
    arrive_room(cu, &mainroom);

    send_to_user(cu, "", 0, MSG_MOTDSTART);
    print_user_counts(cu);
    send_to_user(cu, "", 0, MSG_MOTDEND);

#ifdef  DEBUG
    logit("enter", "OK");
#endif

    return 0;
}


static void
chat_act(ChatUser *cu, char *msg)
{
    if (*msg && (!RHANDUP(cu->room) || SAY(cu) || ROOMOP(cu)))
    {
	sprintf(chatbuf, "%s " ANSI_COLOR(36) "%s" ANSI_RESET, cu->chatid, msg);
	send_to_room(cu->room, chatbuf, cu->userno, MSG_MESSAGE);
    }
}


static void
chat_ignore(ChatUser *cu, char *msg)
{

    if (RESTRICTED(cu))
    {
	strcpy(chatbuf, "�� �z�S�� ignore �O�H���v�Q");
    }
    else
    {
	char *ignoree;

	ignoree = nextword(&msg);
	if (*ignoree)
	{
	    ChatUser *xuser;
	    xuser = cuser_by_userid(ignoree);
	    if (!xuser) xuser = cuser_by_chatid(ignoree);

	    if (list_belong_id(cu->ignore, ignoree))
	    {
		sprintf(chatbuf, "�� %s �w�g�Q�ᵲ�F", ignoree);
	    } 
	    else if (xuser == NULL)
	    {
		// try more harder to see if this user can be
		// pre-ignored. Do not use xuser!
		if (list_add_id(&(cu->ignore), ignoree))
		    sprintf(chatbuf, "�� %s �w�g�Q�ᵲ�F", ignoree);
		else
		    sprintf(chatbuf, msg_no_such_uid, ignoree);
	    }
	    else if (xuser == cu || CHATSYSOP(xuser) ||
		     (ROOMOP(xuser) && (xuser->room == cu->room)))
	    {
		sprintf(chatbuf, "�� ���i�H���� %s (%s)", 
			xuser->chatid, xuser->userid);
	    }
	    else
	    {

		if (list_belong(cu->ignore, xuser->userno))
		{
		    sprintf(chatbuf, "�� %s (%s) �w�g�Q�ᵲ�F", 
			    xuser->chatid, xuser->userid);
		}
		else
		{
		    list_add(&(cu->ignore), xuser);
		    sprintf(chatbuf, "�� �N [%s] ���J�N�c�F :p", xuser->chatid);
		}
	    }
	}
	else
	{
	    UserList *list;

	    if((list = cu->ignore))
	    {
		int len;
		char buf[16];

		send_to_user(cu, "�� �o�ǤH�Q���J�N�c�F�G", 0, MSG_MESSAGE);
		len = 0;
		do
		{
		    sprintf(buf, "%-13s", list->userid);
		    strcpy(chatbuf + len, buf);
		    len += 13;
		    if (len >= 78)
		    {
			send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
			len = 0;
		    }
		} while((list = list->next));

		if (len == 0)
		    return;
	    }
	    else
	    {
		strcpy(chatbuf, "�� �z�ثe�èS�� ignore ����H");
	    }
	}
    }

    send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
}


static void
chat_unignore(ChatUser *cu, char *msg)
{
    char *ignoree;

    ignoree = nextword(&msg);

    if (*ignoree)
    {
	sprintf(chatbuf, (list_delete(&(cu->ignore), ignoree)) ?
		"�� [%s] ���A�Q�A�N���F" :
		"�� �z�å����� [%s]�A�Х� /ignore �ˬd�C��C", ignoree);
    }
    else
    {
	strcpy(chatbuf, "�� �Ы����ϥΪ� ID");
    }
    send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
}

static void
chat_unban(ChatUser *cu, char *msg)
{
    char *unban;
    UserList **list;

    if (!ROOMOP(cu))
    {
	send_to_user(cu, msg_not_op, 0, MSG_MESSAGE);
	return;
    }

    unban = nextword(&msg);
    list = &(cu->room->ban);

    if (*unban)
    {
	sprintf(chatbuf, (list_delete(list, unban)) ?
		"�� [%s] ���A�Q�C���¦W��" :
		"�� [%s] �ä��b�¦W�椤�A�Х� /ban �ˬd�C��", unban);
    }
    else
    {
	strcpy(chatbuf, "�� �Ы����ϥΪ� ID");
    }
    send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
}


static void
chat_join(ChatUser *cu, char *msg)
{
    if (RESTRICTED(cu))
    {
	send_to_user(cu, "�� �z�S���[�J��L��ѫǪ��v��", 0, MSG_MESSAGE);
    }
    else
    {
	char *roomid = nextword(&msg);

	if (*roomid)
	    enter_room(cu, roomid, msg);
	else
	    send_to_user(cu, "�� �Ы��w��ѫǪ��W�r", 0, MSG_MESSAGE);
    }
}


static void
chat_kick(ChatUser *cu, char *msg)
{
    char *twit;
    ChatUser *xuser;
    ChatRoom *room;

    if (!ROOMOP(cu))
    {
	send_to_user(cu, msg_not_op, 0, MSG_MESSAGE);
	return;
    }

    twit = nextword(&msg);
    xuser = cuser_by_chatid(twit);

    if (xuser == NULL)
    {
	sprintf(chatbuf, msg_no_such_id, twit);
	send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
	return;
    }

    room = cu->room;
    if (room != xuser->room || CLOAK(xuser))
    {                             /* Thor: ��ѫ������N */
	sprintf(chatbuf, msg_not_here, twit);
	send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
	return;
    }

    if (CHATSYSOP(xuser))
    {                             /* Thor: �𤣨� CHATSYSOP */
	sprintf(chatbuf, "�� ���i�H kick [%s]", twit);
	send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
	return;
    }

    exit_room(xuser, EXIT_KICK, (char *) NULL);

    if (room == &mainroom)
	logout_user(xuser);
    else
	enter_room(xuser, MAIN_NAME, (char *) NULL);
}


static void
chat_makeop(ChatUser *cu, char *msg)
{
    char *newop;
    ChatUser *xuser;
    ChatRoom *room;
    int wasop = 0;

    if (!ROOMOP(cu))
    {
	send_to_user(cu, msg_not_op, 0, MSG_MESSAGE);
	return;
    }

    newop = nextword(&msg);
    xuser = cuser_by_chatid(newop);

    if (xuser == NULL)
    {
	sprintf(chatbuf, msg_no_such_id, newop);
	send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
	return;
    }

    if (cu == xuser)
    {
	sprintf(chatbuf, "�� �z�w�g�O�޲z��(Op)�F�A�L�k���ܦۤv���v��");
	send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
	return;
    }

    room = cu->room;

    if (room != xuser->room || CLOAK(xuser))
    {                             /* Thor: ��ѫ������N */
	sprintf(chatbuf, msg_not_here, xuser->chatid);
	send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
	return;
    }

    wasop = ROOMOP(xuser) ? 1 : 0;
    xuser->uflag ^= PERM_ROOMOP;
    user_changed(xuser);

    if (wasop == (ROOMOP(xuser) ? 1 : 0))
    {
	// �ʤ��F�L
	sprintf(chatbuf, "�� �L�k���ܹ�誺�޲z��(Op)�v���C");
	send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
	return;
    }

    sprintf(chatbuf, "�� %s %s %s ���޲z��(Op)�v��",
	    cu->chatid, 
	    ROOMOP(xuser) ? "���@�F" : "�Ѱ��F",
	    xuser->chatid);

    if (!CLOAK(cu))               /* Thor: ��ѫ������N */
	send_to_room(room, chatbuf, 0, MSG_MESSAGE);
}



static void
chat_invite(ChatUser *cu, char *msg)
{
    char *invitee;
    ChatUser *xuser;
    ChatRoom *room;
    UserList **list;

    if (!ROOMOP(cu))
    {
	send_to_user(cu, msg_not_op, 0, MSG_MESSAGE);
	return;
    }

    invitee = nextword(&msg);
    xuser = cuser_by_chatid(invitee);
    if (xuser == NULL)
    {
	sprintf(chatbuf, msg_no_such_id, invitee);
	send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
	return;
    }

    room = cu->room;
    assert(room);
    list = &(room->invite);

    if (list_belong(*list, xuser->userno))
    {
	sprintf(chatbuf, "�� %s �w�g�����L�ܽФF", xuser->chatid);
	send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
	return;
    }
    list_add(list, xuser);

    sprintf(chatbuf, "�� %s �ܽбz�� [%s] ��ѫ�",
	    cu->chatid, room->name);
    send_to_user(xuser, chatbuf, 0, MSG_MESSAGE); /* Thor: �n���n�i�H ignore? */
    sprintf(chatbuf, "�� %s ����z���ܽФF", xuser->chatid);
    send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
}

static void
chat_ban(ChatUser *cu, char *msg)
{
    char *banned;
    ChatUser *xuser;
    ChatRoom *room;
    UserList **list;
    int unum = 0;

    banned = nextword(&msg);

    if (!banned || !*banned)
    {
	// list all
	UserList *list;
	if(!(list = cu->room->ban))
	{
	    strcpy(chatbuf, "�� �ثe�¦W��O�Ū�");
	} 
	else 
	{
	    int len;
	    char buf[16];

	    send_to_user(cu, "�� �¦W��C��G", 0, MSG_MESSAGE);
	    len = 0;
	    do
	    {
		sprintf(buf, "%-13s", list->userid);
		strcpy(chatbuf + len, buf);
		len += 13;
		if (len >= 78)
		{
		    send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
		    len = 0;
		}
	    } while((list = list->next));
	    chatbuf[len] = 0;
	}
	if (chatbuf[0])
	    send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
	return;
    }

    if (!ROOMOP(cu))
    {
	send_to_user(cu, msg_not_op, 0, MSG_MESSAGE);
	return;
    }
    xuser = cuser_by_chatid(banned);

    if (!xuser)
	unum = searchuser(banned, NULL);
    else
	unum = xuser->userno;

    if (!unum)
    {
	sprintf(chatbuf, msg_no_such_id, banned);
	send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
	return;
    }

    room = cu->room;
    assert(room);
    list = &(room->ban);

    if (list_belong(*list, unum))
    {
	sprintf(chatbuf, "�� %s �w�g�b�¦W�椺�F", banned);
	send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
	return;
    }

    if (xuser)
    {
	list_add(list, xuser);
	sprintf(chatbuf, "�� %s (%s) �w�Q�[�J�¦W��", xuser->chatid, xuser->userid);
    }
    else
    {
	list_add_id(list, banned);
	sprintf(chatbuf, "�� %s �w�Q�[�J�¦W��", banned);
    }

    send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
}


static void
chat_broadcast(ChatUser *cu, char *msg)
{
    if (!CHATSYSOP(cu))
    {
	send_to_user(cu, "�� �z�S���b��ѫǼs�����v�O!", 0, MSG_MESSAGE);
	return;
    }
    if (*msg == '\0')
    {
	send_to_user(cu, "�� �Ы��w�s�����e", 0, MSG_MESSAGE);
	return;
    }
    sprintf(chatbuf, ANSI_COLOR(1) "�� " BBSNAME "��ѫǼs���� [%s]....." ANSI_RESET,
	    cu->chatid);
    send_to_room(ROOM_ALL, chatbuf, 0, MSG_MESSAGE);
    sprintf(chatbuf, "�� %s", msg);
    send_to_room(ROOM_ALL, chatbuf, 0, MSG_MESSAGE);
}


static void
chat_goodbye(ChatUser *cu, char *msg)
{
    exit_room(cu, EXIT_LOGOUT, msg);
    /* Thor: �n���n�[ logout_user(cu) ? */
}


/* --------------------------------------------- */
/* MUD-like social commands : action             */
/* --------------------------------------------- */

struct ChatAction
{
    char *verb;                   /* �ʵ� */
    char *chinese;                /* ����½Ķ */
    char *part1_msg;              /* ���� */
    char *part2_msg;              /* �ʧ@ */
};


static ChatAction party_data[] =
{
    {"aluba", "���|��", "��", "�[�W�W�l���|��!!"},
    {"adore", "����", "��", "���������p�ʷʦ���,�s�������K�K"},
    {"bearhug", "����", "�������֩�", ""},
    {"blade", "�@�M", "�@�M�ҵ{��", "�e�W���"},
    {"bless", "����", "����", "�߷Q�Ʀ�"},
    {"board", "�D���O", "��", "��h���D���O"},
    {"bokan", "��\\", "���x�L�X�A�W�իݵo�K�K��M���A�q���E�{�A��", "�ϥX�F��o--��an�I"},
  {"bow", "���`", "���`���q���V", "���`"},
  {"box", "������", "�}�l���\\������A��", "�@�xŦ����"},
  {"boy", "������", "�q�I�᮳�X�F������A��", "�V���F"},
  {"bye", "�T�T", "�V", "���T�T!!"},
  {"call", "�I��", "�j�n���I��,��~~", "��~~~�A�b���̰ڰڰڰ�~~~~"},
  {"caress", "����", "���������N��", ""},
  {"clap", "���x", "�V", "���P���x"},
  {"claw", "���", "�q�߫}�ֶ�ɤF���ߤ��A��", "��o���h����"},
  {"comfort", "�w��", "�Ũ��w��", ""},
  {"cong", "����", "�q�I�᮳�X�F�Ԭ��A��I��I����", ""},
  {"cpr", "�f��f", "���", "���f��f�H�u�I�l"},
  {"cringe", "�^��", "�V", "���`�}���A�n���^��"},
  {"cry", "�j��", "�V", "�z�ޤj��"},
  {"dance", "���R", "�ԤF", "���⽡���_�R"  },
  {"destroy", "����", "���_�F�y���j�����G��z�A�F�V", ""},
  {"dogleg", "���L", "��", "���L"},
  {"drivel", "�y�f��", "���", "�y�f��"},
  {"envy", "�r�}", "�V", "�y�S�X�r�}������"},
  {"eye", "�e��i", "��", "�W�e��i"},
  {"fire", "�R��", "���ۤ������K�Ψ��V", ""},
  {"forgive", "���", "�����D�p�A��̤F", ""},
  {"french", "�k���k", "����Y����", "���V�̡���z�I�@�Ӯ������k���`�k"},
  {"giggle", "�̯�", "���", "�̶̪��b��"},
  {"glue", "�ɤ�", "�ΤT���A��", "�����H�F�_��"},
  {"goodbye", "�i�O", "�\\���L�L���V", "�i�O"},
  {"grin", "�l��", "��", "�S�X���c�����e"},
  {"growl", "�H��", "��", "�H�����w"},
  {"hand", "����", "��", "����"},
  {"hide", "��", "���b", "�I��"},
  {"hospitl", "�e��|", "��", "�e�i��|"},
  {"hug", "�֩�", "�����a�֩�", ""},
  {"hrk", "�@�s��", "�Ií�F���ΡA�׻E�F���l�A��", "�ϥX�F�@�O��o--��yu--��an�I�I�I"},
  {"jab", "�W�H", "�ŬX���W��", ""},
  {"judo", "�L�ӺL", "���F", "�����̡A�ਭ�K�K�ڡA�O�@�O�L�ӺL�I"},
  {"kickout", "��", "�Τj�}��", "���s�U�h�F"},
  {"kick", "��H", "��", "�𪺦��h����"},
  {"kiss", "���k", "���k", "���y�U"},
  {"laugh", "�J��", "�j�n�J��", ""},
  {"levis", "����", "���G����", "�I��l�K�͡I"},
  {"lick", "�Q", "�g�Q", ""},
  {"lobster", "����", "�I�i�f���ΩT�w�A��", "����b�a�O�W"},
  {"love", "���", "��", "�`�������"},
  {"marry", "�D�B", "���ۤE�ʤE�Q�E�������V", "�D�B"},
  {"no", "���n��", "���R���", "�n�Y~~~~���n��~~~~"},
  {"nod", "�I�Y", "�V", "�I�Y�٬O"},
  {"nudge", "���{�l", "�Τ�y��", "���Ψ{�l"},
  {"pad", "��ӻH", "����", "���ӻH"},
  {"pettish", "���b", "��", "���n�ݮ�a���b"},
  {"pili", "�R�E", "�ϥX �g�l�� �Ѧa�� ��Y�b �T���X�@���V", "~~~~~~"},
  {"pinch", "���H", "�ΤO����", "�����«C"},
  {"roll", "���u", "��X�h���O������,", "�b�a�W�u�Ӻu�h"},
  {"protect", "�O�@", "�O�@��", ""},
  {"pull", "��", "���R�a�Ԧ�", "����"},
  {"punch", "�~�H", "�����~�F", "�@�y"},
  {"rascal", "�A��", "��", "�A��"},
  {"recline", "�J�h", "�p��", "���h�̺εۤF�K�K"},
  {"respond", "�t�d", "�w��", "���G�y���n���A�ڷ|�t�d���K�K�z"},
  {"shrug", "�q��", "�L�`�a�V", "�q�F�q�ӻH"},
  {"sigh", "�ۮ�", "��", "�ۤF�@�f��"},
  {"slap", "���ե�", "�԰Ԫ��ڤF", "�@�y�ե�"},
  {"smooch", "�֧k", "�֧k��", ""},
  {"snicker", "�ѯ�", "�K�K�K..����", "�ѯ�"},
  {"sniff", "���h", "��", "�ᤧ�H��"},
  {"spank", "������", "�Τڴx��", "���v��"},
  {"squeeze", "���", "���a�֩��", ""},
  {"sysop", "�l��", "�s�X�F����A��", "���F�I"},
  {"thank", "�P��", "�V", "�P�±o�����a"},
  {"tickle", "�k�o", "�B�T!�B�T!�k", "���o"},
  {"wake", "�n��", "�����a��", "�n��"},
  {"wave", "����", "���", "���R���n��"},
  {"welcome", "�w��", "�w��", "�i�ӤK���@�U"},
  {"what", "����", "���G�y", "�����M�K�z��ť�Y?�H?�S?�z"},
  {"whip", "�@�l", "��W��������A���@�l�h��", ""},
  {"wink", "�w��", "��", "�������w�w����"},
  {"zap", "�r��", "��", "�ƨg������"},
  {NULL, NULL, NULL, NULL}
};

static int
party_action(ChatUser *cu, char *cmd, char *party)
{
    ChatAction *cap;
    char *verb;

    for (cap = party_data; (verb = cap->verb); cap++)
    {
	if (!str_equal(cmd, verb))
	    continue;
	if (*party == '\0')
	    party = "�j�a";
	else
	{
	    ChatUser *xuser;

	    xuser = fuzzy_cuser_by_chatid(party);
	    if (xuser == NULL)
	    {                       /* Thor.0724: �� userid�]���q */
		xuser = cuser_by_userid(party);
	    }

	    if (xuser == NULL)
	    {
		sprintf(chatbuf, msg_no_such_id, party);
		send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
		return 0;
	    }
	    else if (xuser == FUZZY_USER)
	    {
		sprintf(chatbuf, "�� �Ы�����ѥN��");
		send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
		return 0;
	    }
	    else if (cu->room != xuser->room || CLOAK(xuser))
	    {
		sprintf(chatbuf, msg_not_here, party);
		send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
		return 0;
	    }
	    else
	    {
		party = xuser->chatid;
	    }
	}
	sprintf(chatbuf, ANSI_COLOR(1;32) "%s " ANSI_COLOR(31) "%s" ANSI_COLOR(33) " %s " ANSI_COLOR(31) "%s" ANSI_RESET,
		cu->chatid, cap->part1_msg, party, cap->part2_msg);
	send_to_room(cu->room, chatbuf, cu->userno, MSG_MESSAGE);
	return 0;
    }
    return 1;
}


/* --------------------------------------------- */
/* MUD-like social commands : speak              */
/* --------------------------------------------- */


static ChatAction speak_data[] =
{
    { "ask", "�߰�", "��", NULL },
    { "chant", "�q�|", "���n�q�|", NULL },
    { "cheer", "�ܪ�", "�ܪ�", NULL },
    { "chuckle", "����", "����", NULL },
    { "curse", "�t�F", "�t�F", NULL },
    { "demand", "�n�D", "�n�D", NULL },
    { "frown", "�K���Y", "�٬�", NULL },
    { "groan", "�D�u", "�D�u", NULL },
    { "grumble", "�o�c��", "�o�c��", NULL },
    { "guitar", "�u��", "��u�ۦN�L�A��۵�", NULL },
    { "hum", "���", "���ۻy", NULL },
    { "moan", "���", "���", NULL },
    { "notice", "�j��", "�j��", NULL },
    { "order", "�R�O", "�R�O", NULL },
    { "ponder", "�H��", "�H��", NULL },
    { "pout", "���L", "���ۼL��", NULL },
    { "pray", "��ë", "��ë", NULL },
    { "request", "���D", "���D", NULL },
    { "shout", "�j�|", "�j�|", NULL },
    { "sing", "�ۺq", "�ۺq", NULL },
    { "smile", "�L��", "�L��", NULL },
    { "smirk", "����", "����", NULL },
    { "swear", "�o�}", "�o�}", NULL },
    { "tease", "�J��", "�J��", NULL },
    { "whimper", "��|", "��|����", NULL },
    { "yawn", "����", "�䥴�����仡", NULL },
    { "yell", "�j��", "�j��", NULL },
    { NULL, NULL, NULL, NULL }
};


static int
speak_action(ChatUser *cu, char *cmd, char *msg)
{
    ChatAction *cap;
    char *verb;

    for (cap = speak_data; (verb = cap->verb); cap++)
    {
	if (!str_equal(cmd, verb))
	    continue;
	sprintf(chatbuf, ANSI_COLOR(1;32) "%s " ANSI_COLOR(31) "%s�G" ANSI_COLOR(33) " %s" ANSI_RESET,
		cu->chatid, cap->part1_msg, msg);
	send_to_room(cu->room, chatbuf, cu->userno, MSG_MESSAGE);
	return 0;
    }
    return 1;
}


/* -------------------------------------------- */
/* MUD-like social commands : condition          */
/* -------------------------------------------- */


static ChatAction condition_data[] =
{
    { "applaud", "���", "�԰԰԰԰԰԰�....", NULL },
    { "ayo", "�����", "�����~~~", NULL },
    { "back", "���^��", "�^�ӧ����~��ľ�", NULL },
    { "blood", "�b�夤", "�˦b��y����", NULL },
    { "blush", "�y��", "�y�����F", NULL },
    { "broke", "�߸H", "���߯}�H���@���@����", NULL },
    { "careles", "�S�H�z", "���㳣�S���H�z�� :~~~~", NULL },
    { "chew", "�ߥʤl", "�ܱy�����߰_�ʤl�ӤF", NULL },
    { "climb", "���s", "�ۤv�C�C���W�s�ӡK�K", NULL },
    { "cold", "�P�_�F", "�P�_�F,���������ڥX�h�� :~~~(", NULL },
    { "cough", "�y��", "�y�F�X�n", NULL },
    { "die", "����", "�������", NULL },
    { "faint", "����", "�������", NULL },
    { "flop", "������", "��쭻����... �ƭˡI", NULL },
    { "fly", "���ƵM", "���ƵM", NULL },
    { "gold", "�����P", "�۵ۡG�y���|�������|����  �X�����! �o�a�x�A�����P�A���a�˾H�ӡI�z", NULL },
    { "gulu", "�{�l�j", "���{�l�o�X�B�P~~~�B�P~~~���n��", NULL },
    { "haha", "�z����", "�z������.....^o^", NULL },
    { "helpme", "�D��", "�j��~~~�ϩR��~~~~", NULL },
    { "hoho", "������", "���������Ӥ���", NULL },
    { "happy", "����", "�����o�b�a�W���u", NULL },
    { "idle", "�b��F", "�b��F", NULL },
    { "jacky", "�̮�", "�l�l�몺�̨Ӯ̥h", NULL },
    { "luck", "���B", "�z�I�֮�աI", NULL },
    { "macarn", "�@�ػR", "�}�l���_�F��a��a��e��a�����", NULL },
    { "miou", "�p�p", "�p�p�f�]�f�]������", NULL },
    { "mouth", "��L", "��L��!!", NULL },
    { "nani", "���|", "�G�`���ڮ�??", NULL },
    { "nose", "�y���", "�y���", NULL },
    { "puke", "�æR", "�æR��", NULL },
    { "rest", "��", "�𮧤��A�Фť��Z", NULL },
    { "reverse", "½�{", "½�{", NULL },
    { "room", "�}�ж�", "r-o-O-m-r-O-��-Mmm-rR��........", NULL },
    { "shake", "�n�Y", "�n�F�n�Y", NULL },
    { "sleep", "�ε�", "�w�b��L�W�εۤF�A�f���y�i��L�A�y������I", NULL },
    { "so", "�N��l", "�N��l!!", NULL },
    { "sorry", "�D�p", "���!!�ڹ藍�_�j�a,�ڹ藍�_��a���|~~~~~~���~~~~~", NULL },
    { "story", "���j", "�}�l���j�F", NULL },
    { "strut", "�n�\\��", "�j�n�j�\\�a��", NULL },
    { "suicide", "�۱�", "�۱�", NULL },
    { "tea", "�w��", "�w�F���n��", NULL },
    { "think", "���", "�n���Y�Q�F�@�U", NULL },
    { "tongue", "�R��", "�R�F�R���Y", NULL },
    { "wall", "����", "�]�h����", NULL },
    { "wawa", "�z�z", "�z�z�z~~~~~!!!!!  ~~~>_<~~~", NULL },
    { "www", "�L�L", "�L�L�L!!!", NULL },
    { "zzz", "���I", "�I�P~~~~ZZzZz�C��ZZzzZzzzZZ...", NULL },
    { NULL, NULL, NULL, NULL }
};


static int
condition_action(ChatUser *cu, char *cmd)
{
    ChatAction *cap;
    char *verb;

    for (cap = condition_data; (verb = cap->verb); cap++)
    {
	if (str_equal(cmd, verb))
	{
	    sprintf(chatbuf, ANSI_COLOR(1;32) "%s " ANSI_COLOR(31) "%s" ANSI_RESET,
		    cu->chatid, cap->part1_msg);
	    send_to_room(cu->room, chatbuf, cu->userno, MSG_MESSAGE);
	    return 1;
	}
    }
    return 0;
}


/* --------------------------------------------- */
/* MUD-like social commands : help               */
/* --------------------------------------------- */


static char *dscrb[] =
{
    ANSI_COLOR(1;37) "�i Verb + Nick�G   �ʵ� + ���W�r �j" ANSI_COLOR(36) "   �ҡG//kick piggy" ANSI_RESET,
    ANSI_COLOR(1;37) "�i Verb + Message�G�ʵ� + �n������ �j" ANSI_COLOR(36) "   �ҡG//sing �ѤѤ���" ANSI_RESET,
    ANSI_COLOR(1;37) "�i Verb�G�ʵ� �j    �����G�¸ܭ���" ANSI_RESET, NULL
};
ChatAction *catbl[] =
{
    party_data, speak_data, condition_data, NULL
};

static void
chat_partyinfo(ChatUser *cu, char *msg)
{
    if (!common_client_command)
	return;                     /* only allow common client to retrieve it */

    sprintf(chatbuf, "3 �ʧ@  ���  ���A");
    send_to_user(cu, chatbuf, 0, MSG_PARTYINFO);
}

static void
chat_party(ChatUser *cu, char *msg)
{
    int kind, i;
    ChatAction *cap;

    if (!common_client_command)
	return;

    kind = atoi(nextword(&msg));
    if (kind < 0 || kind > 2)
	return;

    sprintf(chatbuf, "%d  %s", kind, kind == 2 ? "I" : "");

    /* Xshadow: �u�� condition �~�O immediate mode */
    send_to_user(cu, chatbuf, 0, MSG_PARTYLISTSTART);

    cap = catbl[kind];
    for (i = 0; cap[i].verb; i++)
    {
	sprintf(chatbuf, "%-10s %-20s", cap[i].verb, cap[i].chinese);
	/* for (j=0;j<1000000;j++); */
	send_to_user(cu, chatbuf, 0, MSG_PARTYLIST);
    }

    sprintf(chatbuf, "%d", kind);
    send_to_user(cu, chatbuf, 0, MSG_PARTYLISTEND);
}


#define SCREEN_WIDTH    80
#define MAX_VERB_LEN    8
#define VERB_NO         10

static void
view_action_verb(ChatUser *cu, char cmd)       /* Thor.0726: �s�[�ʵ�������� */
{
    register int i;
    register char *p, *q, *data, *expn;
    register ChatAction *cap;

    send_to_user(cu, "/c", 0, MSG_CLRSCR);

    data = chatbuf;

    if (cmd < '1' || cmd > '3')
    {                             /* Thor.0726: �g�o���n, �Q��k��i... */
	for (i = 0; (p = dscrb[i]); i++)
	{
	    sprintf(data, "  [//]help %d          - MUD-like ����ʵ�   �� %d ��", i + 1, i + 1);
	    send_to_user(cu, data, 0, MSG_MESSAGE);
	    send_to_user(cu, p, 0, MSG_MESSAGE);
	    send_to_user(cu, " ", 0, MSG_MESSAGE);    /* Thor.0726: ����, �ݭn " "
						       * ��? */
	}
    }
    else
    {
	send_to_user(cu, dscrb[cmd-'1'], 0, MSG_MESSAGE);

	expn = chatbuf + 100;       /* Thor.0726: ���Ӥ��|overlap�a? */

	*data = '\0';
	*expn = '\0';

	cap = catbl[cmd-'1'];

	for (i = 0; (p = cap[i].verb); i++)
	{
	    q = cap[i].chinese;

	    strcat(data, p);
	    strcat(expn, q);

	    if (((i + 1) % VERB_NO) == 0 || cap[i+1].verb==NULL)
	    {
		send_to_user(cu, data, 0, MSG_MESSAGE);
		send_to_user(cu, expn, 0, MSG_MESSAGE); /* Thor.0726: ��ܤ������ */
		*data = '\0';
		*expn = '\0';
	    }
	    else
	    {
		strncat(data, "        ", MAX_VERB_LEN - strlen(p));
		strncat(expn, "        ", MAX_VERB_LEN - strlen(q));
	    }
	}
    }
    /* send_to_user(cu, " ",0); *//* Thor.0726: ����, �ݭn " " ��? */
}

/* ----------------------------------------------------- */
/* chat user service routines                            */
/* ----------------------------------------------------- */


static ChatCmd chatcmdlist[] =
{
    {"act", chat_act, 0},
    {"ban", chat_ban, 0},
    {"bye", chat_goodbye, 0},
    {"chatroom", chat_chatroom, 1}, /* Xshadow: for common client */
    {"cloak", chat_cloak, 2},
    {"flags", chat_setroom, 0},
    {"ignore", chat_ignore, 1},
    {"invite", chat_invite, 0},
    {"join", chat_join, 0},
    {"kick", chat_kick, 1},
    {"msg", chat_private, 0},
    {"nick", chat_nick, 0},
    {"operator", chat_makeop, 0},
    {"party", chat_party, 1},       /* Xshadow: party data for common client */
    {"partyinfo", chat_partyinfo, 1},       /* Xshadow: party info for common * client */
    {"query", chat_query, 0},

    {"room", chat_list_rooms, 0},
    {"unignore", chat_unignore, 1},
    {"unban", chat_unban, 1},
    {"whoin", chat_list_by_room, 1},
    {"wall", chat_broadcast, 2},

    {"who", chat_map_chatids_thisroom, 0},
    {"list", chat_list_users, 0},
    {"topic", chat_topic, 0},
    {"version", chat_version, 1},
    {"xinfo", chat_xinfo, 1},

    {NULL, NULL, 0}
};

/* Thor: 0 ���� exact, 1 �n exactly equal, 2 ���K���O */


static int
command_execute(ChatUser *cu)
{
    char *cmd, *msg;
    ChatCmd *cmdrec;
    int match;

    msg = cu->ibuf;

    /* Validation routine */

    if (cu->room == NULL)
    {
	/* MUST give special /! or /-! command if not in the room yet */
	if(strncmp(msg, "/!", 2)==0) {
	    return login_user(cu, msg+2);
	}
	return -1;
    }

    if(msg[0] == '\0')
	return 0;

    /* If not a "/"-command, it goes to the room. */
    if (msg[0] != '/')
    {
	char buf[16];

	sprintf(buf, "%s:", cu->chatid);
	sprintf(chatbuf, "%-10s%s", buf, msg);
	if (!CLOAK(cu))           /* Thor: ��ѫ������N */
	    send_to_room(cu->room, chatbuf, cu->userno, MSG_MESSAGE);
	return 0;
    }

    msg++;
    cmd = nextword(&msg);
    match = 0;

    if (*cmd == '/')
    {
	cmd++;
	if (!*cmd || str_equal(cmd, "help"))
	{
	    /* Thor.0726: �ʵ����� */
	    cmd = nextword(&msg);
	    view_action_verb(cu, *cmd);
	    match = 1;
	}
	else if (party_action(cu, cmd, msg) == 0)
	    match = 1;
	else if (speak_action(cu, cmd, msg) == 0)
	    match = 1;
	else
	    match = condition_action(cu, cmd);
    }
    else
    {
	char *str;

	common_client_command = 0;
	for(cmdrec = chatcmdlist; (str = cmdrec->cmdstr); cmdrec++)
	{
		switch (cmdrec->exact)
		{
		case 1:                   /* exactly equal */
		    match = str_equal(cmd, str);
		    break;
		case 2:                   /* Thor: secret command */
		    if (CHATSYSOP(cu))
			match = str_equal(cmd, str);
		    break;
		default:                  /* not necessary equal */
		    match = str_match(cmd, str) >= 0;
		    break;
		}

		if (match)
		{
		    cmdrec->cmdfunc(cu, msg);
		    break;
		}
	    }
    }

    if (!match)
    {
	sprintf(chatbuf, "�� ���O���~�G/%s", cmd);
	send_to_user(cu, chatbuf, 0, MSG_MESSAGE);
    }
    return 0;
}


/* ----------------------------------------------------- */
/* serve chat_user's connection                          */
/* ----------------------------------------------------- */


static int
cuser_serve(ChatUser *cu)
{
    register int ch, len, isize;
    register char *str, *cmd;
    char buf[80];

    str = buf;
    len = recv(cu->sock, str, sizeof(buf) - 1, 0);
    if (len <= 0)
    {
	/* disconnected */

	exit_room(cu, EXIT_LOSTCONN, (char *) NULL);
	return -1;
    }

    isize = cu->isize;
    cmd = cu->ibuf;
    while (len--) {
	ch = *str++;

	if (ch == '\r' || ch == '\0')
	    continue;
	if (ch == '\n')
	{
	    cmd[isize]='\0';
	    isize = 0;

	    if (command_execute(cu) < 0)
		return -1;

	    continue;
	}
	if (isize < sizeof(cu->ibuf)-1)
	    cmd[isize++] = ch;
    }
    cu->isize = isize;
    return 0;
}


/* ----------------------------------------------------- */
/* chatroom server core routines                         */
/* ----------------------------------------------------- */

static int
start_daemon()
{
    int fd, value;
    char buf[80];
    struct linger ld;
    struct rlimit limit;
    time_t dummy;
    struct tm *dummy_time;

    /*
     * More idiot speed-hacking --- the first time conversion makes the C
     * library open the files containing the locale definition and time zone.
     * If this hasn't happened in the parent process, it happens in the
     * children, once per connection --- and it does add up.
     */

    time(&dummy);
    dummy_time = gmtime(&dummy);
    dummy_time = localtime(&dummy);
    strftime(buf, 80, "%d/%b/%Y:%H:%M:%S", dummy_time);

    /* --------------------------------------------------- */
    /* speed-hacking DNS resolve                           */
    /* --------------------------------------------------- */

    gethostname(buf, sizeof(buf));

    /* Thor: �U�@server�|������connection, �N�^�h����, client �Ĥ@���|�i�J���� */
    /* �ҥH���� listen �� */

    /* --------------------------------------------------- */
    /* detach daemon process                               */
    /* --------------------------------------------------- */

    close(0);
    close(1);
    close(2);

#ifndef SELFTEST
    if (fork())
	exit(0);
#endif

    chdir(BBSHOME);

    setsid();

#ifndef SELFTEST
    attach_SHM();
#endif
    /* --------------------------------------------------- */
    /* adjust the resource limit                           */
    /* --------------------------------------------------- */

    getrlimit(RLIMIT_NOFILE, &limit);
    limit.rlim_cur = limit.rlim_max;
    setrlimit(RLIMIT_NOFILE, &limit);

    fd = open(CHAT_PIDFILE, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd >= 0)
    {
	/* sprintf(buf, "%5d\n", value); */
	sprintf(buf, "%5d\n", (int)getpid());
	write(fd, buf, 6);
	close(fd);
    }

    value = 1;
    fd = tobind(XCHATD_ADDR);

    ld.l_onoff = ld.l_linger = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof(ld)) == -1)
    {
	perror("setsockopt");
	exit(-1);
    }

    return fd;
}


static void
free_resource(int fd)
{
    static int loop = 0;
    register ChatUser *user;
    register int sock, num;

    /* ���s�p�� maxfd */
    num = 0;
    for (user = mainuser; user; user = user->unext)
    {
	num++;
	sock = user->sock;
	if (fd < sock)
	    fd = sock;
    }

    sprintf(chatbuf, "%d, %d user (maxfds %d -> %d)", ++loop, num, maxfds, fd+1);
    logit("LOOP", chatbuf);

    maxfds = fd + 1;
}


#ifdef  SERVER_USAGE
static void
server_usage()
{
    struct rusage ru;
    char buf[2048];

    if (getrusage(RUSAGE_SELF, &ru))
	return;

    sprintf(buf, "\n[Server Usage]\n\n"
	    "user time: %.6f\n"
	    "system time: %.6f\n"
	    "maximum resident set size: %lu P\n"
	    "integral resident set size: %lu\n"
	    "page faults not requiring physical I/O: %ld\n"
	    "page faults requiring physical I/O: %ld\n"
	    "swaps: %ld\n"
	    "block input operations: %ld\n"
	    "block output operations: %ld\n"
	    "messages sent: %ld\n"
	    "messages received: %ld\n"
	    "signals received: %ld\n"
	    "voluntary context switches: %ld\n"
	    "involuntary context switches: %ld\n"
	    "\n",

	    (double) ru.ru_utime.tv_sec + (double) ru.ru_utime.tv_usec / 1000000.0,
	    (double) ru.ru_stime.tv_sec + (double) ru.ru_stime.tv_usec / 1000000.0,
	    ru.ru_maxrss,
	    ru.ru_idrss,
	    ru.ru_minflt,
	    ru.ru_majflt,
	    ru.ru_nswap,
	    ru.ru_inblock,
	    ru.ru_oublock,
	    ru.ru_msgsnd,
	    ru.ru_msgrcv,
	    ru.ru_nsignals,
	    ru.ru_nvcsw,
	    ru.ru_nivcsw);

    write(flog, buf, strlen(buf));
}
#endif


static void
abort_server()
{
    log_close();
    exit(1);
}


static void
reaper()
{
    int state;

    while (waitpid(-1, &state, WNOHANG | WUNTRACED) > 0)
    {
    }
}

#ifdef SELFTESTER
#define MAXTESTUSER 20

int selftest_connect(void)
{
    struct sockaddr_in sin;
    struct hostent *h;
    int cfd;

    if (!(h = gethostbyname("localhost"))) {
	perror("gethostbyname");
	return -1;
    }
    memset(&sin, 0, sizeof sin);
#ifdef __FreeBSD__
    sin.sin_len = sizeof(sin);
#endif
    sin.sin_family = PF_INET;
    memcpy(&sin.sin_addr, h->h_addr, h->h_length);
    sin.sin_port = htons(NEW_CHATPORT);
    cfd = socket(sin.sin_family, SOCK_STREAM, 0);
    if (connect(cfd, (struct sockaddr *) & sin, sizeof sin) != 0) {
	perror("connect");
	return -1;
    }
    return cfd;
}

static int
selftest_send(int fd, char *buf)
{
    int             len;
    char            genbuf[200];

    snprintf(genbuf, sizeof(genbuf), "%s\n", buf);
    len = strlen(genbuf);
    return (send(fd, genbuf, len, 0) == len);
}
void selftest_testing(void)
{
    int cfd;
    char userid[IDLEN+1];
    char inbuf[1024], buf[1024];

    cfd=selftest_connect();
    if(cfd<0) exit(1);
    while(1) {
	snprintf(userid, sizeof(userid), "%ld", random()%(MAXTESTUSER*2));
	sprintf(buf, "/%s! %s %s %s", random()%4==0?"-":"",userid, userid, "passwd");
	selftest_send(cfd, buf);
	if (recv(cfd, inbuf, 3, 0) != 3) {
	    close(cfd);
	    return;
	}
	if (!strcmp(inbuf, CHAT_LOGIN_OK))
	    break;
    }

    if(random()%4!=0) {
	sprintf(buf, "/j %d", random()%5);
	selftest_send(cfd, buf);
    }

    while(1) {
	int i;
	int r;
	int inlen;
	fd_set rset,xset;
	struct timeval zerotv;

	FD_ZERO(&rset);
	FD_SET(cfd, &rset);
	FD_ZERO(&xset);
	FD_SET(cfd, &xset);
	zerotv.tv_sec=0;
	zerotv.tv_usec=0;
	select(cfd+1, &rset, NULL, &xset, &zerotv);
	if(FD_ISSET(cfd, &rset)) {
	    inlen=read(cfd, inbuf, sizeof(inbuf));
	    if(inlen<0) break;
	}
	if(FD_ISSET(cfd, &xset)) {
	    inlen=read(cfd, inbuf, sizeof(inbuf));
	    if(inlen<0) break;
	}


	if(random()%10==0) {
	    switch(random()%4) {
		case 0:
		    r=random()%(sizeof(party_data)/sizeof(party_data[0])-1);
		    sprintf(buf, "//%s",party_data[r].verb);
		    break;
		case 1:
		    r=random()%(sizeof(speak_data)/sizeof(speak_data[0])-1);
		    sprintf(buf, "//%s",speak_data[r].verb);
		    break;
		case 2:
		    r=random()%(sizeof(condition_data)/sizeof(condition_data[0])-1);
		    sprintf(buf, "//%s",condition_data[r].verb);
		    break;
		case 3:
		    sprintf(buf, "blah");
		    break;
	    }
	} else {
		r=random()%(sizeof(chatcmdlist)/sizeof(chatcmdlist[0])-1);
		sprintf(buf, "/%s",chatcmdlist[r].cmdstr);
		if(strncmp("/flag",buf,5)==0) {
		    if(random()%2)
			strcat(buf," +");
		    else
			strcat(buf," -");
		    strcat(buf,random()%2?"l":"L");
		    strcat(buf,random()%2?"h":"H");
		    strcat(buf,random()%2?"s":"S");
		    strcat(buf,random()%2?"t":"T");
		} else if(strncmp("/bye",buf,4)==0) {
		    switch(random()%10) {
			case 0: strcpy(buf,"//"); break;
			case 1: strcpy(buf,"//1"); break;
			case 2: strcpy(buf,"//2"); break;
			case 3: strcpy(buf,"//3"); break;
			case 4: strcpy(buf,"/bye"); break;
			case 5: strcpy(buf,"/help op"); break;
			case 6: close(cfd); return; break;
			case 7: strcpy(buf,"/help"); break;
			case 8: strcpy(buf,"/help"); break;
			case 9: strcpy(buf,"/help"); break;
		    }
		}
	}
	for(i=random()%3; i>0; i--) {
	    char tmp[1024];
	    sprintf(tmp," %ld", random()%(MAXTESTUSER*2));
	    strcat(buf, tmp);
	}

	selftest_send(cfd, buf);
    }
    close(cfd);
}

void selftest_test(void)
{
    while(1) {
	fprintf(stderr,".");
	selftest_testing();
	usleep(1000);
    }
}

void selftest(void)
{
    int i;
    pid_t pid;

    pid=fork();
    if(pid<0) exit(1);
    if(pid) return;
    sleep(1);

    for(i=0; i<MAXTESTUSER; i++) {
	if(fork()==0)
	    selftest_test();
	sleep(1);
	random();
    }

    exit(0);
}
#endif

int
main(int argc, char *argv[])
{
    register int msock, csock, nfds;
    register ChatUser *cu, *cunext;
    register fd_set *rptr, *xptr;
    fd_set rset, xset;
    struct timeval tv;
    time4_t uptime, tmaintain;

#ifdef SELFTESTER
    if(argc>1) {
	Signal(SIGPIPE, SIG_IGN);
      selftest();
      return 0;
    }
#endif
    msock = start_daemon();

    setgid(BBSGID);
    setuid(BBSUID);

    boot_time = time(0);
    log_init();

//    Signal(SIGBUS, SIG_IGN);
//    Signal(SIGSEGV, SIG_IGN);
    Signal(SIGPIPE, SIG_IGN);
    Signal(SIGURG, SIG_IGN);

    Signal(SIGCHLD, reaper);
    Signal(SIGTERM, abort_server);

#ifdef  SERVER_USAGE
    Signal(SIGPROF, server_usage);
#endif

    /* ----------------------------- */
    /* init variable : rooms & users */
    /* ----------------------------- */

    mainuser = NULL;
    memset(&mainroom, 0, sizeof(mainroom));
    strcpy(mainroom.name, MAIN_NAME);
    strcpy(mainroom.topic, MAIN_TOPIC);

    /* ----------------------------------- */
    /* main loop                           */
    /* ----------------------------------- */

    FD_ZERO(&mainfds);
    FD_SET(msock, &mainfds);
    rptr = &rset;
    xptr = &xset;
    maxfds = msock + 1;
    tmaintain = time(0) + CHAT_INTERVAL;

    for (;;)
    {
	uptime = time(0);
	if (tmaintain < uptime)
	{
	    tmaintain = uptime + CHAT_INTERVAL;

	    /* client/server �����Q�� ping-pong ��k�P�_ user �O���O�٬��� */
	    /* �p�G client �w�g�����F�A�N����� resource */

	    free_resource(msock);
#ifdef SELFTEST
	    break;
#endif
	}

	memcpy(rptr, &mainfds, sizeof(fd_set));
	memcpy(xptr, &mainfds, sizeof(fd_set));

	/* Thor: for future reservation bug */

	tv.tv_sec = CHAT_INTERVAL;
	tv.tv_usec = 0;

	nfds = select(maxfds, rptr, NULL, xptr, &tv);

	/* free idle user & chatroom's resource when no traffic */
	if (nfds == 0)
	    continue;

	/* check error condition */
	if (nfds < 0)
	    continue;

	/* accept new connection */
	if (FD_ISSET(msock, rptr)) {
	    csock = accept(msock, NULL, NULL);

	    if(csock < 0) {
		if(errno != EINTR) {
		    // TODO
		}
	    } else {
		cu = (ChatUser *) malloc(sizeof(ChatUser));
		if(cu == NULL) {
		    close(csock);
		    logit("accept", "malloc fail");
		} else {
		    memset(cu, 0, sizeof(ChatUser));
		    cu->sock = csock;
		    cu->unext = mainuser;
		    mainuser = cu;

		    totaluser++;
		    FD_SET(csock, &mainfds);
		    if (csock >= maxfds)
			maxfds = csock + 1;

#ifdef  DEBUG
		    logit("accept", "OK");
#endif
		}
	    }
	}

	for (cu = mainuser; cu && nfds>0; cu = cunext) {
	    /* logout_user() �| free(cu); ���� cu->next �O�U�� */
	    /* FIXME �Y��n cu �b main room /kick cu->next, �h cu->next �|�Q free �� */
	    cunext = cu->unext;
	    csock = cu->sock;
	    if (FD_ISSET(csock, xptr)) {
		logout_user(cu);
		nfds--;
	    } else if (FD_ISSET(csock, rptr)) {
		if (cuser_serve(cu) < 0)
		    logout_user(cu);
		nfds--;
	    }
	}

	/* end of main loop */
    }
    return 0;
}
