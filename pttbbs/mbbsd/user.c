/* $Id$ */
#include "bbs.h"

#define FN_NOTIN_WHITELIST_NOTICE "etc/whitemail.notice"

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
    int             i, in;
    unsigned int    pbits = cuser.loginview;

    clear();
    move(4, 0);
    for (i = 0; i < NUMVIEWFILE && loginview_file[i][0]; i++)
	prints("    %c. %-20s %-15s \n", 'A' + i,
	       loginview_file[i][1], ((pbits >> i) & 1 ? "��" : "��"));
    in = i;

    clrtobot();
    while ((i = vmsgf("�Ы� [A-%c] �����]�w�A�� [Return] �����G", 
		    'A'+in-1))!='\r')
       {
	i = i - 'a';
	if (i >= in || i < 0)
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
   if (currutmp && (currutmp->alerts & ALERT_PWD))
       currutmp->alerts &= ~ALERT_PWD;

   day = 180 - (now - cuser.timeremovebadpost ) / DAY_SECONDS;
   if(day>0 && day<=180)
     {
      vmsgf("�C 180 �Ѥ~��ӽФ@��, �ٳ� %d ��.", day);
      vmsg("�z�]�i�H�`�N����O�_���ҰʪA�Ȥ覡�R���H��.");
      return 0;
     }

   if(
      vmsg("���@�N�L�u����W�w,�ճW,�H�ΪO�W[y/N]?")!='y' ||
      vmsg("���@�N�L�����[���ڸs,���x�O,�L���U�O�D�v�O[y/N]?")!='y' ||
      vmsg("���@�N�ԷV�o���N�q����,����|����,����O�s�i[y/N]?")!='y' )

     {vmsg("�бz��ҲM����A�ӥӽЧR��."); return 0;}

   if(search_ulistn(usernum,2))
     {vmsg("�еn�X��L����, �_�h�����z."); return 0;}
   if(cuser.badpost)
   {
       int prev = cuser.badpost--;
       cuser.timeremovebadpost = now;
       passwd_update(usernum, &cuser);
       log_filef("log/cancelbadpost.log", LOG_CREAT,
	        "%s %s �R���@�g�H�� (%d -> %d �g)\n", 
		Cdate(&now), cuser.userid, prev, cuser.badpost);
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
    prints("\t\t�N���ʺ�: %s(%s)\n", u->userid, u->nickname);
    prints("\t\t�u��m�W: %s", u->realname);
#if FOREIGN_REG_DAY > 0
    prints(" %s%s",
	   u->uflag2 & FOREIGN ? "(�~�y: " : "",
	   u->uflag2 & FOREIGN ?
		(u->uflag2 & LIVERIGHT) ? "�ä[�~�d)" : "�����o�~�d�v)"
		: "");
#elif defined(FOREIGN_REG)
    prints(" %s", u->uflag2 & FOREIGN ? "(�~�y)" : "");
#endif
    outs("\n"); // end of realname
    prints("\t\t¾�~�Ǿ�: %s\n", u->career);
    prints("\t\t�~��a�}: %s\n", u->address);
    prints("\t\t�q�ܤ��: %s", u->phone);
	if (u->mobile) 
	    prints(" / %010d", u->mobile);
	outs("\n");

    prints("\t\t�q�l�H�c: %s\n", u->email);
    prints("\t\t�Ȧ�b��: %d �Ȩ�\n", u->money);
    prints("\t\t��    �O: %s\n", sex[u->sex%8]);
    prints("\t\t��    ��: %04i/%02i/%02i (%s��18��)\n",
	   u->year + 1900, u->month, u->day, 
	   resolve_over18_user(u) ? "�w" : "��");

    prints("\t\t���U���: %s (�w��%d��)\n", 
	    Cdate(&u->firstlogin), (int)((now - u->firstlogin)/DAY_SECONDS));
    prints("\t\t�W���W��: %s (%s)\n", 
	    u->lasthost, Cdate(&u->lastlogin));

    if (adminmode) {
	strcpy(genbuf, "bTCPRp#@XWBA#VSM0123456789ABCDEF");
	for (diff = 0; diff < 32; diff++)
	    if (!(u->userlevel & (1 << diff)))
		genbuf[diff] = '-';
	prints("\t\t�b���v��: %s\n", genbuf);
	prints("\t\t�{�Ҹ��: %s\n", u->justify);
    }

    prints("\t\t�W���峹: %d �� / %d �g\n",
	   u->numlogins, u->numposts);

    sethomedir(genbuf, u->userid);
    prints("\t\t�p�H�H�c: %d ��  (�ʶR�H�c: %d ��)\n",
	   get_num_records(genbuf, sizeof(fileheader_t)),
	   u->exmailbox);

    if (!adminmode) {
	diff = (now - login_start_time) / 60;
	prints("\t\t���d����: %d �p�� %2d ��\n",
	       diff / 60, diff % 60);
    }

    /* Thor: �Q�ݬݳo�� user �O���ǪO���O�D */
    if (u->userlevel >= PERM_BM) {
	int             i;
	boardheader_t  *bhdr;

	outs("\t\t����O�D: ");

	for (i = 0, bhdr = bcache; i < numboards; i++, bhdr++) {
	    if (is_uBM(bhdr->BM, u->userid)) {
		outs(bhdr->brdname);
		outc(' ');
	    }
	}
	outc('\n');
    }

    // conditional fields
#ifdef ASSESS
    prints("\t\t�u �H ��: �u:%d / �H:%d\n",
           u->goodpost, u->badpost);
#endif // ASSESS

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

#ifdef PLAY_ANGEL
    if (adminmode)
	prints("\t\t�p �� ��: %s\n",
		u->myangel[0] ? u->myangel : "�L");
#endif

    outs("        " ANSI_COLOR(30;41) "�r�s�r�s�r�s�r�s�r�s�r�s�r�s�r�s�r�s�r�s�r�s�r"
	 "�s�r�s�r�s�r�s" ANSI_RESET);

    outs((u->userlevel & PERM_LOGINOK) ?
	 "\n�z�����U�{�Ǥw�g�����A�w��[�J����" :
	 "\n�p�G�n���@�v���A�аѦҥ������G���z���U");

#ifdef NEWUSER_LIMIT
    if ((u->lastlogin - u->firstlogin < 3 * DAY_SECONDS) && !HasUserPerm(PERM_POST))
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
	    "�ɶ�: %s\n", ctime4(&now));
    fprintf(fp,
	    ANSI_COLOR(1;32) "%s" ANSI_RESET "�P�M�G\n     " ANSI_COLOR(1;32) "%s" ANSI_RESET
	    "�]" ANSI_COLOR(1;35) "%s" ANSI_RESET "�欰�A\n"
	    "�H�ϥ������W�A�B�H" ANSI_COLOR(1;35) "%s" ANSI_RESET "�A�S���q��\n\n"
	    "�Ш� " BN_LAW " �d�߬����k�W��T�A�ñq�D���i�J:\n"
	    "(P)lay�i�T�ֻP�𶢡j=>(P)ay�i��tt�q�c�� �j=> (1)ViolateLaw ú�@��\n"
	    "�Hú��@��C\n",
	    police, crime, reason, result);
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
   while((ui = (userinfo_t *) search_ulistn(num, i)) != NULL)
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
	// 20080417, according to the request of admins, the numpost>100
	// if very easy to achive for such bad users (of course, because
	// usually they violate law by massive posting).
	// Also if he wants to prevent system auto cp check, he will
	// post -> logout -> login -> post. So both numlogin and numpost
	// are not good.
	// We changed the rule to registration date [2 month].
	if (HasUserPerm(PERM_POLICE) && ((now - u->firstlogin) >= 2*30*DAY_SECONDS))
	{
	    vmsg("�ϥΪ̵��U�w�W�L 60 �ѡA�L�k�尣�C");
	    return;
	}

        kick_all(u->userid);
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
	DBCS_NOINTRESC,
#endif
	DEFBACKUP_FLAG,
	0,
    };

    static const char* desc1[] = {
	"�ʺA�ݪO",
	"���ä峹�ק�Ÿ�(����/�פ�) (~)",
	"��Φ�m�N���ק�Ÿ� (+)",
#ifdef DBCSAWARE
	"�۰ʰ������줸�r��(�p��������)",
	"�T��b���줸���ϥΦ�X(�h�@�r����)",
#endif
	"�w�]�ƥ��H��P�䥦�O��", //"�P��ѰO��",
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
	    static const char *wm[] = 
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
	    if (HasUserPerm(PERM_ANGEL))
	    {
		static const char *msgs[ANGELPAUSE_MODES] = {
		    "�}�� (�����Ҧ��p�D�H�o��)",
		    "���� (�u�����w�^���L���p�D�H�����D)",
		    "���� (������Ҧ��p�D�H�����D)",
		};
		prints("%c. %-40s%s\n", 
			'1' + iax++, 
			"�p�Ѩϯ��٩I�s��",
			msgs[currutmp->angelpause % ANGELPAUSE_MODES]);
	    }
#endif // PLAY_ANGEL
	}

	/* input */
	key = vmsgf("�Ы� [a-%c,1-%c] �����]�w�A�䥦���N�䵲��: ", 
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
	    if (key == iax-1) 
	    { 
		angel_toggle_pause();
		// dirty = 1; // pure utmp change does not need pw dirty
		continue; 
	    }
	}
#endif //PLAY_ANGEL
    }

    grayout(1, b_lines-2, GRAYOUT_DARK);
    move(b_lines-1, 0); clrtoeol();

    if(dirty)
    {
	passwd_update(usernum, &cuser);
	outs("�]�w�w�x�s�C\n");
    } else {
	outs("�����]�w�C\n");
    }

    redrawwin(); // in case we changed output pref (like DBCS)
    vmsg("�]�w����");
}


void
uinfo_query(userec_t *u, int adminmode, int unum)
{
    userec_t        x;
    int    i = 0, fail;
    int             ans;
    char            buf[STRLEN];
    char            genbuf[200];
    char	    pre_confirmed = 0;
    int y = 0;
    int perm_changed;
    int mail_changed;
    int money_changed;
    int tokill = 0;
    int changefrom = 0;

    fail = 0;
    mail_changed = money_changed = perm_changed = 0;

    {
	// verify unum
	int xuid =  getuser(u->userid, &x);
	if (xuid != unum)
	{
	    move(b_lines-1, 0); clrtobol();
	    prints(ANSI_COLOR(1;31) "���~��T: unum=%d (lookup xuid=%d)"
		    ANSI_RESET "\n", unum, xuid);
	    vmsg("�t�ο��~: �ϥΪ̸�Ƹ��X (unum) ���X�C�Ц� " BN_BUGREPORT "���i�C");
	    return;
	}
    }

    memcpy(&x, u, sizeof(userec_t));
    ans = vans(adminmode ?
    "(1)����(2)�K�X(3)�v��(4)��b��(5)��ID(6)�d��(7)�f�P(M)�H�c  [0]���� " :
    "�п�� (1)�ק��� (2)�]�w�K�X (M)�ק�H�c (C) �ӤH�Ƴ]�w ==> [0]���� ");

    if (ans > '2' && ans != 'm' && ans != 'c' && !adminmode)
	ans = '0';

    if (ans == '1' || ans == '3' || ans == 'm') {
	clear();
	y = 1;
	move(y++, 0);
	outs(msg_uid);
	outs(x.userid);
    }

    if (adminmode && ((ans >= '1' && ans <= '7') || ans == 'm') &&
	search_ulist(unum))
    {
	if (vans("�ϥΪ̥ثe���b�u�W�A�ק��Ʒ|����U�u�C�T�w�n�~��ܡH (y/N): ") 
		!= 'y')
		return;
    }
    switch (ans) {
    case 'c':
	// Customize can only apply to self.
	if (!adminmode)
	    Customize();
	return;

    case 'm':
	do {
	    getdata_str(y, 0, "�q�l�H�c [�ܰʭn���s�{��]�G", buf, 
		    sizeof(x.email), DOECHO, x.email);

	    strip_blank(buf, buf);

	    // fast break
	    if (!buf[0] || strcasecmp(buf, "x") == 0)
		break;

	    // TODO �o�̤]�n emaildb_check
#ifdef USE_EMAILDB
	    if (isvalidemail(buf))
	    {
		int email_count = emaildb_check_email(buf, strlen(buf));
		if (email_count < 0)
		    vmsg("�Ȯɤ����\\ email �{��, �еy��A��");
		else if (email_count >= EMAILDB_LIMIT) 
		    vmsg("���w�� E-Mail �w���U�L�h�b��, �ШϥΨ�L E-Mail");
		else // valid
		    break;
	    }
	    continue;
#endif
	} while (!isvalidemail(buf) && vmsg("�{�ҫH�c����ΨϥΧK�O�H�c"));
	y++;

	// admins may want to use special names
	if (buf[0] &&
		strcmp(buf, x.email) && 
		(strchr(buf, '@') || adminmode)) {

	    // TODO �o�̤]�n emaildb_check
#ifdef USE_EMAILDB
	    if (emaildb_update_email(cuser.userid, strlen(cuser.userid),
			buf, strlen(buf)) < 0) {
		vmsg("�Ȯɤ����\\ email �{��, �еy��A��");
		break;
	    }
#endif
	    strlcpy(x.email, buf, sizeof(x.email));
	    mail_changed = 1;
	    delregcodefile();
	}
	break;

    case '7':
	violate_law(&x, unum);
	return;
    case '1':
	move(0, 0);
	outs("�гv���ק�C");

	getdata_buf(y++, 0, " �� ��  �G", x.nickname,
		    sizeof(x.nickname), DOECHO);
	if (adminmode) {
	    getdata_buf(y++, 0, "�u��m�W�G",
			x.realname, sizeof(x.realname), DOECHO);
	    getdata_buf(y++, 0, "�~��a�}�G",
			x.address, sizeof(x.address), DOECHO);
	    getdata_buf(y++, 0, "�Ǿ�¾�~�G", x.career,
		    sizeof(x.career), DOECHO);
	    getdata_buf(y++, 0, "�q�ܸ��X�G", x.phone,
		    sizeof(x.phone), DOECHO);
	}
	buf[0] = 0;
	if (x.mobile)
	    snprintf(buf, sizeof(buf), "%010d", x.mobile);
	getdata_buf(y++, 0, "������X�G", buf, 11, NUMECHO);
	x.mobile = atoi(buf);
	snprintf(genbuf, sizeof(genbuf), "%d", (u->sex + 1) % 8);
	getdata_str(y++, 0, "�ʧO (1)���� (2)�j�� (3)���} (4)���� (5)���� "
		    "(6)���� (7)�Ӫ� (8)�q���G",
		    buf, 3, NUMECHO, genbuf);
	if (buf[0] >= '1' && buf[0] <= '8')
	    x.sex = (buf[0] - '1') % 8;
	else
	    x.sex = u->sex % 8;

	while (1) {
	    snprintf(genbuf, sizeof(genbuf), "%04i/%02i/%02i",
		     u->year + 1900, u->month, u->day);
	    if (getdata_str(y, 0, "�ͤ� �褸/���/���G", buf, 11, DOECHO, genbuf) == 0) {
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
	    y++;
	    break;
	}

#ifdef PLAY_ANGEL
	if (adminmode) {
	    const char* prompt;
	    userec_t the_angel;
	    if (x.myangel[0] == 0 || x.myangel[0] == '-' ||
		    (getuser(x.myangel, &the_angel) &&
		     the_angel.userlevel & PERM_ANGEL))
		prompt = "�p �� �ϡG";
	    else
		prompt = "�p�Ѩϡ]���b���w�L�p�Ѩϸ��^�G";
	    while (1) {
	        userec_t xuser;
		getdata_str(y, 0, prompt, buf, IDLEN + 1, DOECHO,
			x.myangel);
		if(buf[0] == 0 || strcmp(buf, "-") == 0 ||
			(getuser(buf, &xuser) &&
			    (xuser.userlevel & PERM_ANGEL)) ||
			strcmp(x.myangel, buf) == 0){
		    strlcpy(x.myangel, xuser.userid, IDLEN + 1);
		    ++y;
		    break;
		}

		prompt = "�p �� �ϡG";
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
		    getdata_buf(y, 0, mybuf, genbuf + 11, 80 - 11, DOECHO);
		    ++y;

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
	    int tmp;
	    if (HasUserPerm(PERM_BBSADM)) {
		snprintf(genbuf, sizeof(genbuf), "%d", x.money);
		if (getdata_str(y++, 0, "�Ȧ�b��G", buf, 10, DOECHO, genbuf))
		    if ((tmp = atol(buf)) != 0) {
			if (tmp != x.money) {
			    money_changed = 1;
			    changefrom = x.money;
			    x.money = tmp;
			}
		    }
	    }
	    snprintf(genbuf, sizeof(genbuf), "%d", x.exmailbox);
	    if (getdata_str(y++, 0, "�ʶR�H�c�G", buf, 6,
			    DOECHO, genbuf))
		if ((tmp = atoi(buf)) != 0)
		    x.exmailbox = (int)tmp;

	    getdata_buf(y++, 0, "�{�Ҹ�ơG", x.justify,
			sizeof(x.justify), DOECHO);
	    getdata_buf(y++, 0, "�̪���{�����G",
			x.lasthost, sizeof(x.lasthost), DOECHO);

	    snprintf(genbuf, sizeof(genbuf), "%d", x.numlogins);
	    if (getdata_str(y++, 0, "�W�u���ơG", buf, 10, DOECHO, genbuf))
		if ((tmp = atoi(buf)) >= 0)
		    x.numlogins = tmp;
	    snprintf(genbuf, sizeof(genbuf), "%d", u->numposts);
	    if (getdata_str(y++, 0, "�峹�ƥءG", buf, 10, DOECHO, genbuf))
		if ((tmp = atoi(buf)) >= 0)
		    x.numposts = tmp;
#ifdef ASSESS
	    snprintf(genbuf, sizeof(genbuf), "%d", u->goodpost);
	    if (getdata_str(y++, 0, "�u�}�峹��:", buf, 10, DOECHO, genbuf))
		if ((tmp = atoi(buf)) >= 0)
		    x.goodpost = tmp;
	    snprintf(genbuf, sizeof(genbuf), "%d", u->badpost);
	    if (getdata_str(y++, 0, "�c�H�峹��:", buf, 10, DOECHO, genbuf))
		if ((tmp = atoi(buf)) >= 0)
		    x.badpost = tmp;
#endif // ASSESS

	    snprintf(genbuf, sizeof(genbuf), "%d", u->vl_count);
	    if (getdata_str(y++, 0, "�H�k�O���G", buf, 10, DOECHO, genbuf))
		if ((tmp = atoi(buf)) >= 0)
		    x.vl_count = tmp;

	    snprintf(genbuf, sizeof(genbuf),
		     "%d/%d/%d", u->five_win, u->five_lose, u->five_tie);
	    if (getdata_str(y++, 0, "���l�Ѿ��Z ��/��/�M�G", buf, 16, DOECHO,
			    genbuf))
		while (1) {
		    char *p;
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
	    if (getdata_str(y++, 0, "�H�Ѿ��Z ��/��/�M�G", buf, 16, DOECHO,
			    genbuf))
		while (1) {
		    char *p;
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
	    if (getdata_str(y++, 0, "��b 1)�x�W 2)��L�G", buf, 2, DOECHO, x.uflag2 & FOREIGN ? "2" : "1"))
		if ((tmp = atoi(buf)) > 0){
		    if (tmp == 2){
			x.uflag2 |= FOREIGN;
		    }
		    else
			x.uflag2 &= ~FOREIGN;
		}
	    if (x.uflag2 & FOREIGN)
		if (getdata_str(y++, 0, "�ä[�~�d�v 1)�O 2)�_�G", buf, 2, DOECHO, x.uflag2 & LIVERIGHT ? "1" : "2")){
		    if ((tmp = atoi(buf)) > 0){
			if (tmp == 1){
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
	}
	break;

    case '2':
	y = 19;
	if (!adminmode) {
	    if (!getdata(y++, 0, "�п�J��K�X�G", buf, PASSLEN, NOECHO) ||
		!checkpasswd(u->passwd, buf)) {
		outs("\n\n�z��J���K�X�����T\n");
		fail++;
		break;
	    }
	} 
	if (!getdata(y++, 0, "�г]�w�s�K�X�G", buf, PASSLEN, NOECHO)) {
	    outs("\n\n�K�X�]�w����, �~��ϥ��±K�X\n");
	    fail++;
	    break;
	}
	strlcpy(genbuf, buf, PASSLEN);

	move(y+1, 0);
	outs("�Ъ`�N�]�w�K�X�u���e�K�Ӧr�����ġA�W�L���N�۰ʩ����C");

	getdata(y++, 0, "���ˬd�s�K�X�G", buf, PASSLEN, NOECHO);
	if (strncmp(buf, genbuf, PASSLEN)) {
	    outs("\n\n�s�K�X�T�{����, �L�k�]�w�s�K�X\n");
	    fail++;
	    break;
	}
	buf[8] = '\0';
	strlcpy(x.passwd, genpasswd(buf), sizeof(x.passwd));

	// for admin mode, do verify after.
	if (adminmode)
	{
            FILE *fp;
	    char  witness[3][IDLEN+1], title[100];
	    int uid;
	    for (i = 0; i < 3; i++) {
		if (!getdata(y + i, 0, "�п�J��U�ҩ����ϥΪ̡G",
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
		    // Adjust upper or lower case
                    strlcpy(witness[i], atuser.userid, sizeof(witness[i]));
		}
	    }
	    y += 3;

	    if (i < 3 || fail > 0 || vans(msg_sure_ny) != 'y')
	    {
		fail++;
		break;
	    }
	    pre_confirmed = 1;

	    sprintf(title, "%s ���K�X���]�q�� (by %s)",u->userid, cuser.userid);
            unlink("etc/updatepwd.log");
	    if(! (fp = fopen("etc/updatepwd.log", "w")))
	    {
		move(b_lines-1, 0); clrtobot();
		outs("�t�ο��~: �L�k�إ߳q���ɡA�Ц� " BN_BUGREPORT " ���i�C");
		fail++; pre_confirmed = 0;
		break;
	    }

	    fprintf(fp, "%s �n�D�K�X���]:\n"
		    "���ҤH�� %s, %s, %s",
		    u->userid, witness[0], witness[1], witness[2] );
	    fclose(fp);

	    post_file(BN_SECURITY, title, "etc/updatepwd.log", "[�t�Φw����]");
	    mail_id(u->userid, title, "etc/updatepwd.log", cuser.userid);
	    for(i=0; i<3; i++)
	    {
		mail_id(witness[i], title, "etc/updatepwd.log", cuser.userid);
	    }
	}
	break;

    case '3':
	{
	    int tmp = setperms(x.userlevel, str_permid);
	    if (tmp == x.userlevel)
		fail++;
	    else {
		perm_changed = 1;
		changefrom = x.userlevel;
		x.userlevel = tmp;
	    }
	}
	break;

    case '4':
	tokill = 1;
	{
	    char reason[STRLEN];
	    char title[STRLEN], msg[1024];
	    while (!getdata(b_lines-3, 0, "�п�J�z�ѥH�ܭt�d�G", reason, 50, DOECHO));
	    if (vans(msg_sure_ny) != 'y')
	    {
		fail++;
		break;
	    }
	    pre_confirmed = 1;
	    snprintf(title, sizeof(title), 
		    "�R��ID: %s (����: %s)", x.userid, cuser.userid);
	    snprintf(msg, sizeof(msg), 
		    "�b�� %s �ѯ��� %s ����R���A�z��:\n %s\n\n"
		    "�u��m�W:%s\n��}:%s\n�{�Ҹ��:%s\nEmail:%s\n",
		    x.userid, cuser.userid, reason,
		    x.realname, x.address, x.justify, x.email);
	    post_msg(BN_SECURITY, title, msg, "[�t�Φw����]");
	}
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
	chicken_toggle_death(x.userid);
	break;
    default:
	return;
    }

    if (fail) {
	pressanykey();
	return;
    }

    if (!pre_confirmed)
    {
	if (vans(msg_sure_ny) != 'y')
	    return;
    }

    // now confirmed. do everything directly.

    // perm_changed is by sysop only.
    if (perm_changed) {
	sendalert(x.userid,  ALERT_PWD_PERM); // force to reload perm
	post_change_perm(changefrom, x.userlevel, cuser.userid, x.userid);
#ifdef PLAY_ANGEL
	if (x.userlevel & ~changefrom & PERM_ANGEL)
	    mail_id(x.userid, "�ͻH���X�ӤF�I", "etc/angel_notify", "[�W��]");
#endif
    }
    if (strcmp(u->userid, x.userid)) {
	char            src[STRLEN], dst[STRLEN];
	kick_all(u->userid);
	sethomepath(src, u->userid);
	sethomepath(dst, x.userid);
	Rename(src, dst);
	setuserid(unum, x.userid);
    }
    if (mail_changed && !adminmode) {
	// wait registration.
	x.userlevel &= ~(PERM_LOGINOK | PERM_POST);
    }
    memcpy(u, &x, sizeof(x));
    if (tokill) {
	kick_all(x.userid);
	kill_user(unum, x.userid);
	return;
    } else
	log_usies("SetUser", x.userid);

    if (money_changed) {
	char title[TTLEN+1];
	char msg[512];
	char reason[50];
	clrtobot();
	clear();
	while (!getdata(5, 0, "�п�J�z�ѥH�ܭt�d�G",
		    reason, sizeof(reason), DOECHO));

	snprintf(msg, sizeof(msg),
		"   ����" ANSI_COLOR(1;32) "%s" ANSI_RESET "��" ANSI_COLOR(1;32) "%s" ANSI_RESET "����"
		"�q" ANSI_COLOR(1;35) "%d" ANSI_RESET "�令" ANSI_COLOR(1;35) "%d" ANSI_RESET "\n"
		"   " ANSI_COLOR(1;37) "����%s�ק���z�ѬO�G%s" ANSI_RESET,
		cuser.userid, x.userid, changefrom, x.money,
		cuser.userid, reason);
	snprintf(title, sizeof(title),
		"[���w���i] ����%s�ק�%s�����i", cuser.userid,
		x.userid);
	post_msg(BN_SECURITY, title, msg, "[�t�Φw����]");
	setumoney(unum, x.money);
    }

    passwd_update(unum, &x);

    if (adminmode)
    {
	sendalert(x.userid, ALERT_PWD_RELOAD);
	kick_all(x.userid);
    }

    // resolve_over18 only works for cuser
    if (!adminmode)
	resolve_over18();
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
	    show_file(genbuf, 13, 10, SHOWFILE_ALLOW_COLOR);
	    return;
	}
    }
#endif /* defined(CHESSCOUNTRY) */

    sethomefile(genbuf, user->userid, fn_plans);
    if (!show_file(genbuf, 7, MAX_QUERYLINES, SHOWFILE_ALLOW_COLOR))
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
    char            genbuf[PATHLEN];
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
isvalidemail(const char *email)
{
    FILE           *fp;
    char            buf[128], *c;
    if (!strstr(email, "@"))
	return 0;
    for (c = strstr(email, "@"); *c != 0; ++c)
	if ('A' <= *c && *c <= 'Z')
	    *c += 32;

    // allow list
    if ((fp = fopen("etc/whitemail", "rt")))
    {
	int allow = 0;
	while (fgets(buf, sizeof(buf), fp)) {
	    if (buf[0] == '#')
		continue;
	    chomp(buf);
	    c = buf+1;
	    // vmsgf("%c %s %s",buf[0], c, email);
	    switch(buf[0])
	    {
		case 'A': if (strcasecmp(c, email) == 0)	allow = 1; break;
		case 'P': if (strcasestr(email, c))	allow = 1; break;
		case 'S': if (strcasecmp(strstr(email, "@") + 1, c) == 0) allow = 1; break;
		case '%': allow = 1; break; // allow all
	        // domain match (*@c | *@*.c)
		case 'D': if (strlen(email) > strlen(c))
			  {
			      // c2 points to starting of possible c.
			      const char *c2 = email + strlen(email) - strlen(c);
			      if (strcasecmp(c2, c) != 0)
				  break;
			      c2--;
			      if (*c2 == '.' || *c2 == '@')
				  allow = 1;
			  }
			  break;
	    }
	    if (allow) break;
	}
	fclose(fp);
	if (!allow) 
	{
	    // show whitemail notice if it exists.
	    if (dashf(FN_NOTIN_WHITELIST_NOTICE))
	    {
		VREFSCR scr = vscr_save();
		more(FN_NOTIN_WHITELIST_NOTICE, NA);
		pressanykey();
		vscr_restore(scr);
	    }
	    return 0;
	}
    }

    // reject list
    if ((fp = fopen("etc/banemail", "r"))) {
	while (fgets(buf, sizeof(buf), fp)) {
	    if (buf[0] == '#')
		continue;
	    chomp(buf);
	    c = buf+1;
	    switch(buf[0])
	    {
		case 'A': if (strcasecmp(c, email) == 0)	return 0;
		case 'P': if (strcasestr(email, c))	return 0;
		case 'S': if (strcasecmp(strstr(email, "@") + 1, c) == 0) return 0;
		case 'D': if (strlen(email) > strlen(c))
			  {
			      // c2 points to starting of possible c.
			      const char *c2 = email + strlen(email) - strlen(c);
			      if (strcasecmp(c2, c) != 0)
				  break;
			      c2--;
			      if (*c2 == '.' || *c2 == '@')
				  return 0;
			  }
			  break;
	    }
	}
	fclose(fp);
    }
    return 1;
}

/* �C�X�Ҧ����U�ϥΪ� */
struct ListAllUsetCtx {
    int usercounter;
    int totalusers;
    unsigned short u_list_special;
    int y;
};

static int
u_list_CB(void *data, int num, userec_t * uentp)
{
    char            permstr[8], *ptr;
    register int    level;
    struct ListAllUsetCtx *ctx = (struct ListAllUsetCtx*) data;
    (void)num;

    if (uentp == NULL) {
	move(2, 0);
	clrtoeol();
	prints(ANSI_REVERSE "  �ϥΪ̥N��   %-25s   �W��  �峹  %s  "
	       "�̪���{���     " ANSI_COLOR(0) "\n",
	       "�︹�ʺ�",
	       HasUserPerm(PERM_SEEULEVELS) ? "����" : "");
	ctx->y = 3;
	return 0;
    }
    if (bad_user_id(uentp->userid))
	return 0;

    if ((uentp->userlevel & ~(ctx->u_list_special)) == 0)
	return 0;

    if (ctx->y == b_lines) {
	int ch;
	prints(ANSI_COLOR(34;46) "  �w��� %d/%d �H(%d%%)  " ANSI_COLOR(31;47) "  "
	       "(Space)" ANSI_COLOR(30) " �ݤU�@��  " ANSI_COLOR(31) "(Q)" ANSI_COLOR(30) " ���}  " ANSI_RESET,
	       ctx->usercounter, ctx->totalusers, ctx->usercounter * 100 / ctx->totalusers);
	ch = igetch();
	if (ch == 'q' || ch == 'Q')
	    return -1;
	ctx->y = 3;
    }
    if (ctx->y == 3) {
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
    ctx->usercounter++;
    ctx->y++;
    return 0;
}

int
u_list(void)
{
    char            genbuf[3];
    struct ListAllUsetCtx data, *ctx = &data;

    setutmpmode(LAUSERS);
    ctx->u_list_special = ctx->usercounter = 0;
    ctx->totalusers = SHM->number;
    if (HasUserPerm(PERM_SEEULEVELS)) {
	getdata(b_lines - 1, 0, "�[�� [1]�S���� (2)�����H",
		genbuf, 3, DOECHO);
	if (genbuf[0] != '2')
	    ctx->u_list_special = PERM_BASIC | PERM_CHAT | PERM_PAGE | PERM_POST | PERM_LOGINOK | PERM_BM;
    }
    u_list_CB(ctx, 0, NULL);
    passwd_apply(ctx, u_list_CB);
    move(b_lines, 0);
    clrtoeol();
    prints(ANSI_COLOR(34;46) "  �w��� %d/%d ���ϥΪ�(�t�ήe�q�L�W��)  "
	   ANSI_COLOR(31;47) "  (�Ы����N���~��)  " ANSI_RESET, ctx->usercounter, ctx->totalusers);
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
    vs_hdr("�]�w�۰ʰ������줸�r�� (��������)");
    move(2, 0);
    outs(ANSI_RESET
    "* �����䴩�۰ʰ�������r�����ʻP�s��A�����ǳs�u�{�� (�pxxMan)\n"
    "  �]�|�ۦ�չϰ����B�h�e����A��O�K�|�y��" ANSI_COLOR(1;37)
      "�@�����ʨ�Ӥ���r���{�H�C" ANSI_RESET "\n\n"
    "* ���s�u�{���B�z���ʮe���y����ܤβ��ʤW�~�P�����D�A�ҥH�ڭ̫�ĳ�z\n"
    "  �����ӵ{���W���]�w�]�q�`�s�u����(���������줸��)����v�^�A\n"
    "  �� BBS �t�Υi�H���T������A���e���C\n\n"
    ANSI_COLOR(1;33) 
    "* �p�G�z�ݤ����W���������]�L�ҿסA�ڭ̷|�۰ʰ����A�X�z���]�w�C"
    ANSI_RESET "\n"
    "  �Цb�]�w�n�s�u�{�����z���n���Ҧ����" ANSI_COLOR(1;33)
    "�@�U" ANSI_RESET "�z��L�W��" ANSI_COLOR(1;33)
    "��" ANSI_RESET "\n" ANSI_COLOR(1;36)
    "  (���k��V��μg BS/Backspace ���˰h��P Del �R���䧡�i)\n"
	    ANSI_RESET);

    /* clear buffer */
    peek_input(0.1, Ctrl('C'));
    drop_input();

    while (1)
    {
	int ch = 0;

	move(14, 0);
	outs("�o�O�����ϡA�z����з|�X�{�b" 
		ANSI_REVERSE "�o��" ANSI_RESET);
	move(14, 15*2);
	ch = igetch();
	if(ch != KEY_LEFT && ch != KEY_RIGHT &&
	   ch != KEY_BS && ch != KEY_BS2)
	{
	    move(16, 0);
	    bell();
	    outs("�Ы��@�U�W�����w����I �A����O����F�I");
	} else {
	    move(16, 0);
	    /* Actually you may also use num_in_buf here.  those clients
	     * usually sends doubled keys together in one packet.
	     * However when I was writing this, a bug (existed for more than 3
	     * years) of num_in_buf forced me to write new wait_input.
	     * Anyway it is fixed now.
	     */
	    refresh();
	    if(wait_input(0.1, 0))
	    // if(igetch() == ch)
	    // if (num_in_buf() > 0)
	    {
		/* evil dbcs aware client */
		outs("������z���s�u�{���|�ۦ�B�z��в��ʡC\n"
			// "�Y���]���y���s���W�����D���������B�z�C\n\n"
			"�w�]�w���u���z���s�u�{���B�z��в��ʡv\n");
		ret = 1;
	    } else {
		/* good non-dbcs aware client */
		outs("�z���s�u�{�����G���|�h�e����A"
			"�o�� BBS �i�H���Ǫ�����e���C\n"
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
    drop_input();
    pressanykey();
    return ret;
}
#endif 

/* vim:sw=4
 */
