/* $Id$ */
#include "bbs.h"

static int recommend(int ent, fileheader_t * fhdr, char *direct);

#ifdef ASSESS
static char *badpost_reason[] = {
    "廣告", "不當用辭", "人身攻擊"
};
#endif

static void
mail_by_link(char *owner, char *title, char *path)
{
    char            genbuf[200];
    fileheader_t    mymail;

    snprintf(genbuf, sizeof(genbuf),
	     BBSHOME "/home/%c/%s", cuser.userid[0], cuser.userid);
    stampfile(genbuf, &mymail);
    strlcpy(mymail.owner, owner, sizeof(mymail.owner));
    snprintf(mymail.title, sizeof(mymail.title), title);
    unlink(genbuf);
    Link(path, genbuf);
    snprintf(genbuf, sizeof(genbuf),
	     BBSHOME "/home/%c/%s/.DIR", cuser.userid[0], cuser.userid);

    append_record(genbuf, &mymail, sizeof(mymail));
}


void
anticrosspost()
{
    char            buf[200];

    snprintf(buf, sizeof(buf),
	    "\033[1;33;46m%s \033[37;45mcross post 文章 \033[37m %s\033[m\n",
	    cuser.userid, ctime(&now));
    log_file("etc/illegal_money", buf, 1);

    post_violatelaw(cuser.userid, "Ptt系統警察", "Cross-post", "罰單處份");
    cuser.userlevel |= PERM_VIOLATELAW;
    cuser.vl_count++;
    mail_by_link("Ptt警察部隊", "Cross-Post罰單",
		 BBSHOME "/etc/crosspost.txt");
    passwd_update(usernum, &cuser);
    exit(0);
}

/* Heat CharlieL */
int
save_violatelaw()
{
    char            buf[128], ok[3];

    setutmpmode(VIOLATELAW);
    clear();
    stand_title("繳罰單中心");

    if (!(cuser.userlevel & PERM_VIOLATELAW)) {
	mprints(22, 0, "\033[1;31m你無聊啊? 你又沒有被開罰單~~\033[m");
	pressanykey();
	return 0;
    }
    reload_money();
    if (cuser.money < (int)cuser.vl_count * 1000) {
	snprintf(buf, sizeof(buf), "\033[1;31m這是你第 %d 次違反本站法規"
		 "必須繳出 %d $Ptt ,你只有 %d 元, 錢不夠啦!!\033[m",
		 (int)cuser.vl_count, (int)cuser.vl_count * 1000, cuser.money);
	mprints(22, 0, buf);
	pressanykey();
	return 0;
    }
    move(5, 0);
    prints("\033[1;37m你知道嗎? 因為你的違法 "
	   "已經造成很多人的不便\033[m\n");
    prints("\033[1;37m你是否確定以後不會再犯了？\033[m\n");

    if (!getdata(10, 0, "確定嗎？[y/n]:", ok, sizeof(ok), LCECHO) ||
	ok[0] == 'n' || ok[0] == 'N') {
	mprints(22, 0, "\033[1;31m等你想通了再來吧!! "
		"我相信你不會知錯不改的~~~\033[m");
	pressanykey();
	return 0;
    }
    snprintf(buf, sizeof(buf), "這是你第 %d 次違法 必須繳出 %d $Ptt",
	     cuser.vl_count, cuser.vl_count * 1000);
    mprints(11, 0, buf);

    if (!getdata(10, 0, "要付錢[y/n]:", ok, sizeof(ok), LCECHO) ||
	ok[0] == 'N' || ok[0] == 'n') {

	mprints(22, 0, "\033[1;31m 嗯 存夠錢 再來吧!!!\033[m");
	pressanykey();
	return 0;
    }
    demoney(-1000 * cuser.vl_count);
    cuser.userlevel &= (~PERM_VIOLATELAW);
    passwd_update(usernum, &cuser);
    return 0;
}

/*
 * void make_blist() { CreateNameList(); apply_boards(g_board_names); }
 */

static time_t   *board_note_time;

void
set_board()
{
    boardheader_t  *bp;

    bp = getbcache(currbid);
    if( !HasPerm(bp) ){
	vmsg("access control violation, exit");
	u_exit("access control violation!");
	exit(-1);
    }
    board_note_time = &bp->bupdate;
    if(bp->BM[0] <= ' ')
	strcpy(currBM, "徵求中");
    else
	snprintf(currBM, sizeof(currBM), "板主：%s", bp->BM);
    currmode = (currmode & (MODE_DIRTY | MODE_GROUPOP)) | MODE_STARTED;

    if (HAS_PERM(PERM_ALLBOARD) || is_BM_cache(currbid))
	currmode = currmode | MODE_BOARD | MODE_POST;
    else if (haspostperm(currboard))
	currmode |= MODE_POST;
}

static void
readtitle()
{
    boardheader_t  *bp;
    char    *brd_title;

    bp = getbcache(currbid);
    if(bp->bvote != 2 && bp->bvote)
	brd_title = "本看板進行投票中";
    else
	brd_title = bp->title + 7;

    showtitle(currBM, brd_title);
    prints("[←]離開 [→]閱\讀 [^P]發表文章 [b]備忘錄 [d]刪除 [z]精華區 "
	   "[TAB]文摘 [h]elp\n\033[7m  編號    日 期 作  者       文  章  標  題"
	   "                      人氣:%-5d   \033[m", SHM->bcache[currbid - 1].nuser);
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
    else if (currmode & MODE_BOARD || HAS_PERM(PERM_LOGINOK)) {
	if (ent->filemode & FILE_MARKED)
	    type = (type == ' ') ? 'm' : 'M';

	else if (TagNum && !Tagger(atoi(ent->filename + 2), 0, TAG_NIN))
	    type = 'D';

	else if (ent->filemode & FILE_SOLVED)
	    type = 's';
    }
    title = subject(mark = ent->title);
    if (ent->filemode & FILE_VOTE)
	color = '2', mark = "ˇ";
    else if (ent->filemode & FILE_BID)
	color = '6', mark = "＄";
    else if (title == mark)
	color = '1', mark = "□";
    else
	color = '3', mark = "R:";

    if (title[45])
	strlcpy(title + 42, " …", sizeof(title) - 42);	/* 把多餘的 string 砍掉 */

    if (!strncmp(title, "[公告]", 6))
	special = 1;
#if 1
    if (!strchr(ent->owner, '.') && !SHM->GV2.e.noonlineuser &&
	(uentp = search_ulist_userid(ent->owner)) && isvisible(currutmp, uentp))
	isonline = 1;
#else
    if (!strchr(ent->owner, '.') && (uid = searchuser(ent->owner)) &&
	!SHM->GV2.e.noonlineuser &&
	(uentp = search_ulist(uid)) && isvisible(currutmp, uentp))
	isonline = 1;
#endif
    if(ent->recommend>99)
	  strcpy(recom,"1m爆");
    else if(ent->recommend>9)
	  sprintf(recom,"3m%2d",ent->recommend);
    else if(ent->recommend>0)
	  sprintf(recom,"2m%2d",ent->recommend);
    else strcpy(recom,"0m  "); 

    prints(
#ifdef COLORDATE
	   "%6d %c\033[1;3%4.4s\033[%dm%-6s\033[m\033[%dm%-13.12s",
#else
	   "%6d %c\033[1;3%4.4s\033[m%-6s\033[%dm%-13.12s",
#endif
	   num, type, recom,
#ifdef COLORDATE
	   (ent->date[3] + ent->date[4]) % 7 + 31,
#endif
	   ent->date, isonline, ent->owner);
	   
    if (strncmp(currtitle, title, TTLEN))
	prints("\033[m%s \033[1m%.*s\033[m%s\n",
	       mark, special ? 6 : 0, title, special ? title + 6 : title);
    else
	prints("\033[1;3%cm%s %s\033[m\n",
	       color, mark, title);
}

int
cmpfilename(fileheader_t * fhdr)
{
    return (!strcmp(fhdr->filename, currfile));
}

int
cmpfmode(fileheader_t * fhdr)
{
    return (fhdr->filemode & currfmode);
}

int
cmpfowner(fileheader_t * fhdr)
{
    return !strcasecmp(fhdr->owner, currowner);
}

int
whereami(int ent, fileheader_t * fhdr, char *direct)
{
    boardheader_t  *bh, *p[32], *root;
    int             i, j;

    if (!currutmp->brc_id)
	return 0;

    move(1, 0);
    clrtobot();
    bh = getbcache(currutmp->brc_id);
    root = getbcache(1);
    p[0] = bh;
    for (i = 0; i < 31 && p[i]->parent != root && p[i]->parent; i++)
	p[i + 1] = p[i]->parent;
    j = i;
    prints("我在哪?\n%-40.40s %.13s\n", p[j]->title + 7, p[j]->BM);
    for (j--; j >= 0; j--)
	prints("%*s %-13.13s %-37.37s %.13s\n", (i - j) * 2, "",
	       p[j]->brdname, p[j]->title,
	       p[j]->BM);

    pressanykey();
    return FULLUPDATE;
}

static int
substitute_check(fileheader_t * fhdr)
{
    fileheader_t    hdr;
    char            genbuf[100];
    int             num = 0;

    /* rocker.011018: 串接模式用reference增進效率 */
    if ((currmode & MODE_SELECT) && (fhdr->money & FHR_REFERENCE)) {
	num = fhdr->money & ~FHR_REFERENCE;
	setbdir(genbuf, currboard);
	get_record(genbuf, &hdr, sizeof(hdr), num);

	/* 再這裡要check一下原來的dir裡面是不是有被人動過... */
	if (strcmp(hdr.filename, fhdr->filename))
	    num = getindex(genbuf, fhdr->filename, sizeof(fileheader_t));

	substitute_record(genbuf, fhdr, sizeof(*fhdr), num);
    }
    return num;
}
static int
do_select(int ent, fileheader_t * fhdr, char *direct)
{
    char            bname[20];
    char            bpath[60];
    boardheader_t  *bh;
    struct stat     st;
    int             i;

    setutmpmode(SELECT);
    move(0, 0);
    clrtoeol();
    generalnamecomplete(MSG_SELECT_BOARD, bname, sizeof(bname),
			SHM->Bnumber,
			completeboard_compar,
			completeboard_permission,
			completeboard_getname);
    if (bname[0] == '\0' || !(i = getbnum(bname)))
	return FULLUPDATE;
    bh = getbcache(i);
    if (!HasPerm(bh))
	return FULLUPDATE;
    strlcpy(bname, bh->brdname, sizeof(bname));
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
    setbdir(direct, currboard);

    move(1, 0);
    clrtoeol();
    return NEWDIRECT;
}

/* ----------------------------------------------------- */
/* 改良 innbbsd 轉出信件、連線砍信之處理程序             */
/* ----------------------------------------------------- */
void
outgo_post(fileheader_t * fh, char *board)
{
    FILE           *foo;

    if ((foo = fopen("innd/out.bntp", "a"))) {
	fprintf(foo, "%s\t%s\t%s\t%s\t%s\n", board,
		fh->filename, cuser.userid, cuser.username, fh->title);
	fclose(foo);
    }
}

static void
cancelpost(fileheader_t * fh, int by_BM, char *newpath)
{
    FILE           *fin, *fout;
    char           *ptr, *brd;
    fileheader_t    postfile;
    char            genbuf[200];
    char            nick[STRLEN], fn1[STRLEN];

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

	if ((fout = fopen("innd/cancel.bntp", "a"))) {
	    fprintf(fout, "%s\t%s\t%s\t%s\t%s\n", currboard, fh->filename,
		    cuser.userid, nick, fh->title);
	    fclose(fout);
	}
	fclose(fin);
	Rename(fn1, newpath);
	setbdir(genbuf, brd);
	setbtotal(getbnum(brd));
	append_record(genbuf, &postfile, sizeof(postfile));
    }
}

/* ----------------------------------------------------- */
/* 發表、回應、編輯、轉錄文章                            */
/* ----------------------------------------------------- */
void
do_reply_title(int row, char *title)
{
    char            genbuf[200];
    char            genbuf2[4];

    if (strncasecmp(title, str_reply, 4))
	snprintf(save_title, sizeof(save_title), "Re: %s", title);
    else
	strlcpy(save_title, title, sizeof(save_title));
    save_title[TTLEN - 1] = '\0';
    snprintf(genbuf, sizeof(genbuf), "採用原標題《%.60s》嗎?[Y] ", save_title);
    getdata(row, 0, genbuf, genbuf2, 4, LCECHO);
    if (genbuf2[0] == 'n' || genbuf2[0] == 'N')
	getdata(++row, 0, "標題：", save_title, TTLEN, DOECHO);
}

static void
do_unanonymous_post(char *fpath)
{
    fileheader_t    mhdr;
    char            title[128];
    char            genbuf[200];

    setbpath(genbuf, "UnAnonymous");
    if (dashd(genbuf)) {
	stampfile(genbuf, &mhdr);
	unlink(genbuf);
	Link(fpath, genbuf);
	strlcpy(mhdr.owner, cuser.userid, sizeof(mhdr.owner));
	strlcpy(mhdr.title, save_title, sizeof(mhdr.title));
	mhdr.filemode = 0;
	setbdir(title, "UnAnonymous");
	append_record(title, &mhdr, sizeof(mhdr));
    }
}

#ifdef NO_WATER_POST
#ifndef DEBUG
static time_t   last_post_time = 0;
#endif
static time_t   water_counts = 0;
#endif

void 
do_crosspost(char *brd, fileheader_t *postfile, const char *fpath)
{
    char            genbuf[200];
    int             len = 42-strlen(currboard);
    fileheader_t    fh;
    if(!strncasecmp(postfile->title, str_reply, 3))
        len=len+4;
    setbpath(genbuf, brd);
    stampfile(genbuf, &fh);
    strcpy(fh.owner, postfile->owner);
    strcpy(fh.date, postfile->date);
    sprintf(fh.title,"%-*.*s.%s版",  len, len, postfile->title, currboard);
    unlink(genbuf);
    Link((char *)fpath, genbuf);
    postfile->filemode = FILE_LOCAL;
    setbdir(genbuf, brd);
    if (append_record(genbuf, &fh, sizeof(fileheader_t)) != -1) {
	setbtotal(getbnum(brd));
    }
}
static void 
setupbidinfo(bid_t *bidinfo)
{
        char buf[256];
        bidinfo->enddate = gettime(20, now+86400,"結束標案於");
        do
         getdata_str(21,0,"底價:",buf, 8, LCECHO, "1");
        while((bidinfo->high=atoi(buf))<=0);
        do
           getdata_str(21,20, "每標至少增加多少:",buf, 5, LCECHO, "1");
        while((bidinfo->increment=atoi(buf))<=0);
        getdata(21,44, "直接購買價(可不設):",buf, 10, LCECHO);
        bidinfo->buyitnow=atoi(buf);
	
        getdata_str(22,0,
          "付款方式: 1.Ptt幣 2.郵局或銀行轉帳 3.支票或電匯 4.郵局貨到付款 [1]:",
          buf, 3, LCECHO,"1");
          bidinfo->payby=(buf[0]-'1');
        if(bidinfo->payby<0 ||bidinfo->payby>3)bidinfo->payby=0;
        getdata_str(23,0, "運費(0:免運費或文中說明)[0]:", buf, 6, LCECHO, "0"); 
        bidinfo->shipping = atoi(buf);
        if(bidinfo->shipping<0)  bidinfo->shipping=0;
}
static void
print_bidinfo(FILE *io, bid_t bidinfo)
{
    char *payby[4]={ "Ptt幣","郵局或銀行轉帳","支票或電匯","郵局貨到付款"};
    if(io)
    {
     if(!bidinfo.userid[0])
      fprintf(io,"起標價:    %-20d\n",bidinfo.high);
     else 
      fprintf(io, "目前最高價:%-20d出價者:%-16s\n",bidinfo.high, bidinfo.userid);
     fprintf(io, "付款方式:  %-20s結束於:%-16s\n",payby[bidinfo.payby%4],Cdate(& bidinfo.enddate));
     if(bidinfo.buyitnow)
      fprintf(io, "直接購買價:%-20d",bidinfo.buyitnow);
     if(bidinfo.shipping)
      fprintf(io, "運費:%d", bidinfo.shipping);
     fprintf(io, "\n");
    }
    else
    {
     if(!bidinfo.userid[0])
      prints("起標價:    %-20d\n",bidinfo.high);
     else 
      prints("目前最高價:%-20d出價者:%-16s\n",bidinfo.high, bidinfo.userid);
     prints("付款方式:  %-20s結束於:%-16s\n",payby[bidinfo.payby%4],Cdate(& bidinfo.enddate));
     if(bidinfo.buyitnow)
      prints("直接購買價:%-20d",bidinfo.buyitnow);
     if(bidinfo.shipping)
      prints("運費:%d", bidinfo.shipping);
     prints("\n");
    }

}
static int
do_general(int isbid)
{
    bid_t bidinfo;
    fileheader_t    postfile;
    char            fpath[80], buf[80];
    int             aborted, defanony, ifuseanony, i;
    char            genbuf[200], *owner, ctype[8][5] = {"問題", "建議", "討論", "心得", "閒聊", "請益", "公告", "情報"};
    boardheader_t  *bp;
    int             islocal, posttype=-1;

    ifuseanony = 0;
    bp = getbcache(currbid);

    clear();
    if (!(currmode & MODE_POST)
#ifdef FOREIGN_REG
	    // 不是外籍使用者在 PttForeign 板
	    && !((cuser.uflag2 & FOREIGN) && strcmp(bp->brdname, "PttForeign") == 0)
#endif
	) {
	move(5, 10);
	outs("對不起，您目前無法在此發表文章！");
	pressanykey();
	return FULLUPDATE;
    }
#ifdef NO_WATER_POST
#ifndef DEBUG /* why we need this in DEBUG mode? */
    /* 三分鐘內最多發表五篇文章 */
    if (currutmp->lastact - last_post_time < 60 * 3) {
	if (water_counts >= 5) {
	    move(5, 10);
	    outs("對不起，您的文章太水囉，待會再post吧！小秘訣:可用'X'推薦文章");
	    pressanykey();
	    return FULLUPDATE;
	}
    } else {
	last_post_time = currutmp->lastact;
	water_counts = 0;
    }
#endif
#endif

    if(!isbid)
       setbfile(genbuf, currboard, FN_POST_NOTE);
    else
       setbfile(genbuf, currboard, FN_POST_BID);

    if (more(genbuf, NA) == -1)
      {
       if(!isbid)
   	   more("etc/" FN_POST_NOTE, NA);
       else
   	   more("etc/" FN_POST_BID, NA);
      }
    move(19, 0);
    prints("%s於【\033[33m %s\033[m 】 \033[32m%s\033[m 看板\n",
           isbid?"公開招標":"發表文章",
	  currboard, bp->title + 7);

    if(isbid)
       {
	memset(&bidinfo,0,sizeof(bidinfo)); 
        setupbidinfo(&bidinfo);
        postfile.money=bidinfo.high;
        move(20,0);
        clrtobot();
       }
    if (quote_file[0])
	do_reply_title(20, currtitle);
    else {
	if(!isbid)
        {
         move(21,0);
         prints("種類：");
         for(i=0; i<8 && bp->posttype[i*4]; i++)
            strncpy(ctype[i],bp->posttype+4*i,4);
         if(i==0) i=8;
         for(aborted=0; aborted<i; aborted++)
            prints("%d.%4.4s ", aborted+1, ctype[aborted]);
         sprintf(buf,"(1-%d或不選)",i);
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
	getdata_buf(22, 0, "標題：", save_title, TTLEN, DOECHO);
	strip_ansi(save_title, save_title, STRIP_ALL);
    }
    if (save_title[0] == '\0')
	return FULLUPDATE;

    curredit &= ~EDIT_MAIL;
    curredit &= ~EDIT_ITEM;
    setutmpmode(POSTING);
    /* 未具備 Internet 權限者，只能在站內發表文章 */
    if (HAS_PERM(PERM_INTERNET))
	local_article = 0;
    else
	local_article = 1;

    /* build filename */
    setbpath(fpath, currboard);
    stampfile(fpath, &postfile);
    if(isbid)
      {
       aborted = (int)fopen(fpath, "w");
       if(aborted)
         {
          print_bidinfo((FILE*)aborted, bidinfo);
          fclose((FILE*)aborted);
         }
      }
    else if(posttype!=-1 && ((1<<posttype) & bp->posttype_f))
     {
          setbnfile(genbuf, bp->brdname, "postsample", posttype);
          Copy(genbuf, fpath);
     }
    
    aborted = vedit(fpath, YEA, &islocal);
    if (aborted == -1) {
	unlink(fpath);
	pressanykey();
	return FULLUPDATE;
    }
    water_counts++;		/* po成功 */

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
    /* 錢 */
    aborted = (aborted > MAX_POST_MONEY * 2) ? MAX_POST_MONEY : aborted / 2;
    if(ifuseanony)
       {
        postfile.filemode |= FILE_ANONYMOUS;
        postfile.money = currutmp->uid;
       }
    else if(!isbid)
       postfile.money = aborted;
    
    strlcpy(postfile.owner, owner, sizeof(postfile.owner));
    strlcpy(postfile.title, save_title, sizeof(postfile.title));
    if (islocal)		/* local save */
	postfile.filemode |= FILE_LOCAL;

    setbdir(buf, currboard);
    if(isbid)
       {
	 sprintf(genbuf, "%s.bid", fpath);
         append_record(genbuf,(void*) &bidinfo, sizeof(bidinfo));
         postfile.filemode |= FILE_BID ;
       }
    if (append_record(buf, &postfile, sizeof(postfile)) != -1) {
	setbtotal(currbid);

	if (currmode & MODE_SELECT)
	    append_record(currdirect, &postfile, sizeof(postfile));
	if (!islocal && !(bp->brdattr & BRD_NOTRAN))
	    outgo_post(&postfile, currboard);
	brc_addlist(postfile.filename);

	if (!(currbrdattr & BRD_HIDE) &&
	    (!bp->level || (currbrdattr & BRD_POSTMASK))) {
            do_crosspost(ALLPOST, &postfile, fpath);
	}
	outs("順利貼出佈告，");

#ifdef MAX_POST_MONEY
	aborted = (aborted > MAX_POST_MONEY) ? MAX_POST_MONEY : aborted;
#endif
	if (strcmp(currboard, "Test") && !ifuseanony) {
	    prints("這是您的第 %d 篇文章。",++cuser.numposts);
            if(postfile.filemode&FILE_BID)
                prints("招標文章沒有稿酬。");
            else if(currbrdattr&BRD_BAD)
                prints("違法改進中看板沒有稿酬。");
            else
              {
                prints(" 稿酬 %d 銀。",aborted);
                demoney(aborted);    
              }
	    passwd_update(usernum, &cuser);	/* post 數 */
	} else
	    outs("測試信件不列入紀錄，敬請包涵。");

	/* 回應到原作者信箱 */

	if (curredit & EDIT_BOTH) {
	    char           *str, *msg = "回應至作者信箱";

	    if ((str = strchr(quote_user, '.'))) {
		if (
#ifndef USE_BSMTP
		    bbs_sendmail(fpath, save_title, str + 1)
#else
		    bsmtp(fpath, save_title, str + 1, 0)
#endif
		    < 0)
		    msg = "作者無法收信";
	    } else {
		sethomepath(genbuf, quote_user);
		stampfile(genbuf, &postfile);
		unlink(genbuf);
		Link(fpath, genbuf);

		strlcpy(postfile.owner, cuser.userid, sizeof(postfile.owner));
		strlcpy(postfile.title, save_title, sizeof(postfile.title));
		sethomedir(genbuf, quote_user);
		if (append_record(genbuf, &postfile, sizeof(postfile)) == -1)
		    msg = err_uid;
	    }
	    outs(msg);
	    curredit ^= EDIT_BOTH;
	}
	if (currbrdattr & BRD_ANONYMOUS)
	    do_unanonymous_post(fpath);
    }
    pressanykey();
    return FULLUPDATE;
}

int
do_post()
{
    boardheader_t  *bp;
    bp = getbcache(currbid);
    if (bp->brdattr & BRD_VOTEBOARD)
	return do_voteboard(0);
    else if (!(bp->brdattr & BRD_GROUPBOARD))
	return do_general(0);
    return 0;
}

int
do_post_vote()
{
    return do_voteboard(1);
}

int
do_post_openbid()
{
    boardheader_t  *bp;
    bp = getbcache(currbid);
    if (!(bp->brdattr & BRD_VOTEBOARD))
	return do_general(1);
    return 0;
}

static void
do_generalboardreply(fileheader_t * fhdr)
{
    char            genbuf[3];
    getdata(b_lines - 1, 0,
	    "▲ 回應至 (F)看板 (M)作者信箱 (B)二者皆是 (Q)取消？[F] ",
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
    }
    *quote_file = 0;
}

int
getindex(char *fpath, char *fname, int size)
{
#define READSIZE 64  // 8192 / sizeof(fileheader_t)
    int             fd, i, len, now = 1; /* now starts from 1 */
    fileheader_t    fhdrs[READSIZE];

    if ((fd = open(fpath, O_RDONLY, 0)) != -1) {
	while( (len = read(fd, fhdrs, sizeof(fhdrs))) > 0 ){
	    len /= sizeof(fileheader_t);
	    for( i = 0 ; i < len ; ++i, ++now )
		if (!strcmp(fhdrs[i].filename, fname)) {
		    close(fd);
		    return now;
		}
	}
	close(fd);
    }
    return 0;
}

int
invalid_brdname(char *brd)
{
    register char   ch;

    ch = *brd++;
    if (not_alnum(ch))
	return 1;
    while ((ch = *brd++)) {
	if (not_alnum(ch) && ch != '_' && ch != '-' && ch != '.')
	    return 1;
    }
    return 0;
}

static int
b_call_in(int ent, fileheader_t * fhdr, char *direct)
{
    userinfo_t     *u = search_ulist(searchuser(fhdr->owner));
    if (u) {
	int             fri_stat;
	fri_stat = friend_stat(currutmp, u);
	if (isvisible_stat(currutmp, u, fri_stat) && call_in(u, fri_stat))
	    return FULLUPDATE;
    }
    return DONOTHING;
}


static int
b_posttype(int ent, fileheader_t * fhdr, char *direct)
{
   boardheader_t  *bp;
   int i, aborted;
   char filepath[256], genbuf[60], title[5], posttype_f, posttype[33]="";

   if(!(currmode & MODE_BOARD)) return DONOTHING;
   
   bp = getbcache(currbid);

   move(2,0);
   clrtobot();
   posttype_f =  bp->posttype_f;
   for(i=0; i<8; i++)
     {
       move(2,0);
       outs("文章種類:       ");
       strncpy(genbuf, bp->posttype+i*4, 4);
       genbuf[4]=0;
       sprintf(title,"%d.",i+1);
       if(!getdata_buf(2,11, title, genbuf, 5, DOECHO)) break;
       sprintf(posttype+i*4,"%-4.4s", genbuf); 
       if( posttype_f & (1<<i) )
          {
            if(getdata(2, 20, "設定範本格式？(Y/n)", genbuf, 3, LCECHO) &&
                genbuf[0]=='n')
                {
                 posttype_f &= ~(1<<i);
                 continue;
                }
          }
       else if (!getdata(2, 20, "設定範本格式？(y/N)", genbuf, 3, LCECHO) ||
              genbuf[0]!='y') continue;

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
   strncpy(bp->posttype, posttype, 32); /* 這邊應該要防race condition */

   substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
   return FULLUPDATE;
}

static void
do_reply(fileheader_t * fhdr)
{
    boardheader_t  *bp;
    bp = getbcache(currbid);
    if (bp->brdattr & BRD_VOTEBOARD || (fhdr->filemode & FILE_VOTE))
	do_voteboardreply(fhdr);
    else
	do_generalboardreply(fhdr);
}

static int
reply_post(int ent, fileheader_t * fhdr, char *direct)
{
    if (!(currmode & MODE_POST))
	return DONOTHING;

    setdirpath(quote_file, direct, fhdr->filename);
    do_reply(fhdr);
    *quote_file = 0;
    return FULLUPDATE;
}

static int
edit_post(int ent, fileheader_t * fhdr, char *direct)
{
    int             num;
    char            fpath[80], fpath0[80];
    char            genbuf[200];
    fileheader_t    postfile;
    boardheader_t  *bp = getbcache(currbid);

    if (strcmp(bp->brdname, "Security") == 0)
	return DONOTHING;

    if (!HAS_PERM(PERM_SYSOP) && ((bp->brdattr & BRD_VOTEBOARD) || fhdr->filemode & FILE_VOTE))
	return DONOTHING;

    if( !HAS_PERM(PERM_SYSOP) &&
	(!(currmode & MODE_POST) || strcmp(fhdr->owner, cuser.userid) != 0) )
	return DONOTHING;

    if( currmode & MODE_SELECT )
	return DONOTHING;

#ifdef SAFE_ARTICLE_DELETE
    if( fhdr->filename[0] == '.' )
	return DONOTHING;
#endif

    setutmpmode(REEDIT);
    setdirpath(genbuf, direct, fhdr->filename);
    local_article = fhdr->filemode & FILE_LOCAL;
    strlcpy(save_title, fhdr->title, sizeof(save_title));

    /* rocker.011018: 這裡是不是該檢查一下修改文章後的money和原有的比較? */
    if (vedit(genbuf, 0, NULL) != -1) {
	substitute_record(direct, fhdr, sizeof(*fhdr), ent);
	setbpath(fpath, currboard);
	stampfile(fpath, &postfile);
	strlcpy(genbuf, fhdr->filename, sizeof(genbuf));
	setbfile(fpath0, currboard, fhdr->filename);

	for(num = 2; genbuf[num] != 0; num++){
	    if(genbuf[num] == '.'){
		genbuf[num] = 0;
		break;
	    }
	}
	
	sprintf(postfile.filename, "%s.A.%3.3X", genbuf, rand() & 0xFFF);
	setdirpath(fpath, fpath, postfile.filename);
	unlink(fpath);

	Rename(fpath0, fpath);

	/* rocker.011018: fix 串接模式改文章後文章就不見的bug */
	if ((currmode & MODE_SELECT) && (fhdr->money & FHR_REFERENCE)) {
	    fileheader_t    hdr;

	    num = fhdr->money & ~FHR_REFERENCE;
	    setbdir(fpath0, currboard);
	    get_record(fpath0, &hdr, sizeof(hdr), num);

	    /* 再這裡要check一下原來的dir裡面是不是有被人動過... */
	    if (!strcmp(hdr.filename, fhdr->filename)) {
		strlcpy(hdr.filename, postfile.filename, sizeof(hdr.filename));
		strlcpy(hdr.title, save_title, sizeof(hdr.title));
		substitute_record(fpath0, &hdr, sizeof(hdr), num);
	    }
	}
	strlcpy(fhdr->filename, postfile.filename, sizeof(fhdr->filename));
	strlcpy(fhdr->title, save_title, sizeof(fhdr->title));
	brc_addlist(postfile.filename);
	substitute_record(direct, fhdr, sizeof(*fhdr), ent);
	/* rocker.011018: 順便更新一下cache */
	touchdircache(currbid);

	if (!(currbrdattr & BRD_HIDE) && (!bp->level || (currbrdattr & BRD_POSTMASK)))
	    do_crosspost(ALLPOST, fhdr, fpath);
    }
    return FULLUPDATE;
}

#define UPDATE_USEREC   (currmode |= MODE_DIRTY)

static int
cross_post(int ent, fileheader_t * fhdr, char *direct)
{
    char            xboard[20], fname[80], xfpath[80], xtitle[80], inputbuf[10];
    fileheader_t    xfile;
    FILE           *xptr;
    int             author = 0;
    char            genbuf[200];
    char            genbuf2[4];
    boardheader_t  *bp;
    if (!(currmode & MODE_POST)) {
	move(5, 10);
	outs("對不起，您目前無法轉錄文章！");
	pressanykey();
	return FULLUPDATE;
    }
    move(2, 0);
    clrtoeol();
    move(3, 0);
    clrtoeol();
    move(1, 0);
    bp = getbcache(currbid);
    if (bp && (bp->brdattr & BRD_VOTEBOARD) )
	return FULLUPDATE;
    generalnamecomplete("轉錄本文章於看板：", xboard, sizeof(xboard),
			SHM->Bnumber,
			completeboard_compar,
			completeboard_permission,
			completeboard_getname);
    if (*xboard == '\0' || !haspostperm(xboard))
	return FULLUPDATE;

    if ((ent = str_checksum(fhdr->title)) != 0 &&
	ent == postrecord.checksum[0]) {
	/* 檢查 cross post 次數 */
	if (postrecord.times++ > MAX_CROSSNUM)
	    anticrosspost();
    } else {
	postrecord.times = 0;
	postrecord.checksum[0] = ent;
    }

    ent = 1;
    if (HAS_PERM(PERM_SYSOP) || !strcmp(fhdr->owner, cuser.userid)) {
	getdata(2, 0, "(1)原文轉載 (2)舊轉錄格式？[1] ",
		genbuf, 3, DOECHO);
	if (genbuf[0] != '2') {
	    ent = 0;
	    getdata(2, 0, "保留原作者名稱嗎?[Y] ", inputbuf, 3, DOECHO);
	    if (inputbuf[0] != 'n' && inputbuf[0] != 'N')
		author = 1;
	}
    }
    if (ent)
	snprintf(xtitle, sizeof(xtitle), "[轉錄]%.66s", fhdr->title);
    else
	strlcpy(xtitle, fhdr->title, sizeof(xtitle));

    snprintf(genbuf, sizeof(genbuf), "採用原標題《%.60s》嗎?[Y] ", xtitle);
    getdata(2, 0, genbuf, genbuf2, 4, LCECHO);
    if (genbuf2[0] == 'n' || genbuf2[0] == 'N') {
	if (getdata_str(2, 0, "標題：", genbuf, TTLEN, DOECHO, xtitle))
	    strlcpy(xtitle, genbuf, sizeof(xtitle));
    }
    getdata(2, 0, "(S)存檔 (L)站內 (Q)取消？[Q] ", genbuf, 3, LCECHO);
    if (genbuf[0] == 'l' || genbuf[0] == 's') {
	int             currmode0 = currmode;

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
	strlcpy(xfpath, currboard, sizeof(xfpath));
	strlcpy(currboard, xboard, sizeof(currboard));
	write_header(xptr);
	strlcpy(currboard, xfpath, sizeof(currboard));

	fprintf(xptr, "※ [本文轉錄自 %s 看板]\n\n", currboard);

	b_suckinfile(xptr, fname);
	addsignature(xptr, 0);
	fclose(xptr);
	/*
	 * Cross fs有問題 } else { unlink(xfpath); link(fname, xfpath); }
	 */
	setbdir(fname, xboard);
	append_record(fname, &xfile, sizeof(xfile));
	bp = getbcache(getbnum(xboard));
	if (!xfile.filemode && !(bp->brdattr & BRD_NOTRAN))
	    outgo_post(&xfile, xboard);
	setbtotal(getbnum(xboard));
	cuser.numposts++;
	UPDATE_USEREC;
	outs("文章轉錄完成");
	pressanykey();
	currmode = currmode0;
    }
    return FULLUPDATE;
}
static int
read_post(int ent, fileheader_t * fhdr, char *direct)
{
    char            genbuf[200];
    int             more_result;

    if (fhdr->owner[0] == '-')
	return DONOTHING;

    setdirpath(genbuf, direct, fhdr->filename);

    if ((more_result = more(genbuf, YEA)) == -1)
        return FULLUPDATE;

    brc_addlist(fhdr->filename);
    strncpy(currtitle, subject(fhdr->title), TTLEN);
    strncpy(currowner, subject(fhdr->owner), IDLEN + 2);

    switch (more_result) {
    case 1:
	return READ_PREV;
    case 2:
	return RELATE_PREV;
    case 3:
	return READ_NEXT;
    case 4:
	return RELATE_NEXT;
    case 5:
	return RELATE_FIRST;
    case 6:
	return FULLUPDATE;
    case 7:
    case 8:
	if ((currmode & MODE_POST)) {
	    strlcpy(quote_file, genbuf, sizeof(quote_file));
	    do_reply(fhdr);
	    *quote_file = 0;
	}
	return FULLUPDATE;
    case 9:
	return 'A';
    case 10:
	return 'a';
    case 11:
	return '/';
    case 12:
	return '?';
    }


    outmsg("\033[34;46m  閱\讀文章  \033[31;47m  (R/Y)\033[30m回信 \033[31m"
	 "(=[]<>)\033[30m相關主題 \033[31m(↑↓)\033[30m上下封 \033[31m(←)"
	   "\033[30m離開  \033[m");

    switch (egetch()) {
    case 'q':
    case 'Q':
    case KEY_LEFT:
	break;

    case ' ':
    case KEY_RIGHT:
    case KEY_DOWN:
    case KEY_PGDN:
    case 'n':
    case Ctrl('N'):
	return READ_NEXT;

    case KEY_UP:
    case 'p':
    case Ctrl('P'):
    case KEY_PGUP:
	return READ_PREV;

    case '=':
	return RELATE_FIRST;

    case ']':
    case 't':
	return RELATE_NEXT;

    case '[':
	return RELATE_PREV;

    case '.':
    case '>':
	return THREAD_NEXT;

    case ',':
    case '<':
	return THREAD_PREV;

    case Ctrl('I'):
	t_idle();
	return FULLUPDATE;
	
    case 'X':
	recommend(ent, fhdr, direct);
	return FULLUPDATE;

    case 'y':
    case 'r':
    case 'R':
    case 'Y':
	if ((currmode & MODE_POST)) {
	    strlcpy(quote_file, genbuf, sizeof(quote_file));
	    do_reply(fhdr);
	    *quote_file = 0;
	}
    }
    return FULLUPDATE;
}

/* ----------------------------------------------------- */
/* 採集精華區                                            */
/* ----------------------------------------------------- */
static int
b_man()
{
    char            buf[64];

    setapath(buf, currboard);
    if ((currmode & MODE_BOARD) || HAS_PERM(PERM_SYSOP)) {
	char            genbuf[128];
	int             fd;
	snprintf(genbuf, sizeof(genbuf), "%s/.rebuild", buf);
	if ((fd = open(genbuf, O_CREAT, 0640)) > 0)
	    close(fd);
    }
    return a_menu(currboard, buf, HAS_PERM(PERM_ALLBOARD) ? 2 :
		  (currmode & MODE_BOARD ? 1 : 0));
}

#ifndef NO_GAMBLE
static int
stop_gamble()
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
join_gamble(int ent, fileheader_t * fhdr, char *direct)
{
    if (!HAS_PERM(PERM_LOGINOK))
	return DONOTHING;
    if (stop_gamble()) {
	vmsg("目前未舉辦賭盤或賭盤已開獎");
	return DONOTHING;
    }
    ticket(currbid);
    return FULLUPDATE;
}
static int
hold_gamble(int ent, fileheader_t * fhdr, char *direct)
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
	getdata(b_lines - 1, 0, "已經有舉辦賭盤, "
		"是否要 [停止下注]?(N/y)：", yn, 3, LCECHO);
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
	getdata(b_lines - 1, 0, "已經有舉辦賭盤, "
		"是否要開獎 [否/是]?(N/y)：", yn, 3, LCECHO);
	if (yn[0] != 'y')
	    return FULLUPDATE;
	openticket(currbid);
	return FULLUPDATE;
    } else if (dashf(genbuf)) {
	move(b_lines - 1, 0);
	prints(" 目前系統正在處理開獎事宜, 請結果出爐後再舉辦.......");
	pressanykey();
	return FULLUPDATE;
    }
    getdata(b_lines - 2, 0, "要舉辦賭盤 (N/y):", yn, 3, LCECHO);
    if (yn[0] != 'y')
	return FULLUPDATE;
    getdata(b_lines - 1, 0, "賭什麼? 請輸入主題 (輸入後編輯內容):",
	    msg, 20, DOECHO);
    if (msg[0] == 0 ||
	vedit(fn_ticket_end, NA, NULL) < 0)
	return FULLUPDATE;

    clear();
    showtitle("舉辦賭盤", BBSNAME);
    setbfile(genbuf, currboard, FN_TICKET_ITEMS);

    //sprintf(genbuf, "%s/" FN_TICKET_ITEMS, direct);

    if (!(fp = fopen(genbuf, "w")))
	return FULLUPDATE;
    do {
	getdata(2, 0, "輸入彩票價格 (價格:10-10000):", yn, 6, LCECHO);
	i = atoi(yn);
    } while (i < 10 || i > 10000);
    fprintf(fp, "%d\n", i);
    if (!getdata(3, 0, "設定自動封盤時間?(Y/n)", yn, 3, LCECHO) || yn[0] != 'n') {
	bp->endgamble = gettime(4, now, "封盤於");
	substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
    }
    move(6, 0);
    snprintf(genbuf, sizeof(genbuf),
	     "請到 %s 板 按'f'參與賭博!\n\n一張 %d Ptt幣, 這是%s的賭博\n%s%s\n",
	     currboard,
	     i, i < 100 ? "小賭式" : i < 500 ? "平民級" :
	     i < 1000 ? "貴族級" : i < 5000 ? "富豪級" : "傾家蕩產",
	     bp->endgamble ? "賭盤結束時間: " : "",
	     bp->endgamble ? Cdate(&bp->endgamble) : ""
	     );
    strcat(msg, genbuf);
    prints("請依次輸入彩票名稱, 需提供2~8項. (未滿八項, 輸入直接按enter)\n");
    for (i = 0; i < 8; i++) {
	snprintf(yn, sizeof(yn), " %d)", i + 1);
	getdata(7 + i, 0, yn, genbuf, 9, DOECHO);
	if (!genbuf[0] && i > 1)
	    break;
	fprintf(fp, "%s\n", genbuf);
    }
    fclose(fp);
    move(8 + i, 0);
    prints("賭盤設定完成");
    snprintf(genbuf, sizeof(genbuf), "[公告] %s 板 開始賭博!", currboard);
    post_msg(currboard, genbuf, msg, cuser.userid);
    post_msg("Record", genbuf + 7, msg, "[馬路探子]");
    /* Tim 控制CS, 以免正在玩的user把資料已經寫進來 */
    rename(fn_ticket_end, fn_ticket);
    /* 設定完才把檔名改過來 */

    return FULLUPDATE;
}
#endif

static int
cite_post(int ent, fileheader_t * fhdr, char *direct)
{
    char            fpath[256];
    char            title[TTLEN + 1];

    setbfile(fpath, currboard, fhdr->filename);
    strlcpy(title, "◇ ", sizeof(title));
    strlcpy(title + 3, fhdr->title, TTLEN - 3);
    title[TTLEN] = '\0';
    a_copyitem(fpath, title, 0, 1);
    b_man();
    return FULLUPDATE;
}

int
edit_title(int ent, fileheader_t * fhdr, char *direct)
{
    char            genbuf[200];
    fileheader_t    tmpfhdr = *fhdr;
    int             dirty = 0;

    if (currmode & MODE_BOARD || !strcmp(cuser.userid, fhdr->owner)) {
	if (getdata(b_lines - 1, 0, "標題：", genbuf, TTLEN, DOECHO)) {
	    strlcpy(tmpfhdr.title, genbuf, sizeof(tmpfhdr.title));
	    dirty++;
	}
    }
    if (HAS_PERM(PERM_SYSOP)) {
	if (getdata(b_lines - 1, 0, "作者：", genbuf, IDLEN + 2, DOECHO)) {
	    strlcpy(tmpfhdr.owner, genbuf, sizeof(tmpfhdr.owner));
	    dirty++;
	}
	if (getdata(b_lines - 1, 0, "日期：", genbuf, 6, DOECHO)) {
	    snprintf(tmpfhdr.date, sizeof(tmpfhdr.date), "%.5s", genbuf);
	    dirty++;
	}
    }
    if (currmode & MODE_BOARD || !strcmp(cuser.userid, fhdr->owner)) {
	getdata(b_lines - 1, 0, "確定(Y/N)?[n] ", genbuf, 3, DOECHO);
	if ((genbuf[0] == 'y' || genbuf[0] == 'Y') && dirty) {
	    *fhdr = tmpfhdr;
	    substitute_record(direct, fhdr, sizeof(*fhdr), ent);
	    /* rocker.011018: 這裡應該改成用reference的方式取得原來的檔案 */
	    substitute_check(fhdr);
	    touchdircache(currbid);
	}
	return FULLUPDATE;
    }
    return DONOTHING;
}

static int
solve_post(int ent, fileheader_t * fhdr, char *direct)
{
    if (HAS_PERM(PERM_SYSOP)) {
	fhdr->filemode ^= FILE_SOLVED;
	substitute_record(direct, fhdr, sizeof(*fhdr), ent);
	touchdircache(currbid);
	return PART_REDRAW;
    }
    return DONOTHING;
}

static int
recommend_cancel(int ent, fileheader_t * fhdr, char *direct)
{
    char            yn[5];
    if (!(currmode & MODE_BOARD))
	return DONOTHING;
    getdata(b_lines - 1, 0, "確定要推薦歸零(Y/N)?[n] ", yn, 5, LCECHO);
    if (yn[0] != 'y')
	return FULLUPDATE;
#ifdef ASSESS
    // to save resource
    if (fhdr->recommend > 9)
	inc_goodpost(searchuser(fhdr->owner), -1 * (fhdr->recommend / 10));
#endif
    fhdr->recommend = 0;

    substitute_record(direct, fhdr, sizeof(*fhdr), ent);
    substitute_check(fhdr);
    touchdircache(currbid);
    return FULLUPDATE;
}
static int
do_add_recommend(char *direct, fileheader_t *fhdr, int ent, char *buf)
{
    char    path[256];
    int     fd;
    /*
      race here:
      為了減少 system calls , 現在直接用當前的推文數 +1 寫入 .DIR 中.
      造成
      1.若該文檔名被換掉的話, 推文將寫至舊檔名中 (造成幽靈檔)
      2.沒有重新讀一次, 所以推文數可能被少算
      3.若推的時候前文被刪, 將加到後文的推文數
     */
    setdirpath(path, direct, fhdr->filename);
    if( log_file(path, buf, 0) == -1 ){ // 不 CREATE
	vmsg("推薦/競標失敗");
	return -1;
    }

    /* get_record(direct, fhdr, sizeof(fhdr), ent);
     * This is a solution to avoid most racing (still some), but cost four
     * system calls.                                                        */

    if( fhdr->recommend < 100 ){
	fileheader_t t;
	if( (fd = open(direct, O_WRONLY)) < 0 )
	    return -1;

	++(fhdr->recommend);
	if( lseek(fd, (off_t)(sizeof(*fhdr) * (ent - 1) +
			(int)&t.recommend - (int)&t), SEEK_SET) >= 0)
	    // 如果 lseek 失敗就不會 write
	    write(fd, &fhdr->recommend, sizeof(char));

	close(fd);
	touchdircache(currbid);
    }
    return 0;
}
static int
do_bid(int ent, fileheader_t * fhdr, boardheader_t  *bp, char *direct,  struct tm      *ptime)
{
    char            genbuf[200], fpath[256],say[30],*money;
    bid_t           bidinfo;
    int mymax, next;

    setdirpath(fpath, direct, fhdr->filename);
    strcat(fpath,".bid");
    get_record(fpath, &bidinfo, sizeof(bidinfo), 1);

    move(18,0); clrtobot();
    prints("競標主題: %s\n", fhdr->title);
    print_bidinfo(0, bidinfo);
    if(!bidinfo.payby) money="Ptt$ "; else money=" NT$ ";
    if(now>bidinfo.enddate || bidinfo.high==bidinfo.buyitnow)
    {
	prints("此競標已經結束,");
	if( bidinfo.userid[0]) {
	    /*if(!payby && bidinfo.usermax!=-1)
	      {以Ptt幣自動扣款
	      }*/
	    prints("恭喜 %s 以 %d 得標!", bidinfo.userid, 
		    bidinfo.high);
#ifdef ASSESS
	    if (!(bidinfo.flag & SALE_COMMENTED) && strcmp(bidinfo.userid, currutmp->userid) == 0){
		char tmp = getans("您對於這次交易的評價如何? 1:佳 2:欠佳 3:普通[Q]");
		if ('1' <= tmp && tmp <= '3'){
		    switch(tmp){
			case 1:
			    inc_goodsale(currutmp->uid, 1);
			    break;
			case 2:
			    inc_badpost(currutmp->uid, 1);
			    break;
		    }
		    bidinfo.flag |= SALE_COMMENTED;
		    substitute_record(fpath, &bidinfo, sizeof(bidinfo), 1);
		}
	    }
#endif
	}
	else prints("無人得標!");
	pressanykey();
	return FULLUPDATE;
    }
    if(bidinfo.userid[0])
    {
        prints("下次出價至少要:%s%d", money,bidinfo.high + bidinfo.increment);
	if(bidinfo.buyitnow)
	     prints(" (輸入 %d 等於以直接購買結束)",bidinfo.buyitnow);
	next=bidinfo.high + bidinfo.increment;
    }
    else
    {
        prints("起標價: %d", bidinfo.high);
	next=bidinfo.high;
    }
    if(!strcmp(cuser.userid,bidinfo.userid))
    {
	prints("你是最高得標者!");
        pressanykey();
	return FULLUPDATE;
    }
    if (strcmp(cuser.userid, fhdr->owner) == 0){
	vmsg("警告! 本人不能出價!");
	return FULLUPDATE;
    }
    getdata_str(23,0,"是否要下標? (y/N)", genbuf, 3, LCECHO,"n");
    if(genbuf[0]!='y') return FULLUPDATE;

    getdata(23, 0, "您的最高下標金額(0:取消):", genbuf, 10, LCECHO);

    mymax=atoi(genbuf);

    getdata(23,0,"下標感言:",say,12,DOECHO);

    get_record(fpath, &bidinfo, sizeof(bidinfo), 1);

    if(bidinfo.buyitnow && mymax>bidinfo.buyitnow)
        mymax=bidinfo.buyitnow;
    else if(!bidinfo.userid[0])
	next=bidinfo.high;
    else
	next=bidinfo.high + bidinfo.increment;
    

    if(mymax< next || (bidinfo.payby==0 && cuser.money<mymax ))
    {
	vmsg("取消下標或標金不足搶標");
        return FULLUPDATE;
    }
    
    snprintf(genbuf, sizeof(genbuf),
 "\033[1;31m→ \033[33m%s\033[m\033[33m:%s\033[m%*s %s%-15d標%15s %02d/%02d\n",
	     cuser.userid,say,
	     31 - strlen(cuser.userid) - strlen(say), " ", 
             money,
	     next, fromhost,
	     ptime->tm_mon + 1, ptime->tm_mday);
    do_add_recommend(direct, fhdr,  ent, genbuf);
    if(next > bidinfo.usermax)
    {
       bidinfo.usermax=mymax;
       bidinfo.high=next;
       strcpy(bidinfo.userid,cuser.userid);
    }
    else if(mymax>bidinfo.usermax)
    {
	bidinfo.high=bidinfo.usermax+bidinfo.increment;
        if(bidinfo.high>mymax) bidinfo.high=mymax; 
	bidinfo.usermax=mymax;
        strcpy(bidinfo.userid,cuser.userid);
	
        snprintf(genbuf, sizeof(genbuf),
"\033[1;31m→ \033[33m自動競標%s勝出\033[m\033[33m\033[m%*s%s%-15d標 %02d/%02d\n",
	     cuser.userid, 
	     20 - strlen(cuser.userid) , " ",money, 
	     bidinfo.high, 
	     ptime->tm_mon + 1, ptime->tm_mday);
        do_add_recommend(direct, fhdr,  ent, genbuf);
    }
    else
    {
	 if(mymax+bidinfo.increment<bidinfo.usermax)
	  bidinfo.high=mymax+bidinfo.increment;
	 else
	  bidinfo.high=bidinfo.usermax; /*這邊怪怪的*/ 
        snprintf(genbuf, sizeof(genbuf),
"\033[1;31m→ \033[33m自動競標%s勝出\033[m\033[33m\033[m%*s%s%-15d標 %02d/%02d\n",
	     bidinfo.userid, 
	     20 - strlen(bidinfo.userid) , " ", money, 
	     bidinfo.high,
	     ptime->tm_mon + 1, ptime->tm_mday);
        do_add_recommend(direct, fhdr,  ent, genbuf);
    }
    substitute_record(fpath, &bidinfo, sizeof(bidinfo), 1);
    vmsg("恭喜您! 以最高價搶標完成!");
    return FULLUPDATE;
}

static int
recommend(int ent, fileheader_t * fhdr, char *direct)
{
    struct tm      *ptime = localtime(&now);
    char            buf[200], path[200], yn[5];
    boardheader_t  *bp;
    static time_t   lastrecommend = 0;

    bp = getbcache(currbid);
    if( bp->brdattr & BRD_NORECOMMEND ){
	vmsg("抱歉, 本板禁止推薦或競標");
	return FULLUPDATE;
    }
    if (!(currmode & MODE_POST) || bp->brdattr & BRD_VOTEBOARD || fhdr->filemode & FILE_VOTE) {
	vmsg("您權限不足, 無法推薦!");
	return FULLUPDATE;
    }
#ifdef SAFE_ARTICLE_DELETE
    if( fhdr->filename[0] == '.' ){
	vmsg("本文已刪除");
	return FULLUPDATE;
    }
#endif

    if( fhdr->filemode & FILE_BID){
	return do_bid(ent, fhdr, bp, direct, ptime);
    }
    setdirpath(path, direct, fhdr->filename);

    if (fhdr->recommend == 0 && strcmp(cuser.userid, fhdr->owner) == 0){
	vmsg("警告! 本人不能推薦第一次!");
	return FULLUPDATE;
    }
#ifndef DEBUG
    if (!(currmode & MODE_BOARD) && getuser(cuser.userid) &&
	now - lastrecommend < 40) {
	move(b_lines - 1, 0);
	prints("離上次推薦時間太近囉, 請多花點時間仔細閱\讀文章!");
	pressanykey();
	return FULLUPDATE;
    }
#endif


    if (!getdata(b_lines - 2, 0, "推薦語:", path, 40, DOECHO) ||
	    path == NULL ||
	!getdata(b_lines - 1, 0, "確定要推薦, 請仔細考慮(Y/N)?[n] ",
		 yn, 5, LCECHO)
	|| yn[0] != 'y')
	return FULLUPDATE;

    snprintf(buf, sizeof(buf),
	     "\033[1;31m→ \033[33m%s\033[m\033[33m:%s\033[m%*s推%15s %02d/%02d\n",
	     cuser.userid, path,
	     51 - strlen(cuser.userid) - strlen(path), " ", fromhost,
	     ptime->tm_mon + 1, ptime->tm_mday);
    do_add_recommend(direct, fhdr,  ent, buf);
#ifdef ASSESS
    /* 每 10 次推文 加一次 goodpost */
    if ((fhdr->filemode & FILE_MARKED) && fhdr->recommend % 10 == 0) {
	int uid = searchuser(fhdr->owner);
	if (uid > 0)
	    inc_goodpost(uid, 1);
    }
#endif
    lastrecommend = now;
    return FULLUPDATE;
}

static int
mark_post(int ent, fileheader_t * fhdr, char *direct)
{
    char buf[STRLEN], fpath[STRLEN];

    if (!(currmode & MODE_BOARD))
	return DONOTHING;

    setbpath(fpath, currboard);
    sprintf(buf, "%s/%s", fpath, fhdr->filename);

    if(access(buf, F_OK) < 0)
	return DONOTHING;

    fhdr->filemode ^= FILE_MARKED;

#ifdef ASSESS
    if (!(fhdr->filemode & FILE_BID)){
	if (fhdr->filemode & FILE_MARKED) {
	    if (!(currbrdattr & BRD_BAD) && fhdr->recommend >= 10)
		inc_goodpost(searchuser(fhdr->owner), fhdr->recommend / 10);
	}
	else if (fhdr->recommend > 9)
    	    inc_goodpost(searchuser(fhdr->owner), -1 * (fhdr->recommend / 10));
    }
#endif

    substitute_record(direct, fhdr, sizeof(*fhdr), ent);
    substitute_check(fhdr);
    touchdircache(currbid);
    return PART_REDRAW;
}

int
del_range(int ent, fileheader_t *fhdr, char *direct)
{
    char            num1[8], num2[8];
    int             inum1, inum2;

    /* 有三種情況會進這裡, 信件, 看板, 精華區 */
    if( !(direct[0] == 'h') ){ /* 信件不用 check */
	boardheader_t  *bp = getbcache(currbid);
	if (strcmp(bp->brdname, "Security") == 0)
	    return DONOTHING;
    }

    /* rocker.011018: 串接模式下還是不允許刪除比較好 */
    if (currmode & MODE_SELECT) {
	outmsg("請先回到正常模式後再進行刪除...");
	refresh();
	/* safe_sleep(1); */
	return FULLUPDATE;
    }

    if ((currstat != READING) || (currmode & MODE_BOARD)) {
	getdata(1, 0, "[設定刪除範圍] 起點：", num1, 6, DOECHO);
	inum1 = atoi(num1);
	if (inum1 <= 0) {
	    outmsg("起點有誤");
	    refresh();
	    /* safe_sleep(1); */
	    return FULLUPDATE;
	}
	getdata(1, 28, "終點：", num2, 6, DOECHO);
	inum2 = atoi(num2);
	if (inum2 < inum1) {
	    outmsg("終點有誤");
	    refresh();
	    /* safe_sleep(1); */
	    return FULLUPDATE;
	}
	getdata(1, 48, msg_sure_ny, num1, 3, LCECHO);
	if (*num1 == 'y') {
	    outmsg("處理中,請稍後...");
	    refresh();
	    if (currmode & MODE_SELECT) {
		int             fd, size = sizeof(fileheader_t);
		char            genbuf[100];
		fileheader_t    rsfh;
		int             i = inum1, now;
		if (currstat == RMAIL)
		    sethomedir(genbuf, cuser.userid);
		else
		    setbdir(genbuf, currboard);
		if ((fd = (open(direct, O_RDONLY, 0))) != -1) {
		    if (lseek(fd, (off_t) (size * (inum1 - 1)), SEEK_SET) !=
			-1) {
			while (read(fd, &rsfh, size) == size) {
			    if (i > inum2)
				break;
			    now = getindex(genbuf, rsfh.filename, size);
			    strlcpy(currfile, rsfh.filename, sizeof(currfile));
			    delete_record(genbuf, sizeof(fileheader_t), now);
			//		cmpfilename);
			    i++;
			}
		    }
		    close(fd);
		}
	    }
#ifdef SAFE_ARTICLE_DELETE
	    if( direct[0] == 'b' )
		safe_article_delete_range(direct, inum1, inum2);
	    else
		delete_range(direct, inum1, inum2);
#else
	    delete_range(direct, inum1, inum2);
#endif
	    fixkeep(direct, inum1);

	    if (currmode & MODE_BOARD)
		setbtotal(currbid);

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
    int             not_owned;
    boardheader_t  *bp;

    bp = getbcache(currbid);
    if (strcmp(bp->brdname, "Security") == 0)
	return DONOTHING;
    if ((fhdr->filemode & FILE_MARKED) || (fhdr->filemode & FILE_DIGEST) ||
	(fhdr->owner[0] == '-'))
	return DONOTHING;

    not_owned = strcmp(fhdr->owner, cuser.userid);
    if ((!(currmode & MODE_BOARD) && not_owned) ||
	((bp->brdattr & BRD_VOTEBOARD) && !HAS_PERM(PERM_SYSOP)) ||
	!strcmp(cuser.userid, STR_GUEST))
	return DONOTHING;

    getdata(1, 0, msg_del_ny, genbuf, 3, LCECHO);
    if (genbuf[0] == 'y') {
	strlcpy(currfile, fhdr->filename, sizeof(currfile));
	if(
#ifdef SAFE_ARTICLE_DELETE
	   !safe_article_delete(ent, fhdr, direct)
#else
	   !delete_record(direct, sizeof(fileheader_t), ent)
#endif
	   ) {
	    int num;
	    if (currmode & MODE_SELECT) {
		/* rocker.011018: 利用reference減低loading */
		fileheader_t    hdr;

		/* fhdr->money is not real money now, it's a reference instead.
		 * see select_read() in read.c */
		num = fhdr->money & ~FHR_REFERENCE;
		setbdir(genbuf, currboard);
		get_record(genbuf, &hdr, sizeof(hdr), num);

		/* 再這裡要check一下原來的dir裡面是不是有被人動過... */
		if (strcmp(hdr.filename, fhdr->filename)) {
		    num = getindex(genbuf, fhdr->filename, sizeof(fileheader_t));
		    get_record(genbuf, &hdr, sizeof(hdr), num);
		}
		/* rocker.011018: 這裡要還原被破壞的money */
		fhdr->money = hdr.money;
		delete_record(genbuf, sizeof(fileheader_t), num);
	    }

	    cancelpost(fhdr, not_owned, newpath);
            if(fhdr->filemode & FILE_ANONYMOUS)
		/* When the file is anonymous posted, fhdr->money is author.
		 * see do_general() */
                num = fhdr->money;
            else
	        num = searchuser(fhdr->owner);
#ifdef ASSESS
#define SIZE	sizeof(badpost_reason) / sizeof(char *)

	    if (not_owned && num > 0 && !(currmode & MODE_DIGEST)) {
                getdata(1, 40, "惡劣文章?(y/N)", genbuf, 3, LCECHO);
		if(genbuf[0]=='y') {
		    int i;
		    char reason[64];
		    move(b_lines - 2, 0);
		    for (i = 0; i < SIZE; i++)
			prints("%d.%s ", i + 1, badpost_reason[i]);
		    prints("%d.%s", i + 1, "其他");
		    getdata(b_lines - 1, 0, "請選擇", reason, sizeof(reason), LCECHO);
		    i = reason[0] - '0';
		    if (i <= 0 || i > SIZE)
			getdata(b_lines, 0, "請輸入原因", reason, sizeof(reason), DOECHO);
		    else
			strcpy(reason, badpost_reason[i - 1]);
		    if (!(inc_badpost(num, 1) % 10)){
			post_violatelaw(xuser.userid, "Ptt 系統警察", "劣文累計十篇", "罰單一張");
			mail_violatelaw(xuser.userid, "Ptt 系統警察", "劣文累計十篇", "罰單一張");
			xuser.userlevel |= PERM_VIOLATELAW;
		    }
		    sprintf(genbuf,"劣文退回(%s):%-40.40s", reason, fhdr->title);
		    mail_id(xuser.userid, genbuf, newpath, cuser.userid);
		}
	    }
#undef SIZE
#endif

	    setbtotal(currbid);
	    if (fhdr->money < 0 || fhdr->filemode & FILE_ANONYMOUS)
		fhdr->money = 0;
	    if (not_owned && strcmp(currboard, "Test")) {
		deumoney(num, -fhdr->money);
	    }
	    if (!not_owned && strcmp(currboard, "Test")) {
		if (cuser.numposts)
		    cuser.numposts--;
		/* XXX: is_BM(cuser.userid) is always true @_@ */
		if (!(currmode & MODE_DIGEST && currmode & MODE_BOARD)){
		    move(b_lines - 1, 0);
		    clrtoeol();
		    demoney(-fhdr->money);
		    passwd_update(usernum, &cuser);	/* post 數 */
		    prints("%s，您的文章減為 %d 篇，支付清潔費 %d 銀", msg_del_ok,
			    cuser.numposts, fhdr->money);
		    refresh();
		    pressanykey();
		}
	    }
	    return DIRCHANGED;
	}
    }
    return FULLUPDATE;
}

static int
view_postmoney(int ent, fileheader_t * fhdr, char *direct)
{
    move(b_lines - 1, 0);
    if(currmode & MODE_SELECT){
	vmsg("請在離開目前的選擇模式再查詢");
	return FULLUPDATE;
    }
    clrtoeol();
    if(fhdr->filemode & FILE_ANONYMOUS)
	/* When the file is anonymous posted, fhdr->money is author.
	 * see do_general() */
	prints("匿名管理編號: %d (同一人被查詢時編號相同, 此編號每人看到不相同)",
		fhdr->money + currutmp->pid);
    else
	prints("這一篇文章值 %d 銀", fhdr->money);
    refresh();
    pressanykey();
    return FULLUPDATE;
}

#ifdef OUTJOBSPOOL
/* 看板備份 */
static int
tar_addqueue(int ent, fileheader_t * fhdr, char *direct)
{
    char            email[60], qfn[80], ans[2];
    FILE           *fp;
    char            bakboard, bakman;
    clear();
    showtitle("看板備份", BBSNAME);
    move(2, 0);
    if (!((currmode & MODE_BOARD) || HAS_PERM(PERM_SYSOP))) {
	move(5, 10);
	outs("妳要是板主或是站長才能醬醬啊 -.-\"\"");
	pressanykey();
	return FULLUPDATE;
    }
    snprintf(qfn, sizeof(qfn), BBSHOME "/jobspool/tarqueue.%s", currboard);
    if (access(qfn, 0) == 0) {
	outs("已經排定行程, 稍後會進行備份");
	pressanykey();
	return FULLUPDATE;
    }
    if (!getdata(4, 0, "請輸入目的信箱：", email, sizeof(email), DOECHO))
	return FULLUPDATE;

    /* check email -.-"" */
    if (strstr(email, "@") == NULL || strstr(email, ".bbs@") != NULL) {
	move(6, 0);
	outs("您指定的信箱不正確! ");
	pressanykey();
	return FULLUPDATE;
    }
    getdata(6, 0, "要備份看板內容嗎(Y/N)?[Y]", ans, sizeof(ans), LCECHO);
    bakboard = (ans[0] == 'n' || ans[0] == 'N') ? 0 : 1;
    getdata(7, 0, "要備份精華區內容嗎(Y/N)?[N]", ans, sizeof(ans), LCECHO);
    bakman = (ans[0] == 'y' || ans[0] == 'Y') ? 1 : 0;
    if (!bakboard && !bakman) {
	move(8, 0);
	outs("可是我們只能備份看板或精華區的耶 ^^\"\"\"");
	pressanykey();
	return FULLUPDATE;
    }
    fp = fopen(qfn, "w");
    fprintf(fp, "%s\n", cuser.userid);
    fprintf(fp, "%s\n", email);
    fprintf(fp, "%d,%d\n", bakboard, bakman);
    fclose(fp);

    move(10, 0);
    outs("系統已經將您的備份排入行程, \n");
    outs("稍後將會在系統負荷較低的時候將資料寄給您~ :) ");
    pressanykey();
    return FULLUPDATE;
}
#endif

static int      sequent_ent;
static int      continue_flag;

/* ----------------------------------------------------- */
/* 依序讀新文章                                          */
/* ----------------------------------------------------- */
static int
sequent_messages(fileheader_t * fptr)
{
    static int      idc;
    char            genbuf[200];

    if (fptr == NULL)
	return (idc = 0);

    if (++idc < sequent_ent)
	return 0;

    if (!brc_unread(fptr->filename, brc_num, brc_list))
	return 0;

    if (continue_flag)
	genbuf[0] = 'y';
    else {
	prints("讀取文章於：[%s] 作者：[%s]\n標題：[%s]",
	       currboard, fptr->owner, fptr->title);
	getdata(3, 0, "(Y/N/Quit) [Y]: ", genbuf, 3, LCECHO);
    }

    if (genbuf[0] != 'y' && genbuf[0]) {
	clear();
	return (genbuf[0] == 'q' ? QUIT : 0);
    }
    setbfile(genbuf, currboard, fptr->filename);
    brc_addlist(fptr->filename);

    if (more(genbuf, YEA) == 0)
	outmsg("\033[31;47m  \033[31m(R)\033[30m回信  \033[31m(↓,n)"
	       "\033[30m下一封  \033[31m(←,q)\033[30m離開  \033[m");
    continue_flag = 0;

    switch (egetch()) {
    case KEY_LEFT:
    case 'e':
    case 'q':
    case 'Q':
	break;

    case 'y':
    case 'r':
    case 'Y':
    case 'R':
	if (currmode & MODE_POST) {
	    strlcpy(quote_file, genbuf, sizeof(quote_file));
	    do_reply(fptr);
	    *quote_file = 0;
	}
	break;

    case ' ':
    case KEY_DOWN:
    case '\n':
    case 'n':
	continue_flag = 1;
    }

    clear();
    return 0;
}

static int
sequential_read(int ent, fileheader_t * fhdr, char *direct)
{
    char            buf[40];

    clear();
    sequent_messages((fileheader_t *) NULL);
    sequent_ent = ent;
    continue_flag = 0;
    setbdir(buf, currboard);
    apply_record(buf, sequent_messages, sizeof(fileheader_t));
    return FULLUPDATE;
}

/* ----------------------------------------------------- */
/* 看板備忘錄、文摘、精華區                              */
/* ----------------------------------------------------- */
int
b_note_edit_bname(int bid)
{
    char            buf[64];
    int             aborted;
    boardheader_t  *fh = getbcache(bid);
    setbfile(buf, fh->brdname, fn_notes);
    aborted = vedit(buf, NA, NULL);
    if (aborted == -1) {
	clear();
	outs(msg_cancel);
	pressanykey();
    } else {
	if (!getdata(2, 0, "設定有效期限天？(n/Y)", buf, 3, LCECHO)
	    || buf[0] != 'n')
	    fh->bupdate = gettime(3, fh->bupdate ? fh->bupdate : now, 
		      "有效日期至");
	else
	    fh->bupdate = 0;
	substitute_record(fn_board, fh, sizeof(boardheader_t), bid);
    }
    return 0;
}

static int
b_notes_edit()
{
    if (currmode & MODE_BOARD) {
	b_note_edit_bname(currbid);
	return FULLUPDATE;
    }
    return 0;
}

static int
b_water_edit()
{
    if (currmode & MODE_BOARD) {
	friend_edit(BOARD_WATER);
	return FULLUPDATE;
    }
    return 0;
}

static int
visable_list_edit()
{
    if (currmode & MODE_BOARD) {
	friend_edit(BOARD_VISABLE);
	hbflreload(currbid);
	return FULLUPDATE;
    }
    return 0;
}

static int
b_post_note()
{
    char            buf[200], yn[3];
    if (currmode & MODE_BOARD) {
	setbfile(buf, currboard, FN_POST_NOTE);
	if (more(buf, NA) == -1)
	    more("etc/" FN_POST_NOTE, NA);
	getdata(b_lines - 2, 0, "是否要用自訂post注意事項?", yn, sizeof(yn), LCECHO);
	if (yn[0] == 'y')
	    vedit(buf, NA, NULL);
	else
	    unlink(buf);


	setbfile(buf, currboard, FN_POST_BID);
	if (more(buf, NA) == -1)
	    more("etc/" FN_POST_BID, NA);
	getdata(b_lines - 2, 0, "是否要用自訂競標文章注意事項?", yn, sizeof(yn), LCECHO);
	if (yn[0] == 'y')
	    vedit(buf, NA, NULL);
	else
	    unlink(buf);

	return FULLUPDATE;
    }
    return 0;
}


static int
can_vote_edit()
{
    if (currmode & MODE_BOARD) {
	friend_edit(FRIEND_CANVOTE);
	return FULLUPDATE;
    }
    return 0;
}

static int
bh_title_edit()
{
    boardheader_t  *bp;

    if (currmode & MODE_BOARD) {
	char            genbuf[BTLEN];

	bp = getbcache(currbid);
	move(1, 0);
	clrtoeol();
	getdata_str(1, 0, "請輸入看板新中文敘述:", genbuf,
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
b_notes()
{
    char            buf[64];

    setbfile(buf, currboard, fn_notes);
    if (more(buf, NA) == -1) {
	clear();
	move(4, 20);
	outs("本看板尚無「備忘錄」。");
    }
    pressanykey();
    return FULLUPDATE;
}

int
board_select()
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
board_digest()
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

int
board_etc()
{
    if (!HAS_PERM(PERM_SYSOP))
	return DONOTHING;
    currmode ^= MODE_ETC;
    if (currmode & MODE_ETC)
	currmode &= ~MODE_POST;
    else if (haspostperm(currboard))
	currmode |= MODE_POST;

    setbdir(currdirect, currboard);
    return NEWDIRECT;
}

static int
good_post(int ent, fileheader_t * fhdr, char *direct)
{
    char            genbuf[200];
    char            genbuf2[200];
    int             delta = 0;

    if ((currmode & MODE_DIGEST) || !(currmode & MODE_BOARD))
	return DONOTHING;

    getdata(1, 0, fhdr->filemode & FILE_DIGEST ?
             "取消看板文摘?(Y/n)" : "收入看板文摘?(Y/n)", genbuf2, 3, LCECHO);
    if(genbuf2[0] == 'n')
	return FULLUPDATE;

    if (fhdr->filemode & FILE_DIGEST) {
	fhdr->filemode = (fhdr->filemode & ~FILE_DIGEST);
	if (!strcmp(currboard, "Note") || !strcmp(currboard, "PttBug") ||
	    !strcmp(currboard, "Artdsn") || !strcmp(currboard, "PttLaw")) {
	    deumoney(searchuser(fhdr->owner), -1000);
	    if (!(currmode & MODE_SELECT))
		fhdr->money -= 1000;
	    else
		delta = -1000;
	}
    } else {
	fileheader_t    digest;
	char           *ptr, buf[64];

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
	Link(genbuf2, genbuf);
	strcpy(ptr, fn_mandex);
	append_record(buf, &digest, sizeof(digest));

#ifdef GLOBAL_DIGEST
	if(!(getbcache(currbid)->brdattr & BRD_HIDE)) { 
          getdata(1, 0, "好文值得出版到全站文摘?(N/y)", genbuf2, 3, LCECHO);
          if(genbuf2[0] == 'y')
	      do_crosspost(GLOBAL_DIGEST, &digest, genbuf);
        }
#endif

	fhdr->filemode = (fhdr->filemode & ~FILE_MARKED) | FILE_DIGEST;
	if (!strcmp(currboard, "Note") || !strcmp(currboard, "PttBug") ||
	    !strcmp(currboard, "Artdsn") || !strcmp(currboard, "PttLaw")) {
	    deumoney(searchuser(fhdr->owner), 1000);
	    if (!(currmode & MODE_SELECT))
		fhdr->money += 1000;
	    else
		delta = 1000;
	}
    }
    substitute_record(direct, fhdr, sizeof(*fhdr), ent);
    touchdircache(currbid);
    /* rocker.011018: 串接模式用reference增進效率 */
    if ((currmode & MODE_SELECT) && (fhdr->money & FHR_REFERENCE)) {
	fileheader_t    hdr;
	char            genbuf[100];
	int             num;

	num = fhdr->money & ~FHR_REFERENCE;
	setbdir(genbuf, currboard);
	get_record(genbuf, &hdr, sizeof(hdr), num);

	/* 再這裡要check一下原來的dir裡面是不是有被人動過... */
	if (strcmp(hdr.filename, fhdr->filename)) {
	    num = getindex(genbuf, fhdr->filename, sizeof(fileheader_t));
	    get_record(genbuf, &hdr, sizeof(hdr), num);
	}
	fhdr->money = hdr.money + delta;

	substitute_record(genbuf, fhdr, sizeof(*fhdr), num);
    }
    return FULLUPDATE;
}

/* help for board reading */
static char    *board_help[] = {
    "\0全功\能看板操作說明",
    "\01基本命令",
    "(p)(↑)   上移一篇文章         (^P/^O/^V)發表文章/競標/活動連署",
    "(n)(↓)   下移一篇文章         (d)       刪除文章",
    "(P)(PgUp) 上移一頁             (S)       串連相關文章",
    "(N)(PgDn) 下移一頁             (##)      跳到 ## 號文章",
    "(r)(→)   閱\讀此篇文章         ($)       跳到最後一篇文章",
    "\01進階命令",
    "(tab)/z   文摘模式/精華區      (a/A)(^Q) 找尋作者/作者資料",
    "(b/f)     展讀備忘錄/參與賭盤  (?)(/)    找尋標題",
    "(V/R)     投票/查詢投票結果    (^W)(X)   我在哪裡/推薦文章/參與競標",
    "(x)(w)    轉錄文章/丟水球      (=)/([]<>-+) 找尋首篇文章/主題式閱\讀",
#ifdef INTERNET_EMAIL
    "(F)       文章寄回Internet郵箱 (U)       將文章 uuencode 後寄回郵箱",
#endif
    "(E)/(Q)   重編文章/查詢價格/匿名(^H)      列出所有的主要標題",
    "\01板主命令",
    "(M/o)     舉行投票/編私投票名單 (m/c/g)  保留文章/選錄精華/文摘",
    "(D)       刪除一段範圍的文章    (T/B)    重編文章標題/重編看板標題",
    "(I)       開放/禁止看板推薦     (t/^D)   標記文章/砍除標記的文章",
    "(O)/(i)   發表注意事項 文章類別 (H)/(Y)  看板隱藏/現身 取消推薦文章",
#ifdef NO_GAMBLE
    "(W/K/v)    編進板畫面/水桶名單/可見名單",
#else
    "(^G)      舉辦賭盤/停止下注/開獎(W/K/v) 編進板畫面/水桶名單/可見名單",
#endif
    NULL
};

static int
b_help()
{
    show_help(board_help);
    return FULLUPDATE;
}

static int
b_changerecommend(int ent, fileheader_t * fhdr, char *direct)
{
    boardheader_t   *bp=NULL;
    if (!((currmode & MODE_BOARD) || HAS_PERM(PERM_SYSOP)))
	return DONOTHING;
    bp = getbcache(currbid); 
    bp->brdattr ^= BRD_NORECOMMEND; 
    substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
    vmsg("本板現在 %s 推薦",
         (bp->brdattr & BRD_NORECOMMEND) ? "禁止" : "開放");
    return FULLUPDATE;
}

/* ----------------------------------------------------- */
/* 板主設定隱形/ 解隱形                                  */
/* ----------------------------------------------------- */
char            board_hidden_status;
#ifdef  BMCHS
static int
change_hidden(int ent, fileheader_t * fhdr, char *direct)
{
    boardheader_t   *bp;
    if (!((currmode & MODE_BOARD) || HAS_PERM(PERM_SYSOP)))
	return DONOTHING;

    bp = getbcache(currbid);
    if (((bp->brdattr & BRD_HIDE) && (bp->brdattr & BRD_POSTMASK))) {
	if (getans("目前板在隱形狀態, 要解隱形嘛(Y/N)?[N]") != 'y')
	    return FULLUPDATE;
	bp->brdattr &= ~BRD_HIDE;
	bp->brdattr &= ~BRD_POSTMASK;
	outs("君心今傳眾人，無處不聞弦歌。\n");
	board_hidden_status = 0;
	hbflreload(currbid);
    } else {
	if (getans("目前板在現形狀態, 要隱形嘛(Y/N)?[N]") != 'y')
	    return FULLUPDATE;
	bp->brdattr |= BRD_HIDE;
	bp->brdattr |= BRD_POSTMASK;
	outs("君心今已掩抑，惟盼善自珍重。\n");
	board_hidden_status = 1;
    }
    substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
    log_usies("SetBoard", bp->brdname);
    pressanykey();
    return FULLUPDATE;
}

static int
change_counting(int ent, fileheader_t * fhdr, char *direct)
{   

    boardheader_t   *bp;
    if (!((currmode & MODE_BOARD) || HAS_PERM(PERM_SYSOP)))
	return DONOTHING;
    bp = getbcache(currbid);
    if (!(bp->brdattr & BRD_HIDE && bp->brdattr & BRD_POSTMASK))
	return FULLUPDATE;

    if (bp->brdattr & BRD_BMCOUNT) {
	if (getans("目前板列入十大排行, 要取消列入十大排行嘛(Y/N)?[N]") != 'y')
	    return FULLUPDATE;
	bp->brdattr &= ~BRD_BMCOUNT;
	outs("你再灌水也不會有十大的呀。\n");
    } else {
	if (getans("目前看板不列入十大排行, 要列入十大嘛(Y/N)?[N]") != 'y')
	    return FULLUPDATE;
	bp->brdattr |= BRD_BMCOUNT;
	outs("快灌水衝十大第一吧。\n");
    }
    substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
    pressanykey();
    return FULLUPDATE;
}

#endif

/* ----------------------------------------------------- */
/* 看板功能表                                            */
/* ----------------------------------------------------- */
/* onekey_size was defined in ../include/pttstruct.h, as ((int)'z') */
onekey_t read_comms[] = {
    NULL, // Ctrl('A') 1
    NULL, // Ctrl('B')
    NULL, // Ctrl('C')
    NULL, // Ctrl('D')
    NULL, // Ctrl('E')
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
    NULL, // Ctrl('X')
    NULL, // Ctrl('Y')
    NULL, // Ctrl('Z') 26
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, // 'A' 65
    bh_title_edit, // 'B'
    board_etc, // 'C'
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
    NULL, // 'J'
    b_water_edit, // 'K'
    solve_post, // 'L'
    b_vote_maintain, // 'M'
    NULL, // 'N'
    b_post_note, // 'O'
    NULL, // 'P'
    view_postmoney, // 'Q'
    b_results, // 'R'
    sequential_read, // 'S'
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
Read()
{
    int             mode0 = currutmp->mode;
    int             stat0 = currstat, tmpbid = currutmp->brc_id;
    char            buf[40];
#ifdef LOG_BOARD
    time_t          usetime = now;
#endif

    if ( ! currboard[0] )
	return 0;

    setutmpmode(READING);
    set_board();

    if (board_visit_time < *board_note_time) {
	setbfile(buf, currboard, fn_notes);
	if(more(buf, NA)!=-1)
   	    pressanykey();
        else
            *board_note_time=0;
    }
    setutmpbid(currbid);
    setbdir(buf, currboard);
    curredit &= ~EDIT_MAIL;
    i_read(READING, buf, readtitle, readdoent, read_comms,
	   currbid);
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
ReadSelect()
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
log_board(char *mode, time_t usetime)
{
    char            buf[256];

    if (usetime > 30) {
	snprintf(buf, sizeof(buf), "USE %-20.20s Stay: %5ld (%s) %s\n",
		 mode, usetime, cuser.userid, ctime(&now));
	log_file(FN_USEBOARD, buf, 1);
    }
}
#endif

int
Select()
{
    char            genbuf[200];

    do_select(0, NULL, genbuf);
    return 0;
}
#ifdef HAVEMOBILE
void
mobile_message(char *mobile, char *message)
{



    bsmtp(char *fpath, char *title, char *rcpt, int method);
}

#endif
