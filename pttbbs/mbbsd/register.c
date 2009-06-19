/* $Id$ */
#include "bbs.h"

#define FN_REGISTER_LOG  "register.log"	// global registration history
#define FN_REJECT_NOTIFY "justify.reject"
#define FN_NOTIN_WHITELIST_NOTICE "etc/whitemail.notice"

// Regform1 file name (deprecated)
#define fn_register	"register.new"

// New style (Regform2) file names:
#define FN_REGFORM	"regform"	// registration form in user home
#define FN_REGFORM_LOG	"regform.log"	// regform history in user home
#define FN_REQLIST	"reg.wait"	// request list file, in global directory (replacing fn_register)

#define FN_REJECT_NOTES	"etc/reg_reject.notes"

#define FN_JOBSPOOL_DIR	"jobspool/"

// #define DBG_DISABLE_CHECK	// disable all input checks
// #define DBG_DRYRUN	// Dry-run test (mainly for RegForm2)

#define MSG_ERR_MAXTRIES "�z���տ��~����J���ƤӦh�A�ФU���A�ӧa"
#define MSG_ERR_TOO_OLD  "�~���i�঳�~�C �Y�o�O�z���u��ͤ�Хt��q�����ȳB�z"
#define MSG_ERR_TOO_YOUNG "�~�����~�C ����/���X�����ӵL�k�ϥ� BBS..."
#define DATE_SAMPLE	 "1911/2/29"

////////////////////////////////////////////////////////////////////////////
// Value Validation
////////////////////////////////////////////////////////////////////////////
static int 
HaveRejectStr(const char *s, const char **rej)
{
    int     i;
    char    *rejectstr[] =
	{"�F", "��", "�A��", "�Y", "��", "�b", "..", "xx",
	 "�A��", "�ާ�", "�q", "�Ѥ~", "�W�H", 
	 /* "��",  (�Y�Ǧ�}��) */
	 "�t", "�u", "�v", "�w", "�x", "�y", "�z", "�{", "�|", "�}", "�~",
	 "��", "��", "��", "��", "��", "��", "��", "��", "��", "��",
	 "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��",
	 "��", "��", "��", "��", "��", NULL};

    if( rej != NULL )
	for( i = 0 ; rej[i] != NULL ; ++i )
	    if( DBCS_strcasestr(s, rej[i]) )
		return 1;

    for( i = 0 ; rejectstr[i] != NULL ; ++i )
	if( DBCS_strcasestr(s, rejectstr[i]) )
	    return 1;

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
reserved_user_id(const char *userid)
{
    if (file_exist_record(FN_CONF_RESERVED_ID, userid))
       return 1;
    return 0;
}

int
bad_user_id(const char *userid)
{
    if(!is_validuserid(userid))
	return 1;

#if defined(STR_REGNEW)
    if (strcasecmp(userid, STR_REGNEW) == 0)
	return 1;
#endif

#if defined(STR_GUEST) && !defined(NO_GUEST_ACCOUNT_REG)
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
    return "�z����J���G�����T";
#endif

}

static char *
isvalidcareer(char *career)
{
#ifndef FOREIGN_REG
    const char    *rejectstr[] = {NULL};
    if (!(career[0] < 0 && strlen(career) >= 6) ||
	strcmp(career, "�a��") == 0 || HaveRejectStr(career, rejectstr) )
	return "�z����J���G�����T";
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
    if (DBCS_strcasestr(career, "��") && 
	DBCS_strcasestr(career, "�t") &&
	DBCS_strcasestr(career, "��") == 0 && 
	(DBCS_strcasestr(career, "��") == 0 && 
	 DBCS_strcasestr(career, "�w") == 0))
	return "�Х[�W�~��";
    return NULL;
}

static int
strlen_without_space(const char *s)
{
    int i = 0;
    while (*s)
	if (*s++ != ' ') i++;
    return i;
}

static char *
isvalidaddr(char *addr)
{
    const char    *rejectstr[] =
	{"�a�y", "�Ȫe", "���P", NULL};

#ifdef DBG_DISABLE_CHECK
    return NULL;
#endif // DBG_DISABLE_CHECK

    // addr[0] > 0: check if address is starting by Chinese.
    if (DBCS_strcasestr(addr, "�H�c") != 0 || DBCS_strcasestr(addr, "�l�F") != 0) 
	return "��p�ڭ̤������l�F�H�c";
    if (strlen_without_space(addr) < 15 ||
	(DBCS_strcasestr(addr, "��") == 0 && 
	 DBCS_strcasestr(addr, "�]") == 0 &&
	 DBCS_strcasestr(addr, "��") == 0 && 
	 DBCS_strcasestr(addr, "��") == 0) ||
	strcmp(&addr[strlen(addr) - 2], "�q") == 0 ||
	strcmp(&addr[strlen(addr) - 2], "��") == 0 ||
	strcmp(&addr[strlen(addr) - 2], "��") == 0 ||
	strcmp(&addr[strlen(addr) - 2], "��") == 0 ||
	strcmp(&addr[strlen(addr) - 2], "��") == 0 ||
	strcmp(&addr[strlen(addr) - 2], "��") == 0 ||
	strcmp(&addr[strlen(addr) - 2], "��") == 0 )
	return "�o�Ӧa�}���G�ä�����";
    if (HaveRejectStr(addr, rejectstr))
	return "�o�Ӧa�}���G���~";
    return NULL;
}

static char *
isvalidphone(char *phone)
{
    int     i;

#ifdef DBG_DISABLE_CHECK
    return NULL;
#endif // DBG_DISABLE_CHECK

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

#ifdef STR_REGNEW
    /* new user should register in 30 mins */
    // XXX �ثe new acccount �ä��|�b utmp �̩� STR_REGNEW...
    if (strcmp(urec->userid, STR_REGNEW) == 0)
	return 30 - value;
#endif

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
check_and_expire_account(int uid, const userec_t * urec, int expireRange)
{
    char            genbuf[200];
    int             val;
    if ((val = compute_user_value(urec, now)) < 0) {
	snprintf(genbuf, sizeof(genbuf), "#%d %-12s %s %d %d %d",
		 uid, urec->userid, Cdatelite(&(urec->lastlogin)),
		 urec->numlogins, urec->numposts, val);

	// �Y�W�L expireRange �h��H�A
	// ���M�N return 0
	if (-val > expireRange)
	{
	    log_usies("DATED", genbuf);
	    // log_usies("CLEAN", genbuf);
	    kill_user(uid, urec->userid);
	} else val = 0;
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
    snprintf(buf, PATHLEN, FN_JOBSPOOL_DIR ".regcode.%s", cuser.userid);
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
// Figlet Captcha System
////////////////////////////////////////////////////////////////////////////
#ifdef USE_FIGLET_CAPTCHA

static int
gen_captcha(char *buf, int szbuf, char *fpath)
{
    // do not use: GQV
    const char *alphabet = "ABCDEFHIJKLMNOPRSTUWXYZ";
    int calphas = strlen(alphabet);
    char cmd[PATHLEN], opts[PATHLEN];
    int i, coptSpace, coptFont;
    static const char *optSpace[] = {
	"-S", "-s", "-k", "-W", 
	// "-o",
	NULL
    };
    static const char *optFont[] = {
	"banner", "big", "slant", "small",
	"smslant", "standard",
	// block family
	"block", "lean",
	// shadow family
	// "shadow", "smshadow",
	// mini (better with large spacing)
	// "mini", 
	NULL
    };

    // fill captcha code
    for (i = 0; i < szbuf-1; i++)
	buf[i] = alphabet[random() % calphas];
    buf[i] = 0; // szbuf-1

    // decide options
    coptSpace = coptFont = 0;
    while (optSpace[coptSpace]) coptSpace++;
    while (optFont[coptFont]) coptFont++;
    snprintf(opts, sizeof(opts),
	    "%s -f %s", 
	    optSpace[random() % coptSpace],
	    optFont [random() % coptFont ]);

    // create file
    snprintf(fpath, PATHLEN, FN_JOBSPOOL_DIR ".captcha.%s", buf);
    snprintf(cmd, sizeof(cmd), FIGLET_PATH " %s %s > %s",
	    opts, buf, fpath);

    // vmsg(cmd);
    if (system(cmd) != 0)
	return 0;

    return 1;
}


static int
_vgetcb_data_upper(int key, VGET_RUNTIME *prt, void *instance)
{
    if (key >= 'a' && key <= 'z')
	key = toupper(key);
    if (key < 'A' || key > 'Z')
    {
	bell();
	return VGETCB_NEXT;
    }
    return VGETCB_NONE;
}

static int
_vgetcb_data_post(int key, VGET_RUNTIME *prt, void *instance)
{
    char *s = prt->buf;
    while (*s)
    {
	if (isascii(*s) && islower(*s))
	    *s = toupper(*s);
	s++;
    }
    return VGETCB_NONE;
}


static int verify_captcha()
{
    char captcha[7] = "", code[STRLEN];
    char fpath[PATHLEN];
    VGET_CALLBACKS vge = { NULL, _vgetcb_data_upper, _vgetcb_data_post };
    int tries = 0, i;

    do {
	// create new captcha
	if (tries % 2 == 0 || !captcha[0])
	{
	    // if generation failed, skip captcha.
	    if (!gen_captcha(captcha, sizeof(captcha), fpath) ||
		    !dashf(fpath))
		return 1;

	    // prompt user about captcha
	    vs_hdr("CAPTCHA �{�ҵ{��");
	    outs("���F�T�{�z�����U�{�ǡA�п�J�U���ϼ���ܪ���r�C\n"
		    "�ϼ˥u�|�Ѥj�g�� A-Z �^��r���զ��C\n\n");
	    show_file(fpath, 4, b_lines-5, SHOWFILE_ALLOW_ALL);
	    unlink(fpath);
	}

	// each run waits 10 seconds.
	for (i = 10; i > 0; i--)
	{
	    move(b_lines-1, 0); clrtobot();
	    prints("�ХJ���ˬd�W�����ϧΡA %d ���Y�i��J...", i);
	    // flush out current input
	    doupdate(); 
	    peek_input(0.1f, Ctrl('C'));
	    drop_input();
	    sleep(1);
	}

	// input captcha
	move(b_lines-1, 0); clrtobot();
	prints("�п�J�ϼ���ܪ� %d �ӭ^��r��: ", (int)strlen(captcha));
	vgetstring(code, strlen(captcha)+1, 0, "", &vge, NULL);

	if (code[0] && strcasecmp(code, captcha) == 0)
	    break;

	// error case.
	if (++tries >= 10)
	    return 0;

	// error
	vmsg("��J���~�A�Э��աC�`�N�զ���r���O�j�g�^��r���C");

    } while (1);

    clear();
    return 1;
}
#else // !USE_FIGLET_CAPTCHA

static int 
verify_captcha()
{
    return 1;
}

#endif // !USE_FIGLET_CAPTCHA

////////////////////////////////////////////////////////////////////////////
// Justify Utilities
////////////////////////////////////////////////////////////////////////////

static void 
email_justify(const userec_t *muser)
{
	char buf[256], genbuf[256];
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

#ifdef HAVEMOBILE
	if (strcmp(muser->email, "m") == 0 || strcmp(muser->email, "M") == 0)
	    mobile_message(mobile, buf);
	else
#endif
	    bsmtp("etc/registermail", buf, muser->email, "non-exist");
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
		passwd_sync_query(uid, &utmp);
		// tolerate for one year.
		check_and_expire_account(uid, &utmp, 365*12*60);
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

    if (passwd_sync_update(uid, (userec_t *)user) == -1) {
	passwd_unlock();
	vmsg("�Ⱥ��F�A�A���I");
	exit(1);
    }

    passwd_unlock();

    return uid;
}

/////////////////////////////////////////////////////////////////////////////
// New Registration (Phase 1: Create Account)
/////////////////////////////////////////////////////////////////////////////

void
new_register(void)
{
    userec_t        newuser;
    char            passbuf[STRLEN];
    int             try, id, uid;
    char 	   *errmsg = NULL;

#ifdef HAVE_USERAGREEMENT
    int haveag = more(HAVE_USERAGREEMENT, YEA);
    while( haveag != -1 ){
	int c = vans("�аݱz�����o���ϥΪ̱��ڶ�? (yes/no) ");
	if (c == 'y')
	    break;
	else if (c == 'n')
	{
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
	    vmsg(MSG_ERR_MAXTRIES);
	    exit(1);
	}
	getdata(17, 0, msg_uid, newuser.userid,
		sizeof(newuser.userid), DOECHO);
        strcpy(passbuf, newuser.userid);

	if (bad_user_id(passbuf))
	    outs("�L�k�����o�ӥN���A�Шϥέ^��r���A�åB���n�]�t�Ů�\n");
	else if ((id = getuser(passbuf, &xuser)) &&
		// >=: see check_and_expire_account definition
		 (minute = check_and_expire_account(id, &xuser, 0)) >= 0) 
	{
	    if (minute == 999999) // XXX magic number.  It should be greater than MAX_USERS at least.
		outs("���N���w�g���H�ϥ� �O��������");
	    else {
		prints("���N���w�g���H�ϥ� �٦� %d �Ѥ~�L�� \n", 
			minute / (60 * 24) + 1);
	    }
	} 
	else if (reserved_user_id(passbuf))
	    outs("���N���w�Ѩt�ΫO�d�A�ШϥΧO���N��\n");
	else // success
	    break;
    }

    // XXX �O�o�̫� create account �e�٭n�A�ˬd�@�� acc

    try = 0;
    while (1) {
	if (++try >= 6) {
	    vmsg(MSG_ERR_MAXTRIES);
	    exit(1);
	}
	move(20, 0); clrtoeol();
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
	    move(19, 0);
	    outs("�K�X��J���~, �Э��s��J�K�X.\n");
	    continue;
	}
	passbuf[8] = '\0';
	strlcpy(newuser.passwd, genpasswd(passbuf), PASSLEN);
	break;
    }
    // set-up more information.
    move(19, 0); clrtobot();

    // warning: because currutmp=NULL, we can simply pass newuser.* to getdata.
    // DON'T DO THIS IF YOUR currutmp != NULL.
    try = 0;
    while (strlen(newuser.nickname) < 2)
    {
	if (++try > 10) {
	    vmsg(MSG_ERR_MAXTRIES);
	    exit(1);
	}
	getdata(19, 0, "�︹�ʺ١G", newuser.nickname,
		sizeof(newuser.nickname), DOECHO);
    }

    try = 0;
    while (strlen(newuser.realname) < 4)
    {
	if (++try > 10) {
	    vmsg(MSG_ERR_MAXTRIES);
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
	    vmsg(MSG_ERR_MAXTRIES);
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
	struct tm tm;

	localtime4_r(&now, &tm);

	if (++try > 20) {
	    vmsg(MSG_ERR_MAXTRIES);
	    exit(1);
	}
	getdata(22, 0, "�ͤ� (�褸�~/��/��, �p " DATE_SAMPLE ")�G", birthday,
		sizeof(birthday), DOECHO);

	if (strcmp(birthday, DATE_SAMPLE) == 0) {
	    vmsg("���n�ƻs�d�ҡI �п�J�A�u��ͤ�");
	    continue;
	}

	if (ParseDate(birthday, &y, &m, &d)) {
	    vmsg("����榡�����T");
	    continue;
	} else if (y < 1930) {
	    vmsg(MSG_ERR_TOO_OLD);
	    continue;
	} else if (y+3 > tm.tm_year+1900) {
	    vmsg(MSG_ERR_TOO_YOUNG);
	    continue;
	}
	newuser.year  = (unsigned char)(y-1900);
	newuser.month = (unsigned char)m;
	newuser.day   = (unsigned char)d;
    }

    if (!verify_captcha())
    {
	vmsg(MSG_ERR_MAXTRIES);
	exit(1);
    }

    setupnewuser(&newuser);

    if( (uid = initcuser(newuser.userid)) < 0) {
	vmsg("�L�k�إ߱b��");
	exit(1);
    }
    log_usies("REGISTER", fromhost);
}

int
check_regmail(char *email)
{
    FILE           *fp;
    char            buf[128], *c;
    int allow = 0;

    c = strchr(email, '@');
    if (c == NULL) return 0;

    // reject multiple '@'
    if (c != strrchr(email, '@')) 
    {
	vmsg("E-Mail ���榡�����T�C");
	return 0;
    }

    // domain tolower
    str_lower(c, c);

    // allow list
    allow = 0;
    if ((fp = fopen("etc/whitemail", "rt")))
    {
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
	    } else vmsg("��p�A�ثe�������� Email �����U�ӽСC");
	    return 0;
	}
    }

    // reject list
    allow = 1;
    if ((fp = fopen("etc/banemail", "r"))) {
	while (allow && fgets(buf, sizeof(buf), fp)) {
	    if (buf[0] == '#')
		continue;
	    chomp(buf);
	    c = buf+1;
	    switch(buf[0])
	    {
		case 'A': if (strcasecmp(c, email) == 0)
			  {
			      allow = 0;
			      // exact match
			      vmsg("���q�l�H�c�w�Q�T����U");
			  }
			  break;
		case 'P': if (strcasestr(email, c))
			  {
			      allow = 0;
			      vmsg("���H�c�w�Q�T��Ω���U (�i��O�K�O�H�c)");
			  }
			  break;
		case 'S': if (strcasecmp(strstr(email, "@") + 1, c) == 0)
			  {
			      allow = 0;
			      vmsg("���H�c�w�Q�T��Ω���U (�i��O�K�O�H�c)");
			  }
			  break;
		case 'D': if (strlen(email) > strlen(c))
			  {
			      // c2 points to starting of possible c.
			      const char *c2 = email + strlen(email) - strlen(c);
			      if (strcasecmp(c2, c) != 0)
				  break;
			      c2--;
			      if (*c2 == '.' || *c2 == '@')
			      {
				  vmsg("���H�c������w�Q�T��Ω���U (�i��O�K�O�H�c)");
				  allow = 0;
			      }
			  }
			  break;
	    }
	}
	fclose(fp);
    }
    return allow;
}

void 
check_birthday(void)
{
    // check birthday
    int changed = 0;
    time_t t = (time_t)now;
    struct tm tm;

    localtime_r(&t, &tm);
    while ( cuser.year < 40 || // magic number 40: see user.c
	    cuser.year+3 > tm.tm_year) 
    {
	char birthday[sizeof("mmmm/yy/dd ")];
	int y, m, d;

	clear();
	vs_hdr("��J�ͤ�");
	move(2,0);
	outs("�������t�X��椺�e���Ũ�סA�бz��J���T���ͤ��T�C");

	getdata(5, 0, "�ͤ� (�褸�~/��/��, �p " DATE_SAMPLE ")�G", birthday,
		sizeof(birthday), DOECHO);

	if (strcmp(birthday, DATE_SAMPLE) == 0) {
	    vmsg("���n�ƻs�d�ҡI �п�J�A�u��ͤ�");
	    continue;
	} 
	if (ParseDate(birthday, &y, &m, &d)) {
	    vmsg("����榡�����T");
	    continue;
	} else if (y < 1930) {
	    vmsg(MSG_ERR_TOO_OLD);
	    continue;
	} else if (y+3 > tm.tm_year+1900) {
	    vmsg(MSG_ERR_TOO_YOUNG);
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

/////////////////////////////////////////////////////////////////////////////
// User Registration (Phase 2: Validation)
/////////////////////////////////////////////////////////////////////////////

void
check_register(void)
{
    char fn[PATHLEN];

    // �w�g�q�L���N���ΤF
    if (HasUserPerm(PERM_LOGINOK) || HasUserPerm(PERM_SYSOP))
	return;

    // ���v���Q�����ӬO�n���L������U�ΡC
    if (!HasUserPerm(PERM_BASIC))
	return;

    /* 
     * �קK�ϥΪ̳Q�h�^���U���A�b���D�h�^����]���e�A
     * �S�e�X�@�����U��C
     */ 
    setuserfile(fn, FN_REJECT_NOTIFY);
    if (dashf(fn))
    {
	int xun = 0, abort = 0;
	userec_t u = {0};
	char buf[PATHLEN] = "";
	FILE *fp = fopen(fn, "rt");

	// load reference
	fgets(buf, sizeof(buf), fp);
	fclose(fp);

	// parse reference
	if (buf[0] == '#')
	{
	    xun = atoi(buf+1);
	    if (xun <= 0 || xun > MAX_USERS ||
		passwd_sync_query(xun, &u) < 0 ||
		!(u.userlevel & (PERM_ACCOUNTS | PERM_ACCTREG)))
		memset(&u, 0, sizeof(u));
	    // now u is valid only if reference is loaded with account sysop.
	}

	// show message.
	more(fn, YEA);
	move(b_lines-4, 0); clrtobot();
	outs("\n" ANSI_COLOR(1;31) 
	     "�e�����U��f�d���ѡC (���O���w�ƥ���z���H�c��)\n"
	     "�Э��s�ӽШ÷ӤW�����ܥ��T��g���U��C\n");

	if (u.userid[0])
	    outs("�p��������D�λݭn�P���ȤH���p���Ы� r �^�H�C");

	outs(ANSI_RESET "\n");


	// force user to confirm.
	while (!abort)
	{
	    switch(vans(u.userid[0] ?
		    "�п�J y �~��ο�J r �^�H������: " : 
		    "�п�J y �~��: "))
	    {
		case 'y':
		    abort = 1;
		    break;

		case 'r':
		    if (!u.userid[0])
			break;

		    // mail to user
		    setuserfile(quote_file, FN_REJECT_NOTIFY);
		    strlcpy(quote_user, "[�h���q��]", sizeof(quote_user));
		    clear();
		    do_innersend(u.userid, NULL, "[���U���D] �h���������D", NULL);
		    abort = 1;
		    // quick return to avoid confusing user
		    unlink(fn);
		    return;
		    break;

		default:
		    bell();
		    break;
	    }
	}

	unlink(fn);
    } 

    /* �^�йL�����{�ҫH��A�δ��g E-mail post �L */
    clear();
    move(9, 0);

    // �L�k�ϥε��U�X = �Q�h�����A���ܤ@�U?
    // (u_register �̭��| vmsg)
    if (HasUserPerm(PERM_NOREGCODE))
    {
    }

    outs("  �z�ثe�|���q�L���U�{�ҵ{�ǡA�вӸԶ�g" 
	    ANSI_COLOR(32) "���U�ӽг�" ANSI_RESET "�A\n"
	 "  �q�i�����H��o�i���ϥ��v�O�C\n\n");

    outs("  �p�G�z���e���ϥ� email ���{�Ҥ覡�q�L���U�{�Ҧ��S�ݨ즹�T���A\n"
	 "  �N��z���{�ҥѩ��Ƥ�����w�Q�����C\n");

    u_register();

#ifdef NEWUSER_LIMIT
    if (cuser.lastlogin - cuser->firstlogin < 3 * DAY_SECONDS)
	cuser.userlevel &= ~PERM_POST;
    more("etc/newuser", YEA);
#endif
}

static int
create_regform_request()
{
    FILE *fn;

    char fname[PATHLEN];
    setuserfile(fname, FN_REGFORM);
    fn = fopen(fname, "wt");	// regform 2: replace model

    if (!fn)
	return 0;

    // create request data
    fprintf(fn, "uid: %s\n",    cuser.userid);
    fprintf(fn, "name: %s\n",   cuser.realname);
    fprintf(fn, "career: %s\n", cuser.career);
    fprintf(fn, "addr: %s\n",   cuser.address);
    fprintf(fn, "phone: %s\n",  cuser.phone);
    fprintf(fn, "email: %s\n",  "x"); // email is apparently 'x' here.
    fprintf(fn, "----\n");
    fclose(fn);

    // regform2 must update request list
    file_append_record(FN_REQLIST, cuser.userid);

    // save justify information
    snprintf(cuser.justify, sizeof(cuser.justify),
	    "<Manual>");
    return 1;
}

static void
toregister(char *email)
{
    clear();
    vs_hdr("�{�ҳ]�w");
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
	strip_blank(email, email);
	if (strcmp(email, "X") == 0) email[0] = 'x';
	if (strcmp(email, "x") == 0)
	    break;
#ifdef HAVEMOBILE
	else if (strcmp(email, "m") == 0 || strcmp(email, "M") == 0) {
	    if (isvalidmobile(mobile)) {
		char            yn[3];
		getdata(16, 0, "�ЦA���T�{�z��J��������X���T��? [y/N]",
			yn, sizeof(yn), LCECHO);
		if (yn[0] == 'y')
		    break;
	    } else {
		move(15, 0); clrtobot();
		move(17, 0);
		outs("���w��������X�����T,"
		       "�Y�z�L��������п�ܨ�L�覡�{��");
	    }

	}
#endif
	else if (check_regmail(email)) {
	    char            yn[3];
#ifdef USE_EMAILDB
	    int email_count;

	    // before long waiting, alert user
	    move(18, 0); clrtobot();
	    outs("���b�T�{ email, �еy��...\n");
	    doupdate();
	    
	    email_count = emaildb_check_email(email, strlen(email));

	    if (email_count < 0) {
		move(15, 0); clrtobot();
		move(17, 0);
		outs("email �{�Ҩt�εo�Ͱ��D, �еy��A�աA�ο�J x �Ĥ�ʻ{�ҡC\n");
		pressanykey();
		return;
	    } else if (email_count >= EMAILDB_LIMIT) { 
		move(15, 0); clrtobot();
		move(17, 0);
		outs("���w�� E-Mail �w���U�L�h�b��, �ШϥΨ�L E-Mail, �ο�J x �Ĥ�ʻ{��\n");
		outs("���`�N��ʻ{�ҳq�`�|��W�ƶg�H�W���ɶ��C\n");
	    } else {
#endif
	    move(17, 0);
	    outs(ANSI_COLOR(1;31) 
	    "\n�����z: �p�G����o�{�z��J�����U��Ʀ����D�A���ȵ��U�|�Q�����A\n"
	    "�쥻�{�ҥΪ� E-mail �]����A�Ψӻ{�ҡC\n" ANSI_RESET);
	    getdata(16, 0, "�ЦA���T�{�z��J�� E-Mail ��m���T��? [y/N]",
		    yn, sizeof(yn), LCECHO);
	    clrtobot();
	    if (yn[0] == 'y')
		break;
#ifdef USE_EMAILDB
	    }
#endif
	} else {
	    move(15, 0); clrtobot();
	    move(17, 0);
	    outs("���w�� E-Mail �����T�C�i��A��J���O�K�O��Email�A\n");
	    outs("�δ����ϥΪ̥H�� E-Mail �{�ҫ�Q�������C\n\n");
	    outs("�Y�z�L E-Mail �п�J x �ѯ�����ʻ{�ҡA\n");
	    outs("���`�N��ʻ{�ҳq�`�|��W�ƶg�H�W���ɶ��C\n");
	}
    }
#ifdef USE_EMAILDB
    // XXX for 'x', the check will be made later... let's simply ignore it.
    if (strcmp(email, "x") != 0 &&
	emaildb_update_email(cuser.userid, strlen(cuser.userid), email, strlen(email)) < 0) {
	move(15, 0); clrtobot();
	move(17, 0);
	outs("email �{�Ҩt�εo�Ͱ��D, �еy��A�աA�ο�J x �Ĥ�ʻ{�ҡC\n");
	pressanykey();
	return;
    }
#endif
    strlcpy(cuser.email, email, sizeof(cuser.email));
 REGFORM2:
    if (strcasecmp(email, "x") == 0) {	/* ��ʻ{�� */
	if (!create_regform_request())
	{
	    vmsg("���U�ӽг�إߥ��ѡC�Ц� " BN_BUGREPORT " ���i�C");
	}
    } else {
	// register by mail or mobile
	snprintf(cuser.justify, sizeof(cuser.justify), "<Email>");
#ifdef HAVEMOBILE
	if (phone != NULL && email[1] == 0 && tolower(email[0]) == 'm')
	    snprintf(cuser.justify, sizeof(cuser.justify),
		    "<Mobile>");
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
    char            ans[3], *errcode;
    int		    i = 0;

    if (cuser.userlevel & PERM_LOGINOK) {
	outs("�z�������T�{�w�g�����A���ݶ�g�ӽЪ�");
	return XEASY;
    }

    // TODO REGFORM 2 checks 2 parts.
    i = file_find_record(FN_REQLIST, cuser.userid);

    if (i > 0)
    {
	vs_hdr("���U��|�b�B�z��");
	move(3, 0);
	prints("   �z�����U�ӽг�|�b�B�z��(�B�z����: %d)�A�Э@�ߵ���\n\n", i);
	outs("   * �p�G�z���e���ϥ� email ���{�Ҥ覡�q�L���U�{�Ҧ��S�ݨ즹�T���A\n"
	     "     �N��z���{�ҥѩ��Ƥ�����w�Q�����C\n\n"
	     "   * �p�G�z�w������U�X�o�ݨ�o�ӵe���A�N��z�b�ϥ� Email ���U��\n"
	     "     " ANSI_COLOR(1;31) "�S�t�~�ӽФF���������H�u�f�֪����U�ӽг�C" 
		ANSI_RESET "\n"
	     "     �i�J�H�u�f�ֵ{�ǫ� Email ���U�X�۰ʥ��ġA�n����f�֧���\n"
	     "      (�|�h��ܦh�ɶ��A�ƤѨ�ƶg�O���`��) �A�ҥH�Э@�ߵ��ԡC\n\n");
	vmsg("�z�����U�ӽг�|�b�B�z��");
	return FULLUPDATE;
    }

    strlcpy(rname, cuser.realname, sizeof(rname));
    strlcpy(addr,  cuser.address,  sizeof(addr));
    strlcpy(email, cuser.email,    sizeof(email));
    strlcpy(career,cuser.career,   sizeof(career));
    strlcpy(phone, cuser.phone,    sizeof(phone));

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
	vs_hdr("EMail�{��");
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
	    snprintf(cuser.justify, sizeof(cuser.justify),
		     "<E-Mail>: %s", Cdate(&now));
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
		toregister(email);
		return FULLUPDATE;
	    }
	} else {
	    toregister(email);
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
	    getfield(13, "�褸/���/��� �p: " DATE_SAMPLE , 
		    "�ͤ�", birthday, sizeof(birthday));
	    if (birthday[0] == 0) {
		snprintf(birthday, sizeof(birthday), "%04i/%02i/%02i",
			 1900 + cuser.year, cuser.month, cuser.day);
		mon = cuser.month;
		day = cuser.day;
		year = cuser.year;
	    } else {
		int y, m, d;
		if (strcmp(birthday, DATE_SAMPLE) == 0) {
		    vmsg("���n�ƻs�d�ҡI �п�J�A�u��ͤ�");
		    continue;
		}
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
	getfield(14, "1.��k 2.��k ", "�ʧO", sex_is, 2);
	getdata(20, 0, "�H�W��ƬO�_���T(Y/N)�H(Q)�������U [N] ",
		ans, 3, LCECHO);
	if (ans[0] == 'q')
	    return 0;
	if (ans[0] == 'y')
	    break;
    }

    // copy values to cuser
    strlcpy(cuser.realname, rname,  sizeof(cuser.realname));
    strlcpy(cuser.address,  addr,   sizeof(cuser.address));
    strlcpy(cuser.email,    email,  sizeof(cuser.email));
    strlcpy(cuser.career,   career, sizeof(cuser.career));
    strlcpy(cuser.phone,    phone,  sizeof(cuser.phone));

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

    // if reach here, email is apparently 'x'.
    toregister(email);

    // update cuser
    passwd_sync_update(usernum, &cuser);

    return FULLUPDATE;
}

////////////////////////////////////////////////////////////////////////////
// Regform Utilities
////////////////////////////////////////////////////////////////////////////

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
    userec_t	u;	// user record
    char	online;

} RegformEntry;

// regform format utilities

// regform loader: deprecated.
// now we use real user record.
/*
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
	    strlcpy(pre->u.userid, v, sizeof(pre->u.userid));
	else if (strcmp(buf, "name") == 0)
	    strlcpy(pre->u.realname, v, sizeof(pre->u.realname));
	else if (strcmp(buf, "career") == 0)
	    strlcpy(pre->u.career, v, sizeof(pre->u.career));
	else if (strcmp(buf, "addr") == 0)
	    strlcpy(pre->u.address, v, sizeof(pre->u.address));
	else if (strcmp(buf, "phone") == 0)
	    strlcpy(pre->u.phone, v, sizeof(pre->u.phone));
    }
    return pre->u.userid[0] ? 1 : 0;
}
*/

static int
print_regform_entry(const RegformEntry *pre, FILE *fp, int close)
{
    fprintf(fp, "uid: %s\n",	pre->u.userid);
    fprintf(fp, "name: %s\n",	pre->u.realname);
    fprintf(fp, "career: %s\n", pre->u.career);
    fprintf(fp, "addr: %s\n",	pre->u.address);
    fprintf(fp, "phone: %s\n",	pre->u.phone);
    if (close)
	fprintf(fp, "----\n");
    return 1;
}

static int
concat_regform_entry_localized(const RegformEntry *pre, char *result, int maxlen)
{
    int len = strlen(result);
    len += snprintf(result + len, maxlen - len, "�ϥΪ�ID: %s\n", pre->u.userid);
    len += snprintf(result + len, maxlen - len, "�u��m�W: %s\n", pre->u.realname);
    len += snprintf(result + len, maxlen - len, "¾�~�Ǯ�: %s\n", pre->u.career);
    len += snprintf(result + len, maxlen - len, "�ثe��}: %s\n", pre->u.address);
    len += snprintf(result + len, maxlen - len, "�q�ܸ��X: %s\n", pre->u.phone);
    len += snprintf(result + len, maxlen - len, "�W����m: %s\n", pre->u.lasthost);
    len += snprintf(result + len, maxlen - len, "----\n");
    return 1;
}

static int
print_regform_entry_localized(const RegformEntry *pre, FILE *fp)
{
    char buf[STRLEN * 6];
    buf[0] = '\0';
    concat_regform_entry_localized(pre, buf, sizeof(buf));
    fputs(buf, fp);
    return 1;
}

int
append_regform(const RegformEntry *pre, const char *logfn, const char *ext)
{
    FILE *fout = fopen(logfn, "at");
    if (!fout)
	return 0;

    print_regform_entry(pre, fout, 0);
    if (ext)
    {
	syncnow();
	fprintf(fout, "Date: %s\n", Cdate(&now));
	if (*ext)
	    fprintf(fout, ext);
    }
    // close it
    fprintf(fout, "----\n");
    fclose(fout);
    return 1;
}

// prototype declare
static void regform_print_reasons(const char *reason, FILE *fp);
static void regform_concat_reasons(const char *reason, char *result, int maxlen);

int regform_estimate_queuesize()
{
    return dashs(FN_REQLIST) / IDLEN;
}

/////////////////////////////////////////////////////////////////////////////
// Administration (SYSOP Validation)
/////////////////////////////////////////////////////////////////////////////

#define REJECT_REASONS	(6)
#define REASON_LEN	(60)
static const char *reasonstr[REJECT_REASONS] = {
    "�п�J�u��m�W",
    "�иԶ�(���~)�Ǯաy�t�z�y�šz�ΪA�ȳ��(�t���ݿ�����¾��)",
    "�ж�g�����} (�t����/�m����, �x�_���аO�o�[��F�ϰ�)",
    "�иԶ�s���q�� (�t�ϽX, �������[ '-', '(', ')' ���Ÿ�)",
    "�к�T�ç����g���U�ӽЪ�",
    "�ХΤ����g�ӽг�",
};

#define REASON_FIRSTABBREV '0'
#define REASON_IN_ABBREV(x) \
    ((x) >= REASON_FIRSTABBREV && (x) - REASON_FIRSTABBREV < REJECT_REASONS)
#define REASON_EXPANDABBREV(x)	 reasonstr[(x) - REASON_FIRSTABBREV]

void
regform_log2board(const RegformEntry *pre, char accepted, 
	const char *reason, int priority)
{
#ifdef BN_ID_RECORD
    char title[STRLEN];
    char *title2 = NULL;
    char msg[STRLEN * REJECT_REASONS];

    snprintf(title, sizeof(title), 
	    "[�f��] %s: %s (%s: %s)", 
	    accepted ? "���q�L":"���h�^", pre->u.userid, 
	    priority ? "���w�f��" : "�f�֪�",
	    cuser.userid);

    // reduce mail header title
    title2 = strchr(title, ' ');
    if (title2) title2++;


    // construct msg
    strlcpy(msg, title2 ? title2 : title, sizeof(msg));
    strlcat(msg, "\n", sizeof(msg));
    if (!accepted) {
	regform_concat_reasons(reason, msg, sizeof(msg));
    }
    strlcat(msg, "\n", sizeof(msg));
    concat_regform_entry_localized(pre, msg, sizeof(msg));

    post_msg(BN_ID_RECORD, title, msg, "[���U�t��]");

#endif  // BN_ID_RECORD
}

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
    sethomefile(buf, muser.userid, FN_REJECT_NOTIFY);
    unlink(buf);

    // update password file
    passwd_sync_update(unum, &muser);

    // alert online users?
    if (search_ulist(unum))
    {
	sendalert(muser.userid,  ALERT_PWD_PERM|ALERT_PWD_JUSTIFY); // force to reload perm
	kick_all(muser.userid);
    }

    // According to suggestions by PTT BBS account sysops,
    // it is better to use anonymous here.
#if FOREIGN_REG_DAY > 0
    if(muser.uflag2 & FOREIGN)
	mail_log2id(muser.userid, "[System] Registration Complete ", "etc/foreign_welcome",
		"[SYSTEM]", 1, 0);
    else
#endif
    // last: send notification mail
    mail_log2id(muser.userid, "[�t�γq��] ���U���\\ ", "etc/registered",
	    "[�t�γq��]", 1, 0);
}

void 
regform_reject(const char *userid, const char *reason, const RegformEntry *pre)
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

    // update password file
    passwd_sync_update(unum, &muser);

    // alert online users?
    if (search_ulist(unum))
    {
	sendalert(muser.userid,  ALERT_PWD_PERM); // force to reload perm
	kick_all(muser.userid);
    }

    // last: send notification
    mkuserdir(muser.userid);
    sethomefile(buf, muser.userid, FN_REJECT_NOTIFY);
    fp = fopen(buf, "wt");
    assert(fp);
    syncnow();

    // log reference for mail-reply.
    fprintf(fp, "#%010d\n\n", usernum);

    if(pre) print_regform_entry_localized(pre, fp);
    fprintf(fp, "%s ���U���ѡC\n", Cdate(&now));


    // prompt user for how to contact if they have problem
    // (deprecated because we allow direct reply now)
    // fprintf(fp, ANSI_COLOR(1;31) "�p��������D�λݭn�P���ȤH���p���Ц�"
    // 	    BN_ID_PROBLEM "�ݪO�C" ANSI_RESET "\n"); 

    // multiple abbrev loop
    regform_print_reasons(reason, fp);
    fprintf(fp, "--\n");
    fclose(fp);

    // if current site has extra notes
    if (dashf(FN_REJECT_NOTES))
	AppendTail(FN_REJECT_NOTES, buf, 0);

    // According to suggestions by PTT BBS account sysops,
    // it is better to use anonymous here.
    //
    // XXX how to handle the notification file better?
    // mail_log2id: do not use move.
    // mail_muser(muser, "[���U����]", buf);
    
    // use regform2! no need to set 'newmail'.
    mail_log2id(muser.userid, "[���U���ѰO��]", buf, "[���U�t��]", 0, 0);
}

// New Regform UI
static void
prompt_regform_ui()
{
    vs_footer(" �f�� ",
	    " (y)����(n)�ڵ�(d)�ᱼ (s)���L(u)�_�� (�ť�/PgDn)�x�s+�U�� (q/END)����");
}

static void
regform_concat_reasons(const char *reason, char *result, int maxlen)
{
    int len = strlen(result);
    // multiple abbrev loop
    if (REASON_IN_ABBREV(reason[0]))
    {
	int i = 0;
	for (i = 0; reason[i] && REASON_IN_ABBREV(reason[i]); i++) {
	    len += snprintf(result + len, maxlen - len, "[�h�^��]] %s\n", REASON_EXPANDABBREV(reason[i]));
	}
    } else {
	len += snprintf(result + len, maxlen - len, "[�h�^��]] %s\n", reason);
    }
}

static void
regform_print_reasons(const char *reason, FILE *fp)
{
    char msg[STRLEN * REJECT_REASONS];
    msg[0] = '\0';
    regform_concat_reasons(reason, msg, sizeof(msg));
    fputs(msg, fp);
}

static void
resolve_reason(char *s, int y, int force)
{
    // should start with REASON_FIRSTABBREV
    const char *reason_prompt = 
	" (0)�u��m�W (1)�Զ�t�� (2)�����}"
	" (3)�Զ�q�� (4)�T���g (5)�����g";

    s[0] = 0;
    move(y, 0);
    outs(reason_prompt); outs("\n");

    do {
	getdata(y+1, 0, "�h�^��]: ", s, REASON_LEN, DOECHO);

	// convert abbrev reasons (format: single digit, or multiple digites)
	if (REASON_IN_ABBREV(s[0]))
	{
	    if (s[1] == 0) // simple replace ment
	    {
		strlcpy(s, REASON_EXPANDABBREV(s[0]),
			REASON_LEN);
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

	if (!force && !*s)
	    return;

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

////////////////////////////////////////////////////////////////////////////
// Regform2 API
////////////////////////////////////////////////////////////////////////////

// registration queue
int
regq_append(const char *userid)
{
    if (file_append_record(FN_REQLIST, userid) < 0)
	return 0;
    return 1;
}

int 
regq_find(const char *userid)
{
    return file_find_record(FN_REQLIST, userid);
}

int
regq_delete(const char *userid)
{
    return file_delete_record(FN_REQLIST, userid, 0);
}

// user home regform operation
int 
regfrm_exist(const char *userid)
{
    char fn[PATHLEN];
    sethomefile(fn, userid, FN_REGFORM);
    return  dashf(fn) ? 1 : 0;
}

int
regfrm_delete(const char *userid)
{
    char fn[PATHLEN];
    sethomefile(fn, userid, FN_REGFORM);

#ifdef DBG_DRYRUN
    // dry run!
    vmsgf("regfrm_delete (%s)", userid);
    return 1;
#endif

    // directly delete.
    unlink(fn);

    // remove from queue
    regq_delete(userid);
    return 1;
}

int
regfrm_load(const char *userid, RegformEntry *pre)
{
    // FILE *fp = NULL;
    char fn[PATHLEN];
    int unum = 0;

    memset(pre, 0, sizeof(RegformEntry));

    // first check if user exists.
    unum = getuser(userid, &(pre->u));

    // valid unum starts at 1.
    if (unum < 1)
	return 0;

    // check if regform exists.
    sethomefile(fn, userid, FN_REGFORM);
    if (!dashf(fn))
	return 0;

#ifndef DBG_DRYRUN
    // check if user is already registered
    if (pre->u.userlevel & PERM_LOGINOK)
    {
	regfrm_delete(userid);
	return 0;
    }
#endif

    // load regform
    // (deprecated in current version, we use real user data now)

    // fill RegformEntry data
    pre->online = search_ulist(unum) ? 1 : 0;

    return 1;
}

int 
regfrm_save(const char *userid, const RegformEntry *pre)
{
    FILE *fp = NULL;
    char fn[PATHLEN];
    int ret = 0;
    sethomefile(fn, userid, FN_REGFORM);

    fp = fopen(fn, "wt");
    if (!fp)
	return 0;
    ret = print_regform_entry(pre, fp, 1);
    fclose(fp);
    return ret;
}

int 
regfrm_trylock(const char *userid)
{
    int fd = 0;
    char fn[PATHLEN];
    sethomefile(fn, userid, FN_REGFORM);
    if (!dashf(fn)) return 0;
    fd = open(fn, O_RDONLY);
    if (fd < 0) return 0;
    if (flock(fd, LOCK_EX|LOCK_NB) == 0)
	return fd;
    close(fd);
    return 0;
}

int 
regfrm_unlock(int lockfd)
{
    int fd = lockfd;
    if (lockfd <= 0)
	return 0;
    lockfd =  flock(fd, LOCK_UN) == 0 ? 1 : 0;
    close(fd);
    return lockfd;
}

// regform processors
int
regfrm_accept(RegformEntry *pre, int priority)
{
    char justify[REGLEN+1], buf[STRLEN*2];
    char fn[PATHLEN], fnlog[PATHLEN];

#ifdef DBG_DRYRUN
    // dry run!
    vmsg("regfrm_accept");
    return 1;
#endif

    sethomefile(fn, pre->u.userid, FN_REGFORM);

    // build justify string
    snprintf(justify, sizeof(justify),
	    "[%s] %s", cuser.userid, Cdate(&now));

    // call handler
    regform_accept(pre->u.userid, justify);

    // log to user home
    sethomefile(fnlog, pre->u.userid, FN_REGFORM_LOG);
    append_regform(pre, fnlog, "");

    // log to global history
    snprintf(buf, sizeof(buf), "Approved: %s -> %s\n", 
	    cuser.userid, pre->u.userid);
    append_regform(pre, FN_REGISTER_LOG, buf);

    // log to board
    regform_log2board(pre, 1, NULL, priority);

    // remove from queue
    unlink(fn);
    regq_delete(pre->u.userid);
    return 1;
}

int
regfrm_reject(RegformEntry *pre, const char *reason, int priority)
{
    char buf[STRLEN*2];
    char fn[PATHLEN];

#ifdef DBG_DRYRUN
    // dry run!
    vmsg("regfrm_reject");
    return 1;
#endif

    sethomefile(fn, pre->u.userid, FN_REGFORM);

    // call handler
    regform_reject(pre->u.userid, reason, pre);

    // log to global history
    snprintf(buf, sizeof(buf), "Rejected: %s -> %s [%s]\n", 
	    cuser.userid, pre->u.userid, reason);
    append_regform(pre, FN_REGISTER_LOG, buf);

    // log to board
    regform_log2board(pre, 0, reason, priority);

    // remove from queue
    unlink(fn);
    regq_delete(pre->u.userid);
    return 1;
}

// working queue
FILE *
regq_init_pull()
{
    FILE *fp = tmpfile(), *src =NULL;
    char buf[STRLEN];
    if (!fp) return NULL;
    src = fopen(FN_REQLIST, "rt");
    if (!src) { fclose(fp); return NULL; }
    while (fgets(buf, sizeof(buf), src))
	fputs(buf, fp);
    fclose(src);
    rewind(fp);
    return fp;
}

int 
regq_pull(FILE *fp, char *uid)
{
    char buf[STRLEN];
    size_t idlen = 0;
    uid[0] = 0;
    if (fgets(buf, sizeof(buf), fp) == NULL)
	return 0;
    idlen = strcspn(buf, str_space);
    if (idlen < 1) return 0;
    if (idlen > IDLEN) idlen = IDLEN;
    strlcpy(uid, buf, idlen+1);
    return 1;
}

int
regq_end_pull(FILE *fp)
{
    // no need to unlink because fp is a tmpfile.
    if (!fp) return 0;
    fclose(fp);
    return 1;
}

// UI part
int
ui_display_regform_single(
	const RegformEntry *pre, 
	int tid, char *reason)
{
    int c;
    const userec_t *xuser = &(pre->u);

    while (1)
    {
	move(1, 0);
	user_display(xuser, 1);
	move(14, 0);
	prints(ANSI_COLOR(1;32) 
		"--------------- �o�O�� %2d �����U�� -----------------------" 
		ANSI_RESET "\n", tid);
	prints("  %-12s: %s %s\n",	"�b��", pre->u.userid,
		(xuser->userlevel & PERM_NOREGCODE) ? 
		ANSI_COLOR(1;31) "  [T:�T��ϥλ{�ҽX���U]" ANSI_RESET: 
		"");
	prints("0.%-12s: %s%s\n",	"�u��m�W", pre->u.realname,
		xuser->uflag2 & FOREIGN ? " (�~�y)" : 
		"");
	prints("1.%-12s: %s\n",	"�A�ȳ��", pre->u.career);
	prints("2.%-12s: %s\n",	"�ثe��}", pre->u.address);
	prints("3.%-12s: %s\n",	"�s���q��", pre->u.phone);

	move(b_lines, 0);
	outs("�O�_���������(Y/N/Q/Del/Skip)�H[S] ");

	// round to ASCII
	while ((c = igetch()) > 0xFF);
	c = tolower(c);

	if (c == 'y' || c == 'q' || c == 'd' || c == 's')
	    return c;
	if (c == 'n')
	{
	    int n = 0;
	    move(3, 0);
	    outs("\n" ANSI_COLOR(1;31) 
		    "�д��X�h�^�ӽЪ��]�A�� <Enter> ����:\n" ANSI_RESET);
	    for (n = 0; n < REJECT_REASONS; n++)
		prints("%d) %s\n", n, reasonstr[n]);
	    outs("\n\n\n"); // preserved for prompt

	    getdata(3+2+REJECT_REASONS+1, 0,"�h�^��]: ",
		    reason, REASON_LEN, DOECHO);
	    if (reason[0] == 0)
		continue;
	    // interprete reason
	    return 'n';
	} 
	else if (REASON_IN_ABBREV(c))
	{
	    // quick set
	    sprintf(reason, "%c", c);
	    return 'n';
	}
	return 's';
    }
    // shall never reach here
    return 's';
}

void
regform2_validate_single(const char *xuid)
{
    int lfd = 0;
    int tid = 0;
    char uid[IDLEN+1];
    char rsn[REASON_LEN];
    FILE *fpregq = regq_init_pull();
    RegformEntry re;

    if (xuid && !*xuid)
	xuid = NULL;

    if (!fpregq)
	return;

    while (regq_pull(fpregq, uid))
    {
	int abort = 0;

	// if target assigned, loop until given target.
	if (xuid && strcasecmp(uid, xuid) != 0)
	    continue;

	// try to load regform.
	if (!regfrm_load(uid, &re))
	{
	    regq_delete(uid);
	    continue;
	}

	// try to lock
	lfd = regfrm_trylock(uid);
	if (lfd <= 0)
	    continue;

	tid ++;

	// display regform and process
	switch(ui_display_regform_single(&re, tid, rsn))
	{
	    case 'y': // accept
		regfrm_accept(&re, xuid ? 1 : 0);
		break;

	    case 'd': // delete
		regfrm_delete(uid);
		break;

	    case 'q': // quit
		abort = 1;
		break;

	    case 'n': // reject
		regfrm_reject(&re, rsn, xuid ? 1 : 0);
		break;

	    case 's': // skip
		// do nothing.
		break;

	    default: // shall never reach here
		assert(0);
		break;
	}
	
	// final processing
	regfrm_unlock(lfd);

	if (abort)
	    break;
    }
    regq_end_pull(fpregq);

    // finishing
    clear(); move(5, 0);
    if (xuid && tid == 0)
	prints("���o�{ %s �����U��C", xuid);
    else
	prints("�z�˵��F %d �����U��C", tid);
    pressanykey();
}

#define FORMS_IN_PAGE (10)

int
regform2_validate_page(int dryrun)
{
    int yMsg = FORMS_IN_PAGE*2+1;
    RegformEntry forms [FORMS_IN_PAGE];
    char ans	[FORMS_IN_PAGE];
    int  lfds	[FORMS_IN_PAGE];
    char rejects[FORMS_IN_PAGE][REASON_LEN];	// reject reason length
    char rsn	[REASON_LEN];
    int cforms = 0,	// current loaded forms
	ci = 0, // cursor index
	ch = 0,	// input key
	i;
    int tid = 0;
    char uid[IDLEN+1];
    FILE *fpregq = regq_init_pull();

    if (!fpregq)
	return 0;

    while (ch != 'q')
    {
	// initialize and prepare
	memset(ans,	0, sizeof(ans));
	memset(rejects, 0, sizeof(rejects));
	memset(forms,	0, sizeof(forms));
	memset(lfds,	0, sizeof(lfds));
	cforms = 0;
	clear();

	// load forms
	while (cforms < FORMS_IN_PAGE)
	{
	    if (!regq_pull(fpregq, uid))
		break;
	    i = cforms; // align index

	    // check if user exists.
	    if (!regfrm_load(uid, &forms[i]))
	    {
		regq_delete(uid);
		continue;
	    }

	    // try to lock
	    lfds[i] = regfrm_trylock(uid);
	    if (lfds[i] <= 0)
		continue;

	    // assign default answers
	    if (forms[i].u.userlevel & PERM_LOGINOK)
		ans[i] = 'd';
#ifdef REGFORM_DISABLE_ONLINE_USER
	    else if (forms[i].online)
		ans[i] = 's';
#endif // REGFORM_DISABLE_ONLINE_USER

	    // display regform
	    move(i*2, 0);
	    prints("  %2d%s %s%-12s " ANSI_RESET, 
		    i+1, 
		    ( (forms[i].u.userlevel & PERM_LOGINOK) ? 
		      ANSI_COLOR(1;33) "Y" : 
#ifdef REGFORM_DISABLE_ONLINE_USER
			  forms[i].online ? "s" : 
#endif
			  "."),
		    forms[i].online ?  ANSI_COLOR(1;35) : ANSI_COLOR(1),
		    forms[i].u.userid);

	    prints( ANSI_COLOR(1;31) "%19s " 
		    ANSI_COLOR(1;32) "%-40s" ANSI_RESET"\n", 
		    forms[i].u.realname, forms[i].u.career);

	    move(i*2+1, 0); 
	    prints("    %s %-50s%20s\n", 
		    (forms[i].u.userlevel & PERM_NOREGCODE) ? 
		    ANSI_COLOR(1;31) "T" ANSI_RESET : " ",
		    forms[i].u.address, forms[i].u.phone);

	    cforms++, tid ++;
	}

	// if no more forms then leave.
	if (cforms < 1)
	    break;

	// adjust cursor if required
	if (ci >= cforms)
	    ci = cforms-1;

	// display page info
	vbarf(ANSI_REVERSE "\t%s �w��� %d �����U�� ", // "(%2d%%)  ",
		    dryrun? "(���ռҦ�)" : "",
		    tid);

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

		case KEY_HOME: ci = 0; break;
		    /*
		case KEY_END:  ci = cforms-1; break;
		    */

		    // abort
		case KEY_END:
		case 'q':
		    ch = 'q';
		    if (vans("�T�w�n���}�F�ܡH (�����ܧ�N���|�x�s) [y/N]: ") != 'y')
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

		    {
			int blanks = 0;
			// solving blank (undecided entries)
			for (i = 0, blanks = 0; i < cforms; i++)
			    if (ans[i] == 0) blanks ++;

			if (!blanks)
			    break;

			// have more blanks
			ch = vansf("�|�����w�� %d �Ӷ��حn: (S���L/y�q�L/n�ڵ�/e�~��s��): ", 
				blanks);
		    }

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
			resolve_reason(rsn, yMsg, 1);
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
		    resolve_reason(rejects[ci], yMsg, 0);
		    move(yMsg, 0); clrtobot();
		    prompt_regform_ui();

		    if (!rejects[ci][0])
			break;

		    move(yMsg, 0);
		    prints("�h�^ %s ���U���]:\n %s\n", 
			    forms[ci].u.userid, rejects[ci]);

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
	    vs_hdr("���ռҦ�");
	    outs("�z���b������ռҦ��A�ҥH��f�����U��ä��|�ͮġC\n"
		    "�U���C�X���O��~�z�f�������G:\n\n");

	    for (i = 0; i < cforms; i++)
	    {
		char justify[REGLEN+1];
		if (ans[i] == 'y')
		    snprintf(justify, sizeof(justify), // build justify string
			    "%s %s", cuser.userid, Cdate(&now));

		prints("%2d. %-12s - %c %s\n", i+1, forms[i].u.userid, ans[i],
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
		switch(ans[i])
		{
		    case 'y': // accept
			regfrm_accept(&forms[i], 0);
			break;

		    case 'd': // delete
			regfrm_delete(uid);
			break;

		    case 'n': // reject
			regfrm_reject(&forms[i], rejects[i], 0);
			break;

		    case 's': // skip
			// do nothing.
			break;

		    default:
			assert(0);
			break;
		}
	    }
	} // !dryrun

	// unlock all forms
	for (i = 0; i < cforms; i++)
	    regfrm_unlock(lfds[i]);

    } // while (ch != 'q')

    regq_end_pull(fpregq);

    // finishing
    clear(); move(5, 0);
    prints("�z�˵��F %d �����U��C", tid);
    pressanykey();
    return 0;
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
#if 0
static int
auto_scan(char fdata[][STRLEN], char ans[])
{
    int             good = 0;
    int             count = 0;
    int             i;
    char            temp[10];

    if (!strncmp(fdata[1], "�p", 2) || DBCS_strcasestr(fdata[1], "�X")
	|| DBCS_strcasestr(fdata[1], "��") || DBCS_strcasestr(fdata[1], "��")) {
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
	if (DBCS_strcasestr(fdata[1], "������")) {
	    ans[0] = '0';
	    return 1;
	}
	if (DBCS_strcasestr("�����]���P�d�G��", temp))
	    good++;
	else if (DBCS_strcasestr("���C���L���x�E���B", temp))
	    good++;
	else if (DBCS_strcasestr("Ĭ��d�f����i����Ĭ", temp))
	    good++;
	else if (DBCS_strcasestr("�}�¥ۿc�I���έ�", temp))
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
    if (DBCS_strcasestr(fdata[2], "�j")) {
	if (DBCS_strcasestr(fdata[2], "�x") || DBCS_strcasestr(fdata[2], "�H") ||
	    DBCS_strcasestr(fdata[2], "��") || DBCS_strcasestr(fdata[2], "�F") ||
	    DBCS_strcasestr(fdata[2], "�M") || DBCS_strcasestr(fdata[2], "ĵ") ||
	    DBCS_strcasestr(fdata[2], "�v") || DBCS_strcasestr(fdata[2], "�ʶ�") ||
	    DBCS_strcasestr(fdata[2], "����") || DBCS_strcasestr(fdata[2], "��") ||
	    DBCS_strcasestr(fdata[2], "��") || DBCS_strcasestr(fdata[2], "�F�d"))
	    good++;
    } else if (DBCS_strcasestr(fdata[2], "�k��"))
	good++;

    if (DBCS_strcasestr(fdata[3], "�a�y") || DBCS_strcasestr(fdata[3], "�t�z") ||
	DBCS_strcasestr(fdata[3], "�H�c")) {
	ans[0] = '2';
	return 3;
    }
    if (DBCS_strcasestr(fdata[3], "��") || DBCS_strcasestr(fdata[3], "��")) {
	if (DBCS_strcasestr(fdata[3], "��") || DBCS_strcasestr(fdata[3], "��")) {
	    if (DBCS_strcasestr(fdata[3], "��"))
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
#endif

int
m_register(void)
{
    FILE           *fn;
    int             x, y, wid, len;
    char            ans[4];
    char            genbuf[200];

    if (dashs(FN_REQLIST) <= 0) {
	outs("�ثe�õL�s���U���");
	return XEASY;
    }
    fn = fopen(FN_REQLIST, "r");
    assert(fn);

    vs_hdr("�f�֨ϥΪ̵��U���");
    y = 2;
    x = wid = 0;

    while (fgets(genbuf, STRLEN, fn) && x < 65) {
	move(y++, x);
	outs(genbuf);
	len = strlen(genbuf);
	if (len > wid)
	    wid = len;
	if (y >= t_lines - 3) {
	    y = 2;
	    x += wid + 2;
	}
    }

    fclose(fn);

    getdata(b_lines - 1, 0, 
	    "�}�l�f�ֶ� (Y:�浧�Ҧ�/N:���f/E:�㭶�Ҧ�/U:���wID)�H[N] ", 
	    ans, sizeof(ans), LCECHO);

    if (ans[0] == 'y')
	regform2_validate_single(NULL);
    else if (ans[0] == 'e')
	regform2_validate_page(0);
    else if (ans[0] == 'u') {
	vs_hdr("���w�f��");
	usercomplete(msg_uid, genbuf);
	if (genbuf[0])
	    regform2_validate_single(genbuf);
    }

    return 0;
}

/* vim:sw=4
 */
