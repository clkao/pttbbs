/* $Id: user.c,v 1.31 2002/07/21 09:26:02 in2 Exp $ */
#include "bbs.h"

static char    *sex[8] = {
    MSG_BIG_BOY, MSG_BIG_GIRL, MSG_LITTLE_BOY, MSG_LITTLE_GIRL,
    MSG_MAN, MSG_WOMAN, MSG_PLANT, MSG_MIME
};

int
u_loginview()
{
    int             i;
    unsigned int    pbits = cuser.loginview;
    char            choice[5];

    clear();
    move(4, 0);
    for (i = 0; i < NUMVIEWFILE; i++)
	prints("    %c. %-20s %-15s \n", 'A' + i,
	       loginview_file[i][1], ((pbits >> i) & 1 ? "��" : "��"));

    clrtobot();
    while (getdata(b_lines - 1, 0, "�Ы� [A-N] �����]�w�A�� [Return] �����G",
		   choice, sizeof(choice), LCECHO)) {
	i = choice[0] - 'a';
	if (i >= NUMVIEWFILE || i < 0)
	    bell();
	else {
	    pbits ^= (1 << i);
	    move(i + 4, 28);
	    prints((pbits >> i) & 1 ? "��" : "��");
	}
    }

    if (pbits != cuser.loginview) {
	cuser.loginview = pbits;
	passwd_update(usernum, &cuser);
    }
    return 0;
}

void
user_display(userec_t * u, int real)
{
    int             diff = 0;
    char            genbuf[200];

    clrtobot();
    prints(
	   "        \033[30;41m�r�s�r�s�r�s\033[m  \033[1;30;45m    �� �� ��"
	   " �� ��        "
	   "     \033[m  \033[30;41m�r�s�r�s�r�s\033[m\n");
    prints("                �N���ʺ�: %s(%s)\n"
	   "                �u��m�W: %s\n"
	   "                �~���}: %s\n"
	   "                �q�l�H�c: %s\n"
	   "                ��    �O: %s\n"
	   "                �Ȧ�b��: %ld �Ȩ�\n",
	   u->userid, u->username, u->realname, u->address, u->email,
	   sex[u->sex % 8], u->money);

    sethomedir(genbuf, u->userid);
    prints("                �p�H�H�c: %d ��  (�ʶR�H�c: %d ��)\n"
	   "                �����Ҹ�: %s\n"
	   "                ������X: %010d\n"
	   "                ��    ��: %02i/%02i/%02i\n"
	   "                �p���W�r: %s\n",
	   get_num_records(genbuf, sizeof(fileheader_t)),
	   u->exmailbox, u->ident, u->mobile,
	   u->month, u->day, u->year % 100, u->mychicken.name);
    prints("                ���U���: %s", ctime(&u->firstlogin));
    prints("                �e�����{: %s", ctime(&u->lastlogin));
    prints("                �e���I�q: %s", ctime(&u->lastsong));
    prints("                �W���峹: %d �� / %d �g\n",
	   u->numlogins, u->numposts);

    if (real) {
	strlcpy(genbuf, "bTCPRp#@XWBA#VSM0123456789ABCDEF", sizeof(genbuf));
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
    outs("        \033[30;41m�r�s�r�s�r�s�r�s�r�s�r�s�r�s�r�s�r�s�r�s�r�s�r"
	 "�s�r�s�r�s�r�s\033[m");

    outs((u->userlevel & PERM_LOGINOK) ?
	 "\n�z�����U�{�Ǥw�g�����A�w��[�J����" :
	 "\n�p�G�n���@�v���A�аѦҥ������G���z���U");

#ifdef NEWUSER_LIMIT
    if ((u->lastlogin - u->firstlogin < 3 * 86400) && !HAS_PERM(PERM_POST))
	outs("\n�s��W���A�T�ѫ�}���v��");
#endif
}

void
mail_violatelaw(char *crime, char *police, char *reason, char *result)
{
    char            genbuf[200];
    fileheader_t    fhdr;
    FILE           *fp;
    sprintf(genbuf, "home/%c/%s", crime[0], crime);
    stampfile(genbuf, &fhdr);
    if (!(fp = fopen(genbuf, "w")))
	return;
    fprintf(fp, "�@��: [Ptt�k�|]\n"
	    "���D: [���i] �H�k�P�M���i\n"
	    "�ɶ�: %s\n"
	    "\033[1;32m%s\033[m�P�M�G\n     \033[1;32m%s\033[m"
	    "�]\033[1;35m%s\033[m�欰�A\n�H�ϥ������W�A�B�H\033[1;35m%s\033[m�A�S���q��"
	"\n�Ш� PttLaw �d�߬����k�W��T�A�è� Play-Pay-ViolateLaw ú��@��",
	    ctime(&now), police, crime, reason, result);
    fclose(fp);
    sprintf(fhdr.title, "[���i] �H�k�P�M���i");
    strlcpy(fhdr.owner, "[Ptt�k�|]", sizeof(fhdr.owner));
    sprintf(genbuf, "home/%c/%s/.DIR", crime[0], crime);
    append_record(genbuf, &fhdr, sizeof(fhdr));
}

static void
violate_law(userec_t * u, int unum)
{
    char            ans[4], ans2[4];
    char            reason[128];
    move(1, 0);
    clrtobot();
    move(2, 0);
    prints("(1)Cross-post (2)�õo�s�i�H (3)�õo�s��H\n");
    prints("(4)���Z���W�ϥΪ� (8)��L�H�@��B�m�欰\n(9)�� id �欰\n");
    getdata(5, 0, "(0)����", ans, sizeof(ans), DOECHO);
    switch (ans[0]) {
    case '1':
	sprintf(reason, "%s", "Cross-post");
	break;
    case '2':
	sprintf(reason, "%s", "�õo�s�i�H");
	break;
    case '3':
	sprintf(reason, "%s", "�õo�s��H");
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
    getdata(7, 0, msg_sure_ny, ans2, sizeof(ans2), LCECHO);
    if (*ans2 != 'y')
	return;
    if (ans[0] == '9') {
	char            src[STRLEN], dst[STRLEN];
	sprintf(src, "home/%c/%s", u->userid[0], u->userid);
	sprintf(dst, "tmp/%s", u->userid);
	Rename(src, dst);
	log_usies("KILL", u->userid);
	post_violatelaw(u->userid, cuser.userid, reason, "�尣 ID");
	u->userid[0] = '\0';
	setuserid(unum, u->userid);
	passwd_update(unum, u);
    } else {
	u->userlevel |= PERM_VIOLATELAW;
	u->vl_count++;
	passwd_update(unum, u);
	post_violatelaw(u->userid, cuser.userid, reason, "�@��B��");
	mail_violatelaw(u->userid, cuser.userid, reason, "�@��B��");
    }
    pressanykey();
}


void
uinfo_query(userec_t * u, int real, int unum)
{
    userec_t        x;
    register int    i = 0, fail, mail_changed;
    int             uid;
    char            ans[4], buf[STRLEN], *p;
    char            genbuf[200], reason[50];
    unsigned long int money = 0;
    fileheader_t    fhdr;
    int             flag = 0, temp = 0, money_change = 0;

    FILE           *fp;

    fail = mail_changed = 0;

    memcpy(&x, u, sizeof(userec_t));
    getdata(b_lines - 1, 0, real ?
	    "(1)����(2)�]�K�X(3)�]�v��(4)��b��(5)��ID"
	    "(6)��/�_���d��(7)�f�P [0]���� " :
	    "�п�� (1)�ק��� (2)�]�w�K�X ==> [0]���� ",
	    ans, sizeof(ans), DOECHO);

    if (ans[0] > '2' && !real)
	ans[0] = '0';

    if (ans[0] == '1' || ans[0] == '3') {
	clear();
	i = 2;
	move(i++, 0);
	outs(msg_uid);
	outs(x.userid);
    }
    switch (ans[0]) {
    case '7':
	violate_law(&x, unum);
	return;
    case '1':
	move(0, 0);
	outs("�гv���ק�C");

	getdata_buf(i++, 0, " �� ��  �G", x.username,
		    sizeof(x.username), DOECHO);
	if (real) {
	    getdata_buf(i++, 0, "�u��m�W�G",
			x.realname, sizeof(x.realname), DOECHO);
	    getdata_buf(i++, 0, "�����Ҹ��G",
			x.ident, sizeof(x.ident), DOECHO);
	    getdata_buf(i++, 0, "�~��a�}�G",
			x.address, sizeof(x.address), DOECHO);
	}
	sprintf(buf, "%010d", x.mobile);
	getdata_buf(i++, 0, "������X�G", buf, 11, LCECHO);
	x.mobile = atoi(buf);
	getdata_str(i++, 0, "�q�l�H�c[�ܰʭn���s�{��]�G", buf, 50, DOECHO,
		    x.email);
	if (strcmp(buf, x.email) && strchr(buf, '@')) {
	    strlcpy(x.email, buf, sizeof(x.email));
	    mail_changed = 1 - real;
	}
	sprintf(genbuf, "%i", (u->sex + 1) % 8);
	getdata_str(i++, 0, "�ʧO (1)���� (2)�j�� (3)���} (4)���� (5)���� "
		    "(6)���� (7)�Ӫ� (8)�q���G",
		    buf, 3, DOECHO, genbuf);
	if (buf[0] >= '1' && buf[0] <= '8')
	    x.sex = (buf[0] - '1') % 8;
	else
	    x.sex = u->sex % 8;

	while (1) {
	    int             len;

	    sprintf(genbuf, "%02i/%02i/%02i",
		    u->month, u->day, u->year % 100);
	    len = getdata_str(i, 0, "�ͤ� ���/���/�褸�G", buf, 9,
			      DOECHO, genbuf);
	    if (len && len != 8)
		continue;
	    if (!len) {
		x.month = u->month;
		x.day = u->day;
		x.year = u->year;
	    } else if (len == 8) {
		x.month = (buf[0] - '0') * 10 + (buf[1] - '0');
		x.day = (buf[3] - '0') * 10 + (buf[4] - '0');
		x.year = (buf[6] - '0') * 10 + (buf[7] - '0');
	    } else
		continue;
	    if (!real && (x.month > 12 || x.month < 1 || x.day > 31 ||
			  x.day < 1 || x.year > 90 || x.year < 40))
		continue;
	    i++;
	    break;
	}
	if (real) {
	    unsigned long int l;
	    if (HAS_PERM(PERM_BBSADM)) {
		sprintf(genbuf, "%d", x.money);
		if (getdata_str(i++, 0, "�Ȧ�b��G", buf, 10, DOECHO, genbuf))
		    if ((l = atol(buf)) != 0) {
			if (l != x.money) {
			    money_change = 1;
			    money = x.money;
			    x.money = l;
			}
		    }
	    }
	    sprintf(genbuf, "%d", x.exmailbox);
	    if (getdata_str(i++, 0, "�ʶR�H�c�ơG", buf, 4, DOECHO, genbuf))
		if ((l = atol(buf)) != 0)
		    x.exmailbox = (int)l;

	    getdata_buf(i++, 0, "�{�Ҹ�ơG", x.justify,
			sizeof(x.justify), DOECHO);
	    getdata_buf(i++, 0, "�̪���{�����G",
			x.lasthost, sizeof(x.lasthost), DOECHO);

	    sprintf(genbuf, "%d", x.numlogins);
	    if (getdata_str(i++, 0, "�W�u���ơG", buf, 10, DOECHO, genbuf))
		if ((fail = atoi(buf)) >= 0)
		    x.numlogins = fail;

	    sprintf(genbuf, "%d", u->numposts);
	    if (getdata_str(i++, 0, "�峹�ƥءG", buf, 10, DOECHO, genbuf))
		if ((fail = atoi(buf)) >= 0)
		    x.numposts = fail;
	    sprintf(genbuf, "%d", u->vl_count);
	    if (getdata_str(i++, 0, "�H�k�O���G", buf, 10, DOECHO, genbuf))
		if ((fail = atoi(buf)) >= 0)
		    x.vl_count = fail;

	    sprintf(genbuf, "%d/%d/%d", u->five_win, u->five_lose,
		    u->five_tie);
	    if (getdata_str(i++, 0, "���l�Ѿ��Z ��/��/�M�G", buf, 16, DOECHO,
			    genbuf))
		while (1) {
		    p = strtok(buf, "/\r\n");
		    if (!p)
			break;
		    x.five_win = atoi(p);
		    p = strtok(NULL, "/\r\n");
		    if (!p)
			break;
		    x.five_lose = atoi(p);
		    p = strtok(NULL, "/\r\n");
		    if (!p)
			break;
		    x.five_tie = atoi(p);
		    break;
		}
	    sprintf(genbuf, "%d/%d/%d", u->chc_win, u->chc_lose, u->chc_tie);
	    if (getdata_str(i++, 0, "�H�Ѿ��Z ��/��/�M�G", buf, 16, DOECHO,
			    genbuf))
		while (1) {
		    p = strtok(buf, "/\r\n");
		    if (!p)
			break;
		    x.chc_win = atoi(p);
		    p = strtok(NULL, "/\r\n");
		    if (!p)
			break;
		    x.chc_lose = atoi(p);
		    p = strtok(NULL, "/\r\n");
		    if (!p)
			break;
		    x.chc_tie = atoi(p);
		    break;
		}
	    fail = 0;
	}
	break;

    case '2':
	i = 19;
	if (!real) {
	    if (!getdata(i++, 0, "�п�J��K�X�G", buf, PASSLEN, NOECHO) ||
		!checkpasswd(u->passwd, buf)) {
		outs("\n\n�z��J���K�X�����T\n");
		fail++;
		break;
	    }
	} else {
	    char            witness[3][32];
	    for (i = 0; i < 3; i++) {
		if (!getdata(19 + i, 0, "�п�J��U�ҩ����ϥΪ̡G",
			     witness[i], sizeof(witness[i]), DOECHO)) {
		    outs("\n����J�h�L�k���\n");
		    fail++;
		    break;
		} else if (!(uid = getuser(witness[i]))) {
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
		}
	    }
	    if (i < 3)
		break;
	    else
		i = 20;
	}

	if (!getdata(i++, 0, "�г]�w�s�K�X�G", buf, PASSLEN, NOECHO)) {
	    outs("\n\n�K�X�]�w����, �~��ϥ��±K�X\n");
	    fail++;
	    break;
	}
	strncpy(genbuf, buf, PASSLEN);

	getdata(i++, 0, "���ˬd�s�K�X�G", buf, PASSLEN, NOECHO);
	if (strncmp(buf, genbuf, PASSLEN)) {
	    outs("\n\n�s�K�X�T�{����, �L�k�]�w�s�K�X\n");
	    fail++;
	    break;
	}
	buf[8] = '\0';
	strncpy(x.passwd, genpasswd(buf), PASSLEN);
	if (real)
	    x.userlevel &= (!PERM_LOGINOK);
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
	    if (searchuser(genbuf)) {
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
    getdata(b_lines - 1, 0, msg_sure_ny, ans, 3, LCECHO);
    if (*ans == 'y') {
	if (flag)
	    post_change_perm(temp, i, cuser.userid, x.userid);
	if (strcmp(u->userid, x.userid)) {
	    char            src[STRLEN], dst[STRLEN];

	    sethomepath(src, u->userid);
	    sethomepath(dst, x.userid);
	    Rename(src, dst);
	    setuserid(unum, x.userid);
	}
	memcpy(u, &x, sizeof(x));
	if (mail_changed) {
#ifdef EMAIL_JUSTIFY
	    x.userlevel &= ~PERM_LOGINOK;
	    mail_justify();
#endif
	}
	if (i == QUIT) {
	    char            src[STRLEN], dst[STRLEN];

	    sprintf(src, "home/%c/%s", x.userid[0], x.userid);
	    sprintf(dst, "tmp/%s", x.userid);
	    if (Rename(src, dst)) {
		sprintf(genbuf, "/bin/rm -fr %s >/dev/null 2>&1", src);
		/*
		 * do not remove system(genbuf);
		 */
	    }
	    log_usies("KILL", x.userid);
	    x.userid[0] = '\0';
	    setuserid(unum, x.userid);
	} else
	    log_usies("SetUser", x.userid);
	if (money_change)
	    setumoney(unum, x.money);
	passwd_update(unum, &x);
	if (money_change) {
	    strlcpy(genbuf, "boards/S/Security", sizeof(genbuf));
	    stampfile(genbuf, &fhdr);
	    if (!(fp = fopen(genbuf, "w")))
		return;

	    fprintf(fp, "�@��: [�t�Φw����] �ݪO: Security\n"
		    "���D: [���w���i] �����ק�������i\n"
		    "�ɶ�: %s\n"
		    "   ����\033[1;32m%s\033[m��\033[1;32m%s\033[m"
		    "�����q\033[1;35m%ld\033[m�令\033[1;35m%d\033[m",
		    ctime(&now), cuser.userid, x.userid, money, x.money);

	    clrtobot();
	    clear();
	    while (!getdata(5, 0, "�п�J�z�ѥH�ܭt�d�G",
			    reason, sizeof(reason), DOECHO));

	    fprintf(fp, "\n   \033[1;37m����%s�ק���z�ѬO�G%s\033[m",
		    cuser.userid, reason);
	    fclose(fp);
	    sprintf(fhdr.title, "[���w���i] ����%s�ק�%s�����i", cuser.userid,
		    x.userid);
	    strlcpy(fhdr.owner, "[�t�Φw����]", sizeof(fhdr.owner));
	    append_record("boards/S/Security/.DIR", &fhdr, sizeof(fhdr));
	}
    }
}

int
u_info()
{
    move(2, 0);
    user_display(&cuser, 0);
    uinfo_query(&cuser, 0, usernum);
    strlcpy(currutmp->realname, cuser.realname, sizeof(currutmp->realname));
    strlcpy(currutmp->username, cuser.username, sizeof(currutmp->username));
    return 0;
}

int
u_ansi()
{
    showansi ^= 1;
    cuser.uflag ^= COLOR_FLAG;
    outs(reset_color);
    return 0;
}

int
u_cloak()
{
    outs((currutmp->invisible ^= 1) ? MSG_CLOAKED : MSG_UNCLOAK);
    return XEASY;
}

int
u_switchproverb()
{
    /* char *state[4]={"�Υ\\��","�w�h��","�۩w��","SHUTUP"}; */
    char            buf[100];

    cuser.proverb = (cuser.proverb + 1) % 4;
    setuserfile(buf, fn_proverb);
    if (cuser.proverb == 2 && dashd(buf)) {
	FILE           *fp = fopen(buf, "a");

	fprintf(fp, "�y�k�ʪ��A��[�۩w��]�n�O�o�]�y�k�ʪ����e��!!");
	fclose(fp);
    }
    passwd_update(usernum, &cuser);
    return 0;
}

int
u_editproverb()
{
    char            buf[100];

    setutmpmode(PROVERB);
    setuserfile(buf, fn_proverb);
    move(1, 0);
    clrtobot();
    outs("\n\n �Ф@��@��̧���J�Q�t�δ����A�����e,\n"
	 " �x�s��O�o�⪬�A�]�� [�۩w��] �~���@��\n"
	 " �y�k�ʳ̦h100��");
    pressanykey();
    vedit(buf, NA, NULL);
    return 0;
}

void
showplans(char *uid)
{
    char            genbuf[200];

    sethomefile(genbuf, uid, fn_plans);
    if (!show_file(genbuf, 7, MAX_QUERYLINES, ONLY_COLOR))
	prints("�m�ӤH�W���n%s �ثe�S���W��", uid);
}

int
showsignature(char *fname)
{
    FILE           *fp;
    char            buf[256];
    int             i, j;
    char            ch;

    clear();
    move(2, 0);
    setuserfile(fname, "sig.0");
    j = strlen(fname) - 1;

    for (ch = '1'; ch <= '9'; ch++) {
	fname[j] = ch;
	if ((fp = fopen(fname, "r"))) {
	    prints("\033[36m�i ñ�W��.%c �j\033[m\n", ch);
	    for (i = 0; i++ < MAX_SIGLINES && fgets(buf, 256, fp); outs(buf));
	    fclose(fp);
	}
    }
    return j;
}

int
u_editsig()
{
    int             aborted;
    char            ans[4];
    int             j;
    char            genbuf[200];

    j = showsignature(genbuf);

    getdata(0, 0, "ñ�W�� (E)�s�� (D)�R�� (Q)�����H[Q] ",
	    ans, sizeof(ans), LCECHO);

    aborted = 0;
    if (ans[0] == 'd')
	aborted = 1;
    if (ans[0] == 'e')
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
u_editplan()
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
u_editcalendar()
{
    char            genbuf[200];

    getdata(b_lines - 1, 0, "��ƾ� (D)�R�� (E)�s�� [Q]�����H[Q] ",
	    genbuf, 3, LCECHO);

    if (genbuf[0] == 'e') {
	int             aborted;

	setutmpmode(EDITPLAN);
	setcalfile(genbuf, cuser.userid);
	aborted = vedit(genbuf, NA, NULL);
	if (aborted != -1)
	    outs("��ƾ��s����");
	pressanykey();
	return 0;
    } else if (genbuf[0] == 'd') {
	setcalfile(genbuf, cuser.userid);
	unlink(genbuf);
	outmsg("��ƾ�R������");
    }
    return 0;
}

/* �ϥΪ̶�g���U��� */
static void
getfield(int line, char *info, char *desc, char *buf, int len)
{
    char            prompt[STRLEN];
    char            genbuf[200];

    sprintf(genbuf, "����]�w�G%-30.30s (%s)", buf, info);
    move(line, 2);
    outs(genbuf);
    sprintf(prompt, "%s�G", desc);
    if (getdata_str(line + 1, 2, prompt, genbuf, len, DOECHO, buf))
	strlcpy(buf, genbuf, sizeof(buf));
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

static int
ispersonalid(char *inid)
{
    char           *lst = "ABCDEFGHJKLMNPQRSTUVWXYZIO", id[20];
    int             i, j, cksum;

    strlcpy(id, inid, sizeof(id));
    i = cksum = 0;
    if (!isalpha(id[0]) && (strlen(id) != 10))
	return 0;
    id[0] = toupper(id[0]);
    /* A->10, B->11, ..H->17,I->34, J->18... */
    while (lst[i] != id[0])
	i++;
    i += 10;
    id[0] = i % 10 + '0';
    if (!isdigit(id[9]))
	return 0;
    cksum += (id[9] - '0') + (i / 10);

    for (j = 0; j < 9; ++j) {
	if (!isdigit(id[j]))
	    return 0;
	cksum += (id[j] - '0') * (9 - j);
    }
    return (cksum % 10) == 0;
}

static char    *
getregcode(char *buf)
{
    sprintf(buf, "%s", crypt(cuser.userid, "02"));
    return buf;
}

static int
isvaildemail(char *email)
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
	    buf[strlen(buf) - 1] = 0;
	    if (buf[0] == 'A' && strcmp(&buf[1], email) == 0)
		return 0;
	    if (buf[0] == 'P' && strstr(email, &buf[1]))
		return 0;
	    if (buf[0] == 'S' && strcmp(strstr(email, "@") + 1, &buf[1]) == 0)
		return 0;
	}
	fclose(fp);
    }
    return 1;
}

static void
toregister(char *email, char *genbuf, char *phone, char *career,
	   char *ident, char *rname, char *addr, char *mobile)
{
    FILE           *fn;
    char            buf[128];

    sethomefile(buf, cuser.userid, "justify.wait");
    if (phone[0] != 0) {
	fn = fopen(buf, "w");
	fprintf(fn, "%s\n%s\n%s\n%s\n%s\n%s\n",
		phone, career, ident, rname, addr, mobile);
	fclose(fn);
    }
    clear();
    stand_title("�{�ҳ]�w");
    move(2, 0);
    outs("�z�n, �����{�һ{�Ҫ��覡��:\n"
	 "  1.�Y�z�� E-Mail  (���������� yahoo, kimo���K�O�� E-Mail)\n"
	 "    �п�J�z�� E-Mail , �ڭ̷|�H�o�t���{�ҽX���H�󵹱z\n"
	 "    �����Ш� (U)ser => (R)egister ��J�{�ҽX, �Y�i�q�L�{��\n"
	 "\n"
	 "  2.�Y�z�S�� E-Mail , �п�J x ,\n"
	 "    �ڭ̷|�ѯ����˦ۼf�ֱz�����U���\n"
	 "************************************************************\n"
	 "* �`�N!                                                    *\n"
	 "* �z���ӷ|�b��J������Q����������{�ҫH, �Y�L�[������,    *\n"
	 "* �ο�J��o�ͻ{�ҽX���~, �·Э���@�� E-Mail �Χ��ʻ{�� *\n"
	 "************************************************************\n");

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
	    if (isvaildmobile(mobile)) {
		char            yn[3];
		getdata(16, 0, "�ЦA���T�{�z��J��������X���T��? [y/N]",
			yn, sizeof(yn), LCECHO);
		if (yn[0] == 'Y' || yn[0] == 'y')
		    break;
	    } else {
		move(17, 0);
		prints("���w��������X���X�k,"
		       "�Y�z�L��������п�ܨ�L�覡�{��");
	    }

	}
#endif
	else if (isvaildemail(email)) {
	    char            yn[3];
	    getdata(16, 0, "�ЦA���T�{�z��J�� E-Mail ��m���T��? [y/N]",
		    yn, sizeof(yn), LCECHO);
	    if (yn[0] == 'Y' || yn[0] == 'y')
		break;
	} else {
	    move(17, 0);
	    prints("���w�� E-Mail ���X�k,"
		   "�Y�z�L E-Mail �п�J x�ѯ�����ʻ{��");
	}
    }
    strncpy(cuser.email, email, sizeof(cuser.email));
    if (strcasecmp(email, "x") == 0) {	/* ��ʻ{�� */
	if ((fn = fopen(fn_register, "a"))) {
	    fprintf(fn, "num: %d, %s", usernum, ctime(&now));
	    fprintf(fn, "uid: %s\n", cuser.userid);
	    fprintf(fn, "ident: %s\n", ident);
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
	char            tmp[IDLEN + 1];
	if (phone != NULL) {
#ifdef HAVEMOBILE
	    if (strcmp(email, "m") == 0 || strcmp(email, "M") == 0)
		sprintf(genbuf, "%s:%s:<Mobile>", phone, career);
	    else
#endif
		sprintf(genbuf, "%s:%s:<Email>", phone, career);
	    strncpy(cuser.justify, genbuf, REGLEN);
	    sethomefile(buf, cuser.userid, "justify");
	}
	sprintf(buf, "�z�b " BBSNAME " ���{�ҽX: %s", getregcode(genbuf));
	strlcpy(tmp, cuser.userid, sizeof(tmp));
	strlcpy(cuser.userid, "SYSOP", sizeof(cuser.userid));
#ifdef HAVEMOBILE
	if (strcmp(email, "m") == 0 || strcmp(email, "M") == 0)
	    mobile_message(mobile, buf);
	else
#endif
	    bsmtp("etc/registermail", buf, email, 0);
	strlcpy(cuser.userid, tmp, sizeof(cuser.userid));
	outs("\n\n\n�ڭ̧Y�N�H�X�{�ҫH (�z���ӷ|�b 10 ����������)\n"
	     "�����z�i�H��ڻ{�ҫH���D���{�ҽX\n"
	     "��J�� (U)ser -> (R)egister ��N�i�H�������U");
	pressanykey();
	return;
    }
}

int
u_register(void)
{
    char            rname[21], addr[51], ident[12], mobile[21];
    char            phone[21], career[41], email[51], birthday[9], sex_is[2],
                    year, mon, day;
    char            inregcode[14], regcode[50];
    char            ans[3], *ptr;
    char            genbuf[200];
    FILE           *fn;

    if (cuser.userlevel & PERM_LOGINOK) {
	outs("�z�������T�{�w�g�����A���ݶ�g�ӽЪ�");
	return XEASY;
    }
    if ((fn = fopen(fn_register, "r"))) {
	while (fgets(genbuf, STRLEN, fn)) {
	    if ((ptr = strchr(genbuf, '\n')))
		*ptr = '\0';
	    if (strncmp(genbuf, "uid: ", 5) == 0 &&
		strcmp(genbuf + 5, cuser.userid) == 0) {
		fclose(fn);
		outs("�z�����U�ӽг�|�b�B�z���A�Э@�ߵ���");
		return XEASY;
	    }
	}
	fclose(fn);
    }
    strlcpy(ident, cuser.ident, sizeof(ident));
    strlcpy(rname, cuser.realname, sizeof(rname));
    strlcpy(addr, cuser.address, sizeof(addr));
    strlcpy(email, cuser.email, sizeof(email));
    sprintf(mobile, "0%09d", cuser.mobile);
    if (cuser.month == 0 && cuser.day && cuser.year == 0)
	birthday[0] = 0;
    else
	sprintf(birthday, "%02i/%02i/%02i",
		cuser.month, cuser.day, cuser.year % 100);
    sex_is[0] = (cuser.sex % 8) + '1';
    sex_is[1] = 0;
    career[0] = phone[0] = '\0';
    sethomefile(genbuf, cuser.userid, "justify.wait");
    if ((fn = fopen(genbuf, "r"))) {
	fgets(phone, 21, fn);
	phone[strlen(phone) - 1] = 0;
	fgets(career, 41, fn);
	career[strlen(career) - 1] = 0;
	fgets(ident, 12, fn);
	ident[strlen(ident) - 1] = 0;
	fgets(rname, 21, fn);
	rname[strlen(rname) - 1] = 0;
	fgets(addr, 51, fn);
	addr[strlen(addr) - 1] = 0;
	fgets(mobile, 21, fn);
	mobile[strlen(mobile) - 1] = 0;
	fclose(fn);
    }
    if (cuser.year != 0 &&	/* �w�g�Ĥ@����L�F~ ^^" */
	strcmp(cuser.email, "x") != 0 &&	/* �W����ʻ{�ҥ��� */
	strcmp(cuser.email, "X") != 0) {
	clear();
	stand_title("EMail�{��");
	move(2, 0);
	prints("%s(%s) �z�n�A�п�J�z���{�ҽX�C\n"
	       "�αz�i�H��J x�ӭ��s��g E-Mail �Χ�ѯ�����ʻ{��",
	       cuser.userid, cuser.username);
	inregcode[0] = 0;
	getdata(10, 0, "�z����J: ", inregcode, sizeof(inregcode), DOECHO);
	if (strcmp(inregcode, getregcode(regcode)) == 0) {
	    int             unum;
	    if ((unum = getuser(cuser.userid)) == 0) {
		outs("�t�ο��~�A�d�L���H\n\n");
		pressanykey();
		u_exit("getuser error");
		exit(0);
	    }
	    mail_muser(cuser, "[���U���\\�o]", "etc/registeredmail");
	    cuser.userlevel |= (PERM_LOGINOK | PERM_POST);
	    prints("\n���U���\\, ���s�W����N���o�����v��\n"
		   "�Ы��U���@������᭫�s�W��~ :)");
	    sethomefile(genbuf, cuser.userid, "justify.wait");
	    unlink(genbuf);
	    pressanykey();
	    u_exit("registed");
	    exit(0);
	    return QUIT;
	} else if (strcmp(inregcode, "x") != 0 && strcmp(inregcode, "X") != 0) {
	    outs("�{�ҽX���~\n");
	    pressanykey();
	}
	toregister(email, genbuf, phone, career, ident, rname, addr, mobile);
	return FULLUPDATE;
    }
    getdata(b_lines - 1, 0, "�z�T�w�n��g���U���(Y/N)�H[N] ",
	    ans, sizeof(ans), LCECHO);
    if (ans[0] != 'y')
	return FULLUPDATE;

    move(2, 0);
    clrtobot();
    while (1) {
	clear();
	move(1, 0);
	prints("%s(%s) �z�n�A�оڹ��g�H�U�����:",
	       cuser.userid, cuser.username);
	do {
	    getfield(3, "D120908396", "�����Ҹ�", ident, 11);
	    if ('a' <= ident[0] && ident[0] <= 'z')
		ident[0] -= 32;
	} while (!ispersonalid(ident));
	while (1) {
	    getfield(5, "�ХΤ���", "�u��m�W", rname, 20);
	    if (removespace(rname) && rname[0] < 0 &&
		!strstr(rname, "��") && !strstr(rname, "�p"))
		break;
	    vmsg("�z����J�����T");
	}

	while (1) {
	    getfield(7, "�Ǯ�(�t\033[1;33m�t�Ҧ~��\033[m)�γ��¾��",
		     "�A�ȳ��", career, 40);
	    if (!(removespace(career) && career[0] < 0 && strlen(career) >= 4)) {
		vmsg("�z����J�����T");
		continue;
	    }
	    if (strcmp(&career[strlen(career) - 2], "�j") == 0 ||
		strcmp(&career[strlen(career) - 4], "�j��") == 0) {
		vmsg("�·нХ[�t��");
		continue;
	    }
	    break;
	}
	while (1) {
	    getfield(9, "�t\033[1;33m����\033[m�Ϊ��츹�X"
		     "(�x�_�Х[\033[1;33m��F��\033[m)",
		     "�ثe��}", addr, 50);
	    if (!removespace(addr) || addr[0] > 0 || strlen(addr) < 15) {
		vmsg("�o�Ӧa�}�ä��X�k");
		continue;
	    }
	    if (strstr(addr, "�H�c") != NULL || strstr(addr, "�l�F") != NULL) {
		vmsg("��p�ڭ̤������l�F�H�c");
		continue;
	    }
	    if (strstr(addr, "��") == NULL && strstr(addr, "��") == NULL) {
		vmsg("�o�Ӧa�}�ä��X�k");
		continue;
	    }
	    break;
	}
	while (1) {
	    getfield(11, "���[-(), �]�A���~�ϸ�", "�s���q��", phone, 11);
	    if (!removespace(phone) || phone[0] != '0' ||
		strlen(phone) < 9 || phone[1] == '0') {
		vmsg("�o�ӹq�ܸ��X�ä��X�k");
		continue;
	    }
	    break;
	}
	getfield(13, "�u��J�Ʀr �p:0912345678 (�i����)",
		 "������X", mobile, 20);
	while (1) {
	    int             len;

	    getfield(15, "���/���/�褸 �p:09/27/76", "�ͤ�", birthday, 9);
	    len = strlen(birthday);
	    if (!len) {
		sprintf(birthday, "%02i/%02i/%02i",
			cuser.month, cuser.day, cuser.year % 100);
		mon = cuser.month;
		day = cuser.day;
		year = cuser.year;
	    } else if (len == 8) {
		mon = (birthday[0] - '0') * 10 + (birthday[1] - '0');
		day = (birthday[3] - '0') * 10 + (birthday[4] - '0');
		year = (birthday[6] - '0') * 10 + (birthday[7] - '0');
	    } else
		continue;
	    if (mon > 12 || mon < 1 || day > 31 || day < 1 || year > 90 ||
		year < 40)
		continue;
	    break;
	}
	getfield(17, "1.���� 2.�j�� ", "�ʧO", sex_is, 2);
	getdata(18, 0, "�H�W��ƬO�_���T(Y/N)�H(Q)�������U [N] ",
		ans, sizeof(ans), LCECHO);
	if (ans[0] == 'q')
	    return 0;
	if (ans[0] == 'y')
	    break;
    }
    strlcpy(cuser.ident, ident, sizeof(cuser.ident));
    strlcpy(cuser.realname, rname, sizeof(cuser.realname));
    strlcpy(cuser.address, addr, sizeof(cuser.address));
    strlcpy(cuser.email, email, sizeof(cuser.email));
    cuser.mobile = atoi(mobile);
    cuser.sex = (sex_is[0] - '1') % 8;
    cuser.month = mon;
    cuser.day = day;
    cuser.year = year;
    trim(career);
    trim(addr);
    trim(phone);

    toregister(email, genbuf, phone, career, ident, rname, addr, mobile);

    clear();
    move(9, 3);
    prints("�̫�Post�@�g\033[32m�ۧڤ��Ф峹\033[m���j�a�a�A"
	   "�i�D�Ҧ��Ѱ��Y\033[31m�ڨӰ�^$�C\\n\n\n\n");
    pressanykey();
    cuser.userlevel |= PERM_POST;
    brc_initial("WhoAmI");
    set_board();
    do_post();
    cuser.userlevel &= ~PERM_POST;
    return 0;
}

/* �C�X�Ҧ����U�ϥΪ� */
static int      usercounter, totalusers, showrealname;
static ushort   u_list_special;

static int
u_list_CB(userec_t * uentp)
{
    static int      i;
    char            permstr[8], *ptr;
    register int    level;

    if (uentp == NULL) {
	move(2, 0);
	clrtoeol();
	prints("\033[7m  �ϥΪ̥N��   %-25s   �W��  �峹  %s  "
	       "�̪���{���     \033[0m\n",
	       showrealname ? "�u��m�W" : "�︹�ʺ�",
	       HAS_PERM(PERM_SEEULEVELS) ? "����" : "");
	i = 3;
	return 0;
    }
    if (bad_user_id(uentp->userid))
	return 0;

    if ((uentp->userlevel & ~(u_list_special)) == 0)
	return 0;

    if (i == b_lines) {
	prints("\033[34;46m  �w��� %d/%d �H(%d%%)  \033[31;47m  "
	       "(Space)\033[30m �ݤU�@��  \033[31m(Q)\033[30m ���}  \033[m",
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
    strlcpy(permstr, "----", sizeof(permstr));
    if (level & PERM_SYSOP)
	permstr[0] = 'S';
    else if (level & PERM_ACCOUNTS)
	permstr[0] = 'A';
    else if (level & PERM_DENYPOST)
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
	   showrealname ? uentp->realname : uentp->username,
	   uentp->numlogins, uentp->numposts,
	   HAS_PERM(PERM_SEEULEVELS) ? permstr : "", ptr);
    usercounter++;
    i++;
    return 0;
}

int
u_list()
{
    char            genbuf[3];

    setutmpmode(LAUSERS);
    showrealname = u_list_special = usercounter = 0;
    totalusers = SHM->number;
    if (HAS_PERM(PERM_SEEULEVELS)) {
	getdata(b_lines - 1, 0, "�[�� [1]�S���� (2)�����H",
		genbuf, 3, DOECHO);
	if (genbuf[0] != '2')
	    u_list_special = PERM_BASIC | PERM_CHAT | PERM_PAGE | PERM_POST | PERM_LOGINOK | PERM_BM;
    }
    if (HAS_PERM(PERM_CHATROOM) || HAS_PERM(PERM_SYSOP)) {
	getdata(b_lines - 1, 0, "��� [1]�u��m�W (2)�ʺ١H",
		genbuf, 3, DOECHO);
	if (genbuf[0] != '2')
	    showrealname = 1;
    }
    u_list_CB(NULL);
    if (passwd_apply(u_list_CB) == -1) {
	outs(msg_nobody);
	return XEASY;
    }
    move(b_lines, 0);
    clrtoeol();
    prints("\033[34;46m  �w��� %d/%d ���ϥΪ�(�t�ήe�q�L�W��)  "
	   "\033[31;47m  (�Ы����N���~��)  \033[m", usercounter, totalusers);
    egetch();
    return 0;
}
