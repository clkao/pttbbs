/* $Id$ */
#define _XOPEN_SOURCE
#define _ISOC99_SOURCE

#include "bbs.h"

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
checkpasswd(char *passwd, char *plain)
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
bad_user_id(char *userid)
{
    int             len, i;
    len = strlen(userid);

    if (len < 2)
	return 1;

    if (not_alpha(userid[0]))
	return 1;
    for (i = 1; i < len; i++)
	/* DickG: �ץ��F�u��� userid �Ĥ@�Ӧr���� bug */
	if (not_alnum(userid[i]))
	    return 1;

    if (strcasecmp(userid, str_new) == 0)
	return 1;

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
compute_user_value(userec_t * urec, time_t clock)
{
    int             value;

    /* if (urec) has XEMPT permission, don't kick it */
    if ((urec->userid[0] == '\0') || (urec->userlevel & PERM_XEMPT)
    /* || (urec->userlevel & PERM_LOGINOK) */
	|| !strcmp(STR_GUEST, urec->userid))
	return 999999;
    value = (clock - urec->lastlogin) / 60;	/* minutes */

    /* new user should register in 30 mins */
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
check_and_expire_account(int uid, userec_t * urec)
{
    char            genbuf[200], genbuf2[200];
    int             val;
    if ((val = compute_user_value(urec, now)) < 0) {
	snprintf(genbuf, sizeof(genbuf), "#%d %-12s %15.15s %d %d %d",
		uid, urec->userid, ctime(&(urec->lastlogin)) + 4,
		urec->numlogins, urec->numposts, val);
	if (val > -1 * 60 * 24 * 365) {
	    log_usies("CLEAN", genbuf);
	    snprintf(genbuf, sizeof(genbuf), "home/%c/%s", urec->userid[0],
		    urec->userid);
	    snprintf(genbuf2, sizeof(genbuf2), "tmp/%s", urec->userid);
	    if (dashd(genbuf) && Rename(genbuf, genbuf2)) {
		snprintf(genbuf, sizeof(genbuf),
			 "/bin/rm -fr home/%c/%s >/dev/null 2>&1",
			 urec->userid[0], urec->userid);
		system(genbuf);
	    }
            kill_user(uid);
	} else {
	    val = 0;
	    log_usies("DATED", genbuf);
	}
    }
    return val;
}


int
getnewuserid()
{
    char            genbuf[50];
    char    *fn_fresh = ".fresh";
    userec_t        utmp;
    time_t          clock;
    struct stat     st;
    int             fd, i;

    clock = now;

    /* Lazy method : ����M�w�g�M�����L���b�� */
    if ((i = searchnewuser(0)) == 0) {
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
	    for (i = 2; i <= MAX_USERS; i++) {
		passwd_query(i, &utmp);
		check_and_expire_account(i, &utmp);
	    }
	}
    }
    passwd_lock();
    i = searchnewuser(1);
    if ((i <= 0) || (i > MAX_USERS)) {
	passwd_unlock();
	vmsg("��p�A�ϥΪ̱b���w�g���F�A�L�k���U�s���b��");
	exit(1);
    }
    snprintf(genbuf, sizeof(genbuf), "uid %d", i);
    log_usies("APPLY", genbuf);

    kill_user(i);
    passwd_unlock();
    return i;
}

void
new_register()
{
    userec_t        newuser;
    char            passbuf[STRLEN];
    int             allocid, try, id, uid;

#ifdef HAVE_USERAGREEMENT
    more(HAVE_USERAGREEMENT, YEA);
    while( 1 ){
	getdata(b_lines - 1, 0, "�аݱz�����o���ϥΪ̱��ڶ�? (yes/no) ",
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
    memset(&newuser, 0, sizeof(newuser));
    more("etc/register", NA);
    try = 0;
    while (1) {
	if (++try >= 6) {
	    vmsg("�z���տ��~����J�Ӧh�A�ФU���A�ӧa");
	    exit(1);
	}
	getdata(17, 0, msg_uid, newuser.userid,
		sizeof(newuser.userid), DOECHO);

	if (bad_user_id(newuser.userid))
	    outs("�L�k�����o�ӥN���A�Шϥέ^��r���A�åB���n�]�t�Ů�\n");
	else if ((id = getuser(newuser.userid)) &&
		 (id = check_and_expire_account(id, &xuser)) >= 0) {
	    if (id == 999999)
		outs("���N���w�g���H�ϥ� �O��������");
	    else {
		prints("���N���w�g���H�ϥ� �٦�%d�Ѥ~�L�� \n", id / (60 * 24));
	    }
	} else
	    break;
    }

    try = 0;
    while (1) {
	if (++try >= 6) {
	    vmsg("�z���տ��~����J�Ӧh�A�ФU���A�ӧa");
	    exit(1);
	}
	if ((getdata(19, 0, "�г]�w�K�X�G", passbuf,
		     sizeof(passbuf), NOECHO) < 3) ||
	    !strcmp(passbuf, newuser.userid)) {
	    outs("�K�X��²��A���D�J�I�A�ܤ֭n 4 �Ӧr�A�Э��s��J\n");
	    continue;
	}
	strncpy(newuser.passwd, passbuf, PASSLEN);
	getdata(20, 0, "���ˬd�K�X�G", passbuf, sizeof(passbuf), NOECHO);
	if (strncmp(passbuf, newuser.passwd, PASSLEN)) {
	    outs("�K�X��J���~, �Э��s��J�K�X.\n");
	    continue;
	}
	passbuf[8] = '\0';
	strncpy(newuser.passwd, genpasswd(passbuf), PASSLEN);
	break;
    }
    newuser.userlevel = PERM_DEFAULT;
    newuser.uflag = COLOR_FLAG | BRDSORT_FLAG | MOVIE_FLAG;
    newuser.uflag2 = 0;
    newuser.firstlogin = newuser.lastlogin = now;
    newuser.money = 0;
    newuser.pager = 1;
    allocid = getnewuserid();
    if (allocid > MAX_USERS || allocid <= 0) {
	fprintf(stderr, "�����H�f�w�F���M�I\n");
	exit(1);
    }
    if (passwd_update(allocid, &newuser) == -1) {
	fprintf(stderr, "�Ⱥ��F�A�A���I\n");
	exit(1);
    }
    setuserid(allocid, newuser.userid);
    if( (uid = initcuser(newuser.userid)) )
	setumoney(uid, 0);
    else{
	fprintf(stderr, "�L�k�إ߱b��\n");
	exit(1);
    }
}


void
check_register()
{
    char           *ptr = NULL;

    if (HAS_PERM(PERM_LOGINOK))
	return;

    /* 
     * �קK�ϥΪ̳Q�h�^���U���A�b���D�h�^����]���e�A
     * �S�e�X�@�����U��C
     */ 
    if (currutmp->mailalert)
	m_read();

    stand_title("�иԲӶ�g�ӤH���");

    while (strlen(cuser.username) < 2)
	getdata(2, 0, "�︹�ʺ١G", cuser.username,
		sizeof(cuser.username), DOECHO);

    for (ptr = cuser.username; *ptr; ptr++) {
	if (*ptr == 9)		/* TAB convert */
	    *ptr = ' ';
    }
    while (strlen(cuser.realname) < 4)
	getdata(4, 0, "�u��m�W�G", cuser.realname,
		sizeof(cuser.realname), DOECHO);

    while (strlen(cuser.address) < 8)
	getdata(6, 0, "�p���a�}�G", cuser.address,
		sizeof(cuser.address), DOECHO);


    /*
     * if(!strchr(cuser.email, '@')) { bell(); move(t_lines - 4, 0); prints("
     * �z���v�q�A�ж�g�u�ꪺ E-mail address�A" "�H��T�{�դU�����A\n" "
     * 033[44muser@domain_name\033[0m �� \033[44muser"
     * "@\\[ip_number\\]\033[0m�C\n\n" "�� �p�G�z�u���S�� E-mail�A turn]
     * �Y�i�C");
     * 
     * do { getdata(8, 0, "�q�l�H�c�G", cuser.email, sizeof(cuser->email),
     * DOECHO); if(!cuser.email[0]) sprintf(cuser->email, "%s%s",
     * cuser.userid, str_mail_address); } while(!strchr(cuser->email, '@'));
     * 
     * } */
    if (!HAS_PERM(PERM_SYSOP)) {
	/* �^�йL�����{�ҫH��A�δ��g E-mail post �L */
	clear();
	move(9, 3);
	prints("�иԶ�g\033[32m���U�ӽг�\033[m�A"
	       "�q�i�����H��o�i���ϥ��v�O�C\n\n\n\n");
	u_register();

#ifdef NEWUSER_LIMIT
	if (cuser.lastlogin - cuser->firstlogin < 3 * 86400)
	    cuser.userlevel &= ~PERM_POST;
	more("etc/newuser", YEA);
#endif
    }
}
