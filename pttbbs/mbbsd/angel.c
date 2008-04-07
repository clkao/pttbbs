/* $Id$ */
#include "bbs.h"

// PTT-BBS Angel System

#ifdef PLAY_ANGEL

#define FN_ANGELMSG "angelmsg"

void 
angel_toggle_pause()
{
    if (!HasUserPerm(PERM_ANGEL) || !currutmp)
	return;
    currutmp->angelpause ++;
    currutmp->angelpause %= ANGELPAUSE_MODES;

    // maintain deprecated value
    cuser.uflag2 &= ~UF2_ANGEL_OLDMASK;
}

void
angel_parse_nick_fp(FILE *fp, char *nick, int sznick)
{
    char buf[PATHLEN];
    // should be in first line
    rewind(fp);
    *buf = 0;
    if (fgets(buf, sizeof(buf), fp))
    {
	// verify first line
	if (buf[0] == '%' && buf[1] == '%' && buf[2] == '[')
	{
	    chomp(buf+3);
	    strlcpy(nick, buf+3, sznick);
	}
    }
}

void
angel_load_my_fullnick(char *buf, int szbuf)
{
    char fn[PATHLEN];
    FILE *fp = NULL;

    *buf = 0;
    setuserfile(fn, FN_ANGELMSG);
    if ((fp = fopen(fn, "rt")))
    {
	angel_parse_nick_fp(fp, buf, szbuf);
	fclose(fp);
    }
    strlcat(buf, "�p�Ѩ�", szbuf);
}

// cache my angel's nickname
static char _myangel[IDLEN+1] = "",
	    _myangel_nick[IDLEN+1] = "";
static time4_t _myangel_touched = 0;
static char _valid_angelmsg = 0;

void 
angel_reload_nick()
{
    char reload = 0;
    char fn[PATHLEN];
    time4_t ts = 0;
    FILE *fp = NULL;

    fn[0] = 0;
    // see if we have angel id change (reload whole)
    if (strcmp(_myangel, cuser.myangel) != 0)
    {
	strlcpy(_myangel, cuser.myangel, sizeof(_myangel));
	reload = 1;
    }
    // see if we need to check file touch date
    if (!reload && _myangel[0] && _myangel[0] != '-')
    {
	sethomefile(fn, _myangel, FN_ANGELMSG);
	ts = dasht(fn);
	if (ts != -1 && ts > _myangel_touched)
	    reload = 1;
    }
    // if no need to reload, reuse current data.
    if (!reload)
    {
	// vmsg("angel_data: no need to reload.");
	return;
    }

    // reset cache
    _myangel_touched = ts;
    _myangel_nick[0] = 0;
    _valid_angelmsg = 0;

    // quick check
    if (_myangel[0] == '-' || !_myangel[0])
	return;

    // do reload data.
    if (!fn[0])
    {
	sethomefile(fn, _myangel, FN_ANGELMSG);
	ts = dasht(fn);
	_myangel_touched = ts;
    }

    assert(*fn);
    // complex load
    fp = fopen(fn, "rt");
    if (fp)
    {
	_valid_angelmsg = 1;
	angel_parse_nick_fp(fp, _myangel_nick, sizeof(_myangel_nick));
	fclose(fp);
    }
}

const char * 
angel_get_nick()
{
    angel_reload_nick();
    return _myangel_nick;
}

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

int 
t_angelmsg(){
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
		prints("�즳�ʺ١G%s�p�Ѩ�", nick);
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

    getdata_buf(11, 0, "�p�Ѩϼʺ١G", nick, 7, 1);
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
	if (!fp)
	    return 0;
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

inline int
angel_reject_me(userinfo_t * uin){
    int* iter = uin->reject;
    int unum;
    while ((unum = *iter++)) {
	if (unum == currutmp->uid) {
	    return 1;
	}
    }
    return 0;
}


static int
FindAngel(void){
    int nAngel;
    int i, j;
    int choose;
    int trial = 0;
    userinfo_t *u;

    do{
	nAngel = 0;
	// since we have many, many angels now, let's ignore angels in angelpause state.
	j = SHM->currsorted;
	u = NULL;
	for (i = 0; i < SHM->UTMPnumber; ++i)
	{
	    u = &SHM->uinfo[SHM->sorted[j][0][i]];
	    if ((u->userlevel & PERM_ANGEL) && (!u->angelpause) && (u->mode != DEBUGSLEEPING))
		++nAngel;
	}

	if (nAngel == 0)
	    return 0;

	choose = random() % nAngel + 1;
	j = SHM->currsorted;
	for (i = 0; i < SHM->UTMPnumber && choose; ++i)
	{
	    u = &SHM->uinfo[SHM->sorted[j][0][i]];
	    if ((u->userlevel & PERM_ANGEL) && (!u->angelpause) && (u->mode != DEBUGSLEEPING))
		--choose;
	}

	// u should be correct now! No need to check angelpause in this time.
	// u = &(SHM->uinfo[SHM->sorted[j][0][i-1]]);
	if (choose == 0 && u &&
		(u->uid != currutmp->uid) &&
		(u->userlevel & PERM_ANGEL) &&
		!angel_reject_me(u) &&
		u->userid[0]){
	    strlcpy(cuser.myangel, u->userid, sizeof(cuser.myangel));
	    passwd_update(usernum, &cuser);
	    return 1;
	}
    }while(++trial < 5);
    return 0;
}

static inline void
GotoNewHand(){
    char old_board[IDLEN + 1] = "";
    int canRead = 1;

    if (currutmp && currutmp->mode == EDITING)
	return;

    // usually crashed as 'assert(currbid == brc_currbid)'
    if (currboard[0]) {
	strlcpy(old_board, currboard, IDLEN + 1);
	currboard = ""; // force enter_board
    }

    if (enter_board(BN_NEWBIE) == 0)
	canRead = 1;

    if (canRead)
	Read();

    if (canRead && old_board[0])
	enter_board(old_board);
}


static inline void
NoAngelFound(const char* msg){
    // don't worry about the screen - 
    // it should have been backuped before entering here.
    
    grayout(0, b_lines-3, GRAYOUT_DARK);
    move(b_lines-4, 0); clrtobot();
    outs(msg_seperator);
    move(b_lines-2, 0);
    if (!msg)
	msg = "�A���p�Ѩϲ{�b���b�u�W";
    outs(msg);
    if (currutmp == NULL || currutmp->mode != EDITING)
	outs("�A�Х��b�s��O�W�M�䵪�שΫ� Ctrl-P �o��");
    if (vmsg("�Ы����N���~��A�Y�Q�����i�J�s��O�o��Ы� 'y'") == 'y')
	GotoNewHand();
}

static inline void
AngelNotOnline(){
    char buf[PATHLEN];
    FILE *fp;

    // use cached angel data (assume already called before.)
    // angel_reload_nick();
    if (!_valid_angelmsg)
    {
	NoAngelFound(NULL);
	return;
    }

    // valid angelmsg is ready for being loaded.
    sethomefile(buf, cuser.myangel, FN_ANGELMSG);
    fp = fopen(buf, "rt");
    if (!fp)
    {
	// safer
	NoAngelFound(NULL);
	return;
    }
    clear();
    showtitle("�p�Ѩϯd��", BBSNAME);
    move(4, 0);
    buf[0] = 0;
    prints("�z��%s�p�Ѩϲ{�b���b�u�W", _myangel_nick);

    outs("\n͢�d�����A�G\n");
    outs(ANSI_COLOR(1;31;44) "��s�w�w�w�w�w�w�w�w�w�w�w�w�w�w�t" ANSI_COLOR(37) ""
	    "�p�Ѩϯd��" ANSI_COLOR(31) "�u�w�w�w�w�w�w�w�w�w�w�w�w�w�w�s��" ANSI_RESET "\n");
    outs(ANSI_COLOR(1;31) "�~�t" ANSI_COLOR(32) " �p�Ѩ�                          "
	    "                                     " ANSI_COLOR(31) "�u��" ANSI_RESET "\n");
    fgets(buf, sizeof(buf), fp); // skip first line: entry for nick
    while (fgets(buf, sizeof(buf), fp))
    {
	chomp(buf);
	prints(ANSI_COLOR(1;31) "�x" ANSI_RESET "%-74.74s" ANSI_COLOR(1;31) "�x" ANSI_RESET "\n", buf);
    }
    fclose(fp);
    outs(ANSI_COLOR(1;31) "���s�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w"
	    "�w�w�w�w�w�w�w�w�w�w�w�w�w�s��" ANSI_RESET "\n");
    outs(ANSI_COLOR(1;31;44) "��r�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w"
	    "�w�w�w�w�w�w�w�w�w�w�w�w�w�w�r��" ANSI_RESET "\n");
    prints("%55s%s", "�d�����: ", Cdatelite(&_myangel_touched));


    move(b_lines - 4, 0);
    outs("�p�D�H�ϥΤW���D�䤣��p�ѨϽШ�s�⪩(" BN_NEWBIE ")\n"
	    "              �Q�d�����p�ѨϽШ�\\�@��(AngelPray)\n"
	    "                  �Q��ݪO�b�����ܥi��(AskBoard)\n"
	    "�Х��b�U�O�W�M�䵪�שΫ� Ctrl-P �o��");

    // Query if user wants to go to newbie board
    if (vmsg("�Ы����N���~��A�Y�Q�����i�J�s��O�o��Ы� 'y'") == 'y')
	GotoNewHand();
}

static void
TalkToAngel(){
    static char AngelPermChecked = 0;
    static userinfo_t* lastuent = NULL;
    userinfo_t *uent;

    if (strcmp(cuser.myangel, "-") == 0){
	NoAngelFound(NULL);
	return;
    }

    if (cuser.myangel[0] && !AngelPermChecked) {
	userec_t xuser;
	memset(&xuser, 0, sizeof(xuser));
	getuser(cuser.myangel, &xuser); // XXX if user doesn't exist
	if (!(xuser.userlevel & PERM_ANGEL))
	    cuser.myangel[0] = 0;
    }
    AngelPermChecked = 1;

    if (cuser.myangel[0] == 0 && !FindAngel()){
	lastuent = NULL;
	NoAngelFound("�{�b�S���p�ѨϦb�u�W");
	return;
    }

    // now try to load angel data.
    // This MUST be done before calling AngelNotOnline,
    // because it relies on this data.
    angel_reload_nick();

    uent = search_ulist_userid(cuser.myangel);
    if (uent == NULL || angel_reject_me(uent) || uent->mode == DEBUGSLEEPING){
	lastuent = NULL;
	AngelNotOnline();
	return;
    }

    // check angelpause: if talked then should accept.
    if (uent == lastuent) {
	// we've talked to angel.
	// XXX what if uentp reused by other? chance very, very low... 
	if (uent->angelpause >= ANGELPAUSE_REJALL)
	{
	    AngelNotOnline();
	    return;
	}
    } else {
	if (uent->angelpause) {
	    // lastuent = NULL;
	    AngelNotOnline();
	    return;
	}
    }

    more("etc/angel_usage", NA);

    /* �o�q�ܩγ\�i�H�b�p�ѨϦ^�����D�� show �X��
    move(b_lines - 1, 0);
    outs("�{�b�A��id����O�K�A�^���A���D���p�ѨϨä����D�A�O��       \n"
         "�A�i�H��ܤ��V���z�S�ۤv�����ӫO�@�ۤv                   ");
	 */

    {
	char xnick[IDLEN+1], prompt[IDLEN*2];
	snprintf(xnick, sizeof(xnick), "%s�p�Ѩ�", _myangel_nick);
	snprintf(prompt, sizeof(prompt), "��%s�p�Ѩ�: ", _myangel_nick);
	// if success, record uent.
	if (my_write(uent->pid, prompt, xnick, WATERBALL_ANGEL, uent))
	    lastuent = uent;
    }
    return;
}

void
CallAngel(){
    static int      entered = 0;
    screen_backup_t old_screen;

    if (!HasUserPerm(PERM_LOGINOK) || entered)
	return;
    entered = 1;

    scr_dump(&old_screen);

    TalkToAngel();

    scr_restore(&old_screen);

    entered = 0;
}

#endif // PLAY_ANGEL
