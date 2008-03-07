/* $Id$ */
#include "bbs.h"

#define FN_REGISTER_LOG  "register.log"
#define FN_JUSTIFY	 "justify"
#define FN_JUSTIFY_WAIT	 "justify.wait"
#define FN_REJECT_NOTIFY "justify.reject"

////////////////////////////////////////////////////////////////////////////
// Password Hash
////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////
// Value Validation
////////////////////////////////////////////////////////////////////////////
static int 
HaveRejectStr(const char *s, const char **rej)
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

static char *
isvalidname(char *rname)
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

static char *
isvalidcareer(char *career)
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

static char *
isvalidaddr(char *addr)
{
    const char    *rejectstr[] =
	{"�a�y", "�Ȫe", "���P", NULL};

    // addr[0] > 0: check if address is starting by Chinese.
    if (!removespace(addr) || strlen(addr) < 15) 
	return "�o�Ӧa�}���G�ä�����";
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
	return "�o�Ӧa�}���G�ä�����";
    return NULL;
}

static char *
isvalidphone(char *phone)
{
    int     i;
    for( i = 0 ; phone[i] != 0 ; ++i )
	if( !isdigit((int)phone[i]) )
	    return "�Ф��n�[���j�Ÿ�";
    if (!removespace(phone) || 
	strlen(phone) < 9 || 
	strstr(phone, "00000000") != NULL ||
	strstr(phone, "22222222") != NULL    ) {
	return "�o�ӹq�ܸ��X�ä����T(�Чt�ϽX)" ;
    }
    return NULL;
}


////////////////////////////////////////////////////////////////////////////
// Account Expiring
////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////
// Regcode Support
////////////////////////////////////////////////////////////////////////////

#define REGCODE_INITIAL "v6" // always 2 characters

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
    // prevent ambigious characters: oOlI
    const char *alphabet = "qwertyuipasdfghjkzxcvbnmoQWERTYUPASDFGHJKLZXCVBNM";

    /* generate a new regcode */
    buf[13] = 0;
    buf[0] = REGCODE_INITIAL[0];
    buf[1] = REGCODE_INITIAL[1];
    for( i = 2 ; i < 13 ; ++i )
	buf[i] = alphabet[random() % strlen(alphabet)];

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

void
delregcodefile(void)
{
    char    fpath[PATHLEN];
    getregfile(fpath);
    unlink(fpath);
}

////////////////////////////////////////////////////////////////////////////
// Justify Utilities
////////////////////////////////////////////////////////////////////////////

static void
justify_wait(char *userid, char *phone, char *career,
	char *rname, char *addr, char *mobile)
{
    char buf[PATHLEN];
    sethomefile(buf, userid, FN_JUSTIFY_WAIT);
    if (phone[0] != 0) {
	FILE* fn = fopen(buf, "w");
	assert(fn);
	fprintf(fn, "%s\n%s\ndummy\n%s\n%s\n%s\n",
		phone, career, rname, addr, mobile);
	fclose(fn);
    }
}

static void 
email_justify(const userec_t *muser)
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


/* �ϥΪ̶�g���U��� */
static void
getfield(int line, const char *info, const char *desc, char *buf, int len)
{
    char            prompt[STRLEN];
    char            genbuf[200];

    // clear first
    move(line+1, 0); clrtoeol();
    move(line, 0); clrtoeol();
    prints("  ����]�w�G%-30.30s (%s)", buf, info);
    snprintf(prompt, sizeof(prompt), "  %s�G", desc);
    if (getdata_str(line + 1, 0, prompt, genbuf, len, DOECHO, buf))
	strcpy(buf, genbuf);
    move(line+1, 0); clrtoeol();
    move(line, 0); clrtoeol();
    prints("  %s�G%s", desc, buf);
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

/////////////////////////////////////////////////////////////////////////////
// New Registration (Phase 1)
/////////////////////////////////////////////////////////////////////////////

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
	outs(ANSI_COLOR(1;33) 
    "���קK�Q���ݡA�z���K�X�ä��|��ܦb�e���W�A������J����� Enter ��Y�i�C\n"
    "�t�~�Ъ`�N�K�X�u���e�K�Ӧr�����ġA�W�L���N�۰ʩ����C"
	ANSI_RESET);
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

    setuserfile(fn, FN_REJECT_NOTIFY);

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

/////////////////////////////////////////////////////////////////////////////
// User Registration (Phase 2)
/////////////////////////////////////////////////////////////////////////////

static void
toregister(char *email, char *phone, char *career,
	   char *rname, char *addr, char *mobile)
{
    FILE *fn = NULL;

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
	   "���`�N�o�i��|��W�ƶg�Χ�h�ɶ��C" ANSI_RESET "\n"
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
		move(15, 0); clrtobot();
		move(17, 0);
		outs("���w��������X�����T,"
		       "�Y�z�L��������п�ܨ�L�覡�{��");
	    }

	}
#endif
	else if (isvalidemail(email)) {
	    char            yn[3];
#ifdef USE_EMAILDB
	    int email_count = emaildb_check_email(email, strlen(email));

	    if (email_count < 0) {
		move(15, 0); clrtobot();
		move(17, 0);
		outs("�Ȯɤ����\\ email �{�ҵ��U, �еy��A��\n");
		pressanykey();
		return;
	    } else if (email_count >= EMAILDB_LIMIT) { 
		move(15, 0); clrtobot();
		move(17, 0);
		outs("���w�� E-Mail �w���U�L�h�b��, �ШϥΨ�L E-Mail, �ο�J x �Ĥ�ʻ{��\n");
		outs("���`�N��ʻ{�ҳq�`�|��W�ƶg�H�W���ɶ��C\n");
	    } else {
#endif
	    getdata(16, 0, "�ЦA���T�{�z��J�� E-Mail ��m���T��? [y/N]",
		    yn, sizeof(yn), LCECHO);
	    if (yn[0] == 'Y' || yn[0] == 'y')
		break;
#ifdef USE_EMAILDB
	    }
#endif
	} else {
	    move(15, 0); clrtobot();
	    move(17, 0);
	    outs("���w�� E-Mail �����T, �Y�z�L E-Mail �п�J x �ѯ�����ʻ{��\n");
	    outs("���`�N��ʻ{�ҳq�`�|��W�ƶg�H�W���ɶ��C\n");
	}
    }
#ifdef USE_EMAILDB
    if (emaildb_update_email(cuser.userid, strlen(cuser.userid),
		email, strlen(email)) < 0) {
	move(15, 0); clrtobot();
	move(17, 0);
	outs("�Ȯɤ����\\ email �{�ҵ��U, �еy��A��\n");
	pressanykey();
	return;
    }
#endif
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
	    // save justify information
	    snprintf(cuser.justify, sizeof(cuser.justify),
		    "%s:%s:<Manual>", phone, career);
	}
	// XXX what if we cannot open register form?
    } else {
	// register by mail of phone
	snprintf(cuser.justify, sizeof(cuser.justify),
		"%s:%s:<Email>", phone, career);
#ifdef HAVEMOBILE
	if (phone != NULL && email[1] == 0 && tolower(email[0]) == 'm')
	    sprintf(cuser.justify, sizeof(cuser.justify),
		    "%s:%s:<Mobile>", phone, career);
#endif
       email_justify(&cuser);
    }
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
	    outs("   �n����f�֧��� (�|�h��ܦh�ɶ��A�q�`�_�X�Ƥ�) �A�ҥH�Э@�ߵ��ԡC\n\n");

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
    if (cuser.mobile)
	snprintf(mobile, sizeof(mobile), "0%09d", cuser.mobile);
    else
	mobile[0] = 0;
    if (cuser.month == 0 && cuser.day == 0 && cuser.year == 0)
	birthday[0] = 0;
    else
	snprintf(birthday, sizeof(birthday), "%04i/%02i/%02i",
		 1900 + cuser.year, cuser.month, cuser.day);
    sex_is[0] = (cuser.sex % 8) + '1';
    sex_is[1] = 0;
    career[0] = phone[0] = '\0';
    sethomefile(genbuf, cuser.userid, FN_JUSTIFY_WAIT);
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

    // getregcode(regcode);

    // XXX why check by year? 
    // birthday is moved to earlier, so let's check email instead.
    if (cuser.email[0] && // cuser.year != 0 &&	/* �w�g�Ĥ@����L�F~ ^^" */
	strcmp(cuser.email, "x") != 0 &&	/* �W����ʻ{�ҥ��� */
	strcmp(cuser.email, "X") != 0) 
    {
	clear();
	stand_title("EMail�{��");
	move(2, 0);

	prints("�п�J�z���{�ҽX�C(�� %s �}�Y�L�ťժ��Q�T�X)\n"
	       "�ο�J x �ӭ��s��g E-Mail �Χ�ѯ�����ʻ{��\n", REGCODE_INITIAL);
	inregcode[0] = 0;

	do{
	    getdata(10, 0, "�z���{�ҽX�G",
		    inregcode, sizeof(inregcode), DOECHO);
	    if( strcmp(inregcode, "x") == 0 || strcmp(inregcode, "X") == 0 )
		break;
	    if( strlen(inregcode) != 13 || inregcode[0] == ' ')
		vmsg("�{�ҽX��J������A�`�@�����Q�T�X�A�S���ťզr���C");
	    else if( inregcode[0] != REGCODE_INITIAL[0] || inregcode[1] != REGCODE_INITIAL[1] ) {
		/* old regcode */
		vmsg("��J���{�ҽX���~�A" // "�Φ]�t�Ϊ@�Ťw���ġA"
		     "�п�J x ����@�� E-Mail");
	    }
	    else
		break;
	} while( 1 );

	// make it case insensitive.
	if (strcasecmp(inregcode, getregcode(regcode)) == 0) {
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
	    sethomefile(genbuf, cuser.userid, FN_JUSTIFY_WAIT);
	    unlink(genbuf);
	    snprintf(cuser.justify, sizeof(cuser.justify),
		     "%s:%s:email", phone, career);
	    sethomefile(genbuf, cuser.userid, FN_JUSTIFY);
	    log_file(genbuf, LOG_CREAT, cuser.justify);
	    pressanykey();
	    u_exit("registed");
	    exit(0);
	    return QUIT;
	} else if (strcasecmp(inregcode, "x") != 0) {
	    if (regcode[0])
	    {
		vmsg("�{�ҽX���~�I");
		return FULLUPDATE;
	    }
	    else 
	    {
		vmsg("�{�ҽX�w�L���A�Э��s���U�C");
		toregister(email, phone, career, rname, addr, mobile);
		return FULLUPDATE;
	    }
	} else {
	    toregister(email, phone, career, rname, addr, mobile);
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
	move(10, 0); clrtobot();
	while (1) {
	    getfield(10, "�t" ANSI_COLOR(1;33) "����" ANSI_RESET "�Ϊ��츹�X"
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
	    getfield(11, "���[-(), �]�A���~�ϸ�", "�s���q��", phone, 11);
	    if( (errcode = isvalidphone(phone)) == NULL )
		break;
	    else
		vmsg(errcode);
	}
	getfield(12, "�u��J�Ʀr �p:0912345678 (�i����)",
		 "������X", mobile, 20);
	while (1) {
	    getfield(13, "�褸/���/��� �p:1984/02/29", "�ͤ�", birthday, sizeof(birthday));
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
	getfield(14, "1.���� 2.�j�� ", "�ʧO", sex_is, 2);
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

    toregister(email, phone, career, rname, addr, mobile);

    // update cuser
    passwd_update(usernum, &cuser);

    return FULLUPDATE;
}

/////////////////////////////////////////////////////////////////////////////
// Administration (SYSOP Validation)
/////////////////////////////////////////////////////////////////////////////

#define REJECT_REASONS	(6)
#define REASON_LEN	(60)
static const char *reasonstr[REJECT_REASONS] = {
    "��J�u��m�W",
    "�Զ�(���~)�Ǯաy�t�z�y�šz�ΪA�ȳ��(�t���ݿ�����¾��)",
    "��g���㪺��}��� (�t�����W��, �x�_���Чt��F�ϰ�)",
    "�Զ�s���q�� (�t�ϽX, �������[ '-', '(', ')' ���Ÿ�)",
    "��T�ç����g���U�ӽЪ�",
    "�Τ����g�ӽг�",
};

#define REASON_FIRSTABBREV '0'
#define REASON_IN_ABBREV(x) \
    ((x) >= REASON_FIRSTABBREV && (x) - REASON_FIRSTABBREV < REJECT_REASONS)
#define REASON_EXPANDABBREV(x)	 reasonstr[(x) - REASON_FIRSTABBREV]

void 
regform_accept(const char *userid, const char *justify)
{
    char buf[PATHLEN];
    int unum = 0;
    userec_t muser;

    unum = getuser(userid, &muser);
    if (unum == 0)
	return; // invalid user

    muser.userlevel |= (PERM_LOGINOK | PERM_POST);
    strlcpy(muser.justify, justify, sizeof(muser.justify));
    // manual accept sets email to 'x'
    strlcpy(muser.email, "x", sizeof(muser.email)); 

    // handle files
    sethomefile(buf, muser.userid, FN_JUSTIFY_WAIT);
    unlink(buf);
    sethomefile(buf, muser.userid, FN_REJECT_NOTIFY);
    unlink(buf);
    sethomefile(buf, muser.userid, FN_JUSTIFY);
    log_filef(buf, LOG_CREAT, "%s\n", muser.justify);

    // update password file
    passwd_update(unum, &muser);

    // alert online users?
    sendalert(muser.userid,  ALERT_PWD_PERM|ALERT_PWD_JUSTIFY); // force to reload perm

#if FOREIGN_REG_DAY > 0
    if(muser.uflag2 & FOREIGN)
	mail_muser(muser, "[�X�J�Һ޲z��]", "etc/foreign_welcome");
    else
#endif
    // last: send notification mail
    mail_muser(muser, "[���U���\\�o]", "etc/registered");
}

void 
regform_reject(const char *userid, char *reason)
{
    char buf[PATHLEN];
    FILE *fp = NULL;
    int unum = 0;
    userec_t muser;

    unum = getuser(userid, &muser);
    if (unum == 0)
	return; // invalid user

    muser.userlevel &= ~(PERM_LOGINOK | PERM_POST);

    // handle files
    sethomefile(buf, muser.userid, FN_JUSTIFY_WAIT);
    unlink(buf);

    // update password file
    passwd_update(unum, &muser);

    // alert notify users?
    sendalert(muser.userid,  ALERT_PWD_PERM); // force to reload perm

    // last: send notification
    mkuserdir(muser.userid);
    sethomefile(buf, muser.userid, "justify.reject");
    fp = fopen(buf, "wt");
    assert(fp);
    syncnow();
    fprintf(fp, "%s ���U���ѡC\n", Cdate(&now));

    // multiple abbrev loop
    if (REASON_IN_ABBREV(reason[0]))
    {
	int i = 0;
	for (i = 0; i < REASON_LEN && REASON_IN_ABBREV(reason[i]); i++)
	    fprintf(fp, "[�h�^��]] ��%s\n", REASON_EXPANDABBREV(reason[i]));
    } else {
	fprintf(fp, "[�h�^��]] %s\n", reason);
    }
    fclose(fp);
    mail_muser(muser, "[���U����]", buf);
}

// read count entries from regsrc to a temp buffer
FILE *
pull_regform(const char *regfile, char *workfn, int count)
{
    FILE *fp = NULL;

    snprintf(workfn, PATHLEN, "%s.tmp", regfile);
    if (dashf(workfn)) {
	vmsg("��L SYSOP �]�b�f�ֵ��U�ӽг�");
	return NULL;
    }

    // count < 0 means unlimited pulling
    Rename(regfile, workfn);
    if ((fp = fopen(workfn, "r")) == NULL) {
	vmsgf("�t�ο��~�A�L�kŪ�����U�����: %s", workfn);
	return NULL;
    }
    return fp;
}

// write all left in "remains" to regfn.
void
pump_regform(const char *regfn, FILE *remains)
{
    // restore trailing tickets
    char buf[PATHLEN];
    FILE *fout = fopen(regfn, "at");
    if (!fout)
	return;

    while (fgets(buf, sizeof(buf), remains))
	fputs(buf, fout);
    fclose(fout);
}

static void
prompt_regform_ui()
{
    move(b_lines, 0);
    outs(ANSI_COLOR(30;47)  "  "
	    ANSI_COLOR(31) "y" ANSI_COLOR(30) "���� "
	    ANSI_COLOR(31) "n" ANSI_COLOR(30) "�ڵ� "
	    ANSI_COLOR(31) "d" ANSI_COLOR(30) "�R�� "
	    ANSI_COLOR(31) "s" ANSI_COLOR(30) "���L "
	    ANSI_COLOR(31) "u" ANSI_COLOR(30) "�_�� "
	    " "
	    ANSI_COLOR(31) "0-9jk����" ANSI_COLOR(30) "���� "
	    ANSI_COLOR(31) "�ť�/PgDn" ANSI_COLOR(30) "�x�s+�U�� "
	    " "
	    ANSI_COLOR(31) "q/END" ANSI_COLOR(30) "����  "
	    ANSI_RESET);
}

static void
resolve_reason(char *s, int y)
{
    // should start with REASON_FIRSTABBREV
    const char *reason_prompt = 
	" (0)�u��m�W (1)�Զ�t�� (2)�����}"
	" (3)�Զ�q�� (4)�T���g (5)�����g";

    s[0] = 0;
    move(y, 0);
    outs(reason_prompt); outs("\n");

    do {
	getdata(y+1, 0, 
		"�h�^��]: ", s, REASON_LEN, DOECHO);

	// convert abbrev reasons (format: single digit, or multiple digites)
	if (REASON_IN_ABBREV(s[0]))
	{
	    if (s[1] == 0) // simple replace ment
	    {
		strlcpy(s+2, REASON_EXPANDABBREV(s[0]),
			REASON_LEN-2);
		s[0] = 0xbd; // '��'[0];
		s[1] = 0xd0; // '��'[1];
	    } else {
		// strip until all digites
		char *p = s;
		while (*p)
		{
		    if (!REASON_IN_ABBREV(*p))
			*p = ' ';
		    p++;
		}
		strip_blank(s, s);
		strlcat(s, " [�h����]]", REASON_LEN);
	    }
	} 

	if (strlen(s) < 4)
	{
	    if (vmsg("��]�ӵu�C �n�����h�^�ܡH (y/N): ") == 'y')
	    {
		*s = 0;
		return;
	    }
	}
    } while (strlen(s) < 4);
}

// TODO define and use structure instead, even in reg request file.
typedef struct {
    // current format:
    // (optional) num: unum, date
    // [0] uid: xxxxx	(IDLEN=12)
    // [1] name: RRRRRR (20)
    // [2] career: YYYYYYYYYYYYYYYYYYYYYYYYYY (40)
    // [3] addr: TTTTTTTTT (50)
    // [4] phone: 02DDDDDDDD (20)
    // [5] email: x (50) (deprecated)
    // [6] mobile: (deprecated)
    // [7] ----
    //     lasthost: 16
    char userid[IDLEN+1];

    char exist;
    char online;
    char pad   [ 5];     // IDLEN(12)+1+1+1+5=20

    char name  [20];
    char career[40];
    char addr  [50];
    char phone [20];
} RegformEntry;

int
load_regform_entry(RegformEntry *pre, FILE *fp)
{
    char buf[STRLEN];
    char *v;

    memset(pre, 0, sizeof(RegformEntry));
    while (fgets(buf, sizeof(buf), fp))
    {
	if (buf[0] == '-')
	    break;
	buf[sizeof(buf)-1] = 0;
	v = strchr(buf, ':');
	if (v == NULL)
	    continue;
	*v++ = 0;
	if (*v == ' ') v++;
	chomp(v);

	if (strcmp(buf, "uid") == 0)
	    strlcpy(pre->userid, v, sizeof(pre->userid));
	else if (strcmp(buf, "name") == 0)
	    strlcpy(pre->name, v, sizeof(pre->name));
	else if (strcmp(buf, "career") == 0)
	    strlcpy(pre->career, v, sizeof(pre->career));
	else if (strcmp(buf, "addr") == 0)
	    strlcpy(pre->addr, v, sizeof(pre->addr));
	else if (strcmp(buf, "phone") == 0)
	    strlcpy(pre->phone, v, sizeof(pre->phone));
    }
    return pre->userid[0] ? 1 : 0;
}

int
print_regform_entry(const RegformEntry *pre, FILE *fp, int close)
{
    fprintf(fp, "uid: %s\n",	pre->userid);
    fprintf(fp, "name: %s\n",	pre->name);
    fprintf(fp, "career: %s\n", pre->career);
    fprintf(fp, "addr: %s\n",	pre->addr);
    fprintf(fp, "phone: %s\n",	pre->phone);
    fprintf(fp, "email: %s\n",	"x");
    if (close)
	fprintf(fp, "----\n");
    return 1;
}

int
append_regform(const RegformEntry *pre, const char *logfn, 
	const char *varname, const char *varval1, const char *varval2)
{
    FILE *fout = fopen(logfn, "at");
    if (!fout)
	return 0;

    print_regform_entry(pre, fout, 0);
    if (varname && *varname)
    {
	syncnow();
	fprintf(fout, "Date: %s\n", Cdate(&now));
	if (!varval1) varval1 = "";
	fprintf(fout, "%s: %s", varname, varval1);
	if (varval2) fprintf(fout, " %s", varval2);
	fprintf(fout, "\n");
    }
    // close it
    fprintf(fout, "----\n");
    fclose(fout);
    return 1;
}

/////////////////////////////////////////////////////////////////////////////
// Regform UI 
// �B�z Register Form
/////////////////////////////////////////////////////////////////////////////

/* Auto-Regform-Scan
 * FIXME �u�O�@�ΩU��
 *
 * fdata �ΤF�Ӧh magic number
 * return value ���ӬO�� reason (return index + 1)
 * ans[0] �����O�b�޿�ܪ��u���~�����v (Register ���̬ݨ쪺����)
 */
static int
auto_scan(char fdata[][STRLEN], char ans[])
{
    int             good = 0;
    int             count = 0;
    int             i;
    char            temp[10];

    if (!strncmp(fdata[1], "�p", 2) || strstr(fdata[1], "�X")
	|| strstr(fdata[1], "��") || strstr(fdata[1], "��")) {
	ans[0] = '0';
	return 1;
    }
    strlcpy(temp, fdata[1], 3);

    /* �|�r */
    if (!strncmp(temp, &(fdata[1][2]), 2)) {
	ans[0] = '0';
	return 1;
    }
    if (strlen(fdata[1]) >= 6) {
	if (strstr(fdata[1], "������")) {
	    ans[0] = '0';
	    return 1;
	}
	if (strstr("�����]���P�d�G��", temp))
	    good++;
	else if (strstr("���C���L���x�E���B", temp))
	    good++;
	else if (strstr("Ĭ��d�f����i����Ĭ", temp))
	    good++;
	else if (strstr("�}�¥ۿc�I���έ�", temp))
	    good++;
    }
    if (!good)
	return 0;

    if (!strcmp(fdata[2], fdata[3]) ||
	!strcmp(fdata[2], fdata[4]) ||
	!strcmp(fdata[3], fdata[4])) {
	ans[0] = '4';
	return 5;
    }
    if (strstr(fdata[2], "�j")) {
	if (strstr(fdata[2], "�x") || strstr(fdata[2], "�H") ||
	    strstr(fdata[2], "��") || strstr(fdata[2], "�F") ||
	    strstr(fdata[2], "�M") || strstr(fdata[2], "ĵ") ||
	    strstr(fdata[2], "�v") || strstr(fdata[2], "�ʶ�") ||
	    strstr(fdata[2], "����") || strstr(fdata[2], "��") ||
	    strstr(fdata[2], "��") || strstr(fdata[2], "�F�d"))
	    good++;
    } else if (strstr(fdata[2], "�k��"))
	good++;

    if (strstr(fdata[3], "�a�y") || strstr(fdata[3], "�t�z") ||
	strstr(fdata[3], "�H�c")) {
	ans[0] = '2';
	return 3;
    }
    if (strstr(fdata[3], "��") || strstr(fdata[3], "��")) {
	if (strstr(fdata[3], "��") || strstr(fdata[3], "��")) {
	    if (strstr(fdata[3], "��"))
		good++;
	}
    }
    for (i = 0; fdata[4][i]; i++) {
	if (isdigit((int)fdata[4][i]))
	    count++;
    }

    if (count <= 4) {
	ans[0] = '3';
	return 4;
    } else if (count >= 7)
	good++;

    if (good >= 3) {
	ans[0] = 'y';
	return -1;
    } else
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Traditional Regform UI
/////////////////////////////////////////////////////////////////////////////
// TODO XXX process someone directly, according to target_uid.
int
scan_register_form(const char *regfile, int automode, const char *target_uid)
{
    char            genbuf[200];
    char    *field[] = {
	"uid", "name", "career", "addr", "phone", "email", NULL
    };
    char    *finfo[] = {
	"�b��", "�u��m�W", "�A�ȳ��", "�ثe��}",
	"�s���q��", "�q�l�l��H�c", NULL
    };
    char    *reason[REJECT_REASONS+1] = {
	"��J�u��m�W",
	"�Զ�u(���~)�ǮդΡy�t�z�y�šz�v�Ρu�A�ȳ��(�t���ݿ�����¾��)�v",
	"��g���㪺��}��� (�t�����W��, �x�_���Чt��F�ϰ�^",
	"�Զ�s���q�� (�t�ϰ�X, �������Υ[ '-', '(', ')'���Ÿ�",
	"��T�ç����g���U�ӽЪ�",
	"�Τ����g�ӽг�",
	NULL
    };
    char    *autoid = "AutoScan";
    userec_t        muser;
    FILE           *fn, *fout, *freg;
    char            fdata[6][STRLEN];
    char            fname[STRLEN] = "", buf[STRLEN];
    char            ans[4], *ptr, *uid;
    int             n = 0, unum = 0, tid = 0;
    int             nSelf = 0, nAuto = 0;

    uid = cuser.userid;
    move(2, 0);

    fn = pull_regform(regfile, fname, -1);
    if (!fn)
	return -1;

    while( fgets(genbuf, STRLEN, fn) ){
	memset(fdata, 0, sizeof(fdata));
	do {
	    if( genbuf[0] == '-' )
		break;
	    if ((ptr = (char *)strstr(genbuf, ": "))) {
		*ptr = '\0';
		for (n = 0; field[n]; n++) {
		    if (strcmp(genbuf, field[n]) == 0) {
			strlcpy(fdata[n], ptr + 2, sizeof(fdata[n]));
			if ((ptr = (char *)strchr(fdata[n], '\n')))
			    *ptr = '\0';
		    }
		}
	    }
	} while( fgets(genbuf, STRLEN, fn) );
	tid ++;

	if ((unum = getuser(fdata[0], &muser)) == 0) {
	    move(2, 0);
	    clrtobot();
	    outs("�t�ο��~�A�d�L���H\n\n");
	    for (n = 0; field[n]; n++)
		prints("%s     : %s\n", finfo[n], fdata[n]);
	    pressanykey();
	} else {
	    if (automode)
		uid = autoid;

	    if ((!automode || !auto_scan(fdata, ans))) {
		uid = cuser.userid;

		move(1, 0);
		clrtobot();
		prints("�b����m    : %d\n", unum);
		user_display(&muser, 1);
		move(14, 0);
		prints(ANSI_COLOR(1;32) "------------- "
			"�Я����Y��f�֨ϥΪ̸�ơA�o�O�� %d ��"
			"------------" ANSI_RESET "\n", tid);
	    	prints("  %-12s: %s\n", finfo[0], fdata[0]);
#ifdef FOREIGN_REG
		prints("0.%-12s: %s%s\n", finfo[1], fdata[1],
		       muser.uflag2 & FOREIGN ? " (�~�y)" : "");
#else
		prints("0.%-12s: %s\n", finfo[1], fdata[1]);
#endif
		for (n = 2; field[n]; n++) {
		    prints("%d.%-12s: %s\n", n - 1, finfo[n], fdata[n]);
		}
		if (muser.userlevel & PERM_LOGINOK) {
		    ans[0] = getkey("���b���w�g�������U, "
				    "��s(Y/N/Skip)�H[N] ");
		    if (ans[0] != 'y' && ans[0] != 's')
			ans[0] = 'd';
		} else {
		    if (search_ulist(unum) == NULL)
		    {
			move(b_lines, 0); clrtoeol();
			outs("�O�_���������(Y/N/Q/Del/Skip)�H[S] ");
			// FIXME if the user got online here
		        ans[0] = igetch();
		    }
		    else
			ans[0] = 's';
		    ans[0] = tolower(ans[0]);
		    if (ans[0] != 'y' && ans[0] != 'n' && 
			ans[0] != 'q' && ans[0] != 'd' && 
			!('0' <= ans[0] && ans[0] < ('0' + REJECT_REASONS)))
			ans[0] = 's';
		    ans[1] = 0;
		}
		nSelf++;
	    } else
		nAuto++;

	    switch (ans[0]) {
	    case 'q':
		if ((freg = fopen(regfile, "a"))) {
		    for (n = 0; field[n]; n++)
			fprintf(freg, "%s: %s\n", field[n], fdata[n]);
		    fprintf(freg, "----\n");
		    while (fgets(genbuf, STRLEN, fn))
			fputs(genbuf, freg);
		    fclose(freg);
		}
	    case 'd':
		break;

	    case '0': case '1': case '2':
	    case '3': case '4': case '5':
		/* please confirm match REJECT_REASONS here */
	    case 'n':
		if (ans[0] == 'n') {
		    int nf = 0;
		    move(8, 0);
		    clrtobot();
		    outs("�д��X�h�^�ӽЪ��]�A�� <enter> ����\n");
		    for (n = 0; n < REJECT_REASONS; n++)
			prints("%d) ��%s\n", n, reason[n]);
		    outs("\n"); // preserved for prompt
		    for (nf = 0; field[nf]; nf++)
			prints("%s: %s\n", finfo[nf], fdata[nf]);
		} else
		    buf[0] = ans[0];

		if (ans[0] != 'n' ||
		    getdata(9 + n, 0, "�h�^��]: ", buf, 60, DOECHO))
		    if ((buf[0] - '0') >= 0 && (buf[0] - '0') < n) {
			int             i;
			fileheader_t    mhdr;
			char            title[128], buf1[80];
			FILE           *fp;

			sethomepath(buf1, muser.userid);
			stampfile(buf1, &mhdr);
			strlcpy(mhdr.owner, cuser.userid, sizeof(mhdr.owner));
			strlcpy(mhdr.title, "[���U����]", TTLEN);
			mhdr.filemode = 0;
			sethomedir(title, muser.userid);
			if (append_record(title, &mhdr, sizeof(mhdr)) != -1) {
			    char rejfn[PATHLEN];
			    fp = fopen(buf1, "w");
			    
			    for(i = 0; buf[i] && i < sizeof(buf); i++){
				if (buf[i] >= '0' && buf[i] < '0'+n)
				{
				    fprintf(fp, "[�h�^��]] ��%s\n",
					    reason[buf[i] - '0']);
				}
			    }

			    fclose(fp);

			    // build reject file
			    setuserfile(rejfn, "justify.reject");
			    Copy(buf1, rejfn);
			}
			if ((fout = fopen(FN_REGISTER_LOG, "a"))) {
			    for (n = 0; field[n]; n++)
				fprintf(fout, "%s: %s\n", field[n], fdata[n]);
			    fprintf(fout, "Date: %s\n", Cdate(&now));
			    fprintf(fout, "Rejected: %s [%s]\n----\n",
				    uid, buf);
			    fclose(fout);
			}
			break;
		    }
		move(10, 0);
		clrtobot();
		outs("�����h�^�����U�ӽЪ�");
		/* no break? */

	    case 's':
		if ((freg = fopen(regfile, "a"))) {
		    for (n = 0; field[n]; n++)
			fprintf(freg, "%s: %s\n", field[n], fdata[n]);
		    fprintf(freg, "----\n");
		    fclose(freg);
		}
		break;

	    default:
		outs("�H�U�ϥΪ̸�Ƥw�g��s:\n");
		mail_muser(muser, "[���U���\\�o]", "etc/registered");

#if FOREIGN_REG_DAY > 0
		if(muser.uflag2 & FOREIGN)
		    mail_muser(muser, "[�X�J�Һ޲z��]", "etc/foreign_welcome");
#endif

		muser.userlevel |= (PERM_LOGINOK | PERM_POST);
		strlcpy(muser.realname, fdata[1], sizeof(muser.realname));
		strlcpy(muser.address, fdata[3], sizeof(muser.address));
		strlcpy(muser.email, fdata[5], sizeof(muser.email));
		snprintf(genbuf, sizeof(genbuf), "%s:%s:%s",
			 fdata[4], fdata[2], uid);
		strlcpy(muser.justify, genbuf, sizeof(muser.justify));

		passwd_update(unum, &muser);
		// XXX TODO notify users?
		sendalert(muser.userid,  ALERT_PWD_PERM); // force to reload perm

		sethomefile(buf, muser.userid, FN_JUSTIFY);
		log_file(buf, LOG_CREAT, genbuf);

		if ((fout = fopen(FN_REGISTER_LOG, "a"))) {
		    for (n = 0; field[n]; n++)
			fprintf(fout, "%s: %s\n", field[n], fdata[n]);
		    fprintf(fout, "Date: %s\n", Cdate(&now));
		    fprintf(fout, "Approved: %s\n", uid);
		    fprintf(fout, "----\n");
		    fclose(fout);
		}
		sethomefile(genbuf, muser.userid, FN_JUSTIFY_WAIT);
		unlink(genbuf);
		break;
	    }
	}
    }

    fclose(fn);
    unlink(fname);

    move(0, 0);
    clrtobot();

    move(5, 0);
    prints("�z�f�F %d �����U��AAutoScan �f�F %d ��", nSelf, nAuto);

    pressanykey();
    return (0);
}

/////////////////////////////////////////////////////////////////////////////
// New Regform UI
/////////////////////////////////////////////////////////////////////////////

// #define REGFORM_DISABLE_ONLINE_USER
#define FORMS_IN_PAGE (10)

int
handle_register_form(const char *regfile, int dryrun)
{
    int unum = 0;
    int yMsg = FORMS_IN_PAGE*2+1;
    FILE *fp = NULL;
    userec_t muser;
    RegformEntry forms [FORMS_IN_PAGE];
    char ans	[FORMS_IN_PAGE];
    char rejects[FORMS_IN_PAGE][REASON_LEN];	// reject reason length
    char fname  [PATHLEN] = "";
    char justify[REGLEN+1];
    char rsn	[REASON_LEN];
    int cforms = 0,	// current loaded forms
	parsed = 0,	// total parsed forms
	ci = 0, // cursor index
	ch = 0,	// input key
	i, blanks;
    long fsz = 0, fpos = 0;

    // prepare reg tickets
    if (dryrun)
    {
	// directly open regfile to try
	fp = fopen(regfile, "rt");
    } else {
	fp = pull_regform(regfile, fname, -1);
    }

    if (!fp)
	return 0;

    // retreieve file info
    fpos = ftell(fp);
    fseek(fp, 0, SEEK_END);
    fsz = ftell(fp);
    fseek(fp, fpos, SEEK_SET);
    if (!fsz) fsz = 1;

    while (ch != 'q')
    {
	// initialize and prepare
	memset(ans, 0, sizeof(ans));
	memset(rejects, 0, sizeof(rejects));
	memset(forms, 0, sizeof(forms));
	cforms = 0;

	// load forms
	while (cforms < FORMS_IN_PAGE && load_regform_entry(&forms[cforms], fp))
	    cforms++, parsed ++;

	// if no more forms then leave.
	// TODO what if regform error?
	if (cforms < 1)
	    break;

	// adjust cursor if required
	if (ci >= cforms)
	    ci = cforms-1;

	// display them all.
	clear();
	for (i = 0; i < cforms; i++)
	{
	    // fetch user information
	    memset(&muser, 0, sizeof(muser));
	    unum = getuser(forms[i].userid, &muser);
	    forms[i].exist = unum ? 1 : 0;
	    if (unum) forms[i].online = search_ulist(unum) ? 1 : 0;

	    // if already got login level, delete by default.
	    if (!unum)
		ans[i] = 'd';
	    else {
		if (muser.userlevel & PERM_LOGINOK)
		    ans[i] = 'd';
#ifdef REGFORM_DISABLE_ONLINE_USER
		else if (forms[i].online)
		    ans[i] = 's';
#endif // REGFORM_DISABLE_ONLINE_USER
	    }

	    // print
	    move(i*2, 0);
	    prints("  %2d%s %s%-12s " ANSI_RESET, 
		    i+1, 
		    (unum == 0) ? ANSI_COLOR(1;31) "D" :
		    ( (muser.userlevel & PERM_LOGINOK) ? 
		      ANSI_COLOR(1;33) "Y" : 
#ifdef REGFORM_DISABLE_ONLINE_USER
			  forms[i].online ? "s" : 
#endif
			  "."),
		    forms[i].online ?  ANSI_COLOR(1;35) : ANSI_COLOR(1),
		    forms[i].userid);

	    prints( ANSI_COLOR(1;31) "%19s " 
		    ANSI_COLOR(1;32) "%-40s" ANSI_RESET"\n", 
		    forms[i].name, forms[i].career);

	    move(i*2+1, 0); 
	    prints("    %s %-50s%20s\n", 
		    (muser.userlevel & PERM_NOREGCODE) ? 
		    ANSI_COLOR(1;31) "T" ANSI_RESET : " ",
		    forms[i].addr, forms[i].phone);
	}

	// display page info
	{
	    char msg[STRLEN];
	    fpos = ftell(fp);
	    if (fpos > fsz) fsz = fpos*10;
	    snprintf(msg, sizeof(msg),
		    " �w��� %d �����U�� (%2d%%)  ",
		    parsed, (int)(fpos*100/fsz));
	    prints(ANSI_COLOR(7) "\n%78s" ANSI_RESET "\n", msg);
	}

	// handle user input
	prompt_regform_ui();
	ch = 0;
	while (ch != 'q' && ch != ' ') {
	    ch = cursor_key(ci*2, 0);
	    switch (ch)
	    {
		// nav keys
		case KEY_UP:
		case 'k':
		    if (ci > 0) ci--;
		    break;

		case KEY_DOWN:
		case 'j':
		    ch = 'j'; // go next
		    break;

		    // quick nav (assuming to FORMS_IN_PAGE=10)
		case '1': case '2': case '3': case '4': case '5':
		case '6': case '7': case '8': case '9':
		    ci = ch - '1';
		    if (ci >= cforms) ci = cforms-1;
		    break;
		case '0':
		    ci = 10-1;
		    if (ci >= cforms) ci = cforms-1;
		    break;

		    /*
		case KEY_HOME: ci = 0; break;
		case KEY_END:  ci = cforms-1; break;
		    */

		    // abort
		case KEY_END:
		case 'q':
		    ch = 'q';
		    if (getans("�T�w�n���}�F�ܡH (�����ܧ�N���|�x�s) [y/N]: ") != 'y')
		    {
			prompt_regform_ui();
			ch = 0;
			continue;
		    }
		    break;

		    // prepare to go next page
		case KEY_PGDN:
		case ' ':
		    ch = ' ';

		    // solving blank (undecided entries)
		    for (i = 0, blanks = 0; i < cforms; i++)
			if (ans[i] == 0) blanks ++;

		    if (!blanks)
			break;

		    // have more blanks
		    ch = getans("�|�����w�� %d �Ӷ��حn: (S���L/y�q�L/n�ڵ�/e�~��s��): ", 
			    blanks);

		    if (ch == 'e')
		    {
			prompt_regform_ui();
			ch = 0;
			continue;
		    }
		    if (ch == 'y') {
			// do nothing.
		    } else if (ch == 'n') {
			// query reject reason
			resolve_reason(rsn, yMsg);
			if (*rsn == 0)
			    ch = 's';
		    } else ch = 's';

		    // filling answers
		    for (i = 0; i < cforms; i++)
		    {
			if (ans[i] != 0)
			    continue;
			ans[i] = ch;
			if (ch != 'n')
			    continue;
			strlcpy(rejects[i], rsn, REASON_LEN);
		    }

		    ch = ' '; // go to page mode!
		    break;

		    // function keys
		case 'y':	// accept
#ifdef REGFORM_DISABLE_ONLINE_USER
		    if (forms[ci].online)
		    {
			vmsg("�Ȥ��}��f�֦b�u�W�ϥΪ̡C");
			break;
		    }
#endif
		case 's':	// skip
		case 'd':	// delete
		case KEY_DEL:	//delete
		    if (ch == KEY_DEL) ch = 'd';

		    grayout(ci*2, ci*2+1, GRAYOUT_DARK);
		    move_ansi(ci*2, 4); outc(ch);
		    ans[ci] = ch;
		    ch = 'j'; // go next
		    break;

		case 'u':	// undo
#ifdef REGFORM_DISABLE_ONLINE_USER
		    if (forms[ci].online)
		    {
			vmsg("�Ȥ��}��f�֦b�u�W�ϥΪ̡C");
			break;
		    }
#endif
		    grayout(ci*2, ci*2+1, GRAYOUT_NORM);
		    move_ansi(ci*2, 4); outc('.');
		    ans[ci] = 0;
		    ch = 'j'; // go next
		    break;

		case 'n':	// reject
#ifdef REGFORM_DISABLE_ONLINE_USER
		    if (forms[ci].online)
		    {
			vmsg("�Ȥ��}��f�֦b�u�W�ϥΪ̡C");
			break;
		    }
#endif
		    // query for reason
		    resolve_reason(rejects[ci], yMsg);
		    prompt_regform_ui();

		    if (!rejects[ci][0])
			break;

		    move(yMsg, 0);
		    prints("�h�^ %s ���U���]:\n %s\n", forms[ci].userid, rejects[ci]);

		    // do reject
		    grayout(ci*2, ci*2+1, GRAYOUT_DARK);
		    move_ansi(ci*2, 4); outc(ch);
		    ans[ci] = ch;
		    ch = 'j'; // go next

		    break;
	    } // switch(ch)

	    // change cursor
	    if (ch == 'j' && ++ci >= cforms)
		ci = cforms -1;
	} // while(ch != QUIT/SAVE)

	// if exit, we still need to skip all read forms
	if (ch == 'q')
	{
	    for (i = 0; i < cforms; i++)
		ans[i] = 's';
	}

	// page complete (save).
	assert(ch == ' ' || ch == 'q');

	// save/commit if required.
	if (dryrun) 
	{
	    // prmopt for debug
	    clear();
	    stand_title("���ռҦ�");
	    outs("�z���b������ռҦ��A�ҥH��f�����U��ä��|�ͮġC\n"
		    "�U���C�X���O��~�z�f�������G:\n\n");

	    for (i = 0; i < cforms; i++)
	    {
		if (ans[i] == 'y')
		    snprintf(justify, sizeof(justify), // build justify string
			    "%s:%s:%s", forms[i].phone, forms[i].career, cuser.userid);

		prints("%2d. %-12s - %c %s\n", i+1, forms[i].userid, ans[i],
			ans[i] == 'n' ? rejects[i] : 
			ans[i] == 'y' ? justify : "");
	    }
	    if (ch != 'q')
		pressanykey();
	} 
	else 
	{
	    // real functionality
	    for (i = 0; i < cforms; i++)
	    {
		if (ans[i] == 'y')
		{
		    // build justify string
		    snprintf(justify, sizeof(justify), 
			    "%s:%s:%s", forms[i].phone, forms[i].career, cuser.userid);

		    regform_accept(forms[i].userid, justify);
		    // log form to FN_REGISTER_LOG
		    append_regform(&forms[i], FN_REGISTER_LOG,
			    "Approved", cuser.userid, NULL);
		}
		else if (ans[i] == 'n')
		{
		    regform_reject(forms[i].userid, rejects[i]);
		    // log form to FN_REGISTER_LOG
		    append_regform(&forms[i], FN_REGISTER_LOG,
			    "Rejected", cuser.userid, rejects[i]);
		}
		else if (ans[i] == 's')
		{
		    // append form back to fn_register
		    append_regform(&forms[i], fn_register,
			    NULL, NULL, NULL);
		}
	    }
	} // !dryrun

    } // while (ch != 'q')

    // cleaning left regforms
    if (!dryrun)
    {
	pump_regform(regfile, fp);
	fclose(fp);
        unlink(fname);
    } else {
	// directly close file should be OK.
	fclose(fp);
    }

    return 0;
}

int
m_register(void)
{
    FILE           *fn;
    int             x, y, wid, len;
    char            ans[4];
    char            genbuf[200];

    if ((fn = fopen(fn_register, "r")) == NULL) {
	outs("�ثe�õL�s���U���");
	return XEASY;
    }
    stand_title("�f�֨ϥΪ̵��U���");
    y = 2;
    x = wid = 0;

    while (fgets(genbuf, STRLEN, fn) && x < 65) {
	if (strncmp(genbuf, "uid: ", 5) == 0) {
	    move(y++, x);
	    outs(genbuf + 5);
	    len = strlen(genbuf + 5);
	    if (len > wid)
		wid = len;
	    if (y >= t_lines - 3) {
		y = 2;
		x += wid + 2;
	    }
	}
    }
    fclose(fn);
    getdata(b_lines - 1, 0, 
	    "�}�l�f�ֶ�(Auto�۰�/Yes���/No���f/Exp�s�ɭ�)�H[N] ", 
	    ans, sizeof(ans), LCECHO);
    if (ans[0] == 'a')
	scan_register_form(fn_register, 1, NULL);
    else if (ans[0] == 'y')
	scan_register_form(fn_register, 0, NULL);
    else if (ans[0] == 'e')
    {
#ifdef EXP_ADMIN_REGFORM_DRYRUN
	int dryrun = 0;
	if (getans("�A�n�i��´���(T)�٬O�u������f��(y)�H") == 'y')
	{
	    vmsg("�i�J��ڰ���Ҧ��A�Ҧ��f�ְʧ@���O�u���C");
	    dryrun = 0;
	} else {
	    vmsg("���ռҦ��C");
	    dryrun = 1;
	}
	handle_register_form(fn_register, dryrun);
#else
	// run directly.
	handle_register_form(fn_register, 0);
#endif
    }

    return 0;
}

int
cat_register(void)
{
    if (system("cat register.new.tmp >> register.new") == 0 &&
	unlink("register.new.tmp") == 0)
	vmsg("OK �P~~ �~��h�İ��a!!");
    else
	vmsg("�S��kCAT�L�h�O �h�ˬd�@�U�t�Χa!!");
    return 0;
}

/* vim:sw=4
 */
