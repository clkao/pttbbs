/* $Id$ */
#include "bbs.h"

/* �i�����y�Ŷ� */
int
m_loginmsg(void)
{
  char msg[100];
  move(21,0);
  clrtobot();
  if(SHM->loginmsg.pid && SHM->loginmsg.pid != currutmp->pid)
    {
      outs("�ثe�w�g���H�U�� �i�����y�]�w�Х���զn�A�]�w..");
      getmessage(SHM->loginmsg);
    }
  getdata(22, 0, 
     "�i�����y:��������,���z�Z�ϥΪ̬���,�]�w�������۰ʨ���,�T�w�n�]?(y/N)",
          msg, 3, LCECHO);

  if(msg[0]=='y' &&

     getdata_str(23, 0, "�]�w�i�����y:", msg, 56, DOECHO, SHM->loginmsg.last_call_in))
    {
          SHM->loginmsg.pid=currutmp->pid; /*�������h �N����race condition */
          strlcpy(SHM->loginmsg.last_call_in, msg, sizeof(SHM->loginmsg.last_call_in));
          strlcpy(SHM->loginmsg.userid, cuser.userid, sizeof(SHM->loginmsg.userid));
    }
  return 0;
}

/* �ϥΪ̺޲z */
int
m_user(void)
{
    userec_t        xuser;
    int             id;
    char            genbuf[200];

    vs_hdr("�ϥΪ̳]�w");
    usercomplete(msg_uid, genbuf);
    if (*genbuf) {
	move(2, 0);
	if ((id = getuser(genbuf, &xuser))) {
	    user_display(&xuser, 1);
	    if( HasUserPerm(PERM_ACCOUNTS) )
		uinfo_query(xuser.userid, 1, id);
	    else
		pressanykey();
	} else {
	    outs(err_uid);
	    clrtoeol();
	    pressanykey();
	}
    }
    return 0;
}

static int retrieve_backup(userec_t *user)
{
    int     uid;
    char    src[PATHLEN], dst[PATHLEN];
    char    ans;

    if ((uid = searchuser(user->userid, user->userid))) {
	userec_t orig;
	passwd_sync_query(uid, &orig);
	strlcpy(user->passwd, orig.passwd, sizeof(orig.passwd));
	setumoney(uid, user->money);
	passwd_sync_update(uid, user);
	return 0;
    }

    ans = vans("�ثe�� PASSWD �ɨS���� ID�A�s�W�ܡH[y/N]");

    if (ans != 'y') {
	vmsg("�ثe�� PASSWDS �ɨS���� ID�A�Х��s�W���b��");
	return -1;
    }

    if (setupnewuser((const userec_t *)user) >= 0) {
	sethomepath(dst, user->userid);
	if (!dashd(dst)) {
	    snprintf(src, sizeof(src), "tmp/%s", user->userid);
	    if (!dashd(src) || !Rename(src, dst))
		mkuserdir(user->userid);
	}
	return 0;
    }
    return -1;
}

int 
upgrade_passwd(userec_t *puser)
{
    if (puser->version == PASSWD_VERSION)
	return 1;
    if (!puser->userid[0])
	return 1;
    // unknown version
    return 0;

#if 0
    // this is a sample.
    if (puser->version == 2275) // chicken change
    {
	memset(puser->career,  0, sizeof(puser->career));
	memset(puser->phone,   0, sizeof(puser->phone));
	memset(puser->chkpad0, 0, sizeof(puser->chkpad0));
	memset(puser->chkpad1, 0, sizeof(puser->chkpad1));
	memset(puser->chkpad2, 0, sizeof(puser->chkpad2));
	puser->lastseen= 0;
	puser->version = PASSWD_VERSION;
	return ;
    }
#endif
}

static int
search_key_user(const char *passwdfile, int mode)
{
    userec_t        user;
    int             ch;
    int             unum = 0;
    FILE            *fp1 = fopen(passwdfile, "r");
    char            friendfile[PATHLEN]="", key[22], *keymatch;
    int		    keytype = 0;
    int	            isCurrentPwd;

    assert(fp1);

    isCurrentPwd = (strcmp(passwdfile, FN_PASSWD) == 0);
    clear();
    if (!mode)
    {
	getdata(0, 0, "�п�Jid :", key, sizeof(key), DOECHO);
    } else {
	// improved search
	vs_hdr("����r�j�M");
	outs("�j�M���: [0]���� 1.ID 2.�m�W 3.�ʺ� 4.�a�} 5.Mail 6.IP 7.¾�~ 8.�q�� 9.�{��\n");
	getdata(2, 0, "�n�j�M���ظ�ơH", key, 2, NUMECHO);

	if (isascii(key[0]) && isdigit(key[0]))
	    keytype = key[0] - '0';
	if (keytype < 0 || keytype > 9)
	    keytype = 0;
	getdata(3, 0, "�п�J����r: ", key, sizeof(key), DOECHO);
    }

    if(!key[0]) {
	fclose(fp1);
	return 0;
    }
    vs_hdr(key);

    // <= or < ? I'm not sure...
    while ((fread(&user, sizeof(user), 1, fp1)) > 0 && unum++ < MAX_USERS) {

	// skip empty records
	if (!user.userid[0])
	    continue;

	if (!(unum & 0xFF)) {
	    vs_hdr(key);
	    prints("�� [%d] �����\n", unum);
	    refresh();
	}

	// XXX �o�̷|���¸�ơA�n�p�� PWD �� upgrade
	if (!upgrade_passwd(&user))
	    continue;

        keymatch = NULL;

	if (!mode)
	{
	    // only verify id
	    if (!strcasecmp(user.userid, key))
		keymatch = user.userid; 
	} else {
	    // search by keytype
	    if ((!keytype || keytype == 1) &&
		DBCS_strcasestr(user.userid, key))
		keymatch = user.userid;
	    else if ((!keytype || keytype == 2) &&
		DBCS_strcasestr(user.realname, key))
		keymatch = user.realname;
	    else if ((!keytype || keytype == 3) &&
		DBCS_strcasestr(user.nickname, key))
		keymatch = user.nickname;
	    else if ((!keytype || keytype == 4) &&
		DBCS_strcasestr(user.address, key))
		keymatch = user.address;
	    else if ((!keytype || keytype == 5) &&
		strcasestr(user.email, key)) // not DBCS.
		keymatch = user.email;
	    else if ((!keytype || keytype == 6) &&
		strcasestr(user.lasthost, key)) // not DBCS.
		keymatch = user.lasthost;
	    else if ((!keytype || keytype == 7) &&
		DBCS_strcasestr(user.career, key))
		keymatch = user.career;
	    else if ((!keytype || keytype == 8) &&
		DBCS_strcasestr(user.phone, key))
		keymatch = user.phone;
	    else if ((!keytype || keytype == 9) &&
		DBCS_strcasestr(user.justify, key))
		keymatch = user.justify;
	}

        if(keymatch) {
	    vs_hdr(key);
	    prints("�� [%d] �����\n", unum);
	    refresh();

	    user_display(&user, 1);
	    // user_display does not have linefeed in tail.

	    if (isCurrentPwd && HasUserPerm(PERM_ACCOUNTS))
		uinfo_query(user.userid, 1, unum);
	    else
		outs("\n");

	    // XXX don't trust 'user' variable after here
	    // because uinfo_query may have changed it.

	    outs(ANSI_COLOR(44) "               �ť���" \
		 ANSI_COLOR(37) ":�j�M�U�@��          " \
		 ANSI_COLOR(33)" Q" ANSI_COLOR(37)": ���}");
	    outs(mode ? 
                 "      A: add to namelist " ANSI_RESET " " :
		 "      S: ���γƥ����    " ANSI_RESET " ");
	    while (1) {
		while ((ch = vkey()) == 0);
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
		    if (retrieve_backup(&user) >= 0) {
			fclose(fp1);
			return 0;
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
search_user_bypwd(void)
{
    search_key_user(FN_PASSWD, 1);
    return 0;
}

/* �M��ƥ����ϥΪ̸�� */
int
search_user_bybakpwd(void)
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
    outs(" [" ANSI_COLOR(1;31) "1" ANSI_RESET "]�@�ѫe,"
	 " [" ANSI_COLOR(1;31) "2" ANSI_RESET "]��ѫe," 
	 " [" ANSI_COLOR(1;31) "3" ANSI_RESET "]�T�ѫe\n");
    outs(" [" ANSI_COLOR(1;31) "4" ANSI_RESET "]�|�ѫe,"
	 " [" ANSI_COLOR(1;31) "5" ANSI_RESET "]���ѫe,"
	 " [" ANSI_COLOR(1;31) "6" ANSI_RESET "]���ѫe\n");
    outs(" [7]�ƥ���\n");
    do {
	move(5, 1);
	outs("��� => ");
	ch = vkey();
	if (ch == 'q' || ch == 'Q')
	    return 0;
    } while (ch < '1' || ch > '7');
    ch -= '1';
    if( access(choice[ch], R_OK) != 0 )
	vmsg("�ɮפ��s�b");
    else
	search_key_user(choice[ch], 0);
    return 0;
}

static void
bperm_msg(const boardheader_t * board)
{
    prints("\n�]�w [%s] �ݪO��(%s)�v���G", board->brdname,
	   board->brdattr & BRD_POSTMASK ? "�o��" : "�\\Ū");
}

unsigned int
setperms(unsigned int pbits, const char * const pstring[])
{
    register int    i;

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
    while (
       (i = vmsg("�Ы� [A-5] �����]�w�A�� [Return] �����G"))!='\r')
    {
	if (isdigit(i))
	    i = i - '0' + 26;
	else if (isalpha(i))
	    i = tolower(i) - 'a';
	else {
	    bell();
	    continue;
	}

	pbits ^= (1 << i);
	move(i % 16 + 4, i <= 15 ? 24 : 64);
	outs((pbits >> i) & 1 ? "��" : "��");
    }
    return pbits;
}

#ifdef CHESSCOUNTRY
static void
AddingChessCountryFiles(const char* apath)
{
    char filename[PATHLEN];
    char symbolicname[PATHLEN];
    char adir[PATHLEN];
    FILE* fp;
    fileheader_t fh;

    setadir(adir, apath);

    /* creating chess country regalia */
    snprintf(filename, sizeof(filename), "%s/chess_ensign", apath);
    close(open(filename, O_CREAT | O_WRONLY, 0644));

    strlcpy(symbolicname, apath, sizeof(symbolicname));
    stampfile(symbolicname, &fh);
    symlink("chess_ensign", symbolicname);

    strcpy(fh.title, "�� �Ѱ���� (����R���A�t�λݭn)");
    strcpy(fh.owner, str_sysop);
    append_record(adir, &fh, sizeof(fileheader_t));

    /* creating member list */
    snprintf(filename, sizeof(filename), "%s/chess_list", apath);
    if (!dashf(filename)) {
	fp = fopen(filename, "w");
	assert(fp);
	fputs("�Ѱ��W\n"
		"�b��            ����    �[�J���        ���ũγQ�֫R��\n"
		"�w�w�w�w�w�w    �w�w�w  �w�w�w�w�w      �w�w�w�w�w�w�w\n",
		fp);
	fclose(fp);
    }

    strlcpy(symbolicname, apath, sizeof(symbolicname));
    stampfile(symbolicname, &fh);
    symlink("chess_list", symbolicname);

    strcpy(fh.title, "�� �Ѱꦨ���� (����R���A�t�λݭn)");
    strcpy(fh.owner, str_sysop);
    append_record(adir, &fh, sizeof(fileheader_t));

    /* creating profession photos' dir */
    snprintf(filename, sizeof(filename), "%s/chess_photo", apath);
    mkdir(filename, 0755);

    strlcpy(symbolicname, apath, sizeof(symbolicname));
    stampfile(symbolicname, &fh);
    symlink("chess_photo", symbolicname);

    strcpy(fh.title, "�� �Ѱ�Ӥ��� (����R���A�t�λݭn)");
    strcpy(fh.owner, str_sysop);
    append_record(adir, &fh, sizeof(fileheader_t));
}
#endif /* defined(CHESSCOUNTRY) */

/* �۰ʳ]�ߺ�ذ� */
void
setup_man(const boardheader_t * board, const boardheader_t * oldboard)
{
    char            genbuf[200];

    setapath(genbuf, board->brdname);
    mkdir(genbuf, 0755);

#ifdef CHESSCOUNTRY
    if (oldboard == NULL || oldboard->chesscountry != board->chesscountry)
	if (board->chesscountry != CHESSCODE_NONE)
	    AddingChessCountryFiles(genbuf);
	// else // doesn't remove files..
#endif
}

void delete_symbolic_link(boardheader_t *bh, int bid)
{
    assert(0<=bid-1 && bid-1<MAX_BOARD);
    memset(bh, 0, sizeof(boardheader_t));
    substitute_record(fn_board, bh, sizeof(boardheader_t), bid);
    reset_board(bid);
    sort_bcache(); 
    log_usies("DelLink", bh->brdname);
}

int dir_cmp(const void *a, const void *b)
{
  return (atoi( &((fileheader_t *)a)->filename[2] ) -
          atoi( &((fileheader_t *)b)->filename[2] ));
}

void merge_dir(const char *dir1, const char *dir2, int isoutter)
{
     int i, pn, sn;
     fileheader_t *fh;
     char *p1, *p2, bakdir[128], file1[128], file2[128];
     strcpy(file1,dir1);
     strcpy(file2,dir2);
     if((p1=strrchr(file1,'/')))
	 p1 ++;
     else
	 p1 = file1;
     if((p2=strrchr(file2,'/')))
	 p2 ++;
     else
	 p2 = file2;

     pn=get_num_records(dir1, sizeof(fileheader_t));
     sn=get_num_records(dir2, sizeof(fileheader_t));
     if(!sn) return;
     fh= (fileheader_t *)malloc( (pn+sn)*sizeof(fileheader_t));
     get_records(dir1, fh, sizeof(fileheader_t), 1, pn);
     get_records(dir2, fh+pn, sizeof(fileheader_t), 1, sn);
     if(isoutter)
         {
             for(i=0; i<sn; i++)
               if(fh[pn+i].owner[0])
                   strcat(fh[pn+i].owner, "."); 
         }
     qsort(fh, pn+sn, sizeof(fileheader_t), dir_cmp);
     snprintf(bakdir, sizeof(bakdir), "%s.bak", dir1);
     Rename(dir1, bakdir);
     for(i=1; i<=pn+sn; i++ )
        {
         if(!fh[i-1].filename[0]) continue;
         if(i == pn+sn ||  strcmp(fh[i-1].filename, fh[i].filename))
	 {
                fh[i-1].recommend =0;
		fh[i-1].filemode |= 1;
                append_record(dir1, &fh[i-1], sizeof(fileheader_t));
		strcpy(p1, fh[i-1].filename);
                if(!dashf(file1))
		      {
			  strcpy(p2, fh[i-1].filename);
			  Copy(file2, file1);
		      } 
	 }
         else
                fh[i].filemode |= fh[i-1].filemode;
        }
     
     free(fh);
}

int
m_mod_board(char *bname)
{
    boardheader_t   bh, newbh;
    int             bid;
    char            genbuf[PATHLEN], ans[4];

    bid = getbnum(bname);
    if (!bid || !bname[0] || get_record(fn_board, &bh, sizeof(bh), bid) == -1) {
	vmsg(err_bid);
	return -1;
    }
    assert(0<=bid-1 && bid-1<MAX_BOARD);
    prints("�ݪO�W�١G%s\n�ݪO�����G%s\n�ݪObid�G%d\n�ݪOGID�G%d\n"
	   "�O�D�W��G%s", bh.brdname, bh.title, bid, bh.gid, bh.BM);
    bperm_msg(&bh);

    /* Ptt �o���_��|�ɨ�U�� */
    move(9, 0);
    snprintf(genbuf, sizeof(genbuf), "(E)�]�w (V)�H�k/�Ѱ�%s%s [Q]�����H",
	    HasUserPerm(PERM_SYSOP |
		     PERM_BOARD) ? " (B)Vote (S)�Ϧ^ (C)�X�� (G)��L�ѥd" : "",
	    HasUserPerm(PERM_SYSSUBOP | PERM_SYSSUPERSUBOP | PERM_BOARD) ? " (D)�R��" : "");
    getdata(10, 0, genbuf, ans, 3, LCECHO);

    switch (*ans) {
    case 'g':
	if (HasUserPerm(PERM_SYSOP | PERM_BOARD)) {
	    char            path[PATHLEN];
	    setbfile(genbuf, bname, FN_TICKET_LOCK);
	    setbfile(path, bname, FN_TICKET_END);
	    rename(genbuf, path);
	}
	break;
    case 's':
	if (HasUserPerm(PERM_SYSOP | PERM_BOARD)) {
	  snprintf(genbuf, sizeof(genbuf),
		   BBSHOME "/bin/buildir boards/%c/%s &",
		   bh.brdname[0], bh.brdname);
	    system(genbuf);
	}
	break;
    case 'c':
	if (HasUserPerm(PERM_SYSOP)) {
	   char frombname[20], fromdir[PATHLEN];
	    CompleteBoard(MSG_SELECT_BOARD, frombname);
            if (frombname[0] == '\0' || !getbnum(frombname) ||
		!strcmp(frombname,bname))
	                     break;
            setbdir(genbuf, bname);
            setbdir(fromdir, frombname);
            merge_dir(genbuf, fromdir, 0);
	    touchbtotal(bid);
	}
	break;
    case 'b':
	if (HasUserPerm(PERM_SYSOP | PERM_BOARD)) {
	    char            bvotebuf[10];

	    memcpy(&newbh, &bh, sizeof(bh));
	    snprintf(bvotebuf, sizeof(bvotebuf), "%d", newbh.bvote);
	    move(20, 0);
	    prints("�ݪO %s ��Ӫ� BVote�G%d", bh.brdname, bh.bvote);
	    getdata_str(21, 0, "�s�� Bvote�G", genbuf, 5, NUMECHO, bvotebuf);
	    newbh.bvote = atoi(genbuf);
	    assert(0<=bid-1 && bid-1<MAX_BOARD);
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
	    assert(0<=bid-1 && bid-1<MAX_BOARD);
	    substitute_record(fn_board, &newbh, sizeof(newbh), bid);
	    reset_board(bid);
	    log_usies("ViolateLawSet", newbh.brdname);
	}
	break;
    case 'd':
	if (!(HasUserPerm(PERM_SYSOP | PERM_BOARD) ||
		    (HasUserPerm(PERM_SYSSUPERSUBOP) && GROUPOP())))
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
		     "     %s �ݪO %s �R��", bname, cuser.userid);
	    post_msg(BN_SECURITY, bh.title, "�Ъ`�N�R�����X�k��", "[�t�Φw����]");
	    assert(0<=bid-1 && bid-1<MAX_BOARD);
	    substitute_record(fn_board, &bh, sizeof(bh), bid);
	    reset_board(bid);
            sort_bcache(); 
	    log_usies("DelBoard", bh.title);
	    outs("�R�O����");
	}
	break;
    case 'e':
	if( bh.brdattr & BRD_SYMBOLIC ){
	    vmsg("�T���ʳs���ݪO�A�Ъ����ץ���ݪO");
	    break;
	}
	move(8, 0);
	outs("������ [Return] ���ק�Ӷ��]�w");
	memcpy(&newbh, &bh, sizeof(bh));

	while (getdata(9, 0, "�s�ݪO�W�١G", genbuf, IDLEN + 1, DOECHO)) {
	    if (getbnum(genbuf)) {
		move(3, 0);
		outs("���~! �O�W�p�P");
	    } else if ( is_valid_brdname(genbuf) ){
		strlcpy(newbh.brdname, genbuf, sizeof(newbh.brdname));
		break;
	    }
	}

	do {
	    getdata_str(12, 0, "�ݪO���O�G", genbuf, 5, DOECHO, bh.title);
	    if (strlen(genbuf) == 4)
		break;
	} while (1);

	strcpy(newbh.title, genbuf);

	newbh.title[4] = ' ';

	// 7 for category
	getdata_str(14, 0, "�ݪO�D�D�G", genbuf, 
		BTLEN + 1 -7, DOECHO, bh.title + 7);
	if (genbuf[0])
	    strlcpy(newbh.title + 7, genbuf, sizeof(newbh.title) - 7);
	if (getdata_str(15, 0, "�s�O�D�W��G", genbuf, IDLEN * 3 + 3, DOECHO,
			bh.BM)) {
	    trim(genbuf);
	    strlcpy(newbh.BM, genbuf, sizeof(newbh.BM));
            newbh.BMexpire = now;
	}
#ifdef CHESSCOUNTRY
	if (HasUserPerm(PERM_SYSOP)) {
	    snprintf(genbuf, sizeof(genbuf), "%d", bh.chesscountry);
	    if (getdata_str(16, 0,
			"�]�w�Ѱ� (0)�L (1)���l�� (2)�H�� (3)��� (4) �¥մ�",
			ans, sizeof(ans), NUMECHO, genbuf)){
		newbh.chesscountry = atoi(ans);
		if (newbh.chesscountry > CHESSCODE_MAX ||
			newbh.chesscountry < CHESSCODE_NONE)
		    newbh.chesscountry = bh.chesscountry;
	    }
	}
#endif /* defined(CHESSCOUNTRY) */
	if (HasUserPerm(PERM_SYSOP|PERM_BOARD)) {
	    move(1, 0);
	    clrtobot();
	    newbh.brdattr = setperms(newbh.brdattr, str_permboard);
	    move(1, 0);
	    clrtobot();
	}
	{
	    const char* brd_symbol;
	    if (newbh.brdattr & BRD_GROUPBOARD)
        	brd_symbol = "�U";
	    else if (newbh.brdattr & BRD_NOTRAN)
		brd_symbol = "��";
	    else
		brd_symbol = "��";

	    newbh.title[5] = brd_symbol[0];
	    newbh.title[6] = brd_symbol[1];
	}

	if (HasUserPerm(PERM_SYSOP|PERM_BOARD) && !(newbh.brdattr & BRD_HIDE)) {
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
	    char buf[64];

	    if (strcmp(bh.brdname, newbh.brdname)) {
		char            src[60], tar[60];

		setbpath(src, bh.brdname);
		setbpath(tar, newbh.brdname);
		Rename(src, tar);

		setapath(src, bh.brdname);
		setapath(tar, newbh.brdname);
		Rename(src, tar);
	    }
	    setup_man(&newbh, &bh);
	    assert(0<=bid-1 && bid-1<MAX_BOARD);
	    substitute_record(fn_board, &newbh, sizeof(newbh), bid);
	    reset_board(bid);
            sort_bcache(); 
	    log_usies("SetBoard", newbh.brdname);

	    snprintf(buf, sizeof(buf), "[�ݪO�ܧ�] %s (by %s)", bh.brdname, cuser.userid);
	    snprintf(genbuf, sizeof(genbuf),
		    "�O�W: %s => %s\n"
		    "�O�D: %s => %s\n",
		    bh.brdname, newbh.brdname, bh.BM, newbh.BM);
	    post_msg(BN_SECURITY, buf, genbuf, "[�t�Φw����]");
	}
    }
    return 0;
}

/* �]�w�ݪO */
int
m_board(void)
{
    char            bname[32];

    vs_hdr("�ݪO�]�w");
    CompleteBoardAndGroup(msg_bid, bname);
    if (!*bname)
	return 0;
    m_mod_board(bname);
    return 0;
}

static void
str_unify_blank(char *s)
{
    while(*s)
    {
	if (*s == '\t') *s = ' ';
	s++;
    }
}

// ���i�@�U�A�g���̤j�W���C
#define MAX_ENTRIES (100)
#define min(a,b) ((a)<(b) ? (a) : (b))

// TODO ���ѧ�o�� UI �g���� general �@�I...

/* �]�w�t���ɮ� */
int
x_file(void)
{
    char *entries[MAX_ENTRIES] = {NULL};
    int  centries = 0, i = 0;
    char *fn, *v;
    int sel = 0, page = 0;
    char buf[PATHLEN];
    FILE *fp = NULL;

    fp = fopen(FN_CONF_EDITABLE, "rt");
    if (!fp) 
    {
	// you can find a sample in sample/etc/editable
	vmsgf("���]�w�i�s���ɮצC��[%s]�A�Ь��t�ί����C", FN_CONF_EDITABLE);
	return 0;
    }

    // load the editable file.
    // format: filename [ \t]* description
    while (centries < MAX_ENTRIES && fgets(buf, sizeof(buf), fp))
    {
	if (!buf[0] || buf[0] == '#' || buf[0] == '.' 
	    || buf[0] == '/' || buf[0] == ' ')
	    continue;
	str_unify_blank(buf); // replace all \t to ' ' in buf.
	v = strchr(buf, ' '); // find if description exists.
	if (v == NULL) continue;
	fn = strstr(buf, "..");	// see if someone trying to crack
	if (fn && fn < v) continue;
	// reject anything outside etc/ folder.
	if (strncmp(buf, "etc/", strlen("etc/")) != 0)
	    continue;
	chomp(buf);
	entries[centries++] = strdup(buf);
    }
    fclose(fp);
    if (centries == 0)
    {
	vmsg("�L�i�s���ɮסA�Ь��t�ί����C");
	return 0;
    }

    // edit the files!
    while (sel >= 0)
    {
	const int rows = t_lines-2;

	// display.
	clear(); showtitle("�t���ɮ�", "�s��t���ɮ�");
	for (i = page*rows; i < min(centries, (page+1)*rows); i++)
	{
	    // parse entry
	    strlcpy(buf, entries[i], sizeof(buf));
	    fn = buf;
	    v = strchr(fn, ' ');
	    *v++ = 0;
	    while (*v == ' ') v++;
	    if (strlen(fn) > 30)
		strcpy(fn+30-2, "..");
	    prints("  %3d. %s" 
		     "%-36.36s " 
		    ANSI_COLOR(0;1) "%-30.30s"
		    ANSI_RESET "\n", 
		    i+1, 
		    dashf(fn) ? ANSI_COLOR(1;32) : ANSI_COLOR(1;30;40),
		    v, fn);
	}
	vs_footer(" �s��t���ɮ� ",
		" (jk/����/0-9)���� (Enter/��)�s�� (d)�R�� \t(q/��)���X");
	cursor_show(1+sel-page*rows, 0);
	switch((i = vkey()))
	{
	    case 'q': case KEY_LEFT:
		sel = -1; continue;
	    case KEY_HOME: sel = 0; break;
	    case KEY_END:  sel = centries-1; break;
	    case KEY_PGDN: sel += rows; 
			   if (sel >= centries) sel = centries-1;
			   break;
	    case KEY_PGUP: sel -= rows;
			   if (sel < 0) sel = 0;
			   break;
	    case 'k': case KEY_UP: 
		if (sel > 0) sel--; break;
	    case 'j': case KEY_DOWN:
		if (sel < centries-1) sel++; break;

	    case 'd':
		strlcpy(buf, entries[sel], sizeof(buf));
		v = strchr(buf, ' '); *v++ = 0;
		i = vansf("�T�w�n�R�� %s �ܡH (y/N) ", v);
		if (i == 'y')
		    unlink(buf);
		vmsgf("�t���ɮ�[%s]: %s", buf, !dashf(buf) ? 
			"�R�����\\ " : "���R��");
		break;

	    case KEY_ENTER: case KEY_RIGHT:
		strlcpy(buf, entries[sel], sizeof(buf));
		v = strchr(buf, ' '); *v++ = 0;
		i = veditfile(buf);
		// log file change
		if (i != EDIT_ABORTED)
		{
		    log_filef("log/etc_edit.log", LOG_CREAT,
			    "%s %s %s # %s\n", Cdate(&now),
			    cuser.userid, buf, v);
		}
		vmsgf("�t���ɮ�[%s]: %s", buf, (i == -1) ? 
			"������" : "��s����");
		break;

	    default:
		if (i >= '0' && i <= '9')
		{
		    sel = search_num(i, centries-1);
		    if (sel < 0) sel = 0;
		}
		break;
	}
	// change page if required.
	page = sel / rows;
    }

    // free the entries
    for (i = 0; i < centries; i++)
	free(entries[i]);

    return FULLUPDATE;
}

static int add_board_record(const boardheader_t *board)
{
    int bid;
    if ((bid = getbnum("")) > 0) {
	assert(0<=bid-1 && bid-1<MAX_BOARD);
	substitute_record(fn_board, board, sizeof(boardheader_t), bid);
	reset_board(bid);
        sort_bcache(); 
    } else if (SHM->Bnumber >= MAX_BOARD) {
	return -1;
    } else if (append_record(fn_board, (fileheader_t *)board, sizeof(boardheader_t)) == -1) {
	return -1;
    } else {
	addbrd_touchcache();
    }
    return 0;
}

/**
 * open a new board
 * @param whatclass In which sub class
 * @param recover   Forcely open a new board, often used for recovery.
 * @return -1 if failed
 */
int
m_newbrd(int whatclass, int recover)
{
    boardheader_t   newboard;
    char            ans[4];
    char            genbuf[200];

    vs_hdr("�إ߷s�O");
    memset(&newboard, 0, sizeof(newboard));

    newboard.gid = whatclass;
    if (newboard.gid == 0) {
	vmsg("�Х���ܤ@�����O�A�}�O!");
	return -1;
    }
    do {
	if (!getdata(3, 0, msg_bid, newboard.brdname,
		     sizeof(newboard.brdname), DOECHO))
	    return -1;
    } while (!is_valid_brdname(newboard.brdname));

    do {
	getdata(6, 0, "�ݪO���O�G", genbuf, 5, DOECHO);
	if (strlen(genbuf) == 4)
	    break;
    } while (1);

    strcpy(newboard.title, genbuf);

    newboard.title[4] = ' ';

    getdata(8, 0, "�ݪO�D�D�G", genbuf, BTLEN + 1, DOECHO);
    if (genbuf[0])
	strlcpy(newboard.title + 7, genbuf, sizeof(newboard.title) - 7);
    setbpath(genbuf, newboard.brdname);

    if (!recover && 
        (getbnum(newboard.brdname) > 0 || mkdir(genbuf, 0755) == -1)) {
	vmsg("���ݪO�w�g�s�b! �Ш����P�^��O�W");
	return -1;
    }
    newboard.brdattr = BRD_NOTRAN;
#ifdef DEFAULT_AUTOCPLOG
    newboard.brdattr |= BRD_CPLOG;
#endif

    if (HasUserPerm(PERM_SYSOP)) {
	move(1, 0);
	clrtobot();
	newboard.brdattr = setperms(newboard.brdattr, str_permboard);
	move(1, 0);
	clrtobot();
    }
    getdata(9, 0, "�O�ݪO? (N:�ؿ�) (Y/n)�G", genbuf, 3, LCECHO);
    if (genbuf[0] == 'n')
    {
	newboard.brdattr |= BRD_GROUPBOARD;
	newboard.brdattr &= ~BRD_CPLOG;
    }

	{
	    const char* brd_symbol;
	    if (newboard.brdattr & BRD_GROUPBOARD)
        	brd_symbol = "�U";
	    else if (newboard.brdattr & BRD_NOTRAN)
		brd_symbol = "��";
	    else
		brd_symbol = "��";

	    newboard.title[5] = brd_symbol[0];
	    newboard.title[6] = brd_symbol[1];
	}

    newboard.level = 0;
    getdata(11, 0, "�O�D�W��G", newboard.BM, sizeof(newboard.BM), DOECHO);
    newboard.BMexpire = now;
#ifdef CHESSCOUNTRY
    if (getdata_str(12, 0, "�]�w�Ѱ� (0)�L (1)���l�� (2)�H�� (3)���", ans,
		sizeof(ans), LCECHO, "0")){
	newboard.chesscountry = atoi(ans);
	if (newboard.chesscountry > CHESSCODE_MAX ||
		newboard.chesscountry < CHESSCODE_NONE)
	    newboard.chesscountry = CHESSCODE_NONE;
    }
#endif /* defined(CHESSCOUNTRY) */

    if (HasUserPerm(PERM_SYSOP) && !(newboard.brdattr & BRD_HIDE)) {
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

    if (add_board_record(&newboard) < 0) {
	if (SHM->Bnumber >= MAX_BOARD)
	    vmsg("�ݪO�w���A�Ь��t�ί���");
	else
    	    vmsg("�ݪO�g�J����");

	setbpath(genbuf, newboard.brdname);
	rmdir(genbuf);
	return -1;
    }

    getbcache(whatclass)->childcount = 0;
    pressanykey();
    setup_man(&newboard, NULL);
    outs("\n�s�O����");
    post_newboard(newboard.title, newboard.brdname, newboard.BM);
    log_usies("NewBoard", newboard.title);
    pressanykey();
    return 0;
}

int make_symbolic_link(const char *bname, int gid)
{
    boardheader_t   newboard;
    int bid;
    
    bid = getbnum(bname);
    if(bid==0) return -1;
    assert(0<=bid-1 && bid-1<MAX_BOARD);
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

    CompleteBoard(msg_bid, buf);
    if (!buf[0])
	return -1;

    vs_hdr("�إ߬ݪO�s��");

    if (make_symbolic_link(buf, gid) < 0) {
	vmsg("�ݪO�s���إߥ���");
	return -1;
    }
    log_usies("NewSymbolic", buf);
    return 0;
}

static void
adm_give_id_money(const char *user_id, int money, const char *mail_title)
{
    char tt[TTLEN + 1] = {0}; 
    int  unum = searchuser(user_id, NULL); 

    // XXX �����̦��G�Q�γo�ӥ\��ӦP�ɵo���Φ����Areturn value �i��O 0
    // (�Y�N������Q����)
    if (unum <= 0 || pay_as_uid(unum, -money, "����%s: %s",
                                money >= 0 ? "�o���]" : "����",
                                mail_title) < 0) {
	move(12, 0);
	clrtoeol();
	prints("id:%s money:%d ����a!!", user_id, money);
	pressanykey();
    } else {
	snprintf(tt, sizeof(tt), "%s : %d " MONEYNAME " ��", mail_title, money);
	mail_id(user_id, tt, "etc/givemoney.why", "[" BBSMNAME "�Ȧ�]");
    }
}

int
give_money(void)
{
    FILE           *fp, *fp2;
    char           *ptr, *id, *mn;
    char            buf[200] = "", reason[100], tt[TTLEN + 1] = "";
    int             to_all = 0, money = 0;
    int             total_money=0, count=0;

    getdata(0, 0, "���w�ϥΪ�(S) �����ϥΪ�(A) ����(Q)�H[S]", buf, 3, LCECHO);
    if (buf[0] == 'q')
	return 1;
    else if (buf[0] == 'a') {
	to_all = 1;
	getdata(1, 0, "�o�h�ֿ��O?", buf, 20, DOECHO);
	money = atoi(buf);
	if (money <= 0) {
	    move(2, 0);
	    vmsg("��J���~!!");
	    return 1;
	}
    } else {
	if (veditfile("etc/givemoney.txt") < 0)
	    return 1;
    }

    clear();

    unlink("etc/givemoney.log");
    if (!(fp2 = fopen("etc/givemoney.log", "w")))
	return 1;

    getdata(0, 0, "�ʥΰ�w!�п�J����z��(�p���ʦW��):", reason, 40, DOECHO);
    fprintf(fp2,"\n�ϥβz��: %s\n", reason);

    getdata(1, 0, "�n�o���F��(Y/N)[N]", buf, 3, LCECHO);
    if (buf[0] != 'y')
       {
        fclose(fp2);
	return 1;
       }

    getdata(1, 0, "���]�U���D �G", tt, TTLEN, DOECHO);
    fprintf(fp2,"\n���]�U���D: %s\n", tt);
    move(2, 0);

    vmsg("�s���]�U���e");
    if (veditfile("etc/givemoney.why") < 0) {
        fclose(fp2);
	return 1;
    }

    vs_hdr("�o����...");
    if (to_all) {
	int             i, unum;
	for (unum = SHM->number, i = 0; i < unum; i++) {
	    if (bad_user_id(SHM->userid[i]))
		continue;
	    id = SHM->userid[i];
	    adm_give_id_money(id, money, tt);
            fprintf(fp2,"�� %s : %d\n", id, money);
            count++;
	}
        sprintf(buf, "(%d�H:%d"MONEYNAME"��)", count, count*money);
        strcat(reason, buf);
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
            money = atoi(mn);
	    adm_give_id_money(id, money, tt);
            fprintf(fp2,"�� %s : %d\n", id, money);
            total_money += money;
            count++;
	}
	fclose(fp);
        sprintf(buf, "(%d�H:%d"MONEYNAME"��)", count, total_money);
        strcat(reason, buf);
    
    }

    fclose(fp2);

    sprintf(buf, "%s ���]��: %s", cuser.userid, reason);
    post_file(BN_SECURITY, buf, "etc/givemoney.log", "[���]�����i]");
    pressanykey();
    return FULLUPDATE;
}
