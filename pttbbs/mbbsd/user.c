/* $Id$ */
#include "bbs.h"
static char    * const sex[8] = {
    MSG_BIG_BOY, MSG_BIG_GIRL, MSG_LITTLE_BOY, MSG_LITTLE_GIRL,
    MSG_MAN, MSG_WOMAN, MSG_PLANT, MSG_MIME
};

#ifdef CHESSCOUNTRY
static const char * const chess_photo_name[3] = {
    "photo_fivechess", "photo_cchess", "photo_go",
};

static const char * const chess_type[3] = {
    "���l��", "�H��", "���",
};
#endif

int
kill_user(int num, const char *userid)
{
    userec_t u;
    char src[256], dst[256];

    if(!userid || num<=0 ) return -1;
    sethomepath(src, userid);
    snprintf(dst, sizeof(dst), "tmp/%s", userid);
    friend_delete_all(userid, FRIEND_ALOHA);
    delete_allpost(userid);
    if (dashd(src) && Rename(src, dst) == 0) {
	snprintf(src, sizeof(src), "/bin/rm -fr home/%c/%s >/dev/null 2>&1", userid[0], userid);
	system(src);
    }

    memset(&u, 0, sizeof(userec_t));
    log_usies("KILL", getuserid(num));
    setuserid(num, "");
    passwd_update(num, &u);
    return 0;
}
int
u_loginview(void)
{
    int             i;
    unsigned int    pbits = cuser.loginview;

    clear();
    move(4, 0);
    for (i = 0; i < NUMVIEWFILE; i++)
	prints("    %c. %-20s %-15s \n", 'A' + i,
	       loginview_file[i][1], ((pbits >> i) & 1 ? "��" : "��"));

    clrtobot();
    while ((i = getkey("�Ы� [A-N] �����]�w�A�� [Return] �����G"))!='\r')
       {
	i = i - 'a';
	if (i >= NUMVIEWFILE || i < 0)
	    bell();
	else {
	    pbits ^= (1 << i);
	    move(i + 4, 28);
	    outs((pbits >> i) & 1 ? "��" : "��");
	}
    }

    if (pbits != cuser.loginview) {
	cuser.loginview = pbits;
	passwd_update(usernum, &cuser);
    }
    return 0;
}
int u_cancelbadpost(void)
{
   int day;
   if(cuser.badpost==0)
     {vmsg("�A�èS���H��."); return 0;}
        
   if(search_ulistn(usernum,2))
     {vmsg("�еn�X��L����, �_�h�����z."); return 0;}

   passwd_query(usernum, &cuser);
   day = 180 - (now - cuser.timeremovebadpost ) / 86400;
   if(day>0 && day<=180)
     {
      vmsgf("�C 180 �Ѥ~��ӽФ@��, �ٳ� %d ��.", day);
      vmsg("�z�]�i�H�`�N����O�_���ҰʪA�Ȥ覡�R���H��.");
      return 0;
     }

   if(
      getkey("���@�N�L�u����W�w,�ճW,�H�ΪO�W[y/N]?")!='y' ||
      getkey("���@�N�L�����[���ڸs,���x�O,�L���U�O�D�v�O[y/N]?")!='y' ||
      getkey("���@�N�ԷV�o���N�q����,����|����,����O�s�i[y/N]?")!='y' )

     {vmsg("�бz��ҲM����A�ӥӽЧR��."); return 0;}

   if(search_ulistn(usernum,2))
     {vmsg("�еn�X��L����, �_�h�����z."); return 0;}
   if(cuser.badpost)
   {
       cuser.badpost--;
       cuser.timeremovebadpost = now;
       passwd_update(usernum, &cuser);
       log_file("log/cancelbadpost.log", LOG_VF|LOG_CREAT,
	        "%s %s �R���@�g�H��\n", Cdate(&now), cuser.userid);
   }
   vmsg("���߱z�w�g���\\�R���@�g�H��.");
   return 0;
}

void
user_display(const userec_t * u, int adminmode)
{
    int             diff = 0;
    char            genbuf[200];

    clrtobot();
    prints(
	   "        " ANSI_COLOR(30;41) "�r�s�r�s�r�s" ANSI_RESET "  " ANSI_COLOR(1;30;45) "    �� �� ��"
	   " �� ��        "
	   "     " ANSI_RESET "  " ANSI_COLOR(30;41) "�r�s�r�s�r�s" ANSI_RESET "\n");
    prints("                �N���ʺ�: %s(%s)\n"
	   "                �u��m�W: %s"
#if FOREIGN_REG_DAY > 0
	   " %s%s"
#elif defined(FOREIGN_REG)
	   " %s"
#endif
	   "\n"
	   "                �~���}: %s\n"
	   "                �q�l�H�c: %s\n"
	   "                ��    �O: %s\n"
	   "                �Ȧ�b��: %d �Ȩ�\n",
	   u->userid, u->nickname, u->realname,
#if FOREIGN_REG_DAY > 0
	   u->uflag2 & FOREIGN ? "(�~�y: " : "",
	   u->uflag2 & FOREIGN ?
		(u->uflag2 & LIVERIGHT) ? "�ä[�~�d)" : "�����o�~�d�v)"
		: "",
#elif defined(FOREIGN_REG)
	   u->uflag2 & FOREIGN ? "(�~�y)" : "",
#endif
	   u->address, u->email,
	   sex[u->sex % 8], u->money);

    sethomedir(genbuf, u->userid);
    prints("                �p�H�H�c: %d ��  (�ʶR�H�c: %d ��)\n"
	   "                ������X: %010d\n"
	   "                ��    ��: %04i/%02i/%02i\n",
	   get_num_records(genbuf, sizeof(fileheader_t)),
	   u->exmailbox, u->mobile,
	   u->year + 1900, u->month, u->day
	   );

#ifdef ASSESS
    prints("                �u �H ��: �u:%d / �H:%d\n",
           u->goodpost, u->badpost);
#endif // ASSESS

    prints("                �W����m: %s\n", u->lasthost);

#ifdef PLAY_ANGEL
    if (adminmode)
	prints("                �p �� ��: %s\n",
		u->myangel[0] ? u->myangel : "�L");
#endif
    prints("                ���U���: %s", ctime4(&u->firstlogin));
    prints("                �e�����{: %s", ctime4(&u->lastlogin));
    prints("                �W���峹: %d �� / %d �g\n",
	   u->numlogins, u->numposts);

#ifdef CHESSCOUNTRY
    {
	int i, j;
	FILE* fp;
	for(i = 0; i < 2; ++i){
	    sethomefile(genbuf, u->userid, chess_photo_name[i]);
	    fp = fopen(genbuf, "r");
	    if(fp != NULL){
		for(j = 0; j < 11; ++j)
		    fgets(genbuf, 200, fp);
		fgets(genbuf, 200, fp);
		prints("%12s�Ѱ�ۧڴy�z: %s", chess_type[i], genbuf + 11);
		fclose(fp);
	    }
	}
    }
#endif

    if (adminmode) {
	strcpy(genbuf, "bTCPRp#@XWBA#VSM0123456789ABCDEF");
	for (diff = 0; diff < 32; diff++)
	    if (!(u->userlevel & (1 << diff)))
		genbuf[diff] = '-';
	prints("                �{�Ҹ��: %s\n"
	       "                user�v��: %s\n",
	       u->justify, genbuf);
    } else {
	diff = (now - login_start_time) / 60;
	prints("                ���d����: %d �p�� %2d ��\n",
	       diff / 60, diff % 60);
    }

    /* Thor: �Q�ݬݳo�� user �O���ǪO���O�D */
    if (u->userlevel >= PERM_BM) {
	int             i;
	boardheader_t  *bhdr;

	outs("                ����O�D: ");

	for (i = 0, bhdr = bcache; i < numboards; i++, bhdr++) {
	    if (is_uBM(bhdr->BM, u->userid)) {
		outs(bhdr->brdname);
		outc(' ');
	    }
	}
	outc('\n');
    }
    outs("        " ANSI_COLOR(30;41) "�r�s�r�s�r�s�r�s�r�s�r�s�r�s�r�s�r�s�r�s�r�s�r"
	 "�s�r�s�r�s�r�s" ANSI_RESET);

    outs((u->userlevel & PERM_LOGINOK) ?
	 "\n�z�����U�{�Ǥw�g�����A�w��[�J����" :
	 "\n�p�G�n���@�v���A�аѦҥ������G���z���U");

#ifdef NEWUSER_LIMIT
    if ((u->lastlogin - u->firstlogin < 3 * 86400) && !HasUserPerm(PERM_POST))
	outs("\n�s��W���A�T�ѫ�}���v��");
#endif
}

void
mail_violatelaw(const char *crime, const char *police, const char *reason, const char *result)
{
    char            genbuf[200];
    fileheader_t    fhdr;
    FILE           *fp;

    sendalert(crime,  ALERT_PWD_PERM);

    sethomepath(genbuf, crime);
    stampfile(genbuf, &fhdr);
    if (!(fp = fopen(genbuf, "w")))
	return;
    fprintf(fp, "�@��: [" BBSMNAME "ĵ�]\n"
	    "���D: [���i] �H�k���i\n"
	    "�ɶ�: %s\n"
	    ANSI_COLOR(1;32) "%s" ANSI_RESET "�P�M�G\n     " ANSI_COLOR(1;32) "%s" ANSI_RESET
	    "�]" ANSI_COLOR(1;35) "%s" ANSI_RESET "�欰�A\n�H�ϥ������W�A�B�H" ANSI_COLOR(1;35) "%s" ANSI_RESET "�A�S���q��"
	    "\n�Ш� " GLOBAL_LAW " �d�߬����k�W��T�A�è� Play-Pay-ViolateLaw ú��@��",
	    ctime4(&now), police, crime, reason, result);
    fclose(fp);
    strcpy(fhdr.title, "[���i] �H�k�P�M���i");
    strcpy(fhdr.owner, "[" BBSMNAME "ĵ�]");
    sethomedir(genbuf, crime);
    append_record(genbuf, &fhdr, sizeof(fhdr));
}

void
kick_all(char *user)
{
   userinfo_t *ui;
   int num = searchuser(user, NULL), i=1;
   while((ui = (userinfo_t *) search_ulistn(num, i))>0)
       {
         if(ui == currutmp) i++;
         if ((ui->pid <= 0 || kill(ui->pid, SIGHUP) == -1))
                         purge_utmp(ui);
         log_usies("KICK ALL", user);
       }
}

void
violate_law(userec_t * u, int unum)
{
    char            ans[4], ans2[4];
    char            reason[128];
    move(1, 0);
    clrtobot();
    move(2, 0);
    outs("(1)Cross-post (2)�õo�s�i�H (3)�õo�s��H\n");
    outs("(4)���Z���W�ϥΪ� (8)��L�H�@��B�m�欰\n(9)�� id �欰\n");
    getdata(5, 0, "(0)����", ans, 3, DOECHO);
    switch (ans[0]) {
    case '1':
	strcpy(reason, "Cross-post");
	break;
    case '2':
	strcpy(reason, "�õo�s�i�H");
	break;
    case '3':
	strcpy(reason, "�õo�s��H");
	break;
    case '4':
	while (!getdata(7, 0, "�п�J�Q���|�z�ѥH�ܭt�d�G", reason, 50, DOECHO));
	strcat(reason, "[���Z���W�ϥΪ�]");
	break;
    case '8':
    case '9':
	while (!getdata(6, 0, "�п�J�z�ѥH�ܭt�d�G", reason, 50, DOECHO));
	break;
    default:
	return;
    }
    getdata(7, 0, msg_sure_ny, ans2, 3, LCECHO);
    if (*ans2 != 'y')
	return;
    if (ans[0] == '9') {
	if (HasUserPerm(PERM_POLICE) && (u->numlogins > 100 || u->numposts > 100))
	    return;

        kill_user(unum, u->userid);
	post_violatelaw(u->userid, cuser.userid, reason, "�尣 ID");
    } else {
        kick_all(u->userid);
	u->userlevel |= PERM_VIOLATELAW;
	u->timeviolatelaw = now;
	u->vl_count++;
	passwd_update(unum, u);
	post_violatelaw(u->userid, cuser.userid, reason, "�@��B��");
	mail_violatelaw(u->userid, "����ĵ��", reason, "�@��B��");
    }
    pressanykey();
}

void Customize(void)
{
    char    done = 0;
    int     dirty = 0;
    int     key;

    /* cuser.uflag settings */
    static const unsigned int masks1[] = {
	MOVIE_FLAG,
	NO_MODMARK_FLAG	,
	COLORED_MODMARK,
#ifdef DBCSAWARE
	DBCSAWARE_FLAG,
#endif
	0,
    };

    static const char* desc1[] = {
	"�ʺA�ݪO",
	"���ä峹�ק�Ÿ�(����/�פ�) (~)",
	"��Φ�m�N���ק�Ÿ� (+)",
#ifdef DBCSAWARE
	"�۰ʰ������줸�r��(�p��������)",
#endif
	0,
    };

    /* cuser.uflag2 settings */
    static const unsigned int masks2[] = {
	REJ_OUTTAMAIL,
	FAVNEW_FLAG,
	FAVNOHILIGHT,
	0,
    };

    static const char* desc2[] = {
	"�ڦ����~�H",
	"�s�O�۰ʶi�ڪ��̷R",
	"�����ܧڪ��̷R",
	0,
    };

    while ( !done ) {
	int i = 0, ia = 0, ic = 0; /* general uflags */
	int iax = 0; /* extended flags */

	clear();
	showtitle("�ӤH�Ƴ]�w", "�ӤH�Ƴ]�w");
	move(2, 0);
	outs("�z�ثe���ӤH�Ƴ]�w: ");
	move(4, 0);

	/* print uflag options */
	for (i = 0; masks1[i]; i++, ia++)
	{
	    clrtoeol();
	    prints( ANSI_COLOR(1;36) "%c" ANSI_RESET
		    ". %-40s%s\n",
		    'a' + ia, desc1[i],
		    (cuser.uflag & masks1[i]) ? 
		    ANSI_COLOR(1;36) "�O" ANSI_RESET : "�_");
	}
	ic = i;
	/* print uflag2 options */
	for (i = 0; masks2[i]; i++, ia++)
	{
	    clrtoeol();
	    prints( ANSI_COLOR(1;36) "%c" ANSI_RESET
		    ". %-40s%s" ANSI_RESET "\n",
		    'a' + ia, desc2[i],
		    (cuser.uflag2 & masks2[i]) ? 
		    ANSI_COLOR(1;36) "�O" ANSI_RESET : "�_");
	}
	/* extended stuff */
	{
	    char mindbuf[5];
	    const static char *wm[] = 
		{"�@��", "�i��", "����", ""};

	    prints("%c. %-40s%s\n",
		    '1' + iax++,
		    "���y�Ҧ�",
		    wm[(cuser.uflag2 & WATER_MASK)]);
	    memcpy(mindbuf, &currutmp->mind, 4);
	    mindbuf[4] = 0;
	    prints("%c. %-40s%s\n",
		    '1' + iax++,
		    "�ثe���߱�",
		    mindbuf);
#ifdef PLAY_ANGEL
	    /* damn it, dirty stuff here */
	    if( HasUserPerm(PERM_ANGEL) )
	    {
		const char *am[4] = 
		{"�k�k�ҥi", "���k��", "���k��", "�Ȥ������s���p�D�H"};
		prints("%c. %-40s%10s\n",
			'1' + iax++,
			"�}��p�D�H�߰�",
			(REJECT_QUESTION ? "�_" : "�O"));
		prints("%c. %-40s%10s\n",
			'1' + iax++,
			"�������p�D�H�ʧO",
			am[ANGEL_STATUS()]);
	    }
#endif
	}

	/* input */
	key = getkey("�Ы� [a-%c,1-%c] �����]�w�A�䥦���N�䵲��: ", 
		'a' + ia-1, '1' + iax -1);

	if (key >= 'a' && key < 'a' + ia)
	{
	    /* normal pref */
	    key -= 'a';
	    dirty = 1;

	    if(key < ic)
	    {
		cuser.uflag ^= masks1[key];
	    } else {
		key -= ic;
		cuser.uflag2 ^= masks2[key];
	    }
	    continue;
	}

	if (key < '1' || key >= '1' + iax)
	{
	    done = 1; continue;
	}
	/* extended keys */
	key -= '1';

	switch(key)
	{
	    case 0: 
		{
		    int     currentset = cuser.uflag2 & WATER_MASK;  
		    currentset = (currentset + 1) % 3;  
		    cuser.uflag2 &= ~WATER_MASK;  
		    cuser.uflag2 |= currentset;  
		    vmsg("�ץ����y�Ҧ���Х��`���u�A���s�W�u");  
		    dirty = 1;
		}
		continue;
	    case 1:
		{
		    char mindbuf[6] = "";
		    getdata(b_lines - 1, 0, "�{�b���߱�? ",  
			    mindbuf, 5, DOECHO);  
		    if (strcmp(mindbuf, "�q�r") == 0)  
			vmsg("���i�H��ۤv�]�q�r��!");  
		    else if (strcmp(mindbuf, "�جP") == 0)  
			vmsg("�A���O���ѥͤ���!");  
		    else  
			memcpy(currutmp->mind, mindbuf, 4);  
		    dirty = 1;
		}
		continue;
	}
#ifdef PLAY_ANGEL
	if( HasUserPerm(PERM_ANGEL) ){
	    if (key == iax-2)
	    {
		SwitchBeingAngel();
		dirty = 1; continue;
	    } 
	    else if (key == iax-1)
	    {
		SwitchAngelSex(ANGEL_STATUS() + 1);
		dirty = 1; continue;
	    }
	}
#endif
	
    }

    if(dirty)
	passwd_update(usernum, &cuser);

    grayout_lines(0, b_lines, 0);
    vmsg("�]�w����");
}

static char *
getregfile(char *buf)
{
    // not in user's home because s/he could zip his/her home
    snprintf(buf, PATHLEN, "jobspool/.regcode.%s", cuser.userid);
    return buf;
}

static char *
makeregcode(char *buf)
{
    char    fpath[PATHLEN];
    int     fd, i;
    const char *alphabet = "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM";

    /* generate a new regcode */
    buf[13] = 0;
    buf[0] = 'v';
    buf[1] = '6';
    for( i = 2 ; i < 13 ; ++i )
	buf[i] = alphabet[random() % 52];

    getregfile(fpath);
    if( (fd = open(fpath, O_WRONLY | O_CREAT, 0600)) == -1 ){
	perror("open");
	exit(1);
    }
    write(fd, buf, 13);
    close(fd);

    return buf;
}

static char *
getregcode(char *buf)
{
    int     fd;
    char    fpath[PATHLEN];

    getregfile(fpath);
    if( (fd = open(fpath, O_RDONLY)) == -1 ){
	buf[0] = 0;
	return buf;
    }
    read(fd, buf, 13);
    close(fd);
    buf[13] = 0;
    return buf;
}

static void
delregcodefile(void)
{
    char    fpath[PATHLEN];
    getregfile(fpath);
    unlink(fpath);
}

#ifdef DEBUG
int
_debug_testregcode()
{
    char buf[16], rcode[16];
    char myid[16];
    int i = 1;

    clear();
    strcpy(myid, cuser.userid);
    do {
	getdata(0, 0, "��J id (�ťյ���): ",
		buf, IDLEN+1, DOECHO);
	if(buf[0])
	{
	    move(i++, 0);
	    i %= t_lines;
	    if(i == 0)
		i = 1;
	    strcpy(cuser.userid, buf);
	    prints("id: [%s], regcode: [%s]\n",
		    cuser.userid, getregcode(rcode));
	    move(i, 0);
	    clrtoeol();
	}
    } while (buf[0]);
    strcpy(cuser.userid, myid);

    pressanykey();
    return 0;
}
#endif

static void
justify_wait(char *userid, char *phone, char *career,
	char *rname, char *addr, char *mobile)
{
    char buf[PATHLEN];
    sethomefile(buf, userid, "justify.wait");
    if (phone[0] != 0) {
	FILE* fn = fopen(buf, "w");
	assert(fn);
	fprintf(fn, "%s\n%s\ndummy\n%s\n%s\n%s\n",
		phone, career, rname, addr, mobile);
	fclose(fn);
    }
}

static void email_justify(const userec_t *muser)
{
	char            tmp[IDLEN + 1], buf[256], genbuf[256];
	/* 
	 * It is intended to use BBSENAME instead of BBSNAME here.
	 * Because recently many poor users with poor mail clients
	 * (or evil mail servers) cannot handle/decode Chinese 
	 * subjects (BBSNAME) correctly, so we'd like to use 
	 * BBSENAME here to prevent subject being messed up.
	 * And please keep BBSENAME short or it may be truncated
	 * by evil mail servers.
	 */
	snprintf(buf, sizeof(buf),
		 " " BBSENAME " - [ %s ]", makeregcode(genbuf));

	strlcpy(tmp, cuser.userid, sizeof(tmp));
	// XXX dirty, set userid=SYSOP
	strlcpy(cuser.userid, str_sysop, sizeof(cuser.userid));
#ifdef HAVEMOBILE
	if (strcmp(muser->email, "m") == 0 || strcmp(muser->email, "M") == 0)
	    mobile_message(mobile, buf);
	else
#endif
	    bsmtp("etc/registermail", buf, muser->email);
	strlcpy(cuser.userid, tmp, sizeof(cuser.userid));
        move(20,0);
        clrtobot();
	outs("�ڭ̧Y�N�H�X�{�ҫH (�z���ӷ|�b 10 ����������)\n"
	     "�����z�i�H�ھڻ{�ҫH���D���{�ҽX\n"
	     "��J�� (U)ser -> (R)egister ��N�i�H�������U");
	pressanykey();
	return;
}

void
uinfo_query(userec_t *u, int adminmode, int unum)
{
    userec_t        x;
    register int    i = 0, fail, mail_changed;
    int             uid, ans;
    char            buf[STRLEN], *p;
    char            genbuf[200], reason[50];
    int money = 0;
    int             flag = 0, temp = 0, money_change = 0;

    fail = mail_changed = 0;

    memcpy(&x, u, sizeof(userec_t));
    ans = getans(adminmode ?
    "(1)����(2)�K�X(3)�v��(4)��b��(5)��ID(6)�d��(7)�f�P(M)�H�c  [0]���� " :
    "�п�� (1)�ק��� (2)�]�w�K�X (M)�ק�H�c (C) �ӤH�Ƴ]�w ==> [0]���� ");

    if (ans > '2' && ans != 'm' && ans != 'c' && !adminmode)
	ans = '0';

    if (ans == '1' || ans == '3' || ans == 'm') {
	clear();
	i = 1;
	move(i++, 0);
	outs(msg_uid);
	outs(x.userid);
    }
    switch (ans) {
    case 'c':
	Customize();
	return;
    case 'm':
	do {
	    getdata_str(i, 0, "�q�l�H�c[�ܰʭn���s�{��]�G", buf, 50, DOECHO,
		    x.email);
	} while (!isvalidemail(buf) && vmsg("�{�ҫH�c����ΨϥΧK�O�H�c"));
	i++;
	if (strcmp(buf, x.email) && strchr(buf, '@')) {
	    strlcpy(x.email, buf, sizeof(x.email));
	    mail_changed = 1 - adminmode;
	}
	break;
    case '7':
	violate_law(&x, unum);
	return;
    case '1':
	move(0, 0);
	outs("�гv���ק�C");

	getdata_buf(i++, 0, " �� ��  �G", x.nickname,
		    sizeof(x.nickname), DOECHO);
	if (adminmode) {
	    getdata_buf(i++, 0, "�u��m�W�G",
			x.realname, sizeof(x.realname), DOECHO);
	    getdata_buf(i++, 0, "�~��a�}�G",
			x.address, sizeof(x.address), DOECHO);
	}
	snprintf(buf, sizeof(buf), "%010d", x.mobile);
	getdata_buf(i++, 0, "������X�G", buf, 11, LCECHO);
	x.mobile = atoi(buf);
	snprintf(genbuf, sizeof(genbuf), "%i", (u->sex + 1) % 8);
	getdata_str(i++, 0, "�ʧO (1)���� (2)�j�� (3)���} (4)���� (5)���� "
		    "(6)���� (7)�Ӫ� (8)�q���G",
		    buf, 3, DOECHO, genbuf);
	if (buf[0] >= '1' && buf[0] <= '8')
	    x.sex = (buf[0] - '1') % 8;
	else
	    x.sex = u->sex % 8;

	while (1) {
	    snprintf(genbuf, sizeof(genbuf), "%04i/%02i/%02i",
		     u->year + 1900, u->month, u->day);
	    if (getdata_str(i, 0, "�ͤ� �褸/���/���G", buf, 11, DOECHO, genbuf) == 0) {
		x.month = u->month;
		x.day = u->day;
		x.year = u->year;
	    } else {
		int y, m, d;
		if (ParseDate(buf, &y, &m, &d))
		    continue;
		x.month = (unsigned char)m;
		x.day = (unsigned char)d;
		x.year = (unsigned char)(y - 1900);
	    }
	    if (!adminmode && x.year < 40)
		continue;
	    i++;
	    break;
	}

#ifdef PLAY_ANGEL
	if (adminmode) {
	    const char* prompt;
	    userec_t the_angel;
	    if (x.myangel[0] == 0 || x.myangel[0] == '-' ||
		    (getuser(x.myangel, &the_angel) &&
		     the_angel.userlevel & PERM_ANGEL))
		prompt = "�p�ѨϡG";
	    else
		prompt = "�p�Ѩϡ]���b���w�L�p�Ѩϸ��^�G";
	    while (1) {
	        userec_t xuser;
		getdata_str(i, 0, prompt, buf, IDLEN + 1, DOECHO,
			x.myangel);
		if(buf[0] == 0 || strcmp(buf, "-") == 0 ||
			(getuser(buf, &xuser) &&
			    (xuser.userlevel & PERM_ANGEL)) ||
			strcmp(x.myangel, buf) == 0){
		    strlcpy(x.myangel, xuser.userid, IDLEN + 1);
		    ++i;
		    break;
		}

		prompt = "�p�ѨϡG";
	    }
	}
#endif

#ifdef CHESSCOUNTRY
	{
	    int j, k;
	    FILE* fp;
	    for(j = 0; j < 2; ++j){
		sethomefile(genbuf, u->userid, chess_photo_name[j]);
		fp = fopen(genbuf, "r");
		if(fp != NULL){
		    FILE* newfp;
		    char mybuf[200];
		    for(k = 0; k < 11; ++k)
			fgets(genbuf, 200, fp);
		    fgets(genbuf, 200, fp);
		    chomp(genbuf);

		    snprintf(mybuf, 200, "%s�Ѱ�ۧڴy�z�G", chess_type[j]);
		    getdata_buf(i, 0, mybuf, genbuf + 11, 80 - 11, DOECHO);
		    ++i;

		    sethomefile(mybuf, u->userid, chess_photo_name[j]);
		    strcat(mybuf, ".new");
		    if((newfp = fopen(mybuf, "w")) != NULL){
			rewind(fp);
			for(k = 0; k < 11; ++k){
			    fgets(mybuf, 200, fp);
			    fputs(mybuf, newfp);
			}
			fputs(genbuf, newfp);
			fputc('\n', newfp);

			fclose(newfp);

			sethomefile(genbuf, u->userid, chess_photo_name[j]);
			sethomefile(mybuf, u->userid, chess_photo_name[j]);
			strcat(mybuf, ".new");
			
			Rename(mybuf, genbuf);
		    }
		    fclose(fp);
		}
	    }
	}
#endif

	if (adminmode) {
	    int l;
	    if (HasUserPerm(PERM_BBSADM)) {
		snprintf(genbuf, sizeof(genbuf), "%d", x.money);
		if (getdata_str(i++, 0, "�Ȧ�b��G", buf, 10, DOECHO, genbuf))
		    if ((l = atol(buf)) != 0) {
			if (l != x.money) {
			    money_change = 1;
			    money = x.money;
			    x.money = l;
			}
		    }
	    }
	    snprintf(genbuf, sizeof(genbuf), "%d", x.exmailbox);
	    if (getdata_str(i++, 0, "�ʶR�H�c�ơG", buf, 6,
			    DOECHO, genbuf))
		if ((l = atol(buf)) != 0)
		    x.exmailbox = (int)l;

	    getdata_buf(i++, 0, "�{�Ҹ�ơG", x.justify,
			sizeof(x.justify), DOECHO);
	    getdata_buf(i++, 0, "�̪���{�����G",
			x.lasthost, sizeof(x.lasthost), DOECHO);

	    // XXX �@�ܼƤ��n�h�γ~�å� "fail"
	    snprintf(genbuf, sizeof(genbuf), "%d", x.numlogins);
	    if (getdata_str(i++, 0, "�W�u���ơG", buf, 10, DOECHO, genbuf))
		if ((fail = atoi(buf)) >= 0)
		    x.numlogins = fail;
	    snprintf(genbuf, sizeof(genbuf), "%d", u->numposts);
	    if (getdata_str(i++, 0, "�峹�ƥءG", buf, 10, DOECHO, genbuf))
		if ((fail = atoi(buf)) >= 0)
		    x.numposts = fail;
	    snprintf(genbuf, sizeof(genbuf), "%d", u->goodpost);
	    if (getdata_str(i++, 0, "�u�}�峹��:", buf, 10, DOECHO, genbuf))
		if ((fail = atoi(buf)) >= 0)
		    x.goodpost = fail;
	    snprintf(genbuf, sizeof(genbuf), "%d", u->badpost);
	    if (getdata_str(i++, 0, "�c�H�峹��:", buf, 10, DOECHO, genbuf))
		if ((fail = atoi(buf)) >= 0)
		    x.badpost = fail;
	    snprintf(genbuf, sizeof(genbuf), "%d", u->vl_count);
	    if (getdata_str(i++, 0, "�H�k�O���G", buf, 10, DOECHO, genbuf))
		if ((fail = atoi(buf)) >= 0)
		    x.vl_count = fail;

	    snprintf(genbuf, sizeof(genbuf),
		     "%d/%d/%d", u->five_win, u->five_lose, u->five_tie);
	    if (getdata_str(i++, 0, "���l�Ѿ��Z ��/��/�M�G", buf, 16, DOECHO,
			    genbuf))
		while (1) {
		    char *strtok_pos;
		    p = strtok_r(buf, "/\r\n", &strtok_pos);
		    if (!p)
			break;
		    x.five_win = atoi(p);
		    p = strtok_r(NULL, "/\r\n", &strtok_pos);
		    if (!p)
			break;
		    x.five_lose = atoi(p);
		    p = strtok_r(NULL, "/\r\n", &strtok_pos);
		    if (!p)
			break;
		    x.five_tie = atoi(p);
		    break;
		}
	    snprintf(genbuf, sizeof(genbuf),
		     "%d/%d/%d", u->chc_win, u->chc_lose, u->chc_tie);
	    if (getdata_str(i++, 0, "�H�Ѿ��Z ��/��/�M�G", buf, 16, DOECHO,
			    genbuf))
		while (1) {
		    char *strtok_pos;
		    p = strtok_r(buf, "/\r\n", &strtok_pos);
		    if (!p)
			break;
		    x.chc_win = atoi(p);
		    p = strtok_r(NULL, "/\r\n", &strtok_pos);
		    if (!p)
			break;
		    x.chc_lose = atoi(p);
		    p = strtok_r(NULL, "/\r\n", &strtok_pos);
		    if (!p)
			break;
		    x.chc_tie = atoi(p);
		    break;
		}
#ifdef FOREIGN_REG
	    if (getdata_str(i++, 0, "��b 1)�x�W 2)��L�G", buf, 2, DOECHO, x.uflag2 & FOREIGN ? "2" : "1"))
		if ((fail = atoi(buf)) > 0){
		    if (fail == 2){
			x.uflag2 |= FOREIGN;
		    }
		    else
			x.uflag2 &= ~FOREIGN;
		}
	    if (x.uflag2 & FOREIGN)
		if (getdata_str(i++, 0, "�ä[�~�d�v 1)�O 2)�_�G", buf, 2, DOECHO, x.uflag2 & LIVERIGHT ? "1" : "2")){
		    if ((fail = atoi(buf)) > 0){
			if (fail == 1){
			    x.uflag2 |= LIVERIGHT;
			    x.userlevel |= (PERM_LOGINOK | PERM_POST);
			}
			else{
			    x.uflag2 &= ~LIVERIGHT;
			    x.userlevel &= ~(PERM_LOGINOK | PERM_POST);
			}
		    }
		}
#endif
	    fail = 0;
	}
	break;

    case '2':
	i = 19;
	if (!adminmode) {
	    if (!getdata(i++, 0, "�п�J��K�X�G", buf, PASSLEN, NOECHO) ||
		!checkpasswd(u->passwd, buf)) {
		outs("\n\n�z��J���K�X�����T\n");
		fail++;
		break;
	    }
	} else {
            FILE *fp;
	    char  witness[3][32], title[100];
	    for (i = 0; i < 3; i++) {
		if (!getdata(19 + i, 0, "�п�J��U�ҩ����ϥΪ̡G",
			     witness[i], sizeof(witness[i]), DOECHO)) {
		    outs("\n����J�h�L�k���\n");
		    fail++;
		    break;
		} else if (!(uid = searchuser(witness[i], NULL))) {
		    outs("\n�d�L���ϥΪ�\n");
		    fail++;
		    break;
		} else {
		    userec_t        atuser;
		    passwd_query(uid, &atuser);
		    if (now - atuser.firstlogin < 6 * 30 * 24 * 60 * 60) {
			outs("\n���U���W�L�b�~�A�Э��s��J\n");
			i--;
		    }
                    strcpy(witness[i], atuser.userid);
			// Adjust upper or lower case
		}
	    }
	    if (i < 3)
		break;

	    sprintf(title, "%s ���K�X���]�q�� (by %s)",u->userid, cuser.userid);
            unlink("etc/updatepwd.log");
            if(! (fp = fopen("etc/updatepwd.log", "w")))
                     break;

            fprintf(fp, "%s �n�D�K�X���]:\n"
                        "���ҤH�� %s, %s, %s",
                         u->userid, witness[0], witness[1], witness[2] );
            fclose(fp);

            post_file(GLOBAL_SECURITY, title, "etc/updatepwd.log", "[�t�Φw����]");
	    mail_id(u->userid, title, "etc/updatepwd.log", cuser.userid);
	    for(i=0; i<3; i++)
	     {
	       mail_id(witness[i], title, "etc/updatepwd.log", cuser.userid);
             }
            i = 20;
	}

	if (!getdata(i++, 0, "�г]�w�s�K�X�G", buf, PASSLEN, NOECHO)) {
	    outs("\n\n�K�X�]�w����, �~��ϥ��±K�X\n");
	    fail++;
	    break;
	}
	strlcpy(genbuf, buf, PASSLEN);

	getdata(i++, 0, "���ˬd�s�K�X�G", buf, PASSLEN, NOECHO);
	if (strncmp(buf, genbuf, PASSLEN)) {
	    outs("\n\n�s�K�X�T�{����, �L�k�]�w�s�K�X\n");
	    fail++;
	    break;
	}
	buf[8] = '\0';
	strlcpy(x.passwd, genpasswd(buf), sizeof(x.passwd));
	break;

    case '3':
	i = setperms(x.userlevel, str_permid);
	if (i == x.userlevel)
	    fail++;
	else {
	    flag = 1;
	    temp = x.userlevel;
	    x.userlevel = i;
	}
	break;

    case '4':
	i = QUIT;
	break;

    case '5':
	if (getdata_str(b_lines - 3, 0, "�s���ϥΪ̥N���G", genbuf, IDLEN + 1,
			DOECHO, x.userid)) {
	    if (searchuser(genbuf, NULL)) {
		outs("���~! �w�g���P�� ID ���ϥΪ�");
		fail++;
	    } else
		strlcpy(x.userid, genbuf, sizeof(x.userid));
	}
	break;
    case '6':
	if (x.mychicken.name[0])
	    x.mychicken.name[0] = 0;
	else
	    strlcpy(x.mychicken.name, "[��]", sizeof(x.mychicken.name));
	break;
    default:
	return;
    }

    if (fail) {
	pressanykey();
	return;
    }
    if (getans(msg_sure_ny) == 'y') {
	if (flag) {
	    post_change_perm(temp, i, cuser.userid, x.userid);
#ifdef PLAY_ANGEL
	    if (i & ~temp & PERM_ANGEL)
		mail_id(x.userid, "�ͻH���X�ӤF�I", "etc/angel_notify", "[�W��]");
#endif
	}
	if (strcmp(u->userid, x.userid)) {
	    char            src[STRLEN], dst[STRLEN];

	    sethomepath(src, u->userid);
	    sethomepath(dst, x.userid);
	    Rename(src, dst);
	    setuserid(unum, x.userid);
	}
	if (mail_changed) {
	    char justify_tmp[REGLEN + 1];
	    char *phone, *career;
	    char *strtok_pos;
	    strlcpy(justify_tmp, u->justify, sizeof(justify_tmp));

	    x.userlevel &= ~(PERM_LOGINOK | PERM_POST);

	    phone  = strtok_r(justify_tmp, ":", &strtok_pos);
	    career = strtok_r(NULL, ":", &strtok_pos);

	    if (phone  == NULL) phone  = "";
	    if (career == NULL) career = "";

	    snprintf(buf, sizeof(buf), "%d", x.mobile);

	    justify_wait(x.userid, phone, career, x.realname, x.address, buf);
            email_justify(&x);
	}
	memcpy(u, &x, sizeof(x));
	if (i == QUIT) {
            kill_user(unum, x.userid);
	    return;
	} else
	    log_usies("SetUser", x.userid);
	if (money_change) {
	    char title[TTLEN+1];
	    char msg[200];
	    clrtobot();
	    clear();
	    while (!getdata(5, 0, "�п�J�z�ѥH�ܭt�d�G",
			    reason, sizeof(reason), DOECHO));

	    snprintf(msg, sizeof(msg),
		    "   ����" ANSI_COLOR(1;32) "%s" ANSI_RESET "��" ANSI_COLOR(1;32) "%s" ANSI_RESET "����"
		    "�q" ANSI_COLOR(1;35) "%d" ANSI_RESET "�令" ANSI_COLOR(1;35) "%d" ANSI_RESET "\n"
		    "   " ANSI_COLOR(1;37) "����%s�ק���z�ѬO�G%s" ANSI_RESET,
		    cuser.userid, x.userid, money, x.money,
		    cuser.userid, reason);
	    snprintf(title, sizeof(title),
		    "[���w���i] ����%s�ק�%s�����i", cuser.userid,
		    x.userid);
	    post_msg(GLOBAL_SECURITY, title, msg, "[�t�Φw����]");
	    setumoney(unum, x.money);
	}
	passwd_update(unum, &x);
	if(flag)
    	  sendalert(x.userid,  ALERT_PWD_PERM); // force to reload perm
    }
}

int
u_info(void)
{
    move(2, 0);
    user_display(&cuser, 0);
    uinfo_query(&cuser, 0, usernum);
    strlcpy(currutmp->nickname, cuser.nickname, sizeof(currutmp->nickname));
    return 0;
}

int
u_cloak(void)
{
    outs((currutmp->invisible ^= 1) ? MSG_CLOAKED : MSG_UNCLOAK);
    return XEASY;
}

void
showplans_userec(userec_t *user)
{
    char            genbuf[200];

    if(user->userlevel & PERM_VIOLATELAW)
    {
	outs(" " ANSI_COLOR(1;31) "���H�H�W �|��ú��@��" ANSI_RESET);
	return;
    }

#ifdef CHESSCOUNTRY
    if (user_query_mode) {
	int    i = 0;
	FILE  *fp;

       sethomefile(genbuf, user->userid, chess_photo_name[user_query_mode - 1]);
	if ((fp = fopen(genbuf, "r")) != NULL)
	{
	    char   photo[6][256];
	    int    kingdom_bid = 0;
	    int    win = 0, lost = 0;

	    move(7, 0);
	    while (i < 12 && fgets(genbuf, 256, fp))
	    {
		chomp(genbuf);
		if (i < 6)  /* Ū�Ӥ��� */
		    strcpy(photo[i], genbuf);
		else if (i == 6)
		    kingdom_bid = atoi(genbuf);
		else
		    prints("%s %s\n", photo[i - 7], genbuf);

		i++;
	    }
	    fclose(fp);

	    if (user_query_mode == 1) {
		win = user->five_win;
		lost = user->five_lose;
	    } else if(user_query_mode == 2) {
		win = user->chc_win;
		lost = user->chc_lose;
	    }
	    prints("%s <�`�@���Z> %d �� %d ��\n", photo[5], win, lost);


	    /* �Ѱ���� */
	    setapath(genbuf, bcache[kingdom_bid - 1].brdname);
	    strlcat(genbuf, "/chess_ensign", sizeof(genbuf));
	    show_file(genbuf, 13, 10, ONLY_COLOR);
	    return;
	}
    }
#endif /* defined(CHESSCOUNTRY) */

    sethomefile(genbuf, user->userid, fn_plans);
    if (!show_file(genbuf, 7, MAX_QUERYLINES, ONLY_COLOR))
	prints("�m�ӤH�W���n%s �ثe�S���W��", user->userid);
}

void
showplans(const char *uid)
{
    userec_t user;
    if(getuser(uid, &user))
       showplans_userec(&user);
}
/*
 * return value: how many items displayed */
int
showsignature(char *fname, int *j, SigInfo *si)
{
    FILE           *fp;
    char            buf[256];
    int             i, lines = scr_lns;
    char            ch;

    clear();
    move(2, 0);
    lines -= 3;

    setuserfile(fname, "sig.0");
    *j = strlen(fname) - 1;
    si->total = 0;
    si->max = 0;

    for (ch = '1'; ch <= '9'; ch++) {
	fname[*j] = ch;
	if ((fp = fopen(fname, "r"))) {
	    si->total ++;
	    si->max = ch - '1';
	    if(lines > 0 && si->max >= si->show_start)
	    {
		prints(ANSI_COLOR(36) "�i ñ�W��.%c �j" ANSI_RESET "\n", ch);
		lines--;
		if(lines > MAX_SIGLINES/2)
		    si->show_max = si->max;
		for (i = 0; lines > 0 && i < MAX_SIGLINES && 
			fgets(buf, sizeof(buf), fp) != NULL; i++)
		    outs(buf), lines--;
	    }
	    fclose(fp);
	}
    }
    if(lines > 0)
	si->show_max = si->max;
    return si->max;
}

int
u_editsig(void)
{
    int             aborted;
    char            ans[4];
    int             j, browsing = 0;
    char            genbuf[MAXPATHLEN];
    SigInfo	    si;

    memset(&si, 0, sizeof(si));

browse_sigs:

    showsignature(genbuf, &j, &si);
    getdata(0, 0, (browsing || (si.max > si.show_max)) ?
	    "ñ�W�� (E)�s�� (D)�R�� (N)½�� (Q)�����H[Q] ":
	    "ñ�W�� (E)�s�� (D)�R�� (Q)�����H[Q] ",
	    ans, sizeof(ans), LCECHO);

    if(ans[0] == 'n')
    {
	si.show_start = si.show_max + 1;
	if(si.show_start > si.max)
	    si.show_start = 0;
	browsing = 1;
	goto browse_sigs;
    }

    aborted = 0;
    if (ans[0] == 'd')
	aborted = 1;
    else if (ans[0] == 'e')
	aborted = 2;

    if (aborted) {
	if (!getdata(1, 0, "�п��ñ�W��(1-9)�H[1] ", ans, sizeof(ans), DOECHO))
	    ans[0] = '1';
	if (ans[0] >= '1' && ans[0] <= '9') {
	    genbuf[j] = ans[0];
	    if (aborted == 1) {
		unlink(genbuf);
		outs(msg_del_ok);
	    } else {
		setutmpmode(EDITSIG);
		aborted = vedit(genbuf, NA, NULL);
		if (aborted != -1)
		    outs("ñ�W�ɧ�s����");
	    }
	}
	pressanykey();
    }
    return 0;
}

int
u_editplan(void)
{
    char            genbuf[200];

    getdata(b_lines - 1, 0, "�W�� (D)�R�� (E)�s�� [Q]�����H[Q] ",
	    genbuf, 3, LCECHO);

    if (genbuf[0] == 'e') {
	int             aborted;

	setutmpmode(EDITPLAN);
	setuserfile(genbuf, fn_plans);
	aborted = vedit(genbuf, NA, NULL);
	if (aborted != -1)
	    outs("�W����s����");
	pressanykey();
	return 0;
    } else if (genbuf[0] == 'd') {
	setuserfile(genbuf, fn_plans);
	unlink(genbuf);
	outmsg("�W���R������");
    }
    return 0;
}

int
u_editcalendar(void)
{
    char            genbuf[200];

    getdata(b_lines - 1, 0, "��ƾ� (D)�R�� (E)�s�� (H)���� [Q]�����H[Q] ",
	    genbuf, 3, LCECHO);

    if (genbuf[0] == 'e') {
	int             aborted;

	setutmpmode(EDITPLAN);
	sethomefile(genbuf, cuser.userid, "calendar");
	aborted = vedit(genbuf, NA, NULL);
	if (aborted != -1)
	    vmsg("��ƾ��s����");
	return 0;
    } else if (genbuf[0] == 'd') {
	sethomefile(genbuf, cuser.userid, "calendar");
	unlink(genbuf);
	vmsg("��ƾ�R������");
    } else if (genbuf[0] == 'h') {
	move(1, 0);
	clrtoline(b_lines);
	move(3, 0);
	prints("��ƾ�榡����:\n�s��ɥH�@�欰���A�p:\n\n# �����}�Y���O����\n2006/05/04 red �W����!\n\n�䤤�� red �O����ܪ��C��C");
	pressanykey();
    }
    return 0;
}

/* �ϥΪ̶�g���U��� */
static void
getfield(int line, const char *info, const char *desc, char *buf, int len)
{
    char            prompt[STRLEN];
    char            genbuf[200];

    move(line, 2);
    prints("����]�w�G%-30.30s (%s)", buf, info);
    snprintf(prompt, sizeof(prompt), "%s�G", desc);
    if (getdata_str(line + 1, 2, prompt, genbuf, len, DOECHO, buf))
	strcpy(buf, genbuf);
    move(line, 2);
    prints("%s�G%s", desc, buf);
    clrtoeol();
}

static int
removespace(char *s)
{
    int             i, index;

    for (i = 0, index = 0; s[i]; i++) {
	if (s[i] != ' ')
	    s[index++] = s[i];
    }
    s[index] = '\0';
    return index;
}



int
isvalidemail(const char *email)
{
    FILE           *fp;
    char            buf[128], *c;
    if (!strstr(email, "@"))
	return 0;
    for (c = strstr(email, "@"); *c != 0; ++c)
	if ('A' <= *c && *c <= 'Z')
	    *c += 32;

    if ((fp = fopen("etc/banemail", "r"))) {
	while (fgets(buf, sizeof(buf), fp)) {
	    if (buf[0] == '#')
		continue;
	    chomp(buf);
	    if (buf[0] == 'A' && strcasecmp(&buf[1], email) == 0)
		return 0;
	    if (buf[0] == 'P' && strcasestr(email, &buf[1]))
		return 0;
	    if (buf[0] == 'S' && strcasecmp(strstr(email, "@") + 1, &buf[1]) == 0)
		return 0;
	}
	fclose(fp);
    }
    return 1;
}

static void
toregister(char *email, char *genbuf, char *phone, char *career,
	   char *rname, char *addr, char *mobile)
{
    FILE           *fn;
    char            buf[128];

    justify_wait(cuser.userid, phone, career, rname, addr, mobile);

    clear();
    stand_title("�{�ҳ]�w");
    if (cuser.userlevel & PERM_NOREGCODE){
	strcpy(email, "x");
	goto REGFORM2;
    }
    move(1, 0);
    outs("�z�n, �����{�һ{�Ҫ��覡��:\n"
	 "  1.�Y�z�� E-Mail  (���������� yahoo, kimo���K�O�� E-Mail)\n"
	 "    �п�J�z�� E-Mail , �ڭ̷|�H�o�t���{�ҽX���H�󵹱z\n"
	 "    �����Ш� (U)ser => (R)egister ��J�{�ҽX, �Y�i�q�L�{��\n"
	 "\n"
	 "  2.�Y�z�S�� E-Mail �άO�@���L�k����{�ҫH, �п�J x \n"
	 "  �|�������˦ۤH�u�f�ֵ��U��ơA" ANSI_COLOR(1;33)
	   "���`�N�o�i��|��W�ƤѩΧ�h�ɶ��C" ANSI_RESET "\n"
	 "**********************************************************\n"
	 "* �`�N!                                                  *\n"
	 "* �q�`���ӷ|�b��J������Q����������{�ҫH, �Y�L�[������ *\n"
	 "* �Ш�l��U�����ˬd�O�_�Q��@�U���H(SPAM)�F�A�t�~�Y�O   *\n"
	 "* ��J��o�ͻ{�ҽX���~�Э���@�� E-Mail                  *\n"
	 "**********************************************************\n");

#ifdef HAVEMOBILE
    outs("  3.�Y�z����������B�Q�Ĩ����²�T�{�Ҫ��覡 , �п�J m \n"
	 "    �ڭ̱N�|�H�o�t���{�ҽX��²�T���z \n"
	 "    �����Ш�(U)ser => (R)egister ��J�{�ҽX, �Y�i�q�L�{��\n");
#endif

    while (1) {
	email[0] = 0;
	getfield(15, "�����{�ҥ�", "E-Mail Address", email, 50);
	if (strcmp(email, "x") == 0 || strcmp(email, "X") == 0)
	    break;

#ifdef HAVEMOBILE
	else if (strcmp(email, "m") == 0 || strcmp(email, "M") == 0) {
	    if (isvalidmobile(mobile)) {
		char            yn[3];
		getdata(16, 0, "�ЦA���T�{�z��J��������X���T��? [y/N]",
			yn, sizeof(yn), LCECHO);
		if (yn[0] == 'Y' || yn[0] == 'y')
		    break;
	    } else {
		move(17, 0);
		outs("���w��������X���X�k,"
		       "�Y�z�L��������п�ܨ�L�覡�{��");
	    }

	}
#endif
	else if (isvalidemail(email)) {
	    char            yn[3];
	    getdata(16, 0, "�ЦA���T�{�z��J�� E-Mail ��m���T��? [y/N]",
		    yn, sizeof(yn), LCECHO);
	    if (yn[0] == 'Y' || yn[0] == 'y')
		break;
	} else {
	    move(17, 0);
	    outs("���w�� E-Mail ���X�k, �Y�z�L E-Mail �п�J x �ѯ�����ʻ{��\n");
	    outs("���`�N��ʻ{�ҳq�`�|��W�ƤѪ��ɶ��C\n");
	}
    }
    strlcpy(cuser.email, email, sizeof(cuser.email));
 REGFORM2:
    if (strcasecmp(email, "x") == 0) {	/* ��ʻ{�� */
	if ((fn = fopen(fn_register, "a"))) {
	    fprintf(fn, "num: %d, %s", usernum, ctime4(&now));
	    fprintf(fn, "uid: %s\n", cuser.userid);
	    fprintf(fn, "name: %s\n", rname);
	    fprintf(fn, "career: %s\n", career);
	    fprintf(fn, "addr: %s\n", addr);
	    fprintf(fn, "phone: %s\n", phone);
	    fprintf(fn, "mobile: %s\n", mobile);
	    fprintf(fn, "email: %s\n", email);
	    fprintf(fn, "----\n");
	    fclose(fn);
	}
    } else {
	if (phone != NULL) {
#ifdef HAVEMOBILE
	    if (strcmp(email, "m") == 0 || strcmp(email, "M") == 0)
		sprintf(genbuf, sizeof(genbuf),
			"%s:%s:<Mobile>", phone, career);
	    else
#endif
		snprintf(genbuf, sizeof(genbuf),
			 "%s:%s:<Email>", phone, career);
	    strlcpy(cuser.justify, genbuf, sizeof(cuser.justify));
	    sethomefile(buf, cuser.userid, "justify");
	}
       email_justify(&cuser);
    }
}

static int HaveRejectStr(const char *s, const char **rej)
{
    int     i;
    char    *ptr, *rejectstr[] =
	{"�F", "��", "��", "�A��", "�Y", "��", "�b", "..", "xx",
	 "�A��", "�ާ�", "�q", "�Ѥ~", "�W�H", 
	 "�t", "�u", "�v", "�w", "�x", "�y", "�z", "�{", "�|", "�}", "�~",
	 "��", "��", "��", "��",/*"��",*/    "��", "��", "��", "��", "��",
	 "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��",
	 "��", "��", "��", "��", "��", NULL};

    if( rej != NULL )
	for( i = 0 ; rej[i] != NULL ; ++i )
	    if( strstr(s, rej[i]) )
		return 1;

    for( i = 0 ; rejectstr[i] != NULL ; ++i )
	if( strstr(s, rejectstr[i]) )
	    return 1;

    if( (ptr = strstr(s, "��")) != NULL ){
	if( ptr != s && strncmp(ptr - 1, "����", 4) == 0 )
	    return 0;
	return 1;
    }
    return 0;
}

static char *isvalidname(char *rname)
{
#ifdef FOREIGN_REG
    return NULL;
#else
    const char    *rejectstr[] =
	{"��", "�D", "���Y", "�p��", "�p��", "���H", "�Ѥ�", "�ѧ�", "�_��",
	 "����", "�ӭ�", "���Y", "�p�n", "�p�j", "���k", "�p�f", "�j�Y", 
	 "���D", "�P��", "�_�_", "���l", "�j�Y", "�p�p", "�p��", "�p�f",
	 "�f�f", "�K", "��", "�ݷ�", "�j��", "�L",
	 NULL};
    if( removespace(rname) && rname[0] < 0 &&
	strlen(rname) >= 4 &&
	!HaveRejectStr(rname, rejectstr) &&
	strncmp(rname, "�p", 2) != 0   && //�_�Y�O�u�p�v
	strncmp(rname, "�ڬO", 4) != 0 && //�_�Y�O�u�ڬO�v
	!(strlen(rname) == 4 && strncmp(&rname[2], "��", 2) == 0) &&
	!(strlen(rname) >= 4 && strncmp(&rname[0], &rname[2], 2) == 0))
	return NULL;
    return "�z����J�����T";
#endif

}

static char *isvalidcareer(char *career)
{
#ifndef FOREIGN_REG
    const char    *rejectstr[] = {NULL};
    if (!(removespace(career) && career[0] < 0 && strlen(career) >= 6) ||
	strcmp(career, "�a��") == 0 || HaveRejectStr(career, rejectstr) )
	return "�z����J�����T";
    if (strcmp(&career[strlen(career) - 2], "�j") == 0 ||
	strcmp(&career[strlen(career) - 4], "�j��") == 0 ||
	strcmp(career, "�ǥͤj��") == 0)
	return "�·нХ[�Ǯըt��";
    if (strcmp(career, "�ǥͰ���") == 0)
	return "�·п�J�ǮզW��";
#else
    if( strlen(career) < 6 )
	return "�z����J�����T";
#endif
    return NULL;
}

static char *isvalidaddr(char *addr)
{
    const char    *rejectstr[] =
	{"�a�y", "�Ȫe", "���P", NULL};

    if (!removespace(addr) || addr[0] > 0 || strlen(addr) < 15) 
	return "�o�Ӧa�}�ä��X�k";
    if (strstr(addr, "�H�c") != NULL || strstr(addr, "�l�F") != NULL) 
	return "��p�ڭ̤������l�F�H�c";
    if ((strstr(addr, "��") == NULL && strstr(addr, "�]") == NULL &&
	 strstr(addr, "��") == NULL && strstr(addr, "��") == NULL) ||
	HaveRejectStr(addr, rejectstr)             ||
	strcmp(&addr[strlen(addr) - 2], "�q") == 0 ||
	strcmp(&addr[strlen(addr) - 2], "��") == 0 ||
	strcmp(&addr[strlen(addr) - 2], "��") == 0 ||
	strcmp(&addr[strlen(addr) - 2], "��") == 0 ||
	strcmp(&addr[strlen(addr) - 2], "��") == 0 ||
	strcmp(&addr[strlen(addr) - 2], "��") == 0 ||
	strcmp(&addr[strlen(addr) - 2], "��") == 0    )
	return "�o�Ӧa�}�ä��X�k";
    return NULL;
}

static char *isvalidphone(char *phone)
{
    int     i;
    for( i = 0 ; phone[i] != 0 ; ++i )
	if( !isdigit((int)phone[i]) )
	    return "�Ф��n�[���j�Ÿ�";
    if (!removespace(phone) || 
	strlen(phone) < 9 || 
	strstr(phone, "00000000") != NULL ||
	strstr(phone, "22222222") != NULL    ) {
	return "�o�ӹq�ܸ��X�ä��X�k(�Чt�ϽX)" ;
    }
    return NULL;
}

int
u_register(void)
{
    char            rname[20], addr[50], mobile[16];
#ifdef FOREIGN_REG
    char            fore[2];
#endif
    char            phone[20], career[40], email[50], birthday[11], sex_is[2];
    unsigned char   year, mon, day;
    char            inregcode[14], regcode[50];
    char            ans[3], *ptr, *errcode;
    char            genbuf[200];
    FILE           *fn;

    if (cuser.userlevel & PERM_LOGINOK) {
	outs("�z�������T�{�w�g�����A���ݶ�g�ӽЪ�");
	return XEASY;
    }
    if ((fn = fopen(fn_register, "r"))) {
	int i =0;
	while (fgets(genbuf, STRLEN, fn)) {
	    if ((ptr = strchr(genbuf, '\n')))
		*ptr = '\0';
	    if (strncmp(genbuf, "uid: ", 5) != 0)
		continue;
	    i++;
	    if(strcmp(genbuf + 5, cuser.userid) != 0)
		continue;
	    fclose(fn);
	    /* idiots complain about this, so bug them */
	    clear();
	    move(3, 0);
	    prints("   �z�����U�ӽг�|�b�B�z��(�B�z����: %d)�A�Э@�ߵ���\n\n", i);
	    outs("   �p�G�z�w������U�X�o�ݨ�o�ӵe���A���N��z�b�ϥ� Email ���U��\n");
	    outs("   " ANSI_COLOR(1;31) "�S�t�~�ӽФF���������H�u�f�֪����U�ӽг�C" 
		    ANSI_RESET "\n\n");
	    // outs("�Ӧ��A�����ݻ�����...\n");
	    outs("   �i�J�H�u�f�ֵ{�ǫ� Email ���U�۰ʥ��ġA�����U�X�]�S�ΡA\n");
	    outs("   �n����f�֧��� (�|�h��ܦh�ɶ��A�q�`�_�X�@��) �A�ҥH�Э@�ߵ��ԡC\n\n");

	    /* �U���O����� code �һݭn�� message */
#if 0
	    outs("   �t�~�Ъ`�N�A�Y�����f���U��ɱz���b���W�h�|�L�k�f�֡B�۰ʸ��L�C\n");
	    outs("   �ҥH���Լf�֮ɽФű����C�Y�W�L��T�Ѥ����Q�f��A�q�`�N�O�o�ӭ�]�C\n");
#endif

	    vmsg("�z�����U�ӽг�|�b�B�z��");
	    return FULLUPDATE;
	}
	fclose(fn);
    }
    strlcpy(rname, cuser.realname, sizeof(rname));
    strlcpy(addr, cuser.address, sizeof(addr));
    strlcpy(email, cuser.email, sizeof(email));
    snprintf(mobile, sizeof(mobile), "0%09d", cuser.mobile);
    if (cuser.month == 0 && cuser.day && cuser.year == 0)
	birthday[0] = 0;
    else
	snprintf(birthday, sizeof(birthday), "%04i/%02i/%02i",
		 1900 + cuser.year, cuser.month, cuser.day);
    sex_is[0] = (cuser.sex % 8) + '1';
    sex_is[1] = 0;
    career[0] = phone[0] = '\0';
    sethomefile(genbuf, cuser.userid, "justify.wait");
    if ((fn = fopen(genbuf, "r"))) {
	fgets(genbuf, sizeof(genbuf), fn);
	chomp(genbuf);
	strlcpy(phone, genbuf, sizeof(phone));

	fgets(genbuf, sizeof(genbuf), fn);
	chomp(genbuf);
	strlcpy(career, genbuf, sizeof(career));

	fgets(genbuf, sizeof(genbuf), fn); // old version compatible

	fgets(genbuf, sizeof(genbuf), fn);
	chomp(genbuf);
	strlcpy(rname, genbuf, sizeof(rname));

	fgets(genbuf, sizeof(genbuf), fn);
	chomp(genbuf);
	strlcpy(addr, genbuf, sizeof(addr));

	fgets(genbuf, sizeof(genbuf), fn);
	chomp(genbuf);
	strlcpy(mobile, genbuf, sizeof(mobile));

	fclose(fn);
    }

    if (cuser.userlevel & PERM_NOREGCODE) {
	vmsg("�z���Q���\\�ϥλ{�ҽX�{�ҡC�ж�g���U�ӽг�");
	goto REGFORM;
    }

    if (cuser.year != 0 &&	/* �w�g�Ĥ@����L�F~ ^^" */
	strcmp(cuser.email, "x") != 0 &&	/* �W����ʻ{�ҥ��� */
	strcmp(cuser.email, "X") != 0) {
	clear();
	stand_title("EMail�{��");
	move(2, 0);
	prints("%s(%s) �z�n�A�п�J�z���{�ҽX�C\n"
	       "�αz�i�H��J x �ӭ��s��g E-Mail �Χ�ѯ�����ʻ{��\n",
	       cuser.userid, cuser.nickname);
	inregcode[0] = 0;

	do{
	    getdata(10, 0, "�z���{�ҽX�G",
		    inregcode, sizeof(inregcode), DOECHO);
	    if( strcmp(inregcode, "x") == 0 || strcmp(inregcode, "X") == 0 )
		break;
	    if( inregcode[0] != 'v' || inregcode[1] != '6' ) {
		/* old regcode */
		vmsg("�z��J���{�ҽX�]�t�Ϊ@�Ťw���ġA"
		     "�п�J x ����@�� E-Mail");
	    } else if( strlen(inregcode) != 13 )
		vmsg("�{�ҽX��J�������A���Ӥ@�@���Q�T�X�C");
	    else
		break;
	} while( 1 );

	if (strcmp(inregcode, getregcode(regcode)) == 0) {
	    int             unum;
	    delregcodefile();
	    if ((unum = searchuser(cuser.userid, NULL)) == 0) {
		vmsg("�t�ο��~�A�d�L���H�I");
		u_exit("getuser error");
		exit(0);
	    }
	    mail_muser(cuser, "[���U���\\�o]", "etc/registeredmail");
#if FOREIGN_REG_DAY > 0
	    if(cuser.uflag2 & FOREIGN)
		mail_muser(cuser, "[�X�J�Һ޲z��]", "etc/foreign_welcome");
#endif
	    cuser.userlevel |= (PERM_LOGINOK | PERM_POST);
	    outs("\n���U���\\, ���s�W����N���o�����v��\n"
		   "�Ы��U���@������᭫�s�W��~ :)");
	    sethomefile(genbuf, cuser.userid, "justify.wait");
	    unlink(genbuf);
	    snprintf(cuser.justify, sizeof(cuser.justify),
		     "%s:%s:email", phone, career);
	    sethomefile(genbuf, cuser.userid, "justify");
	    log_file(genbuf, LOG_CREAT, cuser.justify);
	    pressanykey();
	    u_exit("registed");
	    exit(0);
	    return QUIT;
	} else if (strcmp(inregcode, "x") != 0 &&
		   strcmp(inregcode, "X") != 0) {
	    vmsg("�{�ҽX���~�I");
	} else {
	    toregister(email, genbuf, phone, career, rname, addr, mobile);
	    return FULLUPDATE;
	}
    }

    REGFORM:
    getdata(b_lines - 1, 0, "�z�T�w�n��g���U���(Y/N)�H[N] ",
	    ans, 3, LCECHO);
    if (ans[0] != 'y')
	return FULLUPDATE;

    move(2, 0);
    clrtobot();
    while (1) {
	clear();
	move(1, 0);
	prints("%s(%s) �z�n�A�оڹ��g�H�U�����:",
	       cuser.userid, cuser.nickname);
#ifdef FOREIGN_REG
	fore[0] = 'y';
	fore[1] = 0;
	getfield(2, "Y/n", "�O�_�{�b��b�x�W", fore, 2);
    	if (fore[0] == 'n')
	    fore[0] |= FOREIGN;
	else
	    fore[0] = 0;
#endif
	while (1) {
	    getfield(8, 
#ifdef FOREIGN_REG
                     "�ХΥ��W",
#else
                     "�ХΤ���",
#endif
                     "�u��m�W", rname, 20);
	    if( (errcode = isvalidname(rname)) == NULL )
		break;
	    else
		vmsg(errcode);
	}

	move(11, 0);
	outs("  �кɶq�ԲӪ���g�z���A�ȳ��A�j�M�|�սг·�"
	     "�[" ANSI_COLOR(1;33) "�t��" ANSI_RESET "�A���q���Х[" ANSI_COLOR(1;33) "¾��" ANSI_RESET "�A\n"
	     "  �ȵL�u�@�г·ж�g" ANSI_COLOR(1;33) "���~�Ǯ�" ANSI_RESET "�C\n");
	while (1) {
	    getfield(9, "(���~)�Ǯ�(�t" ANSI_COLOR(1;33) "�t�Ҧ~��" ANSI_RESET ")�γ��¾��",
		     "�A�ȳ��", career, 40);
	    if( (errcode = isvalidcareer(career)) == NULL )
		break;
	    else
		vmsg(errcode);
	}
	while (1) {
	    getfield(11, "�t" ANSI_COLOR(1;33) "����" ANSI_RESET "�Ϊ��츹�X"
		     "(�x�_�Х[" ANSI_COLOR(1;33) "��F��" ANSI_RESET ")",
		     "�ثe��}", addr, sizeof(addr));
	    if( (errcode = isvalidaddr(addr)) == NULL
#ifdef FOREIGN_REG
                || fore[0] 
#endif
		)
		break;
	    else
		vmsg(errcode);
	}
	while (1) {
	    getfield(13, "���[-(), �]�A���~�ϸ�", "�s���q��", phone, 11);
	    if( (errcode = isvalidphone(phone)) == NULL )
		break;
	    else
		vmsg(errcode);
	}
	getfield(15, "�u��J�Ʀr �p:0912345678 (�i����)",
		 "������X", mobile, 20);
	while (1) {
	    getfield(17, "�褸/���/��� �p:1984/02/29", "�ͤ�", birthday, sizeof(birthday));
	    if (birthday[0] == 0) {
		snprintf(birthday, sizeof(birthday), "%04i/%02i/%02i",
			 1900 + cuser.year, cuser.month, cuser.day);
		mon = cuser.month;
		day = cuser.day;
		year = cuser.year;
	    } else {
		int y, m, d;
		if (ParseDate(birthday, &y, &m, &d)) {
		    vmsg("�z����J�����T");
		    continue;
		}
		mon = (unsigned char)m;
		day = (unsigned char)d;
		year = (unsigned char)(y - 1900);
	    }
	    if (year < 40) {
		vmsg("�z����J�����T");
		continue;
	    }
	    break;
	}
	getfield(19, "1.���� 2.�j�� ", "�ʧO", sex_is, 2);
	getdata(20, 0, "�H�W��ƬO�_���T(Y/N)�H(Q)�������U [N] ",
		ans, 3, LCECHO);
	if (ans[0] == 'q')
	    return 0;
	if (ans[0] == 'y')
	    break;
    }
    strlcpy(cuser.realname, rname, sizeof(cuser.realname));
    strlcpy(cuser.address, addr, sizeof(cuser.address));
    strlcpy(cuser.email, email, sizeof(cuser.email));
    cuser.mobile = atoi(mobile);
    cuser.sex = (sex_is[0] - '1') % 8;
    cuser.month = mon;
    cuser.day = day;
    cuser.year = year;
#ifdef FOREIGN_REG
    if (fore[0])
	cuser.uflag2 |= FOREIGN;
    else
	cuser.uflag2 &= ~FOREIGN;
#endif
    trim(career);
    trim(addr);
    trim(phone);

    toregister(email, genbuf, phone, career, rname, addr, mobile);

    return FULLUPDATE;
}

/* �C�X�Ҧ����U�ϥΪ� */
static int      usercounter, totalusers;
static unsigned short u_list_special;

static int
u_list_CB(int num, userec_t * uentp)
{
    static int      i;
    char            permstr[8], *ptr;
    register int    level;

    if (uentp == NULL) {
	move(2, 0);
	clrtoeol();
	prints(ANSI_COLOR(7) "  �ϥΪ̥N��   %-25s   �W��  �峹  %s  "
	       "�̪���{���     " ANSI_COLOR(0) "\n",
	       "�︹�ʺ�",
	       HasUserPerm(PERM_SEEULEVELS) ? "����" : "");
	i = 3;
	return 0;
    }
    if (bad_user_id(uentp->userid))
	return 0;

    if ((uentp->userlevel & ~(u_list_special)) == 0)
	return 0;

    if (i == b_lines) {
	prints(ANSI_COLOR(34;46) "  �w��� %d/%d �H(%d%%)  " ANSI_COLOR(31;47) "  "
	       "(Space)" ANSI_COLOR(30) " �ݤU�@��  " ANSI_COLOR(31) "(Q)" ANSI_COLOR(30) " ���}  " ANSI_RESET,
	       usercounter, totalusers, usercounter * 100 / totalusers);
	i = igetch();
	if (i == 'q' || i == 'Q')
	    return QUIT;
	i = 3;
    }
    if (i == 3) {
	move(3, 0);
	clrtobot();
    }
    level = uentp->userlevel;
    strlcpy(permstr, "----", 8);
    if (level & PERM_SYSOP)
	permstr[0] = 'S';
    else if (level & PERM_ACCOUNTS)
	permstr[0] = 'A';
    else if (level & PERM_SYSOPHIDE)
	permstr[0] = 'p';

    if (level & (PERM_BOARD))
	permstr[1] = 'B';
    else if (level & (PERM_BM))
	permstr[1] = 'b';

    if (level & (PERM_XEMPT))
	permstr[2] = 'X';
    else if (level & (PERM_LOGINOK))
	permstr[2] = 'R';

    if (level & (PERM_CLOAK | PERM_SEECLOAK))
	permstr[3] = 'C';

    ptr = (char *)Cdate(&uentp->lastlogin);
    ptr[18] = '\0';
    prints("%-14s %-27.27s%5d %5d  %s  %s\n",
	   uentp->userid,
	   uentp->nickname,
	   uentp->numlogins, uentp->numposts,
	   HasUserPerm(PERM_SEEULEVELS) ? permstr : "", ptr);
    usercounter++;
    i++;
    return 0;
}

int
u_list(void)
{
    char            genbuf[3];

    setutmpmode(LAUSERS);
    u_list_special = usercounter = 0;
    totalusers = SHM->number;
    if (HasUserPerm(PERM_SEEULEVELS)) {
	getdata(b_lines - 1, 0, "�[�� [1]�S���� (2)�����H",
		genbuf, 3, DOECHO);
	if (genbuf[0] != '2')
	    u_list_special = PERM_BASIC | PERM_CHAT | PERM_PAGE | PERM_POST | PERM_LOGINOK | PERM_BM;
    }
    u_list_CB(0, NULL);
    if (passwd_apply(u_list_CB) == -1) {
	outs(msg_nobody);
	return XEASY;
    }
    move(b_lines, 0);
    clrtoeol();
    prints(ANSI_COLOR(34;46) "  �w��� %d/%d ���ϥΪ�(�t�ήe�q�L�W��)  "
	   ANSI_COLOR(31;47) "  (�Ы����N���~��)  " ANSI_RESET, usercounter, totalusers);
    igetch();
    return 0;
}

#ifdef DBCSAWARE

/* detect if user is using an evil client that sends double
 * keys for DBCS data.
 * True if client is evil.
 */

int u_detectDBCSAwareEvilClient()
{
    int ret = 0;

    clear();
    move(1, 0);
    outs(ANSI_RESET
	    "* �����䴩�۰ʰ�������r�����ʻP�s��A�����ǳs�u�{��(�pxxMan)\n"
	    "  �|�ۦ�B�z�B�h�e����A��O�K�|�y��" ANSI_COLOR(1;37)
	    "�@�����ʨ�Ӥ���r���{�H�C" ANSI_RESET "\n\n"
	    "* ���s�u�{���B�z���ʮe���y���\\�h"
	    "��ܤβ��ʤW�����D�A�ҥH�ڭ̫�ĳ�z\n"
	    "  �����ӵ{���W�������]�w�]�q�`�s�u����(���������줸��)����v�^�A\n"
	    "  �� BBS �t�Υi�H���T������A���e���C\n\n"
	    ANSI_COLOR(1;33) 
	    "* �p�G�z�ݤ����W���������]�L�ҿסA�ڭ̷|�۰ʰ����A�X�z���]�w�C"
	    ANSI_RESET "\n"
	    "  �Цb�]�w�n�s�u�{�����z���n���Ҧ����" ANSI_COLOR(1;33)
	    "�@�U" ANSI_RESET "�z��L�W��" ANSI_COLOR(1;33)
	    "��" ANSI_RESET "\n" ANSI_COLOR(1;36)
	    "  (�t�~���k��V��μg BS/Backspace ���˰h��P Del �R���䧡�i)\n"
	    ANSI_RESET);

    /* clear buffer */
    while(num_in_buf() > 0)
	igetch();

    while (1)
    {
	int ch = 0;

	move(12, 0);
	outs("�o�O�����ϡA�z����з|�X�{�b" 
		ANSI_COLOR(7) "�o��" ANSI_RESET);
	move(12, 15*2);
	ch = igetch();
	if(ch != KEY_LEFT && ch != KEY_RIGHT &&
		ch != Ctrl('H') && ch != '\177')
	{
	    move(14, 0);
	    outs("�Ы��@�U�W�����w����I �A����O����F�I");
	} else {
	    move(16, 0);
	    /* Actually you may also use num_in_buf here.  those clients
	     * usually sends doubled keys together in one packet.
	     * However when I was writing this, a bug (existed for more than 3
	     * years) of num_in_buf forced me to write new wait_input.
	     * Anyway it is fixed now.
	     */
	    if(wait_input(0.1, 1))
	    // if(igetch() == ch)
	    // if (num_in_buf() > 0)
	    {
		/* evil dbcs aware client */
		outs("������z���s�u�{���|�ۦ�B�z��в��ʡC\n\n"
			// "�Y���]���y���s���W�����D���������B�z�C\n\n"
			"�w�]�w���u���z���s�u�{���B�z��в��ʡv\n");
		ret = 1;
	    } else {
		/* good non-dbcs aware client */
		outs("�z���s�u�{�����G���|�h�e����A"
			"�o�� BBS �i�H���Ǫ�����e���C\n\n"
			"�w�]�w���u�� BBS ���A�������B�z��в��ʡv\n");
		ret = 0;
	    }
	    outs(  "\n�Y�Q���ܳ]�w�Ц� �ӤH�]�w�� �� �ӤH�Ƴ]�w �� \n"
		   "    �վ�u�۰ʰ������줸�r��(�p��������)�v���]�w");
	    while(num_in_buf())
		igetch();
	    break;
	}
    }
    pressanykey();
    return ret;
}
#endif 

/* vim:sw=4
 */
