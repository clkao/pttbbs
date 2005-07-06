/* $Id$ */
#include "bbs.h"

#define WHEREAMI_LEVEL	16

static int recommend(int ent, fileheader_t * fhdr, const char *direct);
int mailalert(const char *userid);

#ifdef ASSESS
static char * const badpost_reason[] = {
    "�s�i", "�������", "�H������"
};
#endif

void
anticrosspost(void)
{
    log_file("etc/illegal_money",  LOG_CREAT | LOG_VF,
             ANSI_COLOR(1;33;46) "%s "
             ANSI_COLOR(37;45) "cross post �峹 "
             ANSI_COLOR(37) " %s" ANSI_RESET "\n", 
             cuser.userid, ctime4(&now));
    kick_all(cuser.userid);
    post_violatelaw(cuser.userid, "Ptt�t��ĵ��", "Cross-post", "�@��B��");
    cuser.userlevel |= PERM_VIOLATELAW;
    cuser.vl_count++;
    mail_id(cuser.userid, "Cross-Post�@��",
	    "etc/crosspost.txt", "Pttĵ���");
    u_exit("Cross Post");
    exit(0);
}

/* Heat CharlieL */
int
save_violatelaw(void)
{
    char            buf[128], ok[3];

    setutmpmode(VIOLATELAW);
    clear();
    stand_title("ú�@�椤��");

    if (!(cuser.userlevel & PERM_VIOLATELAW)) {
	mouts(22, 0, ANSI_COLOR(1;31) "�A�L���? �A�S�S���Q�}�@��~~" ANSI_RESET);
	pressanykey();
	return 0;
    }
    reload_money();
    if (cuser.money < (int)cuser.vl_count * 1000) {
	snprintf(buf, sizeof(buf),
		 ANSI_COLOR(1;31) "�o�O�A�� %d ���H�ϥ����k�W"
		 "����ú�X %d $Ptt ,�A�u�� %d ��, ��������!!" ANSI_RESET,
           (int)cuser.vl_count, (int)cuser.vl_count * 1000, cuser.money);
	mouts(22, 0, buf);
	pressanykey();
	return 0;
    }
    move(5, 0);
    outs(ANSI_COLOR(1;37) "�A���D��? �]���A���H�k "
	   "�w�g�y���ܦh�H�����K" ANSI_RESET "\n");
    outs(ANSI_COLOR(1;37) "�A�O�_�T�w�H�ᤣ�|�A�ǤF�H" ANSI_RESET "\n");

    if (!getdata(10, 0, "�T�w�ܡH[Y/n]:", ok, sizeof(ok), LCECHO) ||
	ok[0] == 'n' || ok[0] == 'N') {
	mouts(22, 0, ANSI_COLOR(1;31) "���A�Q�q�F�A�ӧa!! "
		"�ڬ۫H�A���|�������諸~~~" ANSI_RESET);
	pressanykey();
	return 0;
    }
    snprintf(buf, sizeof(buf), "�o�O�A�� %d ���H�k ����ú�X %d $Ptt",
	     cuser.vl_count, cuser.vl_count * 1000);
    mouts(11, 0, buf);

    if (!getdata(10, 0, "�n�I��[Y/n]:", ok, sizeof(ok), LCECHO) ||
	ok[0] == 'N' || ok[0] == 'n') {

	mouts(22, 0, ANSI_COLOR(1;31) " �� �s���� �A�ӧa!!!" ANSI_RESET);
	pressanykey();
	return 0;
    }

    reload_money();
    if (cuser.money < (int)cuser.vl_count * 1000) return 0; //Ptt:check one more time

    demoney(-1000 * cuser.vl_count);
    cuser.userlevel &= (~PERM_VIOLATELAW);
    passwd_update(usernum, &cuser);
    return 0;
}

/*
 * void make_blist() { CreateNameList(); apply_boards(g_board_names); }
 */

static time4_t  *board_note_time = NULL;

void
set_board(void)
{
    boardheader_t  *bp;

    bp = getbcache(currbid);
    if( !HasBoardPerm(bp) ){
	vmsg("access control violation, exit");
	u_exit("access control violation!");
	exit(-1);
    }

    if( HasUserPerm(PERM_SYSOP) &&
	(bp->brdattr & BRD_HIDE) &&
	!is_BM_cache(bp - bcache + 1) &&
	hbflcheck((int)(bp - bcache) + 1, currutmp->uid) )
	vmsg("�i�J���g���v�ݪO");

    board_note_time = &bp->bupdate;
    /* board_visit_time was broken before.
     * It seems like that the behavior should be
     *  "Show board notes once per login"
     * but the real bahavior was
     *  "Show board everytime I enter board".
     * So, let's disable board_visit_time now.
     */
    // board_visit_time = getbrdtime(ptr->bid);

    if(bp->BM[0] <= ' ')
	strcpy(currBM, "�x�D��");
    else
	snprintf(currBM, sizeof(currBM), "�O�D�G%s", bp->BM);

    /* init basic perm, but post perm is checked on demand */
    currmode = (currmode & (MODE_DIRTY | MODE_GROUPOP)) | MODE_STARTED;
    if (!HasUserPerm(PERM_NOCITIZEN) && 
         (HasUserPerm(PERM_ALLBOARD) || is_BM_cache(currbid))) {
	currmode = currmode | MODE_BOARD | MODE_POST | MODE_POSTCHECKED;
    }
}

/* check post perm on demand, no double checks in current board */
int
CheckPostPerm(void)
{
    if (!(currmode & MODE_POSTCHECKED)) {
	currmode |= MODE_POSTCHECKED;
	if (haspostperm(currboard)) {
	    currmode |= MODE_POST;
	    return 1;
	}
	currmode &= ~MODE_POST;
	return 0;
    }
    return (currmode & MODE_POST);
}

static void
readtitle(void)
{
    boardheader_t  *bp;
    char    *brd_title;

    bp = getbcache(currbid);
    if(bp->bvote != 2 && bp->bvote)
	brd_title = "���ݪO�i��벼��";
    else
	brd_title = bp->title + 7;

    showtitle(currBM, brd_title);
    prints("[��]���} [��]�\\Ū [^P]�o��峹 [b]�Ƨѿ� [d]�R�� [z]��ذ� "
      "[TAB]��K [h]elp\n" ANSI_COLOR(7) "  �s��    �� �� �@  ��       ��  ��  ��  �D");
#ifdef USE_COOLDOWN
    if (bp->brdattr & BRD_COOLDOWN && !((currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP)))
        prints("                                   " ANSI_RESET);
    else 
#endif
        prints("                      �H��:%-5d   " ANSI_RESET,
	   SHM->bcache[currbid - 1].nuser);
}

static void
readdoent(int num, fileheader_t * ent)
{
    int             type;
    char           *mark, *title, color, special = 0, isonline = 0, recom[5];
    userinfo_t     *uentp;
    type = brc_unread(ent->filename, brc_num, brc_list) ? '+' : ' ';
    if ((currmode & MODE_BOARD) && (ent->filemode & FILE_DIGEST))
	type = (type == ' ') ? '*' : '#';
    else if (currmode & MODE_BOARD || HasUserPerm(PERM_LOGINOK)) {
	if (ent->filemode & FILE_MARKED)
         {
            if(ent->filemode & FILE_SOLVED)
	       type = '!';
            else 
	       type = (type == ' ') ? 'm' : 'M';
         }
	else if (TagNum && !Tagger(atoi(ent->filename + 2), 0, TAG_NIN))
	    type = 'D';
	else if (ent->filemode & FILE_SOLVED)
	    type = (type == ' ') ? 's': 'S';
    }
    title = subject(ent->title);
    if (ent->filemode & FILE_VOTE)
	color = '2', mark = "��";
    else if (ent->filemode & FILE_BID)
	color = '6', mark = "�C";
    else if (title == ent->title)
	color = '1', mark = "��";
    else
	color = '3', mark = "R:";

    /* ��h�l�� string �屼 */
    if (title[45])
	strlcpy(title + PROPER_TITLE_LEN, " �K", sizeof(title) -  PROPER_TITLE_LEN);

    if (!strncmp(title, "[���i]", 6))
	special = 1;
#if 1
    if (!strchr(ent->owner, '.') && !SHM->GV2.e.noonlineuser &&
	(uentp = search_ulist_userid(ent->owner)) && isvisible(currutmp, uentp))
	isonline = 1;
#else
    if (!strchr(ent->owner, '.') && (uid = searchuser(ent->owner, NULL)) &&
	!SHM->GV2.e.noonlineuser &&
	(uentp = search_ulist(uid)) && isvisible(currutmp, uentp))
	isonline = 1;
#endif
    if(ent->recommend>99)
	  strcpy(recom,"1m�z");
    else if(ent->recommend>9)
	  sprintf(recom,"3m%2d",ent->recommend);
    else if(ent->recommend>0)
	  sprintf(recom,"2m%2d",ent->recommend);
    else if(ent->recommend<-99)
	  sprintf(recom,"0mXX");
    else if(ent->recommend<-10)
	  sprintf(recom,"0mX%d",-ent->recommend);
    else strcpy(recom,"0m  "); 

    if (ent->filemode & FILE_BOTTOM)
           outs("  " ANSI_COLOR(1;33) " �� " ANSI_RESET);
    else
           prints("%6d", num);

    prints(
#ifdef COLORDATE
	   " %c\033[1;3%4.4s" ANSI_COLOR(%d) "%-6s" ANSI_RESET ANSI_COLOR(%d) "%-13.12s",
#else
	   " %c\033[1;3%4.4s" ANSI_RESET "%-6s" ANSI_COLOR(%d) "%-13.12s",
#endif
	    type, recom,
#ifdef COLORDATE
	   (ent->date[3] + ent->date[4]) % 7 + 31,
#endif
	   ent->date, isonline, ent->owner);
	   
    if (strncmp(currtitle, title, TTLEN))
	prints(ANSI_RESET "%s " ANSI_COLOR(1) "%.*s" ANSI_RESET "%s\n",
	       mark, special ? 6 : 0, title, special ? title + 6 : title);
    else
	prints("\033[1;3%cm%s %s" ANSI_RESET "\n",
	       color, mark, title);
}

int
whereami(int ent, const fileheader_t * fhdr, const char *direct)
{
    boardheader_t  *bh, *p[WHEREAMI_LEVEL];
    int             i, j;

    if (!currutmp->brc_id)
	return 0;

    move(1, 0);
    clrtobot();
    bh = getbcache(currutmp->brc_id);
    p[0] = bh;
    for (i = 0; i+1 < WHEREAMI_LEVEL && p[i]->parent>1 && p[i]->parent < numboards; i++)
	p[i + 1] = getbcache(p[i]->parent);
    j = i;
    prints("�ڦb��?\n%-40.40s %.13s\n", p[j]->title + 7, p[j]->BM);
    for (j--; j >= 0; j--)
	prints("%*s %-13.13s %-37.37s %.13s\n", (i - j) * 2, "",
	       p[j]->brdname, p[j]->title,
	       p[j]->BM);

    pressanykey();
    return FULLUPDATE;
}


static int
do_select(int ent, const fileheader_t * fhdr, const char *direct)
{
    char            bname[20];
    char            bpath[60];
    boardheader_t  *bh;
    struct stat     st;
    int             i;

    setutmpmode(SELECT);
    move(0, 0);
    clrtoeol();
    CompleteBoard(MSG_SELECT_BOARD, bname);
    if (bname[0] == '\0' || !(i = getbnum(bname)))
	return FULLUPDATE;
    bh = getbcache(i);
    if (!HasBoardPerm(bh))
	return FULLUPDATE;
    strlcpy(bname, bh->brdname, sizeof(bname));
    brc_update();
    currbid = i;

    setbpath(bpath, bname);
    if ((*bname == '\0') || (stat(bpath, &st) == -1)) {
	move(2, 0);
	clrtoeol();
	outs(err_bid);
	return FULLUPDATE;
    }
    setutmpbid(currbid);

    brc_initial_board(bname);
    set_board();
    setbdir(currdirect, currboard);

    move(1, 0);
    clrtoeol();
    return NEWDIRECT;
}

/* ----------------------------------------------------- */
/* ��} innbbsd ��X�H��B�s�u��H���B�z�{��             */
/* ----------------------------------------------------- */
void
outgo_post(const fileheader_t *fh, const char *board, const char *userid, const char *nickname)
{
    FILE           *foo;

    if ((foo = fopen("innd/out.bntp", "a"))) {
	fprintf(foo, "%s\t%s\t%s\t%s\t%s\n",
		board, fh->filename, userid, nickname, fh->title);
	fclose(foo);
    }
}

static void
cancelpost(const fileheader_t *fh, int by_BM, char *newpath)
{
    FILE           *fin, *fout;
    char           *ptr, *brd;
    fileheader_t    postfile;
    char            genbuf[200];
    char            nick[STRLEN], fn1[STRLEN];
    int             len = 42-strlen(currboard);
    struct tm      *ptime = localtime4(&now);

    if(!fh->filename[0]) return;
    setbfile(fn1, currboard, fh->filename);
    if ((fin = fopen(fn1, "r"))) {
	brd = by_BM ? "deleted" : "junk";

	setbpath(newpath, brd);
	stampfile(newpath, &postfile);
	memcpy(postfile.owner, fh->owner, IDLEN + TTLEN + 10);

	nick[0] = '\0';
	while (fgets(genbuf, sizeof(genbuf), fin)) {
	    if (!strncmp(genbuf, str_author1, LEN_AUTHOR1) ||
		!strncmp(genbuf, str_author2, LEN_AUTHOR2)) {
		if ((ptr = strrchr(genbuf, ')')))
		    *ptr = '\0';
		if ((ptr = (char *)strchr(genbuf, '(')))
		    strlcpy(nick, ptr + 1, sizeof(nick));
		break;
	    }
	}
	if(!strncasecmp(postfile.title, str_reply, 3))
	    len=len+4;
	sprintf(postfile.title, "%-*.*s.%s�O", len, len, fh->title, currboard);

	if ((fout = fopen("innd/cancel.bntp", "a"))) {
	    fprintf(fout, "%s\t%s\t%s\t%s\t%s\n", currboard, fh->filename,
		    cuser.userid, nick, fh->title);
	    fclose(fout);
	}
	fclose(fin);
        log_file(fn1,  LOG_CREAT | LOG_VF, "\n�� Deleted by: %s (%s) %d/%d",
                 cuser.userid, fromhost, ptime->tm_mon + 1, ptime->tm_mday);
	Rename(fn1, newpath);
	setbdir(genbuf, brd);
	setbtotal(getbnum(brd));
	append_record(genbuf, &postfile, sizeof(postfile));
    }
}

/* ----------------------------------------------------- */
/* �o��B�^���B�s��B����峹                            */
/* ----------------------------------------------------- */
void
do_reply_title(int row, const char *title)
{
    char            genbuf[200];
    char            genbuf2[4];

    if (strncasecmp(title, str_reply, 4))
	snprintf(save_title, sizeof(save_title), "Re: %s", title);
    else
	strlcpy(save_title, title, sizeof(save_title));
    save_title[TTLEN - 1] = '\0';
    snprintf(genbuf, sizeof(genbuf), "�ĥέ���D�m%.60s�n��?[Y] ", save_title);
    getdata(row, 0, genbuf, genbuf2, 4, LCECHO);
    if (genbuf2[0] == 'n' || genbuf2[0] == 'N')
	getdata(++row, 0, "���D�G", save_title, TTLEN, DOECHO);
}
/*
static void
do_unanonymous_post(const char *fpath)
{
    fileheader_t    mhdr;
    char            title[128];
    char            genbuf[200];

    setbpath(genbuf, "UnAnonymous");
    if (dashd(genbuf)) {
	stampfile(genbuf, &mhdr);
	unlink(genbuf);
	// XXX: Link should use BBSHOME/blah
	Link(fpath, genbuf);
	strlcpy(mhdr.owner, cuser.userid, sizeof(mhdr.owner));
	strlcpy(mhdr.title, save_title, sizeof(mhdr.title));
	mhdr.filemode = 0;
	setbdir(title, "UnAnonymous");
	append_record(title, &mhdr, sizeof(mhdr));
    }
}
*/

void 
do_crosspost(const char *brd, fileheader_t *postfile, const char *fpath)
{
    char            genbuf[200];
    int             len = 42-strlen(currboard);
    fileheader_t    fh;
    if(!strncasecmp(postfile->title, str_reply, 3))
        len=len+4;
    setbpath(genbuf, brd);
    stampfile(genbuf, &fh);
    if(!strcmp(brd, "UnAnonymous"))
       strcpy(fh.owner, cuser.userid);
    else
       strcpy(fh.owner, postfile->owner);
    strcpy(fh.date, postfile->date);
    sprintf(fh.title,"%-*.*s.%s�O",  len, len, postfile->title, currboard);
    unlink(genbuf);
    Copy((char *)fpath, genbuf);
    postfile->filemode = FILE_LOCAL;
    setbdir(genbuf, brd);
    if (append_record(genbuf, &fh, sizeof(fileheader_t)) != -1) {
	if(strcmp(brd, ALLPOST) == 0)
	{
	    /* quick update */
	    int bid = getbnum(ALLPOST);
	    touchbpostnum(bid, 1);
	    SHM->lastposttime[bid - 1] = now;
	} else
	    touchbtotal(getbnum(brd));
    }
}
static void 
setupbidinfo(bid_t *bidinfo)
{
    char buf[256];
    bidinfo->enddate = gettime(20, now+86400,"�����Юש�");
    do{
	getdata_str(21, 0, "����:", buf, 8, LCECHO, "1");
    } while( (bidinfo->high = atoi(buf)) <= 0 );
    do{
	getdata_str(21, 20, "�C�ЦܤּW�[�h��:", buf, 5, LCECHO, "1");
    } while( (bidinfo->increment = atoi(buf)) <= 0 );
    getdata(21,44, "�����ʶR��(�i���]):",buf, 10, LCECHO);
    bidinfo->buyitnow = atoi(buf);
	
    getdata_str(22,0,
		"�I�ڤ覡: 1.Ptt�� 2.�l���λȦ���b 3.�䲼�ιq�� 4.�l���f��I�� [1]:",
		buf, 3, LCECHO,"1");
    bidinfo->payby = (buf[0] - '1');
    if( bidinfo->payby < 0 || bidinfo->payby > 3)
	bidinfo->payby = 0;
    getdata_str(23, 0, "�B�O(0:�K�B�O�Τ夤����)[0]:", buf, 6, LCECHO, "0"); 
    bidinfo->shipping = atoi(buf);
    if( bidinfo->shipping < 0 )
	bidinfo->shipping = 0;
}
static void
print_bidinfo(FILE *io, bid_t bidinfo)
{
    char *payby[4]={"Ptt��", "�l���λȦ���b", "�䲼�ιq��", "�l���f��I��"};
    if(io){
	if( !bidinfo.userid[0] )
	    fprintf(io, "�_�л�:    %-20d\n", bidinfo.high);
	else 
	    fprintf(io, "�ثe�̰���:%-20d�X����:%-16s\n",
		    bidinfo.high, bidinfo.userid);
	fprintf(io, "�I�ڤ覡:  %-20s������:%-16s\n",
		payby[bidinfo.payby % 4], Cdate(& bidinfo.enddate));
	if(bidinfo.buyitnow)
	    fprintf(io, "�����ʶR��:%-20d", bidinfo.buyitnow);
	if(bidinfo.shipping)
	    fprintf(io, "�B�O:%d", bidinfo.shipping);
	fprintf(io, "\n");
    }
    else{
	if(!bidinfo.userid[0])
	    prints("�_�л�:    %-20d\n", bidinfo.high);
	else 
	    prints("�ثe�̰���:%-20d�X����:%-16s\n",
		   bidinfo.high, bidinfo.userid);
	prints("�I�ڤ覡:  %-20s������:%-16s\n",
	       payby[bidinfo.payby % 4], Cdate(& bidinfo.enddate));
	if(bidinfo.buyitnow)
	    prints("�����ʶR��:%-20d", bidinfo.buyitnow);
	if(bidinfo.shipping)
	    prints("�B�O:%d", bidinfo.shipping);
	outc('\n');
    }
}

static int
do_general(int isbid)
{
    bid_t           bidinfo;
    fileheader_t    postfile;
    char            fpath[80], buf[80];
    int             aborted, defanony, ifuseanony, i;
    char            genbuf[200], *owner;
    char            ctype[8][5] = {"���D", "��ĳ", "�Q��", "�߱o",
				   "����", "�Яq", "���i", "����"};
    boardheader_t  *bp;
    int             islocal, posttype=-1;

    ifuseanony = 0;
    bp = getbcache(currbid);

    if( !CheckPostPerm()
#ifdef FOREIGN_REG
	// ���O�~�y�ϥΪ̦b PttForeign �O
	&& !((cuser.uflag2 & FOREIGN) && strcmp(bp->brdname, "PttForeign") == 0)
#endif
	) {
	vmsg("�藍�_�A�z�ثe�L�k�b���o��峹�I");
	return READ_REDRAW;
    }

#ifndef DEBUG
    if ( !((currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP)) &&
	    (cuser.firstlogin > (now - (time4_t)bcache[currbid - 1].post_limit_regtime * 2592000) ||
	    cuser.numlogins < ((unsigned int)(bcache[currbid - 1].post_limit_logins) * 10) ||
	    cuser.numposts < ((unsigned int)(bcache[currbid - 1].post_limit_posts) * 10)) ) {
	move(5, 10);
	vmsg("�A������`��I");
	return FULLUPDATE;
    }
#ifdef USE_COOLDOWN
   if(check_cooldown(bp))
       return READ_REDRAW;
#endif
#endif
    clear();

    if(likely(!isbid))
       setbfile(genbuf, currboard, FN_POST_NOTE);
    else
       setbfile(genbuf, currboard, FN_POST_BID);

    if (more(genbuf, NA) == -1) {
	if(!isbid)
	    more("etc/" FN_POST_NOTE, NA);
	else
	    more("etc/" FN_POST_BID, NA);
    }
    move(19, 0);
    prints("%s��i" ANSI_COLOR(33) " %s" ANSI_RESET " �j "
	   ANSI_COLOR(32) "%s" ANSI_RESET " �ݪO\n",
           isbid?"���}�ۼ�":"�o��峹",
	  currboard, bp->title + 7);

    if (unlikely(isbid)) {
	memset(&bidinfo,0,sizeof(bidinfo)); 
	setupbidinfo(&bidinfo);
	postfile.multi.money=bidinfo.high;
	move(20,0);
	clrtobot();
    }
    if (quote_file[0])
	do_reply_title(20, currtitle);
    else {
	if (!isbid) {
	    move(21,0);
	    outs("�����G");
	    for(i=0; i<8 && bp->posttype[i*4]; i++)
		strncpy(ctype[i],bp->posttype+4*i,4);
	    if(i==0) i=8;
	    for(aborted=0; aborted<i; aborted++)
		prints("%d.%4.4s ", aborted+1, ctype[aborted]);
	    sprintf(buf,"(1-%d�Τ���)",i);
	    getdata(21, 6+7*i, buf, save_title, 3, LCECHO); 
	    posttype = save_title[0] - '1';
	    if (posttype >= 0 && posttype < i)
		snprintf(save_title, sizeof(save_title),
			"[%s] ", ctype[posttype]);
	    else
	    {
		save_title[0] = '\0';
		posttype=-1;
	    }
	}
	getdata_buf(22, 0, "���D�G", save_title, TTLEN, DOECHO);
	strip_ansi(save_title, save_title, STRIP_ALL);
    }
    if (save_title[0] == '\0')
	return FULLUPDATE;

    curredit &= ~EDIT_MAIL;
    curredit &= ~EDIT_ITEM;
    setutmpmode(POSTING);
    /* ����� Internet �v���̡A�u��b�����o��峹 */
    /* �O�D�w�]�����s�� */
    if (HasUserPerm(PERM_INTERNET) && !(bp->brdattr & BRD_LOCALSAVE))
	local_article = 0;
    else
	local_article = 1;

    /* build filename */
    setbpath(fpath, currboard);
    stampfile(fpath, &postfile);
    if(isbid) {
	FILE    *fp;
	if( (fp = fopen(fpath, "w")) != NULL ){
	    print_bidinfo(fp, bidinfo);
	    fclose(fp);
	}
    }
    else if(posttype!=-1 && ((1<<posttype) & bp->posttype_f)) {
	setbnfile(genbuf, bp->brdname, "postsample", posttype);
	Copy(genbuf, fpath);
    }
    
    aborted = vedit(fpath, YEA, &islocal);
    if (aborted == -1) {
	unlink(fpath);
	pressanykey();
	return FULLUPDATE;
    }
    /* set owner to Anonymous , for Anonymous board */

#ifdef HAVE_ANONYMOUS
    /* Ptt and Jaky */
    defanony = currbrdattr & BRD_DEFAULTANONYMOUS;
    if ((currbrdattr & BRD_ANONYMOUS) &&
	((strcmp(real_name, "r") && defanony) || (real_name[0] && !defanony))
	) {
	strcat(real_name, ".");
	owner = real_name;
	ifuseanony = 1;
    } else
	owner = cuser.userid;
#else
    owner = cuser.userid;
#endif

    /* �� */
    if (aborted > MAX_POST_MONEY * 2)
	aborted = MAX_POST_MONEY;
    else
	aborted /= 2;

    if(ifuseanony) {
	postfile.filemode |= FILE_ANONYMOUS;
	postfile.multi.anon_uid = currutmp->uid;
    }
    else if(!isbid)
       postfile.multi.money = aborted;
    
    strlcpy(postfile.owner, owner, sizeof(postfile.owner));
    strlcpy(postfile.title, save_title, sizeof(postfile.title));
    if (islocal)		/* local save */
	postfile.filemode |= FILE_LOCAL;

    setbdir(buf, currboard);
    if(isbid) {
	sprintf(genbuf, "%s.bid", fpath);
	append_record(genbuf,(void*) &bidinfo, sizeof(bidinfo));
	postfile.filemode |= FILE_BID ;
    }
    if (append_record(buf, &postfile, sizeof(postfile)) != -1) {
	setbtotal(currbid);

	if( currmode & MODE_SELECT )
	    append_record(currdirect, &postfile, sizeof(postfile));
	if( !islocal && !(bp->brdattr & BRD_NOTRAN) ){
#ifdef HAVE_ANONYMOUS
	    if( ifuseanony )
		outgo_post(&postfile, currboard, owner, "Anonymous.");
	    else
#endif
		outgo_post(&postfile, currboard, cuser.userid, cuser.nickname);
	}
	brc_addlist(postfile.filename);

	if (!(currbrdattr & BRD_HIDE) &&
	    (!bp->level || (currbrdattr & BRD_POSTMASK))) {
            do_crosspost(ALLPOST, &postfile, fpath);
	}
	outs("���Q�K�X�G�i�A");

#ifdef MAX_POST_MONEY
	if (aborted > MAX_POST_MONEY)
	    aborted = MAX_POST_MONEY;
#endif
	if (strcmp(currboard, "Test") && !ifuseanony) {
	    prints("�o�O�z���� %d �g�峹�C",++cuser.numposts);
            if(postfile.filemode&FILE_BID)
                outs("�ۼФ峹�S���Z�S�C");
            else if(currbrdattr&BRD_BAD)
                outs("�H�k��i���ݪO�S���Z�S�C");
            else
              {
                prints(" �Z�S %d �ȡC",aborted);
                demoney(aborted);    
              }
	} else
	    outs("���իH�󤣦C�J�����A�q�Х]�[�C");

	/* �^�����@�̫H�c */

	if (curredit & EDIT_BOTH) {
	    char           *str, *msg = "�^���ܧ@�̫H�c";

	    if ((str = strchr(quote_user, '.'))) {
		if (
#ifndef USE_BSMTP
		    bbs_sendmail(fpath, save_title, str + 1)
#else
		    bsmtp(fpath, save_title, str + 1, 0)
#endif
		    < 0)
		    msg = "�@�̵L�k���H";
	    } else {
		sethomepath(genbuf, quote_user);
		stampfile(genbuf, &postfile);
		unlink(genbuf);
		Copy(fpath, genbuf);

		strlcpy(postfile.owner, cuser.userid, sizeof(postfile.owner));
		strlcpy(postfile.title, save_title, sizeof(postfile.title));
		sethomedir(genbuf, quote_user);
		if (append_record(genbuf, &postfile, sizeof(postfile)) == -1)
		    msg = err_uid;
		else
		    mailalert(quote_user);
	    }
	    outs(msg);
	    curredit ^= EDIT_BOTH;
	}
	if (currbrdattr & BRD_ANONYMOUS)
            do_crosspost("UnAnonymous", &postfile, fpath);
#ifdef USE_COOLDOWN
        if(bp->nuser>30)
          {
	   if (cooldowntimeof(usernum)<now)
	      add_cooldowntime(usernum, 5);
           add_posttimes(usernum, 1);
          }
#endif
    }
    pressanykey();
    return FULLUPDATE;
}

int
do_post(void)
{
    boardheader_t  *bp;
    STATINC(STAT_DOPOST);
    bp = getbcache(currbid);
    if (bp->brdattr & BRD_VOTEBOARD)
	return do_voteboard(0);
    else if (!(bp->brdattr & BRD_GROUPBOARD))
	return do_general(0);
    return 0;
}

int
do_post_vote(void)
{
    return do_voteboard(1);
}

int
do_post_openbid(void)
{
    char ans[4];
    boardheader_t  *bp;

    bp = getbcache(currbid);
    if (!(bp->brdattr & BRD_VOTEBOARD))
    {
	getdata(b_lines - 1, 0,
		"�T�w�n���}�ۼжܡH [y/N] ",
		ans, sizeof(ans), LCECHO);
	if(ans[0] != 'y')
	    return FULLUPDATE;

	return do_general(1);
    }
    return 0;
}

static void
do_generalboardreply(const fileheader_t * fhdr)
{
    char            genbuf[3];
    
    if ( !((currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP)) &&
	    (cuser.firstlogin > (now - (time4_t)bcache[currbid - 1].post_limit_regtime * 2592000) ||
	    cuser.numlogins < ((unsigned int)(bcache[currbid - 1].post_limit_logins) * 10) ||
	    cuser.numposts < ((unsigned int)(bcache[currbid - 1].post_limit_posts) * 10)) ) {
	getdata(b_lines - 1, 0,	"�� �^���� (M)�@�̫H�c (Q)�����H[M] ",
		genbuf, sizeof(genbuf), LCECHO);
	switch (genbuf[0]) {
	    case 'q':
		break;
	    default:
		mail_reply(0, fhdr, 0);
	}
    }
    else {
	getdata(b_lines - 1, 0,
		"�� �^���� (F)�ݪO (M)�@�̫H�c (B)�G�̬ҬO (Q)�����H[F] ",
		genbuf, sizeof(genbuf), LCECHO);
	switch (genbuf[0]) {
	    case 'm':
		mail_reply(0, fhdr, 0);
	    case 'q':
		break;

	    case 'b':
		curredit = EDIT_BOTH;
	    default:
		strlcpy(currtitle, fhdr->title, sizeof(currtitle));
		strlcpy(quote_user, fhdr->owner, sizeof(quote_user));
		do_post();
		curredit &= ~EDIT_BOTH;
	}
    }
    *quote_file = 0;
}


int
invalid_brdname(const char *brd)
{
    register char   ch, rv=0;

    ch = *brd++;
    if (!isalpha((int)ch))
	rv =  2;
    while ((ch = *brd++)) {
	if (not_alnum(ch) && ch != '_' && ch != '-' && ch != '.')
	    return (1|rv);
    }
    return rv;
}

static int
b_call_in(int ent, const fileheader_t * fhdr, const char *direct)
{
    userinfo_t     *u = search_ulist(searchuser(fhdr->owner, NULL));
    if (u) {
	int             fri_stat;
	fri_stat = friend_stat(currutmp, u);
	if (isvisible_stat(currutmp, u, fri_stat) && call_in(u, fri_stat))
	    return FULLUPDATE;
    }
    return DONOTHING;
}


static int
b_posttype(int ent, const fileheader_t * fhdr, const char *direct)
{
   boardheader_t  *bp;
   int i, aborted;
   char filepath[256], genbuf[60], title[5], posttype_f, posttype[33]="";

   if(!(currmode & MODE_BOARD)) return DONOTHING;
   
   bp = getbcache(currbid);

   move(2,0);
   clrtobot();
   posttype_f = bp->posttype_f;
   for( i = 0 ; i < 8 ; ++i ){
       move(2,0);
       outs("�峹����:       ");
       strncpy(genbuf, bp->posttype + i * 4, 4);
       genbuf[4] = 0;
       sprintf(title, "%d.", i + 1);
       if( !getdata_buf(2, 11, title, genbuf, 5, DOECHO) )
	   break;
       sprintf(posttype + i * 4, "%-4.4s", genbuf); 
       if( posttype_f & (1<<i) ){
	   if( getdata(2, 20, "�]�w�d���榡�H(Y/n)", genbuf, 3, LCECHO) &&
	       genbuf[0]=='n' ){
	       posttype_f &= ~(1<<i);
	       continue;
	   }
       }
       else if ( !getdata(2, 20, "�]�w�d���榡�H(y/N)", genbuf, 3, LCECHO) ||
		 genbuf[0] != 'y' )
	   continue;

       setbnfile(filepath, bp->brdname, "postsample", i);
       aborted = vedit(filepath, NA, NULL);
       if (aborted == -1) {
           clear();
           posttype_f &= ~(1<<i);
           continue;
       }
       posttype_f |= (1<<i);
   }
   bp->posttype_f = posttype_f; 
   strncpy(bp->posttype, posttype, 32); /* �o�����ӭn��race condition */

   substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
   return FULLUPDATE;
}

static int
do_reply(const fileheader_t * fhdr)
{
    boardheader_t  *bp;
    if (!CheckPostPerm() ) return DONOTHING;
    if (fhdr->filemode &FILE_SOLVED)
     {
       if(fhdr->filemode & FILE_MARKED)
         {
          vmsg("�ܩ�p, ���峹�w���רüаO, ���o�^��.");
          return FULLUPDATE;
         }
       if(getkey("���g�峹�w����, �O�_�u���n�^��?(y/N)")!='y')
          return FULLUPDATE;
     }

    bp = getbcache(currbid);
    setbfile(quote_file, bp->brdname, fhdr->filename);
    if (bp->brdattr & BRD_VOTEBOARD || (fhdr->filemode & FILE_VOTE))
	do_voteboardreply(fhdr);
    else
	do_generalboardreply(fhdr);
    *quote_file = 0;
    return FULLUPDATE;
}

static int
reply_post(int ent, const fileheader_t * fhdr, const char *direct)
{
    return do_reply(fhdr);
}

static int
edit_post(int ent, fileheader_t * fhdr, const char *direct)
{
    char            fpath[80];
    char            genbuf[200];
    fileheader_t    postfile;
    boardheader_t  *bp = getbcache(currbid);

    if (strcmp(bp->brdname, "Security") == 0)
	return DONOTHING;

    if (!HasUserPerm(PERM_SYSOP) &&
	((bp->brdattr & BRD_VOTEBOARD)  ||
	 (fhdr->filemode & FILE_VOTE)   ||
	 !CheckPostPerm()               ||
	 strcmp(fhdr->owner, cuser.userid) != 0 ||
	 strcmp(cuser.userid, STR_GUEST) == 0))
	return DONOTHING;

    if( currmode & MODE_SELECT )
	return DONOTHING;

#ifdef SAFE_ARTICLE_DELETE
    if( fhdr->filename[0] == '.' )
	return DONOTHING;
#endif

    setutmpmode(REEDIT);
    setbpath(fpath, currboard);
    stampfile(fpath, &postfile);
    setdirpath(genbuf, direct, fhdr->filename);
    local_article = fhdr->filemode & FILE_LOCAL;
    Copy(genbuf, fpath);
    strlcpy(save_title, fhdr->title, sizeof(save_title));

    if (vedit(fpath, 0, NULL) != -1) {
        Rename(fpath, genbuf);
        if(strcmp(save_title, fhdr->title)){
	    // Ptt: here is the black hole problem
	    // (try getindex)
	    strcpy(fhdr->title, save_title);
	    substitute_ref_record(direct, fhdr, ent);
	}
    }
    return FULLUPDATE;
}

#define UPDATE_USEREC   (currmode |= MODE_DIRTY)

static int
cross_post(int ent, const fileheader_t * fhdr, const char *direct)
{
    char            xboard[20], fname[80], xfpath[80], xtitle[80];
    char            inputbuf[10], genbuf[200], genbuf2[4];
    fileheader_t    xfile;
    FILE           *xptr;
    int             author;
    boardheader_t  *bp;

    if (!CheckPostPerm()) {
	move(5, 10);
	outs("�藍�_�A�z�ثe�L�k����峹�I");
	pressanykey();
	return FULLUPDATE;
    }
    move(2, 0);
    clrtoeol();
    move(1, 0);
    bp = getbcache(currbid);
    if (bp && (bp->brdattr & BRD_VOTEBOARD) )
	return FULLUPDATE;
    CompleteBoard("������峹��ݪO�G", xboard);
    if (*xboard == '\0' || !haspostperm(xboard))
	return FULLUPDATE;

    /* �ɥ��ܼ� */
    ent = str_checksum(fhdr->title);
    author = getbnum(xboard);

    if ((ent != 0 && ent == postrecord.checksum[0]) &&
	(author != 0 && author != postrecord.last_bid)) {
	/* �ˬd cross post ���� */
	if (postrecord.times++ > MAX_CROSSNUM)
	    anticrosspost();
    } else {
	postrecord.times = 0;
	postrecord.last_bid = author;
	postrecord.checksum[0] = ent;
    }

    if ( !((currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP)) &&
	    (cuser.firstlogin > (now - (time4_t)bcache[author - 1].post_limit_regtime * 2592000) ||
	    cuser.numlogins < ((unsigned int)(bcache[author - 1].post_limit_logins) * 10) ||
	    cuser.numposts < ((unsigned int)(bcache[author - 1].post_limit_posts) * 10)) ) {
	move(5, 10);
	vmsg("�A������`��I");
	return FULLUPDATE;
    }

#ifdef USE_COOLDOWN
       if(check_cooldown(bp))
	  return FULLUPDATE;
#endif

    ent = 1;
    author = 0;
    if (HasUserPerm(PERM_SYSOP) || !strcmp(fhdr->owner, cuser.userid)) {
	getdata(2, 0, "(1)������ (2)������榡�H[1] ",
		genbuf, 3, DOECHO);
	if (genbuf[0] != '2') {
	    ent = 0;
	    getdata(2, 0, "�O�d��@�̦W�ٶ�?[Y] ", inputbuf, 3, DOECHO);
	    if (inputbuf[0] != 'n' && inputbuf[0] != 'N')
		author = '1';
	}
    }
    if (ent)
	snprintf(xtitle, sizeof(xtitle), "[���]%.66s", fhdr->title);
    else
	strlcpy(xtitle, fhdr->title, sizeof(xtitle));

    snprintf(genbuf, sizeof(genbuf), "�ĥέ���D�m%.60s�n��?[Y] ", xtitle);
    getdata(2, 0, genbuf, genbuf2, 4, LCECHO);
    if (genbuf2[0] == 'n' || genbuf2[0] == 'N') {
	if (getdata_str(2, 0, "���D�G", genbuf, TTLEN, DOECHO, xtitle))
	    strlcpy(xtitle, genbuf, sizeof(xtitle));
    }
    getdata(2, 0, "(S)�s�� (L)���� (Q)�����H[Q] ", genbuf, 3, LCECHO);
    if (genbuf[0] == 'l' || genbuf[0] == 's') {
	int             currmode0 = currmode;
	const char     *save_currboard;

	currmode = 0;
	setbpath(xfpath, xboard);
	stampfile(xfpath, &xfile);
	if (author)
	    strlcpy(xfile.owner, fhdr->owner, sizeof(xfile.owner));
	else
	    strlcpy(xfile.owner, cuser.userid, sizeof(xfile.owner));
	strlcpy(xfile.title, xtitle, sizeof(xfile.title));
	if (genbuf[0] == 'l') {
	    xfile.filemode = FILE_LOCAL;
	}
	setbfile(fname, currboard, fhdr->filename);
	xptr = fopen(xfpath, "w");

	strlcpy(save_title, xfile.title, sizeof(save_title));
	save_currboard = currboard;
	currboard = xboard;
	write_header(xptr, save_title);
	currboard = save_currboard;

	fprintf(xptr, "�� [��������� %s �ݪO]\n\n", currboard);

	b_suckinfile(xptr, fname);
	addsignature(xptr, 0);
	fclose(xptr);
	/*
	 * Cross fs�����D } else { unlink(xfpath); link(fname, xfpath); }
	 */
	setbdir(fname, xboard);
	append_record(fname, &xfile, sizeof(xfile));
	bp = getbcache(getbnum(xboard));
	if (!xfile.filemode && !(bp->brdattr & BRD_NOTRAN))
	    outgo_post(&xfile, xboard, cuser.userid, cuser.nickname);
#ifdef USE_COOLDOWN
        if(bp->nuser>30)
          {
           if (cooldowntimeof(usernum)<now)
              add_cooldowntime(usernum, 5);
           add_posttimes(usernum, 1);
          }
#endif
	setbtotal(getbnum(xboard));
	cuser.numposts++;
	UPDATE_USEREC;
	outs("�峹�������");
	pressanykey();
	currmode = currmode0;
    }
    return FULLUPDATE;
}

static int
read_post(int ent, fileheader_t * fhdr, const char *direct)
{
    char            genbuf[100];
    int             more_result;

    if (fhdr->owner[0] == '-')
	return READ_SKIP;

    STATINC(STAT_READPOST);
    setdirpath(genbuf, direct, fhdr->filename);

    more_result = more(genbuf, YEA);

    {
	int posttime=atoi(fhdr->filename+2);
	if(posttime>now-12*3600)
	    STATINC(STAT_READPOST_12HR);
	else if(posttime>now-1*86400)
	    STATINC(STAT_READPOST_1DAY);
	else if(posttime>now-3*86400)
	    STATINC(STAT_READPOST_3DAY);
	else if(posttime>now-7*86400)
	    STATINC(STAT_READPOST_7DAY);
	else
	    STATINC(STAT_READPOST_OLD);
    }
    brc_addlist(fhdr->filename);
    strncpy(currtitle, subject(fhdr->title), TTLEN);

    if (more_result) {
	if(more_result == -1)
		return READ_SKIP;
        if(more_result == 999) {
            return do_reply(fhdr);
	}
        if(more_result == 998) {
            recommend(ent, fhdr, direct);
	    return FULLUPDATE;
	}
        else return more_result;
    } 
    return FULLUPDATE;
}

int
do_limitedit(int ent, fileheader_t * fhdr, const char *direct)
{
    char            genbuf[256], buf[256];
    int             temp;
    boardheader_t  *bp = getbcache(currbid);

    if (!((currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP)))
	return DONOTHING;
    
    strcpy(buf, "��� ");
    if (HasUserPerm(PERM_SYSOP))
	strcat(buf, "(A)���O�o���� ");
    strcat(buf, "(B)���O�w�]");
    if (fhdr->filemode & FILE_VOTE)
	strcat(buf, " (C)���g");
    strcat(buf, "�s�p���� (Q)�����H[Q]");
    genbuf[0] = getans(buf);

    if (HasUserPerm(PERM_SYSOP) && genbuf[0] == 'a') {
	sprintf(genbuf, "%u", bp->post_limit_regtime);
	do {
	    getdata_buf(b_lines - 1, 0, "���U�ɶ����� (�H'��'�����A0~255)�G", genbuf, 4, LCECHO);
	    temp = atoi(genbuf);
	} while (temp < 0 || temp > 255);
	bp->post_limit_regtime = (unsigned char)temp;
	
	sprintf(genbuf, "%u", bp->post_limit_logins * 10);
	do {
	    getdata_buf(b_lines - 1, 0, "�W�����ƤU�� (0~2550)�G", genbuf, 5, LCECHO);
	    temp = atoi(genbuf);
	} while (temp < 0 || temp > 2550);
	bp->post_limit_logins = (unsigned char)(temp / 10);
	
	sprintf(genbuf, "%u", bp->post_limit_posts * 10);
	do {
	    getdata_buf(b_lines - 1, 0, "�峹�g�ƤU�� (0~2550)�G", genbuf, 5, LCECHO);
	    temp = atoi(genbuf);
	} while (temp < 0 || temp > 2550);
	bp->post_limit_posts = (unsigned char)(temp / 10);
	substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
	log_usies("SetBoard", bp->brdname);
	vmsg("�ק粒���I");
	return FULLUPDATE;
    }
    else if (genbuf[0] == 'b') {
	sprintf(genbuf, "%u", bp->vote_limit_regtime);
	do {
	    getdata_buf(b_lines - 1, 0, "���U�ɶ����� (�H'��'�����A0~255)�G", genbuf, 4, LCECHO);
	    temp = atoi(genbuf);
	} while (temp < 0 || temp > 255);
	bp->vote_limit_regtime = (unsigned char)temp;
	
	sprintf(genbuf, "%u", bp->vote_limit_logins * 10);
	do {
	    getdata_buf(b_lines - 1, 0, "�W�����ƤU�� (0~2550)�G", genbuf, 5, LCECHO);
	    temp = atoi(genbuf);
	} while (temp < 0 || temp > 2550);
	bp->vote_limit_logins = (unsigned char)(temp / 10);
	
	sprintf(genbuf, "%u", bp->vote_limit_posts * 10);
	do {
	    getdata_buf(b_lines - 1, 0, "�峹�g�ƤU�� (0~2550)�G", genbuf, 5, LCECHO);
	    temp = atoi(genbuf);
	} while (temp < 0 || temp > 2550);
	bp->vote_limit_posts = (unsigned char)(temp / 10);
	substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
	log_usies("SetBoard", bp->brdname);
	vmsg("�ק粒���I");
	return FULLUPDATE;
    }
    else if ((fhdr->filemode & FILE_VOTE) && genbuf[0] == 'c') {
	sprintf(genbuf, "%u", fhdr->multi.vote_limits.regtime);
	do {
	    getdata_buf(b_lines - 1, 0, "���U�ɶ����� (�H'��'�����A0~255)�G", genbuf, 4, LCECHO);
	    temp = atoi(genbuf);
	} while (temp < 0 || temp > 255);
	fhdr->multi.vote_limits.regtime = temp;
	
	sprintf(genbuf, "%u", (unsigned int)(fhdr->multi.vote_limits.logins) * 10);
	do {
	    getdata_buf(b_lines - 1, 0, "�W�����ƤU�� (0~2550)�G", genbuf, 5, LCECHO);
	    temp = atoi(genbuf);
	} while (temp < 0 || temp > 2550);
	temp /= 10;
	fhdr->multi.vote_limits.logins = (unsigned char)temp;
	
	sprintf(genbuf, "%u", (unsigned int)(fhdr->multi.vote_limits.posts) * 10);
	do {
	    getdata_buf(b_lines - 1, 0, "�峹�g�ƤU�� (0~2550)�G", genbuf, 5, LCECHO);
	    temp = atoi(genbuf);
	} while (temp < 0 || temp > 2550);
	temp /= 10;
	fhdr->multi.vote_limits.posts = (unsigned char)temp;
	substitute_ref_record(direct, fhdr, ent);
	vmsg("�ק粒���I");
	return FULLUPDATE;
    }
    vmsg("�����ק�");
    return FULLUPDATE;
}

/* ----------------------------------------------------- */
/* �Ķ���ذ�                                            */
/* ----------------------------------------------------- */
static int
b_man(void)
{
    char            buf[PATHLEN];

    setapath(buf, currboard);
    if ((currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP)) {
	char            genbuf[128];
	int             fd;
	snprintf(genbuf, sizeof(genbuf), "%s/.rebuild", buf);
	if ((fd = open(genbuf, O_CREAT, 0640)) > 0)
	    close(fd);
    }
    return a_menu(currboard, buf, HasUserPerm(PERM_ALLBOARD) ? 2 :
		  (currmode & MODE_BOARD ? 1 : 0),
		  NULL);
}

#ifndef NO_GAMBLE
static int
stop_gamble(void)
{
    boardheader_t  *bp = getbcache(currbid);
    char            fn_ticket[128], fn_ticket_end[128];
    if (!bp->endgamble || bp->endgamble > now)
	return 0;

    setbfile(fn_ticket, currboard, FN_TICKET);
    setbfile(fn_ticket_end, currboard, FN_TICKET_END);

    rename(fn_ticket, fn_ticket_end);
    if (bp->endgamble) {
	bp->endgamble = 0;
	substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
    }
    return 1;
}
static int
join_gamble(int ent, const fileheader_t * fhdr, const char *direct)
{
    if (!HasUserPerm(PERM_LOGINOK))
	return DONOTHING;
    if (stop_gamble()) {
	vmsg("�ثe���|���L�ν�L�w�}��");
	return DONOTHING;
    }
    ticket(currbid);
    return FULLUPDATE;
}
static int
hold_gamble(int ent, const fileheader_t * fhdr, const char *direct)
{
    char            fn_ticket[128], fn_ticket_end[128], genbuf[128], msg[256] = "",
                    yn[10] = "";
    boardheader_t  *bp = getbcache(currbid);
    int             i;
    FILE           *fp = NULL;

    if (!(currmode & MODE_BOARD))
	return 0;
    setbfile(fn_ticket, currboard, FN_TICKET);
    setbfile(fn_ticket_end, currboard, FN_TICKET_END);
    setbfile(genbuf, currboard, FN_TICKET_LOCK);
    if (dashf(fn_ticket)) {
	getdata(b_lines - 1, 0, "�w�g���|���L, "
		"�O�_�n [����U�`]?(N/y)�G", yn, 3, LCECHO);
	if (yn[0] != 'y')
	    return FULLUPDATE;
	rename(fn_ticket, fn_ticket_end);
	if (bp->endgamble) {
	    bp->endgamble = 0;
	    substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);

	}
	return FULLUPDATE;
    }
    if (dashf(fn_ticket_end)) {
	getdata(b_lines - 1, 0, "�w�g���|���L, "
		"�O�_�n�}�� [�_/�O]?(N/y)�G", yn, 3, LCECHO);
	if (yn[0] != 'y')
	    return FULLUPDATE;
	openticket(currbid);
	return FULLUPDATE;
    } else if (dashf(genbuf)) {
	vmsg(" �ثe�t�Υ��b�B�z�}���Ʃy, �е��G�X�l��A�|��.......");
	return FULLUPDATE;
    }
    getdata(b_lines - 2, 0, "�n�|���L (N/y):", yn, 3, LCECHO);
    if (yn[0] != 'y')
	return FULLUPDATE;
    getdata(b_lines - 1, 0, "�䤰��? �п�J�D�D (��J��s�褺�e):",
	    msg, 20, DOECHO);
    if (msg[0] == 0 ||
	vedit(fn_ticket_end, NA, NULL) < 0)
	return FULLUPDATE;

    clear();
    showtitle("�|���L", BBSNAME);
    setbfile(genbuf, currboard, FN_TICKET_ITEMS);

    //sprintf(genbuf, "%s/" FN_TICKET_ITEMS, direct);

    if (!(fp = fopen(genbuf, "w")))
	return FULLUPDATE;
    do {
	getdata(2, 0, "��J�m������ (����:10-10000):", yn, 6, LCECHO);
	i = atoi(yn);
    } while (i < 10 || i > 10000);
    fprintf(fp, "%d\n", i);
    if (!getdata(3, 0, "�]�w�۰ʫʽL�ɶ�?(Y/n)", yn, 3, LCECHO) || yn[0] != 'n') {
	bp->endgamble = gettime(4, now, "�ʽL��");
	substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
    }
    move(6, 0);
    snprintf(genbuf, sizeof(genbuf),
	     "�Ш� %s �O ��'f'�ѻP���!\n\n�@�i %d Ptt��, �o�O%s�����\n%s%s\n",
	     currboard,
	     i, i < 100 ? "�p�䦡" : i < 500 ? "������" :
	     i < 1000 ? "�Q�گ�" : i < 5000 ? "�I����" : "�ɮa����",
	     bp->endgamble ? "��L�����ɶ�: " : "",
	     bp->endgamble ? Cdate(&bp->endgamble) : ""
	     );
    strcat(msg, genbuf);
    outs("�Ш̦���J�m���W��, �ݴ���2~8��. (�����K��, ��J������enter)\n");
    for( i = 0 ; i < 8 ; ++i ){
	snprintf(yn, sizeof(yn), " %d)", i + 1);
	getdata(7 + i, 0, yn, genbuf, 9, DOECHO);
	if (!genbuf[0] && i > 1)
	    break;
	fprintf(fp, "%s\n", genbuf);
    }
    fclose(fp);

    setbfile(genbuf, currboard, FN_TICKET_RECORD);
    unlink(genbuf); // Ptt: �����Q�Τ��Pid�P���|����
    setbfile(genbuf, currboard, FN_TICKET_USER);
    unlink(genbuf); // Ptt: �����Q�Τ��Pid�P���|����

    move(8 + i, 0);
    outs("��L�]�w����");
    snprintf(genbuf, sizeof(genbuf), "[���i] %s �O �}�l���!", currboard);
    post_msg(currboard, genbuf, msg, cuser.userid);
    post_msg("Record", genbuf + 7, msg, "[�������l]");
    /* Tim ����CS, �H�K���b����user���Ƥw�g�g�i�� */
    rename(fn_ticket_end, fn_ticket);
    /* �]�w���~���ɦW��L�� */

    return FULLUPDATE;
}
#endif

static int
cite_post(int ent, const fileheader_t * fhdr, const char *direct)
{
    char            fpath[256];
    char            title[TTLEN + 1];

    setbfile(fpath, currboard, fhdr->filename);
    strlcpy(title, "�� ", sizeof(title));
    strlcpy(title + 3, fhdr->title, TTLEN - 3);
    title[TTLEN] = '\0';
    a_copyitem(fpath, title, 0, 1);
    b_man();
    return FULLUPDATE;
}

int
edit_title(int ent, fileheader_t * fhdr, const char *direct)
{
    char            genbuf[200];
    fileheader_t    tmpfhdr = *fhdr;
    int             dirty = 0;

    if (currmode & MODE_BOARD || !strcmp(cuser.userid, fhdr->owner)) {
	if (getdata(b_lines - 1, 0, "���D�G", genbuf, TTLEN, DOECHO)) {
	    strlcpy(tmpfhdr.title, genbuf, sizeof(tmpfhdr.title));
	    dirty++;
	}
    }
    if (HasUserPerm(PERM_SYSOP)) {
	if (getdata(b_lines - 1, 0, "�@�̡G", genbuf, IDLEN + 2, DOECHO)) {
	    strlcpy(tmpfhdr.owner, genbuf, sizeof(tmpfhdr.owner));
	    dirty++;
	}
	if (getdata(b_lines - 1, 0, "����G", genbuf, 6, DOECHO)) {
	    snprintf(tmpfhdr.date, sizeof(tmpfhdr.date), "%.5s", genbuf);
	    dirty++;
	}
    }
    if (currmode & MODE_BOARD || !strcmp(cuser.userid, fhdr->owner)) {
	getdata(b_lines - 1, 0, "�T�w(Y/N)?[n] ", genbuf, 3, DOECHO);
	if ((genbuf[0] == 'y' || genbuf[0] == 'Y') && dirty) {
	    *fhdr = tmpfhdr;
	    substitute_ref_record(direct, fhdr, ent);
	}
	return FULLUPDATE;
    }
    return DONOTHING;
}

static int
solve_post(int ent, fileheader_t * fhdr, const char *direct)
{
    if ((currmode & MODE_BOARD)) {
	fhdr->filemode ^= FILE_SOLVED;
        substitute_ref_record(direct, fhdr, ent);
	return PART_REDRAW;
    }
    return DONOTHING;
}


static int
recommend_cancel(int ent, fileheader_t * fhdr, const char *direct)
{
    char            yn[5];
    if (!(currmode & MODE_BOARD))
	return DONOTHING;
    getdata(b_lines - 1, 0, "�T�w�n�����k�s(Y/N)?[n] ", yn, 5, LCECHO);
    if (yn[0] != 'y')
	return FULLUPDATE;
#ifdef ASSESS
    // to save resource
    if (fhdr->recommend > 9)
	inc_goodpost(fhdr->owner, -1 * (fhdr->recommend / 10));
#endif
    fhdr->recommend = 0;

    substitute_ref_record(direct, fhdr, ent);
    return FULLUPDATE;
}

static int
do_add_recommend(const char *direct, fileheader_t *fhdr,
		 int ent, const char *buf, int type)
{
    char    path[256];
    int     update = 0;
    /*
      race here:
      ���F��� system calls , �{�b�����η�e������� +1 �g�J .DIR ��.
      �y��
      1.�Y�Ӥ��ɦW�Q��������, ����N�g�����ɦW�� (�y�����F��)
      2.�S�����sŪ�@��, �ҥH����ƥi��Q�ֺ�
      3.�Y�����ɭԫe��Q�R, �N�[���媺�����

     */
    setdirpath(path, direct, fhdr->filename);
    if( log_file(path, 0, buf) == -1 ){ // �� CREATE
	vmsg("����/�v�Х���");
	return -1;
    }

    /* This is a solution to avoid most racing (still some), but cost four
     * system calls.                                                        */
    if(type == 0 && fhdr->recommend < 100 )
          update = 1;
    else if(type == 1 && fhdr->recommend > -100)
          update = -1;
    
    if( update ){
        int fd;
        //Ptt: update only necessary
	if( (fd = open(direct, O_RDWR)) < 0 )
	    return -1;
	if( lseek(fd, (sizeof(fileheader_t) * (ent - 1) +
		       (char *)&fhdr->recommend - (char *)fhdr),
		  SEEK_SET) >= 0 ){
	    // �p�G lseek ���ѴN���| write
            read(fd, &fhdr->recommend, sizeof(char));
            fhdr->recommend += update;
            lseek(fd, -1, SEEK_CUR);
	    write(fd, &fhdr->recommend, sizeof(char));
	}
	close(fd);
    }
    return 0;
}

static int
do_bid(int ent, fileheader_t * fhdr, const boardheader_t  *bp,
       const char *direct,  const struct tm *ptime)
{
    char            genbuf[200], fpath[256],say[30],*money;
    bid_t           bidinfo;
    int             mymax, next;

    setdirpath(fpath, direct, fhdr->filename);
    strcat(fpath, ".bid");
    get_record(fpath, &bidinfo, sizeof(bidinfo), 1);

    move(18,0);
    clrtobot();
    prints("�v�ХD�D: %s\n", fhdr->title);
    print_bidinfo(0, bidinfo);
    money = bidinfo.payby ? " NT$ " : "Ptt$ ";
    if( now > bidinfo.enddate || bidinfo.high == bidinfo.buyitnow ){
	outs("���v�Фw�g����,");
	if( bidinfo.userid[0] ) {
	    /*if(!payby && bidinfo.usermax!=-1)
	      {�HPtt���۰ʦ���
	      }*/
	    prints("���� %s �H %d �o��!", bidinfo.userid, bidinfo.high);
#ifdef ASSESS
	    if (!(bidinfo.flag & SALE_COMMENTED) && strcmp(bidinfo.userid, currutmp->userid) == 0){
		char tmp = getans("�z���o������������p��? 1:�� 2:��� 3:���q[Q]");
		if ('1' <= tmp && tmp <= '3'){
		    switch(tmp){
			case 1:
			    inc_goodsale(bidinfo.userid, 1);
			    break;
			case 2:
			    inc_badsale(bidinfo.userid, 1);
			    break;
		    }
		    bidinfo.flag |= SALE_COMMENTED;
                    
		    substitute_record(fpath, &bidinfo, sizeof(bidinfo), 1);
		}
	    }
#endif
	}
	else outs("�L�H�o��!");
	pressanykey();
	return FULLUPDATE;
    }

    if( bidinfo.userid[0] ){
        prints("�U���X���ܤ֭n:%s%d", money,bidinfo.high + bidinfo.increment);
	if( bidinfo.buyitnow )
	     prints(" (��J %d ����H�����ʶR����)",bidinfo.buyitnow);
	next = bidinfo.high + bidinfo.increment;
    }
    else{
        prints("�_�л�: %d", bidinfo.high);
	next=bidinfo.high;
    }
    if( !strcmp(cuser.userid,bidinfo.userid) ){
	outs("�A�O�̰��o�Ъ�!");
        pressanykey();
	return FULLUPDATE;
    }
    if( strcmp(cuser.userid, fhdr->owner) == 0 ){
	vmsg("ĵ�i! ���H����X��!");
	getdata_str(23, 0, "�O�_�n��������? (y/N)", genbuf, 3, LCECHO,"n");
	if( genbuf[0] != 'y' )
	    return FULLUPDATE;
	snprintf(genbuf, sizeof(genbuf),
		ANSI_COLOR(1;31) "�� "
		ANSI_COLOR(33) "���%s��������"
		ANSI_RESET "%*s"
		"��%15s %02d/%02d\n",
		cuser.userid, (int)(45 - strlen(cuser.userid) - strlen(money)),
		" ", fromhost, ptime->tm_mon + 1, ptime->tm_mday);
	do_add_recommend(direct, fhdr,  ent, genbuf, 0);
	bidinfo.enddate = now;
	substitute_record(fpath, &bidinfo, sizeof(bidinfo), 1);
	vmsg("�������Ч���");
	return FULLUPDATE;
    }
    getdata_str(23, 0, "�O�_�n�U��? (y/N)", genbuf, 3, LCECHO,"n");
    if( genbuf[0] != 'y' )
	return FULLUPDATE;

    getdata(23, 0, "�z���̰��U�Ъ��B(0:����):", genbuf, 10, LCECHO);
    mymax = atoi(genbuf);
    if( mymax <= 0 ){
	vmsg("�����U��");
        return FULLUPDATE;
    }

    getdata(23,0,"�U�зP��:",say,12,DOECHO);
    get_record(fpath, &bidinfo, sizeof(bidinfo), 1);

    if( bidinfo.buyitnow && mymax > bidinfo.buyitnow )
        mymax = bidinfo.buyitnow;
    else if( !bidinfo.userid[0] )
	next = bidinfo.high;
    else
	next = bidinfo.high + bidinfo.increment;

    if( mymax< next || (bidinfo.payby == 0 && cuser.money < mymax) ){
	vmsg("�Ъ������m��");
        return FULLUPDATE;
    }
    
    snprintf(genbuf, sizeof(genbuf),
	     ANSI_COLOR(1;31) "�� " ANSI_COLOR(33) "%s" ANSI_RESET ANSI_COLOR(33) ":%s" ANSI_RESET "%*s"
	     "%s%-15d��%15s %02d/%02d\n",
	     cuser.userid, say,
	     (int)(31 - strlen(cuser.userid) - strlen(say)), " ", 
             money,
	     next, fromhost,
	     ptime->tm_mon + 1, ptime->tm_mday);
    do_add_recommend(direct, fhdr,  ent, genbuf, 0);
    if( next > bidinfo.usermax ){
	bidinfo.usermax = mymax;
	bidinfo.high = next;
	strcpy(bidinfo.userid, cuser.userid);
    }
    else if( mymax > bidinfo.usermax ) {
	bidinfo.high = bidinfo.usermax + bidinfo.increment;
        if( bidinfo.high > mymax )
	    bidinfo.high = mymax; 
	bidinfo.usermax = mymax;
        strcpy(bidinfo.userid, cuser.userid);
	
        snprintf(genbuf, sizeof(genbuf),
		 ANSI_COLOR(1;31) "�� " ANSI_COLOR(33) "�۰��v��%s�ӥX" ANSI_RESET
		 ANSI_COLOR(33) ANSI_RESET "%*s%s%-15d�� %02d/%02d\n",
		 cuser.userid, 
		 (int)(20 - strlen(cuser.userid)), " ", money, 
		 bidinfo.high, 
		 ptime->tm_mon + 1, ptime->tm_mday);
        do_add_recommend(direct, fhdr,  ent, genbuf, 0);
    }
    else {
	if( mymax + bidinfo.increment < bidinfo.usermax )
	    bidinfo.high = mymax + bidinfo.increment;
	 else
	     bidinfo.high=bidinfo.usermax; /*�o��ǩǪ�*/ 
        snprintf(genbuf, sizeof(genbuf),
		 ANSI_COLOR(1;31) "�� " ANSI_COLOR(33) "�۰��v��%s�ӥX"
		 ANSI_RESET ANSI_COLOR(33) ANSI_RESET "%*s%s%-15d�� %02d/%02d\n",
		 bidinfo.userid, 
		 (int)(20 - strlen(bidinfo.userid)), " ", money, 
		 bidinfo.high,
		 ptime->tm_mon + 1, ptime->tm_mday);
        do_add_recommend(direct, fhdr, ent, genbuf, 0);
    }
    substitute_record(fpath, &bidinfo, sizeof(bidinfo), 1);
    vmsg("���߱z! �H�̰����m�Ч���!");
    return FULLUPDATE;
}

static int
recommend(int ent, fileheader_t * fhdr, const char *direct)
{
    struct tm      *ptime = localtime4(&now);
    char            buf[200], msg[53];
    static const char *ctype[3] = {
		       "��", "�N", "��"
		   }, *ctype_attr[3] = {
		       ANSI_COLOR(1;33),
		       ANSI_COLOR(1;31),
		       ANSI_COLOR(1;37),
		   }, *ctype_attr2[3] = {
		       ANSI_COLOR(1;37),
		       ANSI_COLOR(1;31),
		       ANSI_COLOR(1;31),
		   }, *ctype_long[3] = {
		       "�ȱo����",
		       "�����N�n",
		       "�u�[����"
		   };
    int             type, maxlength;
    boardheader_t  *bp;
    static time4_t  lastrecommend = 0;

    bp = getbcache(currbid);
    if (bp->brdattr & BRD_NORECOMMEND || 
        ((fhdr->filemode & FILE_MARKED) && (fhdr->filemode & FILE_SOLVED))) {
	vmsg("��p, �T����˩��v��");
	return FULLUPDATE;
    }
    if (!CheckPostPerm() || bp->brdattr & BRD_VOTEBOARD || fhdr->filemode & FILE_VOTE) {
	vmsg("�z�v������, �L�k����!");
	return FULLUPDATE;
    }
#ifdef SAFE_ARTICLE_DELETE
    if (fhdr->filename[0] == '.') {
	vmsg("����w�R��");
	return FULLUPDATE;
    }
#endif

    if( fhdr->filemode & FILE_BID){
	return do_bid(ent, fhdr, bp, direct, ptime);
    }

    type = 0;

    /* clear screen */
    move(b_lines-3, 0);
    clrtobot();
    outs(MSG_SEPERATOR);

#ifndef OLDRECOMMEND
    if (!(bp->brdattr & BRD_NOBOO))
    {
	move(b_lines-2, 0);
	outs(ANSI_COLOR(1)  "�zı�o�o�g�峹 ");
	prints("%s1.%s %s2.%s %s3.%s " ANSI_RESET "[1]? ",
		ctype_attr[0], ctype_long[0],
		ctype_attr[1], ctype_long[1],
		ctype_attr[2], ctype_long[2]);
	// poor BBS term has problem positioning with ANSI.
	move(b_lines-2, 53); 
	type = igetch() - '1';
	if(type < 0 || type > 2)
	    type = 0;
    }
#endif

    if (fhdr->recommend == 0 && strcmp(cuser.userid, fhdr->owner) == 0 &&
	    type != 2) {
	mouts(b_lines-1, 0, "���H���˩μN�Ĥ@��, ��H �� �[���覡");
        type = 2;
    }
#ifndef DEBUG
    if (!(currmode & MODE_BOARD)&& now - lastrecommend < 90 && type != 2) {
	mouts(b_lines-1, 0,"���ˮɶ��Ӫ�, ��H �� �[���覡");
        type = 2;
    }
#endif
    if(type > 2 || type < 0)
	type = 0;

#ifdef OLDRECOMMEND
    maxlength = 51 - strlen(cuser.userid);
    strcpy(buf, "�n������: ");
#else
    maxlength = 53 - strlen(cuser.userid);
    strcpy(buf, ctype_long[type]);
    strcat(buf, ": ");
#endif

    if (!getdata(b_lines-2, 0, buf, msg, maxlength, DOECHO))
	return FULLUPDATE;

    if(getans("�T�w�n%s��? �ХJ�ӦҼ{[y/N]: ", 
		ctype[type]) != 'y')
	return FULLUPDATE;

    STATINC(STAT_RECOMMEND);

#ifdef OLDRECOMMEND
    snprintf(buf, sizeof(buf),
	     ANSI_COLOR(1;31) "�� " ANSI_COLOR(33) "%s" 
	     ANSI_RESET ANSI_COLOR(33) ":%-*s" ANSI_RESET
	     "��%15s %02d/%02d\n",
	     cuser.userid, maxlength, msg,
	     fromhost, ptime->tm_mon + 1, ptime->tm_mday);
#else
    snprintf(buf, sizeof(buf),
	     "%s%s " ANSI_COLOR(33) "%s" ANSI_RESET ANSI_COLOR(33) 
	     ":%-*s" ANSI_RESET "%15s %02d/%02d\n",
             ctype_attr2[type], ctype[type],
	     cuser.userid, 
	     maxlength,
             msg,
             fromhost,
	     ptime->tm_mon + 1, ptime->tm_mday);
#endif
    do_add_recommend(direct, fhdr,  ent, buf, type);

#ifdef ASSESS
    /* �C 10 ������ �[�@�� goodpost */
    if (type ==0 && (fhdr->filemode & FILE_MARKED) && fhdr->recommend % 10 == 0)
	inc_goodpost(fhdr->owner, 1);
#endif
    lastrecommend = now;
    return FULLUPDATE;
}

static int
mark_post(int ent, fileheader_t * fhdr, const char *direct)
{
    char buf[STRLEN], fpath[STRLEN];

    if (!(currmode & MODE_BOARD))
	return DONOTHING;

    setbpath(fpath, currboard);
    sprintf(buf, "%s/%s", fpath, fhdr->filename);

    if( !(fhdr->filemode & FILE_MARKED) && /* �Y�ثe�٨S�� mark �~�n check */
	access(buf, F_OK) < 0 )
	return DONOTHING;

    fhdr->filemode ^= FILE_MARKED;

#ifdef ASSESS
    if (!(fhdr->filemode & FILE_BID)){
	if (fhdr->filemode & FILE_MARKED) {
	    if (!(currbrdattr & BRD_BAD) && fhdr->recommend >= 10)
		inc_goodpost(fhdr->owner, fhdr->recommend / 10);
	}
	else if (fhdr->recommend > 9)
    	    inc_goodpost(fhdr->owner, -1 * (fhdr->recommend / 10));
    }
#endif
 
    substitute_ref_record(direct, fhdr, ent);
    return PART_REDRAW;
}

int
del_range(int ent, const fileheader_t *fhdr, const char *direct)
{
    char            num1[8], num2[8];
    int             inum1, inum2;
    boardheader_t  *bp = NULL;

    /* ���T�ر��p�|�i�o��, �H��, �ݪO, ��ذ� */
    if( !(direct[0] == 'h') ){ /* �H�󤣥� check */
        bp = getbcache(currbid);
	if (strcmp(bp->brdname, "Security") == 0)
	    return DONOTHING;
    }

    /* rocker.011018: �걵�Ҧ��U�٬O�����\�R������n */
    if (currmode & MODE_SELECT) {
	vmsg("�Х��^�쥿�`�Ҧ���A�i��R��...");
	return FULLUPDATE;
    }

    if ((currstat != READING) || (currmode & MODE_BOARD)) {
	getdata(1, 0, "[�]�w�R���d��] �_�I�G", num1, 6, DOECHO);
	inum1 = atoi(num1);
	if (inum1 <= 0) {
	    vmsg("�_�I���~");
	    return FULLUPDATE;
	}
	getdata(1, 28, "���I�G", num2, 6, DOECHO);
	inum2 = atoi(num2);
	if (inum2 < inum1) {
	    vmsg("���I���~");
	    return FULLUPDATE;
	}
	getdata(1, 48, msg_sure_ny, num1, 3, LCECHO);
	if (*num1 == 'y') {
	    outmsg("�B�z��,�еy��...");
	    refresh();
#ifdef SAFE_ARTICLE_DELETE
	    if(bp && !(currmode & MODE_DIGEST) && bp->nuser > 30 )
		safe_article_delete_range(direct, inum1, inum2);
	    else
		delete_range(direct, inum1, inum2);
#else
	    delete_range(direct, inum1, inum2);
#endif
	    fixkeep(direct, inum1);

	    if (currmode & MODE_BOARD) // Ptt:update cache
		setbtotal(currbid);
            else if(currstat == RMAIL)
                setupmailusage();

	    return DIRCHANGED;
	}
	return FULLUPDATE;
    }
    return DONOTHING;
}

static int
del_post(int ent, fileheader_t * fhdr, char *direct)
{
    char            genbuf[100], newpath[256];
    int             not_owned, tusernum;
    boardheader_t  *bp;

    bp = getbcache(currbid);

    if(fhdr->filemode & FILE_ANONYMOUS)
                /* When the file is anonymous posted, fhdr->multi.anon_uid is author.
                 * see do_general() */
        tusernum = fhdr->multi.anon_uid;
    else
        tusernum = searchuser(fhdr->owner, NULL);

    if (strcmp(bp->brdname, "Security") == 0)
	return DONOTHING;
    if ((fhdr->filemode & FILE_BOTTOM) || 
       (fhdr->filemode & FILE_MARKED) || (fhdr->filemode & FILE_DIGEST) ||
	(fhdr->owner[0] == '-'))
	return DONOTHING;

    not_owned = (tusernum == usernum ? 0: 1);
    if ((!(currmode & MODE_BOARD) && not_owned) ||
	((bp->brdattr & BRD_VOTEBOARD) && !HasUserPerm(PERM_SYSOP)) ||
	!strcmp(cuser.userid, STR_GUEST))
	return DONOTHING;

    if (currmode & MODE_SELECT) { 
        vmsg("�Ц^��@��Ҧ��A�R���峹");
        return DONOTHING;
    }
    getdata(1, 0, msg_del_ny, genbuf, 3, LCECHO);
    if (genbuf[0] == 'y') {
	if(
#ifdef SAFE_ARTICLE_DELETE
	   (bp->nuser>30 && !(currmode & MODE_DIGEST) &&
            !safe_article_delete(ent, fhdr, direct)) ||
#endif
	   !delete_record(direct, sizeof(fileheader_t), ent)
	   ) {

	    cancelpost(fhdr, not_owned, newpath);
#ifdef ASSESS
#define SIZE	sizeof(badpost_reason) / sizeof(char *)

	    if (not_owned && tusernum > 0 && !(currmode & MODE_DIGEST)) {
                getdata(1, 40, "�c�H�峹?(y/N)", genbuf, 3, LCECHO);
		if(genbuf[0]=='y') {
		    int i;
		    char *userid=getuserid(tusernum);
		    move(b_lines - 2, 0);
		    for (i = 0; i < SIZE; i++)
			prints("%d.%s ", i + 1, badpost_reason[i]);
		    prints("%d.%s", i + 1, "��L");
		    getdata(b_lines - 1, 0, "�п��[0:�����H��]:", genbuf, 3, LCECHO);
		    i = genbuf[0] - '1';
		    if (i >= 0 && i < SIZE)
		        sprintf(genbuf,"�H��h�^(%s)", badpost_reason[i]);
                    else if(i==SIZE)
                       {
		        strcpy(genbuf,"�H��h�^(");
		        getdata_buf(b_lines, 0, "�п�J��]", genbuf+9, 
                                 50, DOECHO);
                        strcat(genbuf,")");
                       }
                    if(i>=0 && i <= SIZE)
		    {
                      strncat(genbuf, fhdr->title, 64-strlen(genbuf)); 

#ifdef USE_COOLDOWN
                      add_cooldowntime(tusernum, 60);
                      add_posttimes(tusernum, 15); //Ptt: �ᵲ post for 1 hour
#endif

		      if (!(inc_badpost(userid, 1) % 5)){
                        userec_t xuser;
			post_violatelaw(userid, "Ptt �t��ĵ��", "�H��֭p 5 �g", "�@��@�i");
			mail_violatelaw(userid, "Ptt �t��ĵ��", "�H��֭p 5 �g", "�@��@�i");
                        kick_all(userid);
                        passwd_query(tusernum, &xuser);
                        xuser.money = moneyof(tusernum);
                        xuser.vl_count++;
		        xuser.userlevel |= PERM_VIOLATELAW;
			passwd_update(tusernum, &xuser);
		       }
		       mail_id(userid, genbuf, newpath, cuser.userid);
		   }
                }
	    }
#undef SIZE
#endif

	    setbtotal(currbid);
	    if (fhdr->multi.money < 0 || fhdr->filemode & FILE_ANONYMOUS)
		fhdr->multi.money = 0;
	    if (not_owned && tusernum && fhdr->multi.money > 0 &&
		strcmp(currboard, "Test")) {
		deumoney(tusernum, -fhdr->multi.money);
#ifdef USE_COOLDOWN
		if (bp->brdattr & BRD_COOLDOWN)
		    add_cooldowntime(tusernum, 15);
#endif
	    }
	    if (!not_owned && strcmp(currboard, "Test")) {
		if (cuser.numposts)
		    cuser.numposts--;
		if (!(currmode & MODE_DIGEST && currmode & MODE_BOARD)){
		    demoney(-fhdr->multi.money);
		    vmsg("�z���峹� %d �g�A��I�M��O %d ��", 
			    cuser.numposts, fhdr->multi.money);
		}
	    }
	    return DIRCHANGED;
	}
    }
    return FULLUPDATE;
}

static int  // Ptt: �ץ��Y��   
show_filename(int ent, const fileheader_t * fhdr, const char *direct)
{
    if(!HasUserPerm(PERM_SYSOP)) return DONOTHING;

    vmsg("�ɮצW��: %s ", fhdr->filename);
    return PART_REDRAW;
}

static int
view_postmoney(int ent, const fileheader_t * fhdr, const char *direct)
{
    if(currmode & MODE_SELECT){
	vmsg("�Цb���}�ثe����ܼҦ��A�d��");
	return FULLUPDATE;
    }
    if(fhdr->filemode & FILE_BOTTOM)
	/* donothing because substitute_ref_record forgot to update multi.money */
	vmsg("�m�����峹�ܭ��n�A�O�o�ݴN�n�F�A�O�z��������");
    else if(fhdr->filemode & FILE_ANONYMOUS)
	/* When the file is anonymous posted, fhdr->multi.anon_uid is author.
	 * see do_general() */
	vmsg("�ΦW�޲z�s��: %d (�P�@�H���X�|�@��)",
		fhdr->multi.anon_uid + (int)currutmp->pid);
    else
	vmsg("�o�@�g�峹�� %d ��", fhdr->multi.money);
    return FULLUPDATE;
}

#ifdef OUTJOBSPOOL
/* �ݪO�ƥ� */
static int
tar_addqueue(int ent, const fileheader_t * fhdr, const char *direct)
{
    char            email[60], qfn[80], ans[2];
    FILE           *fp;
    char            bakboard, bakman;
    clear();
    showtitle("�ݪO�ƥ�", BBSNAME);
    move(2, 0);
    if (!((currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP))) {
	move(5, 10);
	outs("�p�n�O�O�D�άO�����~������� -.-\"\"");
	pressanykey();
	return FULLUPDATE;
    }
    snprintf(qfn, sizeof(qfn), BBSHOME "/jobspool/tarqueue.%s", currboard);
    if (access(qfn, 0) == 0) {
	outs("�w�g�Ʃw��{, �y��|�i��ƥ�");
	pressanykey();
	return FULLUPDATE;
    }
    if (!getdata(4, 0, "�п�J�ت��H�c�G", email, sizeof(email), DOECHO))
	return FULLUPDATE;

    /* check email -.-"" */
    if (strstr(email, "@") == NULL || strstr(email, ".bbs@") != NULL) {
	move(6, 0);
	outs("�z���w���H�c�����T! ");
	pressanykey();
	return FULLUPDATE;
    }
    getdata(6, 0, "�n�ƥ��ݪO���e��(Y/N)?[Y]", ans, sizeof(ans), LCECHO);
    bakboard = (ans[0] == 'n' || ans[0] == 'N') ? 0 : 1;
    getdata(7, 0, "�n�ƥ���ذϤ��e��(Y/N)?[N]", ans, sizeof(ans), LCECHO);
    bakman = (ans[0] == 'y' || ans[0] == 'Y') ? 1 : 0;
    if (!bakboard && !bakman) {
	move(8, 0);
	outs("�i�O�ڭ̥u��ƥ��ݪO�κ�ذϪ��C ^^\"\"\"");
	pressanykey();
	return FULLUPDATE;
    }
    fp = fopen(qfn, "w");
    fprintf(fp, "%s\n", cuser.userid);
    fprintf(fp, "%s\n", email);
    fprintf(fp, "%d,%d\n", bakboard, bakman);
    fclose(fp);

    move(10, 0);
    outs("�t�Τw�g�N�z���ƥ��ƤJ��{, \n");
    outs("�y��N�|�b�t�έt�����C���ɭԱN��ƱH���z~ :) ");
    pressanykey();
    return FULLUPDATE;
}
#endif

/* ----------------------------------------------------- */
/* �ݪO�Ƨѿ��B��K�B��ذ�                              */
/* ----------------------------------------------------- */
int
b_note_edit_bname(int bid)
{
    char            buf[PATHLEN];
    int             aborted;
    boardheader_t  *fh = getbcache(bid);
    setbfile(buf, fh->brdname, fn_notes);
    aborted = vedit(buf, NA, NULL);
    if (aborted == -1) {
	clear();
	outs(msg_cancel);
	pressanykey();
    } else {
	if (!getdata(2, 0, "�]�w���Ĵ����ѡH(n/Y)", buf, 3, LCECHO)
	    || buf[0] != 'n')
	    fh->bupdate = gettime(3, fh->bupdate ? fh->bupdate : now, 
		      "���Ĥ����");
	else
	    fh->bupdate = 0;
	substitute_record(fn_board, fh, sizeof(boardheader_t), bid);
    }
    return 0;
}

static int
b_notes_edit(void)
{
    if (currmode & MODE_BOARD) {
	b_note_edit_bname(currbid);
	return FULLUPDATE;
    }
    return 0;
}

static int
b_water_edit(void)
{
    if (currmode & MODE_BOARD) {
	friend_edit(BOARD_WATER);
	return FULLUPDATE;
    }
    return 0;
}

static int
visable_list_edit(void)
{
    if (currmode & MODE_BOARD) {
	friend_edit(BOARD_VISABLE);
	hbflreload(currbid);
	return FULLUPDATE;
    }
    return 0;
}

static int
b_post_note(void)
{
    char            buf[200], yn[3];
    if (currmode & MODE_BOARD) {
	setbfile(buf, currboard, FN_POST_NOTE);
	if (more(buf, NA) == -1)
	    more("etc/" FN_POST_NOTE, NA);
	getdata(b_lines - 2, 0, "�O�_�n�Φۭqpost�`�N�ƶ�?",
		yn, sizeof(yn), LCECHO);
	if (yn[0] == 'y')
	    vedit(buf, NA, NULL);
	else
	    unlink(buf);


	setbfile(buf, currboard, FN_POST_BID);
	if (more(buf, NA) == -1)
	    more("etc/" FN_POST_BID, NA);
	getdata(b_lines - 2, 0, "�O�_�n�Φۭq�v�Ф峹�`�N�ƶ�?",
		yn, sizeof(yn), LCECHO);
	if (yn[0] == 'y')
	    vedit(buf, NA, NULL);
	else
	    unlink(buf);

	return FULLUPDATE;
    }
    return 0;
}


static int
can_vote_edit(void)
{
    if (currmode & MODE_BOARD) {
	friend_edit(FRIEND_CANVOTE);
	return FULLUPDATE;
    }
    return 0;
}

static int
bh_title_edit(void)
{
    boardheader_t  *bp;

    if (currmode & MODE_BOARD) {
	char            genbuf[BTLEN];

	bp = getbcache(currbid);
	move(1, 0);
	clrtoeol();
	getdata_str(1, 0, "�п�J�ݪO�s����ԭz:", genbuf,
		    BTLEN - 16, DOECHO, bp->title + 7);

	if (!genbuf[0])
	    return 0;
	strip_ansi(genbuf, genbuf, STRIP_ALL);
	strlcpy(bp->title + 7, genbuf, sizeof(bp->title) - 7);
	substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
	log_usies("SetBoard", currboard);
	return FULLUPDATE;
    }
    return 0;
}

static int
b_notes(void)
{
    char            buf[PATHLEN];
    int mr = 0;

    setbfile(buf, currboard, fn_notes);
    mr = more(buf, NA);

    if (mr == -1)
    {
	clear();
	move(4, 20);
	outs("���ݪO�|�L�u�Ƨѿ��v�C");
    }
    if(mr != READ_NEXT)
	    pressanykey();
    return FULLUPDATE;
}

int
board_select(void)
{
    char            fpath[80];
    char            genbuf[100];

    currmode &= ~MODE_SELECT;
    snprintf(fpath, sizeof(fpath), "SR.%s", cuser.userid);
    setbfile(genbuf, currboard, fpath);
    unlink(genbuf);
    if (currstat == RMAIL)
	sethomedir(currdirect, cuser.userid);
    else
	setbdir(currdirect, currboard);
    return NEWDIRECT;
}

int
board_digest(void)
{
    if (currmode & MODE_SELECT)
	board_select();
    currmode ^= MODE_DIGEST;
    if (currmode & MODE_DIGEST)
	currmode &= ~MODE_POST;
    else if (haspostperm(currboard))
	currmode |= MODE_POST;

    setbdir(currdirect, currboard);
    return NEWDIRECT;
}


static int
push_bottom(int ent, fileheader_t *fhdr, const char *direct)
{
    int num;
    char buf[256];
    if ((currmode & MODE_DIGEST) || !(currmode & MODE_BOARD))
        return DONOTHING;
    setbottomtotal(currbid);  // <- Ptt : will be remove when stable
    num = getbottomtotal(currbid);
    if( getans(fhdr->filemode & FILE_BOTTOM ?
	       "�����m�����i?(y/N)":
	       "�[�J�m�����i?(y/N)") != 'y' )
	return READ_REDRAW;
    if(!(fhdr->filemode & FILE_BOTTOM) ){
          sprintf(buf, "%s.bottom", direct);
          if(num >= 5){
              vmsg("���o�W�L 5 �g���n���i �к�²!");
              return READ_REDRAW;
	  }
	  fhdr->filemode ^= FILE_BOTTOM;
	  fhdr->multi.refer.flag = 1;
          fhdr->multi.refer.ref = ent;
          append_record(buf, fhdr, sizeof(fileheader_t)); 
    }
    else{
	fhdr->filemode ^= FILE_BOTTOM;
	num = delete_record(direct, sizeof(fileheader_t), ent);
    }
    setbottomtotal(currbid);
    return DIRCHANGED;
}

static int
good_post(int ent, fileheader_t * fhdr, const char *direct)
{
    char            genbuf[200];
    char            genbuf2[200];
    int             delta = 0;

    if ((currmode & MODE_DIGEST) || !(currmode & MODE_BOARD))
	return DONOTHING;

    if(getans(fhdr->filemode & FILE_DIGEST ? 
              "�����ݪO��K?(Y/n)" : "���J�ݪO��K?(Y/n)") == 'n')
	return READ_REDRAW;

    if (fhdr->filemode & FILE_DIGEST) {
	fhdr->filemode = (fhdr->filemode & ~FILE_DIGEST);
	if (!strcmp(currboard, "Note") || !strcmp(currboard, "PttBug") ||
	    !strcmp(currboard, "Artdsn") || !strcmp(currboard, "PttLaw")) {
	    deumoney(searchuser(fhdr->owner, NULL), -1000); // TODO if searchuser() return 0
	    if (!(currmode & MODE_SELECT))
		fhdr->multi.money -= 1000;
	    else
		delta = -1000;
	}
    } else {
	fileheader_t    digest;
	char           *ptr, buf[PATHLEN];

	memcpy(&digest, fhdr, sizeof(digest));
	digest.filename[0] = 'G';
	strlcpy(buf, direct, sizeof(buf));
	ptr = strrchr(buf, '/');
	assert(ptr);
	ptr++;
	ptr[0] = '\0';
	snprintf(genbuf, sizeof(genbuf), "%s%s", buf, digest.filename);

	if (dashf(genbuf))
	    unlink(genbuf);

	digest.filemode = 0;
	snprintf(genbuf2, sizeof(genbuf2), "%s%s", buf, fhdr->filename);
	Copy(genbuf2, genbuf);
	strcpy(ptr, fn_mandex);
	append_record(buf, &digest, sizeof(digest));

#ifdef GLOBAL_DIGEST
	if(!(getbcache(currbid)->brdattr & BRD_HIDE)) { 
          getdata(1, 0, "�n��ȱo�X���������K?(N/y)", genbuf2, 3, LCECHO);
          if(genbuf2[0] == 'y')
	      do_crosspost(GLOBAL_DIGEST, &digest, genbuf);
        }
#endif

	fhdr->filemode = (fhdr->filemode & ~FILE_MARKED) | FILE_DIGEST;
	if (!strcmp(currboard, "Note") || !strcmp(currboard, "PttBug") ||
	    !strcmp(currboard, "Artdsn") || !strcmp(currboard, "PttLaw")) {
	    deumoney(searchuser(fhdr->owner, NULL), 1000); // TODO if searchuser() return 0
	    if (!(currmode & MODE_SELECT))
		fhdr->multi.money += 1000;
	    else
		delta = 1000;
	}
    }
    substitute_ref_record(direct, fhdr, ent);
    return FULLUPDATE;
}

static int
b_help(void)
{
    show_helpfile(fn_boardhelp);
    return FULLUPDATE;
}

static int
b_changerecommend(int ent, const fileheader_t * fhdr, const char *direct)
{
    boardheader_t   *bp=NULL;
    if (!((currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP)))
	return DONOTHING;
    bp = getbcache(currbid); 

#ifdef OLDRECOMMEND
    bp->brdattr ^= BRD_NORECOMMEND;
#else
    if(bp->brdattr & BRD_NOBOO)
	bp->brdattr ^= BRD_NORECOMMEND;
    if(!(bp->brdattr & BRD_NORECOMMEND) || !(bp->brdattr & BRD_NOBOO))
	bp->brdattr ^= BRD_NOBOO;
#endif

    substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);

#ifdef OLDRECOMMEND
    vmsg("���O�{�b %s ����",
	    (bp->brdattr & BRD_NORECOMMEND) ? "�T��" : "�}��");
#else
    vmsg("���O�{�b %s ����, %s �N�n",
	    (bp->brdattr & BRD_NORECOMMEND) ? "�T��" : "�}��",
	    (bp->brdattr & BRD_NOBOO) ? "�T��" : "�}��");
#endif
    return FULLUPDATE;
}

/* ----------------------------------------------------- */
/* �O�D�]�w����/ ������                                  */
/* ----------------------------------------------------- */
char            board_hidden_status;
#ifdef  BMCHS
static int
change_hidden(int ent, const fileheader_t * fhdr, const char *direct)
{
    boardheader_t   *bp;
    if (!((currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP)))
	return DONOTHING;

    bp = getbcache(currbid);
    if (((bp->brdattr & BRD_HIDE) && (bp->brdattr & BRD_POSTMASK))) {
	if (getans("�ثe�O�b���Ϊ��A, �n�����ι�(y/N)?") != 'y')
	    return FULLUPDATE;
	bp->brdattr &= ~BRD_HIDE;
	bp->brdattr &= ~BRD_POSTMASK;
	outs("�g�ߤ��ǲ��H�A�L�B���D���q�C\n");
	board_hidden_status = 0;
	hbflreload(currbid);
    } else {
	if (getans("�ثe�O�b�{�Ϊ��A, �n���ι�(y/N)?") != 'y')
	    return FULLUPDATE;
	bp->brdattr |= BRD_HIDE;
	bp->brdattr |= BRD_POSTMASK;
	outs("�g�ߤ��w����A���ߵ��۬í��C\n");
	board_hidden_status = 1;
    }
    substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
    log_usies("SetBoard", bp->brdname);
    pressanykey();
    return FULLUPDATE;
}

static int
change_counting(int ent, const fileheader_t * fhdr, const char *direct)
{   

    boardheader_t   *bp;
    if (!((currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP)))
	return DONOTHING;
    bp = getbcache(currbid);
    if (!(bp->brdattr & BRD_HIDE && bp->brdattr & BRD_POSTMASK))
	return FULLUPDATE;

    if (bp->brdattr & BRD_BMCOUNT) {
	if (getans("�ثe�O�C�J�Q�j�Ʀ�, �n�����C�J�Q�j�Ʀ��(Y/N)?[N]") != 'y')
	    return FULLUPDATE;
	bp->brdattr &= ~BRD_BMCOUNT;
	outs("�A�A����]���|���Q�j���r�C\n");
    } else {
	if (getans("�ثe�ݪO���C�J�Q�j�Ʀ�, �n�C�J�Q�j��(Y/N)?[N]") != 'y')
	    return FULLUPDATE;
	bp->brdattr |= BRD_BMCOUNT;
	outs("������ĤQ�j�Ĥ@�a�C\n");
    }
    substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
    pressanykey();
    return FULLUPDATE;
}

#endif

/**
 * ���ܥثe�Ҧb�O�峹���w�]�x�s�覡
 */
static int
change_localsave(int ent, const fileheader_t * fhdr, const char *direct)
{
    boardheader_t *bp;
    if (!((currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP)))
	return DONOTHING;

    bp = getbcache(currbid);
    if (bp->brdattr & BRD_LOCALSAVE) {
	if (getans("�ثe�O�w�]�����s��, �n���ܹ�(y/N)?") != 'y')
	    return FULLUPDATE;
	bp->brdattr &= ~BRD_LOCALSAVE;
	outs("�峹�w�]��X�A�Ц��Ҹ`��C\n");
    } else {
	if (getans("�ثe�O�w�]���ڦs��, �n���ܹ�(y/N)?") != 'y')
	    return FULLUPDATE;
	bp->brdattr |= BRD_LOCALSAVE;
	outs("�峹�w�]����X�A��H�n�ۦ��ܳ�C\n");
    }
    substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
    pressanykey();
    return FULLUPDATE;
}

/**
 * �]�w�u���O�ͥi post �Υ����H���i post
 */
static int
change_restrictedpost(int ent, fileheader_t * fhdr, char *direct){
    boardheader_t *bp;
    if (!HasUserPerm(PERM_SYSOP))
	return DONOTHING;

    bp = getbcache(currbid);
    if (bp->brdattr & BRD_RESTRICTEDPOST) {
	if (getans("�ثe�u���O�ͥi post, �n�}���(y/N)?") != 'y')
	    return FULLUPDATE;
	bp->brdattr &= ~BRD_RESTRICTEDPOST;
	outs("�j�a���i�H post �峹�F�C\n");
    } else {
	if (getans("�ثe�����H���i post, �n����u���O�ͥi post ��(y/N)?") != 'y')
	    return FULLUPDATE;
	bp->brdattr |= BRD_RESTRICTEDPOST;
	outs("�u�ѪO�ͥi�H post �F�C\n");
    }
    substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
    pressanykey();
    return FULLUPDATE;
}

#ifdef USE_COOLDOWN

int check_cooldown(boardheader_t *bp)
{
    int diff = cooldowntimeof(usernum) - now; 
    int i, limit[8] = {4000,1,2000,2,1000,3,30,10};

    if(diff<0)
	SHM->cooldowntime[usernum - 1] &= 0xFFFFFFF0;
    else if( !((currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP)))
    {
      if( bp->brdattr & BRD_COOLDOWN )
       {
  	 vmsg("�N�R�@�U�a�I (���� %d �� %d ��)", diff/60, diff%60);
	 return 1;
       }
      else if(posttimesof(usernum)==15)
      {
	 vmsg("�藍�_�A�z�Q�]�H��I (���� %d �� %d ��)", diff/60, diff%60);
	 return 1;
      }
#ifdef NO_WATER_POST
      else
      {
        for(i=0; i<4; i++)
          if(bp->nuser>limit[i*2] && posttimesof(usernum)>=limit[i*2+1])
          {
	    vmsg("�藍�_�A�z���峹�Ӥ��o�I��'X'���ˤ峹 (���� %d �� %d ��)", 
		  diff/60, diff%60);
	    return 1;
          }
      }
#endif // NO_WATER_POST
   }
   return 0;
}
/**
 * �]�w�ݪO�N�R�\��, ����ϥΪ̵o��ɶ�
 */
static int
change_cooldown(int ent, const fileheader_t * fhdr, const char *direct)
{
    boardheader_t *bp = getbcache(currbid);
    
    if (!HasUserPerm(PERM_SYSOP))
	return DONOTHING;

    if (bp->brdattr & BRD_COOLDOWN) {
	if (getans("�ثe���Ť�, �n�}���(y/N)?") != 'y')
	    return FULLUPDATE;
	bp->brdattr &= ~BRD_COOLDOWN;
	outs("�j�a���i�H post �峹�F�C\n");
    } else {
	if (getans("�n���� post �W�v, ���Ŷ�(y/N)?") != 'y')
	    return FULLUPDATE;
	bp->brdattr |= BRD_COOLDOWN;
	outs("�}�l�N�R�C\n");
    }
    substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
    pressanykey();
    return FULLUPDATE;
}
#endif

/* ----------------------------------------------------- */
/* �ݪO�\���                                            */
/* ----------------------------------------------------- */
/* onekey_size was defined in ../include/pttstruct.h, as ((int)'z') */
const onekey_t read_comms[] = {
    show_filename, // Ctrl('A') 
    NULL, // Ctrl('B')
    NULL, // Ctrl('C')
    NULL, // Ctrl('D')
    change_restrictedpost, // Ctrl('E')
    NULL, // Ctrl('F')
#ifdef NO_GAMBLE
    NULL, // Ctrl('G')
#else
    hold_gamble, // Ctrl('G')
#endif
    NULL, // Ctrl('H')
    board_digest, // Ctrl('I') KEY_TAB 9
    NULL, // Ctrl('J')
    NULL, // Ctrl('K')
    NULL, // Ctrl('L')
    NULL, // Ctrl('M')
#ifdef BMCHS
    change_counting, // Ctrl('N')
#else
    NULL, // Ctrl('N')
#endif
    do_post_openbid, // Ctrl('O')
    do_post, // Ctrl('P')
    NULL, // Ctrl('Q')
    NULL, // Ctrl('R')
    NULL, // Ctrl('S')
    NULL, // Ctrl('T')
    NULL, // Ctrl('U')
    do_post_vote, // Ctrl('V')
    whereami, // Ctrl('W')
    change_localsave, // Ctrl('X')
    NULL, // Ctrl('Y')
    push_bottom, // Ctrl('Z') 26
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, // 'A' 65
    bh_title_edit, // 'B'
    do_limitedit, // 'C'
    del_range, // 'D'
    edit_post, // 'E'
    NULL, // 'F'
    NULL, // 'G'
#ifdef BMCHS
    change_hidden, // 'H'
#else
    NULL, // 'H'
#endif
    b_changerecommend, // 'I'
#ifdef USE_COOLDOWN
    change_cooldown, // 'J'
#else
    NULL, // 'J'
#endif
    b_water_edit, // 'K'
    solve_post, // 'L'
    b_vote_maintain, // 'M'
    NULL, // 'N'
    b_post_note, // 'O'
    NULL, // 'P'
    view_postmoney, // 'Q'
    b_results, // 'R'
    NULL, // 'S'
    edit_title, // 'T'
    NULL, // 'U'
    b_vote, // 'V'
    b_notes_edit, // 'W'
    recommend, // 'X'
    recommend_cancel, // 'Y'
    NULL, // 'Z' 90
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, // 'a' 97
    b_notes, // 'b'
    cite_post, // 'c'
    del_post, // 'd'
    NULL, // 'e'
#ifdef NO_GAMBLE
    NULL, // 'f'
#else
    join_gamble, // 'f'
#endif
    good_post, // 'g'
    b_help, // 'h'
    b_posttype, // 'i'
    NULL, // 'j'
    NULL, // 'k'
    NULL, // 'l'
    mark_post, // 'm'
    NULL, // 'n'
    can_vote_edit, // 'o'
    NULL, // 'p'
    NULL, // 'q'
    read_post, // 'r'
    do_select, // 's'
    NULL, // 't'
#ifdef OUTJOBSPOOL
    tar_addqueue, // 'u'
#else
    NULL, // 'u'
#endif
    visable_list_edit, // 'v'
    b_call_in, // 'w'
    cross_post, // 'x'
    reply_post, // 'y'
    b_man, // 'z' 122
};

int
Read(void)
{
    int             mode0 = currutmp->mode;
    int             stat0 = currstat, tmpbid = currutmp->brc_id;
    char            buf[40];
#ifdef LOG_BOARD
    time4_t         usetime = now;
#endif

    if ( !currboard[0] )
	brc_initial_board(DEFAULT_BOARD);

    setutmpmode(READING);
    set_board();

    if (board_note_time && board_visit_time < *board_note_time) {
	int mr = 0;

	setbfile(buf, currboard, fn_notes);
	mr = more(buf, NA);
	if(mr == -1)
            *board_note_time=0;
	else if (mr != READ_NEXT)
	    pressanykey();
    }
    setutmpbid(currbid);
    setbdir(buf, currboard);
    curredit &= ~EDIT_MAIL;
    i_read(READING, buf, readtitle, readdoent, read_comms,
	   currbid);
    currmode &= ~MODE_POSTCHECKED;
#ifdef LOG_BOARD
    log_board(currboard, now - usetime);
#endif
    brc_update();
    setutmpbid(tmpbid);
    currutmp->mode = mode0;
    currstat = stat0;
    return 0;
}

void
ReadSelect(void)
{
    int             mode0 = currutmp->mode;
    int             stat0 = currstat;
    char            genbuf[200];

    currstat = SELECT;
    if (do_select(0, 0, genbuf) == NEWDIRECT)
	Read();
    setutmpbid(0);
    currutmp->mode = mode0;
    currstat = stat0;
}

#ifdef LOG_BOARD
static void
log_board(iconst char *mode, time4_t usetime)
{
    if (usetime > 30) {
	log_file(FN_USEBOARD, LOG_CREAT | LOG_VF,
		 "USE %-20.20s Stay: %5ld (%s) %s\n", 
                 mode, usetime, cuser.userid, ctime4(&now));
    }
}
#endif

int
Select(void)
{
    char            genbuf[200];

    do_select(0, NULL, genbuf);
    return 0;
}

#ifdef HAVEMOBILE
void
mobile_message(const char *mobile, char *message)
{
    bsmtp(char *fpath, char *title, char *rcpt, int method);
}
#endif
