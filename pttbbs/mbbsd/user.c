/* $Id$ */
#include "bbs.h"

static char    *sex[8] = {
    MSG_BIG_BOY, MSG_BIG_GIRL, MSG_LITTLE_BOY, MSG_LITTLE_GIRL,
    MSG_MAN, MSG_WOMAN, MSG_PLANT, MSG_MIME
};
int
kill_user(int num)
{
  userec_t u;
  memset(&u, 0, sizeof(u));
  setuserid(num, "");
  passwd_index_update(num, &u);
  return 0;
}
int
u_loginview()
{
    int             i;
    unsigned int    pbits = cuser->loginview;
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

    if (pbits != cuser->loginview) {
	cuser->loginview = pbits;
	passwd_update(usernum, cuser);
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
	   "                �u��m�W: %s"
#ifdef FOREIGN_REG
	   " %s%s"
#endif
	   "\n"
	   "                �~���}: %s\n"
	   "                �q�l�H�c: %s\n"
	   "                ��    �O: %s\n"
	   "                �Ȧ�b��: %d �Ȩ�\n",
	   u->userid, u->username, u->realname,
#ifdef FOREIGN_REG
	   u->uflag2 & FOREIGN ? "(�~�y: " : "",
	   u->uflag2 & FOREIGN ?
		(u->uflag2 & LIVERIGHT) ? "�ä[�~�d)" : "�����o�~�d�v)"
		: "",
#endif
	   u->address, u->email,
	   sex[u->sex % 8], u->money);

    sethomedir(genbuf, u->userid);
    prints("                �p�H�H�c: %d ��  (�ʶR�H�c: %d ��)\n"
	   "                ������X: %010d\n"
	   "                ��    ��: %02i/%02i/%02i\n"
	   "                �p���W�r: %s\n",
	   get_num_records(genbuf, sizeof(fileheader_t)),
	   u->exmailbox, u->mobile,
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
    snprintf(genbuf, sizeof(genbuf), "home/%c/%s", crime[0], crime);
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
    snprintf(fhdr.title, sizeof(fhdr.title), "[���i] �H�k�P�M���i");
    strlcpy(fhdr.owner, "[Ptt�k�|]", sizeof(fhdr.owner));
    snprintf(genbuf, sizeof(genbuf), "home/%c/%s/.DIR", crime[0], crime);
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
	snprintf(reason, sizeof(reason), "%s", "Cross-post");
	break;
    case '2':
	snprintf(reason, sizeof(reason), "%s", "�õo�s�i�H");
	break;
    case '3':
	snprintf(reason, sizeof(reason), "%s", "�õo�s��H");
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
	snprintf(src, sizeof(src), "home/%c/%s", u->userid[0], u->userid);
	snprintf(dst, sizeof(dst), "tmp/%s", u->userid);
	Rename(src, dst);
	log_usies("KILL", u->userid);
	post_violatelaw(u->userid, cuser->userid, reason, "�尣 ID");
        kill_user(unum);

    } else {
	u->userlevel |= PERM_VIOLATELAW;
	u->vl_count++;
	passwd_update(unum, u);
	post_violatelaw(u->userid, cuser->userid, reason, "�@��B��");
	mail_violatelaw(u->userid, cuser->userid, reason, "�@��B��");
    }
    pressanykey();
}

static void Customize(void)
{
    char    ans[4], done = 0, mindbuf[5];
    char    *wm[3] = {"�@��", "�i��", "����"};

    showtitle("�ӤH�Ƴ]�w", "�ӤH�Ƴ]�w");
    memcpy(mindbuf, &currutmp->mind, 4);
    mindbuf[4] = 0;
    while( !done ){
	move(2, 0);
	prints("�z�ثe���ӤH�Ƴ]�w: ");
	move(4, 0);
	prints("%-30s%10s\n", "A. ���y�Ҧ�",
	       wm[(cuser->uflag2 & WATER_MASK)]);
	prints("%-30s%10s\n", "B. �������~�H",
	       ((cuser->userlevel & PERM_NOOUTMAIL) ? "�_" : "�O"));
	prints("%-30s%10s\n", "C. �s�O�۰ʶi�ڪ��̷R",
	       ((cuser->uflag2 & FAVNEW_FLAG) ? "�O" : "�_"));
	prints("%-30s%10s\n", "D. �ثe���߱�", mindbuf);
	prints("%-30s%10s\n", "E. ���G����ܧڪ��̷R", 
	       ((cuser->uflag2 & FAVNOHILIGHT) ? "�_" : "�O"));
	getdata(b_lines - 1, 0, "�Ы� [A-E] �����]�w�A�� [Return] �����G",
		ans, sizeof(ans), DOECHO);

	switch( ans[0] ){
	case 'A':
	case 'a':{
	    int     currentset = cuser->uflag2 & WATER_MASK;
	    currentset = (currentset + 1) % 3;
	    cuser->uflag2 &= ~WATER_MASK;
	    cuser->uflag2 |= currentset;
	    vmsg("�ץ����y�Ҧ���Х��`���u�A���s�W�u");
	}
	    break;
	case 'B':
	case 'b':
	    cuser->userlevel ^= PERM_NOOUTMAIL;
	    break;
	case 'C':
	case 'c':
	    cuser->uflag2 ^= FAVNEW_FLAG;
	    if (cuser->uflag2 & FAVNEW_FLAG)
		subscribe_newfav();
	    break;
	case 'D':
	case 'd':{
	    getdata(b_lines - 1, 0, "�{�b���߱�? ",
		    mindbuf, sizeof(mindbuf), DOECHO);
	    if (strcmp(mindbuf, "�q�r") == 0)
		vmsg("���i�H��ۤv�]�q�r��!");
	    else if (strcmp(mindbuf, "�جP") == 0)
		vmsg("�A���O���ѥͤ���!");
	    else
		memcpy(currutmp->mind, mindbuf, 4);
	}
	    break;
	case 'E':
	case 'e':
	    cuser->uflag2 ^= FAVNOHILIGHT;
	    break;
	default:
	    done = 1;
	}
	passwd_update(usernum, cuser);
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
	    "�п�� (1)�ק��� (2)�]�w�K�X (C) �ӤH�Ƴ]�w ==> [0]���� ",
	    ans, sizeof(ans), DOECHO);

    if (ans[0] > '2' && ans[0] != 'C' && ans[0] != 'c' && !real)
	ans[0] = '0';

    if (ans[0] == '1' || ans[0] == '3') {
	clear();
	i = 1;
	move(i++, 0);
	outs(msg_uid);
	outs(x.userid);
    }
    switch (ans[0]) {
    case 'C':
    case 'c':
	Customize();
	return;
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
#ifdef FOREIGN_REG
	    getdata_buf(i++, 0, cuser->uflag2 & FOREIGN ? "�@�Ӹ��X" : "�����Ҹ��G", x.ident, sizeof(x.ident), DOECHO);
#else
	    getdata_buf(i++, 0, "�����Ҹ��G", x.ident, sizeof(x.ident), DOECHO);
#endif
	    getdata_buf(i++, 0, "�~��a�}�G",
			x.address, sizeof(x.address), DOECHO);
	}
	snprintf(buf, sizeof(buf), "%010d", x.mobile);
	getdata_buf(i++, 0, "������X�G", buf, 11, LCECHO);
	x.mobile = atoi(buf);
	getdata_str(i++, 0, "�q�l�H�c[�ܰʭn���s�{��]�G", buf, 50, DOECHO,
		    x.email);
	if (strcmp(buf, x.email) && strchr(buf, '@')) {
	    strlcpy(x.email, buf, sizeof(x.email));
	    mail_changed = 1 - real;
	}
	snprintf(genbuf, sizeof(genbuf), "%i", (u->sex + 1) % 8);
	getdata_str(i++, 0, "�ʧO (1)���� (2)�j�� (3)���} (4)���� (5)���� "
		    "(6)���� (7)�Ӫ� (8)�q���G",
		    buf, 3, DOECHO, genbuf);
	if (buf[0] >= '1' && buf[0] <= '8')
	    x.sex = (buf[0] - '1') % 8;
	else
	    x.sex = u->sex % 8;

	while (1) {
	    int             len;

	    snprintf(genbuf, sizeof(genbuf), "%02i/%02i/%02i",
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
		snprintf(genbuf, sizeof(genbuf), "%d", x.money);
		if (getdata_str(i++, 0, "�Ȧ�b��G", buf, 10, DOECHO, genbuf))
		    if ((l = atol(buf)) != 0) {
			if (l != (unsigned)x.money) {
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
	    snprintf(genbuf, sizeof(genbuf),
		     "%d/%d/%d", u->chc_win, u->chc_lose, u->chc_tie);
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
#ifdef FOREIGN_REG
	    if (getdata_str(i++, 0, "���y 1)���� 2)�~��G", buf, 2, DOECHO, x.uflag2 & FOREIGN ? "2" : "1"))
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
	if ((unsigned)i == x.userlevel)
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
	    post_change_perm(temp, i, cuser->userid, x.userid);
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

	    snprintf(src, sizeof(src), "home/%c/%s", x.userid[0], x.userid);
	    snprintf(dst, sizeof(dst), "tmp/%s", x.userid);
	    Rename(src, dst);	/* do not remove user home */
	    log_usies("KILL", x.userid);
            kill_user(unum);
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
		    ctime(&now), cuser->userid, x.userid, money, x.money);

	    clrtobot();
	    clear();
	    while (!getdata(5, 0, "�п�J�z�ѥH�ܭt�d�G",
			    reason, sizeof(reason), DOECHO));

	    fprintf(fp, "\n   \033[1;37m����%s�ק���z�ѬO�G%s\033[m",
		    cuser->userid, reason);
	    fclose(fp);
	    snprintf(fhdr.title, sizeof(fhdr.title),
		     "[���w���i] ����%s�ק�%s�����i", cuser->userid,
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
    user_display(cuser, 0);
    uinfo_query(cuser, 0, usernum);
    strlcpy(currutmp->username, cuser->username, sizeof(currutmp->username));
    return 0;
}

int
u_ansi()
{
    showansi ^= 1;
    cuser->uflag ^= COLOR_FLAG;
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

    cuser->proverb = (cuser->proverb + 1) % 4;
    setuserfile(buf, fn_proverb);
    if (cuser->proverb == 2 && dashd(buf)) {
	FILE           *fp = fopen(buf, "a");
	assert(fp);

	fprintf(fp, "�y�k�ʪ��A��[�۩w��]�n�O�o�]�y�k�ʪ����e��!!");
	fclose(fp);
    }
    passwd_update(usernum, cuser);
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
showsignature(char *fname, int *j)
{
    FILE           *fp;
    char            buf[256];
    int             i, num = 0;
    char            ch;

    clear();
    move(2, 0);
    setuserfile(fname, "sig.0");
    *j = strlen(fname) - 1;

    for (ch = '1'; ch <= '9'; ch++) {
	fname[*j] = ch;
	if ((fp = fopen(fname, "r"))) {
	    prints("\033[36m�i ñ�W��.%c �j\033[m\n", ch);
	    for (i = 0; i < MAX_SIGLINES && fgets(buf, sizeof(buf), fp); i++)
		outs(buf);
	    num++;
	    fclose(fp);
	}
    }
    return num;
}

int
u_editsig()
{
    int             aborted;
    char            ans[4];
    int             j;
    char            genbuf[200];

    showsignature(genbuf, &j);

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
	setcalfile(genbuf, cuser->userid);
	aborted = vedit(genbuf, NA, NULL);
	if (aborted != -1)
	    outs("��ƾ��s����");
	pressanykey();
	return 0;
    } else if (genbuf[0] == 'd') {
	setcalfile(genbuf, cuser->userid);
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

static int
ispersonalid(char *inid)
{
    char           *lst = "ABCDEFGHJKLMNPQRSTUVXYWZIO", id[20];
    int             i, j, cksum;

    strlcpy(id, inid, sizeof(id));
    i = cksum = 0;
    if (!isalpha(id[0]) && (strlen(id) != 10))
	return 0;
    if (!(id[1] == '1' || id[1] == '2'))
	return 0;
    id[0] = toupper(id[0]);

    if( strcmp(id, "A100000001") == 0 ||
	strcmp(id, "A200000003") == 0 ||
	strcmp(id, "A123456789") == 0    )
	return 0;
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
    sprintf(buf, "%s", crypt(cuser->userid, "02"));
    return buf;
}

static int
isvalidemail(char *email)
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

    sethomefile(buf, cuser->userid, "justify.wait");
    if (phone[0] != 0) {
	fn = fopen(buf, "w");
	assert(fn);
	fprintf(fn, "%s\n%s\n%s\n%s\n%s\n%s\n",
		phone, career, ident, rname, addr, mobile);
	fclose(fn);
    }
    clear();
    stand_title("�{�ҳ]�w");
    if (cuser->userlevel & PERM_NOREGCODE){
	strcpy(email, "x");
	goto REGFORM2;
    }
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
	    if (isvalidmobile(mobile)) {
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
	else if (isvalidemail(email)) {
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
    strncpy(cuser->email, email, sizeof(cuser->email));
 REGFORM2:
    if (strcasecmp(email, "x") == 0) {	/* ��ʻ{�� */
	if ((fn = fopen(fn_register, "a"))) {
	    fprintf(fn, "num: %d, %s", usernum, ctime(&now));
	    fprintf(fn, "uid: %s\n", cuser->userid);
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
		sprintf(genbuf, sizeof(genbuf),
			"%s:%s:<Mobile>", phone, career);
	    else
#endif
		snprintf(genbuf, sizeof(genbuf),
			 "%s:%s:<Email>", phone, career);
	    strncpy(cuser->justify, genbuf, REGLEN);
	    sethomefile(buf, cuser->userid, "justify");
	}
	snprintf(buf, sizeof(buf),
		 "�z�b " BBSNAME " ���{�ҽX: %s", getregcode(genbuf));
	strlcpy(tmp, cuser->userid, sizeof(tmp));
	strlcpy(cuser->userid, "SYSOP", sizeof(cuser->userid));
#ifdef HAVEMOBILE
	if (strcmp(email, "m") == 0 || strcmp(email, "M") == 0)
	    mobile_message(mobile, buf);
	else
#endif
	    bsmtp("etc/registermail", buf, email, 0);
	strlcpy(cuser->userid, tmp, sizeof(cuser->userid));
	outs("\n\n\n�ڭ̧Y�N�H�X�{�ҫH (�z���ӷ|�b 10 ����������)\n"
	     "�����z�i�H��ڻ{�ҫH���D���{�ҽX\n"
	     "��J�� (U)ser -> (R)egister ��N�i�H�������U");
	pressanykey();
	return;
    }
}

static int HaveRejectStr(char *s, char **rej)
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
    char    *rejectstr[] =
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
}

static char *isvalidcareer(char *career)
{
    char    *rejectstr[] = {NULL};
    if (!(removespace(career) && career[0] < 0 && strlen(career) >= 6) ||
	strcmp(career, "�a��") == 0 || HaveRejectStr(career, rejectstr) )
	return "�z����J�����T";
    if (strcmp(&career[strlen(career) - 2], "�j") == 0 ||
	strcmp(&career[strlen(career) - 4], "�j��") == 0 ||
	strcmp(career, "�ǥͤj��") == 0)
	return "�·нХ[�Ǯըt��";
    if (strcmp(career, "�ǥͰ���") == 0)
	return "�·п�J�ǮզW��";
    return NULL;
}

static char *isvalidaddr(char *addr)
{
    char    *rejectstr[] =
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
	if( !isdigit(phone[i]) )
	    return "�Ф��n�[���j�Ÿ�";
    if (!removespace(phone) || phone[0] != '0' ||
	strlen(phone) < 9 || phone[1] == '0' ||
	strstr(phone, "00000000") != NULL ||
	strstr(phone, "22222222") != NULL    ) {
	return "�o�ӹq�ܸ��X�ä��X�k(�Чt�ϽX)" ;
    }
    return NULL;
}

int
u_register(void)
{
    char            rname[21], addr[51], ident[12], mobile[21];
#ifdef FOREIGN_REG
    char            fore[2];
#endif
    char            phone[21], career[41], email[51], birthday[9], sex_is[2],
                    year, mon, day;
    char            inregcode[14], regcode[50];
    char            ans[3], *ptr, *errcode;
    char            genbuf[200];
    FILE           *fn;

    if (cuser->userlevel & PERM_LOGINOK) {
	outs("�z�������T�{�w�g�����A���ݶ�g�ӽЪ�");
	return XEASY;
    }
    if ((fn = fopen(fn_register, "r"))) {
	while (fgets(genbuf, STRLEN, fn)) {
	    if ((ptr = strchr(genbuf, '\n')))
		*ptr = '\0';
	    if (strncmp(genbuf, "uid: ", 5) == 0 &&
		strcmp(genbuf + 5, cuser->userid) == 0) {
		fclose(fn);
		outs("�z�����U�ӽг�|�b�B�z���A�Э@�ߵ���");
		return XEASY;
	    }
	}
	fclose(fn);
    }
    strlcpy(ident, cuser->ident, sizeof(ident));
    strlcpy(rname, cuser->realname, sizeof(rname));
    strlcpy(addr, cuser->address, sizeof(addr));
    strlcpy(email, cuser->email, sizeof(email));
    snprintf(mobile, sizeof(mobile), "0%09d", cuser->mobile);
    if (cuser->month == 0 && cuser->day && cuser->year == 0)
	birthday[0] = 0;
    else
	snprintf(birthday, sizeof(birthday), "%02i/%02i/%02i",
		 cuser->month, cuser->day, cuser->year % 100);
    sex_is[0] = (cuser->sex % 8) + '1';
    sex_is[1] = 0;
    career[0] = phone[0] = '\0';
    sethomefile(genbuf, cuser->userid, "justify.wait");
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

    if (cuser->userlevel & PERM_NOREGCODE) {
	vmsg("�z���Q���\\�ϥλ{�ҽX�{�ҡC�ж�g���U�ӽг�");
	goto REGFORM;
    }

    if (cuser->year != 0 &&	/* �w�g�Ĥ@����L�F~ ^^" */
	strcmp(cuser->email, "x") != 0 &&	/* �W����ʻ{�ҥ��� */
	strcmp(cuser->email, "X") != 0) {
	clear();
	stand_title("EMail�{��");
	move(2, 0);
	prints("%s(%s) �z�n�A�п�J�z���{�ҽX�C\n"
	       "�αz�i�H��J x�ӭ��s��g E-Mail �Χ�ѯ�����ʻ{��\n",
	       cuser->userid, cuser->username);
	inregcode[0] = 0;
	do{
	    getdata(10, 0, "�z����J: ", inregcode, sizeof(inregcode), DOECHO);
	    if( strcmp(inregcode, "x") == 0 ||
		strcmp(inregcode, "X") == 0 ||
		strlen(inregcode) == 13 )
		break;
	    if( strlen(inregcode) != 13 )
		vmsg("�{�ҽX��J�������A���Ӥ@�@���Q�T�X�C");
	} while( 1 );

	if (strcmp(inregcode, getregcode(regcode)) == 0) {
	    int             unum;
	    if ((unum = getuser(cuser->userid)) == 0) {
		vmsg("�t�ο��~�A�d�L���H�I");
		u_exit("getuser error");
		exit(0);
	    }
	    mail_muser(*cuser, "[���U���\\�o]", "etc/registeredmail");
	    if(cuser->uflag2 & FOREIGN)
		mail_muser(*cuser, "[�X�J�Һ޲z��]", "etc/foreign_welcome");
	    cuser->userlevel |= (PERM_LOGINOK | PERM_POST);
	    prints("\n���U���\\, ���s�W����N���o�����v��\n"
		   "�Ы��U���@������᭫�s�W��~ :)");
	    sethomefile(genbuf, cuser->userid, "justify.wait");
	    unlink(genbuf);
	    pressanykey();
	    u_exit("registed");
	    exit(0);
	    return QUIT;
	} else if (strcmp(inregcode, "x") != 0 &&
		   strcmp(inregcode, "X") != 0) {
	    vmsg("�{�ҽX���~�I");
	} else {
	    toregister(email, genbuf, phone, career,
		       ident, rname, addr, mobile);
	    return FULLUPDATE;
	}
    }

    REGFORM:
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
	       cuser->userid, cuser->username);
#ifdef FOREIGN_REG
	fore[0] = 'y';
	fore[1] = 0;
	getfield(2, "Y/n", "�O�_�������y�H", fore, 2);
    	if (fore[0] == 'n')
	    fore[0] |= FOREIGN;
	else
	    fore[0] = 0;
	if (!fore[0]){
#endif
	    while( 1 ){
		getfield(3, "D123456789", "�����Ҹ�", ident, 11);
		if ('a' <= ident[0] && ident[0] <= 'z')
		    ident[0] -= 32;
		if( ispersonalid(ident) )
		    break;
		vmsg("�z����J�����T(�Y�����D�·Ц�SYSOP�O)");
	    }
#ifdef FOREIGN_REG
	}
	else{
	    int i;
	    while( 1 ){
		getfield(3, "0123456789", "�@�Ӹ��X", ident, 11);
		move(5, 2);
		prints("�`�N�G�@�Ӹ��X���~�̱N�L�k���o�i�@�B���v���I");
		getdata(6, 0, "�O�_�T�w(Y/N)", ans, sizeof(ans), LCECHO);
		if (ans[0] == 'y' || ans[0] == 'Y')
		    break;
		vmsg("�Э��s��J(�Y�����D�·Ц�SYSOP�O)");
	    }
	    for(i = 0; ans[i] != 0; i++)
		if ('a' <= ident[0] && ident[0] <= 'z')
		    ident[0] -= 32;
	    if( ispersonalid(ident) ){
		fore[0] = 0;
		vmsg("�z�������w��אּ�����y");
	    }
	}
#endif
	while (1) {
	    getfield(8, "�ХΤ���", "�u��m�W", rname, 20);
	    if( (errcode = isvalidname(rname)) == NULL )
		break;
	    else
		vmsg(errcode);
	}

	move(6, 0);
	prints("�·бz�ɶq�ԲӪ���g�z���A�ȳ��, �j�M�|�սг·�"
	       "�[\033[1;33m�t��\033[m, ���q���Х[¾��\n"
	       "�Ф��n��²�g�H�K�y���~�| :)"
	       );
	while (1) {
	    getfield(9, "�Ǯ�(�t\033[1;33m�t�Ҧ~��\033[m)�γ��¾��",
		     "�A�ȳ��", career, 40);
	    if( (errcode = isvalidcareer(career)) == NULL )
		break;
	    else
		vmsg(errcode);
	}
	while (1) {
	    getfield(11, "�t\033[1;33m����\033[m�Ϊ��츹�X"
		     "(�x�_�Х[\033[1;33m��F��\033[m)",
		     "�ثe��}", addr, 50);
	    if( (errcode = isvalidaddr(addr)) == NULL )
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
	    int             len;

	    getfield(17, "���/���/�褸 �p:09/27/76", "�ͤ�", birthday, 9);
	    len = strlen(birthday);
	    if (!len) {
		snprintf(birthday, sizeof(birthday), "%02i/%02i/%02i",
			 cuser->month, cuser->day, cuser->year % 100);
		mon = cuser->month;
		day = cuser->day;
		year = cuser->year;
	    } else if (len == 8) {
		mon = (birthday[0] - '0') * 10 + (birthday[1] - '0');
		day = (birthday[3] - '0') * 10 + (birthday[4] - '0');
		year = (birthday[6] - '0') * 10 + (birthday[7] - '0');
	    } else{
		vmsg("�z����J�����T");
		continue;
	    }
	    if (mon > 12 || mon < 1 || day > 31 || day < 1 || year > 90 ||
		year < 40){
		vmsg("�z����J�����T");
		continue;
	    }
	    break;
	}
	getfield(19, "1.���� 2.�j�� ", "�ʧO", sex_is, 2);
	getdata(20, 0, "�H�W��ƬO�_���T(Y/N)�H(Q)�������U [N] ",
		ans, sizeof(ans), LCECHO);
	if (ans[0] == 'q')
	    return 0;
	if (ans[0] == 'y')
	    break;
    }
    strlcpy(cuser->ident, ident, sizeof(cuser->ident));
    strlcpy(cuser->realname, rname, sizeof(cuser->realname));
    strlcpy(cuser->address, addr, sizeof(cuser->address));
    strlcpy(cuser->email, email, sizeof(cuser->email));
    cuser->mobile = atoi(mobile);
    cuser->sex = (sex_is[0] - '1') % 8;
    cuser->month = mon;
    cuser->day = day;
    cuser->year = year;
#ifdef FOREIGN_REG
    if (fore[0])
	cuser->uflag2 |= FOREIGN;
    else
	cuser->uflag2 &= ~FOREIGN;
#endif
    trim(career);
    trim(addr);
    trim(phone);

    toregister(email, genbuf, phone, career, ident, rname, addr, mobile);

    clear();
    move(9, 3);
    prints("�̫�Post�@�g\033[32m�ۧڤ��Ф峹\033[m���j�a�a�A"
	   "�i�D�Ҧ��Ѱ��Y\033[31m�ڨӰ�^$�C\\n\n\n\n");
    pressanykey();
    cuser->userlevel |= PERM_POST;
    brc_initial_board("WhoAmI");
    set_board();
    do_post();
    cuser->userlevel &= ~PERM_POST;
    return 0;
}

/* �C�X�Ҧ����U�ϥΪ� */
static int      usercounter, totalusers;
static ushort   u_list_special;

static int
u_list_CB(int num, userec_t * uentp)
{
    static int      i;
    char            permstr[8], *ptr;
    register int    level;

    if (uentp == NULL) {
	move(2, 0);
	clrtoeol();
	prints("\033[7m  �ϥΪ̥N��   %-25s   �W��  �峹  %s  "
	       "�̪���{���     \033[0m\n",
	       "�︹�ʺ�",
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
	   uentp->username,
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
    u_list_special = usercounter = 0;
    totalusers = SHM->number;
    if (HAS_PERM(PERM_SEEULEVELS)) {
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
    prints("\033[34;46m  �w��� %d/%d ���ϥΪ�(�t�ήe�q�L�W��)  "
	   "\033[31;47m  (�Ы����N���~��)  \033[m", usercounter, totalusers);
    egetch();
    return 0;
}
