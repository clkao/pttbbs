/* $Id: talk.c,v 1.62 2002/06/04 18:31:59 ptt Exp $ */
#include "bbs.h"

#define QCAST   int (*)(const void *, const void *)

static char *IdleTypeTable[] = {
    "���b��b��", "���H�ӹq", "�V����", "�����P��", "�������A", "�ڦb���"
};
static char *sig_des[] = {
    "����", "���", "", "�U��", "�H��", "�t��"
};

#define MAX_SHOW_MODE 3
#define M_INT 15		/* monitor mode update interval */
#define P_INT 20		/* interval to check for page req. in
				 * talk/chat */
#define BOARDFRI  1

typedef struct talkwin_t {
    int curcol, curln;
    int sline, eline;
} talkwin_t;

typedef struct pickup_t {
    userinfo_t *ui;
    int friend, uoffset;
} pickup_t;

//extern int bind( /* int,struct sockaddr *, int */ );
//extern char *getuserid();

/* �O�� friend �� user number */
//#define PICKUP_WAYS     7		//�����k�h�u��
#define PICKUP_WAYS     6

static char *fcolor[11] = {
    "", "\033[36m", "\033[32m", "\033[1;32m",
    "\033[33m", "\033[1;33m", "\033[1;37m", "\033[1;37m",
    "\033[31m", "\033[1;35m", "\033[1;36m"
};
static char save_page_requestor[40];
static char page_requestor[40];
static char description[30];
static FILE *flog;

int iswritable_stat(userinfo_t *uentp, int fri_stat)
{
    if( uentp == currutmp )
	return 0;

    if( HAS_PERM(PERM_SYSOP) )
	return 1;

    if( !HAS_PERM(PERM_LOGINOK) )
	return 0;

    return (uentp->pager != 3 && (fri_stat & HFM || uentp->pager != 4));
}

int isvisible_stat(userinfo_t * me, userinfo_t * uentp, int fri_stat)
{
    if (uentp->userid[0] == 0)
	return 0;
    
    if (PERM_HIDE(uentp) && !(PERM_HIDE(me)))/* ��赵�����ΦӧA�S�� */
	return 0;
    else if ((me->userlevel & PERM_SYSOP) ||
             ((fri_stat & HRM) && (fri_stat & HFM)))
	                          /* �����ݪ�������H */
	return 1;
    
    if (uentp->invisible && !(me->userlevel & PERM_SEECLOAK)) return 0;

    return !(fri_stat & HRM);
}

char *modestring(userinfo_t * uentp, int simple)
{
    static char modestr[40];
    static char *notonline = "���b���W";
    register int mode = uentp->mode;
    register char *word;
    int fri_stat;

/* for debugging */
    if (mode >= MAX_MODES){
	syslog(LOG_WARNING, "what!? mode = %d", mode);
	word = ModeTypeTable[mode % MAX_MODES];
    }
    else
	word = ModeTypeTable[mode];
    fri_stat = friend_stat(currutmp, uentp);
    if( !(HAS_PERM(PERM_SYSOP) || HAS_PERM(PERM_SEECLOAK)) &&
	((uentp->invisible || (fri_stat & HRM)) &&
	 !((fri_stat & HFM) && (fri_stat & HRM)))             )
	return notonline;
    else if (mode == EDITING){
	sprintf(modestr, "E:%s",
		ModeTypeTable[uentp->destuid < EDITING ? uentp->destuid :
			      EDITING]);
	word = modestr;
    }
    else if (!mode && *uentp->chatid == 1){
	if (!simple)
	    sprintf(modestr, "�^�� %s", getuserid(uentp->destuid));
	else
	    sprintf(modestr, "�^���I�s");
    }
    else if (!mode && *uentp->chatid == 2)
	if (uentp->msgcount < 10){
	    char *cnum[10] =
		{"", "�@", "��", "�T", "�|", "��", "��", "�C",
		 "�K", "�E"};
	    sprintf(modestr, "��%s�����y", cnum[uentp->msgcount]);
	}
	else
	    sprintf(modestr, "����F @_@");
    else if (!mode && *uentp->chatid == 3)
	sprintf(modestr, "���y�ǳƤ�");
    else if (!mode)
	return (uentp->destuid == 6) ? uentp->chatid :
	    IdleTypeTable[(0 <= uentp->destuid && uentp->destuid < 6) ?
			  uentp->destuid : 0];
    else if (simple)
	return word;
    else if (uentp->in_chat && mode == CHATING)
	sprintf(modestr, "%s (%s)", word, uentp->chatid);
    else if (mode == TALK){
	if (!isvisible_uid(uentp->destuid))/* Leeym ���(����)���� */
	    sprintf(modestr, "%s", "��� �Ů�");/* Leeym �j�a�ۤv�o���a�I */
	else
	    sprintf(modestr, "%s %s", word, getuserid(uentp->destuid));
    }
    else if (mode == M_FIVE){
	if (!isvisible_uid(uentp->destuid))
	    sprintf(modestr, "%s", "���l�� �Ů�");
	else
	    sprintf(modestr, "%s %s", word, getuserid(uentp->destuid));
    }
    else if (mode == CHC){
	if (isvisible_uid(uentp->destuid))
	    sprintf(modestr, "%s", "�U�H��");
	else
	    sprintf(modestr, "�U�H�� %s", getuserid(uentp->destuid));
    }
    else if (mode != PAGE && mode != TQUERY)
	return word;
    else
	sprintf(modestr, "%s %s", word, getuserid(uentp->destuid));

    return (modestr);
}

int set_friend_bit(userinfo_t * me, userinfo_t * ui) {
    int unum, *myfriends, hit=0, n;

    /* �P�_���O�_���ڪ��B�� ? */
    unum = ui->uid;
    myfriends = me->friend;
    while ((n = *myfriends++)){
	if (unum == n){
	    hit = IFH;
	    break;
	}
    }
    
    /* �P�_�ڬO�_����誺�B�� ? */
    myfriends = ui->friend;
    while ((unum = *myfriends++)){
	if (unum == me->uid){
	    hit |= HFM;
	    break;
	}
    }
    
/* �P�_���O�_���ڪ����H ? */

    unum = ui->uid;
    myfriends = me->reject;
    while ((n = *myfriends++)){
	if (unum == n){
	    hit |= IRH;
	    break;
	}
    }

/* �P�_�ڬO�_����誺���H ? */
    myfriends = ui->reject;
    while ((unum = *myfriends++)){
	if (unum == me->uid){
	    hit |= HRM;
	    break;
	}
    }
    return hit;
}

int reverse_friend_stat(int stat)
{
     int stat1=0;
     if(stat & IFH)
          stat1 |=HFM;
     if(stat & IRH)
          stat1 |=HRM;
     if(stat & HFM)
          stat1 |=IFH;
     if(stat & HRM)
          stat1 |=IRH;   
     if(stat & IBH)
          stat1 |=IBH;   
     return stat1; 
}

int login_friend_online(void)
{
    userinfo_t *uentp;
    int i, stat, stat1;
    int offset=(int) (currutmp - &utmpshm->uinfo[0]);
    for (i=0;i<utmpshm->number && currutmp->friendtotal<MAX_FRIEND; i++){
	uentp = (utmpshm->sorted[utmpshm->currsorted][0][i]);
	if(uentp && uentp->uid && (stat=set_friend_bit(currutmp,uentp))){
	    stat1=reverse_friend_stat(stat);
	    stat <<= 24;
	    stat |= (int) (uentp - &utmpshm->uinfo[0]);
	    currutmp->friend_online[currutmp->friendtotal++]=stat; 
	    if(uentp!=currutmp && uentp->friendtotal<MAX_FRIEND){
		stat1 <<= 24;
		stat1 |= offset;
		uentp->friend_online[uentp->friendtotal++]=stat1;
	    }
	}
    } 
    return 0;
}

int logout_friend_online(userinfo_t *utmp)
{
    int i, j, k;
    int offset=(int) (utmp - &utmpshm->uinfo[0]);
    userinfo_t *ui;
    while( utmp->friendtotal > 0 ){
	i = utmp->friendtotal-1;
	j = (utmp->friend_online[i] & 0xFFFFFF);
	utmp->friend_online[i]=0;
	ui = &utmpshm->uinfo[j]; 
	if(ui->pid && ui!=utmp){
            for(k=0; k<ui->friendtotal > 0 && k < MAX_FRIEND &&
		    (int)(ui->friend_online[k] & 0xFFFFFF) !=offset; k++);
            if(k<ui->friendtotal){
		ui->friendtotal--;
		ui->friend_online[k]=ui->friend_online[ui->friendtotal];
		ui->friend_online[ui->friendtotal]=0;
	    }
	}
	utmp->friendtotal--;
	utmp->friend_online[utmp->friendtotal]=0;
    }
    return 0;
}


int friend_stat(userinfo_t *me, userinfo_t * ui)
{
  int i, j, hit=0;
  /* �ݪO�n�� */
  if (me->brc_id && ui->brc_id == me->brc_id){
      hit = IBH;
  }
  for(i=0;me->friend_online[i] && i < MAX_FRIEND;i++){
      j = (me->friend_online[i] & 0xFFFFFF);
      if( 0 <= j && j < MAX_ACTIVE && ui == &utmpshm->uinfo[j] ){
	  hit |= me->friend_online[i] >>24;
	  break;
      }
  }
  if (PERM_HIDE(ui))
      return hit & ST_FRIEND;   
  return hit;
}

int isvisible_uid(int tuid)
{
    userinfo_t *uentp;

    if(!tuid || !(uentp = search_ulist(tuid)))
       return 1;
    return isvisible(currutmp, uentp);
}

/* �u��ʧ@ */
static void my_kick(userinfo_t * uentp)
{
    char genbuf[200];

    getdata(1, 0, msg_sure_ny, genbuf, 4, LCECHO);
    clrtoeol();
    if (genbuf[0] == 'y'){
	sprintf(genbuf, "%s (%s)", uentp->userid, uentp->username);
	log_usies("KICK ", genbuf);
	if((uentp->pid <= 0 || kill(uentp->pid, SIGHUP) == -1) && (errno == ESRCH))
	    purge_utmp(uentp);
	outs("��X�h�o");
    }
    else
	outs(msg_cancel);
    pressanykey();
}

static void chicken_query(char *userid)
{
    char buf[100];

    if (getuser(userid)){
	if (xuser.mychicken.name[0]){
	    time_diff(&(xuser.mychicken));
	    if (!isdeadth(&(xuser.mychicken))){
		show_chicken_data(&(xuser.mychicken), NULL);
		sprintf(buf, "\n\n�H�W�O %s ���d�����..", userid);
		outs(buf);
	    }
	}
	else{
	    move(1, 0);
	    clrtobot();
	    sprintf(buf, "\n\n%s �èS���i�d��..", userid);
	    outs(buf);
	}
	pressanykey();
    }
}

int my_query(char *uident)
{
    userec_t muser;
    int tuid, i, fri_stat=0;
    unsigned long int j;
    userinfo_t *uentp;
    static const char *money[10] =
	{"�ťx���v", "���h", "�M�H", "���q", "�p�d",
	 "�p�I", "���I", "�j�I��", "�I�i�İ�", "�񺸻\\��"};
    static const char *sex[8] =
	{MSG_BIG_BOY, MSG_BIG_GIRL,
	 MSG_LITTLE_BOY, MSG_LITTLE_GIRL,
	 MSG_MAN, MSG_WOMAN, MSG_PLANT, MSG_MIME};
    
    if ((tuid = getuser(uident))){
	memcpy(&muser, &xuser, sizeof(muser));
	move(1, 0);
	clrtobot();
	move(1, 0);
	setutmpmode(TQUERY);
	currutmp->destuid = tuid;
	
	j = muser.money;
	for (i = 0; i < 10 && j > 10; i++)
	    j /= 10;
	prints("�m�עҼʺ١n%s(%s)%*s�m�g�٪��p�n%s\n",
	       muser.userid,
	       muser.username,
	       26 - strlen(muser.userid) - strlen(muser.username), "",
	       money[i]);
	prints("�m�W�����ơn%d��", muser.numlogins);
	move(2, 40);
	prints("�m�峹�g�ơn%d�g\n", muser.numposts);
	
	if((uentp = (userinfo_t *) search_ulist(tuid)))
	    fri_stat=friend_stat(currutmp, uentp); 
	prints("\033[1;33m�m�ثe�ʺA�n%-28.28s\033[m",
	       (uentp && isvisible_stat(currutmp, uentp, fri_stat)) ?
	       modestring(uentp, 0) : "���b���W");
	
	outs(((uentp && uentp->mailalert) || load_mailalert(muser.userid))
	     ? "�m�p�H�H�c�n���s�i�H���٨S��\n" :
	     "�m�p�H�H�c�n�Ҧ��H�󳣬ݹL�F\n");
	prints("�m�W���W���n%-28.28s�m�W���G�m�n%s\n",
	       Cdate(&muser.lastlogin),
	       (muser.lasthost[0] ? muser.lasthost : "(����)"));
        if ((uentp && fri_stat&HFM && !uentp->invisible) || HAS_PERM(PERM_SYSOP))
	    prints("�m ��  �O �n%-28.28s�m�p���]���n%ld �Ȩ�\n",
		   sex[muser.sex % 8],
		   muser.money);
	prints("�m���l�Ѿ��Z�n%3d �� %3d �� %3d �M      "
	       "�m�H�Ѿ��Z�n%3d �� %3d �� %3d �M",
	       muser.five_win, muser.five_lose, muser.five_tie,
	       muser.chc_win, muser.chc_lose, muser.chc_tie);
	showplans(uident);
	pressanykey();
	return FULLUPDATE;
    }
    return DONOTHING;
}

static char t_last_write[200] = "";

void water_scr(water_t *tw, int which, char type)
{
    if( type == 1 ){
	int    i;
	int    colors[] = {33, 37, 33, 37, 33};
	move(8 + which, 28);prints(" ");
	move(8 + which, 28);
	prints("\033[1;37;45m  %c %-14s \033[0m",
	       tw->uin ? ' ' : 'x',
	       tw->userid);
	for( i = 0 ; i < 5 ; ++i ){
	    move(16 + i, 4);
	    prints(" ");
	    move(16 + i, 4);
	    if( tw->msg[ (tw->top - i + 4) % 5 ].last_call_in[0] != 0 )
		prints("\033[0m   \033[1;%d;44m��%-64s\033[0m   \n",
		       colors[i],
		       tw->msg[ (tw->top - i + 4) % 5 ].last_call_in);
	    else
		prints("\033[0m�@\n");
	}

	move(21, 4);prints(" ");
	move(21, 4);
	prints("\033[0m   \033[1;37;46m%-66s\033[0m   \n",
	       tw->msg[5].last_call_in);

	move(0, 0);prints(" ");
	move(0, 0);prints("\033[0m���� %s:", tw->userid);
	clrtoeol();
	move(0, strlen(tw->userid) + 6);
    }
    else{
	move(8 + which, 28);
	prints("123456789012345678901234567890");
	move(8 + which, 28);
	prints("\033[1;37;44m  %c %-13s�@\033[0m",
	       tw->uin ? ' ' : 'x',
	       tw->userid);
    }
}

void my_write2(void)
{
    int     i, ch, currstat0;
    char    genbuf[256], msg[80], done = 0, c0, which;
    water_t *tw;
    unsigned        char    mode0;

    wmofo = 0;
    currstat0 = currstat;
    c0 = currutmp->chatid[0];
    mode0 = currutmp->mode;
    currutmp->mode = 0;
    currutmp->chatid[0] = 3;
    currstat = XMODE;

    // init screen
    move(7, 28);
    prints("\033[1;33;46m �� ���y������H ��\033[0m");
    for( i = 0 ; i < 5 ; ++i )
	if( swater[i] == NULL || swater[i]->pid == 0 )
	    break;
	else{
	    if( swater[i]->uin &&
		(swater[i]->pid != swater[i]->uin->pid ||
		 swater[i]->userid[0]!=swater[i]->uin->userid[0]) )
		swater[i]->uin = (userinfo_t*)search_ulist_pid(swater[i]->pid);
	    water_scr(swater[i], i, 0);
	}
    move(15, 4);
    prints("\033[0m \033[1;35m��\033[1;36m�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w"
	   "�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w\033[1;35m��\033[0m ");
    move(22, 4);
    prints(" \033[1;35m��\033[1;36m�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w"
	   "�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w\033[1;35m��\033[0m ");
    water_scr(swater[0], 0, 1);
    refresh();

    which = 0;
    do{
	switch( (ch = igetkey()) ){
	case Ctrl('T'):
	case KEY_UP:
	    if( water_usies != 1 ){
		water_scr(swater[(int)which], which, 0);
		which = (which - 1 + water_usies) % water_usies;
		water_scr(swater[(int)which], which, 1);
		refresh();
	    }
	    break;

	case KEY_DOWN:
	case Ctrl('R'):
	    if( water_usies != 1 ){
		water_scr(swater[(int)which], which, 0);
		which = (which + 1 + water_usies) % water_usies;
		water_scr(swater[(int)which], which, 1);
		refresh();
	    }
	    break;

	case KEY_LEFT:
	    done = 1;
	    break;

	default:
	    done = 1;
	    tw = swater[(int)which];

	    if( !tw->uin )
		break;

	    if( ch != '\r' && ch != '\n' ){
		msg[0] = ch, msg[1] = 0;
	    }
	    else
		msg[0] = 0;
	    move(0, 0);prints("\033[m"); clrtoeol();
	    sprintf(genbuf, "���� %s:", tw->userid);
	    if( !oldgetdata(0, 0, genbuf, msg,
			    80-strlen(tw->userid)-6, DOECHO) )
		break;

	    if( my_write(tw->pid, msg, tw->userid, 4, tw->uin) )
		strncpy(tw->msg[5].last_call_in, t_last_write,
			sizeof(tw->msg[5].last_call_in));
	    break;
	}
    } while( !done );

    currstat = currstat0;
    currutmp->chatid[0] = c0;
    currutmp->mode = mode0;
    if( wmofo == 1 )
	write_request(0);
    wmofo = -1;
}

/* 
   �Q�I�s���ɾ�:
   1. ��s�դ��y flag = 1 (pre-edit)
   2. �^���y     flag = 0
   3. �W��aloha  flag = 2 (pre-edit)
   4. �s��       flag = 3 if SYSOP, otherwise flag = 1 (pre-edit)
   5. ����y     flag = 0
   6. my_write2  flag = 4 (pre-edit) but confirm
*/
int my_write(pid_t pid, char *prompt, char *id, int flag, userinfo_t *puin)
{
    int len, currstat0 = currstat, fri_stat;
    char msg[80], destid[IDLEN + 1];
    char genbuf[200], buf[200], c0 = currutmp->chatid[0];
    unsigned char mode0 = currutmp->mode;
    struct tm *ptime;
    userinfo_t *uin;
    uin = (puin != NULL) ? puin : (userinfo_t *)search_ulist_pid(pid);
    strcpy(destid, id);
    
    if(!uin && !(flag == 0 && water_which->count> 0)) {
      outmsg("\033[1;33;41m�V�|! ���w���]�F(���b���W)! \033[37m~>_<~\033[m");
	clrtoeol();
	refresh();
	watermode = -1;
	return 0;
    }
    currutmp->mode = 0;
    currutmp->chatid[0] = 3;
    currstat = XMODE;
    
    ptime = localtime(&now);
    
    if(flag == 0) {
	/* �@����y */
	watermode = 0;
	if(!(len = getdata(0, 0, prompt, msg, 56, DOECHO))) {
	    outmsg("\033[1;33;42m��F! ��A�@��...\033[m");
	    clrtoeol();
	    refresh();
	    currutmp->chatid[0] = c0;
	    currutmp->mode = mode0;
	    currstat = currstat0;
	    watermode = -1;
	    return 0;
	}
	
	if(watermode > 0) {
	    int i;
	    
	    i = (water_which->top- watermode + MAX_REVIEW) % MAX_REVIEW;
	    uin = (userinfo_t *)search_ulist_pid(water_which->msg[i].pid);
	    strcpy(destid, water_which->msg[i].userid);
	}
    } else {
	/* pre-edit �����y */
	strcpy(msg, prompt);
	len = strlen(msg);
    }
    
    strip_ansi(msg, msg, 0);
    if(uin && *uin->userid && (flag == 0 || flag == 4)) {
	sprintf(buf, "�ᵹ %s : %s [Y/n]?", uin->userid, msg);
	getdata(0, 0, buf, genbuf, 3, LCECHO);
	if(genbuf[0] == 'n') {
	    outmsg("\033[1;33;42m��F! ��A�@��...\033[m");
	    clrtoeol();
	    refresh();
	    currutmp->chatid[0] = c0;
	    currutmp->mode = mode0;
	    currstat = currstat0;
	    watermode = -1;
	    return 0;
	}
    }
    
    watermode = -1;
    if(!uin || !*uin->userid || strcasecmp(destid, uin->userid)) {
	outmsg("\033[1;33;41m�V�|! ���w���]�F(���b���W)! \033[37m~>_<~\033[m");
	clrtoeol();
	refresh();
	currutmp->chatid[0] = c0;
	currutmp->mode = mode0;
	currstat = currstat0;
	return 0;
    }
    
    fri_stat=friend_stat(currutmp, uin); 
    if(flag != 2) { /* aloha �����y���Φs�U�� */
	/* �s��ۤv�����y�� */
	if(!fp_writelog){
	    sethomefile(genbuf, cuser.userid, fn_writelog);
	    fp_writelog = fopen(genbuf, "a");
	}
	if(fp_writelog) {
	    fprintf(fp_writelog, "To %s: %s [%s]\n", 
		    uin->userid, msg, Cdatelite(&now));
	    snprintf(t_last_write, 66, "To %s: %s", uin->userid, msg);
	}
    }
    
    if(flag == 3 && uin->msgcount) {
	/* ���� */
	uin->destuip = currutmp - &utmpshm->uinfo[0];
	uin->sig = 2;
	if(uin->pid > 0) kill(uin->pid, SIGUSR1);
    } else if(flag != 2 &&
	      !HAS_PERM(PERM_SYSOP) &&
	      (uin->pager == 3 || 
	       uin->pager == 2 || 
	       (uin->pager == 4 &&
		!(fri_stat & HFM))))
	outmsg("\033[1;33;41m�V�|! ��訾���F! \033[37m~>_<~\033[m");
    else {
	if(uin->msgcount < MAX_MSGS) {
	    unsigned char pager0 = uin->pager;
	    
	    uin->pager = 2;
	    uin->msgs[uin->msgcount].pid = currpid;
	    strcpy(uin->msgs[uin->msgcount].userid, cuser.userid);
	    strcpy(uin->msgs[uin->msgcount++].last_call_in, msg);
	    uin->pager = pager0;
	} else if (flag != 2)
	    outmsg("\033[1;33;41m�V�|! ��褣��F! (����Ӧh���y) \033[37m@_@\033[m");
	
	if(uin->msgcount >= 1 && (uin->pid <= 0 || kill(uin->pid, SIGUSR2) == -1) && flag != 2)
	    outmsg("\033[1;33;41m�V�|! �S����! \033[37m~>_<~\033[m");
	else if(uin->msgcount == 1 && flag != 2)
	    outmsg("\033[1;33;44m���y�{�L�h�F! \033[37m*^o^*\033[m");
	else if(uin->msgcount > 1 && uin->msgcount < MAX_MSGS && flag != 2)
	    outmsg("\033[1;33;44m�A�ɤW�@��! \033[37m*^o^*\033[m");
    }
    
    clrtoeol();
    refresh();
    
    currutmp->chatid[0] = c0;
    currutmp->mode = mode0;
    currstat = currstat0;
    return 1;
}

void t_display_new(void)
{
    static int t_display_new_flag=0;
    int i, off=2;
    if (t_display_new_flag)
	return;
    else
	t_display_new_flag = 1;

    if( WATERMODE(WATER_ORIG) )
	water_which = &water[0];
    else
	off =3;

    if (water[0].count && watermode > 0){
	move(1, 0);
	outs("�w�w�w�w�w�w�w���w�y�w�^�w�U�w�w�w");
	outs(WATERMODE(WATER_ORIG)                           ?
	     "�w�w�w�w�w�w��[Ctrl-R Ctrl-T]������w�w�w�w�w" :
	     "��[Ctrl-R Ctrl-T Ctrl-F Ctrl-G ]������w�w�w�w");
	if( WATERMODE(WATER_NEW) ){
	    move(2, 0);
	    clrtoeol();
	    for (i = 0; i<6 ; i++){
		if(i>0)
                  if(swater[i-1])
                   {

        	    if( swater[i-1]->uin &&
	           	(swater[i-1]->pid != swater[i-1]->uin->pid ||
		         swater[i-1]->userid[0]!=swater[i-1]->uin->userid[0]) )
              		 swater[i-1]->uin = (userinfo_t*)search_ulist_pid(swater[i-1]->pid);
		    prints("%s%c%-13.13s\033[m",
                           swater[i-1]!=water_which? "" :
			   swater[i-1]->uin?"\033[1;33;47m":
			   "\033[1;33;45m",
                           !swater[i-1]->uin?'#':' ',
                           swater[i-1]->userid);
                   }
                  else
                    prints("              ");
		else
		    prints("%s ����  \033[m",
			   water_which==&water[0]?"\033[1;33;47m ":
			   " "
			   );
	    }
	}
	
	for (i = 0; i < water_which->count; i++){
	    int a = (water_which->top - i - 1 + MAX_REVIEW) % MAX_REVIEW,
		len = 75-strlen(water_which->msg[a].last_call_in)
		-strlen(water_which->msg[a].userid);
	    if(len<0) len=0;
	    
	    move(i + (WATERMODE(WATER_ORIG)?2:3), 0);
	    clrtoeol();
	    if (watermode - 1 != i)
		prints("\033[1;33;46m %s \033[37;45m %s \033[m%*s",
		       water_which->msg[a].userid,
		       water_which->msg[a].last_call_in, len,
		       "");
	    else
		prints("\033[1;44m>\033[1;33;47m%s "
		       "\033[37;45m %s \033[m%*s",
		       water_which->msg[a].userid, 
		       water_which->msg[a].last_call_in,
		       len,"");
	}
	
	if (t_last_write[0]){
	    move(i + off, 0);
	    clrtoeol();
	    prints(t_last_write);
	    i++;
	}
	move(i + off, 0);
	outs("�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w"
	     "�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w");
	if( WATERMODE(WATER_NEW) )
	 while( i++ <= water[0].count ) {
	    move(i + off, 0);
	    clrtoeol();
	}
    }

    t_display_new_flag = 0;
}

int t_display(void)
{
    char genbuf[200], ans[4];
    if(fp_writelog){
	fclose(fp_writelog);
	fp_writelog=NULL;
    }
    setuserfile(genbuf, fn_writelog);
    if (more(genbuf, YEA) != -1){
	getdata(b_lines - 1, 0, "�M��(C) ���ܳƧѿ�(M) �O�d(R) (C/M/R)?[R]",
		ans, sizeof(ans), LCECHO);
	if (*ans == 'm'){
	    fileheader_t mymail;
	    char title[128], buf[80];

	    sethomepath(buf, cuser.userid);
	    stampfile(buf, &mymail);

	    mymail.filemode = FILE_READ | FILE_HOLD;
	    strcpy(mymail.owner, "[��.��.��]");
	    strcpy(mymail.title, "���u\033[37;41m�O��\033[m");
	    sethomedir(title, cuser.userid);
	    Rename(genbuf, buf);
	    append_record(title, &mymail, sizeof(mymail));
	}
	else if (*ans == 'c')
	    unlink(genbuf);
	return FULLUPDATE;
    }
    return DONOTHING;
}

static void do_talk_nextline(talkwin_t * twin)
{
    twin->curcol = 0;
    if (twin->curln < twin->eline)
	++(twin->curln);
    else
	region_scroll_up(twin->sline, twin->eline);
    move(twin->curln, twin->curcol);
}

static void do_talk_char(talkwin_t * twin, int ch)
{
    screenline_t *line;
    int i;
    char ch0, buf[81];

    if (isprint2(ch)){
	ch0 = big_picture[twin->curln].data[twin->curcol];
	if (big_picture[twin->curln].len < 79)
	    move(twin->curln, twin->curcol);
	else
	    do_talk_nextline(twin);
	outc(ch);
	++(twin->curcol);
	line = big_picture + twin->curln;
	if (twin->curcol < line->len){ /* insert */
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
    
    switch (ch){
    case Ctrl('H'):
    case '\177':
	if (twin->curcol == 0)
	    return;
	line = big_picture + twin->curln;
	--(twin->curcol);
	if (twin->curcol < line->len){
	    --(line->len);
	    save_cursor();
	    do_move(twin->curcol, twin->curln);
	    for (i = twin->curcol; i < line->len; i++)
		ochar(line->data[i] = line->data[i + 1]);
	    line->data[i] = 0;
	    ochar(' ');
	    restore_cursor();
	}
	move(twin->curln, twin->curcol);
	return;
    case Ctrl('D'):
	line = big_picture + twin->curln;
	if (twin->curcol < line->len){
	    --(line->len);
	    save_cursor();
	    do_move(twin->curcol, twin->curln);
	    for (i = twin->curcol; i < line->len; i++)
		ochar(line->data[i] = line->data[i + 1]);
	    line->data[i] = 0;
	    ochar(' ');
	    restore_cursor();
	}
	return;
    case Ctrl('G'):
	bell();
	return;
    case Ctrl('B'):
	if (twin->curcol > 0){
	    --(twin->curcol);
	    move(twin->curln, twin->curcol);
	}
	return;
    case Ctrl('F'):
	if (twin->curcol < 79){
	    ++(twin->curcol);
	    move(twin->curln, twin->curcol);
	}
	return;
    case KEY_TAB:
	twin->curcol += 8;
	if (twin->curcol > 80)
	    twin->curcol = 80;
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
	strncpy(buf, (char *)line->data, line->len);
	buf[line->len] = 0;
	do_talk_nextline(twin);
	break;
    case Ctrl('P'):
	line = big_picture + twin->curln;
	strncpy(buf, (char *)line->data, line->len);
	buf[line->len] = 0;
	if (twin->curln > twin->sline){
	    --(twin->curln);
	    move(twin->curln, twin->curcol);
	}
	break;
    case Ctrl('N'):
	line = big_picture + twin->curln;
	strncpy(buf, (char *)line->data, line->len);
	buf[line->len] = 0;
	if (twin->curln < twin->eline){
	    ++(twin->curln);
	    move(twin->curln, twin->curcol);
	}
	break;
    }
    trim(buf);
    if (*buf)
	fprintf(flog, "%s%s: %s%s\n",
		(twin->eline == b_lines - 1) ? "\033[1;35m" : "",
		(twin->eline == b_lines - 1) ?
		getuserid(currutmp->destuid) : cuser.userid, buf,
		(ch == Ctrl('P')) ? "\033[37;45m(Up)\033[m" : "\033[m");
}

static void do_talk(int fd)
{
    struct talkwin_t mywin, itswin;
    char mid_line[128], data[200];
    int i, datac, ch;
    int im_leaving = 0;
    FILE *log;
    struct tm *ptime;
    char genbuf[200], fpath[100];

    ptime = localtime(&now);

    sethomepath(fpath, cuser.userid);
    strcpy(fpath, tempnam(fpath, "talk_"));
    flog = fopen(fpath, "w");

    setuserfile(genbuf, fn_talklog);

    if ((log = fopen(genbuf, "w")))
	fprintf(log, "[%d/%d %d:%02d] & %s\n",
		ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour,
		ptime->tm_min, save_page_requestor);
    setutmpmode(TALK);

    ch = 58 - strlen(save_page_requestor);
    sprintf(genbuf, "%s�i%s", cuser.userid, cuser.username);
    i = ch - strlen(genbuf);
    if (i >= 0)
	i = (i >> 1) + 1;
    else{
	genbuf[ch] = '\0';
	i = 1;
    }
    memset(data, ' ', i);
    data[i] = '\0';

    sprintf(mid_line, "\033[1;46;37m  �ͤѻ��a  \033[45m%s%s�j"
	    " �P  %s%s\033[0m", data, genbuf, save_page_requestor, data);

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

    while (1){
	ch = igetkey();
	if (ch == I_OTHERDATA){
	    datac = recv(fd, data, sizeof(data), 0);
	    if (datac <= 0)
		break;
	    for (i = 0; i < datac; i++)
		do_talk_char(&itswin, data[i]);
	}
	else{
	    if (ch == Ctrl('C')){
		if (im_leaving)
		    break;
		move(b_lines, 0);
		clrtoeol();
		outs("�A���@�� Ctrl-C �N��������͸��o�I");
		im_leaving = 1;
		continue;
	    }
	    if (im_leaving){
		move(b_lines, 0);
		clrtoeol();
		im_leaving = 0;
	    }
	    switch (ch){
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
	    data[0] = (char) ch;
	    if (send(fd, data, 1, 0) != 1)
		break;
	    if (log)
		fprintf(log, "%c", (ch == Ctrl('M')) ? '\n' : (char) *data);
	    do_talk_char(&mywin, *data);
	}
    }
    if (log)
	fclose(log);

    add_io(0, 0);
    close(fd);

    if (flog){
	char ans[4];
	int i;

	fprintf(flog, "\n\033[33;44m���O�e�� [%s] ...     \033[m\n",
		Cdatelite(&now));
	for (i = 0; i < scr_lns; i++)
	    fprintf(flog, "%.*s\n", big_picture[i].len, big_picture[i].data);
	fclose(flog);
	more(fpath, NA);
	getdata(b_lines - 1, 0, "�M��(C) ���ܳƧѿ�(M). (C/M)?[C]",
		ans, sizeof(ans), LCECHO);
	if (*ans == 'm'){
	    fileheader_t mymail;
	    char title[128];

	    sethomepath(genbuf, cuser.userid);
	    stampfile(genbuf, &mymail);
	    mymail.filemode = FILE_READ|FILE_HOLD;
	    strcpy(mymail.owner, "[��.��.��]");
	    sprintf(mymail.title, "��ܰO�� \033[1;36m(%s)\033[m",
		    getuserid(currutmp->destuid));
	    sethomedir(title, cuser.userid);
	    Rename(fpath, genbuf);
	    append_record(title, &mymail, sizeof(mymail));
	}
	else
	    unlink(fpath);
	flog = 0;
    }
    setutmpmode(XINFO);
}

#define lockreturn(unmode, state) if(lockutmpmode(unmode, state)) return 

static void my_talk(userinfo_t * uin, int fri_stat) {
    int sock, msgsock, length, ch, error = 0;
    struct sockaddr_in server;
    pid_t pid;
    char c;
    char genbuf[4];

    unsigned char mode0 = currutmp->mode;

    ch = uin->mode;
    strcpy(currauthor, uin->userid);

    if (ch == EDITING || ch == TALK || ch == CHATING || ch == PAGE ||
	ch == MAILALL || ch == MONITOR || ch == M_FIVE || ch == CHC ||
	(!ch && (uin->chatid[0] == 1 || uin->chatid[0] == 3)) ||
	uin->lockmode == M_FIVE || uin->lockmode == CHC){
	outs("�H�a�b����");
    }
    else if (!HAS_PERM(PERM_SYSOP) &&
	     (((fri_stat& HRM) && !(fri_stat& HFM)) ||
	      ((!uin->pager) && !(fri_stat & HFM)) ) ){
	outs("��������I�s���F");
    }
    else if (!HAS_PERM(PERM_SYSOP) &&
	     (((fri_stat & HRM) && !(fri_stat& HFM)) || uin->pager == 2 )) {
	outs("���ޱ��I�s���F");
    }
    else if (!HAS_PERM(PERM_SYSOP) &&
	     !(fri_stat & HFM) && uin->pager == 4){
	outs("���u�����n�ͪ��I�s");
    }
    else if (!(pid = uin->pid) /*|| (kill(pid, 0) == -1) */ ){
	//resetutmpent();
	outs(msg_usr_left);
    }
    else{
	showplans(uin->userid);
	getdata(2, 0, "�n�M�L(�o) (T)�ͤ�(F)�U���l��(P)���d��"
		"(C)�U�H��(D)�U�t��(N)�S�Ƨ���H�F?[N] ", genbuf, 4, LCECHO);
	switch (*genbuf){
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
	case 'p':
	    reload_chicken();
	    getuser(uin->userid);
	    if (uin->lockmode == CHICKEN || currutmp->lockmode == CHICKEN)
		error = 1;
	    if (!cuser.mychicken.name[0] || !xuser.mychicken.name[0])
		error = 2;
	    if (error){
		outmsg(error == 2 ? "�ëD��H���i�d��" :
		       "���@�誺�d�����b�ϥΤ�");
		bell();
		refresh();
		sleep(1);
		return;
	    }
	    uin->sig = SIG_PK;
	    break;
	default:
	    return;
	}

	uin->turn = 1;
	currutmp->turn = 0;
	strcpy(uin->mateid, currutmp->userid);
	strcpy(currutmp->mateid, uin->userid);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0){
	    perror("sock err");
	    unlockutmpmode();
	    return;
	}
	server.sin_family = PF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = 0;
	if (bind(sock, (struct sockaddr *) &server, sizeof(server)) < 0){
	    close(sock);
	    perror("bind err");
	    unlockutmpmode();
	    return;
	}
	length = sizeof(server);
	if (getsockname(sock, (struct sockaddr *) &server, (socklen_t*)&length) < 0){
	    close(sock);
	    perror("sock name err");
	    unlockutmpmode();
	    return;
	}
	currutmp->sockactive = YEA;
	currutmp->sockaddr = server.sin_port;
	currutmp->destuid = uin->uid;
	setutmpmode(PAGE);
	uin->destuip = currutmp - &utmpshm->uinfo[0];
	if(pid > 0) kill(pid, SIGUSR1);
	clear();
	prints("���I�s %s.....\n��J Ctrl-D ����....", uin->userid);

	listen(sock, 1);
	add_io(sock, 5);
	while (1){
	    ch = igetch();
	    if (ch == I_TIMEOUT){
		ch = uin->mode;
		if (!ch && uin->chatid[0] == 1 &&
		    uin->destuip == currutmp - &utmpshm->uinfo[0]){
		    bell();
		    outmsg("���^����...");
		    refresh();
		}
		else if (ch == EDITING || ch == TALK || ch == CHATING ||
			 ch == PAGE || ch == MAILALL || ch == MONITOR ||
			 ch == M_FIVE || ch == CHC ||
			 (!ch && (uin->chatid[0] == 1 ||
				  uin->chatid[0] == 3))){
		    add_io(0, 0);
		    close(sock);
		    currutmp->sockactive = currutmp->destuid = 0;
		    outmsg("�H�a�b����");
		    pressanykey();
		    unlockutmpmode();
		    return;
		}
		else{
#ifdef linux
		    add_io(sock, 20);	/* added for linux... achen */
#endif
		    move(0, 0);
		    outs("�A");
		    bell();

		    uin->destuip = currutmp - &utmpshm->uinfo[0];
		    if(pid <= 0 || kill(pid, SIGUSR1) == -1){
#ifdef linux
			add_io(sock, 20);	/* added 4 linux... achen */
#endif
			outmsg(msg_usr_left);
			refresh();
			pressanykey();
			unlockutmpmode();
			return;
		    }
		    continue;
		}
	    }
	    
	    if (ch == I_OTHERDATA)
		break;

	    if (ch == '\004'){
		add_io(0, 0);
		close(sock);
		currutmp->sockactive = currutmp->destuid = 0;
		unlockutmpmode();
		return;
	    }
	}

	msgsock = accept(sock, (struct sockaddr *) 0, (socklen_t *) 0);
	if (msgsock == -1){
	    perror("accept");
	    unlockutmpmode();
	    return;
	}
	add_io(0, 0);
	close(sock);
	currutmp->sockactive = NA;
	read(msgsock, &c, sizeof c);

	if (c == 'y'){
	    sprintf(save_page_requestor, "%s (%s)",
		    uin->userid, uin->username);
	    /* gomo */
	    switch (uin->sig){
	    case SIG_DARK:
		main_dark(msgsock, uin);
		break;
	    case SIG_PK:
		chickenpk(msgsock);
		break;
	    case SIG_GOMO:
		gomoku(msgsock);
		break;
	    case SIG_CHC:
		chc(msgsock);
		break;
	    case SIG_TALK:
	    default:
		do_talk(msgsock);
	    }
	}
	else{
	    move(9, 9);
	    outs("�i�^���j ");
	    switch (c){
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
		    char msgbuf[60];
		    
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

static void t_showhelp() {
    clear();

    outs("\033[36m�i �𶢲�Ѩϥλ��� �j\033[m\n\n"
	 "(��)(e)         �������}             (h)             �ݨϥλ���\n"
	 "(��)/(��)(n)    �W�U����             (TAB)           �����ƧǤ覡\n"
	 "(PgUp)(^B)      �W�����             ( )(PgDn)(^F)   �U�����\n"
	 "(Hm)/($)(Ed)    ��/��                (S)             "
	 "�ӷ�/�n�ʹy�z/���Z ����\n"
	 "(m)             �H�H                 (q/c)           "
	 "�d�ߺ���/�d��\n"
	 "(r)             �\\Ū�H��             (l/C)           �ݤW�����T/��������\n"
	 "(f)             ����/�n�ͦC��        (�Ʀr)          ���ܸӨϥΪ�\n"
	 "(p)             �����I�s��           (g/i)           ����/�����߱�\n"
	 "(a/d/o)         �n�� �W�[/�R��/�ק�  (/)(s)          ����ID/�ʺٷj�M");

    if (HAS_PERM(PERM_PAGE)){
	outs("\n\n\033[36m�i ��ͱM���� �j\033[m\n\n"
	     "(��)(t)(Enter)  ��L���o���\n"
	     "(w)             ���u Call in\n"
	     "(W)�������y�覡 �@�� / �i�� / ����\n"
	     "(b)             ��n�ͼs�� (�@�w�n�b�n�ͦC��)\n"
	     "(^R)            �Y�ɦ^�� (���H Call in �A��)");
    }

    if (HAS_PERM(PERM_SYSOP)){
	outs("\n\n\033[36m�i �����M���� �j\033[m\n\n");
	if (HAS_PERM(PERM_SYSOP))
	    outs("(u)/(H)         �]�w�ϥΪ̸��/�������μҦ�\n");
	outs("(R)/(K)         �d�ߨϥΪ̪��u��m�W/���a�J��X�h\n");
    }
    pressanykey();
}

/*
static int listcuent(userinfo_t * uentp)
{
    if((!uentp->invisible || HAS_PERM(PERM_SYSOP) || HAS_PERM(PERM_SEECLOAK)))
	AddNameList(uentp->userid);
    return 0;
}

static void creat_list()
{
    CreateNameList();
    apply_ulist(listcuent);
}
*/

/* Kaede show friend description */
static char *friend_descript(char *uident) {
    static char *space_buf = "                    ";
    static char desc_buf[80];
    char fpath[80], name[IDLEN + 2], *desc, *ptr;
    int len, flag;
    FILE *fp;
    char genbuf[200];

    setuserfile(fpath, friend_file[0]);

    if ((fp = fopen(fpath, "r"))){
	sprintf(name, "%s ", uident);
	len = strlen(name);
	desc = genbuf + 13;

	while ((flag = (int) fgets(genbuf, STRLEN, fp))){
	    if (!memcmp(genbuf, name, len)){
		if ((ptr = strchr(desc, '\n')))
		    ptr[0] = '\0';
		if (desc)
		    break;
	    }
	}
	fclose(fp);
	if (desc && flag)
	    strcpy(desc_buf, desc);
	else
	    return space_buf;

	return desc_buf;
    }
    else
	return space_buf;
}

static char *descript(int show_mode, userinfo_t * uentp, time_t diff,
		      fromcache_t * fcache)
{
    switch (show_mode){
    case 1:
	return friend_descript(uentp->userid);
    case 0:
	return (((uentp->pager != 2 && uentp->pager != 3 && diff) ||
		 HAS_PERM(PERM_SYSOP)) ?
#ifdef WHERE
		uentp->from_alias ? fcache->replace[uentp->from_alias] :
		uentp->from
#else
		uentp->from
#endif
		: "*");
    case 2:
	sprintf(description, "%3d/%3d/%3d", uentp->five_win,
		uentp->five_lose, uentp->five_tie);
	description[20] = 0;
	return description;
    default:
	syslog(LOG_WARNING, "damn!!! what's wrong?? show_mode = %d",
	       show_mode);
	return "";
    }
}

#define MAXPICKUP       20
static int sort_cmpfriend(const void *a, const void *b)
{
    if( ((((pickup_t *)a)->friend)&ST_FRIEND) == 
	((((pickup_t *)b)->friend)&ST_FRIEND)           )
	return strcasecmp( ((pickup_t *)a)->ui->userid,
			   ((pickup_t *)b)->ui->userid  );
    else
	return (((pickup_t *)b)->friend & ST_FRIEND) -
	       (((pickup_t *)a)->friend & ST_FRIEND);
}

int pickup_maxpages(int pickupway, int nfriend, int bfriend)
{
    int     number;
    if( cuser.uflag & FRIEND_FLAG )
	number = nfriend + bfriend;
    else
	number = utmpshm->number +
	    (pickupway == 0 ? nfriend + bfriend : 0);
    return number / MAXPICKUP + (number % MAXPICKUP == 0 ? 0 : 1);
}

static int pickup_myfriend(pickup_t *friends,
			    int *myfriend, int *friendme)
{
    //pickup_t        friends[MAX_FRIEND + 1];
    userinfo_t      *uentp;
    int     i, where, frstate, ngets=0;
    
    *myfriend = *friendme = 1;
    for( i = 0 ; currutmp->friend_online[i] && i < MAX_FRIEND ; ++i ){
	where = currutmp->friend_online[i] & 0xFFFFFF;
	if( 0 <= where && where < MAX_ACTIVE                 &&
	    (uentp = &utmpshm->uinfo[where]) && uentp->pid   &&
	    uentp != currutmp                                &&
	    isvisible_stat(currutmp, uentp,
			   frstate =
			   currutmp->friend_online[i] >> 24) &&
	    ( !(frstate & IRH) || ((frstate & IRH) && (frstate & IFH)) )
	    ){
	    friends[ngets].ui = uentp;
	    friends[ngets].uoffset = where;
            if(uentp->brc_id==currutmp->brc_id)  frstate |= IBH;
	    friends[ngets++].friend = frstate;
	    if( frstate & IFH )
		++*myfriend;
	    if( frstate & HFM )
		++*friendme;
	}
    }
    friends[ngets].ui = currutmp;
    friends[ngets++].friend = 24;
    return ngets;
}

static int pickup_bfriend(pickup_t *friends, int base)
{
    userinfo_t      *ptr;
    int ngets=0;
    friends = friends + base;
    for( ptr = bcache[currutmp->brc_id - 1].u;
	 ptr != NULL && ngets < MAX_FRIEND-base ;
	 ptr = ptr->nextbfriend                             ){
	if( currutmp != ptr && isvisible(currutmp, ptr) &&
            !(friend_stat(currutmp,ptr)&(IFH|HFM)) ){
	    friends[ngets].ui = ptr;
	    friends[ngets++].friend = IBH;
	}
    }
    return ngets;
}

static void pickup(pickup_t *currpickup, int pickup_way, int *page,
		   int *nfriend, int *myfriend, int *friendme, int *bfriend)
{
    /* avoid race condition */
    int     currsorted = utmpshm->currsorted;
    int     utmpnumber = utmpshm->number;
    int     friendtotal= currutmp->friendtotal;

    userinfo_t      **utmp;
    int     which, all=0, sorted_way, size=0;

    if( friendtotal == 0 )
	*myfriend = *friendme = 1;

    if( cuser.uflag & FRIEND_FLAG ||
	(pickup_way == 0 && *page * MAXPICKUP < MAX_FRIEND ) )
    {
	/* [��! �B��] mode.
	   we need to pickup ALL friends (from currutmp friend_online),
	   sort, and get pickup from right starting position */
	pickup_t        friends[MAX_FRIEND];

	*nfriend = pickup_myfriend(friends, myfriend, friendme);

        if( pickup_way == 0 )
               *bfriend=pickup_bfriend(friends,*nfriend);
        else
               *bfriend=0;

        all = *bfriend+ *nfriend ;
        which =  *page * MAXPICKUP;
        if(all>which) // Ptt: �u���b�n�q�X�~�����n sort
         {
          qsort(friends, all, sizeof(pickup_t), sort_cmpfriend);
          size=all-which;
          if(size>MAXPICKUP) size= MAXPICKUP;
          memcpy(currpickup, friends+which, sizeof(pickup_t)*size); 
         }
    }

    if( !(cuser.uflag & FRIEND_FLAG) && size < MAXPICKUP )
    {
	sorted_way = ((pickup_way == 0) ? 0 : (pickup_way - 1));
	utmp = utmpshm->sorted[currsorted][sorted_way];
        which = *page * MAXPICKUP-all;
        if(which<0) which=0;
        for(;which < utmpnumber && size < MAXPICKUP;which++)
          {
              if(currutmp != utmp[which] &&
                 !(friend_stat(currutmp,utmp[which])&ST_FRIEND) &&
                 isvisible_stat(currutmp, utmp[which], 0))
                {
                 currpickup[size].ui = utmp[which];
                 currpickup[size++].friend = 0;
                }
          }
    }
/*
	for( which = (which >= 0 ? which : 0)       ;
	     got < MAXPICKUP && which < utmpnumber  ;
	     ++got, ++which                               ){
	    
	    for( ; which < utmpnumber ; ++which )
		if( currutmp != utmp[which] &&
		    isvisible_stat(currutmp, utmp[which], 0) )
		    break;
	    if( which == utmpnumber )
		break;
	    currpickup[got].ui = utmp[which];
	    currpickup[got].friend = 0;
	}
*/

    for( ; size < MAXPICKUP ; ++size )
	currpickup[size].ui = 0;
}

char    *Mind[] = {
    "   ",
    "^-^", "^_^", "Q_Q", "@_@", "/_\\", "=_=", "-_-", "-.-", ">_<",
    "-_+", "!_!", "o_o", "z_Z", "O_O", "O.O", "$_$", "^*^", "O_<",
    "��!", "��!", "�s!", "��!", ":) ", ":( ", ":~ ", ":q ", ":O ",
    ":D ", ":p ", ";) ", ":> ", ";> ", ":< ", ":)~", ":D~", ">< ",
    "^^;", "^^|", "��;",  NULL
};
static void draw_pickup(int drawall, pickup_t *pickup, int pickup_way,
			int page, int show_mode, int show_uid, int show_board,
			int show_pid, int real_name,
			int myfriend, int friendme, int bfriend)
{
    char    *msg_pickup_way[PICKUP_WAYS] = {
	"��! �B��", "���ͥN��", "���ͰʺA", "�o�b�ɶ�", "�Ӧۦ��", "���l��  "
    };
    char    *MODE_STRING[MAX_SHOW_MODE] = {"�G�m", "�n�ʹy�z", "���l�Ѿ��Z"};
    char    pagerchar[5] = "* -Wf";

    userinfo_t      *uentp;
    int     i, ch, state, friend;
#ifdef SHOW_IDLE_TIME
    char    idlestr[32];
    int     idletime;
#endif

    if( drawall ){
	showtitle((cuser.uflag & FRIEND_FLAG) ? "�n�ͦC��" : "�𶢲��",
		  BBSName);
	prints("\n"
	       "\033[7m  %s P%c�N��         %-17s%-17s%-13s%-10s\033[m\n",
	       show_uid ? "UID" : "No.", 
	       (HAS_PERM(PERM_SEECLOAK) || HAS_PERM(PERM_SYSOP)) ? 'C' : ' ',
	       real_name ? "�m�W" : "�ʺ�",
	       MODE_STRING[show_mode],
	       show_board ? "Board" : "�ʺA",
	       show_pid ? "       PID" : "�Ƶ�  �o�b"
	       );
	move(b_lines, 0);
	outs("\033[31;47m(TAB/f)\033[30m�Ƨ�/�n�� \033[31m(t)\033[30m��� "
	     "\033[31m(a/d/o)\033[30m��� \033[31m(q)\033[30m�d�� "
	     "\033[31m(w)\033[30m���y \033[31m(m)\033[30m�H�H \033[31m(h)"
	     "\033[30m�u�W���U \033[m");
    }

    move(1, 0);
    prints("  �ƧǡG[%s] �W���H�ơG%-4d\033[1;32m�ڪ��B�͡G%-3d"
	   "\033[33m�P�ڬ��͡G%-3d\033[36m�O�͡G%-4d\033[31m�a�H�G"
	   "%-2d\033[m\n",
	   msg_pickup_way[pickup_way], utmpshm->number,
	   myfriend, friendme, currutmp->brc_id ? (bfriend + 1) : 0, 0);

    for( i = 0, ch = page * 20 + 1 ; i < MAXPICKUP ; ++i, ++ch ){
	move(i + 3, 0);	prints("a");
	move(i + 3, 0);
	uentp = pickup[i].ui;
	friend = pickup[i].friend;
	if( uentp == NULL ){
	    prints("\n");
	    continue;
	}
	
	if( !uentp->pid ){
	    prints("%5d   < ������..>\n", ch);
	    continue;
	}

	if( PERM_HIDE(uentp) )
	    state = 9;
	else if( currutmp == uentp )
	    state = 10;
	else if( friend & IRH && !(friend & IFH) )
	    state = 8;
	else
	    state = (friend & ST_FRIEND) >> 2;

#ifdef SHOW_IDLE_TIME
	idletime = (now - uentp->lastact);
	if( idletime > 86400 )
	    strcpy(idlestr, " -----"); 
	else if( idletime >= 3600 )
	    sprintf(idlestr, "%3dh%02d",
		    idletime / 3600, (idletime / 60) % 60);
	else if( idletime > 0 )
	    sprintf(idlestr, "%3d'%02d",
		    idletime / 60, idletime % 60);
	else
	    strcpy(idlestr, "      "); 
#endif

	prints("%5d %c%c%s%-13s%-17.16s\033[m%-17.16s%-13.13s"
	       "\33[33m%-4.4s\33[m%s\n",

	       /* list number or uid */
#ifdef SHOWUID
	       show_uid ? uentp->uid :
#endif
	       ch,

	       /* super friend or pager */
	       (friend & HRM) ? 'X' :  pagerchar[uentp->pager % 5],

	       /* visibility */
	       (uentp->invisible ? ')' : ' '),

	       /* color of userid, userid */
	       fcolor[state], uentp->userid,

	       /* nickname or realname */
#ifdef REALINFO
	       real_name ? uentp->realname :
#endif
	       uentp->username,

	       /* from */
	       descript(show_mode, uentp,
			uentp->pager & !(friend&HRM), fcache),

	       /* board or mode */
#ifdef SHOWBOARD
	       show_board ? (uentp->brc_id == 0 ? "" :
			     bcache[uentp->brc_id - 1].brdname) :
#endif
	       /* %-13.13s */
	       modestring(uentp, 0),
	       
	       /* memo */
	       ((uentp->userlevel & PERM_VIOLATELAW) ? "�q�r" :
		(uentp->birth ? "�جP" : Mind[uentp->mind])),

	       /* idle */
#ifdef SHOW_IDLE_TIME
	       idlestr
#else
	       ""
#endif
	       );

	refresh();
    }
}

int call_in(userinfo_t *uentp, int fri_stat)
{
    if( iswritable_stat(uentp, fri_stat) ){
	char genbuf[60];
	sprintf(genbuf, "Call-In %s �G", uentp->userid);
	my_write(uentp->pid, genbuf, uentp->userid, 0, NULL);
	return 1;
    }
    return 0;
}

static void userlist(void)
{
    /* �ϥΪ̦W��:
       userlist()      : main loop
       draw_pickup()   : show out screen
       pickup()        : generate THIS PAGE pickup list
       pickup_maxpages : return max pages number of all list
       pickup_myfriend : pickup friend (from friend_online) and sort
    */
    pickup_t        currpickup[MAXPICKUP];
    userinfo_t      *uentp;
    static  int     show_mode = 0;
    static  int     show_uid = 0;
    static  int     show_board = 0;
    static  int     show_pid = 0;
    static  int     real_name = 0;
    char    genbuf[256];
    int     page, offset, pickup_way, ch, leave, redraw, redrawall, fri_stat;
    int     nfriend, myfriend, friendme, bfriend, i;
    time_t  lastupdate;

    page = offset = 0;
    pickup_way = 0;
    leave = 0;
    redrawall = 1;
    /* redraw    : �|���}��L�B�z loop , ���s pickup, draw_pickup
                    (�u���e�����ϥΪ̳���)
       redrawall : �|���e�Ҧ������� (�W�������D�C, �U���������C����)
       leave     : ��^�W�@���
    */
    while( !leave ){
	pickup(currpickup, pickup_way, &page,
	       &nfriend, &myfriend, &friendme, &bfriend);
	draw_pickup(redrawall, currpickup, pickup_way, page,
		    show_mode, show_uid, show_board, show_pid, real_name,
		    myfriend, friendme, bfriend);

	/* �p�G�]���������ɭ�, �o�@�������H�Ƥ����,
	    (�q�`���O�̫�@���H�Ƥ������ɭ�)
	   ���n���s�p�� offset �H�K����S���H���a�� */
	if( offset == -1 || currpickup[offset].ui == NULL ){
	    for( offset = (offset == -1 ? MAXPICKUP - 1 : offset ) ;
		 offset >= 0 ; --offset )
		if( currpickup[offset].ui != NULL )
		    break;
	    if( offset == -1 ){
		if( --page < 0 )
		    page = pickup_maxpages(pickup_way, nfriend, bfriend) - 1;
		offset = 0;
		continue;
	    }
	}

	redraw = redrawall = 0;
	lastupdate = now;
	while( !redraw ){
	    ch = cursor_key(offset + 3, 0);
	    uentp = currpickup[offset].ui;
	    fri_stat = currpickup[offset].friend;

	    if (ch == KEY_RIGHT || ch == '\n' || ch == '\r')
		ch = 't';

	    switch (ch){
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
		if( ++offset == MAXPICKUP || currpickup[offset].ui == NULL ){
		    redraw = 1;
		    if( ++page >= pickup_maxpages(pickup_way,
						  nfriend, bfriend) )
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
		if (HAS_PERM(PERM_SYSOP)){
		    currutmp->userlevel ^= PERM_DENYPOST;
		    redrawall = redraw = 1;
		}
		break;

	    case 'D':
		if (HAS_PERM(PERM_SYSOP)){
		    char buf[100];
		    sprintf(buf, "�N�� [%s]�G", currutmp->userid);
		    if (!getdata(1, 0, buf, currutmp->userid,
				 sizeof(buf), DOECHO))
			strcpy(currutmp->userid, cuser.userid);
		    redrawall = redraw = 1;
		}
		break;

	    case 'F':
		if (HAS_PERM(PERM_SYSOP)){
		    char buf[100];

		    sprintf(buf, "�G�m [%s]�G", currutmp->from);
		    if (!getdata(1, 0, buf, currutmp->from,
				 sizeof(currutmp->from), DOECHO))
			strncpy(currutmp->from, fromhost, 23);
		    redrawall = redraw = 1;
		}
		break;

	    case 'C':
#if !HAVE_FREECLOAK
		if (HAS_PERM(PERM_CLOAK))
#endif
		    {
			currutmp->invisible ^= 1;
			redrawall = redraw = 1;
		    }
		break;

	    case ' ':
	    case KEY_PGDN:
	    case Ctrl('F'):{
		int     newpage;
		if( (newpage = page + 1) >= pickup_maxpages(pickup_way,
							    nfriend, bfriend) )
		    newpage = offset = 0;
		if( newpage != page ){
		    page = newpage;
		    redraw = 1;
		} else if( now >= lastupdate + 2 )
		    redrawall = redraw = 1;
	    }
		break;

	    case KEY_UP:
	    case 'k':
		if( --offset == -1 ){
		    offset = MAXPICKUP - 1;
		    if( --page == -1 )
			page = pickup_maxpages(pickup_way, nfriend, bfriend)
			       - 1;
		    redraw = 1;
		}
		break;

	    case KEY_PGUP:
	    case Ctrl('B'):
	    case 'P':
		if( --page == -1 )
		    page = pickup_maxpages(pickup_way, nfriend, bfriend) - 1;
		offset = 0;
		redraw = 1;
		break;

	    case KEY_END:
	    case '$':
		page = pickup_maxpages(pickup_way, nfriend, bfriend) - 1;
		offset = -1;
		redraw = 1;
		break;

	    case '/':
		/*
	        getdata_buf(b_lines-1,0,"�п�J�ʺ�����r:",
			    keyword, sizeof(keyword), DOECHO);
		state = US_PICKUP;
		*/
	        break;

	    case 's':
		if( !(cuser.uflag & FRIEND_FLAG) ){
		    int     si; /* utmpshm->sorted[X][0][si] */
		    int     fi; /* allpickuplist[fi] */
		    char    swid[IDLEN + 1];
		    move(1, 0);
		    si = generalnamecomplete(msg_uid, swid,
					     sizeof(swid), utmpshm->number,
					     completeutmp_compar,
					     completeutmp_permission,
					     completeutmp_getname);
		    if( si >= 0 ){
			pickup_t        friends[MAX_FRIEND + 1];
			int     nGots, i;
			fi = utmpshm->sorted[utmpshm->currsorted][0][si] -
			     &utmpshm->uinfo[0];

			nGots= pickup_myfriend(friends, &myfriend, &friendme);
			for( i = 0 ; i < nGots ; ++i )
			    if( friends[i].uoffset == fi )
				break;
			if( i != nGots ){
			    page = i / 20;
			    offset = i % 20;
			}
			else{
			    page = (si + nGots) / 20;
			    offset = (si + nGots) % 20;
			}			
		    }
		    redrawall = redraw = 1;
		}
		/*
		if ((i = search_pickup(num, actor, pklist)) >= 0)
		    num = i;
		state = US_ACTION;
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
		    int     tmp;
		    if( (tmp = search_num(ch, utmpshm->number)) >= 0 ){
			if( tmp / 20 == page ){
			    /* in2:�ت��b�ثe�o�@��, ����
			           ��s offset , ���έ��e�e�� */
			    offset = tmp % 20;
			}
			else{
			    page = tmp / 20;
			    offset = tmp % 20;
			}
			redrawall = redraw = 1;
		    }
		}
		break;
		
#ifdef REALINFO
	    case 'R':		/* ��ܯu��m�W */
		if (HAS_PERM(PERM_SYSOP)){
		    real_name ^= 1;
		    redrawall = redraw = 1;
		}
		break;
#endif
#ifdef SHOWUID
	    case 'U':
		if (HAS_PERM(PERM_SYSOP)){
		    show_uid ^= 1;
		    redrawall = redraw = 1;
		}
		break;
#endif
#ifdef  SHOWBOARD
	    case 'Y':
		if (HAS_PERM(PERM_SYSOP)){
		    show_board ^= 1;
		    redrawall = redraw = 1;
		}
		break;
#endif
#ifdef  SHOWPID
	    case '#':
		if (HAS_PERM(PERM_SYSOP)){
		    show_pid ^= 1;
		    redrawall = redraw = 1;
		}
		break;
#endif

	    case 'b':		/* broadcast */
		if (cuser.uflag & FRIEND_FLAG || HAS_PERM(PERM_SYSOP)){
		    char ans[4];
		    
		    if (!getdata(0, 0, "�s���T��:", genbuf, 60, DOECHO))
			break;
		    if (getdata(0, 0, "�T�w�s��? [Y]",
				ans, sizeof(ans), LCECHO) &&
			*ans == 'n')
			break;
		    if( HAS_PERM(PERM_SYSOP) ){
			for( i = 0 ; i < utmpshm->number && i<1000 ; ++i ){
			    uentp = utmpshm->sorted[utmpshm->currsorted][0][i];
			    if( uentp->pid && kill(uentp->pid, 0) != -1 )
				my_write(uentp->pid, genbuf,
					 uentp->userid, 1, NULL);
			}
		    }
		    else{
			userinfo_t      *uentp;
			int     where, frstate;
			for( i = 0 ; currutmp->friend_online[i] &&
				 i < MAX_FRIEND ; ++i              ){
			    where = currutmp->friend_online[i] & 0xFFFFFF;
			    if( 0 <= where && where < MAX_ACTIVE &&
				(uentp = &utmpshm->uinfo[where]) &&
				uentp->pid &&
				isvisible_stat(currutmp, uentp,
					       frstate =
					       currutmp->friend_online[i]>>24)
				&& kill(uentp->pid, 0) != -1 &&
				uentp->pager != 3 &&
				(uentp->pager != 4 || frstate & HFM) ){
				my_write(uentp->pid, genbuf,
					 uentp->userid, 1, NULL);
			    }
			}
		    }
		    redrawall = redraw = 1;
		}
		break;

	    case 'S':		/* ��ܦn�ʹy�z */
		show_mode = (++show_mode) % MAX_SHOW_MODE;
		redrawall = redraw = 1;
		break;

	    case 'u':           /* �u�W�ק��� */
		if( HAS_PERM(PERM_ACCOUNTS) ){
		    int     id;
		    userec_t muser;
		    strcpy(currauthor, uentp->userid);
		    stand_title("�ϥΪ̳]�w");
		    move(1, 0);
		    if( (id = getuser(uentp->userid)) > 0 ){
			memcpy(&muser, &xuser, sizeof(muser));
			user_display(&muser, 1);              
			uinfo_query(&muser, 1, id);
		    }
		    redrawall = redraw = 1;
		}
		break;

	    case 'i':
		move(3,0);
		clrtobot();
		for( i = 0 ; Mind[i] != NULL ; ++i ){
		    move(5 + (i - 1) / 7, ((i - 1) % 7) * 10);
		    prints("%2d: %s", i, Mind[i]);
		}
		getdata(b_lines - 1, 0, "�A�{�b���߱� 0:�L q���� [q]:",
				genbuf, 3, LCECHO);
		if( genbuf[0] && genbuf[0] != 'q' && atoi(genbuf) >= 0)
		    currutmp->mind = atoi(genbuf) % i;
		break;
		
	    case Ctrl('S'):
		break;

	    case 't':
		if( HAS_PERM(PERM_LOGINOK) ){
		    if( uentp->pid != currpid &&
			strcmp(uentp->userid, cuser.userid) != 0 ){
			move(1, 0);
			clrtobot();
			move(3, 0);
			my_talk(uentp, fri_stat);
			redrawall = redraw = 1;
		    }
		}
		break;
            case 'K':
                if(HAS_PERM(PERM_ACCOUNTS))
                  {
                        my_kick(uentp);
                        redrawall = redraw = 1;
                  }
                break;
	    case 'w':
                if(call_in(uentp,fri_stat))
                        redrawall = redraw = 1;
		break;
	    case 'a':
		if( HAS_PERM(PERM_LOGINOK) ){
		    friend_add(uentp->userid, FRIEND_OVERRIDE);
		    friend_load();
		    redrawall = redraw = 1;
		}
		break;

	    case 'd':
		if( HAS_PERM(PERM_LOGINOK) ){
		    friend_delete(uentp->userid, FRIEND_OVERRIDE);
		    friend_load();
		    redrawall = redraw = 1;
		}
		break;

	    case 'o':
		if( HAS_PERM(PERM_LOGINOK) ){
		    t_override();
		    redrawall = redraw = 1;
		}
		break;

	    case 'f':
		if( HAS_PERM(PERM_LOGINOK) ){
		    cuser.uflag ^= FRIEND_FLAG;
		    redrawall = redraw = 1;
		}
		break;
		   
	    case 'g':
		if( HAS_PERM(PERM_LOGINOK) &&
		    strcmp(uentp->userid, cuser.userid) != 0 ){
		    move(b_lines - 2, 0);
		    sprintf(genbuf, "�n�� %s �h�ֿ��O?  ", uentp->userid);
                    outs(genbuf);
		    if( getdata(b_lines - 1, 0, "[�Ȧ���b]: ",
				genbuf, 7, LCECHO) ){
                        clrtoeol();
			if( (ch == atoi(genbuf)) <= 0 || ch <= give_tax(ch) )
                            break;
			reload_money();
			
			if( ch > cuser.money )
                            outs("\033[41m �{������~~\033[m");
                        else{
                            deumoney(uentp->uid, ch - give_tax(ch));
                            sprintf(genbuf, "\033[44m ��..�ٳѤU %d ��.."
                                    "\033[m", demoney(-ch));
                            outs(genbuf);
                            sprintf(genbuf, "%s\t��%s\t%d\t%s", cuser.userid,
                                    uentp->userid, ch,
                                    ctime(&currutmp->lastact));
                            log_file(FN_MONEY, genbuf);
                            mail_redenvelop(cuser.userid, uentp->userid,
					    ch - give_tax(ch), 'Y');
                        }
                    }
                    else{
                        clrtoeol();
                        outs("\033[41m �������! \033[m");
                    }
		    redrawall = redraw = 1;
		}
		break;

	    case 'm':
		stand_title("�H  �H");
		prints("[�H�H] ���H�H�G%s", uentp->userid);
		my_send(uentp->userid);
		break;

	    case 'q':
                strcpy(currauthor, uentp->userid);
                my_query(uentp->userid);
		setutmpmode(LUSERS);
		redrawall = redraw = 1;
		break;

	    case 'c':
		if( HAS_PERM(PERM_LOGINOK) ){
		    chicken_query(uentp->userid);
		    redrawall = redraw = 1;
		}
		break;

	    case 'l':
		if( HAS_PERM(PERM_LOGINOK) ){
		    t_display();
		    redrawall = redraw = 1;
		}
		break;

	    case 'h':
		t_showhelp();
		redrawall = redraw = 1;
		break;

	    case 'p':
		if( HAS_PERM(PERM_BASIC) ){
		    t_pager();
		    redrawall = redraw = 1;
		}
		break;

	    case 'W':
		if( HAS_PERM(PERM_LOGINOK) ){
		    int     tmp;                          
                    char    *wm[3] = {"�@��", "�i��", "����"};
                    tmp = cuser.uflag2 & WATER_MASK;
                    cuser.uflag2 -= tmp;
                    tmp = (tmp + 1) % 3;
                    cuser.uflag2 |= tmp;
		    move(4, 0);
		    prints("�t�δ��� �@�� �i�� ���� �T�ؼҦ�\n"
			   "�b������Х��`�U�u�A���s�n�J, �H�T�O���c���T\n"
			   "�ثe������ %s ���y�Ҧ�\n", wm[tmp]);
                    refresh();
                    sleep(2);
		    redrawall = redraw = 1;
		}
		break;

	    case 'r':
		if( HAS_PERM(PERM_LOGINOK) ){
		    m_read();
		    redrawall = redraw = 1;
		}
		break;

	    default:
		if( now >= lastupdate + 2 )
		    redraw = 1;
	    }
	}
    }
}

int t_users(void)
{
    int destuid0 = currutmp->destuid;
    int mode0 = currutmp->mode;
    int stat0 = currstat;

    setutmpmode(LUSERS);
    userlist();
    currutmp->mode = mode0;
    currutmp->destuid = destuid0;
    currstat = stat0;
    return 0;
}

int t_pager(void)
{
    currutmp->pager = (currutmp->pager + 1) % 5;
    return 0;
}

int t_idle(void)
{
    int destuid0 = currutmp->destuid;
    int mode0 = currutmp->mode;
    int stat0 = currstat;
    char genbuf[20];
    char buf[80], passbuf[PASSLEN];

    setutmpmode(IDLE);
    getdata(b_lines - 1, 0, "�z�ѡG[0]�o�b (1)���q�� (2)�V�� (3)���O�� "
	    "(4)�˦� (5)ù�� (6)��L (Q)�S�ơH", genbuf, 3, DOECHO);
    if (genbuf[0] == 'q' || genbuf[0] == 'Q'){
	currutmp->mode = mode0;
	currstat = stat0;
	return 0;
    }
    else if (genbuf[0] >= '1' && genbuf[0] <= '6')
	currutmp->destuid = genbuf[0] - '0';
    else
	currutmp->destuid = 0;

    if (currutmp->destuid == 6)
	if (!cuser.userlevel ||
	    !getdata(b_lines - 1, 0, "�o�b���z�ѡG",
		     currutmp->chatid, sizeof(currutmp->chatid), DOECHO))
	    currutmp->destuid = 0;
    do{
	move(b_lines - 2, 0);
	clrtoeol();
	sprintf(buf, "(��w�ù�)�o�b��]: %s", (currutmp->destuid != 6) ?
		IdleTypeTable[currutmp->destuid] : currutmp->chatid);
	outs(buf);
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

int t_qchicken(void)
{
    char uident[STRLEN];

    stand_title("�d���d��");
    usercomplete(msg_uid, uident);
    if (uident[0])
	chicken_query(uident);
    return 0;
}

int t_query(void)
{
    char uident[STRLEN];

    stand_title("�d�ߺ���");
    usercomplete(msg_uid, uident);
    if (uident[0])
	my_query(uident);
    return 0;
}

int t_talk() {
    char uident[16];
    int tuid, unum, ucount;
    userinfo_t *uentp;
    char genbuf[4];
/*
    if (count_ulist() <= 1){
	outs("�ثe�u�W�u���z�@�H�A���ܽЪB�ͨӥ��{�i" BBSNAME "�j�a�I");
	return XEASY;
    }
*/
    stand_title("���}�ܧX�l");
    generalnamecomplete(msg_uid, uident, sizeof(uident),
			utmpshm->number,
			completeutmp_compar,
			completeutmp_permission,
			completeutmp_getname);
    if (uident[0] == '\0')
	return 0;

    move(3, 0);
    if (!(tuid = searchuser(uident)) || tuid == usernum){
	outs(err_uid);
	pressanykey();
	return 0;
    }

/* multi-login check */
    unum = 1;
    while ((ucount = count_logins(tuid, 0)) > 1){
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
	my_talk(uentp, friend_stat(currutmp, uentp));

    return 0;
}

/* ���H�Ӧ���l�F�A�^���I�s�� */
static userinfo_t *uip;
void talkreply(void)
{
    struct hostent *h;
    char buf[4];
    struct sockaddr_in sin;
    char genbuf[200];
    int a, sig = currutmp->sig;

    talkrequest = NA;
    uip = &utmpshm->uinfo[currutmp->destuip];
    sprintf(page_requestor, "%s (%s)", uip->userid, uip->username);
    currutmp->destuid = uip->uid;
    currstat = XMODE;		/* �קK�X�{�ʵe */

    clear();

    prints("\n\n");
    prints("       (Y) ���ڭ� %s �a�I"
	   "     (A) �ڲ{�b�ܦ��A�е��@�|��A call ��\n", sig_des[sig]);
    prints("       (N) �ڲ{�b���Q %s"
	   "      (B) �藍�_�A�ڦ��Ʊ������A %s\n",
	   sig_des[sig], sig_des[sig]);
    prints("       (C) �Ф��n�n�ڦn�ܡH"
	   "     (D) �ڭn�����o..�U���A��a.......\n");
    prints("       (E) ���ƶܡH�Х��ӫH"
	   "     (F) \033[1;33m�ڦۤv��J�z�Ѧn�F...\033[m\n");
    prints("       (1) %s�H����100�Ȩ��"
	   "  (2) %s�H����1000�Ȩ��..\n\n", sig_des[sig], sig_des[sig]);

    getuser(uip->userid);
    currutmp->msgs[0].pid = uip->pid;
    strcpy(currutmp->msgs[0].userid, uip->userid);
    strcpy(currutmp->msgs[0].last_call_in, "�I�s�B�I�s�Ať��Ц^�� (Ctrl-R)");
    prints("���Ӧ� [%s]�A�@�W�� %d ���A�峹 %d �g\n",
	   uip->from, xuser.numlogins, xuser.numposts);
    showplans(uip->userid);
    show_call_in(0, 0);

    sprintf(genbuf, "�A�Q�� %s %s�ڡH�п��(Y/N/A/B/C/D/E/F/1/2)[N] ",
	    page_requestor, sig_des[sig]);
    getdata(0, 0, genbuf, buf, sizeof(buf), LCECHO);

    if (uip->mode != PAGE){
	sprintf(genbuf, "%s�w����I�s�A��Enter�~��...", page_requestor);
	getdata(0, 0, genbuf, buf, sizeof(buf), LCECHO);
	return;
    }
    currutmp->msgcount = 0;
    strcpy(save_page_requestor, page_requestor);
    memset(page_requestor, 0, sizeof(page_requestor));
    if (!(h = gethostbyname("localhost"))){
	perror("gethostbyname");
	return;
    }
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = h->h_addrtype;
    memcpy(&sin.sin_addr, h->h_addr, h->h_length);
    sin.sin_port = uip->sockaddr;
    a = socket(sin.sin_family, SOCK_STREAM, 0);
    if ((connect(a, (struct sockaddr *) &sin, sizeof(sin)))){
	perror("connect err");
	return;
    }
    if (!buf[0] || !strchr("yabcdef12", buf[0]))
	buf[0] = 'n';
    write(a, buf, 1);
    if (buf[0] == 'f' || buf[0] == 'F'){
	if (!getdata(b_lines, 0, "���઺��]�G", genbuf, 60, DOECHO))
	    strcpy(genbuf, "���i�D�A�� !! ^o^");
	write(a, genbuf, 60);
    }
    uip->destuip = currutmp - &utmpshm->uinfo[0];
    if (buf[0] == 'y')
	switch (sig){
	case SIG_DARK:
	    main_dark(a, uip);
	    break;
	case SIG_PK:
	    chickenpk(a);
	    break;
	case SIG_GOMO:
	    gomoku(a);
	    break;
	case SIG_CHC:
	    chc(a);
	    break;
	case SIG_TALK:
	default:
	    do_talk(a);
	}
    else
	close(a);
    clear();
}

/* ���ͰʺA²�� */
/* not used
static int shortulist(userinfo_t * uentp)
{
    static int lineno, fullactive, linecnt;
    static int moreactive, page, num;
    char uentry[50];
    int state;

    if (!lineno){
	lineno = 3;
	page = moreactive ? (page + p_lines * 3) : 0;
	linecnt = num = moreactive = 0;
	move(1, 70);
	prints("Page: %d", page / (p_lines) / 3 + 1);
	move(lineno, 0);
    }

    if (uentp == NULL){
	int finaltally;

	clrtoeol();
	move(++lineno, 0);
	clrtobot();
	finaltally = fullactive;
	lineno = fullactive = 0;
	return finaltally;
    }

    if ((!HAS_PERM(PERM_SYSOP) &&
	 !HAS_PERM(PERM_SEECLOAK) &&
	 uentp->invisible) ||
	((friend_stat(currutmp, uentp) & HRM) &&
	 !HAS_PERM(PERM_SYSOP))){
	if (lineno >= b_lines)
	    return 0;
	if (num++ < page)
	    return 0;
	memset(uentry, ' ', 25);
	uentry[25] = '\0';
    }
    else{
	fullactive++;
	if (lineno >= b_lines){
	    moreactive = 1;
	    return 0;
	}
	if (num++ < page)
	    return 0;

        state = (currutmp == uentp) ? 10 :
                     (friend_stat(currutmp,uentp)&ST_FRIEND)>>2;

	if (PERM_HIDE(uentp))
	    state = 9;

	sprintf(uentry, "%s%-13s%c%-10s%s ", fcolor[state],
		uentp->userid, uentp->invisible ? '#' : ' ',
		modestring(uentp, 1), state ? "\033[0m" : "");
    }
    if (++linecnt < 3){
	strcat(uentry, "�x");
	outs(uentry);
    }
    else{
	outs(uentry);
	linecnt = 0;
	clrtoeol();
	move(++lineno, 0);
    }
    return 0;
}
*/
