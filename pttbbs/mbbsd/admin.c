/* $Id$ */
#include "bbs.h"

/* �i�����y�Ŷ� */
int
m_loginmsg()
{
  char msg[100];
  move(21,0);
  clrtobot();
  if(SHM->loginmsg.pid && SHM->loginmsg.pid != currutmp->pid)
    {
      prints("�ثe�w�g���H�U�� �i�����y�]�w�Х���զn�A�]�w..");
      getmessage(SHM->loginmsg);
    }
  getdata(22, 0, 
     "�i�����y:��������,���z�Z�ϥΪ̬���,�]�w�������۰ʨ���,�T�w�n�]?(y/N)",
          msg, 3, LCECHO);

  if(msg[0]=='y' &&

     getdata_str(23, 0, "�]�w�i�����y:", msg, 56, DOECHO, SHM->loginmsg.last_call_in))
    {
          SHM->loginmsg.pid=currutmp->pid; /*�������h �N����race condition */
          strcpy(SHM->loginmsg.last_call_in, msg);
          strcpy(SHM->loginmsg.userid, cuser.userid);
    }
  return 0;
}

/* �ϥΪ̺޲z */
int
m_user()
{
    userec_t        muser;
    int             id;
    char            genbuf[200];

    stand_title("�ϥΪ̳]�w");
    usercomplete(msg_uid, genbuf);
    if (*genbuf) {
	move(2, 0);
	if ((id = getuser(genbuf))) {
	    memcpy(&muser, &xuser, sizeof(muser));
	    user_display(&muser, 1);
	    uinfo_query(&muser, 1, id);
	} else {
	    outs(err_uid);
	    clrtoeol();
	    pressanykey();
	}
    }
    return 0;
}

static int
search_key_user(char *passwdfile, int mode)
{
    userec_t        user;
    int             ch;
    int             coun = 0;
    FILE            *fp1 = fopen(passwdfile, "r");
    char            friendfile[128]="", key[22], genbuf[8],
                    *keymatch;


    assert(fp1);
    clear();
    getdata(0, 0, mode ? "�п�J�ϥΪ�����r[�q��|�a�}|�m�W|������|�W���a�I|"
	    "email|�p��id] :" : "�п�Jid :", key, sizeof(key), DOECHO);
    if(!key[0])
	return 0;
    while ((fread(&user, sizeof(user), 1, fp1)) > 0 && coun < MAX_USERS) {
	if (!(++coun & 15)) {
	    move(1, 0);
	    prints("�� [%d] �����\n", coun);
	    refresh();
	}
        keymatch = NULL;
	if (!strcasecmp(user.userid, key))
             keymatch = user.userid; 
        else if(mode) {
             if(strstr(user.realname, key))
                 keymatch = user.realname; 
             else if(strstr(user.username, key))
                 keymatch = user.username; 
             else if(strstr(user.lasthost, key))
                 keymatch = user.lasthost; 
             else if(strstr(user.email, key))
                 keymatch = user.email; 
             else if(strstr(user.address, key))
                 keymatch = user.address; 
             else if(strstr(user.justify, key))
                 keymatch = user.justify; 
             else if(strstr(user.mychicken.name, key))
                 keymatch = user.mychicken.name; 
	     else if(strstr(user.ident, key))
		 keymatch = user.ident;
	}
        if(keymatch) {
	    move(1, 0);
	    prints("�� [%d] �����\n", coun);
	    refresh();

	    user_display(&user, 1);
	    uinfo_query(&user, 1, coun);
	    outs("\033[44m               �ť���\033[37m:�j�M�U�@��"
		 "          \033[33m Q\033[37m: ���}");
	    outs(mode ? 
                 "      A: add to namelist \033[m " :
		 "      S: ���γƥ����    \033[m ");
	    while (1) {
		while ((ch = igetch()) == 0);
                if (ch == 'a' || ch=='A' )
                  {
                   if(!friendfile[0])
                    {
                     friend_special();
                     setfriendfile(friendfile, FRIEND_SPECIAL);
                    }
                   friend_add(user.userid, FRIEND_SPECIAL, keymatch);
                   break;
                  }
		if (ch == ' ')
		    break;
		if (ch == 'q' || ch == 'Q') {
		    fclose(fp1);
		    return 0;
		}
		if (ch == 's' && !mode) {
		    if ((ch = searchuser(user.userid))) {
			setumoney(ch, user.money);
			passwd_update(ch, &user);
			fclose(fp1);
			return 0;
		    } else {
			move(b_lines - 1, 0);
			getdata(0, 0,
				"�ثe�� PASSWD �ɨS���� ID�A�s�W�ܡH[y/N]",
				genbuf, 3, LCECHO);
			if (genbuf[0] != 'y') {
			    outs("�ثe��PASSWDS�ɨS����id "
				 "�Х�new�@�ӳo��id���b��");
			} else {
			    int             allocid = getnewuserid();

			    if (allocid > MAX_USERS || allocid <= 0) {
				fprintf(stderr, "�����H�f�w�F���M�I\n");
				exit(1);
			    }
			    if (passwd_update(allocid, &user) == -1) {
				fprintf(stderr, "�Ⱥ��F�A�A���I\n");
				exit(1);
			    }
			    setuserid(allocid, user.userid);
			    if (!searchuser(user.userid)) {
				fprintf(stderr, "�L�k�إ߱b��\n");
				exit(1);
			    }
			    fclose(fp1);
			    return 0;
			}
		    }
		}
	    }
	}
    }

    fclose(fp1);
    return 0;
}

/* �H���N key �M��ϥΪ� */
int
search_user_bypwd()
{
    search_key_user(FN_PASSWD, 1);
    return 0;
}

/* �M��ƥ����ϥΪ̸�� */
int
search_user_bybakpwd()
{
    char           *choice[] = {
	"PASSWDS.NEW1", "PASSWDS.NEW2", "PASSWDS.NEW3",
	"PASSWDS.NEW4", "PASSWDS.NEW5", "PASSWDS.NEW6",
	"PASSWDS.BAK"
    };
    int             ch;

    clear();
    move(1, 1);
    outs("�п�J�A�n�ΨӴM��ƥ����ɮ� �Ϋ� 'q' ���}\n");
    outs(" [\033[1;31m1\033[m]�@�ѫe, [\033[1;31m2\033[m]��ѫe, "
	 "[\033[1;31m3\033[m]�T�ѫe\n");
    outs(" [\033[1;31m4\033[m]�|�ѫe, [\033[1;31m5\033[m]���ѫe, "
	 "[\033[1;31m6\033[m]���ѫe\n");
    outs(" [7]�ƥ���\n");
    do {
	move(5, 1);
	outs("��� => ");
	ch = igetch();
	if (ch == 'q' || ch == 'Q')
	    return 0;
    } while (ch < '1' || ch > '8');
    ch -= '1';
    search_key_user(choice[ch], 0);
    return 0;
}

static void
bperm_msg(boardheader_t * board)
{
    prints("\n�]�w [%s] �ݪO��(%s)�v���G", board->brdname,
	   board->brdattr & BRD_POSTMASK ? "�o��" : "�\\Ū");
}

unsigned int
setperms(unsigned int pbits, char *pstring[])
{
    register int    i;
    char            choice[4];

    move(4, 0);
    for (i = 0; i < NUMPERMS / 2; i++) {
	prints("%c. %-20s %-15s %c. %-20s %s\n",
	       'A' + i, pstring[i],
	       ((pbits >> i) & 1 ? "��" : "��"),
	       i < 10 ? 'Q' + i : '0' + i - 10,
	       pstring[i + 16],
	       ((pbits >> (i + 16)) & 1 ? "��" : "��"));
    }
    clrtobot();
    while (getdata(b_lines - 1, 0, "�Ы� [A-5] �����]�w�A�� [Return] �����G",
		   choice, sizeof(choice), LCECHO)) {
	i = choice[0] - 'a';
	if (i < 0)
	    i = choice[0] - '0' + 26;
	if (i >= NUMPERMS)
	    bell();
	else {
	    pbits ^= (1 << i);
	    move(i % 16 + 4, i <= 15 ? 24 : 64);
	    prints((pbits >> i) & 1 ? "��" : "��");
	}
    }
    return pbits;
}

/* �۰ʳ]�ߺ�ذ� */
void
setup_man(boardheader_t * board)
{
    char            genbuf[200];

    setapath(genbuf, board->brdname);
    mkdir(genbuf, 0755);
}

void delete_symbolic_link(boardheader_t *bh, int bid)
{
    memset(bh, 0, sizeof(boardheader_t));
    substitute_record(fn_board, bh, sizeof(boardheader_t), bid);
    reset_board(bid);
    log_usies("DelLink", bh->brdname);
}

int
m_mod_board(char *bname)
{
    boardheader_t   bh, newbh;
    int             bid;
    char            genbuf[256], ans[4];

    bid = getbnum(bname);
    if (!bid || !bname[0] || get_record(fn_board, &bh, sizeof(bh), bid) == -1) {
	outs(err_bid);
	pressanykey();
	return -1;
    }
    prints("�ݪO�W�١G%s\n�ݪO�����G%s\n�ݪObid�G%d\n�ݪOGID�G%d\n"
	   "�O�D�W��G%s", bh.brdname, bh.title, bid, bh.gid, bh.BM);
    bperm_msg(&bh);

    /* Ptt �o���_��|�ɨ�U�� */
    move(9, 0);
    snprintf(genbuf, sizeof(genbuf), "(E)�]�w (V)�H�k/�Ѱ�%s%s [Q]�����H",
	    HAS_PERM(PERM_SYSOP) ? " (B)BVote (S)�Ϧ^�峹 (G)��L�ѥd" : "",
	    HAS_PERM(PERM_SYSSUBOP) ? " (D)�R��" : "");
    getdata(10, 0, genbuf, ans, sizeof(ans), LCECHO);

    switch (*ans) {
    case 'g':
	if (HAS_PERM(PERM_SYSOP)) {
	    char            path[256];
	    setbfile(genbuf, bname, FN_TICKET_LOCK);
	    setbfile(path, bname, FN_TICKET_END);
	    rename(genbuf, path);
	}
	break;
    case 's':
	if (HAS_PERM(PERM_SYSOP)) {
	  snprintf(genbuf, sizeof(genbuf),
		   BBSHOME "/bin/buildir boards/%c/%s &",
		   bh.brdname[0], bh.brdname);
	    system(genbuf);
	}
	break;
    case 'b':
	if (HAS_PERM(PERM_SYSOP)) {
	    char            bvotebuf[10];

	    memcpy(&newbh, &bh, sizeof(bh));
	    snprintf(bvotebuf, sizeof(bvotebuf), "%d", newbh.bvote);
	    move(20, 0);
	    prints("�ݪO %s ��Ӫ� BVote�G%d", bh.brdname, bh.bvote);
	    getdata_str(21, 0, "�s�� Bvote�G", genbuf, 5, LCECHO, bvotebuf);
	    newbh.bvote = atoi(genbuf);
	    substitute_record(fn_board, &newbh, sizeof(newbh), bid);
	    reset_board(bid);
	    log_usies("SetBoardBvote", newbh.brdname);
	    break;
	} else
	    break;
    case 'v':
	memcpy(&newbh, &bh, sizeof(bh));
	outs("�ݪO�ثe��");
	outs((bh.brdattr & BRD_BAD) ? "�H�k" : "���`");
	getdata(21, 0, "�T�w���H", genbuf, 5, LCECHO);
	if (genbuf[0] == 'y') {
	    if (newbh.brdattr & BRD_BAD)
		newbh.brdattr = newbh.brdattr & (!BRD_BAD);
	    else
		newbh.brdattr = newbh.brdattr | BRD_BAD;
	    substitute_record(fn_board, &newbh, sizeof(newbh), bid);
	    reset_board(bid);
	    log_usies("ViolateLawSet", newbh.brdname);
	}
	break;
    case 'd':
	if (HAS_PERM(PERM_SYSSUBOP) && !HAS_PERM(PERM_SYSOP))
	    break;
	getdata_str(9, 0, msg_sure_ny, genbuf, 3, LCECHO, "N");
	if (genbuf[0] != 'y' || !bname[0])
	    outs(MSG_DEL_CANCEL);
	else if (bh.brdattr & BRD_SYMBOLIC) {
	    delete_symbolic_link(&bh, bid);
	}
	else {
	    strlcpy(bname, bh.brdname, sizeof(bh.brdname));
	    snprintf(genbuf, sizeof(genbuf),
		    "/bin/tar zcvf tmp/board_%s.tgz boards/%c/%s man/boards/%c/%s >/dev/null 2>&1;"
		    "/bin/rm -fr boards/%c/%s man/boards/%c/%s",
		    bname, bname[0], bname, bname[0],
		    bname, bname[0], bname, bname[0], bname);
	    system(genbuf);
	    memset(&bh, 0, sizeof(bh));
	    snprintf(bh.title, sizeof(bh.title),
		     "%s �ݪO %s �R��", bname, cuser.userid);
	    post_msg("Security", bh.title, "�Ъ`�N�R�����X�k��", "[�t�Φw����]");
	    substitute_record(fn_board, &bh, sizeof(bh), bid);
	    reset_board(bid);
	    log_usies("DelBoard", bh.title);
	    outs("�R�O����");
	}
	break;
    case 'e':
	move(8, 0);
	outs("������ [Return] ���ק�Ӷ��]�w");
	memcpy(&newbh, &bh, sizeof(bh));

	while (getdata(9, 0, "�s�ݪO�W�١G", genbuf, IDLEN + 1, DOECHO)) {
	    if (getbnum(genbuf)) {
		move(3, 0);
		outs("���~! �O�W�p�P");
	    } else if ( !invalid_brdname(genbuf) ){
		strlcpy(newbh.brdname, genbuf, sizeof(newbh.brdname));
		break;
	    }
	}

	do {
	    getdata_str(12, 0, "�ݪO���O�G", genbuf, 5, DOECHO, bh.title);
	    if (strlen(genbuf) == 4)
		break;
	} while (1);

	if (strlen(genbuf) >= 4)
	    strncpy(newbh.title, genbuf, 4);

	newbh.title[4] = ' ';

	getdata_str(14, 0, "�ݪO�D�D�G", genbuf, BTLEN + 1, DOECHO,
		    bh.title + 7);
	if (genbuf[0])
	    strlcpy(newbh.title + 7, genbuf, sizeof(newbh.title) - 7);
	if (getdata_str(15, 0, "�s�O�D�W��G", genbuf, IDLEN * 3 + 3, DOECHO,
			bh.BM)) {
	    trim(genbuf);
	    strlcpy(newbh.BM, genbuf, sizeof(newbh.BM));
	}
	if (HAS_PERM(PERM_SYSOP)) {
	    move(1, 0);
	    clrtobot();
	    newbh.brdattr = setperms(newbh.brdattr, str_permboard);
	    move(1, 0);
	    clrtobot();
	}
	if (newbh.brdattr & BRD_GROUPBOARD)
	    strncpy(newbh.title + 5, "�U", 2);
	else if (newbh.brdattr & BRD_NOTRAN)
	    strncpy(newbh.title + 5, "��", 2);
	else
	    strncpy(newbh.title + 5, "��", 2);

	if (HAS_PERM(PERM_SYSOP) && !(newbh.brdattr & BRD_HIDE)) {
	    getdata_str(14, 0, "�]�wŪ�g�v��(Y/N)�H", ans, sizeof(ans), LCECHO, "N");
	    if (*ans == 'y') {
		getdata_str(15, 0, "���� [R]�\\Ū (P)�o��H", ans, sizeof(ans), LCECHO,
			    "R");
		if (*ans == 'p')
		    newbh.brdattr |= BRD_POSTMASK;
		else
		    newbh.brdattr &= ~BRD_POSTMASK;

		move(1, 0);
		clrtobot();
		bperm_msg(&newbh);
		newbh.level = setperms(newbh.level, str_permid);
		clear();
	    }
	}
	getdata(b_lines - 1, 0, "�бz�T�w(Y/N)�H[Y]", genbuf, 4, LCECHO);

	if ((*genbuf != 'n') && memcmp(&newbh, &bh, sizeof(bh))) {
	    if (strcmp(bh.brdname, newbh.brdname)) {
		char            src[60], tar[60];

		setbpath(src, bh.brdname);
		setbpath(tar, newbh.brdname);
		Rename(src, tar);

		setapath(src, bh.brdname);
		setapath(tar, newbh.brdname);
		Rename(src, tar);
	    }
	    setup_man(&newbh);
	    substitute_record(fn_board, &newbh, sizeof(newbh), bid);
	    reset_board(bid);
	    log_usies("SetBoard", newbh.brdname);
	}
    }
    return 0;
}

/* �]�w�ݪO */
int
m_board()
{
    char            bname[32];

    stand_title("�ݪO�]�w");
    generalnamecomplete(msg_bid, bname, sizeof(bname), SHM->Bnumber,
			completeboard_compar,
			completeboard_permission,
			completeboard_getname);
    if (!*bname)
	return 0;
    m_mod_board(bname);
    return 0;
}

/* �]�w�t���ɮ� */
int
x_file()
{
    int             aborted;
    char            ans[4], *fpath;

    move(b_lines - 6, 0);
    /* Ptt */
    outs("�]�w (1)�����T�{�H (4)post�`�N�ƶ� (5)���~�n�J�T�� (6)���U�d�� (7)�q�L�T�{�q��\n");
    outs("     (8)email post�q�� (9)�t�Υ\\����F (A)���� (B)�����W�� (C)email�q�L�T�{\n");
    outs("     (D)�s�ϥΪ̻ݪ� (E)�����T�{��k (F)�w��e�� (G)�i���e��"
#ifdef MULTI_WELCOME_LOGIN
	 "(X)�R���i���e��"
#endif
	 "\n");
    outs("     (H)�ݪO���� (I)�G�m (J)�X���e�� (K)�ͤ�d (L)�`�� (M)�~�y�ϥΪ̻{�ҳq��\n");
    outs("     (N)�~�y�ϥΪ̹L��ĵ�i�q��\n");
    getdata(b_lines - 1, 0, "[Q]����[1-9 A-N]�H", ans, sizeof(ans), LCECHO);

    switch (ans[0]) {
    case '1':
	fpath = "etc/confirm";
	break;
    case '4':
	fpath = "etc/post.note";
	break;
    case '5':
	fpath = "etc/goodbye";
	break;
    case '6':
	fpath = "etc/register";
	break;
    case '7':
	fpath = "etc/registered";
	break;
    case '8':
	fpath = "etc/emailpost";
	break;
    case '9':
	fpath = "etc/hint";
	break;
    case 'b':
	fpath = "etc/sysop";
	break;
    case 'c':
	fpath = "etc/bademail";
	break;
    case 'd':
	fpath = "etc/newuser";
	break;
    case 'e':
	fpath = "etc/justify";
	break;
    case 'f':
	fpath = "etc/Welcome";
	break;
    case 'g':
#ifdef MULTI_WELCOME_LOGIN
	getdata(b_lines - 1, 0, "�ĴX�Ӷi���e��[0-4]", ans, sizeof(ans), LCECHO);
	if (ans[0] == '1') {
	    fpath = "etc/Welcome_login.1";
	} else if (ans[0] == '2') {
	    fpath = "etc/Welcome_login.2";
	} else if (ans[0] == '3') {
	    fpath = "etc/Welcome_login.3";
	} else if (ans[0] == '4') {
	    fpath = "etc/Welcome_login.4";
	} else {
	    fpath = "etc/Welcome_login.0";
	}
#else
	fpath = "etc/Welcome_login";
#endif
	break;

#ifdef MULTI_WELCOME_LOGIN
    case 'x':
	getdata(b_lines - 1, 0, "�ĴX�Ӷi���e��[1-4]", ans, sizeof(ans), LCECHO);
	if (ans[0] == '1') {
	    unlink("etc/Welcome_login.1");
	    outs("ok");
	} else if (ans[0] == '2') {
	    unlink("etc/Welcome_login.2");
	    outs("ok");
	} else if (ans[0] == '3') {
	    unlink("etc/Welcome_login.3");
	    outs("ok");
	} else if (ans[0] == '4') {
	    unlink("etc/Welcome_login.4");
	    outs("ok");
	} else {
	    outs("�ҫ��w���i���e���L�k�R��");
	}
	pressanykey();
	return FULLUPDATE;

#endif

    case 'h':
	fpath = "etc/expire.conf";
	break;
    case 'i':
	fpath = "etc/domain_name_query";
	break;
    case 'j':
	fpath = "etc/Logout";
	break;
    case 'k':
	fpath = "etc/Welcome_birth";
	break;
    case 'l':
	fpath = "etc/feast";
	break;
    case 'm':
	fpath = "etc/foreign_welcome";
	break;
    case 'n':
	fpath = "etc/foreign_expired_warn";
	break;
    default:
	return FULLUPDATE;
    }
    aborted = vedit(fpath, NA, NULL);
    prints("\n\n�t���ɮ�[%s]�G%s", fpath,
	   (aborted == -1) ? "������" : "��s����");
    pressanykey();
    return FULLUPDATE;
}

static int add_board_record(boardheader_t *board)
{
    int bid;
    if ((bid = getbnum("")) > 0) {
	substitute_record(fn_board, board, sizeof(boardheader_t), bid);
	reset_board(bid);
    } else if (append_record(fn_board, (fileheader_t *)board, sizeof(boardheader_t)) == -1) {
	return -1;
    } else {
	addbrd_touchcache();
    }
    return 0;
}

int
m_newbrd(int recover)
{
    boardheader_t   newboard;
    char            ans[4];
    char            genbuf[200];

    stand_title("�إ߷s�O");
    memset(&newboard, 0, sizeof(newboard));

    newboard.gid = class_bid;
    if (newboard.gid == 0) {
	move(6, 0);
	outs("�Х���ܤ@�����O�A�}�O!");
	pressanykey();
	return -1;
    }
    do {
	if (!getdata(3, 0, msg_bid, newboard.brdname,
		     sizeof(newboard.brdname), DOECHO))
	    return -1;
    } while (invalid_brdname(newboard.brdname));

    do {
	getdata(6, 0, "�ݪO���O�G", genbuf, 5, DOECHO);
	if (strlen(genbuf) == 4)
	    break;
    } while (1);

    strncpy(newboard.title, genbuf, 4);

    newboard.title[4] = ' ';

    getdata(8, 0, "�ݪO�D�D�G", genbuf, BTLEN + 1, DOECHO);
    if (genbuf[0])
	strlcpy(newboard.title + 7, genbuf, sizeof(newboard.title) - 7);
    setbpath(genbuf, newboard.brdname);

    if (recover) {
	if (dashd(genbuf)) {
	    outs("���ݪO�w�g�s�b! �Ш����P�^��O�W");
	    pressanykey();
	    return -1;
	}
    } else if (getbnum(newboard.brdname) > 0 || mkdir(genbuf, 0755) == -1) {
	outs("���ݪO�w�g�s�b! �Ш����P�^��O�W");
	pressanykey();
	return -1;
    }
    newboard.brdattr = BRD_NOTRAN;

    if (HAS_PERM(PERM_SYSOP)) {
	move(1, 0);
	clrtobot();
	newboard.brdattr = setperms(newboard.brdattr, str_permboard);
	move(1, 0);
	clrtobot();
    }
    getdata(9, 0, "�O�ݪO? (N:�ؿ�) (Y/n)�G", genbuf, 3, LCECHO);
    if (genbuf[0] == 'n')
	newboard.brdattr |= BRD_GROUPBOARD;

    if (newboard.brdattr & BRD_GROUPBOARD)
	strncpy(newboard.title + 5, "�U", 2);
    else if (newboard.brdattr & BRD_NOTRAN)
	strncpy(newboard.title + 5, "��", 2);
    else
	strncpy(newboard.title + 5, "��", 2);

    newboard.level = 0;
    getdata(11, 0, "�O�D�W��G", newboard.BM, sizeof(newboard.BM), DOECHO);

    if (HAS_PERM(PERM_SYSOP) && !(newboard.brdattr & BRD_HIDE)) {
	getdata_str(14, 0, "�]�wŪ�g�v��(Y/N)�H", ans, sizeof(ans), LCECHO, "N");
	if (*ans == 'y') {
	    getdata_str(15, 0, "���� [R]�\\Ū (P)�o��H", ans, sizeof(ans), LCECHO, "R");
	    if (*ans == 'p')
		newboard.brdattr |= BRD_POSTMASK;
	    else
		newboard.brdattr &= (~BRD_POSTMASK);

	    move(1, 0);
	    clrtobot();
	    bperm_msg(&newboard);
	    newboard.level = setperms(newboard.level, str_permid);
	    clear();
	}
    }

    add_board_record(&newboard);
    pressanykey();
    setup_man(&newboard);

    outs("\n�s�O����");
    post_newboard(newboard.title, newboard.brdname, newboard.BM);
    log_usies("NewBoard", newboard.title);
    pressanykey();
    return 0;
}

int make_symbolic_link(char *bname, int gid)
{
    boardheader_t   newboard;
    int bid;
    
    bid = getbnum(bname);
    memset(&newboard, 0, sizeof(newboard));

    /*
     * known issue:
     *   These two stuff will be used for sorting.  But duplicated brdnames
     *   may cause wrong binary-search result.  So I replace the last 
     *   letters of brdname to '~'(ascii code 126) in order to correct the
     *   resuilt, thought I think it's a dirty solution.
     *
     *   Duplicate entry with same brdname may cause wrong result, if
     *   searching by key brdname.  But people don't need to know a board
     *   is symbolic, so just let SYSOP know it. You may want to read
     *   board.c:load_boards().
     */

    strlcpy(newboard.brdname, bname, sizeof(newboard.brdname));
    newboard.brdname[strlen(bname) - 1] = '~';
    strlcpy(newboard.title, bcache[bid - 1].title, sizeof(newboard.title));
    strcpy(newboard.title + 5, "�I�ݪO�s��");

    newboard.gid = gid;
    BRD_LINK_TARGET(&newboard) = bid;
    newboard.brdattr = BRD_NOTRAN | BRD_SYMBOLIC;

    if (add_board_record(&newboard) < 0)
	return -1;
    return bid;
}

int make_symbolic_link_interactively(int gid)
{
    char buf[32];

    generalnamecomplete(msg_bid, buf, sizeof(buf), SHM->Bnumber,
			completeboard_compar,
			completeboard_permission,
			completeboard_getname);
    if (!buf[0])
	return -1;

    stand_title("�إ߬ݪO�s��");

    if (make_symbolic_link(buf, gid) < 0) {
	vmsg("�ݪO�s���إߥ���");
	return -1;
    }
    log_usies("NewSymbolic", buf);
    return 0;
}

static int
auto_scan(char fdata[][STRLEN], char ans[])
{
    int             good = 0;
    int             count = 0;
    int             i;
    char            temp[10];

    if (!strncmp(fdata[2], "�p", 2) || strstr(fdata[2], "�X")
	|| strstr(fdata[2], "��") || strstr(fdata[2], "��")) {
	ans[0] = '0';
	return 1;
    }
    strncpy(temp, fdata[2], 2);
    temp[2] = '\0';

    /* �|�r */
    if (!strncmp(temp, &(fdata[2][2]), 2)) {
	ans[0] = '0';
	return 1;
    }
    if (strlen(fdata[2]) >= 6) {
	if (strstr(fdata[2], "������")) {
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

    if (!strcmp(fdata[3], fdata[4]) ||
	!strcmp(fdata[3], fdata[5]) ||
	!strcmp(fdata[4], fdata[5])) {
	ans[0] = '4';
	return 5;
    }
    if (strstr(fdata[3], "�j")) {
	if (strstr(fdata[3], "�x") || strstr(fdata[3], "�H") ||
	    strstr(fdata[3], "��") || strstr(fdata[3], "�F") ||
	    strstr(fdata[3], "�M") || strstr(fdata[3], "ĵ") ||
	    strstr(fdata[3], "�v") || strstr(fdata[3], "�ʶ�") ||
	    strstr(fdata[3], "����") || strstr(fdata[3], "��") ||
	    strstr(fdata[3], "��") || strstr(fdata[3], "�F�d"))
	    good++;
    } else if (strstr(fdata[3], "�k��"))
	good++;

    if (strstr(fdata[4], "�a�y") || strstr(fdata[4], "�t�z") ||
	strstr(fdata[4], "�H�c")) {
	ans[0] = '2';
	return 3;
    }
    if (strstr(fdata[4], "��") || strstr(fdata[4], "��")) {
	if (strstr(fdata[4], "��") || strstr(fdata[4], "��")) {
	    if (strstr(fdata[4], "��"))
		good++;
	}
    }
    for (i = 0; fdata[5][i]; i++) {
	if (isdigit(fdata[5][i]))
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

/* �B�z Register Form */
int
scan_register_form(char *regfile, int automode, int neednum)
{
    char            genbuf[200];
    char    *logfile = "register.log";
    char    *field[] = {
	"uid", "ident", "name", "career", "addr", "phone", "email", NULL
    };
    char    *finfo[] = {
	"�b��", "�����Ҹ�", "�u��m�W", "�A�ȳ��", "�ثe��}",
	"�s���q��", "�q�l�l��H�c", NULL
    };
    char    *reason[] = {
	"��J�u��m�W",
	"�ԲӶ�g�z���u�ǮեH�Ρy��t�z�y�~�šz�v�Ρu�A�ȳ��(�ԲӦ�¾��)�v",
	"��g���㪺��}��� (�t�����W��, �x�_���Чt��F�ϰ�^",
	"�Զ�s���q�� (�t�ϰ�X, �������Υ[ \"-\", \"(\", \")\"���Ÿ�",
	"�T���g���U�ӽЪ�",
	"�Τ����g�ӽг�",
	"��J�u�ꨭ���Ҧr��",
	NULL
    };
    char    *autoid = "AutoScan";
    userec_t        muser;
    FILE           *fn, *fout, *freg;
    char            fdata[7][STRLEN];
    char            fname[STRLEN], buf[STRLEN];
    char            ans[4], *ptr, *uid;
    int             n = 0, unum = 0;
    int             nSelf = 0, nAuto = 0;

    uid = cuser.userid;
    snprintf(fname, sizeof(fname), "%s.tmp", regfile);
    move(2, 0);
    if (dashf(fname)) {
	if (neednum == 0) {	/* �ۤv�i Admin �Ӽf�� */
	    outs("��L SYSOP �]�b�f�ֵ��U�ӽг�");
	    pressanykey();
	}
	return -1;
    }
    Rename(regfile, fname);
    if ((fn = fopen(fname, "r")) == NULL) {
	prints("�t�ο��~�A�L�kŪ�����U�����: %s", fname);
	pressanykey();
	return -1;
    }
    if (neednum) {		/* �Q�j���f�� */
	move(1, 0);
	clrtobot();
	prints("�U��㦳�����v�����H�A���U��ֿn�W�L�@�ʥ��F�A�·бz�����f %d ��\n", neednum);
	prints("�]�N�O�j���G�Q�����@���ƶq�A��M�A�z�]�i�H�h�f\n�S�f�����e�A�t�Τ��|���A���X��I����");
	pressanykey();
    }
    memset(fdata, 0, sizeof(fdata));
    while (fgets(genbuf, STRLEN, fn)) {
	if ((ptr = (char *)strstr(genbuf, ": "))) {
	    *ptr = '\0';
	    for (n = 0; field[n]; n++) {
		if (strcmp(genbuf, field[n]) == 0) {
		    strlcpy(fdata[n], ptr + 2, sizeof(fdata[n]));
		    if ((ptr = (char *)strchr(fdata[n], '\n')))
			*ptr = '\0';
		}
	    }
	} else if ((unum = getuser(fdata[0])) == 0) {
	    move(2, 0);
	    clrtobot();
	    outs("�t�ο��~�A�d�L���H\n\n");
	    for (n = 0; field[n]; n++)
		prints("%s     : %s\n", finfo[n], fdata[n]);
	    pressanykey();
	    neednum--;
	} else {
	    neednum--;
	    memcpy(&muser, &xuser, sizeof(muser));
	    if (automode)
		uid = autoid;

	    if ((!automode || !auto_scan(fdata, ans))) {
		uid = cuser.userid;

		move(1, 0);
		prints("�b����m    �G%d\n", unum);
		user_display(&muser, 1);
		move(14, 0);
		prints("\033[1;32m------------- �Я����Y��f�֨ϥΪ̸�ơA�z�٦� %d ��---------------\033[m\n", neednum);
	    	prints("  %-12s�G%s\n", finfo[0], fdata[0]);
		prints("  %-12s�G%s\n", finfo[1], fdata[1]);
#ifdef FOREIGN_REG
		prints("0.%-12s�G%s%s\n", finfo[2], fdata[2], muser.uflag2 & FOREIGN ? " (�~�y)" : "");
#else
		prints("0.%-12s�G%s\n", finfo[2], fdata[2]);
#endif
		for (n = 3; field[n]; n++) {
		    prints("%d.%-12s�G%s\n", n - 2, finfo[n], fdata[n]);
		}
		if (muser.userlevel & PERM_LOGINOK) {
		    getdata(b_lines - 1, 0, "\033[1;32m���b���w�g�������U, "
		    "��s(Y/N/Skip)�H\033[m[N] ", ans, sizeof(ans), LCECHO);
		    if (ans[0] != 'y' && ans[0] != 's')
			ans[0] = 'd';
		} else {
		    move(b_lines - 1, 0);
		    prints("�O�_���������(Y/N/Q/Del/Skip)�H[S] ");
		    if (search_ulist(unum) == NULL)
			ans[0] = igetch();
		    else
			ans[0] = 's';
		    if ('A' <= ans[0] && ans[0] <= 'Z')
			ans[0] += 32;
		    if (ans[0] != 'y' && ans[0] != 'n' && ans[0] != 'q' &&
			ans[0] != 'd' && !('0' <= ans[0] && ans[0] <= '4'))
			ans[0] = 's';
		    ans[1] = 0;
		}
		nSelf++;
	    } else
		nAuto++;
	    if (neednum > 0 && ans[0] == 'q') {
		move(2, 0);
		clrtobot();
		prints("�S�f������h�X");
		pressanykey();
		ans[0] = 's';
	    }
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
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case 'n':
		if (ans[0] == 'n') {
		    for (n = 0; field[n]; n++)
			prints("%s: %s\n", finfo[n], fdata[n]);
		    move(9, 0);
		    prints("�д��X�h�^�ӽЪ��]�A�� <enter> ����\n");
		    for (n = 0; reason[n]; n++)
			prints("%d) ��%s\n", n, reason[n]);
		} else
		    buf[0] = ans[0];
		if (ans[0] != 'n' ||
		    getdata(10 + n, 0, "�h�^��]�G", buf, 60, DOECHO))
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
			    fp = fopen(buf1, "w");
			    
			    for(i = 0; buf[i] && i < sizeof(buf); i++){
				if (!isdigit(buf[i]))
				    continue;
				snprintf(genbuf, sizeof(genbuf),
				    "[�h�^��]] ��%s", reason[buf[i] - '0']);
				fprintf(fp, "%s\n", genbuf);
			    }

			    fclose(fp);
			}
			if ((fout = fopen(logfile, "a"))) {
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
		prints("�����h�^�����U�ӽЪ�");
	    case 's':
		if ((freg = fopen(regfile, "a"))) {
		    for (n = 0; field[n]; n++)
			fprintf(freg, "%s: %s\n", field[n], fdata[n]);
		    fprintf(freg, "----\n");
		    fclose(freg);
		}
		break;
	    default:
		prints("�H�U�ϥΪ̸�Ƥw�g��s:\n");
		mail_muser(muser, "[���U���\\�o]", "etc/registered");
		if(muser.uflag2 & FOREIGN)
		    mail_muser(muser, "[�X�J�Һ޲z��]", "etc/foreign_welcome");
		muser.userlevel |= (PERM_LOGINOK | PERM_POST);
		strlcpy(muser.realname, fdata[2], sizeof(muser.realname));
		strlcpy(muser.address, fdata[4], sizeof(muser.address));
		strlcpy(muser.email, fdata[6], sizeof(muser.email));
		snprintf(genbuf, sizeof(genbuf),
			 "%s:%s:%s", fdata[5], fdata[3], uid);
		strncpy(muser.justify, genbuf, REGLEN);
		sethomefile(buf, muser.userid, "justify");
		strncat(genbuf, "\n", sizeof(genbuf));
		log_file(buf, genbuf, 1);
		passwd_update(unum, &muser);

		if ((fout = fopen(logfile, "a"))) {
		    for (n = 0; field[n]; n++)
			fprintf(fout, "%s: %s\n", field[n], fdata[n]);
		    fprintf(fout, "Date: %s\n", Cdate(&now));
		    fprintf(fout, "Approved: %s\n", uid);
		    fprintf(fout, "----\n");
		    fclose(fout);
		}
		sethomefile(genbuf, muser.userid, "justify.wait");
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

    /**	DickG: �N�f�F�X����������� post �� Security �O�W	***********/
    /*
     * DickG: �]���s�������W���ݼf�֤�סA�O�G�S�����n�d�U record ime(buf,
     * 200, "%Y/%m/%d/%H:%M", pt);
     * 
     * strcpy(xboard, "Security"); setbpath(xfpath, xboard); stampfile(xfpath,
     * &xfile); strcpy(xfile.owner, "�t��"); strcpy(xfile.title, "[���i]
     * �f�ְO��"); xptr = fopen(xfpath, "w"); fprintf(xptr, "\n�ɶ��G%s %s
     * �f�F %d �����U��\n AutoScan �f�F %d �����U��\n �@�p %d ���C", buf,
     * cuser.userid, nSelf, nAuto, nSelf+nAuto); fclose(xptr); setbdir(fname,
     * xboard); append_record(fname, &xfile, sizeof(xfile));
     * outgo_post(&xfile, xboard); touchbtotal(getbnum(xboard));
     * cuser.numposts++;
     */
    /*********************************************/
    pressanykey();
    return (0);
}

int
m_register()
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
    getdata(b_lines - 1, 0, "�}�l�f�ֶ�(Auto/Yes/No)�H[N] ", ans, sizeof(ans), LCECHO);
    if (ans[0] == 'a')
	scan_register_form(fn_register, 1, 0);
    else if (ans[0] == 'y')
	scan_register_form(fn_register, 0, 0);

    return 0;
}

int
cat_register()
{
    if (system("cat register.new.tmp >> register.new") == 0 &&
	system("rm -f register.new.tmp") == 0)
	mprints(22, 0, "OK �P~~ �~��h�İ��a!!");
    else
	mprints(22, 0, "�S��kCAT�L�h�O �h�ˬd�@�U�t�Χa!!");
    pressanykey();
    return 0;
}

static void
give_id_money(char *user_id, int money, FILE * log_fp, char *mail_title, time_t t)
{
    char            tt[TTLEN + 1] = {0};

    if (deumoney(searchuser(user_id), money) < 0) {
	move(12, 0);
	clrtoeol();
	prints("id:%s money:%d ����a!!", user_id, money);
	pressanykey();
    } else {
	fprintf(log_fp, "%d %s %d", (int)t, user_id, money);
	snprintf(tt, sizeof(tt), "%s : %d ptt ��", mail_title, money);
	mail_id(user_id, tt, "etc/givemoney.why", "[PTT �Ȧ�]");
    }
}

int
give_money()
{
    FILE           *fp, *fp2;
    char           *ptr, *id, *mn;
    char            buf[200] = "", tt[TTLEN + 1] = "";
    struct tm      *pt = localtime(&now);
    int             to_all = 0, money = 0;

    getdata(0, 0, "���w�ϥΪ�(S) �����ϥΪ�(A) ����(Q)�H[S]", buf, sizeof(buf), LCECHO);
    if (buf[0] == 'q')
	return 1;
    else if (buf[0] == 'a') {
	to_all = 1;
	getdata(1, 0, "�o�h�ֿ��O?", buf, 20, DOECHO);
	money = atoi(buf);
	if (money <= 0) {
	    move(2, 0);
	    prints("��J���~!!");
	    pressanykey();
	    return 1;
	}
    } else {
	if (vedit("etc/givemoney.txt", NA, NULL) < 0)
	    return 1;
    }

    clear();
    getdata(0, 0, "�n�o���F��(Y/N)[N]", buf, 3, LCECHO);
    if (buf[0] != 'y')
	return 1;

    if (!(fp2 = fopen("etc/givemoney.log", "a")))
	return 1;
    strftime(buf, sizeof(buf), "%Y/%m/%d/%H:%M", pt);
    fprintf(fp2, "%s\n", buf);


    getdata(1, 0, "���]�U���D �G", tt, TTLEN, DOECHO);
    move(2, 0);

    prints("�s���]�U���e");
    pressanykey();
    if (vedit("etc/givemoney.why", NA, NULL) < 0) {
        fclose(fp2);
	return 1;
    }

    stand_title("�o����...");
    if (to_all) {
	int             i, unum;
	for (unum = SHM->number, i = 0; i < unum; i++) {
	    if (bad_user_id(SHM->userid[i]))
		continue;
	    id = SHM->userid[i];
	    give_id_money(id, money, fp2, tt, now);
	}
	//something wrong @ _ @
	    //give_money_post("�����ϥΪ�", atoi(money));
    } else {
	if (!(fp = fopen("etc/givemoney.txt", "r+"))) {
	    fclose(fp2);
	    return 1;
	}
	while (fgets(buf, sizeof(buf), fp)) {
	    clear();
	    if (!(ptr = strchr(buf, ':')))
		continue;
	    *ptr = '\0';
	    id = buf;
	    mn = ptr + 1;
	    give_id_money(id, atoi(mn), fp2, tt, now);
	    give_money_post(id, atoi(mn));
	}
	fclose(fp);
    }

    fclose(fp2);
    pressanykey();
    return FULLUPDATE;
}
