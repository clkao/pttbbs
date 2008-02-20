/* $Id$ */
#include "bbs.h"

// prototype of crypt()
char *crypt(const char *key, const char *salt);

char           *
genpasswd(char *pw)
{
    if (pw[0]) {
	char            saltc[2], c;
	int             i;

	i = 9 * getpid();
	saltc[0] = i & 077;
	saltc[1] = (i >> 6) & 077;

	for (i = 0; i < 2; i++) {
	    c = saltc[i] + '.';
	    if (c > '9')
		c += 7;
	    if (c > 'Z')
		c += 6;
	    saltc[i] = c;
	}
	return crypt(pw, saltc);
    }
    return "";
}

// NOTE it will clean string in "plain"
int
checkpasswd(const char *passwd, char *plain)
{
    int             ok;
    char           *pw;

    ok = 0;
    pw = crypt(plain, passwd);
    if(pw && strcmp(pw, passwd)==0)
	ok = 1;
    memset(plain, 0, strlen(plain));

    return ok;
}

/* �ˬd user ���U���p */
int
bad_user_id(const char *userid)
{
    if(!is_validuserid(userid))
	return 1;

    if (strcasecmp(userid, str_new) == 0)
	return 1;

#ifdef NO_GUEST_ACCOUNT_REG
    if (strcasecmp(userid, STR_GUEST) == 0)
	return 1;
#endif

    /* in2: �쥻�O��strcasestr,
            ���L���ǤH�������n�X�{�o�Ӧr�����ٺ�X�z�a? */
    if( strncasecmp(userid, "fuck", 4) == 0 ||
        strncasecmp(userid, "shit", 4) == 0 )
	return 1;

    /*
     * while((ch = *(++userid))) if(not_alnum(ch)) return 1;
     */
    return 0;
}

/* -------------------------------- */
/* New policy for allocate new user */
/* (a) is the worst user currently  */
/* (b) is the object to be compared */
/* -------------------------------- */
static int
compute_user_value(const userec_t * urec, time4_t clock)
{
    int             value;

    /* if (urec) has XEMPT permission, don't kick it */
    if ((urec->userid[0] == '\0') || (urec->userlevel & PERM_XEMPT)
    /* || (urec->userlevel & PERM_LOGINOK) */
	|| !strcmp(STR_GUEST, urec->userid))
	return 999999;
    value = (clock - urec->lastlogin) / 60;	/* minutes */

    /* new user should register in 30 mins */
    // XXX �ثe new acccount �ä��|�b utmp �̩� str_new...
    if (strcmp(urec->userid, str_new) == 0)
	return 30 - value;

#if 0
    if (!urec->numlogins)	/* �� login ���\�̡A���O�d */
	return -1;
    if (urec->numlogins <= 3)	/* #login �֩�T�̡A�O�d 20 �� */
	return 20 * 24 * 60 - value;
#endif
    /* ���������U�̡A�O�d 15 �� */
    /* �@�뱡�p�A�O�d 120 �� */
    return (urec->userlevel & PERM_LOGINOK ? 120 : 15) * 24 * 60 - value;
}

int
check_and_expire_account(int uid, const userec_t * urec)
{
    char            genbuf[200];
    int             val;
    if ((val = compute_user_value(urec, now)) < 0) {
	snprintf(genbuf, sizeof(genbuf), "#%d %-12s %15.15s %d %d %d",
		 uid, urec->userid, ctime4(&(urec->lastlogin)) + 4,
		 urec->numlogins, urec->numposts, val);
	if (val > -1 * 60 * 24 * 365) {
	    log_usies("CLEAN", genbuf);
            kill_user(uid, urec->userid);
	} else {
	    val = 0;
	    log_usies("DATED", genbuf);
	}
    }
    return val;
}


int
setupnewuser(const userec_t *user)
{
    char            genbuf[50];
    char           *fn_fresh = ".fresh";
    userec_t        utmp;
    time_t          clock;
    struct stat     st;
    int             fd, uid;

    clock = now;

    // XXX race condition...
    if (dosearchuser(user->userid, NULL))
    {
	vmsg("��}�����֡A�O�H�w�g�m���F�I");
	exit(1);
    }

    /* Lazy method : ����M�w�g�M�����L���b�� */
    if ((uid = dosearchuser("", NULL)) == 0) {
	/* �C 1 �Ӥp�ɡA�M�z user �b���@�� */
	if ((stat(fn_fresh, &st) == -1) || (st.st_mtime < clock - 3600)) {
	    if ((fd = open(fn_fresh, O_RDWR | O_CREAT, 0600)) == -1)
		return -1;
	    write(fd, ctime(&clock), 25);
	    close(fd);
	    log_usies("CLEAN", "dated users");

	    fprintf(stdout, "�M��s�b����, �еy�ݤ���...\n\r");

	    if ((fd = open(fn_passwd, O_RDWR | O_CREAT, 0600)) == -1)
		return -1;

	    /* ����o������n�q 2 �}�l... Ptt:�]��SYSOP�b1 */
	    for (uid = 2; uid <= MAX_USERS; uid++) {
		passwd_query(uid, &utmp);
		check_and_expire_account(uid, &utmp);
	    }
	}
    }

    /* initialize passwd semaphores */
    if (passwd_init())
	exit(1);

    passwd_lock();

    uid = dosearchuser("", NULL);
    if ((uid <= 0) || (uid > MAX_USERS)) {
	passwd_unlock();
	vmsg("��p�A�ϥΪ̱b���w�g���F�A�L�k���U�s���b��");
	exit(1);
    }

    setuserid(uid, user->userid);
    snprintf(genbuf, sizeof(genbuf), "uid %d", uid);
    log_usies("APPLY", genbuf);

    SHM->money[uid - 1] = user->money;

    if (passwd_update(uid, (userec_t *)user) == -1) {
	passwd_unlock();
	vmsg("�Ⱥ��F�A�A���I");
	exit(1);
    }

    passwd_unlock();

    return uid;
}

// checking functions (in user.c now...)
char *isvalidname(char *rname);

void
new_register(void)
{
    userec_t        newuser;
    char            passbuf[STRLEN];
    int             try, id, uid;
    char 	   *errmsg = NULL;

#ifdef HAVE_USERAGREEMENT
    more(HAVE_USERAGREEMENT, YEA);
    while( 1 ){
	getdata(b_lines, 0, "�аݱz�����o���ϥΪ̱��ڶ�? (yes/no) ",
		passbuf, 4, LCECHO);
	if( passbuf[0] == 'y' )
	    break;
	if( passbuf[0] == 'n' ){
	    vmsg("��p, �z���n�����ϥΪ̱��ڤ~����U�b���ɨ��ڭ̪��A�ȭ�!");
	    exit(1);
	}
	vmsg("�п�J y��ܱ���, n��ܤ�����");
    }
#endif

    // setup newuser
    memset(&newuser, 0, sizeof(newuser));
    newuser.version = PASSWD_VERSION;
    newuser.userlevel = PERM_DEFAULT;
    newuser.uflag = BRDSORT_FLAG | MOVIE_FLAG;
    newuser.uflag2 = 0;
    newuser.firstlogin = newuser.lastlogin = now;
    newuser.money = 0;
    newuser.pager = PAGER_ON;
    strlcpy(newuser.lasthost, fromhost, sizeof(newuser.lasthost));

#ifdef DBCSAWARE
    if(u_detectDBCSAwareEvilClient())
	newuser.uflag &= ~DBCSAWARE_FLAG;
    else
	newuser.uflag |= DBCSAWARE_FLAG;
#endif

    more("etc/register", NA);
    try = 0;
    while (1) {
        userec_t xuser;
	int minute;

	if (++try >= 6) {
	    vmsg("�z���տ��~����J�Ӧh�A�ФU���A�ӧa");
	    exit(1);
	}
	getdata(17, 0, msg_uid, newuser.userid,
		sizeof(newuser.userid), DOECHO);
        strcpy(passbuf, newuser.userid);

	if (bad_user_id(passbuf))
	    outs("�L�k�����o�ӥN���A�Шϥέ^��r���A�åB���n�]�t�Ů�\n");
	else if ((id = getuser(passbuf, &xuser)) &&
		 (minute = check_and_expire_account(id, &xuser)) >= 0) {
	    if (minute == 999999) // XXX magic number.  It should be greater than MAX_USERS at least.
		outs("���N���w�g���H�ϥ� �O��������");
	    else {
		prints("���N���w�g���H�ϥ� �٦� %d �Ѥ~�L�� \n", 
			minute / (60 * 24) + 1);
	    }
	} else
	    break;
    }

    // XXX �O�o�̫� create account �e�٭n�A�ˬd�@�� acc

    try = 0;
    while (1) {
	if (++try >= 6) {
	    vmsg("�z���տ��~����J�Ӧh�A�ФU���A�ӧa");
	    exit(1);
	}
	move(19, 0); clrtoeol();
	outs(ANSI_COLOR(1;33) "���קK�Q���ݡA�z���K�X�ä��|��ܦb�e���W�A������J����� Enter ��Y�i�C" ANSI_RESET);
	if ((getdata(18, 0, "�г]�w�K�X�G", passbuf,
		     sizeof(passbuf), NOECHO) < 3) ||
	    !strcmp(passbuf, newuser.userid)) {
	    outs("�K�X��²��A���D�J�I�A�ܤ֭n 4 �Ӧr�A�Э��s��J\n");
	    continue;
	}
	strlcpy(newuser.passwd, passbuf, PASSLEN);
	getdata(19, 0, "���ˬd�K�X�G", passbuf, sizeof(passbuf), NOECHO);
	if (strncmp(passbuf, newuser.passwd, PASSLEN)) {
	    outs("�K�X��J���~, �Э��s��J�K�X.\n");
	    continue;
	}
	passbuf[8] = '\0';
	strlcpy(newuser.passwd, genpasswd(passbuf), PASSLEN);
	break;
    }
    // set-up more information.

    // warning: because currutmp=NULL, we can simply pass newuser.* to getdata.
    // DON'T DO THIS IF YOUR currutmp != NULL.
    try = 0;
    while (strlen(newuser.nickname) < 2)
    {
	if (++try > 10) {
	    vmsg("�z���տ��~����J�Ӧh�A�ФU���A�ӧa");
	    exit(1);
	}
	getdata(19, 0, "�︹�ʺ١G", newuser.nickname,
		sizeof(newuser.nickname), DOECHO);
    }

    try = 0;
    while (strlen(newuser.realname) < 4)
    {
	if (++try > 10) {
	    vmsg("�z���տ��~����J�Ӧh�A�ФU���A�ӧa");
	    exit(1);
	}
	getdata(20, 0, "�u��m�W�G", newuser.realname,
		sizeof(newuser.realname), DOECHO);

	if ((errmsg = isvalidname(newuser.realname)))
	{
	    memset(newuser.realname, 0, sizeof(newuser.realname));
	    vmsg(errmsg); 
	}
    }

    try = 0;
    while (strlen(newuser.address) < 8)
    {
	// do not use isvalidaddr to check,
	// because that requires foreign info.
	if (++try > 10) {
	    vmsg("�z���տ��~����J�Ӧh�A�ФU���A�ӧa");
	    exit(1);
	}
	getdata(21, 0, "�p���a�}�G", newuser.address,
		sizeof(newuser.address), DOECHO);
    }

    try = 0;
    while (newuser.year < 40) // magic number 40: see user.c
    {
	char birthday[sizeof("mmmm/yy/dd ")];
	int y, m, d;

	if (++try > 20) {
	    vmsg("�z���տ��~����J�Ӧh�A�ФU���A�ӧa");
	    exit(1);
	}
	getdata(22, 0, "�ͤ� (�褸�~/��/��, �p 1984/02/29)�G", birthday,
		sizeof(birthday), DOECHO);

	if (ParseDate(birthday, &y, &m, &d)) {
	    vmsg("����榡�����T");
	    continue;
	} else if (y < 1940) {
	    vmsg("�A�u��������ѶܡH");
	    continue;
	}
	newuser.year  = (unsigned char)(y-1900);
	newuser.month = (unsigned char)m;
	newuser.day   = (unsigned char)d;
    }

    setupnewuser(&newuser);

    if( (uid = initcuser(newuser.userid)) < 0) {
	vmsg("�L�k�إ߱b��");
	exit(1);
    }
    log_usies("REGISTER", fromhost);
}

void 
check_birthday(void)
{
    // check birthday
    int changed = 0;
   
    while (cuser.year < 40) // magic number 40: see user.c
    {
	char birthday[sizeof("mmmm/yy/dd ")];
	int y, m, d;

	clear();
	stand_title("��J�ͤ�");
	move(2,0);
	outs("�������t�X��椺�e���Ũ�סA�бz��J���T���ͤ��T�C");

	getdata(5, 0, "�ͤ� (�褸�~/��/��, �p 1984/02/29)�G", birthday,
		sizeof(birthday), DOECHO);

	if (ParseDate(birthday, &y, &m, &d)) {
	    vmsg("����榡�����T");
	    continue;
	} else if (y < 1940) {
	    vmsg("�A�u��������ѶܡH");
	    continue;
	}
	cuser.year  = (unsigned char)(y-1900);
	cuser.month = (unsigned char)m;
	cuser.day   = (unsigned char)d;
	changed = 1;
    }

    if (changed) {
	clear();
	resolve_over18();
    }
}

void
check_register(void)
{
    char fn[PATHLEN];

    check_birthday();

    if (HasUserPerm(PERM_LOGINOK))
	return;

    // see admin.c
    setuserfile(fn, "justify.reject");


    /* 
     * �קK�ϥΪ̳Q�h�^���U���A�b���D�h�^����]���e�A
     * �S�e�X�@�����U��C
     */ 
    if (dashf(fn))
    {
	more(fn, NA);
	move(b_lines-3, 0);
	outs("�W�����U��f�d���ѡC\n"
	     "�Э��s�ӽШ÷ӤW�����ܥ��T��g���U��C");
	while(getans("�п�J y �~��: ") != 'y');
	unlink(fn);
    } else
    if (ISNEWMAIL(currutmp))
	m_read();

    if (!HasUserPerm(PERM_SYSOP)) {
	/* �^�йL�����{�ҫH��A�δ��g E-mail post �L */
	clear();
	move(9, 3);
	outs("�иԶ�g" ANSI_COLOR(32) "���U�ӽг�" ANSI_RESET "�A"
	       "�q�i�����H��o�i���ϥ��v�O�C\n\n\n\n");
	u_register();

#ifdef NEWUSER_LIMIT
	if (cuser.lastlogin - cuser->firstlogin < 3 * 86400)
	    cuser.userlevel &= ~PERM_POST;
	more("etc/newuser", YEA);
#endif
    }
}
/* vim:sw=4
 */
