/* $Id$ */
#include "bbs.h"

#define QCAST   int (*)(const void *, const void *)

static char    * const IdleTypeTable[] = {
    "���b��b��", "���H�ӹq", "�V����", "�����P��", "�������A", "�ڦb���"
};
static char    * const sig_des[] = {
    "����", "���", "", "�U��", "�H��", "�t��", "�U���", "�U�¥մ�",
};
static char    * const withme_str[] = {
  "�ͤ�", "�U���l��", "���d��", "�U�H��", "�U�t��", "�U���", "�U�¥մ�", NULL
};

#define MAX_SHOW_MODE 6
#define M_INT 15		/* monitor mode update interval */
#define P_INT 20		/* interval to check for page req. in
				 * talk/chat */
#define BOARDFRI  1

typedef struct talkwin_t {
    int             curcol, curln;
    int             sline, eline;
}               talkwin_t;

typedef struct pickup_t {
    userinfo_t     *ui;
    int             friend, uoffset;
}               pickup_t;

/* �O�� friend �� user number */
//
#define PICKUP_WAYS     8

static char    * const fcolor[11] = {
    "", ANSI_COLOR(36), ANSI_COLOR(32), ANSI_COLOR(1;32),
    ANSI_COLOR(33), ANSI_COLOR(1;33), ANSI_COLOR(1;37), ANSI_COLOR(1;37),
    ANSI_COLOR(31), ANSI_COLOR(1;35), ANSI_COLOR(1;36)
};
static char     save_page_requestor[40];
static char     page_requestor[40];

userinfo_t *uip;

int
iswritable_stat(const userinfo_t * uentp, int fri_stat)
{
    if (uentp == currutmp)
	return 0;

    if (HasUserPerm(PERM_SYSOP))
	return 1;

    if (!HasUserPerm(PERM_LOGINOK) || HasUserPerm(PERM_VIOLATELAW))
	return 0;

    return (uentp->pager != PAGER_ANTIWB && 
	    (fri_stat & HFM || uentp->pager != PAGER_FRIENDONLY));
}

int
isvisible_stat(const userinfo_t * me, const userinfo_t * uentp, int fri_stat)
{
    if (!uentp || uentp->userid[0] == 0)
	return 0;

    /* to avoid paranoid users get crazy*/
    if (uentp->mode == DEBUGSLEEPING)
	return 0;

    if (PERM_HIDE(uentp) && !(PERM_HIDE(me)))	/* ��赵�����ΦӧA�S�� */
	return 0;
    else if ((me->userlevel & PERM_SYSOP) ||
	     ((fri_stat & HRM) && (fri_stat & HFM)))
	/* �����ݪ�������H */
	return 1;

    if (uentp->invisible && !(me->userlevel & PERM_SEECLOAK))
	return 0;

    return !(fri_stat & HRM);
}

int query_online(const char *userid)
{
    userinfo_t *uentp;

    if (!userid || !*userid)
	return 0;

    if (!isalnum(*userid))
	return 0;

    if (strchr(userid, '.') || SHM->GV2.e.noonlineuser)
	return 0;

    uentp = search_ulist_userid(userid);

    if (!uentp ||!isvisible(currutmp, uentp))
	return 0;

    return 1;
}

const char           *
modestring(const userinfo_t * uentp, int simple)
{
    static char     modestr[40];
    static char *const notonline = "���b���W";
    register int    mode = uentp->mode;
    register char  *word;
    int             fri_stat;

    /* for debugging */
    if (mode >= MAX_MODES) {
	syslog(LOG_WARNING, "what!? mode = %d", mode);
	word = ModeTypeTable[mode % MAX_MODES];
    } else
	word = ModeTypeTable[mode];

    fri_stat = friend_stat(currutmp, uentp);
    if (!(HasUserPerm(PERM_SYSOP) || HasUserPerm(PERM_SEECLOAK)) &&
	((uentp->invisible || (fri_stat & HRM)) &&
	 !((fri_stat & HFM) && (fri_stat & HRM))))
	return notonline;
    else if (mode == EDITING) {
	snprintf(modestr, sizeof(modestr), "E:%s",
		ModeTypeTable[uentp->destuid < EDITING ? uentp->destuid :
			      EDITING]);
	word = modestr;
    } else if (!mode && *uentp->chatid == 1) {
	if (!simple)
	    snprintf(modestr, sizeof(modestr), "�^�� %s",
		    isvisible_uid(uentp->destuid) ?
		    getuserid(uentp->destuid) : "�Ů�");
	else
	    snprintf(modestr, sizeof(modestr), "�^���I�s");
    } 
    else if (!mode && *uentp->chatid == 3)
	snprintf(modestr, sizeof(modestr), "���y�ǳƤ�");
    else if (
#ifdef NOKILLWATERBALL
	     uentp->msgcount > 0
#else
	     (!mode) && *uentp->chatid == 2
#endif
	     )
	if (uentp->msgcount < 10) {
	    const char *cnum[10] =
	    {"", "�@", "��", "�T", "�|", "��", 
		 "��", "�C", "�K", "�E"};
	    snprintf(modestr, sizeof(modestr),
		     "��%s�����y", cnum[(int)(uentp->msgcount)]);
	} else
	    snprintf(modestr, sizeof(modestr), "����F @_@");
    else if (!mode)
	return (uentp->destuid == 6) ? uentp->chatid :
	    IdleTypeTable[(0 <= uentp->destuid && uentp->destuid < 6) ?
			  uentp->destuid : 0];
    else if (simple)
	return word;
    else if (uentp->in_chat && mode == CHATING)
	snprintf(modestr, sizeof(modestr), "%s (%s)", word, uentp->chatid);
    else if (mode == TALK || mode == M_FIVE || mode == CHC || mode == UMODE_GO
	    || mode == DARK) {
	if (!isvisible_uid(uentp->destuid))	/* Leeym ���(����)���� */
	    snprintf(modestr, sizeof(modestr), "%s �Ů�", word);
	/* Leeym * �j�a�ۤv�o���a�I */
	else
	    snprintf(modestr, sizeof(modestr),
		     "%s %s", word, getuserid(uentp->destuid));
    } else if (mode == CHESSWATCHING) {
	snprintf(modestr, sizeof(modestr), "�[��");
    } else if (mode != PAGE && mode != TQUERY)
	return word;
    else
	snprintf(modestr, sizeof(modestr),
		 "%s %s", word, getuserid(uentp->destuid));

    return (modestr);
}

int
set_friend_bit(const userinfo_t * me, const userinfo_t * ui)
{
    int             unum, hit = 0;
    const int *myfriends;

    /* �P�_���O�_���ڪ��B�� ? */
    if( intbsearch(ui->uid, me->myfriend, me->nFriends) )
	hit = IFH;

    /* �P�_�ڬO�_����誺�B�� ? */
    if( intbsearch(me->uid, ui->myfriend, ui->nFriends) )
	hit |= HFM;

    /* �P�_���O�_���ڪ����H ? */
    myfriends = me->reject;
    while ((unum = *myfriends++)) {
	if (unum == ui->uid) {
	    hit |= IRH;
	    break;
	}
    }

    /* �P�_�ڬO�_����誺���H ? */
    myfriends = ui->reject;
    while ((unum = *myfriends++)) {
	if (unum == me->uid) {
	    hit |= HRM;
	    break;
	}
    }
    return hit;
}

inline int
he_reject_me(userinfo_t * uin){
    int* iter = uin->reject;
    int unum;
    while ((unum = *iter++)) {
	if (unum == currutmp->uid) {
	    return 1;
	}
    }
    return 0;
}

int
reverse_friend_stat(int stat)
{
    int             stat1 = 0;
    if (stat & IFH)
	stat1 |= HFM;
    if (stat & IRH)
	stat1 |= HRM;
    if (stat & HFM)
	stat1 |= IFH;
    if (stat & HRM)
	stat1 |= IRH;
    if (stat & IBH)
	stat1 |= IBH;
    return stat1;
}

#ifdef OUTTACACHE
int sync_outta_server(int sfd)
{
    int i;
    int offset = (int)(currutmp - &SHM->uinfo[0]);

    int cmd, res;
    int nfs;
    ocfs_t  fs[MAX_FRIEND*2];

    cmd = -2;
    if(towrite(sfd, &cmd, sizeof(cmd))<0 ||
	    towrite(sfd, &offset, sizeof(offset))<0 ||
	    towrite(sfd, &currutmp->uid, sizeof(currutmp->uid)) < 0 ||
	    towrite(sfd, currutmp->myfriend, sizeof(currutmp->myfriend))<0 ||
	    towrite(sfd, currutmp->reject, sizeof(currutmp->reject))<0)
	return -1;

    if(toread(sfd, &res, sizeof(res))<0)
	return -1;

    if(res<0)
	return -1;
    if(res==2) {
	close(sfd);
	outs("�n�J���W�c, ���קK�t�έt���L��, �еy��A��\n");
	refresh();
	sleep(30);
	log_usies("REJECTLOGIN", NULL);
	memset(currutmp, 0, sizeof(userinfo_t));
	exit(0);
    }

    if(toread(sfd, &nfs, sizeof(nfs))<0)
	return -1;
    if(nfs<0 || nfs>MAX_FRIEND*2) {
	fprintf(stderr, "invalid nfs=%d\n",nfs);
	return -1;
    }

    if(toread(sfd, fs, sizeof(fs[0])*nfs)<0)
	return -1;

    close(sfd);

    for(i=0; i<nfs; i++) {
	if( SHM->uinfo[fs[i].index].uid != fs[i].uid )
	    continue; // double check, server may not know user have logout
	currutmp->friend_online[currutmp->friendtotal++]
	    = fs[i].friendstat;
	/* XXX: race here */
	if( SHM->uinfo[fs[i].index].friendtotal < MAX_FRIEND )
	    SHM->uinfo[fs[i].index].friend_online[ SHM->uinfo[fs[i].index].friendtotal++ ] = fs[i].rfriendstat;
    }

    if(res==1) {
	vmsg("�Ф��W�c�n�J�H�K�y���t�ιL�׭t��");
    }
    return 0;
}
#endif

void login_friend_online(void)
{
    userinfo_t     *uentp;
    int             i, stat, stat1;
    int             offset = (int)(currutmp - &SHM->uinfo[0]);

#ifdef OUTTACACHE
    int sfd;
    /* OUTTACACHE is TOO slow, let's prompt user here. */
    move(b_lines-2, 0); clrtobot();
    outs("\n���b��s�P�P�B�u�W�ϥΪ̤Φn�ͦW��A�t�έt���q�j�ɷ|�ݮɸ��[...\n");
    refresh();

    sfd = toconnect(OUTTACACHEHOST, OUTTACACHEPORT);
    if(sfd>=0) {
	int res=sync_outta_server(sfd);
	if(res==0) // sfd will be closed if return 0
	    return;
	close(sfd);
    }
#endif

    for (i = 0; i < SHM->UTMPnumber && currutmp->friendtotal < MAX_FRIEND; i++) {
	uentp = (&SHM->uinfo[SHM->sorted[SHM->currsorted][0][i]]);
	if (uentp && uentp->uid && (stat = set_friend_bit(currutmp, uentp))) {
	    stat1 = reverse_friend_stat(stat);
	    stat <<= 24;
	    stat |= (int)(uentp - &SHM->uinfo[0]);
	    currutmp->friend_online[currutmp->friendtotal++] = stat;
	    if (uentp != currutmp && uentp->friendtotal < MAX_FRIEND) {
		stat1 <<= 24;
		stat1 |= offset;
		uentp->friend_online[uentp->friendtotal++] = stat1;
	    }
	}
    }
    return;
}

/* TODO merge with util/shmctl.c logout_friend_online() */
int
logout_friend_online(userinfo_t * utmp)
{
    int my_friend_idx, thefriend;
    int k;
    int             offset = (int)(utmp - &SHM->uinfo[0]);
    userinfo_t     *ui;
    for(; utmp->friendtotal>0; utmp->friendtotal--) {
	if( !(0 <= utmp->friendtotal && utmp->friendtotal < MAX_FRIEND) )
	    return 1;
	my_friend_idx=utmp->friendtotal-1;
	thefriend = (utmp->friend_online[my_friend_idx] & 0xFFFFFF);
	utmp->friend_online[my_friend_idx]=0;

	if( !(0 <= thefriend && thefriend < USHM_SIZE) )
	    continue;

	ui = &SHM->uinfo[thefriend]; 
	if(ui->pid==0 || ui==utmp)
	    continue;
	if(ui->friendtotal > MAX_FRIEND || ui->friendtotal<0)
	    continue;
	for (k = 0; k < ui->friendtotal && k < MAX_FRIEND &&
	    (int)(ui->friend_online[k] & 0xFFFFFF) != offset; k++);
	if (k < ui->friendtotal && k < MAX_FRIEND) {
	  ui->friendtotal--;
	  ui->friend_online[k] = ui->friend_online[ui->friendtotal];
	  ui->friend_online[ui->friendtotal] = 0;
	}
    }
    return 0;
}


int
friend_stat(const userinfo_t * me, const userinfo_t * ui)
{
    int             i, j, hit = 0;
    /* �ݪO�n�� */
    if (me->brc_id && ui->brc_id == me->brc_id) {
	hit = IBH;
    }
    for (i = 0; me->friend_online[i] && i < MAX_FRIEND; i++) {
	j = (me->friend_online[i] & 0xFFFFFF);
	if (VALID_USHM_ENTRY(j) && ui == &SHM->uinfo[j]) {
	    hit |= me->friend_online[i] >> 24;
	    break;
	}
    }
    if (PERM_HIDE(ui))
	return hit & ST_FRIEND;
    return hit;
}

int
isvisible_uid(int tuid)
{
    userinfo_t     *uentp;

    if (!tuid || !(uentp = search_ulist(tuid)))
	return 1;
    return isvisible(currutmp, uentp);
}

/* �u��ʧ@ */
static void
my_kick(userinfo_t * uentp)
{
    char            genbuf[200];

    getdata(1, 0, msg_sure_ny, genbuf, 4, LCECHO);
    clrtoeol();
    if (genbuf[0] == 'y') {
	snprintf(genbuf, sizeof(genbuf),
		 "%s (%s)", uentp->userid, uentp->nickname);
	log_usies("KICK ", genbuf);
	if ((uentp->pid <= 0 || kill(uentp->pid, SIGHUP) == -1) && (errno == ESRCH))
	    purge_utmp(uentp);
	outs("��X�h�o");
    } else
	outs(msg_cancel);
    pressanykey();
}

static void
chicken_query(const char *userid)
{
    userec_t xuser;
    if (getuser(userid, &xuser)) {
	if (xuser.mychicken.name[0]) {
	    time_diff(&(xuser.mychicken));
	    if (!isdeadth(&(xuser.mychicken))) {
		show_chicken_data(&(xuser.mychicken), NULL);
		prints("\n\n�H�W�O %s ���d�����..", xuser.userid);
	    }
	} else {
	    move(1, 0);
	    clrtobot();
	    prints("\n\n%s �èS���i�d��..", xuser.userid);
	}
	pressanykey();
    }
}

int
my_query(const char *uident)
{
    userec_t        muser;
    int             tuid, fri_stat = 0;
    userinfo_t     *uentp;
    const char *sex[8] =
    {MSG_BIG_BOY, MSG_BIG_GIRL,
	MSG_LITTLE_BOY, MSG_LITTLE_GIRL,
    MSG_MAN, MSG_WOMAN, MSG_PLANT, MSG_MIME};
    static time_t last_query;

    STATINC(STAT_QUERY);
    if ((tuid = getuser(uident, &muser))) {
	move(1, 0);
	clrtobot();
	move(1, 0);
	setutmpmode(TQUERY);
	currutmp->destuid = tuid;

	if ((uentp = (userinfo_t *) search_ulist(tuid)))
	    fri_stat = friend_stat(currutmp, uentp);

	prints("�m�עҼʺ١n%s(%s)%*s�m�g�٪��p�n%s",
	       muser.userid,
	       muser.nickname,
	       strlen(muser.userid) + strlen(muser.nickname) >= 26 ? 0 :
	       (int)(26 - strlen(muser.userid) - strlen(muser.nickname)), "",
	       money_level(muser.money));
	if (uentp && ((fri_stat & HFM && !uentp->invisible) || strcmp(muser.userid,cuser.userid) == 0))
	    prints(" ($%d)", muser.money);
	outc('\n');

	prints("�m�W�����ơn%d��", muser.numlogins);
	move(2, 40);
#ifdef ASSESS
	prints("�m�峹�g�ơn%d�g (�u:%d/�H:%d)\n", muser.numposts, muser.goodpost, muser.badpost);
#else
	prints("�m�峹�g�ơn%d�g\n", muser.numposts);
#endif

	prints(ANSI_COLOR(1;33) "�m�ثe�ʺA�n%-28.28s" ANSI_RESET,
	       (uentp && isvisible_stat(currutmp, uentp, fri_stat)) ?
	       modestring(uentp, 0) : "���b���W");

	outs(((uentp && ISNEWMAIL(uentp)) || load_mailalert(muser.userid))
	     ? "�m�p�H�H�c�n���s�i�H���٨S��\n" :
	     "�m�p�H�H�c�n�Ҧ��H�󳣬ݹL�F\n");
	prints("�m�W���W���n%-28.28s�m�W���G�m�n%s\n",
	       Cdate(&muser.lastlogin),
	       (muser.lasthost[0] ? muser.lasthost : "(����)"));
	prints("�m���l�Ѿ��Z�n%3d �� %3d �� %3d �M      "
	       "�m�H�Ѿ��Z�n%3d �� %3d �� %3d �M\n",
	       muser.five_win, muser.five_lose, muser.five_tie,
	       muser.chc_win, muser.chc_lose, muser.chc_tie);
#ifdef ASSESS
	prints("�m�v�е���n �u %d / �H %d", muser.goodsale, muser.badsale);
	move(6, 40);
#endif
	if ((uentp && ((fri_stat & HFM) || strcmp(muser.userid,cuser.userid) == 0) && !uentp->invisible))
	    prints("�m ��  �O �n%-28.28s\n", sex[muser.sex % 8]);

	showplans_userec(&muser);
	if(HasUserPerm(PERM_SYSOP|PERM_POLICE) ) 
	{
          if(vmsg("T: �}�߻@��")=='T')
		  violate_law(&muser, tuid);
	}
	else
	   pressanykey();
	if(now-last_query<1)
	    sleep(2);
	else if(now-last_query<2)
	    sleep(1);
	last_query=now;
	return FULLUPDATE;
    }
    return DONOTHING;
}

static char     t_last_write[80];

void check_water_init(void)
{
    if(water==NULL) {
	water = (water_t*)malloc(sizeof(water_t)*6);
	memset(water, 0, sizeof(water_t)*6);
	water_which = &water[0];

	strlcpy(water[0].userid, " ���� ", sizeof(water[0].userid));
    }
}
		
static void
water_scr(const water_t * tw, int which, char type)
{
    if (type == 1) {
	int             i;
	const int colors[] = {33, 37, 33, 37, 33};
	move(8 + which, 28);
	outc(' ');
	move(8 + which, 28);
	prints(ANSI_COLOR(1;37;45) "  %c %-14s " ANSI_COLOR(0) "",
	       tw->uin ? ' ' : 'x',
	       tw->userid);
	for (i = 0; i < 5; ++i) {
	    move(16 + i, 4);
	    outc(' ');
	    move(16 + i, 4);
	    if (tw->msg[(tw->top - i + 4) % 5].last_call_in[0] != 0)
		prints(ANSI_COLOR(0) "   " ANSI_COLOR(1;%d;44) "��%-64s" ANSI_COLOR(0) "   \n",
		       colors[i],
		       tw->msg[(tw->top - i + 4) % 5].last_call_in);
	    else
		outs(ANSI_COLOR(0) "�@\n");
	}

	move(21, 4);
	outc(' ');
	move(21, 4);
	prints(ANSI_COLOR(0) "   " ANSI_COLOR(1;37;46) "%-66s" ANSI_COLOR(0) "   \n",
	       tw->msg[5].last_call_in);

	move(0, 0);
	outc(' ');
	move(0, 0);
#ifdef PLAY_ANGEL
	if (tw->msg[0].msgmode == MSGMODE_TOANGEL)
	    outs(ANSI_COLOR(0) "�^���p�D�H:");
	else
#endif
	prints(ANSI_COLOR(0) "���� %s:", tw->userid);
	clrtoeol();
	move(0, strlen(tw->userid) + 6);
    } else {
	move(8 + which, 28);
	outs("123456789012345678901234567890");
	move(8 + which, 28);
	prints(ANSI_COLOR(1;37;44) "  %c %-13s�@" ANSI_COLOR(0) "",
	       tw->uin ? ' ' : 'x',
	       tw->userid);
    }
}

void
my_write2(void)
{
    int             i, ch, currstat0;
    char            genbuf[256], msg[80], done = 0, c0, which;
    water_t        *tw;
    unsigned char   mode0;

    check_water_init();
    if (swater[0] == NULL)
	return;
    wmofo = REPLYING;
    currstat0 = currstat;
    c0 = currutmp->chatid[0];
    mode0 = currutmp->mode;
    currutmp->mode = 0;
    currutmp->chatid[0] = 3;
    currstat = DBACK;

    //init screen
    move(WB_OFO_USER_TOP, WB_OFO_USER_LEFT);
    outs(ANSI_COLOR(1;33;46) " �� ���y������H ��" ANSI_COLOR(0) "");
    for (i = 0; i < WB_OFO_USER_HEIGHT;++i)
	if (swater[i] == NULL || swater[i]->pid == 0)
	    break;
	else {
	    if (swater[i]->uin &&
		(swater[i]->pid != swater[i]->uin->pid ||
		 swater[i]->userid[0] != swater[i]->uin->userid[0]))
		swater[i]->uin = search_ulist_pid(swater[i]->pid);
	    water_scr(swater[i], i, 0);
	}
    move(WB_OFO_MSG_TOP, WB_OFO_MSG_LEFT);
    outs(ANSI_COLOR(0) " " ANSI_COLOR(1;35) "��" ANSI_COLOR(1;36) "�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w"
	   "�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w" ANSI_COLOR(1;35) "��" ANSI_COLOR(0) " ");
    move(WB_OFO_MSG_BOTTOM, WB_OFO_MSG_LEFT);
    outs(" " ANSI_COLOR(1;35) "��" ANSI_COLOR(1;36) "�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w"
	   "�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w" ANSI_COLOR(1;35) "��" ANSI_COLOR(0) " ");
    water_scr(swater[0], 0, 1);
    refresh();

    which = 0;
    do {
	switch ((ch = igetch())) {
	case Ctrl('T'):
	case KEY_UP:
	    if (water_usies != 1) {
		water_scr(swater[(int)which], which, 0);
		which = (which - 1 + water_usies) % water_usies;
		water_scr(swater[(int)which], which, 1);
		refresh();
	    }
	    break;

	case KEY_DOWN:
	case Ctrl('R'):
	    if (water_usies != 1) {
		water_scr(swater[(int)which], which, 0);
		which = (which + 1 + water_usies) % water_usies;
		water_scr(swater[(int)which], which, 1);
		refresh();
	    }
	    break;

	case KEY_LEFT:
	    done = 1;
	    break;

	case KEY_UNKNOWN:
	    break;

	default:
	    done = 1;
	    tw = swater[(int)which];

	    if (!tw->uin)
		break;

	    if (ch != '\r' && ch != '\n') {
		msg[0] = ch, msg[1] = 0;
	    } else
		msg[0] = 0;
	    move(0, 0);
	    outs(ANSI_RESET);
	    clrtoeol();
#ifndef PLAY_ANGEL
	    snprintf(genbuf, sizeof(genbuf), "���� %s:", tw->userid);
	    i = WATERBALL_CONFIRM;
#else
	    if (tw->msg[0].msgmode == MSGMODE_WRITE) {
		snprintf(genbuf, sizeof(genbuf), "���� %s:", tw->userid);
		i = WATERBALL_CONFIRM;
	    } else if (tw->msg[0].msgmode == MSGMODE_TOANGEL) {
		strcpy(genbuf, "�^���p�D�H:");
		i = WATERBALL_CONFIRM_ANSWER;
	    } else { /* tw->msg[0].msgmode == MSGMODE_FROMANGEL */
		strcpy(genbuf, "�A�ݥL�@���G");
		i = WATERBALL_CONFIRM_ANGEL;
	    }
#endif
	    if (!oldgetdata(0, 0, genbuf, msg,
			80 - strlen(tw->userid) - 6, DOECHO))
		break;

	    if (my_write(tw->pid, msg, tw->userid, i, tw->uin))
		strlcpy(tw->msg[5].last_call_in, t_last_write,
			sizeof(tw->msg[5].last_call_in));
	    break;
	}
    } while (!done);

    currstat = currstat0;
    currutmp->chatid[0] = c0;
    currutmp->mode = mode0;
    if (wmofo == RECVINREPLYING) {
	wmofo = NOTREPLYING;
	write_request(0);
    }
    wmofo = NOTREPLYING;
}

/*
 * �Q�I�s���ɾ�:
 * 1. ��s�դ��y flag = WATERBALL_PREEDIT, 1 (pre-edit)
 * 2. �^���y     flag = WATERBALL_GENERAL, 0
 * 3. �W��aloha  flag = WATERBALL_ALOHA,   2 (pre-edit)
 * 4. �s��       flag = WATERBALL_SYSOP,   3 if SYSOP
 *               flag = WATERBALL_PREEDIT, 1 otherwise
 * 5. ����y     flag = WATERBALL_GENERAL, 0
 * 6. my_write2  flag = WATERBALL_CONFIRM, 4 (pre-edit but confirm)
 * 7. (when defined PLAY_ANGEL)
 *    �I�s�p�Ѩ� flag = WATERBALL_ANGEL,   5 (id = "�p�Ѩ�")
 * 8. (when defined PLAY_ANGEL)
 *    �^���p�D�H flag = WATERBALL_ANSWER,  6 (���� id)
 * 9. (when defined PLAY_ANGEL)
 *    �I�s�p�Ѩ� flag = WATERBALL_CONFIRM_ANGEL, 7 (pre-edit)
 * 10. (when defined PLAY_ANGEL)
 *    �^���p�D�H flag = WATERBALL_CONFIRM_ANSWER, 8 (pre-edit)
 */
int
my_write(pid_t pid, const char *prompt, const char *id, int flag, userinfo_t * puin)
{
    int             len, currstat0 = currstat, fri_stat = -1;
    char            msg[80], destid[IDLEN + 1];
    char            genbuf[200], buf[200], c0 = currutmp->chatid[0];
    unsigned char   mode0 = currutmp->mode;
    userinfo_t     *uin;
    uin = (puin != NULL) ? puin : (userinfo_t *) search_ulist_pid(pid);
    strlcpy(destid, id, sizeof(destid));
    check_water_init();

    /* what if uin is NULL but other conditions are not true?
     * will this situation cause SEGV?
     * should this "!uin &&" replaced by "!uin ||" ?
     */
    if ((!uin || !uin->userid[0]) && !((flag == WATERBALL_GENERAL
#ifdef PLAY_ANGEL
		|| flag == WATERBALL_ANGEL || flag == WATERBALL_ANSWER 
#endif
            )
	    && water_which->count > 0)) {
	vmsg("�V�|! ���w���]�F(���b���W)! ");
	watermode = -1;
	return 0;
    }
    currutmp->mode = 0;
    currutmp->chatid[0] = 3;
    currstat = DBACK;

    if (flag == WATERBALL_GENERAL
#ifdef PLAY_ANGEL
	    || flag == WATERBALL_ANGEL || flag == WATERBALL_ANSWER
#endif
	    ) {
	/* �@����y */
	watermode = 0;

	/* should we alert if we're in disabled mode? */
	switch(currutmp->pager)
	{
	    case PAGER_DISABLE:
	    case PAGER_ANTIWB:
		move(1, 0);  clrtoeol();
		outs(ANSI_COLOR(1;31) "�A���I�s���ثe�������O�H����y�A���i��L�k�^�ܡC" ANSI_RESET);
		break;

	    case PAGER_FRIENDONLY:
#if 0
		// �p�G��西�b�U���A�o�Ӧn������í�| crash (?) */
		if (uin && uin->userid[0])
		{
		    fri_stat = friend_stat(currutmp, uin);
		    if(fri_stat & HFM)
			break;
		}
#endif
		move(1, 0);  clrtoeol();
		outs(ANSI_COLOR(1;31) "�A���I�s���ثe�u�����n�ͥ���y�A�Y���D�n�ͫh�i��L�k�^�ܡC" ANSI_RESET);
		break;
	}

	if (!(len = getdata(0, 0, prompt, msg, 56, DOECHO))) {
	    currutmp->chatid[0] = c0;
	    currutmp->mode = mode0;
	    currstat = currstat0;
	    watermode = -1;
	    return 0;
	}

	if (watermode > 0) {
	    int             i;

	    i = (water_which->top - watermode + MAX_REVIEW) % MAX_REVIEW;
	    uin = (userinfo_t *) search_ulist_pid(water_which->msg[i].pid);
#ifdef PLAY_ANGEL
	    if (water_which->msg[i].msgmode == MSGMODE_FROMANGEL)
		flag = WATERBALL_ANGEL;
	    else if (water_which->msg[i].msgmode == MSGMODE_TOANGEL)
		flag = WATERBALL_ANSWER;
	    else
		flag = WATERBALL_GENERAL;
#endif
	    strlcpy(destid, water_which->msg[i].userid, sizeof(destid));
	}
    } else {
	/* pre-edit �����y */
	strlcpy(msg, prompt, sizeof(msg));
	len = strlen(msg);
    }

    strip_ansi(msg, msg, STRIP_ALL);
    if (uin && *uin->userid &&
	    (flag == WATERBALL_GENERAL || flag == WATERBALL_CONFIRM
#ifdef PLAY_ANGEL
	     || flag == WATERBALL_ANGEL || flag == WATERBALL_ANSWER
	     || flag == WATERBALL_CONFIRM_ANGEL
	     || flag == WATERBALL_CONFIRM_ANSWER
#endif
	     )) 
    {
	snprintf(buf, sizeof(buf), "�ᵹ %s : %s [Y/n]?", destid, msg);

	getdata(0, 0, buf, genbuf, 3, LCECHO);
	if (genbuf[0] == 'n') {
	    currutmp->chatid[0] = c0;
	    currutmp->mode = mode0;
	    currstat = currstat0;
	    watermode = -1;
	    return 0;
	}
    }
    watermode = -1;
    if (!uin || !*uin->userid || (strcasecmp(destid, uin->userid)
#ifdef PLAY_ANGEL
	    && flag != WATERBALL_ANGEL && flag != WATERBALL_CONFIRM_ANGEL) ||
	    ((flag == WATERBALL_ANGEL || flag == WATERBALL_CONFIRM_ANGEL)
	     && strcasecmp(cuser.myangel, uin->userid)
#endif
	    )) {
	vmsg("�V�|! ���w���]�F(���b���W)! ");
	currutmp->chatid[0] = c0;
	currutmp->mode = mode0;
	currstat = currstat0;
	return 0;
    }
    if(fri_stat < 0)
	fri_stat = friend_stat(currutmp, uin);
    // else, fri_stat was already calculated. */

    if (flag != WATERBALL_ALOHA) {	/* aloha �����y���Φs�U�� */
	/* �s��ۤv�����y�� */
	if (!fp_writelog) {
	    sethomefile(genbuf, cuser.userid, fn_writelog);
	    fp_writelog = fopen(genbuf, "a");
	}
	if (fp_writelog) {
	    fprintf(fp_writelog, "To %s: %s [%s]\n",
		    destid, msg, Cdatelite(&now));
	    snprintf(t_last_write, 66, "To %s: %s", destid, msg);
	}
    }
    if (flag == WATERBALL_SYSOP && uin->msgcount) {
	/* ���� */
	uin->destuip = currutmp - &SHM->uinfo[0];
	uin->sig = 2;
	if (uin->pid > 0)
	    kill(uin->pid, SIGUSR1);
    } else if ((flag != WATERBALL_ALOHA &&
#ifdef PLAY_ANGEL
	       flag != WATERBALL_ANGEL &&
	       flag != WATERBALL_ANSWER &&
	       flag != WATERBALL_CONFIRM_ANGEL &&
	       flag != WATERBALL_CONFIRM_ANSWER &&
	       /* Angel accept or not is checked outside.
		* Avoiding new users don't know what pager is. */
#endif
	       !HasUserPerm(PERM_SYSOP) &&
	       (uin->pager == PAGER_ANTIWB ||
		uin->pager == PAGER_DISABLE ||
		(uin->pager == PAGER_FRIENDONLY &&
		 !(fri_stat & HFM))))
#ifdef PLAY_ANGEL
	       || ((flag == WATERBALL_ANGEL || flag == WATERBALL_CONFIRM_ANGEL)
		   && he_reject_me(uin))
#endif
	       ) {
	outmsg(ANSI_COLOR(1;33;41) "�V�|! ��訾���F! " ANSI_COLOR(37) "~>_<~" ANSI_RESET);
    } else {
	int     write_pos = uin->msgcount; /* try to avoid race */
	if ( write_pos < (MAX_MSGS - 1) ) { /* race here */
	    unsigned char   pager0 = uin->pager;

	    uin->msgcount = write_pos + 1;
	    uin->pager = PAGER_DISABLE;
	    uin->msgs[write_pos].pid = currpid;
#ifdef PLAY_ANGEL
	    if (flag == WATERBALL_ANSWER || flag == WATERBALL_CONFIRM_ANSWER)
		strlcpy(uin->msgs[write_pos].userid, "�p�Ѩ�", 
		    sizeof(uin->msgs[write_pos].userid));
	    else
#endif
		strlcpy(uin->msgs[write_pos].userid, cuser.userid,
			sizeof(uin->msgs[write_pos].userid));
	    strlcpy(uin->msgs[write_pos].last_call_in, msg,
		    sizeof(uin->msgs[write_pos].last_call_in));
#ifndef PLAY_ANGEL
	    uin->msgs[write_pos].msgmode = MSGMODE_WRITE;
#else
	    switch (flag) {
		case WATERBALL_ANGEL:
		case WATERBALL_CONFIRM_ANGEL:
		    uin->msgs[write_pos].msgmode = MSGMODE_TOANGEL;
		    break;
		case WATERBALL_ANSWER:
		case WATERBALL_CONFIRM_ANSWER:
		    uin->msgs[write_pos].msgmode = MSGMODE_FROMANGEL;
		    break;
		default:
		    uin->msgs[write_pos].msgmode = MSGMODE_WRITE;
	    }
#endif
	    uin->pager = pager0;
	} else if (flag != WATERBALL_ALOHA)
	    outmsg(ANSI_COLOR(1;33;41) "�V�|! ��褣��F! (����Ӧh���y) " ANSI_COLOR(37) "@_@" ANSI_RESET);

	if (uin->msgcount >= 1 &&
#ifdef NOKILLWATERBALL
	    !(uin->wbtime = now) /* race */
#else
	    (uin->pid <= 0 || kill(uin->pid, SIGUSR2) == -1) 
#endif
	    && flag != WATERBALL_ALOHA)
	    outmsg(ANSI_COLOR(1;33;41) "�V�|! �S����! " ANSI_COLOR(37) "~>_<~" ANSI_RESET);
	else if (uin->msgcount == 1 && flag != WATERBALL_ALOHA)
	    outmsg(ANSI_COLOR(1;33;44) "���y�{�L�h�F! " ANSI_COLOR(37) "*^o^*" ANSI_RESET);
	else if (uin->msgcount > 1 && uin->msgcount < MAX_MSGS &&
		flag != WATERBALL_ALOHA)
	    outmsg(ANSI_COLOR(1;33;44) "�A�ɤW�@��! " ANSI_COLOR(37) "*^o^*" ANSI_RESET);

#if defined(NOKILLWATERBALL) && defined(PLAY_ANGEL)
	/* Questioning and answering should better deliver immediately. */
	if ((flag == WATERBALL_ANGEL || flag == WATERBALL_ANSWER ||
	    flag == WATERBALL_CONFIRM_ANGEL ||
	    flag == WATERBALL_CONFIRM_ANSWER) && uin->pid)
	    kill(uin->pid, SIGUSR2);
#endif
    }

    clrtoeol();

    currutmp->chatid[0] = c0;
    currutmp->mode = mode0;
    currstat = currstat0;
    return 1;
}

void
getmessage(msgque_t msg)
{
    int     write_pos = currutmp->msgcount; 
    if ( write_pos < (MAX_MSGS - 1) ) { 
            unsigned char pager0 = currutmp->pager;
 	    currutmp->msgcount = write_pos+1;
            memcpy(&currutmp->msgs[write_pos], &msg, sizeof(msgque_t));
            currutmp->pager = pager0;
   	    write_request(SIGUSR1);
        }
}

void
t_display_new(void)
{
    static int      t_display_new_flag = 0;
    int             i, off = 2;
    if (t_display_new_flag)
	return;
    else
	t_display_new_flag = 1;

    check_water_init();
    if (WATERMODE(WATER_ORIG))
	water_which = &water[0];
    else
	off = 3;

    if (water[0].count && watermode > 0) {
	move(1, 0);
	outs("�w�w�w�w�w�w�w���w�y�w�^�w�U�w�w�w");
	outs(WATERMODE(WATER_ORIG) ?
	     "�w�w�w�w�w�w��[Ctrl-R Ctrl-T]������w�w�w�w�w" :
	     "��[Ctrl-R Ctrl-T Ctrl-F Ctrl-G ]������w�w�w�w");
	if (WATERMODE(WATER_NEW)) {
	    move(2, 0);
	    clrtoeol();
	    for (i = 0; i < 6; i++) {
		if (i > 0)
		    if (swater[i - 1]) {

			if (swater[i - 1]->uin &&
			    (swater[i - 1]->pid != swater[i - 1]->uin->pid ||
			     swater[i - 1]->userid[0] != swater[i - 1]->uin->userid[0]))
			    swater[i - 1]->uin = (userinfo_t *) search_ulist_pid(swater[i - 1]->pid);
			prints("%s%c%-13.13s" ANSI_RESET,
			       swater[i - 1] != water_which ? "" :
			       swater[i - 1]->uin ? ANSI_COLOR(1;33;47) :
			       ANSI_COLOR(1;33;45),
			       !swater[i - 1]->uin ? '#' : ' ',
			       swater[i - 1]->userid);
		    } else
			outs("              ");
		else
		    prints("%s ����  " ANSI_RESET,
			   water_which == &water[0] ? ANSI_COLOR(1;33;47) " " :
			   " "
			);
	    }
	}
	for (i = 0; i < water_which->count; i++) {
	    int a = (water_which->top - i - 1 + MAX_REVIEW) % MAX_REVIEW;
	    int len = 75 - strlen(water_which->msg[a].last_call_in)
		- strlen(water_which->msg[a].userid);
	    if (len < 0)
		len = 0;

	    move(i + (WATERMODE(WATER_ORIG) ? 2 : 3), 0);
	    clrtoeol();
	    if (watermode - 1 != i)
		prints(ANSI_COLOR(1;33;46) " %s " ANSI_COLOR(37;45) " %s " ANSI_RESET "%*s",
		       water_which->msg[a].userid,
		       water_which->msg[a].last_call_in, len,
		       "");
	    else
		prints(ANSI_COLOR(1;44) ">" ANSI_COLOR(1;33;47) "%s "
		       ANSI_COLOR(37;45) " %s " ANSI_RESET "%*s",
		       water_which->msg[a].userid,
		       water_which->msg[a].last_call_in,
		       len, "");
	}

	if (t_last_write[0]) {
	    move(i + off, 0);
	    clrtoeol();
	    outs(t_last_write);
	    i++;
	}
	move(i + off, 0);
	outs("�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w"
	     "�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w");
	if (WATERMODE(WATER_NEW))
	    while (i++ <= water[0].count) {
		move(i + off, 0);
		clrtoeol();
	    }
    }
    t_display_new_flag = 0;
}

int
t_display(void)
{
    char            genbuf[200], ans[4];
    if (fp_writelog) {
	fclose(fp_writelog);
	fp_writelog = NULL;
    }
    setuserfile(genbuf, fn_writelog);
    if (more(genbuf, YEA) != -1) {
	move(b_lines - 4, 0);
	clrtobot();

	outs(ANSI_COLOR(1;33;45) "�����y��z�{�� " ANSI_RESET "\n"
	     "�����z: �i�N���y�s�J�H�c(M)��, ��i�l����j�ӫH��e�� u,\n"
	     "�t�η|�N���y�������s��z��H�e���z��! " ANSI_RESET "\n");

	getdata(b_lines - 1, 0, "�M��(C) �s�J�H�c(M) �O�d(R) (C/M/R)?[R]",
		ans, sizeof(ans), LCECHO);
	if (*ans == 'm') {
	    fileheader_t    mymail;
	    char            title[128], buf[80];

	    sethomepath(buf, cuser.userid);
	    stampfile(buf, &mymail);

	    mymail.filemode = FILE_READ ;
	    strlcpy(mymail.owner, "[��.��.��]", sizeof(mymail.owner));
	    strlcpy(mymail.title, "���u�O��", sizeof(mymail.title));
	    sethomedir(title, cuser.userid);
	    Rename(genbuf, buf);
	    append_record(title, &mymail, sizeof(mymail));
	} else if (*ans == 'c')
	    unlink(genbuf);
	return FULLUPDATE;
    }
    return DONOTHING;
}

static void
do_talk_nextline(talkwin_t * twin)
{
    twin->curcol = 0;
    if (twin->curln < twin->eline)
	++(twin->curln);
    else
	region_scroll_up(twin->sline, twin->eline);
    move(twin->curln, twin->curcol);
}

static void
do_talk_char(talkwin_t * twin, int ch, FILE *flog)
{
    screenline_t   *line;
    int             i;
    char            ch0, buf[81];

    if (isprint2(ch)) {
	ch0 = big_picture[twin->curln].data[twin->curcol];
	if (big_picture[twin->curln].len < 79)
	    move(twin->curln, twin->curcol);
	else
	    do_talk_nextline(twin);
	outc(ch);
	++(twin->curcol);
	line = big_picture + twin->curln;
	if (twin->curcol < line->len) {	/* insert */
	    ++(line->len);
	    memcpy(buf, line->data + twin->curcol, 80);
	    save_cursor();
	    do_move(twin->curcol, twin->curln);
	    ochar(line->data[twin->curcol] = ch0);
	    for (i = twin->curcol + 1; i < line->len; i++)
		ochar(line->data[i] = buf[i - twin->curcol - 1]);
	    restore_cursor();
	}
	line->data[line->len] = 0;
	return;
    }
    switch (ch) {
    case Ctrl('H'):
    case '\177':
	if (twin->curcol == 0)
	    return;
	line = big_picture + twin->curln;
	--(twin->curcol);

	if (twin->curcol < line->len) {
	    int delta = 1;
#ifdef DBCSAWARE
	    if (twin->curcol > 0 && ISDBCSAWARE() && 
		    getDBCSstatus(line->data, twin->curcol) == DBCS_TRAILING)
		twin->curcol--, delta++;
#endif
	    line->len -= delta;
	    save_cursor();
	    do_move(twin->curcol, twin->curln);
	    for (i = twin->curcol; i < line->len; i++)
		ochar(line->data[i] = line->data[i + delta]);
	    while (delta-- > 0)
	    {
		line->data[i++] = 0;
		ochar(' ');
	    }
	    restore_cursor();
	}
	move(twin->curln, twin->curcol);
	return;
    case Ctrl('D'):
	line = big_picture + twin->curln;
	if (twin->curcol < line->len) {
	    int delta = 1;
#ifdef DBCSAWARE
	    if (ISDBCSAWARE() && 
		    getDBCSstatus(line->data, twin->curcol) == DBCS_LEADING)
		delta++;
#endif
	    line->len -= delta;
	    save_cursor();
	    do_move(twin->curcol, twin->curln);
	    for (i = twin->curcol; i < line->len; i++)
		ochar(line->data[i] = line->data[i + delta]);
	    while (delta-- > 0)
	    {
		line->data[i++] = 0;
		ochar(' ');
	    }
	    restore_cursor();
	}
	return;
    case Ctrl('G'):
	bell();
	return;

    case Ctrl('B'):
	if (twin->curcol > 0) {
	    --(twin->curcol);
#ifdef DBCSAWARE
	    line = big_picture + twin->curln;
	    if(twin->curcol > 0 && twin->curcol < line->len && ISDBCSAWARE())
	    {
		line = big_picture + twin->curln;
		if(getDBCSstatus(line->data, twin->curcol) == DBCS_TRAILING)
		    twin->curcol --;
	    }
#endif
	    move(twin->curln, twin->curcol);
	}
	return;
    case Ctrl('F'):
	if (twin->curcol < 79) {
	    ++(twin->curcol);
#ifdef DBCSAWARE
	    line = big_picture + twin->curln;
	    if(twin->curcol < 79 && twin->curcol < line->len && ISDBCSAWARE())
	    {
		line = big_picture + twin->curln;
		if(getDBCSstatus(line->data, twin->curcol) == DBCS_TRAILING)
		    twin->curcol++;
	    }
#endif
	    move(twin->curln, twin->curcol);
	}
	return;

    case KEY_TAB:
	twin->curcol += 8;
	if (twin->curcol > 80)
	    twin->curcol = 80;
#ifdef DBCSAWARE
	line = big_picture + twin->curln;
	if(twin->curcol > 0 && twin->curcol < line->len &&
		getDBCSstatus(line->data, twin->curcol) == DBCS_TRAILING)
	    twin->curcol--;
#endif
	move(twin->curln, twin->curcol);
	return;
    case Ctrl('A'):
	twin->curcol = 0;
	move(twin->curln, twin->curcol);
	return;
    case Ctrl('K'):
	clrtoeol();
	return;
    case Ctrl('Y'):
	twin->curcol = 0;
	move(twin->curln, twin->curcol);
	clrtoeol();
	return;
    case Ctrl('E'):
	twin->curcol = big_picture[twin->curln].len;
	move(twin->curln, twin->curcol);
	return;
    case Ctrl('M'):
    case Ctrl('J'):
	line = big_picture + twin->curln;
	strlcpy(buf, (char *)line->data, line->len + 1);
	do_talk_nextline(twin);
	break;
    case Ctrl('P'):
	line = big_picture + twin->curln;
	strlcpy(buf, (char *)line->data, line->len + 1);
	if (twin->curln > twin->sline) {
	    --(twin->curln);
	    move(twin->curln, twin->curcol);
	}
#ifdef DBCSAWARE
	line = big_picture + twin->curln;
	if(twin->curcol > 0 && twin->curcol < line->len &&
		getDBCSstatus(line->data, twin->curcol) == DBCS_TRAILING)
	    move(twin->curln, --twin->curcol);
#endif
	break;
    case Ctrl('N'):
	line = big_picture + twin->curln;
	strlcpy(buf, (char *)line->data, line->len + 1);
	if (twin->curln < twin->eline) {
	    ++(twin->curln);
	    move(twin->curln, twin->curcol);
	}
#ifdef DBCSAWARE
	line = big_picture + twin->curln;
	if(twin->curcol > 0 && twin->curcol < line->len &&
		getDBCSstatus(line->data, twin->curcol) == DBCS_TRAILING)
	    move(twin->curln, --twin->curcol);
#endif
	break;
    }
    trim(buf);
    if (*buf)
	fprintf(flog, "%s%s: %s%s\n",
		(twin->eline == b_lines - 1) ? ANSI_COLOR(1;35) : "",
		(twin->eline == b_lines - 1) ?
		getuserid(currutmp->destuid) : cuser.userid, buf,
		(ch == Ctrl('P')) ? ANSI_COLOR(37;45) "(Up)" ANSI_RESET : ANSI_RESET);
}

static void
do_talk(int fd)
{
    struct talkwin_t mywin, itswin;
    char            mid_line[128], data[200];
    int             i, datac, ch;
    int             im_leaving = 0;
    FILE           *log, *flog;
    struct tm      *ptime;
    char            genbuf[200], fpath[100];

    STATINC(STAT_DOTALK);
    ptime = localtime4(&now);

    setuserfile(fpath, "talk_XXXXXX");
    flog = fdopen(mkstemp(fpath), "w");

    setuserfile(genbuf, fn_talklog);

    if ((log = fopen(genbuf, "w")))
	fprintf(log, "[%d/%d %d:%02d] & %s\n",
		ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour,
		ptime->tm_min, save_page_requestor);
    setutmpmode(TALK);

    ch = 58 - strlen(save_page_requestor);
    snprintf(genbuf, sizeof(genbuf), "%s�i%s", cuser.userid, cuser.nickname);
    i = ch - strlen(genbuf);
    if (i >= 0)
	i = (i >> 1) + 1;
    else {
	genbuf[ch] = '\0';
	i = 1;
    }
    memset(data, ' ', i);
    data[i] = '\0';

    snprintf(mid_line, sizeof(mid_line),
	     ANSI_COLOR(1;46;37) "  �ͤѻ��a  " ANSI_COLOR(45) "%s%s�j"
	     " �P  %s%s" ANSI_COLOR(0) "", data, genbuf, save_page_requestor, data);

    memset(&mywin, 0, sizeof(mywin));
    memset(&itswin, 0, sizeof(itswin));

    i = b_lines >> 1;
    mywin.eline = i - 1;
    itswin.curln = itswin.sline = i + 1;
    itswin.eline = b_lines - 1;

    clear();
    move(i, 0);
    outs(mid_line);
    move(0, 0);

    add_io(fd, 0);

    while (1) {
	ch = igetch();
	if (ch == I_OTHERDATA) {
	    datac = recv(fd, data, sizeof(data), 0);
	    if (datac <= 0)
		break;
	    for (i = 0; i < datac; i++)
		do_talk_char(&itswin, data[i], flog);
	} else if (ch == KEY_UNKNOWN) {
	  // skip
	} else {
	    if (ch == Ctrl('C')) {
		if (im_leaving)
		    break;
		move(b_lines, 0);
		clrtoeol();
		outs("�A���@�� Ctrl-C �N��������͸��o�I");
		im_leaving = 1;
		continue;
	    }
	    if (im_leaving) {
		move(b_lines, 0);
		clrtoeol();
		im_leaving = 0;
	    }
	    switch (ch) {
	    case KEY_LEFT:	/* ��2byte����אּ�@byte */
		ch = Ctrl('B');
		break;
	    case KEY_RIGHT:
		ch = Ctrl('F');
		break;
	    case KEY_UP:
		ch = Ctrl('P');
		break;
	    case KEY_DOWN:
		ch = Ctrl('N');
		break;
	    }
	    data[0] = (char)ch;
	    if (send(fd, data, 1, 0) != 1)
		break;
	    if (log)
		fputc((ch == Ctrl('M')) ? '\n' : (char)*data, log);
	    do_talk_char(&mywin, *data, flog);
	}
    }
    if (log)
	fclose(log);

    add_io(0, 0);
    close(fd);

    if (flog) {
	char            ans[4];
	int             i;

	fprintf(flog, "\n" ANSI_COLOR(33;44) "���O�e�� [%s] ...     " ANSI_RESET "\n",
		Cdatelite(&now));
	for (i = 0; i < scr_lns; i++)
	    fprintf(flog, "%.*s\n", big_picture[i].len, big_picture[i].data);
	fclose(flog);
	more(fpath, NA);
	getdata(b_lines - 1, 0, "�M��(C) ���ܳƧѿ�(M). (C/M)?[C]",
		ans, sizeof(ans), LCECHO);
	if (*ans == 'm') {
	    fileheader_t    mymail;
	    char            title[128];

	    sethomepath(genbuf, cuser.userid);
	    stampfile(genbuf, &mymail);
	    mymail.filemode = FILE_READ ;
	    strlcpy(mymail.owner, "[��.��.��]", sizeof(mymail.owner));
	    snprintf(mymail.title, sizeof(mymail.title),
		     "��ܰO�� " ANSI_COLOR(1;36) "(%s)" ANSI_RESET,
		     getuserid(currutmp->destuid));
	    sethomedir(title, cuser.userid);
	    Rename(fpath, genbuf);
	    append_record(title, &mymail, sizeof(mymail));
	} else
	    unlink(fpath);
	flog = 0;
    }
    setutmpmode(XINFO);
}

#define lockreturn(unmode, state) if(lockutmpmode(unmode, state)) return

int make_connection_to_somebody(userinfo_t *uin, int timeout){
    int sock, length, pid, ch;
    struct sockaddr_in server;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
	perror("sock err");
	unlockutmpmode();
	return -1;
    }
    server.sin_family = PF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = 0;
    if (bind(sock, (struct sockaddr *) & server, sizeof(server)) < 0) {
	close(sock);
	perror("bind err");
	unlockutmpmode();
	return -1;
    }
    length = sizeof(server);
#if defined(Solaris) && __OS_MAJOR_VERSION__ == 5 && __OS_MINOR_VERSION__ < 7
    if (getsockname(sock, (struct sockaddr *) & server, & length) < 0) {
#else
    if (getsockname(sock, (struct sockaddr *) & server, (socklen_t *) & length) < 0) {
#endif
	close(sock);
	perror("sock name err");
	unlockutmpmode();
	return -1;
    }
    currutmp->sockactive = YEA;
    currutmp->sockaddr = server.sin_port;
    currutmp->destuid = uin->uid;
    setutmpmode(PAGE);
    uin->destuip = currutmp - &SHM->uinfo[0];
    pid = uin->pid;
    if (pid > 0)
	kill(pid, SIGUSR1);
    clear();
    prints("���I�s %s.....\n��J Ctrl-D ����....", uin->userid);

    if(listen(sock, 1)<0) {
	close(sock);
	return -1;
    }
    add_io(sock, timeout);

    while (1) {
	ch = igetch();
	if (ch == I_TIMEOUT) {
	    ch = uin->mode;
	    if (!ch && uin->chatid[0] == 1 &&
		    uin->destuip == currutmp - &SHM->uinfo[0]) {
		bell();
		outmsg("���^����...");
		refresh();
	    } else if (ch == EDITING || ch == TALK || ch == CHATING ||
		    ch == PAGE || ch == MAILALL || ch == MONITOR ||
		    ch == M_FIVE || ch == CHC ||
		    (!ch && (uin->chatid[0] == 1 ||
			     uin->chatid[0] == 3))) {
		add_io(0, 0);
		close(sock);
		currutmp->sockactive = currutmp->destuid = 0;
		vmsg("�H�a�b����");
		unlockutmpmode();
		return -1;
	    } else {
		// change to longer timeout
		add_io(sock, 20);
		move(0, 0);
		outs("�A");
		bell();

		uin->destuip = currutmp - &SHM->uinfo[0];
		if (pid <= 0 || kill(pid, SIGUSR1) == -1) {
		    close(sock);
		    currutmp->sockactive = currutmp->destuid = 0;
		    add_io(0, 0);
		    vmsg(msg_usr_left);
		    unlockutmpmode();
		    return -1;
		}
		continue;
	    }
	}
	if (ch == I_OTHERDATA)
	    break;

	if (ch == '\004') {
	    add_io(0, 0);
	    close(sock);
	    currutmp->sockactive = currutmp->destuid = 0;
	    unlockutmpmode();
	    return -1;
	}
    }
    return sock;
}

void
my_talk(userinfo_t * uin, int fri_stat, char defact)
{
    int             sock, msgsock, error = 0, ch;
    pid_t           pid;
    char            c;
    char            genbuf[4];
    unsigned char   mode0 = currutmp->mode;
    userec_t        xuser;

    genbuf[0] = defact;
    ch = uin->mode;
    strlcpy(currauthor, uin->userid, sizeof(currauthor));

    if (ch == EDITING || ch == TALK || ch == CHATING || ch == PAGE ||
	ch == MAILALL || ch == MONITOR || ch == M_FIVE || ch == CHC ||
	ch == DARK || ch == UMODE_GO || ch == CHESSWATCHING || ch == REVERSI ||
	(!ch && (uin->chatid[0] == 1 || uin->chatid[0] == 3)) ||
	uin->lockmode == M_FIVE || uin->lockmode == CHC) {
	if (ch == CHC || ch == M_FIVE || ch == UMODE_GO ||
		ch == CHESSWATCHING || ch == REVERSI) {
	    sock = make_connection_to_somebody(uin, 20);
	    if (sock < 0)
		vmsg("�L�k�إ߳s�u");
	    else {
#if defined(Solaris) && __OS_MAJOR_VERSION__ == 5 && __OS_MINOR_VERSION__ < 7
		msgsock = accept(sock, (struct sockaddr *) 0, 0);
#else
		msgsock = accept(sock, (struct sockaddr *) 0, (socklen_t *) 0);
#endif
		if (msgsock == -1) {
		    perror("accept");
		    close(sock);
		    return;
		}
		close(sock);
		strlcpy(currutmp->mateid, uin->userid, sizeof(currutmp->mateid));

		switch (uin->sig) {
		    case SIG_CHC:
			chc(msgsock, CHESS_MODE_WATCH);
			break;

		    case SIG_GOMO:
			gomoku(msgsock, CHESS_MODE_WATCH);
			break;

		    case SIG_GO:
			gochess(msgsock, CHESS_MODE_WATCH);
			break;

		    case SIG_REVERSI:
			reversi(msgsock, CHESS_MODE_WATCH);
			break;
		}
	    }
	}
	else
	    outs("�H�a�b����");
    } else if (!HasUserPerm(PERM_SYSOP) &&
	       (((fri_stat & HRM) && !(fri_stat & HFM)) ||
		((!uin->pager) && !(fri_stat & HFM)))) {
	outs("��������I�s���F");
    } else if (!HasUserPerm(PERM_SYSOP) &&
	     (((fri_stat & HRM) && !(fri_stat & HFM)) || uin->pager == PAGER_DISABLE)) {
	outs("���ޱ��I�s���F");
    } else if (!HasUserPerm(PERM_SYSOP) &&
	       !(fri_stat & HFM) && uin->pager == PAGER_FRIENDONLY) {
	outs("���u�����n�ͪ��I�s");
    } else if (!(pid = uin->pid) /* || (kill(pid, 0) == -1) */ ) {
	//resetutmpent();
	outs(msg_usr_left);
    } else {
	int i,j;

	if (!defact) {
	    showplans(uin->userid);
	    move(2, 0);
	    for(i=0;i<2;i++) {
		if(uin->withme & (WITHME_ALLFLAG<<i)) {
		    if(i==0)
			outs("�w���ڡG");
		    else
			outs("�ЧO��ڡG");
		    for(j=0; j<32 && withme_str[j/2]; j+=2)
			if(uin->withme & (1<<(j+i)))
			    if(withme_str[j/2]) {
				outs(withme_str[j/2]);
				outc(' ');
			    }
		    outc('\n');
		}
	    }
	    move(4, 0);
	    outs("�n�M�L(�o) (T)�ͤ�(F)�U���l��(P)���d��(C)�U�H��(D)�U�t��(G)�U���(R)�U�¥մ�");
	    getdata(5, 0, "           (N)�S�Ƨ���H�F?[N] ", genbuf, 4, LCECHO);
	}

	switch (*genbuf) {
	case 'y':
	case 't':
	    uin->sig = SIG_TALK;
	    break;
	case 'f':
	    lockreturn(M_FIVE, LOCK_THIS);
	    uin->sig = SIG_GOMO;
	    break;
	case 'c':
	    lockreturn(CHC, LOCK_THIS);
	    uin->sig = SIG_CHC;
	    break;
	case 'd':
	    uin->sig = SIG_DARK;
	    break;
	case 'g':
	    uin->sig = SIG_GO;
	    break;
	case 'r':
	    uin->sig = SIG_REVERSI;
	    break;
	case 'p':
	    reload_chicken();
	    getuser(uin->userid, &xuser);
	    if (uin->lockmode == CHICKEN || currutmp->lockmode == CHICKEN)
		error = 1;
	    if (!cuser.mychicken.name[0] || !xuser.mychicken.name[0])
		error = 2;
	    if (error) {
		vmsg(error == 2 ? "�ëD��H���i�d��" :
		       "���@�誺�d�����b�ϥΤ�");
		return;
	    }
	    uin->sig = SIG_PK;
	    break;
	default:
	    return;
	}

	uin->turn = 1;
	currutmp->turn = 0;
	strlcpy(uin->mateid, currutmp->userid, sizeof(uin->mateid));
	strlcpy(currutmp->mateid, uin->userid, sizeof(currutmp->mateid));

	sock = make_connection_to_somebody(uin, 5);
	if(sock==-1) {
	    vmsg("�L�k�إ߳s�u");
	    return;
	}

#if defined(Solaris) && __OS_MAJOR_VERSION__ == 5 && __OS_MINOR_VERSION__ < 7
	msgsock = accept(sock, (struct sockaddr *) 0, 0);
#else
	msgsock = accept(sock, (struct sockaddr *) 0, (socklen_t *) 0);
#endif
	if (msgsock == -1) {
	    perror("accept");
	    close(sock);
	    unlockutmpmode();
	    return;
	}
	add_io(0, 0);
	close(sock);
	currutmp->sockactive = NA;

	if (uin->sig == SIG_CHC || uin->sig == SIG_GOMO ||
		uin->sig == SIG_GO || uin->sig == SIG_REVERSI)
	    ChessEstablishRequest(msgsock);

	add_io(msgsock, 0);
	while ((ch = igetch()) != I_OTHERDATA) {
	    if (ch == Ctrl('D')) {
		add_io(0, 0);
		close(msgsock);
		unlockutmpmode();
		return;
	    }
	}

	if (read(msgsock, &c, sizeof(c)) != sizeof(c))
	    c = 'n';
	add_io(0, 0);

	if (c == 'y') {
	    snprintf(save_page_requestor, sizeof(save_page_requestor),
		     "%s (%s)", uin->userid, uin->nickname);
	    switch (uin->sig) {
	    case SIG_DARK:
		main_dark(msgsock, uin);
		break;
	    case SIG_PK:
		chickenpk(msgsock);
		break;
	    case SIG_GOMO:
		gomoku(msgsock, CHESS_MODE_VERSUS);
		break;
	    case SIG_CHC:
		chc(msgsock, CHESS_MODE_VERSUS);
		break;
	    case SIG_GO:
		gochess(msgsock, CHESS_MODE_VERSUS);
		break;
	    case SIG_REVERSI:
		reversi(msgsock, CHESS_MODE_VERSUS);
		break;
	    case SIG_TALK:
	    default:
		do_talk(msgsock);
	    }
	} else {
	    move(9, 9);
	    outs("�i�^���j ");
	    switch (c) {
	    case 'a':
		outs("�ڲ{�b�ܦ��A�е��@�|��A call �ڡA�n�ܡH");
		break;
	    case 'b':
		prints("�藍�_�A�ڦ��Ʊ������A %s....", sig_des[uin->sig]);
		break;
	    case 'd':
		outs("�ڭn�����o..�U���A��a..........");
		break;
	    case 'c':
		outs("�Ф��n�n�ڦn�ܡH");
		break;
	    case 'e':
		outs("��ڦ��ƶܡH�Х��ӫH��....");
		break;
	    case 'f':
		{
		    char            msgbuf[60];

		    read(msgsock, msgbuf, 60);
		    prints("�藍�_�A�ڲ{�b�����A %s�A�]��\n", sig_des[uin->sig]);
		    move(10, 18);
		    outs(msgbuf);
		}
		break;
	    case '1':
		prints("%s�H����100�Ȩ��..", sig_des[uin->sig]);
		break;
	    case '2':
		prints("%s�H����1000�Ȩ��..", sig_des[uin->sig]);
		break;
	    default:
		prints("�ڲ{�b���Q %s ��.....:)", sig_des[uin->sig]);
	    }
	    close(msgsock);
	}
    }
    currutmp->mode = mode0;
    currutmp->destuid = 0;
    unlockutmpmode();
    pressanykey();
}

/* ��榡��Ѥ��� */
#define US_PICKUP       1234
#define US_RESORT       1233
#define US_ACTION       1232
#define US_REDRAW       1231

static void
t_showhelp(void)
{
    clear();

    outs(ANSI_COLOR(36) "�i �𶢲�Ѩϥλ��� �j" ANSI_RESET "\n\n"
	 "(��)(e)         �������}             (h)             �ݨϥλ���\n"
	 "(��)/(��)(n)    �W�U����             (TAB)           �����ƧǤ覡\n"
	 "(PgUp)(^B)      �W�����             ( )(PgDn)(^F)   �U�����\n"
	 "(Hm)/($)(Ed)    ��/��                (S)             �ӷ�/�n�ʹy�z/���Z ����\n"
	 "(m)             �H�H                 (q/c)           �d�ߺ���/�d��\n"
	 "(r)             �\\Ū�H��             (l/C)           �ݤW�����T/��������\n"
	 "(f)             ����/�n�ͦC��        (�Ʀr)          ���ܸӨϥΪ�\n"
	 "(p)             �����I�s��           (g/i)           ����/�����߱�\n"
	 "(a/d/o)         �n�� �W�[/�R��/�ק�  (s)             ���� ID �j�M\n"
	 "(N)             �ק�ʺ�             (y)             �ڷQ��H��ѡB�U�ѡK\n");

    if (HasUserPerm(PERM_PAGE)) {
	outs("\n" ANSI_COLOR(36) "�i ��ͱM���� �j" ANSI_RESET "\n"
	     "(��)(t)(Enter)  ��L���o���\n"
	     "(w)             ���u Call in\n"
	     "(^W)�������y�覡 �@�� / �i�� / ����\n"
	     "(b)             ��n�ͼs�� (�@�w�n�b�n�ͦC��)\n"
	     "(^R)            �Y�ɦ^�� (���H Call in �A��)\n");
    }
    if (HasUserPerm(PERM_SYSOP)) {
	outs("\n" ANSI_COLOR(36) "�i �����M���� �j" ANSI_RESET "\n\n");
	outs("(u)/(H)         �]�w�ϥΪ̸��/�������μҦ�\n");
	outs("(K)             ���a�J��X�h\n");
#if defined(SHOWBOARD) && defined(DEBUG)
	outs("(Y)             ��ܥ��b�ݤ���O\n");
#endif
    }
#ifdef PLAY_ANGEL
    if (HasUserPerm(PERM_LOGINOK))
	pressanykey_or_callangel();
    else
#endif
    pressanykey();
}

/* Kaede show friend description */
static char    *
friend_descript(const userinfo_t * uentp, char *desc_buf, int desc_buflen)
{
    char           *space_buf = "", *flag;
    char            fpath[80], name[IDLEN + 2], *desc, *ptr;
    int             len;
    FILE           *fp;
    char            genbuf[STRLEN];

    STATINC(STAT_FRIENDDESC);
    if((set_friend_bit(currutmp,uentp)&IFH)==0)
	return space_buf;

    setuserfile(fpath, friend_file[0]);

    STATINC(STAT_FRIENDDESC_FILE);
    if ((fp = fopen(fpath, "r"))) {
	snprintf(name, sizeof(name), "%s ", uentp->userid);
	len = strlen(name);
	desc = genbuf + 13;

	/* TODO maybe none linear search, or fread, or cache */
	while ((flag = fgets(genbuf, STRLEN, fp))) {
	    if (!memcmp(genbuf, name, len)) {
		if ((ptr = strchr(desc, '\n')))
		    ptr[0] = '\0';
		break;
	    }
	}
	fclose(fp);
	if (flag)
	    strlcpy(desc_buf, desc, desc_buflen);
	else
	    return space_buf;

	return desc_buf;
    } else
	return space_buf;
}

static const char    *
descript(int show_mode, const userinfo_t * uentp, int diff)
{
    static char     description[30];
    switch (show_mode) {
    case 1:
	return friend_descript(uentp, description, sizeof(description));
    case 0:
	return (((uentp->pager != PAGER_DISABLE && uentp->pager != PAGER_ANTIWB && diff) ||
		 HasUserPerm(PERM_SYSOP)) ?
#ifdef WHERE
		uentp->from_alias ? SHM->home_desc[uentp->from_alias] :
		uentp->from
#else
		uentp->from
#endif
		: "*");
    case 2:
	snprintf(description, sizeof(description),
		 "%4d/%4d/%2d %c", uentp->five_win,
		 uentp->five_lose, uentp->five_tie,
		 (uentp->withme&WITHME_FIVE)?'o':(uentp->withme&WITHME_NOFIVE)?'x':' ');
	return description;
    case 3:
	snprintf(description, sizeof(description),
		 "%4d/%4d/%2d %c", uentp->chc_win,
		 uentp->chc_lose, uentp->chc_tie,
		 (uentp->withme&WITHME_CHESS)?'o':(uentp->withme&WITHME_NOCHESS)?'x':' ');
	return description;
    case 4:
	snprintf(description, sizeof(description),
		 "%4d %s", uentp->chess_elo_rating, 
		 (uentp->withme&WITHME_CHESS)?"��ڤU��":(uentp->withme&WITHME_NOCHESS)?"�O���":"");
	return description;
    case 5:
	snprintf(description, sizeof(description),
		 "%4d/%4d/%2d %c", uentp->go_win,
		 uentp->go_lose, uentp->go_tie,
		 (uentp->withme&WITHME_GO)?'o':(uentp->withme&WITHME_NOGO)?'x':' ');
	return description;
    default:
	syslog(LOG_WARNING, "damn!!! what's wrong?? show_mode = %d",
	       show_mode);
	return "";
    }
}

/*
 * userlist
 * 
 * ���O���L�j���� bbs�b��@�ϥΪ̦W���, ���O�N�Ҧ� online users ���@����
 * local space ��, ���өҶ��n���覡 sort �n (�p���� userid , ���l��, �ӷ���
 * ��) . �o�N�y���j�q�����O: ������C�ӤH���n���F���ͳo�@���� 20 �ӤH�����
 * �ӥh sort ��L�@�U�H�����?
 *
 * �@��ӻ�, �@�����㪺�ϥΪ̦W��i�H�����u�n�Ͱϡv�M�u�D�n�Ͱϡv. ���P�H��
 * �u�n�Ͱϡv���ӷ|�������@��, ���L�u�D�n�Ͱϡv���ӬO�����@�˪�. �w��o���S
 * ��, ��Ϧ����P����@�覡.
 *
 * + �n�Ͱ�
 *   �n�Ͱϥu���b�ƦC�覡�� [��! �B��] ���ɭԡu�i��v�|�Ψ�.
 *   �C�� process�i�H�z�L currutmp->friend_online[]�o�줬�۶����n�����Y����
 *   �� (���]�A�O��, �O�ͬO�t�~�ͥX�Ӫ�) ���L friend_online[]�O unorder��.
 *   �ҥH���n����Ҧ����H���X��, ���s sort �@��.
 *   �n�Ͱ� (���ۥ��@�観�]�n��+ �O��) �̦h�u�|�� MAX_FRIENDS��
 *   �]�����ͦn�ͰϪ� cost �۷�, "�ण���ʹN���n����"
 *
 * + �D�n�Ͱ�
 *   �z�L shmctl utmpsortd , �w�� (�q�`�@��@��) �N�������H���ӦU�ؤ��P����
 *   �� sort �n, ��m�b SHM->sorted��.
 *
 * ���U��, �ڭ̨C���u�q�T�w���_�l��m��, �S�O�O���D�����n, ���M���|�h���ͦn
 * �Ͱ�. 
 *
 * �U�� function �K�n
 * sort_cmpfriend()   sort function, key: friend type
 * pickup_maxpages()  # pages of userlist
 * pickup_myfriend()  ���ͦn�Ͱ�
 * pickup_bfriend()   ���ͪO��
 * pickup()           ���ͬY�@���ϥΪ̦W��
 * draw_pickup()      ��e����X
 * userlist()         �D�禡, �t�d�I�s pickup()/draw_pickup() �H�Ϋ���B�z
 *
 * SEE ALSO
 *     include/pttstruct.h
 *
 * BUGS
 *     �j�M���ɭԨS����k����ӤH�W��
 *
 * AUTHOR
 *     in2 <in2@in2home.org>
 */
char    nPickups;

static int
sort_cmpfriend(const void *a, const void *b)
{
    if (((((pickup_t *) a)->friend) & ST_FRIEND) ==
	((((pickup_t *) b)->friend) & ST_FRIEND))
	return strcasecmp(((pickup_t *) a)->ui->userid,
			  ((pickup_t *) b)->ui->userid);
    else
	return (((pickup_t *) b)->friend & ST_FRIEND) -
	    (((pickup_t *) a)->friend & ST_FRIEND);
}

int
pickup_maxpages(int pickupway, int nfriends)
{
    int             number;
    if (cuser.uflag & FRIEND_FLAG)
	number = nfriends;
    else
	number = SHM->UTMPnumber +
	    (pickupway == 0 ? nfriends : 0);
    return (number - 1) / nPickups + 1;
}

static int
pickup_myfriend(pickup_t * friends,
		int *myfriend, int *friendme, int *badfriend)
{
    userinfo_t     *uentp;
    int             i, where, frstate, ngets = 0;

    STATINC(STAT_PICKMYFRIEND);
    *badfriend = 0;
    *myfriend = *friendme = 1;
    for (i = 0; currutmp->friend_online[i] && i < MAX_FRIEND; ++i) {
	where = currutmp->friend_online[i] & 0xFFFFFF;
	if (VALID_USHM_ENTRY(where) &&
	    (uentp = &SHM->uinfo[where]) && uentp->pid &&
	    uentp != currutmp &&
	    isvisible_stat(currutmp, uentp,
			   frstate =
			   currutmp->friend_online[i] >> 24)){
	    if( frstate & IRH )
		++*badfriend;
	    if( !(frstate & IRH) || ((frstate & IRH) && (frstate & IFH)) ){
		friends[ngets].ui = uentp;
		friends[ngets].uoffset = where;
		friends[ngets++].friend = frstate;
		if (frstate & IFH)
		    ++* myfriend;
		if (frstate & HFM)
		    ++* friendme;
	    }
	}
    }
    /* ��ۤv�[�J�n�Ͱ� */
    friends[ngets].ui = currutmp;
    friends[ngets++].friend = (IFH | HFM);
    return ngets;
}

static int
pickup_bfriend(pickup_t * friends, int base)
{
    userinfo_t     *uentp;
    int             i, ngets = 0;
    int             currsorted = SHM->currsorted, number = SHM->UTMPnumber;

    STATINC(STAT_PICKBFRIEND);
    friends = friends + base;
    for (i = 0; i < number && ngets < MAX_FRIEND - base; ++i) {
	uentp = &SHM->uinfo[SHM->sorted[currsorted][0][i]];
	/* TODO isvisible() ���ƥΨ�F friend_stat() */
	if (uentp && uentp->pid && uentp->brc_id == currutmp->brc_id &&
	    currutmp != uentp && isvisible(currutmp, uentp) &&
	    (base || !(friend_stat(currutmp, uentp) & (IFH | HFM)))) {
	    friends[ngets].ui = uentp;
	    friends[ngets++].friend = IBH;
	}
    }
    return ngets;
}

static void
pickup(pickup_t * currpickup, int pickup_way, int *page,
       int *nfriend, int *myfriend, int *friendme, int *bfriend, int *badfriend)
{
    /* avoid race condition */
    int             currsorted = SHM->currsorted;
    int             utmpnumber = SHM->UTMPnumber;
    int             friendtotal = currutmp->friendtotal;

    int    *ulist;
    userinfo_t *u;
    int             which, sorted_way, size = 0, friend;

    if (friendtotal == 0)
	*myfriend = *friendme = 1;

    /* ���ͦn�Ͱ� */
    which = *page * nPickups;
    if( (cuser.uflag & FRIEND_FLAG) || /* �u��ܦn�ͼҦ� */
	((pickup_way == 0) &&          /* [��! �B��] mode */
	 (
	  /* �t�O��, �n�Ͱϳ̦h�u�|�� (friendtotal + �O��) ��*/
	  (currutmp->brc_id && which < (friendtotal + 1 +
					getbcache(currutmp->brc_id)->nuser)) ||
	  
	  /* ���t�O��, �̦h�u�|�� friendtotal�� */
	  (!currutmp->brc_id && which < friendtotal + 1)
	  ))) {
	pickup_t        friends[MAX_FRIEND];

	/* TODO �� friendtotal<which �ɥu����ܪO��, ���� pickup_myfriend */
	*nfriend = pickup_myfriend(friends, myfriend, friendme, badfriend);

	if (pickup_way == 0 && currutmp->brc_id != 0
#ifdef USE_COOLDOWN
		&& !(getbcache(currutmp->brc_id)->brdattr & BRD_COOLDOWN)
#endif
		){
	    /* TODO �u�ݭn which+nPickups-*nfriend �ӪO��, ���@�w�n��ӱ��@�M */
	    *nfriend += pickup_bfriend(friends, *nfriend);
	    *bfriend = SHM->bcache[currutmp->brc_id - 1].nuser;
	}
	else
	    *bfriend = 0;
	if (*nfriend > which) {
	    /* �u���b�n�q�X�~�����n sort */
	    /* TODO �n�͸�O�ͥi�H���} sort, �i��u�ݭn��@ */
	    /* TODO �n�ͤW�U���~�ݭn sort �@��, ���ݭn�C�� sort. 
	     * �i���@�@�� dirty bit ��ܬO�_ sort �L. 
	     * suggested by WYchuang@ptt */
	    qsort(friends, *nfriend, sizeof(pickup_t), sort_cmpfriend);
	    size = *nfriend - which;
	    if (size > nPickups)
		size = nPickups;
	    memcpy(currpickup, friends + which, sizeof(pickup_t) * size);
	}
    } else
	*nfriend = 0;

    if (!(cuser.uflag & FRIEND_FLAG) && size < nPickups) {
	sorted_way = ((pickup_way == 0) ? 7 : (pickup_way - 1));
	ulist = SHM->sorted[currsorted][sorted_way];
	which = *page * nPickups - *nfriend;
	if (which < 0)
	    which = 0;

	for (; which < utmpnumber && size < nPickups; which++) {
	    u = &SHM->uinfo[ulist[which]];

	    friend = friend_stat(currutmp, u);
	    /* TODO isvisible() ���ƥΨ�F friend_stat() */
	    if ((pickup_way ||
		 (currutmp != u && !(friend & ST_FRIEND))) &&
		isvisible(currutmp, u)) {
		currpickup[size].ui = u;
		currpickup[size++].friend = friend;
	    }
	}
    }

    for (; size < nPickups; ++size)
	currpickup[size].ui = 0;
}

static void
draw_pickup(int drawall, pickup_t * pickup, int pickup_way,
	    int page, int show_mode, int show_uid, int show_board,
	    int show_pid, int myfriend, int friendme, int bfriend, int badfriend)
{
    char           *msg_pickup_way[PICKUP_WAYS] = {
	"��! �B��", "���ͥN��", "���ͰʺA", "�o�b�ɶ�", "�Ӧۦ��", " ���l�� ", "  �H��  ", "  ���  ",
    };
    char           *MODE_STRING[MAX_SHOW_MODE] = {
	"�G�m", "�n�ʹy�z", "���l�Ѿ��Z", "�H�Ѿ��Z", "�H�ѵ��Ť�", "��Ѿ��Z",
    };
    char            pagerchar[5] = "* -Wf";

    userinfo_t     *uentp;
    int             i, ch, state, friend;
    char            mind[5];

#ifdef SHOW_IDLE_TIME
    char            idlestr[32];
    int             idletime;
#endif

    /* wide screen support */
    int wNick = 17, wMode = 12; //13; , one byte give number for ptt always > 10000 online.

    if (t_columns > 80)
    {
	int d = t_columns - 80;

	/* rule: try to give extra space to both nick and mode,
	 * because nick is smaller, try nick first then mode. */
	if (d >= sizeof(cuser.nickname) - wNick)
	{
	    d -= (sizeof(cuser.nickname) - wNick);
	    wNick = sizeof(cuser.nickname);
	    wMode += d;
	} else {
	    wNick += d;
	}
    }

    if (drawall) {
	showtitle((cuser.uflag & FRIEND_FLAG) ? "�n�ͦC��" : "�𶢲��",
		  BBSName);
	prints("\n"
	       ANSI_COLOR(7) "   %s P%c�N��         %-*s%-17s%-*s%10s" 
	       ANSI_RESET "\n",
	       show_uid ? "UID " : "�s��",
	       (HasUserPerm(PERM_SEECLOAK) || HasUserPerm(PERM_SYSOP)) ? 
	       		'C' : ' ',
		wNick, "�ʺ�",
	       MODE_STRING[show_mode],
	        wMode, show_board ? "Board" : "�ʺA",
	       show_pid ? "       PID" : "�߱�  "
#ifdef SHOW_IDLE_TIME
	       "�o�b"
#else
	       "    "
#endif
	    );
	move(b_lines, 0);
	outslr(	
		ANSI_COLOR(34;46) " �𶢲�� "
		ANSI_COLOR(31;47) " (TAB/f)" ANSI_COLOR(30) "�Ƨ�/�n�� " 
		ANSI_COLOR(31) "(a/o)" ANSI_COLOR(30) "��� " 
		ANSI_COLOR(31) "(q/w)" ANSI_COLOR(30) "�d��/����y "
		ANSI_COLOR(31) "(t/m)" ANSI_COLOR(30) "���/�g�H ",
		80-10,
		ANSI_COLOR(31) "(h)" ANSI_COLOR(30) "���� " ANSI_RESET,
		8);
    }
    move(1, 0);
    prints("  �Ƨ�:[%s] �W���H��:%-4d " 
	    ANSI_COLOR(1;32) "�ڪ��B��:%-3d "
	   ANSI_COLOR(33) "�P�ڬ���:%-3d " 
	   ANSI_COLOR(36) "�O��:%-4d " 
	   ANSI_COLOR(31) "�a�H:%-2d" 
	   ANSI_RESET "\n",
	   msg_pickup_way[pickup_way], SHM->UTMPnumber,
	   myfriend, friendme, currutmp->brc_id ? bfriend : 0, badfriend);

    for (i = 0, ch = page * nPickups + 1; i < nPickups; ++i, ++ch) {
	move(i + 3, 0);
	outc('a');
	move(i + 3, 0);
	uentp = pickup[i].ui;
	friend = pickup[i].friend;
	if (uentp == NULL) {
	    outc('\n');
	    continue;
	}
	if (!uentp->pid) {
	    prints("%5d   < ������..>\n", ch);
	    continue;
	}
	if (PERM_HIDE(uentp))
	    state = 9;
	else if (currutmp == uentp)
	    state = 10;
	else if (friend & IRH && !(friend & IFH))
	    state = 8;
	else
	    state = (friend & ST_FRIEND) >> 2;

#ifdef SHOW_IDLE_TIME
	idletime = (now - uentp->lastact);
	if (idletime > 86400)
	    strlcpy(idlestr, " -----", sizeof(idlestr));
	else if (idletime >= 3600)
	    snprintf(idlestr, sizeof(idlestr), "%3dh%02d",
		     idletime / 3600, (idletime / 60) % 60);
	else if (idletime > 0)
	    snprintf(idlestr, sizeof(idlestr), "%3d'%02d",
		     idletime / 60, idletime % 60);
	else
	    strlcpy(idlestr, "      ", sizeof(idlestr));
#endif

	if ((uentp->userlevel & PERM_VIOLATELAW))
	    memcpy(mind, "�q�r", 4);
	else if (uentp->birth)
	    memcpy(mind, "�جP", 4);
	else
	    memcpy(mind, uentp->mind, 4);
	mind[4] = 0;

	/* TODO
	 * will this be faster if we use pure outc/outs?
	 */
	prints("%7d %c%c%s%-13s%-*.*s " ANSI_RESET "%-16.16s %-*.*s"
	       ANSI_COLOR(33) "%-4.4s" ANSI_RESET "%s\n",

	/* list number or uid */
#ifdef SHOWUID
	       show_uid ? uentp->uid :
#endif
	       ch,

	/* super friend or pager */
	       (friend & HRM) ? 'X' : pagerchar[uentp->pager % 5],

	/* visibility */
	       (uentp->invisible ? ')' : ' '),

	/* color of userid, userid */
	       fcolor[state], uentp->userid,

	/* nickname */
	       wNick-1, wNick-1, uentp->nickname,

	/* from */
	       descript(show_mode, uentp,
			uentp->pager & !(friend & HRM)),

	/* board or mode */
	       wMode, wMode,
#if defined(SHOWBOARD) && defined(DEBUG)
	       show_board ? (uentp->brc_id == 0 ? "" :
			     getbcache(uentp->brc_id)->brdname) :
#endif
	       modestring(uentp, 0),

	/* memo */
	       mind,

	/* idle */
#ifdef SHOW_IDLE_TIME
	       idlestr
#else
	       ""
#endif
	    );

	//refresh();
    }
}

void set_withme_flag(void)
{
    int i;
    char genbuf[20];
    int line;
    
    move(1, 0);
    clrtobot();

    do {
	move(1, 0);
	line=1;
	for(i=0; i<16 && withme_str[i]; i++) {
	    clrtoeol();
	    if(currutmp->withme&(1<<(i*2)))
		prints("[%c] �ګܷQ��H%s, �w�����H���\n",'a'+i, withme_str[i]);
	    else if(currutmp->withme&(1<<(i*2+1)))
		prints("[%c] �ڤ��ӷQ%s\n",'a'+i, withme_str[i]);
	    else
		prints("[%c] (%s)�S�N��\n",'a'+i, withme_str[i]);
	    line++;
	}
	getdata(line,0,"�Φr������ [�Q/���Q/�S�N��]",genbuf, sizeof(genbuf), DOECHO);
	for(i=0;genbuf[i];i++) {
	    int ch=genbuf[i];
	    ch=tolower(ch);
	    if('a'<=ch && ch<'a'+16) {
		ch-='a';
		if(currutmp->withme&(1<<ch*2)) {
		    currutmp->withme&=~(1<<ch*2);
		    currutmp->withme|=1<<(ch*2+1);
		} else if(currutmp->withme&(1<<(ch*2+1))) {
		    currutmp->withme&=~(1<<(ch*2+1));
		} else {
		    currutmp->withme|=1<<(ch*2);
		}
	    }
	}
    } while(genbuf[0]!='\0');
}

int
call_in(const userinfo_t * uentp, int fri_stat)
{
    if (iswritable_stat(uentp, fri_stat)) {
	char            genbuf[60];
	snprintf(genbuf, sizeof(genbuf), "�� %s ���y: ", uentp->userid);
	my_write(uentp->pid, genbuf, uentp->userid, WATERBALL_GENERAL, NULL);
	return 1;
    }
    return 0;
}

inline static void
userlist(void)
{
    pickup_t       *currpickup;
    userinfo_t     *uentp;
    static char     show_mode = 0;
    static char     show_uid = 0;
    static char     show_board = 0;
    static char     show_pid = 0;
    static int      pickup_way = 0;
    char            skippickup = 0, redraw, redrawall;
    int             page, offset, ch, leave, fri_stat;
    int             nfriend, myfriend, friendme, bfriend, badfriend, i;
    time4_t          lastupdate;

    nPickups = b_lines - 3;
    currpickup = (pickup_t *)malloc(sizeof(pickup_t) * nPickups);
    page = offset = 0 ;
    nfriend = myfriend = friendme = bfriend = badfriend = 0;
    leave = 0;
    redrawall = 1;
    /*
     * �U�� flag :
     * redraw:    ���s pickup(), draw_pickup() (�Ȥ�����, ���t���D�C����)
     * redrawall: �������e (�t���D�C����, ���A���w redraw �~�|����)
     * leave:     ���}�ϥΪ̦W��
     */
    while (!leave) {
	if( !skippickup )
	    pickup(currpickup, pickup_way, &page,
		   &nfriend, &myfriend, &friendme, &bfriend, &badfriend);
	draw_pickup(redrawall, currpickup, pickup_way, page,
		    show_mode, show_uid, show_board, show_pid,
		    myfriend, friendme, bfriend, badfriend);

	/*
	 * �p�G�]���������ɭ�, �o�@�������H�Ƥ����,
	 * (�q�`���O�̫�@���H�Ƥ������ɭ�) ���n���s�p�� offset
	 * �H�K����S���H���a��
	 */
	if (offset == -1 || currpickup[offset].ui == NULL) {
	    for (offset = (offset == -1 ? nPickups - 1 : offset);
		 offset >= 0; --offset)
		if (currpickup[offset].ui != NULL)
		    break;
	    if (offset == -1) {
		if (--page < 0)
		    page = pickup_maxpages(pickup_way, nfriend) - 1;
		offset = 0;
		continue;
	    }
	}
	skippickup = redraw = redrawall = 0;
	lastupdate = now;
	while (!redraw) {
	    ch = cursor_key(offset + 3, 0);
	    uentp = currpickup[offset].ui;
	    fri_stat = currpickup[offset].friend;

	    switch (ch) {
	    case KEY_LEFT:
	    case 'e':
	    case 'E':
		redraw = leave = 1;
		break;

	    case KEY_TAB:
		pickup_way = (pickup_way + 1) % PICKUP_WAYS;
		redraw = 1;
		redrawall = 1;
		break;

	    case KEY_DOWN:
	    case 'n':
	    case 'j':
		if (++offset == nPickups || currpickup[offset].ui == NULL) {
		    redraw = 1;
		    if (++page >= pickup_maxpages(pickup_way,
						  nfriend))
			offset = page = 0;
		    else
			offset = 0;
		}
		break;

	    case '0':
	    case KEY_HOME:
		page = offset = 0;
		redraw = 1;
		break;

	    case 'H':
		if (HasUserPerm(PERM_SYSOP)||HasUserPerm(PERM_OLDSYSOP)) {
		    currutmp->userlevel ^= PERM_SYSOPHIDE;
		    redrawall = redraw = 1;
		}
		break;

	    case 'C':
#if !HAVE_FREECLOAK
		if (HasUserPerm(PERM_CLOAK))
#endif
		{
		    currutmp->invisible ^= 1;
		    redrawall = redraw = 1;
		}
		break;

	    case ' ':
	    case KEY_PGDN:
	    case Ctrl('F'):{
		    int             newpage;
		    if ((newpage = page + 1) >= pickup_maxpages(pickup_way,
								nfriend))
			newpage = offset = 0;
		    if (newpage != page) {
			page = newpage;
			redraw = 1;
		    } else if (now >= lastupdate + 2)
			redrawall = redraw = 1;
		}
		break;

	    case KEY_UP:
	    case 'k':
		if (--offset == -1) {
		    offset = nPickups - 1;
		    if (--page == -1)
			page = pickup_maxpages(pickup_way, nfriend)
			    - 1;
		    redraw = 1;
		}
		break;

	    case KEY_PGUP:
	    case Ctrl('B'):
	    case 'P':
		if (--page == -1)
		    page = pickup_maxpages(pickup_way, nfriend) - 1;
		offset = 0;
		redraw = 1;
		break;

	    case KEY_END:
	    case '$':
		page = pickup_maxpages(pickup_way, nfriend) - 1;
		offset = -1;
		redraw = 1;
		break;

	    case '/':
		/*
		 * getdata_buf(b_lines-1,0,"�п�J�ʺ�����r:", keyword,
		 * sizeof(keyword), DOECHO); state = US_PICKUP;
		 */
		break;

	    case 's':
		if (!(cuser.uflag & FRIEND_FLAG)) {
		    int             si;	/* utmpshm->sorted[X][0][si] */
		    int             fi;	/* allpickuplist[fi] */
		    char            swid[IDLEN + 1];
		    move(1, 0);
		    si = CompleteOnlineUser(msg_uid, swid);
		    if (si >= 0) {
			pickup_t        friends[MAX_FRIEND + 1];
			int             nGots, i;
			int *ulist =
			    SHM->sorted[SHM->currsorted]
			    [((pickup_way == 0) ? 0 : (pickup_way - 1))];
			
			fi = ulist[si];
			nGots = pickup_myfriend(friends, &myfriend,
						&friendme, &badfriend);
			for (i = 0; i < nGots; ++i)
			    if (friends[i].uoffset == fi)
				break;

			fi = 0;
			offset = 0;
			if( i != nGots ){
			    page = i / nPickups;
			    for( ; i < nGots && fi < nPickups ; ++i )
				if( isvisible(currutmp, friends[i].ui) )
				    currpickup[fi++] = friends[i];
			    i = 0;
			}
			else{
			    page = (si + nGots) / nPickups;
			    i = si;
			}

			for( ; fi < nPickups && i < SHM->UTMPnumber ; ++i )
			{
			    userinfo_t *u;
			    u = &SHM->uinfo[ulist[i]];
			    if( isvisible(currutmp, u) ){
				currpickup[fi].ui = u;
				currpickup[fi++].friend = 0;
			    }
			}
			skippickup = 1;
		    }
		    redrawall = redraw = 1;
		}
		/*
		 * if ((i = search_pickup(num, actor, pklist)) >= 0) num = i;
		 * state = US_ACTION;
		 */
		break;

	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
		{		/* Thor: �i�H���Ʀr����ӤH */
		    int             tmp;
		    if ((tmp = search_num(ch, SHM->UTMPnumber)) >= 0) {
			if (tmp / nPickups == page) {
			    /*
			     * in2:�ت��b�ثe�o�@��, ���� ��s offset ,
			     * ���έ��e�e��
			     */
			    offset = tmp % nPickups;
			} else {
			    page = tmp / nPickups;
			    offset = tmp % nPickups;
			}
			redrawall = redraw = 1;
		    }
		}
		break;

#ifdef SHOWUID
	    case 'U':
		if (HasUserPerm(PERM_SYSOP)) {
		    show_uid ^= 1;
		    redrawall = redraw = 1;
		}
		break;
#endif
#if defined(SHOWBOARD) && defined(DEBUG)
	    case 'Y':
		if (HasUserPerm(PERM_SYSOP)) {
		    show_board ^= 1;
		    redrawall = redraw = 1;
		}
		break;
#endif
#ifdef  SHOWPID
	    case '#':
		if (HasUserPerm(PERM_SYSOP)) {
		    show_pid ^= 1;
		    redrawall = redraw = 1;
		}
		break;
#endif

	    case 'b':		/* broadcast */
		if (cuser.uflag & FRIEND_FLAG || HasUserPerm(PERM_SYSOP)) {
		    char            genbuf[60]="[�s��]";
		    char            ans[4];

		    if (!getdata(0, 0, "�s���T��:", genbuf+6, 54, DOECHO))
			break;
                    
		    if (!getdata(0, 0, "�T�w�s��? [N]",
				ans, sizeof(ans), LCECHO) ||
			ans[0] != 'y')
			break;
		    if (!(cuser.uflag & FRIEND_FLAG) && HasUserPerm(PERM_SYSOP)) {
			msgque_t msg;
			getdata(1, 0, "�A���T�w�����s��? [N]",
				ans, sizeof(ans), LCECHO);
			if( ans[0] != 'y' && ans[0] != 'Y' ){
			    vmsg("abort");
			    break;
			}

			msg.pid = currpid;
			strlcpy(msg.userid, cuser.userid, sizeof(msg.userid));
			snprintf(msg.last_call_in, sizeof(msg.last_call_in),
				 "[�s��]%s", genbuf);
			for (i = 0; i < SHM->UTMPnumber; ++i) {
			    // XXX why use sorted list?
			    //     can we just scan uinfo with proper checking?
			    uentp = &SHM->uinfo[
                                      SHM->sorted[SHM->currsorted][0][i]];
			    if (uentp->pid && kill(uentp->pid, 0) != -1){
				int     write_pos = uentp->msgcount;
				if( write_pos < (MAX_MSGS - 1) ){
				    uentp->msgcount = write_pos + 1;
				    memcpy(&uentp->msgs[write_pos], &msg,
					   sizeof(msg));
#ifdef NOKILLWATERBALL
				    uentp->wbtime = now;
#else
				    kill(uentp->pid, SIGUSR2);
#endif
				}
			    }
			}
		    } else {
			userinfo_t     *uentp;
			int             where, frstate;
			for (i = 0; currutmp->friend_online[i] &&
			     i < MAX_FRIEND; ++i) {
			    where = currutmp->friend_online[i] & 0xFFFFFF;
			    if (VALID_USHM_ENTRY(where) &&
				(uentp = &SHM->uinfo[where]) &&
				uentp->pid &&
				isvisible_stat(currutmp, uentp,
					       frstate =
					   currutmp->friend_online[i] >> 24)
				&& kill(uentp->pid, 0) != -1 &&
				uentp->pager != PAGER_ANTIWB &&
				(uentp->pager != PAGER_FRIENDONLY || frstate & HFM) &&
				!(frstate & IRH)) {
				my_write(uentp->pid, genbuf, uentp->userid,
					WATERBALL_PREEDIT, NULL);
			    }
			}
		    }
		    redrawall = redraw = 1;
		}
		break;

	    case 'S':		/* ��ܦn�ʹy�z */
		show_mode = (show_mode+1) % MAX_SHOW_MODE;
#ifdef CHESSCOUNTRY
		if (show_mode == 2)
		    user_query_mode = 1;
		else if (show_mode == 3 || show_mode == 4)
		    user_query_mode = 2;
		else if (show_mode == 5)
		    user_query_mode = 3;
		else
		    user_query_mode = 0;
#endif /* defined(CHESSCOUNTRY) */
		redrawall = redraw = 1;
		break;

	    case 'u':		/* �u�W�ק��� */
		if (HasUserPerm(PERM_ACCOUNTS|PERM_SYSOP)) {
		    int             id;
		    userec_t        muser;
		    strlcpy(currauthor, uentp->userid, sizeof(currauthor));
		    stand_title("�ϥΪ̳]�w");
		    move(1, 0);
		    if ((id = getuser(uentp->userid, &muser)) > 0) {
			user_display(&muser, 1);
			if( HasUserPerm(PERM_ACCOUNTS) )
			    uinfo_query(&muser, 1, id);
			else
			    pressanykey();
		    }
		    redrawall = redraw = 1;
		}
		break;

	    case 'i':{
		    char            mindbuf[5];
		    getdata(b_lines - 1, 0, "�{�b���߱�? ",
			    mindbuf, sizeof(mindbuf), DOECHO);
		    if (strcmp(mindbuf, "�q�r") == 0)
			vmsg("���i�H��ۤv�]�q�r��!");
		    else if (strcmp(mindbuf, "�جP") == 0)
			vmsg("�A���O���ѥͤ���!");
		    else
			memcpy(currutmp->mind, mindbuf, 4);
		}
		redrawall = redraw = 1;
		break;

	    case Ctrl('S'):
		break;

	    case KEY_RIGHT:
	    case '\n':
	    case '\r':
	    case 't':
		if (HasUserPerm(PERM_LOGINOK)) {
		    if (uentp->pid != currpid &&
			    strcmp(uentp->userid, cuser.userid) != 0) {
			move(1, 0);
			clrtobot();
			move(3, 0);
			my_talk(uentp, fri_stat, 0);
			redrawall = redraw = 1;
		    }
		}
		break;
	    case 'K':
		if (HasUserPerm(PERM_ACCOUNTS|PERM_SYSOP)) {
		    my_kick(uentp);
		    redrawall = redraw = 1;
		}
		break;
	    case 'w':
		if (call_in(uentp, fri_stat))
		    redrawall = redraw = 1;
		break;
	    case 'a':
		if (HasUserPerm(PERM_LOGINOK) && !(fri_stat & IFH)) {
		    if (getans("�T�w�n�[�J�n�Ͷ� [N/y]") == 'y') {
			friend_add(uentp->userid, FRIEND_OVERRIDE,uentp->nickname);
			friend_load(FRIEND_OVERRIDE);
		    }
		    redrawall = redraw = 1;
		}
		break;

	    case 'd':
		if (HasUserPerm(PERM_LOGINOK) && (fri_stat & IFH)) {
		    if (getans("�T�w�n�R���n�Ͷ� [N/y]") == 'y') {
			friend_delete(uentp->userid, FRIEND_OVERRIDE);
			friend_load(FRIEND_OVERRIDE);
		    }
		    redrawall = redraw = 1;
		}
		break;

	    case 'o':
		if (HasUserPerm(PERM_LOGINOK)) {
		    t_override();
		    redrawall = redraw = 1;
		}
		break;

	    case 'f':
		if (HasUserPerm(PERM_LOGINOK)) {
		    cuser.uflag ^= FRIEND_FLAG;
		    redrawall = redraw = 1;
		}
		break;

	    case 'g':
		if (HasUserPerm(PERM_LOGINOK) &&
		    strcmp(uentp->userid, cuser.userid) != 0) {
                    char genbuf[10];
		    char userid[IDLEN + 1];
		    int touid=uentp->uid;
		    strlcpy(userid, uentp->userid, sizeof(userid));
		    move(b_lines - 2, 0);
		    prints("�n�� %s �h�ֿ��O?  ", userid);
		    if (getdata(b_lines - 1, 0, "[�Ȧ���b]: ",
				genbuf, 7, LCECHO)) {
			clrtoeol();
			if ((ch = atoi(genbuf)) <= 0 || ch <= give_tax(ch)){
			    redrawall = redraw = 1;
			    break;
			}
			if (getans("�T�w�n�� %s %d Ptt ����? [N/y]",
                             userid, ch) != 'y'){
			    redrawall = redraw = 1;
			    break;
			}
			if (do_give_money(userid, touid, ch) < 0)
			    vmsgf("������ѡA�ٳѤU %d ��", SHM->money[usernum - 1]);
			else
			    vmsgf("������\\�A�ٳѤU %d ��", SHM->money[usernum - 1]);
		    } else {
			clrtoeol();
			vmsg(" �������! ");
		    }
		    redrawall = redraw = 1;
		}
		break;

	    case 'm':
		if (HasUserPerm(PERM_LOGINOK)) {
		    char   userid[IDLEN + 1];
		    strlcpy(userid, uentp->userid, sizeof(userid));
		    stand_title("�H  �H");
		    prints("[�H�H] ���H�H�G%s", userid);
		    my_send(userid);
		    setutmpmode(LUSERS);
		    redrawall = redraw = 1;
		}
		break;

	    case 'q':
		strlcpy(currauthor, uentp->userid, sizeof(currauthor));
		my_query(uentp->userid);
		setutmpmode(LUSERS);
		redrawall = redraw = 1;
		break;

	    case 'c':
		if (HasUserPerm(PERM_LOGINOK)) {
		    chicken_query(uentp->userid);
		    redrawall = redraw = 1;
		}
		break;

	    case 'l':
		if (HasUserPerm(PERM_LOGINOK)) {
		    t_display();
		    redrawall = redraw = 1;
		}
		break;

	    case 'h':
		t_showhelp();
		redrawall = redraw = 1;
		break;

	    case 'p':
		if (HasUserPerm(PERM_BASIC)) {
		    t_pager();
		    redrawall = redraw = 1;
		}
		break;

	    case Ctrl('W'):
		if (HasUserPerm(PERM_LOGINOK)) {
		    int             tmp;
		    char           *wm[3] = {"�@��", "�i��", "����"};
		    tmp = cuser.uflag2 & WATER_MASK;
		    cuser.uflag2 -= tmp;
		    tmp = (tmp + 1) % 3;
		    cuser.uflag2 |= tmp;
		    /* vmsg cannot support multi lines */
		    move(b_lines - 4, 0);
		    clrtobot();
		    move(b_lines - 3, 0);
		    prints("�t�δ��� �@�� �i�� ���� �T�ؼҦ�\n"
		    "�b������Х��`�U�u�A���s�n�J, �H�T�O���c���T\n");
		    vmsgf( "�ثe������ %s ���y�Ҧ�", wm[tmp]);
		    redrawall = redraw = 1;
		}
		break;

	    case 'r':
		if (HasUserPerm(PERM_LOGINOK)) {
		    if (curredit & EDIT_MAIL) {
			/* deny reentrance, which may cause many problems */
			vmsg("�A�i�J�ϥΪ̦C��e�N�w�g�b�\\Ū�H��F");
		    } else {
			m_read();
			setutmpmode(LUSERS);
		    }
		    redrawall = redraw = 1;
		}
		break;

	    case 'N':
		if (HasUserPerm(PERM_LOGINOK)) {
		    char tmp_nick[sizeof(cuser.nickname)];
		    // XXX why do so many copy here?
		    // why not just use cuser.nickname?
		    // XXX old code forget to initialize.
		    // will changing to init everytime cause user
		    // complain?

		    strlcpy(tmp_nick, currutmp->nickname, sizeof(cuser.nickname));

		    if (oldgetdata(1, 0, "�s���ʺ�: ",
				tmp_nick, sizeof(tmp_nick), DOECHO) > 0)
		    {
			strlcpy(cuser.nickname, tmp_nick, sizeof(cuser.nickname));
			strcpy(currutmp->nickname, cuser.nickname);
		    }
		    redrawall = redraw = 1;
		}
		break;

	    case 'y':
		set_withme_flag();
		redrawall = redraw = 1;
		break;

	    default:
		if (now >= lastupdate + 2)
		    redraw = 1;
	    }
	}
    }
    free(currpickup);
}

int
t_users(void)
{
    int             destuid0 = currutmp->destuid;
    int             mode0 = currutmp->mode;
    int             stat0 = currstat;

    assert(strncmp(cuser.userid, currutmp->userid, IDLEN)==0);
    if( strncmp(cuser.userid , currutmp->userid, IDLEN) != 0 ){
	abort_bbs(0);
    }

    setutmpmode(LUSERS);
    userlist();
    currutmp->mode = mode0;
    currutmp->destuid = destuid0;
    currstat = stat0;
    return 0;
}

int
t_pager(void)
{
    currutmp->pager = (currutmp->pager + 1) % PAGER_MODES;
    return 0;
}

int
t_idle(void)
{
    int             destuid0 = currutmp->destuid;
    int             mode0 = currutmp->mode;
    int             stat0 = currstat;
    char            genbuf[20];
    char            passbuf[PASSLEN];
    int             idle_type;
    char            idle_reason[sizeof(currutmp->chatid)];


    setutmpmode(IDLE);
    getdata(b_lines - 1, 0, "�z�ѡG[0]�o�b (1)���q�� (2)�V�� (3)���O�� "
	    "(4)�˦� (5)ù�� (6)��L (Q)�S�ơH", genbuf, 3, DOECHO);
    if (genbuf[0] == 'q' || genbuf[0] == 'Q') {
	currutmp->mode = mode0;
	currstat = stat0;
	return 0;
    } else if (genbuf[0] >= '1' && genbuf[0] <= '6')
	idle_type = genbuf[0] - '0';
    else
	idle_type = 0;

    if (idle_type == 6) {
	if (cuser.userlevel && getdata(b_lines - 1, 0, "�o�b���z�ѡG", idle_reason, sizeof(idle_reason), DOECHO)) {
	    strlcpy(currutmp->chatid, idle_reason, sizeof(currutmp->chatid));
	} else {
	    idle_type = 0;
	}
    }
    currutmp->destuid = idle_type;
    do {
	move(b_lines - 2, 0);
	clrtoeol();
	prints("(��w�ù�)�o�b��]: %s", (idle_type != 6) ?
		 IdleTypeTable[idle_type] : idle_reason);
	refresh();
	getdata(b_lines - 1, 0, MSG_PASSWD, passbuf, sizeof(passbuf), NOECHO);
	passbuf[8] = '\0';
    }
    while (!checkpasswd(cuser.passwd, passbuf) &&
	   strcmp(STR_GUEST, cuser.userid));

    currutmp->mode = mode0;
    currutmp->destuid = destuid0;
    currstat = stat0;

    return 0;
}

int
t_qchicken(void)
{
    char            uident[STRLEN];

    stand_title("�d���d��");
    usercomplete(msg_uid, uident);
    if (uident[0])
	chicken_query(uident);
    return 0;
}

int
t_query(void)
{
    char            uident[STRLEN];

    stand_title("�d�ߺ���");
    usercomplete(msg_uid, uident);
    if (uident[0])
	my_query(uident);
    return 0;
}

int
t_talk(void)
{
    char            uident[16];
    int             tuid, unum, ucount;
    userinfo_t     *uentp;
    char            genbuf[4];
    /*
     * if (count_ulist() <= 1){ outs("�ثe�u�W�u���z�@�H�A���ܽЪB�ͨӥ��{�i"
     * BBSNAME "�j�a�I"); return XEASY; }
     */
    stand_title("���}�ܧX�l");
    CompleteOnlineUser(msg_uid, uident);
    if (uident[0] == '\0')
	return 0;

    move(3, 0);
    if (!(tuid = searchuser(uident, uident)) || tuid == usernum) {
	outs(err_uid);
	pressanykey();
	return 0;
    }
    /* multi-login check */
    unum = 1;
    while ((ucount = count_logins(tuid, 0)) > 1) {
	outs("(0) ���Q talk �F...\n");
	count_logins(tuid, 1);
	getdata(1, 33, "�п�ܤ@�Ӳ�ѹ�H [0]�G", genbuf, 4, DOECHO);
	unum = atoi(genbuf);
	if (unum == 0)
	    return 0;
	move(3, 0);
	clrtobot();
	if (unum > 0 && unum <= ucount)
	    break;
    }

    if ((uentp = (userinfo_t *) search_ulistn(tuid, unum)))
	my_talk(uentp, friend_stat(currutmp, uentp), 0);

    return 0;
}

int
reply_connection_request(const userinfo_t *uip)
{
    char            buf[4], genbuf[200];

    if (uip->mode != PAGE) {
	snprintf(genbuf, sizeof(genbuf),
		 "%s�w����I�s�A��Enter�~��...", page_requestor);
	getdata(0, 0, genbuf, buf, sizeof(buf), LCECHO);
	return -1;
    }
    return establish_talk_connection(uip);
}

int
establish_talk_connection(const userinfo_t *uip)
{
    int                    a;
    struct sockaddr_in sin;

    currutmp->msgcount = 0;
    strlcpy(save_page_requestor, page_requestor, sizeof(save_page_requestor));
    memset(page_requestor, 0, sizeof(page_requestor));
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = PF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sin.sin_port = uip->sockaddr;
    if ((a = socket(sin.sin_family, SOCK_STREAM, 0)) < 0) {
	perror("socket err");
	return -1;
    }
    if ((connect(a, (struct sockaddr *) & sin, sizeof(sin)))) {
	//perror("connect err");
	return -1;
    }
    return a;
}

/* ���H�Ӧ���l�F�A�^���I�s�� */
void
talkreply(void)
{
    char            buf[4];
    char            genbuf[200];
    int             a, sig = currutmp->sig;
    int             currstat0 = currstat;
    int             r;
    int             is_chess;
    userec_t        xuser;
    void          (*sig_pipe_handle)(int);

    uip = &SHM->uinfo[currutmp->destuip];
    currutmp->destuid = uip->uid;
    currstat = REPLY;		/* �קK�X�{�ʵe */

    is_chess = (sig == SIG_CHC || sig == SIG_GOMO || sig == SIG_GO || sig == SIG_REVERSI);

    a = reply_connection_request(uip);
    if (a < 0) {
	clear();
	currstat = currstat0;
	return;
    }
    if (is_chess)
	ChessAcceptingRequest(a);

    clear();

    outs("\n\n");
    // FIXME CRASH here
    assert(sig>=0 && sig<sizeof(sig_des)/sizeof(sig_des[0]));
    prints("       (Y) ���ڭ� %s �a�I"
	    "     (A) �ڲ{�b�ܦ��A�е��@�|��A call ��\n", sig_des[sig]);
    prints("       (N) �ڲ{�b���Q %s"
	    "      (B) �藍�_�A�ڦ��Ʊ������A %s\n",
	    sig_des[sig], sig_des[sig]);
    prints("       (C) �Ф��n�n�ڦn�ܡH"
	    "       (D) �ڭn�����o..�U���A��a.......\n");
    prints("       (E) ���ƶܡH�Х��ӫH"
	    "       (F) " ANSI_COLOR(1;33) "�ڦۤv��J�z�Ѧn�F..." ANSI_RESET "\n");
    prints("       (1) %s�H����100�Ȩ��"
	    "  (2) %s�H����1000�Ȩ��..\n\n", sig_des[sig], sig_des[sig]);

    snprintf(page_requestor, sizeof(page_requestor),
	    "%s (%s)", uip->userid, uip->nickname);
    getuser(uip->userid, &xuser);
    currutmp->msgs[0].pid = uip->pid;
    strlcpy(currutmp->msgs[0].userid, uip->userid, sizeof(currutmp->msgs[0].userid));
    strlcpy(currutmp->msgs[0].last_call_in, "�I�s�B�I�s�Ať��Ц^�� (Ctrl-R)",
	    sizeof(currutmp->msgs[0].last_call_in));
    currutmp->msgs[0].msgmode = MSGMODE_TALK;
    prints("���Ӧ� [%s]�A�@�W�� %d ���A�峹 %d �g\n",
	    uip->from, xuser.numlogins, xuser.numposts);

    if (is_chess)
	ChessShowRequest();
    else {
	showplans(uip->userid);
	show_call_in(0, 0);
    }

    snprintf(genbuf, sizeof(genbuf),
	    "�A�Q�� %s %s�ڡH�п��(Y/N/A/B/C/D/E/F/1/2)[N] ",
	    page_requestor, sig_des[sig]);
    getdata(0, 0, genbuf, buf, sizeof(buf), LCECHO);

    if (!buf[0] || !strchr("yabcdef12", buf[0]))
	buf[0] = 'n';

    sig_pipe_handle = Signal(SIGPIPE, SIG_IGN);
    r = write(a, buf, 1);
    if (buf[0] == 'f' || buf[0] == 'F') {
	if (!getdata(b_lines, 0, "���઺��]�G", genbuf, 60, DOECHO))
	    strlcpy(genbuf, "���i�D�A�� !! ^o^", sizeof(genbuf));
	r = write(a, genbuf, 60);
    }
    Signal(SIGPIPE, sig_pipe_handle);

    if (r == -1) {
	snprintf(genbuf, sizeof(genbuf),
		 "%s�w����I�s�A��Enter�~��...", page_requestor);
	getdata(0, 0, genbuf, buf, sizeof(buf), LCECHO);
	clear();
	currstat = currstat0;
	return;
    }

    uip->destuip = currutmp - &SHM->uinfo[0];
    if (buf[0] == 'y')
	switch (sig) {
	case SIG_DARK:
	    main_dark(a, uip);
	    break;
	case SIG_PK:
	    chickenpk(a);
	    break;
	case SIG_GOMO:
	    gomoku(a, CHESS_MODE_VERSUS);
	    break;
	case SIG_CHC:
	    chc(a, CHESS_MODE_VERSUS);
	    break;
	case SIG_GO:
	    gochess(a, CHESS_MODE_VERSUS);
	    break;
	case SIG_REVERSI:
	    reversi(a, CHESS_MODE_VERSUS);
	    break;
	case SIG_TALK:
	default:
	    do_talk(a);
	}
    else
	close(a);
    clear();
    currstat = currstat0;
}

#ifdef PLAY_ANGEL
/* �p�ѨϤp�D�H�B�z�禡 */
int
t_changeangel(){
    char buf[4];

    /* cuser.myangel == "-" means banned for calling angel */
    if (cuser.myangel[0] == '-' || cuser.myangel[1] == 0) return 0;

    getdata(b_lines - 1, 0,
	    "�󴫤p�Ѩϫ�N�L�k���^�F��I �O�_�n�󴫤p�ѨϡH [y/N]",
	    buf, 3, LCECHO);
    if (buf[0] == 'y' || buf[0] == 'Y') {
	char buf[100];
	snprintf(buf, sizeof(buf), "%s�p�D�H %s ���� %s �p�Ѩ�\n",
		ctime4(&now), cuser.userid, cuser.myangel);
	buf[24] = ' '; // replace '\n'
	log_file(BBSHOME "/log/changeangel.log", LOG_CREAT, buf);

	cuser.myangel[0] = 0;
	outs("�p�Ѩϧ�s�����A�U���I�s�ɷ|��X�s���p�Ѩ�");
    }
    return XEASY;
}

int t_angelmsg(){
    char msg[3][74] = { "", "", "" };
    char nick[10] = "";
    char buf[512];
    int i;
    FILE* fp;

    setuserfile(buf, "angelmsg");
    fp = fopen(buf, "r");
    if (fp) {
	i = 0;
	if (fgets(msg[0], sizeof(msg[0]), fp)) {
	    chomp(msg[0]);
	    if (strncmp(msg[0], "%%[", 3) == 0) {
		strlcpy(nick, msg[0] + 3, 7);
		move(4, 0);
		prints("�즳�ʺ١G%s", nick);
		msg[0][0] = 0;
	    } else
		i = 1;
	} else
	    msg[0][0] = 0;

	move(5, 0);
	outs("�즳�d���G\n");
	if(msg[0][0])
	    outs(msg[0]);
	for (; i < 3; ++i) {
	    if(fgets(msg[i], sizeof(msg[0]), fp)) {
		outs(msg[i]);
		chomp(msg[i]);
	    } else
		break;
	}
	fclose(fp);
    }

    getdata_buf(11, 0, "�ʺ١G", nick, 7, 1);
    do {
	move(12, 0);
	clrtobot();
	outs("���b���ɭԭn��p�D�H������O�H"
	     "�̦h�T��A��[Enter]����");
	for (i = 0; i < 3 &&
		getdata_buf(14 + i, 0, "�G", msg[i], sizeof(msg[i]), DOECHO);
		++i);
	getdata(b_lines - 2, 0, "(S)�x�s (E)���s�ӹL (Q)�����H[S]",
		buf, 4, LCECHO);
    } while (buf[0] == 'E' || buf[0] == 'e');
    if (buf[0] == 'Q' || buf[0] == 'q')
	return 0;
    setuserfile(buf, "angelmsg");
    if (msg[0][0] == 0)
	unlink(buf);
    else {
	FILE* fp = fopen(buf, "w");
	if(nick[0])
	    fprintf(fp, "%%%%[%s\n", nick);
	for (i = 0; i < 3 && msg[i][0]; ++i) {
	    fputs(msg[i], fp);
	    fputc('\n', fp);
	}
	fclose(fp);
    }
    return 0;
}

static int
FindAngel(void){
    int nAngel;
    int i, j;
    int choose;
    int trial = 0;
    int mask;

    if (cuser.sex < 6) /* ���`�ʧO */
	mask = 1 | (2 << (cuser.sex & 1));
    else
	mask = 7;

    do{
	nAngel = 0;
	j = SHM->currsorted;
	for (i = 0; i < SHM->UTMPnumber; ++i)
	    if ((SHM->uinfo[SHM->sorted[j][0][i]].userlevel & PERM_ANGEL)
		    && (SHM->uinfo[SHM->sorted[j][0][i]].angel & mask) == 0)
		++nAngel;

	if (nAngel == 0)
	    return 0;

	choose = random() % nAngel + 1;
	j = SHM->currsorted;
	for (i = 0; i < SHM->UTMPnumber && choose; ++i)
	    if ((SHM->uinfo[SHM->sorted[j][0][i]].userlevel & PERM_ANGEL)
		    && (SHM->uinfo[SHM->sorted[j][0][i]].angel & mask) == 0)
		--choose;

	if (choose == 0 && SHM->uinfo[SHM->sorted[j][0][i - 1]].uid != currutmp->uid
		&& (SHM->uinfo[SHM->sorted[j][0][i - 1]].userlevel & PERM_ANGEL)
		&& ((SHM->uinfo[SHM->sorted[j][0][i - 1]].angel & mask) == 0)
		&& !he_reject_me(&SHM->uinfo[SHM->sorted[j][0][i - 1]]) ){
	    strlcpy(cuser.myangel, SHM->uinfo[SHM->sorted[j][0][i - 1]].userid, IDLEN + 1);
	    passwd_update(usernum, &cuser);
	    return 1;
	}
    }while(++trial < 5);
    return 0;
}

static inline void
GotoNewHand(){
    if (currutmp && currutmp->mode != EDITING){
	char old_board[IDLEN + 1] = "";
	if (currboard[0])
	    strlcpy(old_board, currboard, IDLEN + 1);

	if (enter_board(GLOBAL_NEWBIE)==0) {
	    Read();
	}

	if (old_board[0])
	    enter_board(old_board);
    }
}

static inline void
NoAngelFound(const char* msg){
    move(b_lines, 0);
    outs(msg);
    if (currutmp == NULL || currutmp->mode != EDITING)
	outs("�A�Х��b�s��O�W�M�䵪�שΫ� Ctrl-P �o��");
    clrtoeol();
    refresh();
    sleep(1);
    GotoNewHand();
    return;
}

static inline void
AngelNotOnline(){
    char buf[256];
    const static char* const not_online_message = "�z���p�Ѩϲ{�b���b�u�W";
    if (cuser.myangel[0] != '-')
	sethomefile(buf, cuser.myangel, "angelmsg");
    if (cuser.myangel[0] == '-' || !dashf(buf))
	NoAngelFound(not_online_message);
    else {
	FILE* fp = fopen(buf, "r");
	clear();
	showtitle("�p�Ѩϯd��", BBSNAME);
	move(4, 0);
	clrtobot();

	buf[0] = 0;
	fgets(buf, sizeof(buf), fp);
	if (strncmp(buf, "%%[", 3) == 0) {
	    chomp(buf);
	    prints("�z��%s�p�Ѩϲ{�b���b�u�W", buf + 3);
	    fgets(buf, sizeof(buf), fp);
	} else
	    outs(not_online_message);

	outs("\n͢�d�����A�G\n");
	outs(ANSI_COLOR(1;31;44) "��s�w�w�w�w�w�w�w�w�w�w�w�w�w�w�t" ANSI_COLOR(37) ""
	     "�p�Ѩϯd��" ANSI_COLOR(31) "�u�w�w�w�w�w�w�w�w�w�w�w�w�w�w�s��" ANSI_RESET "\n");
	outs(ANSI_COLOR(1;31) "�~�t" ANSI_COLOR(32) " �p�Ѩ�                          "
	     "                                     " ANSI_COLOR(31) "�u��" ANSI_RESET "\n");

	do {
	    chomp(buf);
	    prints(ANSI_COLOR(1;31) "�x" ANSI_RESET "%-74.74s" ANSI_COLOR(1;31) "�x" ANSI_RESET "\n", buf);
	} while (fgets(buf, sizeof(buf), fp));

	outs(ANSI_COLOR(1;31) "���s�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w"
		"�w�w�w�w�w�w�w�w�w�w�w�w�w�s��" ANSI_RESET "\n");
	outs(ANSI_COLOR(1;31;44) "��r�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w"
		"�w�w�w�w�w�w�w�w�w�w�w�w�w�w�r��" ANSI_RESET "\n");

	move(b_lines - 4, 0);
	outs("�p�D�H�ϥΤW���D�䤣��p�ѨϽШ�s�⪩(" GLOBAL_NEWBIE ")\n"
	     "              �Q�d�����p�ѨϽШ�\\�@��(AngelPray)\n"
	     "                  �Q��ݪO�b�����ܥi��(AskBoard)\n"
	     "�Х��b�U�O�W�M�䵪�שΫ� Ctrl-P �o��");
	pressanykey();

	GotoNewHand();
    }
}

static void
TalkToAngel(){
    static int AngelPermChecked = 0;
    userinfo_t* uent;
    userec_t xuser;

    if (strcmp(cuser.myangel, "-") == 0){
	AngelNotOnline();
	return;
    }

    if (cuser.myangel[0] && !AngelPermChecked) {
	getuser(cuser.myangel, &xuser); // XXX if user doesn't exist
	if (!(xuser.userlevel & PERM_ANGEL))
	    cuser.myangel[0] = 0;
    }
    AngelPermChecked = 1;

    if (cuser.myangel[0] == 0 && ! FindAngel()){
	NoAngelFound("�{�b�S���p�ѨϦb�u�W");
	return;
    }

    uent = search_ulist_userid(cuser.myangel);
    if (uent == 0 || (uent->angel & 1) || he_reject_me(uent)){
	AngelNotOnline();
	return;
    }

    more("etc/angel_usage", NA);

    /* �o�q�ܩγ\�i�H�b�p�ѨϦ^�����D�� show �X��
    move(b_lines - 1, 0);
    outs("�{�b�A��id����O�K�A�^���A���D���p�ѨϨä����D�A�O��       \n"
         "�A�i�H��ܤ��V���z�S�ۤv�����ӫO�@�ۤv                   ");
	 */

    my_write(uent->pid, "�ݤp�ѨϡG ", "�p�Ѩ�", WATERBALL_ANGEL, uent);
    return;
}

void
CallAngel(){
    static int      entered = 0;
    screen_backup_t old_screen;

    if (!HasUserPerm(PERM_LOGINOK) || entered)
	return;
    entered = 1;

    screen_backup(&old_screen);

    TalkToAngel();

    screen_restore(&old_screen);

    entered = 0;
}

void
SwitchBeingAngel(){
    cuser.uflag2 ^= REJ_QUESTION;
    currutmp->angel ^= 1;
}

void
SwitchAngelSex(int newmode){
    ANGEL_SET(newmode);
    currutmp->angel = (currutmp->angel & ~0x6) | ((newmode & 3) << 1);
}

int
t_switchangel(){
    SwitchBeingAngel();
    outs(REJECT_QUESTION ? "�𮧤@�|��" : "�}��p�D�H�ݰ��D");
    return XEASY;
}
#endif
