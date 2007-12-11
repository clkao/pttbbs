/* $Id$ */
#include "bbs.h"

#ifdef EDITPOST_SMARTMERGE

#ifdef SMARTMERGE_MD5

// To enable SmartMerge/MD5,
// please get MD5 from XySSL or other systems:
// http://xyssl.org/code/source/md5/
#include "md5.h"
#include "md5.c"

#define SMHASHLEN (16)

#else // !SMARTMERGE_MD5, use FNV64

#include "fnv_hash.h"

#define SMHASHLEN (64/8)

#endif // !SMARTMERGE_MD5

#endif // EDITPOST_SMARTMERGE

#define WHEREAMI_LEVEL	16

static int recommend(int ent, fileheader_t * fhdr, const char *direct);
static int do_add_recommend(const char *direct, fileheader_t *fhdr,
		 int ent, const char *buf, int type);

#ifdef ASSESS
static char * const badpost_reason[] = {
    "�s�i", "�������", "�H������"
};
#endif

/* TODO multi.money is a mess.
 * please help verify and finish these.
 */
/* modes to invalid multi.money */
#define INVALIDMONEY_MODES (FILE_ANONYMOUS | FILE_BOTTOM | FILE_DIGEST | FILE_BID)

/* query money by fileheader pointer.
 * return <0 if money is not valid.
 */
int 
query_file_money(const fileheader_t *pfh)
{
    fileheader_t hdr;

    if(	(currmode & MODE_SELECT) &&  
	(pfh->multi.refer.flag) &&
	(pfh->multi.refer.ref > 0)) // really? not sure, copied from other's code
    {
	char genbuf[MAXPATHLEN];

	/* it is assumed that in MODE_SELECT, currboard is selected. */
	setbfile(genbuf, currboard, FN_DIR);
	get_record(genbuf, &hdr, sizeof(hdr), pfh->multi.refer.ref);
	pfh = &hdr;
    }

    if(pfh->filemode & INVALIDMONEY_MODES || pfh->multi.money > MAX_POST_MONEY)
	return -1;

    return pfh->multi.money;
}

/* hack for listing modes */
enum LISTMODES {
    LISTMODE_DATE = 0,
    LISTMODE_MONEY,
};
static char *listmode_desc[] = {
    "�� ��",
    "�� ��",
};
static int currlistmode = LISTMODE_DATE;

#define IS_LISTING_MONEY \
	(currlistmode == LISTMODE_MONEY || \
	 ((currmode & MODE_SELECT) && (currsrmode & RS_MONEY)))

void
anticrosspost(void)
{
    log_file("etc/illegal_money",  LOG_CREAT | LOG_VF,
             ANSI_COLOR(1;33;46) "%s "
             ANSI_COLOR(37;45) "cross post �峹 "
             ANSI_COLOR(37) " %s" ANSI_RESET "\n", 
             cuser.userid, ctime4(&now));
    post_violatelaw(cuser.userid, BBSMNAME "�t��ĵ��", 
	    "Cross-post", "�@��B��");
    cuser.userlevel |= PERM_VIOLATELAW;
    cuser.timeviolatelaw = now;
    cuser.vl_count++;
    mail_id(cuser.userid, "Cross-Post�@��",
	    "etc/crosspost.txt", BBSMNAME "ĵ���");
    if ((now - cuser.firstlogin) / 86400 < 14)
	delete_allpost(cuser.userid);
    kick_all(cuser.userid); // XXX: in2: wait for testing
    u_exit("Cross Post");
    exit(0);
}

/* Heat CharlieL */
int
save_violatelaw(void)
{
    char            buf[128], ok[3];
    int             day;

    setutmpmode(VIOLATELAW);
    clear();
    stand_title("ú�@�椤��");

    if (!(cuser.userlevel & PERM_VIOLATELAW)) {
	vmsg("�A�S���Q�}�@��~~");
	return 0;
    }
    day =  cuser.vl_count*3 - (now - cuser.timeviolatelaw)/86400;
    if (day > 0) {
        vmsgf("�̷ӹH�W����, �A�ٻݭn�Ϭ� %d �Ѥ~��ú�@��", day);
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

    if(bp->BM[0] <= ' ')
	strcpy(currBM, "�x�D��");
    else
    {
	/* calculate with other title information */
	int l = 0;

	snprintf(currBM, sizeof(currBM), "�O�D:%s", bp->BM);
	/* title has +7 leading symbols */
	l += strlen(bp->title);
	if(l >= 7) 
	    l -= 7;
	else 
	    l = 0;
	l += 8 + strlen(currboard); /* trailing stuff */
	l += strlen(bp->brdname);
	l = t_columns - l -strlen(currBM);

#ifdef _DEBUG
	{
	    char buf[MAXPATHLEN];
	    sprintf(buf, "title=%d, brdname=%d, currBM=%d, t_c=%d, l=%d",
		    strlen(bp->title), strlen(bp->brdname),
		    strlen(currBM), t_columns, l);
	    vmsg(buf);
	}
#endif

	if(l < 0 && ((l += strlen(currBM)) > 7))
	{
	    currBM[l] = 0;
	    currBM[l-1] = currBM[l-2] = '.';
	}
    }

    /* init basic perm, but post perm is checked on demand */
    currmode = (currmode & (MODE_DIRTY | MODE_GROUPOP)) | MODE_STARTED;
    if (!HasUserPerm(PERM_NOCITIZEN) && 
         (HasUserPerm(PERM_ALLBOARD) || is_BM_cache(currbid))) {
	currmode = currmode | MODE_BOARD | MODE_POST | MODE_POSTCHECKED;
    }
}

/* check post perm on demand, no double checks in current board
 * currboard MUST be defined!
 * XXX can we replace currboard with currbid ? */
int
CheckPostPerm(void)
{
    static time4_t last_chk_time = 0x0BAD0BB5; /* any magic number */
    static int last_board_index = 0; /* for speed up */
    int valid_index = 0;
    boardheader_t *bp = NULL;
    
    if (currmode & MODE_DIGEST)
	return 0;

    if (currmode & MODE_POSTCHECKED)
    {
	/* checked? let's check if perm reloaded */
	if (last_board_index < 1 || last_board_index > SHM->Bnumber)
	{
	    /* invalid board index, refetch. */
	    last_board_index = getbnum(currboard);
	    valid_index = 1;
	}
	assert(0<=last_board_index-1 && last_board_index-1<MAX_BOARD);
	bp = getbcache(last_board_index);

	if(bp->perm_reload != last_chk_time)
	    currmode &= ~MODE_POSTCHECKED;
    }

    if (!(currmode & MODE_POSTCHECKED))
    {
	if(!valid_index)
	{
	    last_board_index = getbnum(currboard);
	    bp = getbcache(last_board_index);
	}
	last_chk_time = bp->perm_reload;
	currmode |= MODE_POSTCHECKED;

	// vmsg("reload board postperm");
	
	if (haspostperm(currboard)) {
	    currmode |= MODE_POST;
	    return 1;
	}
	currmode &= ~MODE_POST;
	return 0;
    }
    return (currmode & MODE_POST);
}

int CheckPostRestriction(int bid)
{
    if ((currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP))
	return 1;

    // check first-login
    if (cuser.firstlogin > (now - (time4_t)bcache[bid - 1].post_limit_regtime * 2592000))
	return 0;
    if (cuser.numlogins / 10 < (unsigned int)bcache[bid - 1].post_limit_logins)
	return 0;
    if (cuser.numposts  / 10 < (unsigned int)bcache[bid - 1].post_limit_posts)
	return 0;
    if  (cuser.badpost > (255 - (unsigned int)bcache[bid - 1].post_limit_badpost))
	return 0;

    return 1;
}

static void
readtitle(void)
{
    boardheader_t  *bp;
    char    *brd_title;

    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
    bp = getbcache(currbid);
    if(bp->bvote != 2 && bp->bvote)
	brd_title = "���ݪO�i��벼��";
    else
	brd_title = bp->title + 7;

    showtitle(currBM, brd_title);
    outs("[��]���} [��]�\\Ū [^P]�o��峹 [b]�Ƨѿ� [d]�R�� [z]��ذ� [TAB]��K [h]����\n");
    prints(ANSI_COLOR(7) "   �s��    %s �@  ��       ��  ��  ��  �D", 
	    IS_LISTING_MONEY ? listmode_desc[LISTMODE_MONEY] : listmode_desc[currlistmode]);

#ifdef USE_COOLDOWN
    if (    bp->brdattr & BRD_COOLDOWN && 
	    !((currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP)))
	outslr("", 44, ANSI_RESET, 0);
    else 
#endif
    {
	char buf[32];
	assert(0<=currbid-1 && currbid-1<MAX_BOARD);
	sprintf(buf, "�H��:%d ",
	   SHM->bcache[currbid - 1].nuser);
	outslr("", 44, buf, -1);
	outs(ANSI_RESET);
    }
}

static void
readdoent(int num, fileheader_t * ent)
{
    int  type = ' ';
    char *mark, *title,
	 color, special = 0, isonline = 0, recom[8];
    char *typeattr = "";
    char isunread = 0, oisunread = 0;

    oisunread = isunread = 
	brc_unread(currbid, ent->filename, ent->modified);

    // modified tag
    if (isunread == 2)
    {
	// ignore unread, if user doesn't want to show it.
	if (cuser.uflag & NO_MODMARK_FLAG)
	{
	    oisunread = isunread = 0;
	}
	// if user wants colored marks, use 'read' marks
	else if (cuser.uflag & COLORED_MODMARK)
	{
	    isunread = 0;
	    typeattr = ANSI_COLOR(36);
	}
    }

    type = isunread ? '+' : ' ';
    if (isunread == 2) type = '~';

    // handle 'type"
    if ((currmode & MODE_BOARD) && (ent->filemode & FILE_DIGEST))
	type = (type == ' ') ? '*' : '#';
    else if (currmode & MODE_BOARD || HasUserPerm(PERM_LOGINOK)) 
    {
	if (ent->filemode & FILE_MARKED)
	{
	    if(ent->filemode & FILE_SOLVED)
		type = '!';
	    else if (isunread == 0)
		type = 'm';
	    else if (isunread == 1)
		type = 'M';
	    else if (isunread == 2)
		type = '=';
	} else if (TagNum && !Tagger(atoi(ent->filename + 2), 0, TAG_NIN))
	    type = 'D';
	else if (ent->filemode & FILE_SOLVED)
	    type = (type == ' ') ? 's': 'S';
    }

    // the only special case: ' ' with isunread == 2,
    // change to '+' with gray attribute.
    if (type == ' ' && oisunread == 2)
    {
	typeattr = ANSI_COLOR(1;30);
	type = '+';
    }

    title = ent->filename[0]!='L' ? subject(ent->title) : "<������w>";
    if (ent->filemode & FILE_VOTE)
	color = '2', mark = "��";
    else if (ent->filemode & FILE_BID)
	color = '6', mark = "�C";
    else if (title == ent->title)
	color = '1', mark = "��";
    else
	color = '3', mark = "R:";

    /* ��L���� title �屼�C �e������ 33 �Ӧr���C */
    {
	int l = t_columns - 34; /* 33+1, for trailing one more space */
	unsigned char *p = (unsigned char*)title;

	/* strlen ���K�� safe print checking */
	while (*p && l > 0)
	{
	    /* �������Ӱ� DBCS checking, �i�o�g�F */
	    if(*p < ' ')
		*p = ' ';
	    p++, l--;
	}

	if (*p && l <= 0)
	    strcpy((char*)p-3, " �K");
    }

    if (!strncmp(title, "[���i]", 6))
	special = 1;

    isonline = query_online(ent->owner);

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

    /* start printing */
    if (ent->filemode & FILE_BOTTOM)
	outs("  " ANSI_COLOR(1;33) "  �� " ANSI_RESET);
    else
	/* recently we found that many boards have >10k articles,
	 * so it's better to use 5+2 (2 for cursor marker) here.
	 * XXX if we are in big term, enlarge here.
	 */
	prints("%7d", num);

    prints(" %s%c" ESC_STR "[0;1;3%4.4s" ANSI_RESET, 
	    typeattr, type, recom);

    if(IS_LISTING_MONEY)
    {
	int m = query_file_money(ent);
	if(m < 0)
	    outs(" ---- ");
	else
	    prints("%5d ", m);
    }
    else // LISTMODE_DATE
    {
#ifdef COLORDATE
	prints(ANSI_COLOR(%d) "%-6s" ANSI_RESET,
		(ent->date[3] + ent->date[4]) % 7 + 31, ent->date);
#else
	prints("%-6s", ent->date);
#endif
    }

    // print author
    if(isonline) outs(ANSI_COLOR(1));
    prints("%-13.12s", ent->owner);
    if(isonline) outs(ANSI_RESET);
	   
    if (strncmp(currtitle, title, TTLEN))
	prints("%s " ANSI_COLOR(1) "%.*s" ANSI_RESET "%s\n",
	       mark, special ? 6 : 0, title, special ? title + 6 : title);
    else
	prints("\033[1;3%cm%s %s" ANSI_RESET "\n",
	       color, mark, title);
}

int
whereami(void)
{
    boardheader_t  *bh, *p[WHEREAMI_LEVEL];
    int             i, j;
    int bid = currbid;

    if (!bid)
	return 0;

    move(1, 0);
    clrtobot();
    assert(0<=bid-1 && bid-1<MAX_BOARD);
    bh = getbcache(bid);
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
do_select(void)
{
    char            bname[20];

    setutmpmode(SELECT);
    move(0, 0);
    clrtoeol();
    CompleteBoard(MSG_SELECT_BOARD, bname);

    if(enter_board(bname) < 0)
	return FULLUPDATE;

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
    char            nick[STRLEN], fn1[MAXPATHLEN];
    int             len = 42-strlen(currboard);
    struct tm      *ptime = localtime4(&now);

    if(!fh->filename[0]) return;
    setbfile(fn1, currboard, fh->filename);
    if ((fin = fopen(fn1, "r"))) {
	brd = by_BM ? "deleted" : "junk";

        memcpy(&postfile, fh, sizeof(fileheader_t));
	setbpath(newpath, brd);
	stampfile_u(newpath, &postfile);
	
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

static void
do_deleteCrossPost(const fileheader_t *fh, char bname[])
{
    char bdir[MAXPATHLEN]="", file[MAXPATHLEN]="";
    fileheader_t newfh;
    if(!bname || !fh) return;

    int i, bid = getbnum(bname);
    if(bid <=0 || !fh->filename[0]) return;

    boardheader_t  *bp = getbcache(bid);
    if(!bp) return;

    setbdir(bdir, bname);
    setbfile(file, bname, fh->filename);
    memcpy(&newfh, fh, sizeof(fileheader_t)); 
    // Ptt: protect original fh 
    // because getindex safe_article_delete will change fh in some case
    if( (i=getindex(bdir, &newfh, 0))>0)
    {
#ifdef SAFE_ARTICLE_DELETE
        if(bp && !(currmode & MODE_DIGEST) && bp->nuser > 30 )
	        safe_article_delete(i, &newfh, bdir);
        else
#endif
                delete_record(bdir, sizeof(fileheader_t), i);
	unlink(file);
	setbtotal(bid);
    }
}

static void
deleteCrossPost(const fileheader_t *fh, char *bname)
{
    if(!fh || !fh->filename[0]) return;

    if(!strcmp(bname, ALLPOST) || !strcmp(bname, "NEWIDPOST") ||
       !strcmp(bname, ALLHIDPOST) || !strcmp(bname, "UnAnonymous"))
    {
        int len=0;
	char xbname[TTLEN + 1], *po = strrchr(fh->title, '.');
	if(!po) return;
	po++;
        len = (int) strlen(po)-2;

	if(len > TTLEN) return;
	sprintf(xbname, "%.*s", len, po);
	do_deleteCrossPost(fh, xbname);
    }
    else
    {
	do_deleteCrossPost(fh, ALLPOST);
    }
}

void
delete_allpost(const char *userid)
{
    fileheader_t fhdr;
    int     fd, i;
    char    bdir[MAXPATHLEN]="", file[MAXPATHLEN]="";

    if(!userid) return;

    setbdir(bdir, ALLPOST);
    if( (fd = open(bdir, O_RDWR)) != -1) 
    {
       for(i=0; read(fd, &fhdr, sizeof(fileheader_t)) >0; i++){
           if(strcmp(fhdr.owner, userid))
             continue;
           deleteCrossPost(&fhdr, ALLPOST);
	   setbfile(file, ALLPOST, fhdr.filename);
	   unlink(file);

           sprintf(fhdr.title, "(����w�Q�R��)");
           strcpy(fhdr.filename, ".deleted");
           strcpy(fhdr.owner, "-");
           lseek(fd, sizeof(fileheader_t) * i, SEEK_SET);
           write(fd, &fhdr, sizeof(fileheader_t));
       }
       close(fd);
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
    char            tmp_title[STRLEN];

    if (strncasecmp(title, str_reply, 4))
	snprintf(tmp_title, sizeof(tmp_title), "Re: %s", title);
    else
	strlcpy(tmp_title, title, sizeof(tmp_title));
    tmp_title[TTLEN - 1] = '\0';
    snprintf(genbuf, sizeof(genbuf), "�ĥέ���D�m%.60s�n��?[Y] ", tmp_title);
    getdata(row, 0, genbuf, genbuf2, 4, LCECHO);
    if (genbuf2[0] == 'n' || genbuf2[0] == 'N')
	getdata(++row, 0, "���D�G", tmp_title, TTLEN, DOECHO);
    // don't getdata() on non-local variable save_title directly, to avoid reentrant crash.
    strlcpy(save_title, tmp_title, sizeof(save_title));
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
do_crosspost(const char *brd, fileheader_t *postfile, const char *fpath,
             int isstamp)
{
    char            genbuf[200];
    int             len = 42-strlen(currboard);
    fileheader_t    fh;
    int bid = getbnum(brd);

    if(bid <= 0 || bid > MAX_BOARD) return;

    if(!strncasecmp(postfile->title, str_reply, 3))
        len=len+4;

    memcpy(&fh, postfile, sizeof(fileheader_t));
    if(isstamp) 
    {
         setbpath(genbuf, brd);
         stampfile(genbuf, &fh); 
    }
    else
         setbfile(genbuf, brd, postfile->filename);

    if(!strcmp(brd, "UnAnonymous"))
       strcpy(fh.owner, cuser.userid);

    sprintf(fh.title,"%-*.*s.%s�O",  len, len, postfile->title, currboard);
    unlink(genbuf);
    Copy((char *)fpath, genbuf);
    postfile->filemode = FILE_LOCAL;
    setbdir(genbuf, brd);
    if (append_record(genbuf, &fh, sizeof(fileheader_t)) != -1) {
	SHM->lastposttime[bid - 1] = now;
	touchbpostnum(bid, 1);
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
		"�I�ڤ覡: 1." MONEYNAME "�� 2.�l���λȦ���b"
		"3.�䲼�ιq�� 4.�l���f��I�� [1]:",
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
    char *payby[4]={MONEYNAME "��", "�l���λȦ���b", 
	"�䲼�ιq��", "�l���f��I��"};
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
    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
    bp = getbcache(currbid);

    if( !CheckPostPerm()
#ifdef FOREIGN_REG
	// ���O�~�y�ϥΪ̦b PttForeign �O
	&& !((cuser.uflag2 & FOREIGN) && 
	    strcmp(bp->brdname, GLOBAL_FOREIGN) == 0)
#endif
	) {
	vmsg("�藍�_�A�z�ثe�L�k�b���o��峹�I");
	return READ_REDRAW;
    }

#ifndef DEBUG
    if ( !CheckPostRestriction(currbid) )
    {
	move(5, 10); // why move (5, 10)?
	vmsg("�A������`��I (�i���j�g I �d�ݭ���)");
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
	char tmp_title[STRLEN];
	if (!isbid) {
	    move(21,0);
	    outs("�����G");
	    for(i=0; i<8 && bp->posttype[i*4]; i++)
		strlcpy(ctype[i],bp->posttype+4*i,5);
	    if(i==0) i=8;
	    for(aborted=0; aborted<i; aborted++)
		prints("%d.%4.4s ", aborted+1, ctype[aborted]);
	    sprintf(buf,"(1-%d�Τ���)",i);
	    getdata(21, 6+7*i, buf, tmp_title, 3, LCECHO); 
	    posttype = tmp_title[0] - '1';
	    if (posttype >= 0 && posttype < i)
		snprintf(tmp_title, sizeof(tmp_title),
			"[%s] ", ctype[posttype]);
	    else
	    {
		tmp_title[0] = '\0';
		posttype=-1;
	    }
	}
	getdata_buf(22, 0, "���D�G", tmp_title, TTLEN, DOECHO);
	strip_ansi(tmp_title, tmp_title, STRIP_ALL);
	if( strcmp(tmp_title, "[711iB] �W�[�W�����Ƶ{��") == 0 ){
	    cuser.userlevel |= PERM_VIOLATELAW;
	    sleep(60);
	    u_exit("bad program");
	}
	strlcpy(save_title, tmp_title, sizeof(save_title));
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
    /* set owner to Anonymous for Anonymous board */

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
    {
	/* general article */
	postfile.modified = dasht(fpath);
	postfile.multi.money = aborted;
    }
    
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
    strcpy(genbuf, fpath);
    setbpath(fpath, currboard);
    stampfile_u(fpath, &postfile);   
    // Ptt: stamp file again to make it order
    //      fix the bug that search failure in getindex
    //      stampfile_u is used when you don't want to clear other fields
    if (append_record(buf, &postfile, sizeof(postfile)) == -1)
    {
        unlink(genbuf);
    }
    else
    {
        rename(genbuf, fpath);
#ifdef LOGPOST
	{
            FILE    *fp = fopen("log/post", "a");
            fprintf(fp, "%d %s boards/%c/%s/%s\n",
                    now, cuser.userid, currboard[0], currboard,
                    postfile.filename);
            fclose(fp);
        }
#endif
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
	brc_addlist(postfile.filename, postfile.modified);

        if( !bp->level || (currbrdattr & BRD_POSTMASK))
        {
	        if ((now - cuser.firstlogin) / 86400 < 14)
            		do_crosspost("NEWIDPOST", &postfile, fpath, 0);

		if (!(currbrdattr & BRD_HIDE) )
            		do_crosspost(ALLPOST, &postfile, fpath, 0);
	        else	
            		do_crosspost(ALLHIDPOST, &postfile, fpath, 0);
	}
	outs("���Q�K�X�G�i�A");

#ifdef MAX_POST_MONEY
	if (aborted > MAX_POST_MONEY)
	    aborted = MAX_POST_MONEY;
#endif
	if (strcmp(currboard, "Test") && !ifuseanony &&
	    !(currbrdattr&BRD_BAD)) {
	    prints("�o�O�z���� %d �g�峹�C",++cuser.numposts);
            if(postfile.filemode&FILE_BID)
                outs("�ۼФ峹�S���Z�S�C");
            else
              {
                prints(" �Z�S %d �ȡC",aborted);
                demoney(aborted);    
              }
	} else
	    outs("���C�J�����A�q�Х]�[�C");

	/* �^�����@�̫H�c */

	if (curredit & EDIT_BOTH) {
	    char           *str, *msg = "�^���ܧ@�̫H�c";

	    if ((str = strchr(quote_user, '.'))) {
		if (
#ifndef USE_BSMTP
		    bbs_sendmail(fpath, save_title, str + 1)
#else
		    bsmtp(fpath, save_title, str + 1)
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
		    sendalert(quote_user, ALERT_NEW_MAIL);
	    }
	    outs(msg);
	    curredit ^= EDIT_BOTH;
	}
	if (currbrdattr & BRD_ANONYMOUS)
            do_crosspost("UnAnonymous", &postfile, fpath, 0);
#ifdef USE_COOLDOWN
        if(bp->nuser>30)
	{
	    if (cooldowntimeof(usernum)<now)
		add_cooldowntime(usernum, 5);
	}
	add_posttimes(usernum, 1);
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
    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
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

    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
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
do_generalboardreply(/*const*/ fileheader_t * fhdr)
{
    char            genbuf[3];
    
    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
    if (!CheckPostRestriction(currbid))
    {
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

int
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
   char filepath[PATHLEN], genbuf[60], title[5], posttype_f, posttype[33]="";

   if(!(currmode & MODE_BOARD)) return DONOTHING;
   
   assert(0<=currbid-1 && currbid-1<MAX_BOARD);
   bp = getbcache(currbid);

   move(2,0);
   clrtobot();
   posttype_f = bp->posttype_f;
   for( i = 0 ; i < 8 ; ++i ){
       move(2,0);
       outs("�峹����:       ");
       strlcpy(genbuf, bp->posttype + i * 4, 5);
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
   strlcpy(bp->posttype, posttype, sizeof(bp->posttype)); /* �o�����ӭn��race condition */

   assert(0<=currbid-1 && currbid-1<MAX_BOARD);
   substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
   return FULLUPDATE;
}

static int
do_reply(/*const*/ fileheader_t * fhdr)
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

    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
    bp = getbcache(currbid);
    if (bp->brdattr & BRD_NOREPLY) {
	vmsg("�ܩ�p, ���O���}��^�Ф峹.");
	return FULLUPDATE;
    }

    setbfile(quote_file, bp->brdname, fhdr->filename);
    if (bp->brdattr & BRD_VOTEBOARD || (fhdr->filemode & FILE_VOTE))
	do_voteboardreply(fhdr);
    else
	do_generalboardreply(fhdr);
    *quote_file = 0;
    return FULLUPDATE;
}

static int
reply_post(int ent, /*const*/ fileheader_t * fhdr, const char *direct)
{
    return do_reply(fhdr);
}

#ifdef EDITPOST_SMARTMERGE

#define HASHPF_RET_OK (0)

// return: 0 - ok; otherwise - fail.
static int
hash_partial_file( char *path, size_t sz, unsigned char output[SMHASHLEN] )
{
    int fd;
    size_t n;
    unsigned char buf[1024];

#ifdef SMARTMERGE_MD5

    md5_context ctx;
    md5_starts( &ctx );

#else // !SMARTMERGE_MD5, use FNV64

    Fnv64_t fnvseed = FNV1_64_INIT;
    assert(SMHASHLEN == sizeof(fnvseed));

#endif // SMARTMERGE_MD5

    fd = open(path, O_RDONLY);
    if (fd < 0)
	return 1;

    while(  sz > 0 &&
	    (n = read(fd, buf, sizeof(buf))) > 0 )
    {
	if (n > sz) n = sz;

#ifdef SMARTMERGE_MD5
	md5_update( &ctx, buf, (int) n );
#else // !SMARTMERGE_MD5, use FNV64
	fnvseed = fnv_64_buf(buf, (int) n, fnvseed);
#endif //!SMARTMERGE_MD5

	sz -= n;
    }
    close(fd);

    if (sz > 0) // file is different
	return 2;

#ifdef SMARTMERGE_MD5
    md5_finish( &ctx, output );
#else // !SMARTMERGE_MD5, use FNV64
    memcpy(output, (void*) &fnvseed, sizeof(fnvseed));
#endif //!SMARTMERGE_MD5

    return HASHPF_RET_OK;
}
#endif // EDITPOST_SMARTMERGE

int
edit_post(int ent, fileheader_t * fhdr, const char *direct)
{
    char            fpath[80];
    char            genbuf[200];
    fileheader_t    postfile;
    boardheader_t  *bp = getbcache(currbid);
    int		    recordTouched = 0;
    time4_t	    oldmt, newmt;
    off_t	    oldsz;

#ifdef EDITPOST_SMARTMERGE
    char	    canDoSmartMerge = 1;
#endif // EDITPOST_SMARTMERGE

    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
    if (strcmp(bp->brdname, GLOBAL_SECURITY) == 0)
	return DONOTHING;

    // XXX ������ɰ_�A edit_post �w�g���|�� + ���F...
    // �������O Sysop Edit ����a�Φ��C
    // ���Ѧ��ŧ�ӤH�g�� mode �O��W edit �a

    if ((bp->brdattr & BRD_VOTEBOARD)  ||
	    (fhdr->filemode & FILE_VOTE)   ||
	    !CheckPostPerm()               ||
	    strcmp(fhdr->owner, cuser.userid) != EQUSTR ||
	    strcmp(cuser.userid, STR_GUEST) == EQUSTR)
	return DONOTHING;

    // special modes (plus MODE_DIGEST?)
    if( currmode & MODE_SELECT )
	return DONOTHING;

#ifdef SAFE_ARTICLE_DELETE
    if( fhdr->filename[0] == '.' )
	return DONOTHING;
#endif

    setutmpmode(REEDIT);

    // TODO �ѩ�{�b�ɮ׳��O�����\�^���ɡA
    // �b��ݪO�ؿ��}�w�S���ܤj�N�q�C (�Ĳv�y���@�I)
    // �i�H�Ҽ{��}�b user home dir
    // �n�B�O�ݪO���ɮ׼Ƥ��|�g�����C (when someone crashed)
    // sethomedir(fpath, cuser.userid);
    // XXX �p�G�A���t�Φ��w���ݪO�M�t���ɡA���N���Ω� user home�C
    setbpath(fpath, currboard);

    // XXX �H�{�b���Ҧ��A�o�O�� temp file
    stampfile(fpath, &postfile);
    setdirpath(genbuf, direct, fhdr->filename);
    local_article = fhdr->filemode & FILE_LOCAL;

    // copying takes long time, add some visual effect
    grayout_lines(0, b_lines-1, 0);
    move(b_lines-1, 0); clrtoeol();
    outs("���b���J�ɮ�...");
    refresh();

    Copy(genbuf, fpath);
    strlcpy(save_title, fhdr->title, sizeof(save_title));

    // so far this is what we copied now...
    oldmt = dasht(genbuf);
    oldsz = dashs(fpath); // should be equal to genbuf(src).
			  // use fpath (dest) in case some 
			  // modification was made.
    do {
#ifdef EDITPOST_SMARTMERGE

	unsigned char oldsum[SMHASHLEN] = {0}, newsum[SMHASHLEN] = {0};

	//  make checksum of file genbuf
	if (canDoSmartMerge &&
	    hash_partial_file(fpath, oldsz, oldsum) != HASHPF_RET_OK)
	    canDoSmartMerge = 0;

#endif // EDITPOST_SMARTMERGE

	if (vedit(fpath, 0, NULL) == -1)
	    break;

	newmt = dasht(genbuf);

	if (newmt != oldmt)
	{
	    move(b_lines-7, 0);
	    clrtobot();
	    outs(ANSI_COLOR(1;33) "�� �ɮפw�Q�ק�L! ��" ANSI_RESET "\n\n");
	}

#ifdef EDITPOST_SMARTMERGE

	// only merge if file is enlarged and modified
	if (newmt == oldmt || dashs(genbuf) < oldsz)
	    canDoSmartMerge = 0;
	
	// make checksum of new file [by oldsz]
	if (canDoSmartMerge &&
	    hash_partial_file(genbuf, oldsz, newsum) != HASHPF_RET_OK)
	    canDoSmartMerge = 0;

	// verify checksum
	if (canDoSmartMerge &&
	    memcmp(oldsum, newsum, sizeof(newsum)) != 0)
	    canDoSmartMerge = 0;

	if (canDoSmartMerge)
	{
	    canDoSmartMerge = 0; // only try merge once
	    outs("�i��۰ʦX�� [Smart Merge]...\n");

	    // smart merge
	    if (AppendTail(genbuf, fpath, oldsz) == 0)
	    {
		// merge ok
		oldmt = newmt;
		outs(ANSI_COLOR(1) 
		    "�X�֦��\\�A�s�ק�(�α���)�w�[�J�z���峹���C\n" 
		    "�z�S���\���������έק�A�Фž�ߡC"
		    ANSI_RESET "\n");

#ifdef  WARN_EXP_SMARTMERGE
		outs(ANSI_COLOR(1;33) 
		    "�۰ʦX�� (Smart Merge) �O���礤���s�\\��A" 
		    "���ˬd�@�U�z���峹�X�֫�O�_���`�C" ANSI_RESET "\n"
		    "�Y�����D�Ц� " GLOBAL_BUGREPORT " �O���i�A���¡C");
#endif 
		vmsg("�X�֧���");
	    } else {
		outs(ANSI_COLOR(31) 
		    "�۰ʦX�֥��ѡC �Ч�ΤH�u��ʽs��X�֡C" ANSI_RESET);
		vmsg("�X�֥���");
	    }
	}

#endif // EDITPOST_SMARTMERGE

	if (oldmt != newmt)
	{
	    int c = 0;
	    outs("�i��O�z�b�s�誺�L�{�����H�i�����έפ�C\n"
	 	 "�z�i�H��ܪ����л\\�ɮ�(y)�B���(n)�A\n"
		 " �άO" ANSI_COLOR(1)"���s�s��" ANSI_RESET
		 "(�s��|�Q�K���s���ɮ׫᭱)(e)�C\n");
	    c = tolower(getans("�n�����л\\�ɮ�/����/���s�� [Y/n/e]�H"));

	    if (c == 'n')
		break;

	    if (c == 'e')
	    {
		FILE *fp, *src;

		/* merge new and old stuff */
		fp = fopen(fpath, "at"); 
		src = fopen(genbuf, "rt");

		if(!fp)
		{
		    vmsg("��p�A�ɮפw�l���C");
		    if(src) fclose(src);
		    unlink(fpath); // fpath is a temp file
		    return FULLUPDATE;
		}

		if(src)
		{
		    int c = 0;
		    struct tm *ptime;

		    fprintf(fp, MSG_SEPERATOR "\n");
		    fprintf(fp, "�H�U���Q�ק�L���̷s���e: ");
		    ptime = localtime4(&newmt);
		    fprintf(fp,
			    " (%02d/%02d %02d:%02d)\n",
			    ptime->tm_mon + 1, ptime->tm_mday, 
			    ptime->tm_hour, ptime->tm_min);
		    fprintf(fp, MSG_SEPERATOR "\n");
		    while ((c = fgetc(src)) >= 0)
			fputc(c, fp);
		    fclose(src);

		    // update oldsz, old mt records
		    oldmt = dasht(genbuf);
		    oldsz = dashs(genbuf);
		}
		fclose(fp);
		continue;
	    }
	}

	// OK to save file.

	// force to remove file first?
	// unlink(genbuf);
        Rename(fpath, genbuf);

	// this is almost always true...
	// whatever.
	{
	    time4_t oldm = fhdr->modified;
	    fhdr->modified = dasht(genbuf);
	    recordTouched = (oldm != fhdr->modified) ? 1 : 0;
	}

        if(strcmp(save_title, fhdr->title)){
	    // Ptt: here is the black hole problem
	    // (try getindex)
	    strcpy(fhdr->title, save_title);
	    recordTouched = 1;
	}

	if(recordTouched)
	{
	    substitute_ref_record(direct, fhdr, ent);
	    // mark my self as "read this file".
	    brc_addlist(fhdr->filename, fhdr->modified);
	}

	break;

    } while (1);

    /* should we do this when editing was aborted? */
    unlink(fpath);

    return FULLUPDATE;
}

#define UPDATE_USEREC   (currmode |= MODE_DIRTY)

static int
cross_post(int ent, fileheader_t * fhdr, const char *direct)
{
    char            xboard[20], fname[80], xfpath[80], xtitle[80];
    char            inputbuf[10], genbuf[200], genbuf2[4];
    fileheader_t    xfile;
    FILE           *xptr;
    int             author;
    boardheader_t  *bp;

    move(2, 0);
    clrtoeol();
    move(1, 0);
    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
    bp = getbcache(currbid);
    if (bp && (bp->brdattr & BRD_VOTEBOARD) )
	return FULLUPDATE;

    CompleteBoard("������峹��ݪO�G", xboard);
    if (*xboard == '\0')
	return FULLUPDATE;

    if (!haspostperm(xboard))
    {
	vmsg("�ݪO���s�b�θӬݪO�T��z�o��峹�I");
	return FULLUPDATE;
    }

    /* �ɥ��ܼ� */
    ent = StringHash(fhdr->title);
    author = getbnum(xboard);
    assert(0<=author-1 && author-1<MAX_BOARD);

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

    if (!CheckPostRestriction(author))
    {
	vmsg("�A������`��I (�i�b�ت��ݪO�����j�g I �d�ݭ���)");
	return FULLUPDATE;
    }

#ifdef USE_COOLDOWN
       if(check_cooldown(getbcache(author)))
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

	if ((bp->brdattr & BRD_HIDE) && (bp->brdattr & BRD_POSTMASK)) 
	{
	    /* invisible board */
	    fprintf(xptr, "�� [��������۬Y���άݪO]\n\n");
	    b_suckinfile_invis(xptr, fname, currboard);
	} else {
	    /* public board */
	    fprintf(xptr, "�� [��������� %s �ݪO]\n\n", currboard);
	    b_suckinfile(xptr, fname);
	}

	addsignature(xptr, 0);
	fclose(xptr);

#ifdef USE_AUTOCPLOG
	/* add cp log. bp is currboard now. */
	if(bp->brdattr & BRD_CPLOG)
	{
	    char buf[MAXPATHLEN], tail[STRLEN];
	    char bname[STRLEN] = "";
	    struct tm *ptime = localtime4(&now);
	    int maxlength = 51 +2 - 6;
	    int bid = getbnum(xboard);

	    assert(0<=bid-1 && bid-1<MAX_BOARD);
	    bp = getbcache(bid);
	    if ((bp->brdattr & BRD_HIDE) && (bp->brdattr & BRD_POSTMASK)) 
	    {
		/* mosaic it */
		/*
		// mosaic method 1
		char  *pbname = bname;
		while(*pbname)
		    *pbname++ = '?';
		*/
		// mosaic method 2
		strcpy(bname, "�Y���άݪO");
	    } else {
		sprintf(bname, "�ݪO %s", xboard);
	    }

	    maxlength -= (strlen(cuser.userid) + strlen(bname));

#ifdef GUESTRECOMMEND
	    snprintf(tail, sizeof(tail),
		    "%15s %02d/%02d",
		    fromhost, 
		    ptime->tm_mon + 1, ptime->tm_mday);
#else
	    maxlength += (15 - 6);
	    snprintf(tail, sizeof(tail),
		    " %02d/%02d %02d:%02d",
		    ptime->tm_mon + 1, ptime->tm_mday,
		    ptime->tm_hour, ptime->tm_min);
#endif
	    snprintf(buf, sizeof(buf),
		    // ANSI_COLOR(32) <- system will add green
		    "�� " ANSI_COLOR(1;32) "%s"
		    ANSI_COLOR(0;32) ":�����"
		    "%s" ANSI_RESET "%*s%s\n" ,
		    cuser.userid, bname, maxlength, "",
		    tail);

	    do_add_recommend(direct, fhdr,  ent, buf, 2);
	} else
#endif
	{
	    int bid = getbnum(xboard);
	    assert(0<=bid-1 && bid-1<MAX_BOARD);
	/* now point bp to new bord */
	bp = getbcache(bid);
	}

	/*
	 * Cross fs�����D } else { unlink(xfpath); link(fname, xfpath); }
	 */
	setbdir(fname, xboard);
	append_record(fname, &xfile, sizeof(xfile));
	if (!xfile.filemode && !(bp->brdattr & BRD_NOTRAN))
	    outgo_post(&xfile, xboard, cuser.userid, cuser.nickname);
#ifdef USE_COOLDOWN
        if(bp->nuser>30)
	{
	    if (cooldowntimeof(usernum)<now)
		add_cooldowntime(usernum, 5);
	}
	add_posttimes(usernum, 1);
#endif
	setbtotal(getbnum(xboard));

	if (strcmp(xboard, "Test") == 0)
	    outs("���իH�󤣦C�J�����A�q�Х]�[�C");
	else
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

    if (fhdr->owner[0] == '-' || fhdr->filename[0] == 'L')
	return READ_SKIP;

    STATINC(STAT_READPOST);
    setdirpath(genbuf, direct, fhdr->filename);

    more_result = more(genbuf, YEA);

#ifdef LOG_CRAWLER
    {
        // kcwu: log crawler
	static int read_count = 0;
        extern Fnv32_t client_code;
        read_count++;

        if (read_count % 1000 == 0) {
            time4_t t = time4(NULL);
            log_file("log/read_alot", LOG_VF|LOG_CREAT,
		    "%d %s %d %s %08x %d\n", t, ctime4(&t), getpid(),
		    cuser.userid, client_code, read_count);
        }
    }
#endif // LOG_CRAWLER

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
    brc_addlist(fhdr->filename, fhdr->modified);
    strlcpy(currtitle, subject(fhdr->title), sizeof(currtitle));

    switch(more_result)
    {
	case -1:
	    clear();
	    vmsg("���峹�L���e");
	    return FULLUPDATE;
	case 999:
	    do_reply(fhdr);
            return FULLUPDATE;
	case 998:
            recommend(ent, fhdr, direct);
	    return FULLUPDATE;
    }
    if(more_result)
	return more_result;
    return FULLUPDATE;
}

int
do_limitedit(int ent, fileheader_t * fhdr, const char *direct)
{
    char            genbuf[256], buf[256];
    int             temp;
    boardheader_t  *bp = getbcache(currbid);

    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
    if (!((currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP) ||
		(HasUserPerm(PERM_SYSSUPERSUBOP) && GROUPOP())))
	return DONOTHING;
    
    strcpy(buf, "��� ");
    if (HasUserPerm(PERM_SYSOP) || (HasUserPerm(PERM_SYSSUPERSUBOP) && GROUPOP()))
	strcat(buf, "(A)���O�o���� ");
    strcat(buf, "(B)���O�w�]");
    if (fhdr->filemode & FILE_VOTE)
	strcat(buf, " (C)���g");
    strcat(buf, "�s�p���� (Q)�����H[Q]");
    genbuf[0] = getans(buf);

    if ((HasUserPerm(PERM_SYSOP) || (HasUserPerm(PERM_SYSSUPERSUBOP) && GROUPOP())) && genbuf[0] == 'a') {
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

	sprintf(genbuf, "%u", 255 - bp->post_limit_badpost);
	do {
	    getdata_buf(b_lines - 1, 0, "�H��g�ƤW�� (0~255)�G", genbuf, 5, LCECHO);
	    temp = atoi(genbuf);
	} while (temp < 0 || temp > 255);
	bp->post_limit_badpost = (unsigned char)(255 - temp);

	assert(0<=currbid-1 && currbid-1<MAX_BOARD);
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

	sprintf(genbuf, "%u", 255 - bp->vote_limit_badpost);
	do {
	    getdata_buf(b_lines - 1, 0, "�H��g�ƤW�� (0~255)�G", genbuf, 5, LCECHO);
	    temp = atoi(genbuf);
	} while (temp < 0 || temp > 255);
	bp->vote_limit_badpost = (unsigned char)(255 - temp);

	assert(0<=currbid-1 && currbid-1<MAX_BOARD);
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

	sprintf(genbuf, "%u", (unsigned int)(fhdr->multi.vote_limits.badpost));
	do {
	    getdata_buf(b_lines - 1, 0, "�H��g�ƤW�� (0~255)�G", genbuf, 5, LCECHO);
	    temp = atoi(genbuf);
	} while (temp < 0 || temp > 255);
	fhdr->multi.vote_limits.badpost = (unsigned char)temp;

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
    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
    if (!bp->endgamble || bp->endgamble > now)
	return 0;

    setbfile(fn_ticket, currboard, FN_TICKET);
    setbfile(fn_ticket_end, currboard, FN_TICKET_END);

    rename(fn_ticket, fn_ticket_end);
    if (bp->endgamble) {
	bp->endgamble = 0;
	assert(0<=currbid-1 && currbid-1<MAX_BOARD);
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
    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
    ticket(currbid);
    return FULLUPDATE;
}
static int
hold_gamble(void)
{
    char            fn_ticket[128], fn_ticket_end[128], genbuf[128], msg[256] = "",
                    yn[10] = "";
    char tmp[128];
    boardheader_t  *bp = getbcache(currbid);
    int             i;
    FILE           *fp = NULL;

    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
    if (!(currmode & MODE_BOARD))
	return 0;
    if (bp->brdattr & BRD_BAD )
	{
      	  vmsg("�H�k�ݪO�T��ϥν�L");
	  return 0;
	}

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
	    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
	    substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);

	}
	return FULLUPDATE;
    }
    if (dashf(fn_ticket_end)) {
	getdata(b_lines - 1, 0, "�w�g���|���L, "
		"�O�_�n�}�� [�_/�O]?(N/y)�G", yn, 3, LCECHO);
	if (yn[0] != 'y')
	    return FULLUPDATE;
        if(cpuload(NULL) > MAX_CPULOAD/4)
            {
	        vmsg("�t���L�� �Щ�t�έt���C�ɶ}��..");
		return FULLUPDATE;
	    }
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
    setbfile(tmp, currboard, FN_TICKET_ITEMS ".tmp");

    //sprintf(genbuf, "%s/" FN_TICKET_ITEMS, direct);

    if (!(fp = fopen(tmp, "w")))
	return FULLUPDATE;
    do {
	getdata(2, 0, "��J�m������ (����:10-10000):", yn, 6, LCECHO);
	i = atoi(yn);
    } while (i < 10 || i > 10000);
    fprintf(fp, "%d\n", i);
    if (!getdata(3, 0, "�]�w�۰ʫʽL�ɶ�?(Y/n)", yn, 3, LCECHO) || yn[0] != 'n') {
	bp->endgamble = gettime(4, now, "�ʽL��");
	assert(0<=currbid-1 && currbid-1<MAX_BOARD);
	substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
    }
    move(6, 0);
    snprintf(genbuf, sizeof(genbuf),
	     "\n�Ш� %s �O ��'f'�ѻP���!\n\n"
	     "�@�i %d " MONEYNAME "��, �o�O%s�����\n%s%s\n",
	     currboard,
	     i, i < 100 ? "�p�䦡" : i < 500 ? "������" :
	     i < 1000 ? "�Q�گ�" : i < 5000 ? "�I����" : "�ɮa����",
	     bp->endgamble ? "��L�����ɶ�: " : "",
	     bp->endgamble ? Cdate(&bp->endgamble) : ""
	     );
    strcat(msg, genbuf);
    outs("�Ш̦���J�m���W��, �ݴ���2~8��. (�����K��, ��J������Enter)\n");
    //outs(ANSI_COLOR(1;33) "�`�N��J��L�k�ק�I\n");
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

    setbfile(genbuf, currboard, FN_TICKET_ITEMS);
    setbfile(tmp, currboard, FN_TICKET_ITEMS ".tmp");
    if(!dashf(fn_ticket))
	Rename(tmp, genbuf);

    snprintf(genbuf, sizeof(genbuf), "[���i] %s �O �}�l���!", currboard);
    post_msg(currboard, genbuf, msg, cuser.userid);
    post_msg("Record", genbuf + 7, msg, "[�������l]");
    /* Tim ����CS, �H�K���b����user���Ƥw�g�g�i�� */
    rename(fn_ticket_end, fn_ticket);
    /* �]�w���~���ɦW��L�� */

    vmsg("��L�]�w����");
    return FULLUPDATE;
}
#endif

static int
cite_post(int ent, const fileheader_t * fhdr, const char *direct)
{
    char            fpath[PATHLEN];
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
    getdata(b_lines - 1, 0, "�T�w�n�����k�s[y/N]? ", yn, 5, LCECHO);
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
    char    path[PATHLEN];
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

    // XXX do lock some day!

    /* This is a solution to avoid most racing (still some), but cost four
     * system calls.                                                        */

    if(type == 0 && fhdr->recommend < 100 )
          update = 1;
    else if(type == 1 && fhdr->recommend > -100)
          update = -1;

    // since we want to do 'modification'...
    fhdr->modified = dasht(path);
    
    if( /* update */ 1){
        int fd;
	off_t sz = dashs(direct);

	// TODO we can also check if fhdr->filename is same.
	// prevent black holes
	if (sz < sizeof(fileheader_t) * (ent))
	{
	    return -1;
	}

        //Ptt: update only necessary
	if( (fd = open(direct, O_RDWR)) < 0 )
	    return -1;

	// let sz = base offset
	sz = (sizeof(fileheader_t) * (ent-1));

	if (lseek(fd, (sz + (char*)&fhdr->modified - (char*)fhdr), 
		    SEEK_SET) < 0)
	{
	    close(fd);
	    return -1;
	}

	write(fd, &fhdr->modified, sizeof(fhdr->modified));
	if( update && 
		lseek(fd, sz + (char*)&fhdr->recommend - (char*)fhdr,
		    SEEK_SET) >= 0 )
	{
	    // �p�G lseek ���ѴN���| write
            read(fd, &fhdr->recommend, sizeof(fhdr->recommend));
            fhdr->recommend += update;
            lseek(fd, -1, SEEK_CUR);
	    write(fd, &fhdr->recommend, sizeof(fhdr->recommend));
	}
	close(fd);

	// mark my self as "read this file".
	brc_addlist(fhdr->filename, fhdr->modified);
    }
    return 0;
}

static int
do_bid(int ent, fileheader_t * fhdr, const boardheader_t  *bp,
       const char *direct,  const struct tm *ptime)
{
    char            genbuf[200], fpath[PATHLEN],say[30],*money;
    bid_t           bidinfo;
    int             mymax, next;

    setdirpath(fpath, direct, fhdr->filename);
    strcat(fpath, ".bid");
    get_record(fpath, &bidinfo, sizeof(bidinfo), 1);

    move(18,0);
    clrtobot();
    prints("�v�ХD�D: %s\n", fhdr->title);
    print_bidinfo(0, bidinfo);
    money = bidinfo.payby ? " NT$ " : MONEYNAME "$ ";
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
    char            buf[PATHLEN], msg[STRLEN];
#ifndef OLDRECOMMEND
    static const char *ctype[3] = {
		       "��", "�N", "��"
		   };
    static const char *ctype_attr[3] = {
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
		       "�u�[������"
		   };
#endif
    int             type, maxlength;
    boardheader_t  *bp;
    static time4_t  lastrecommend = 0;
    static int lastrecommend_bid = -1;
    static char lastrecommend_fname[FNLEN] = "";
    int isGuest = (strcmp(cuser.userid, STR_GUEST) == EQUSTR);
    int logIP = 0;
    int ymsg = b_lines -1;

    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
    bp = getbcache(currbid);
    if (bp->brdattr & BRD_NORECOMMEND || fhdr->filename[0] == 'L' || 
        ((fhdr->filemode & FILE_MARKED) && (fhdr->filemode & FILE_SOLVED))) {
	vmsg("��p, �T����˩��v��");
	return FULLUPDATE;
    }

    if (   !CheckPostPerm() || 
	    bp->brdattr & BRD_VOTEBOARD || 
#ifndef GUESTRECOMMEND
	    isGuest ||
#endif
	    fhdr->filemode & FILE_VOTE) {
	vmsg("�z�v������, �L�k����!"); //  "(�i���j�g I �d�ݭ���)"
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

#ifndef DEBUG
    if (!CheckPostRestriction(currbid))
    {
	vmsg("�A������`��I (�i���j�g I �d�ݭ���)");
	return FULLUPDATE;
    }
#endif

    if((currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP))
    {
	/* I'm BM or SYSOP. */
    } 
    else if (bp->brdattr & BRD_NOFASTRECMD) 
    {
	int d = (int)bp->fastrecommend_pause - (now - lastrecommend);
	if (d > 0)
	{
	    vmsgf("���O�T��ֳt�s�����A�ЦA�� %d ��", d);
	    return FULLUPDATE;
	}
    }
    {
	// kcwu
	static unsigned char lastrecommend_minute = 0;
	static unsigned short recommend_in_minute = 0;
	unsigned char now_in_minute = (unsigned char)(now / 60);
	if(now_in_minute != lastrecommend_minute) {
	    recommend_in_minute = 0;
	    lastrecommend_minute = now_in_minute;
	}
	recommend_in_minute++;
	if(recommend_in_minute>60) {
	    vmsg("�t�θT��u�ɶ����j�q����");
	    return FULLUPDATE;
	}
    }
    {
	// kcwu
	char path[PATHLEN];
	off_t size;
	setdirpath(path, direct, fhdr->filename);
	size = dashs(path);
	if (size > 5*1024*1024) {
	    vmsg("�ɮפӤj, �L�k�~�����, �Хt����o��");
	    return FULLUPDATE;
	}

	if (size > 100*1024) {
	    int d = 10 - (now - lastrecommend);
	    if (d > 0) {
		vmsgf("����w�L��, �T��ֳt�s�����, �ЦA�� %d ��", d);
		return FULLUPDATE;
	    }
	}
    }
		    

#ifdef USE_COOLDOWN
       if(check_cooldown(bp))
	  return FULLUPDATE;
#endif

    type = 0;

    // why "recommend == 0" here?
    // some users are complaining that they like to fxck up system
    // with lots of recommend one-line text.
    // since we don't support recognizing update of recommends now,
    // they tend to use the counter to identify whether an arcitle
    // has new recommends or not.
    // so, make them happy here.
#ifndef OLDRECOMMEND
    // no matter it is first time or not.
    if (strcmp(cuser.userid, fhdr->owner) == 0)
#else
    // old format is one way counter, so let's relax.
    if (fhdr->recommend == 0 && strcmp(cuser.userid, fhdr->owner) == 0)
#endif
    {
	// owner recommend
	type = 2;
	move(ymsg--, 0); clrtoeol();
#ifndef OLDRECOMMEND
	outs("�@�̥��H, �ϥ� �� �[���覡\n");
#else
	outs("�@�̥��H����, �ϥ� �� �[���覡\n");
#endif

    }
#ifndef DEBUG
    else if (!(currmode & MODE_BOARD) && 
	    (now - lastrecommend) < (
#if 0
	    /* i'm not sure whether this is better or not */
		(bp->brdattr & BRD_NOFASTRECMD) ? 
		 bp->fastrecommend_pause :
#endif
		90))
    {
	// too close
	type = 2;
	move(ymsg--, 0); clrtoeol();
	outs("�ɶ��Ӫ�, �ϥ� �� �[���覡\n");
    }
#endif

#ifndef OLDRECOMMEND
    else if (!(bp->brdattr & BRD_NOBOO))
    {
	/* most people use recommendation just for one-line reply. 
	 * so we change default to (2)= comment only now.
#define RECOMMEND_DEFAULT_VALUE (2)
	 */
#define RECOMMEND_DEFAULT_VALUE (0) /* current user behavior */

	move(b_lines, 0); clrtoeol();
	outs(ANSI_COLOR(1)  "�zı�o�o�g�峹 ");
	prints("%s1.%s %s2.%s %s3.%s " ANSI_RESET "[%d]? ",
		ctype_attr[0], ctype_long[0],
		ctype_attr[1], ctype_long[1],
		ctype_attr[2], ctype_long[2],
		RECOMMEND_DEFAULT_VALUE+1);

	// poor BBS term has problem positioning with ANSI.
	move(b_lines, 55); 
	type = igetch() - '1';
	if(type < 0 || type > 2)
	    type = RECOMMEND_DEFAULT_VALUE;
	move(b_lines, 0); clrtoeol();
    }
#endif

    // warn if article is outside post
    if (strchr(fhdr->owner, '.')  != NULL)
    {
	move(ymsg--, 0); clrtoeol();
	outs(ANSI_COLOR(1;31) 
	    "���o�g�峹�Ӧۥ~����H�O�A��@�̥i��L�k�ݨ����C" 
	    ANSI_RESET "\n");
    }

    if(type > 2 || type < 0)
	type = 0;

    maxlength = 78 - 
	3 /* lead */ - 
	6 /* date */ - 
	1 /* space */ -
	6 /* time */;

    if (bp->brdattr & BRD_IPLOGRECMD || isGuest)
    {
	maxlength -= 15 /* IP */;
	logIP = 1;
    }

#ifdef OLDRECOMMEND
    maxlength -= 2; /* '��' */
    maxlength -= strlen(cuser.userid);
    sprintf(buf, "%s %s:", "��" , cuser.userid);

#else // !OLDRECOMMEND
    maxlength -= strlen(cuser.userid);
    sprintf(buf, "%s%s%s %s:", 
	    ctype_attr[type], ctype[type], ANSI_RESET,
	    cuser.userid);
#endif // !OLDRECOMMEND

    move(b_lines, 0);
    clrtoeol();

    if (!getdata(b_lines, 0, buf, msg, maxlength, DOECHO))
	return FULLUPDATE;

    // make sure to do modification
    {
	char ans[3];
	sprintf(buf+strlen(buf), ANSI_COLOR(7) "%-*s" 
		ANSI_RESET " �T�w[y/N]:", maxlength, msg);
	if(!getdata(b_lines, 0, buf, ans, sizeof(ans), LCECHO) ||
		ans[0] != 'y')
	    return FULLUPDATE;
    }

    // log if you want
#ifdef LOG_PUSH
    {
	static  int tolog = 0;
	if( tolog == 0 )
	    tolog =
		(cuser.numlogins < 50 || (now - cuser.firstlogin) < 86400 * 7)
		? 1 : 2;
	if( tolog == 1 ){
	    FILE   *fp;
	    if( (fp = fopen("log/push", "a")) != NULL ){
		fprintf(fp, "%s %d %s %s %s\n", cuser.userid, now, currboard, fhdr->filename, msg);
		fclose(fp);
	    }
	    sleep(1);
	}
    }
#endif // LOG_PUSH

    STATINC(STAT_RECOMMEND);

    {
	/* build tail first. */
	char tail[STRLEN];

	if(logIP)
	{
	    snprintf(tail, sizeof(tail),
		    "%15s %02d/%02d %02d:%02d",
		    fromhost, 
		    ptime->tm_mon+1, ptime->tm_mday,
		    ptime->tm_hour, ptime->tm_min);
	} else {
	    snprintf(tail, sizeof(tail),
		    " %02d/%02d %02d:%02d",
		    ptime->tm_mon+1, ptime->tm_mday,
		    ptime->tm_hour, ptime->tm_min);
	}

#ifdef OLDRECOMMEND
	snprintf(buf, sizeof(buf),
	    ANSI_COLOR(1;31) "�� " ANSI_COLOR(33) "%s" 
	    ANSI_RESET ANSI_COLOR(33) ":%-*s" ANSI_RESET
	    "��%s\n",
	    cuser.userid, maxlength, msg, tail);
#else
	snprintf(buf, sizeof(buf),
	    "%s%s " ANSI_COLOR(33) "%s" ANSI_RESET ANSI_COLOR(33) 
	    ":%-*s" ANSI_RESET "%s\n",
             ctype_attr2[type], ctype[type], cuser.userid, 
	     maxlength, msg, tail);
#endif // OLDRECOMMEND
    }

    do_add_recommend(direct, fhdr,  ent, buf, type);

#ifdef ASSESS
    /* �C 10 ������ �[�@�� goodpost */
    if (type ==0 && (fhdr->filemode & FILE_MARKED) && fhdr->recommend % 10 == 0)
	inc_goodpost(fhdr->owner, 1);
#endif

    lastrecommend = now;
    lastrecommend_bid = currbid;
    strlcpy(lastrecommend_fname, fhdr->filename, sizeof(lastrecommend_fname));
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
	if (strcmp(bp->brdname, GLOBAL_SECURITY) == 0)
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

	    if ((curredit & EDIT_MAIL)==0 && (currmode & MODE_BOARD)) // Ptt:update cache
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
    char            genbuf[100], newpath[PATHLEN];
    int             not_owned, tusernum;
    boardheader_t  *bp;

    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
    bp = getbcache(currbid);

    /* TODO recursive lookup */
    if (currmode & MODE_SELECT) { 
        vmsg("�Ц^��@��Ҧ��A�R���峹");
        return DONOTHING;
    }

    if(fhdr->filemode & FILE_ANONYMOUS)
                /* When the file is anonymous posted, fhdr->multi.anon_uid is author.
                 * see do_general() */
        tusernum = fhdr->multi.anon_uid;
    else
        tusernum = searchuser(fhdr->owner, NULL);

    if (strcmp(bp->brdname, GLOBAL_SECURITY) == 0)
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

    if (fhdr->filename[0]=='L') fhdr->filename[0]='M';

    getdata(1, 0, msg_del_ny, genbuf, 3, LCECHO);
    if (genbuf[0] == 'y') {
	if(
#ifdef SAFE_ARTICLE_DELETE
	   (bp->nuser > 30 && !(currmode & MODE_DIGEST) &&
            !safe_article_delete(ent, fhdr, direct)) ||
#endif
	   !delete_record(direct, sizeof(fileheader_t), ent)
	   ) {

	    cancelpost(fhdr, not_owned, newpath);
            deleteCrossPost(fhdr, bp->brdname);
#ifdef ASSESS
#define SIZE	sizeof(badpost_reason) / sizeof(char *)

	    if (not_owned && tusernum > 0 && !(currmode & MODE_DIGEST)) {
		if (now - atoi(fhdr->filename + 2) > 7 * 24 * 60 * 60)
		    /* post older than a week */
		    genbuf[0] = 'n';
		else
		    getdata(1, 40, "�c�H�峹?(y/N)", genbuf, 3, LCECHO);

		if (genbuf[0]=='y') {
		    int i;
		    char *userid=getuserid(tusernum);
                    int rpt_bid;
 
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
			post_violatelaw(userid, BBSMNAME " �t��ĵ��", 
				"�H��֭p 5 �g", "�@��@�i");
			mail_violatelaw(userid, BBSMNAME " �t��ĵ��", 
				"�H��֭p 5 �g", "�@��@�i");
                        kick_all(userid);
                        passwd_query(tusernum, &xuser);
                        xuser.money = moneyof(tusernum);
                        xuser.vl_count++;
		        xuser.userlevel |= PERM_VIOLATELAW;
			xuser.timeviolatelaw = now;  
			passwd_update(tusernum, &xuser);
		       }
		       sendalert(userid,  ALERT_PWD_BADPOST);
		       mail_id(userid, genbuf, newpath, cuser.userid);

#ifdef BAD_POST_RECORD
                       rpt_bid = getbnum(BAD_POST_RECORD);
                      if (rpt_bid > 0) {
			  fileheader_t report_fh;
			  char report_path[PATHLEN];

			  setbpath(report_path, BAD_POST_RECORD);
			  stampfile(report_path, &report_fh);

			  strcpy(report_fh.owner, "[" BBSMNAME "ĵ�]");
			  snprintf(report_fh.title, sizeof(report_fh.title),
				  "%s �O %s �O�D���� %s �@�g�H��",
				  currboard, cuser.userid, userid);
			  Copy(newpath, report_path);

			  setbdir(report_path, BAD_POST_RECORD);
			  append_record(report_path, &report_fh, sizeof(report_fh));
 
                          touchbtotal(rpt_bid);
		      }
#endif /* defined(BAD_POST_RECORD) */
		   }
                }
	    }
#undef SIZE
#endif

	    setbtotal(currbid);
	    if (fhdr->multi.money < 0 || fhdr->filemode & FILE_ANONYMOUS)
		fhdr->multi.money = 0;
	    if (not_owned && tusernum && fhdr->multi.money > 0 &&
		strcmp(currboard, "Test") && strcmp(currboard, ALLPOST)) {
		deumoney(tusernum, -fhdr->multi.money);
#ifdef USE_COOLDOWN
		if (bp->brdattr & BRD_COOLDOWN)
		    add_cooldowntime(tusernum, 15);
#endif
	    }
	    if (!not_owned && strcmp(currboard, "Test") && 
                strcmp(currboard, ALLPOST)) {
		if (cuser.numposts)
		    cuser.numposts--;
		if (!(currmode & MODE_DIGEST && currmode & MODE_BOARD)){
		    demoney(-fhdr->multi.money);
		    vmsgf("�z���峹� %d �g�A��I�M��O %d ��", 
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
    vmsgf("�ɮצW��: %s ", fhdr->filename);
    return PART_REDRAW;
}

static int
lock_post(int ent, fileheader_t * fhdr, const char *direct)
{
    char fn1[MAXPATHLEN];
    char genbuf[256] = {'\0'};
    int i;

    if (!(currmode & MODE_BOARD) && !HasUserPerm(PERM_SYSOP | PERM_POLICE))
	return DONOTHING;

    if (fhdr->filename[0]=='M') {
	if (!HasUserPerm(PERM_SYSOP | PERM_POLICE))
	    return DONOTHING;

	getdata(b_lines - 1, 0, "�п�J��w�z�ѡG", genbuf, 50, DOECHO);

	if (getans("�n�N�峹��w��(y/N)?") != 'y')
	    return FULLUPDATE;
        setbfile(fn1, currboard, fhdr->filename);
        fhdr->filename[0] = 'L';
    }
    else if (fhdr->filename[0]=='L') {
	if (getans("�n�N�峹��w�Ѱ���(y/N)?") != 'y')
	    return FULLUPDATE;
        fhdr->filename[0] = 'M';
        setbfile(fn1, currboard, fhdr->filename);
    }
    substitute_ref_record(direct, fhdr, ent);
    post_policelog(currboard, fhdr->title, "���", genbuf, fhdr->filename[0] == 'L' ? 1 : 0);
    if (fhdr->filename[0] == 'L') {
	fhdr->filename[0] = 'M';
	do_crosspost("PoliceLog", fhdr, fn1, 0);
	fhdr->filename[0] = 'L';
	snprintf(genbuf, sizeof(genbuf), "%s �O�D��w�峹 - %s", currboard, fhdr->title);
	for (i = 0; i < MAX_BMs && SHM->BMcache[currbid-1][i] != -1; i++)
	    mail_id(SHM->userid[SHM->BMcache[currbid-1][i] - 1], genbuf, fn1, "[�t��]");
    }
    return FULLUPDATE;
} 

static int
view_postmoney(int ent, const fileheader_t * fhdr, const char *direct)
{
    if(fhdr->filemode & FILE_ANONYMOUS)
	/* When the file is anonymous posted, fhdr->multi.anon_uid is author.
	 * see do_general() */
	vmsgf("�ΦW�޲z�s��: %d (�P�@�H���X�|�@��)",
		fhdr->multi.anon_uid + (int)currutmp->pid);
    else {
	int m = query_file_money(fhdr);

	if(m < 0)
	    m = vmsgf("�S��峹�A�L����O���C");
	else
	    m = vmsgf("�o�@�g�峹�� %d ��", m);

	/* QQ: enable money listing mode */
	if (m == 'Q')
	{
	    currlistmode = (currlistmode == LISTMODE_MONEY) ?
		LISTMODE_DATE : LISTMODE_MONEY;
	    vmsg((currlistmode == LISTMODE_MONEY) ? 
		    "�}�Ҥ峹����C��Ҧ�" : "����C�X�峹����");
	}
    }

    return FULLUPDATE;
}

#ifdef OUTJOBSPOOL
/* �ݪO�ƥ� */
static int
tar_addqueue(void)
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
    assert(0<=bid-1 && bid-1<MAX_BOARD);
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
	assert(0<=bid-1 && bid-1<MAX_BOARD);
	substitute_record(fn_board, fh, sizeof(boardheader_t), bid);
    }
    return 0;
}

static int
b_notes_edit(void)
{
    if (currmode & MODE_BOARD) {
	assert(0<=currbid-1 && currbid-1<MAX_BOARD);
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
	assert(0<=currbid-1 && currbid-1<MAX_BOARD);
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

	assert(0<=currbid-1 && currbid-1<MAX_BOARD);
	bp = getbcache(currbid);
	move(1, 0);
	clrtoeol();
	getdata_str(1, 0, "�п�J�ݪO�s����ԭz:", genbuf,
		    BTLEN - 16, DOECHO, bp->title + 7);

	if (!genbuf[0])
	    return 0;
	strip_ansi(genbuf, genbuf, STRIP_ALL);
	strlcpy(bp->title + 7, genbuf, sizeof(bp->title) - 7);
	assert(0<=currbid-1 && currbid-1<MAX_BOARD);
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
    currmode &= ~MODE_SELECT;
    currsrmode = 0;
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

    // MODE_POST may be changed if board is modified.
    // do not change post perm here. use other instead.

    setbdir(currdirect, currboard);
    return NEWDIRECT;
}


static int
push_bottom(int ent, fileheader_t *fhdr, const char *direct)
{
    int num;
    char buf[256];
    if ((currmode & MODE_DIGEST) || !(currmode & MODE_BOARD)
        || fhdr->filename[0]=='L')
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
    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
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
	if (!strcmp(currboard, GLOBAL_NOTE) || 
#ifdef GLOBAL_ARTDSN	    
	    !strcmp(currboard, GLOBAL_ARTDSN) || 
#endif
	    !strcmp(currboard, GLOBAL_BUGREPORT) ||
	    !strcmp(currboard, GLOBAL_LAW)
	    ) 
	{
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
	assert(0<=currbid-1 && currbid-1<MAX_BOARD);
	if(!(getbcache(currbid)->brdattr & BRD_HIDE)) { 
          getdata(1, 0, "�n��ȱo�X���������K?(N/y)", genbuf2, 3, LCECHO);
          if(genbuf2[0] == 'y')
	      do_crosspost(GLOBAL_DIGEST, &digest, genbuf, 1);
        }
#endif

	fhdr->filemode = (fhdr->filemode & ~FILE_MARKED) | FILE_DIGEST;
	if (!strcmp(currboard, GLOBAL_NOTE) || 
#ifdef GLOBAL_ARTDSN	    
	    !strcmp(currboard, GLOBAL_ARTDSN) || 
#endif
	    !strcmp(currboard, GLOBAL_BUGREPORT) ||
	    !strcmp(currboard, GLOBAL_LAW)
	    ) 
	{
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
b_config(void)
{
    boardheader_t   *bp=NULL;
    int touched = 0, finished = 0;
    bp = getbcache(currbid); 
    int i = 0;

    const int ytitle = b_lines - 
#ifndef OLDRECOMMEND
	16;
#else // OLDRECOMMEND
	15;
#endif  // OLDRECOMMEND

    grayout_lines(0, ytitle-1, 0);

    move(ytitle-1, 0); clrtobot();
    // outs(MSG_SEPERATOR); // deprecated by grayout
    move(ytitle, 0);
    outs(ANSI_COLOR(7) " " ); outs(bp->brdname); outs(" �ݪO�]�w");
    i = t_columns - strlen(bp->brdname) - strlen("  �ݪO�]�w") - 2;
    for (; i>0; i--)
	outc(' ');
    outs(ANSI_RESET);

    while(!finished) {
	move(ytitle +2, 0);

	prints(" ����ԭz: %s\n", bp->title);
	prints(" �O�D�W��: %s\n", (bp->BM[0] > ' ')? bp->BM : "(�L)");

	outs("\n");

	prints( " " ANSI_COLOR(1;36) "h" ANSI_RESET 
		" - ���}���A(�O�_����): %s " ANSI_RESET "\n", 
		(bp->brdattr & BRD_HIDE) ? 
		ANSI_COLOR(1)"����":"���}");

	prints( " " ANSI_COLOR(1;36) "r" ANSI_RESET 
		" - %s " ANSI_RESET "���ˤ峹\n", 
		(bp->brdattr & BRD_NORECOMMEND) ? 
		ANSI_COLOR(1)"���i":"�i�H");

#ifndef OLDRECOMMEND
	prints( " " ANSI_COLOR(1;36) "b" ANSI_RESET
	        " - %s " ANSI_RESET "�N��\n", 
		((bp->brdattr & BRD_NORECOMMEND) || (bp->brdattr & BRD_NOBOO))
		? ANSI_COLOR(1)"���i":"�i�H");
#endif
	{
	    int d = 0;

	    if(bp->brdattr & BRD_NORECOMMEND)
	    {
		d = -1;
	    } else {
		if ((bp->brdattr & BRD_NOFASTRECMD) &&
		    (bp->fastrecommend_pause > 0))
		    d = bp->fastrecommend_pause;
	    }

	    prints( " " ANSI_COLOR(1;36) "f" ANSI_RESET 
		    " - %s " ANSI_RESET "�ֳt�s���峹", 
		    d != 0 ?
		     ANSI_COLOR(1)"����": "�i�H");
	    if(d > 0)
		prints(", �̧C���j�ɶ�: %d ��", d);
	    outs("\n");
	}

	prints( " " ANSI_COLOR(1;36) "i" ANSI_RESET 
		" - ����� %s" ANSI_RESET " �O���ӷ� IP\n", 
		(bp->brdattr & BRD_IPLOGRECMD) ? 
		ANSI_COLOR(1)"�n":"����");

#ifdef USE_AUTOCPLOG
	prints( " " ANSI_COLOR(1;36) "x" ANSI_RESET 
		" - ����峹�� %s " ANSI_RESET "�۰ʰO��\n", 
		(bp->brdattr & BRD_CPLOG) ? 
		ANSI_COLOR(1)"�|" : "���|" );
#endif

	prints( " " ANSI_COLOR(1;36) "o" ANSI_RESET 
		" - �Y����H�h�o��ɹw�] %s " ANSI_RESET "\n", 
		(bp->brdattr & BRD_LOCALSAVE) ? 
		"�����s��(����X)" : ANSI_COLOR(1)"���ڦs��(��X)" );

	// use '8' instead of '1', to prevent 'l'/'1' confusion
	prints( " " ANSI_COLOR(1;36) "8" ANSI_RESET 
		" - �����Q�K�� %s " ANSI_RESET
		"�i�J\n", (bp->brdattr & BRD_OVER18) ? 
		ANSI_COLOR(1) "���i�H" : "�i�H" );

	prints( " " ANSI_COLOR(1;36) "y" ANSI_RESET 
		" - %s" ANSI_RESET
		" �^�� (�s�ժ��H�W�~�i�]�w����)\n",
		(bp->brdattr & BRD_NOREPLY) ? 
		ANSI_COLOR(1)"���i�H" : "�i�H" );

	prints( " " ANSI_COLOR(1;36) "e" ANSI_RESET 
		" - �o���v��: %s" ANSI_RESET " (�����~�i�]�w����)\n", 
		(bp->brdattr & BRD_RESTRICTEDPOST) ? 
		ANSI_COLOR(1)"�u���O�ͤ~�i�o��" : "�L�S�O�]�w" );

	move_ansi(b_lines - 10, 52);
	prints("�o�孭��");
	move_ansi(b_lines - 9, 54);
	prints("�W������ %d ���H�W", (int)bp->post_limit_logins * 10);
	move_ansi(b_lines - 8, 54);
	prints("�峹�g�� %d �g�H�W", (int)bp->post_limit_posts * 10);
	move_ansi(b_lines - 7, 54);
	prints("���U�ɶ� %d �Ӥ�H�W", (int)bp->post_limit_regtime);
	move_ansi(b_lines - 6, 54);
	prints("�H��g�� %d �g�H�U", 255 - (int)bp->post_limit_badpost);
	move(b_lines, 0);

	if (!((currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP)))
	{
	    vmsg("�z�惡�O�L�޲z�v��");
	    return FULLUPDATE;
	}

	switch(tolower(getans("�п�J�n���ܪ��]�w, �䥦�䵲��: ")))
	{
#ifdef USE_AUTOCPLOG
	    case 'x':
		bp->brdattr ^= BRD_CPLOG;
		touched = 1;
		break;
#endif
	    case 'o':
		bp->brdattr ^= BRD_LOCALSAVE;
		touched = 1;
		break;

	    case 'e':
		if(HasUserPerm(PERM_SYSOP))
		{
		    bp->brdattr ^= BRD_RESTRICTEDPOST;
		    touched = 1;
		} else {
		    vmsg("�����]�w�ݭn�����v��");
		}
		break;

	    case 'h':
#ifndef BMCHS
		if (!HasUserPerm(PERM_SYSOP))
		{
		    vmsg("�����]�w�ݭn�����v��");
		    break;
		}
#endif
		if(bp->brdattr & BRD_HIDE)
		{
		    bp->brdattr &= ~BRD_HIDE;
		    bp->brdattr &= ~BRD_POSTMASK;
		} else {
		    bp->brdattr |= BRD_HIDE;
		    bp->brdattr |= BRD_POSTMASK;
		}
		touched = 1;
		break;

	    case 'r':
		bp->brdattr ^= BRD_NORECOMMEND;
		touched = 1;
		break;

	    case 'i':
		bp->brdattr ^= BRD_IPLOGRECMD;
		touched = 1;
		break;

	    case 'f':
		bp->brdattr &= ~BRD_NORECOMMEND;
		bp->brdattr ^= BRD_NOFASTRECMD;
		touched = 1;

		if(bp->brdattr & BRD_NOFASTRECMD)
		{
		    char buf[8] = "";

		    if(bp->fastrecommend_pause > 0)
			sprintf(buf, "%d", bp->fastrecommend_pause);
		    getdata_str(b_lines-1, 0, 
			    "�п�J�s���ɶ�����(���: ��) [5~240]: ",
			    buf, 4, ECHO, buf);
		    if(buf[0] >= '0' && buf[0] <= '9')
			bp->fastrecommend_pause = atoi(buf);

		    if( bp->fastrecommend_pause < 5 || 
			bp->fastrecommend_pause > 240)
		    {
			if(buf[0])
			{
			    vmsg("��J�ɶ��L�ġA�Шϥ� 5~240 �������Ʀr�C");
			}
			bp->fastrecommend_pause = 0;
			bp->brdattr &= ~BRD_NOFASTRECMD;
		    }
		}
		break;
#ifndef OLDRECOMMEND
	    case 'b':
		if(bp->brdattr & BRD_NORECOMMEND)
		    bp->brdattr |= BRD_NOBOO;
		bp->brdattr ^= BRD_NOBOO;
		touched = 1;
		if (!(bp->brdattr & BRD_NOBOO))
		    bp->brdattr &= ~BRD_NORECOMMEND;
		break;
#endif
	    case '8':
		bp->brdattr ^= BRD_OVER18;
		touched = 1;		
		break;

	    case 'y':
		if (!(HasUserPerm(PERM_SYSOP) || (HasUserPerm(PERM_SYSSUPERSUBOP) && GROUPOP()) ) ) {
		    vmsg("�����]�w�ݭn�s�ժ��ί����v��");
		    break;
		}
		bp->brdattr ^= BRD_NOREPLY;
		touched = 1;		
		break;

	    default:
		finished = 1;
		break;
	}
    }
    if(touched)
    {
	assert(0<=currbid-1 && currbid-1<MAX_BOARD);
	substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
	vmsg("�w�x�s�s�]�w");
    }
    else
	vmsg("�����ܥ���]�w");

    return FULLUPDATE;
}

/* ----------------------------------------------------- */
/* �O�D�]�w����/ ������                                  */
/* ----------------------------------------------------- */
char            board_hidden_status;
#ifdef  BMCHS
static int
change_hidden(void)
{
    boardheader_t   *bp;
    if (!((currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP)))
	return DONOTHING;

    bp = getbcache(currbid);
    if (((bp->brdattr & BRD_HIDE) && (bp->brdattr & BRD_POSTMASK))) {
	if (getans("�ثe�ݪO���Τ�, �n�Ѱ���(y/N)?") != 'y')
	    return FULLUPDATE;
	bp->brdattr &= ~BRD_HIDE;
	bp->brdattr &= ~BRD_POSTMASK;
	outs("�g�ߤ��ǲ��H�A�L�B���D���q�C\n");
	board_hidden_status = 0;
	hbflreload(currbid);
    } else {
	if (getans("�n�]�w�ݪO�����ζ�(y/N)?") != 'y')
	    return FULLUPDATE;
	bp->brdattr |= BRD_HIDE;
	bp->brdattr |= BRD_POSTMASK;
	outs("�g�ߤ��w����A���ߵ��۬í��C\n");
	board_hidden_status = 1;
    }
    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
    substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
    log_usies("SetBoard", bp->brdname);
    pressanykey();
    return FULLUPDATE;
}

static int
change_counting(void)
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
    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
    substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
    pressanykey();
    return FULLUPDATE;
}

#endif

/**
 * ���ܥثe�Ҧb�O�峹���w�]�x�s�覡
 */
static int
change_localsave(void)
{
    vmsg("���\\��w��X�i�j�g I �ݪO�]�w�A�Ы� I �]�w�C");
    return FULLUPDATE;
}

#ifdef USE_COOLDOWN

int check_cooldown(boardheader_t *bp)
{
    int diff = cooldowntimeof(usernum) - now; 
    int i, limit[8] = {4000,1,2000,2,1000,3,-1,10};

    if(diff<0)
	SHM->cooldowntime[usernum - 1] &= 0xFFFFFFF0;
    else if( !((currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP)))
    {
      if( bp->brdattr & BRD_COOLDOWN )
       {
  	 vmsgf("�N�R�@�U�a�I (���� %d �� %d ��)", diff/60, diff%60);
	 return 1;
       }
      else if(posttimesof(usernum)==0xf)
      {
	 vmsgf("�藍�_�A�z�Q�]�H��I (���� %d �� %d ��)", diff/60, diff%60);
	 return 1;
      }
#ifdef NO_WATER_POST
      else
      {
        for(i=0; i<4; i++)
          if(bp->nuser>limit[i*2] && posttimesof(usernum)>=limit[i*2+1])
          {
	    vmsgf("�藍�_�A�z���峹�α���Ӥ��o�I (���� %d �� %d ��)", 
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
change_cooldown(void)
{
    char genbuf[256] = {'\0'};
    boardheader_t *bp = getbcache(currbid);
    
    if (!(HasUserPerm(PERM_SYSOP | PERM_POLICE) || 
        (HasUserPerm(PERM_SYSSUPERSUBOP) && GROUPOP())))
	return DONOTHING;

    if (bp->brdattr & BRD_COOLDOWN) {
	if (getans("�ثe���Ť�, �n�}���(y/N)?") != 'y')
	    return FULLUPDATE;
	bp->brdattr &= ~BRD_COOLDOWN;
	outs("�j�a���i�H post �峹�F�C\n");
    } else {
	getdata(b_lines - 1, 0, "�п�J�N�R�z�ѡG", genbuf, 50, DOECHO);
	if (getans("�n���� post �W�v, ���Ŷ�(y/N)?") != 'y')
	    return FULLUPDATE;
	bp->brdattr |= BRD_COOLDOWN;
	outs("�}�l�N�R�C\n");
    }
    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
    substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
    post_policelog(bp->brdname, NULL, "�N�R", genbuf, bp->brdattr & BRD_COOLDOWN);
    pressanykey();
    return FULLUPDATE;
}
#endif

/* ----------------------------------------------------- */
/* �ݪO�\���                                            */
/* ----------------------------------------------------- */
/* onekey_size was defined in ../include/pttstruct.h, as ((int)'z') */
const onekey_t read_comms[] = {
    { 1, show_filename }, // Ctrl('A') 
    { 0, NULL }, // Ctrl('B')
    { 0, NULL }, // Ctrl('C')
    { 0, NULL }, // Ctrl('D')
    { 1, lock_post }, // Ctrl('E')
    { 0, NULL }, // Ctrl('F')
#ifdef NO_GAMBLE
    { 0, NULL }, // Ctrl('G')
#else
    { 0, hold_gamble }, // Ctrl('G')
#endif
    { 0, NULL }, // Ctrl('H')
    { 0, board_digest }, // Ctrl('I') KEY_TAB 9
    { 0, NULL }, // Ctrl('J')
    { 0, NULL }, // Ctrl('K')
    { 0, NULL }, // Ctrl('L')
    { 0, NULL }, // Ctrl('M')
#ifdef BMCHS
    { 0, change_counting }, // Ctrl('N')
#else
    { 0, NULL }, // Ctrl('N')
#endif
    { 0, do_post_openbid }, // Ctrl('O')
    { 0, do_post }, // Ctrl('P')
    { 0, NULL }, // Ctrl('Q')
    { 0, NULL }, // Ctrl('R')
    { 0, NULL }, // Ctrl('S')
    { 0, NULL }, // Ctrl('T')
    { 0, NULL }, // Ctrl('U')
    { 0, do_post_vote }, // Ctrl('V')
    { 0, whereami }, // Ctrl('W')
    { 0, change_localsave }, // Ctrl('X')
    { 0, NULL }, // Ctrl('Y')
    { 1, push_bottom }, // Ctrl('Z') 26
    { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL },
    { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL },
    { 1, recommend }, // '%' (m3itoc style)
    { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL },
    { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL },
    { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL },
    { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL },
    { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL },
    { 0, NULL }, { 0, NULL }, { 0, NULL },
    { 0, NULL }, // 'A' 65
    { 0, bh_title_edit }, // 'B'
    { 1, do_limitedit }, // 'C'
    { 1, del_range }, // 'D'
    { 1, edit_post }, // 'E'
    { 0, NULL }, // 'F'
    { 0, NULL }, // 'G'
#ifdef BMCHS
    { 0, change_hidden }, // 'H'
#else
    { 0, NULL }, // 'H'
#endif
    { 0, b_config }, // 'I'
#ifdef USE_COOLDOWN
    { 0, change_cooldown }, // 'J'
#else
    { 0, NULL }, // 'J'
#endif
    { 0, b_water_edit }, // 'K'
    { 1, solve_post }, // 'L'
    { 0, b_vote_maintain }, // 'M'
    { 0, NULL }, // 'N'
    { 0, b_post_note }, // 'O'
    { 0, NULL }, // 'P'
    { 1, view_postmoney }, // 'Q'
    { 0, b_results }, // 'R'
    { 0, NULL }, // 'S'
    { 1, edit_title }, // 'T'
    { 0, NULL }, // 'U'
    { 0, b_vote }, // 'V'
    { 0, b_notes_edit }, // 'W'
    { 1, recommend }, // 'X'
    { 1, recommend_cancel }, // 'Y'
    { 0, NULL }, // 'Z' 90
    { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL },
    { 0, NULL }, // 'a' 97
    { 0, b_notes }, // 'b'
    { 1, cite_post }, // 'c'
    { 1, del_post }, // 'd'
    { 0, NULL }, // 'e'
#ifdef NO_GAMBLE
    { 0, NULL }, // 'f'
#else
    { 0, join_gamble }, // 'f'
#endif
    { 1, good_post }, // 'g'
    { 0, b_help }, // 'h'
    { 0, b_posttype }, // 'i'
    { 0, NULL }, // 'j'
    { 0, NULL }, // 'k'
    { 0, NULL }, // 'l'
    { 1, mark_post }, // 'm'
    { 0, NULL }, // 'n'
    { 0, can_vote_edit }, // 'o'
    { 0, NULL }, // 'p'
    { 0, NULL }, // 'q'
    { 1, read_post }, // 'r'
    { 0, do_select }, // 's'
    { 0, NULL }, // 't'
#ifdef OUTJOBSPOOL
    { 0, tar_addqueue }, // 'u'
#else
    { 0, NULL }, // 'u'
#endif
    { 0, visable_list_edit }, // 'v'
    { 1, b_call_in }, // 'w'
    { 1, cross_post }, // 'x'
    { 1, reply_post }, // 'y'
    { 0, b_man }, // 'z' 122
};

int
Read(void)
{
    int             mode0 = currutmp->mode;
    int             stat0 = currstat, tmpbid = currutmp->brc_id;
    char            buf[PATHLEN];
#ifdef LOG_BOARD
    time4_t         usetime = now;
#endif

    const char *bname = currboard[0] ? currboard : DEFAULT_BOARD;
    if (enter_board(bname) < 0)
	return 0;

    setutmpmode(READING);

    if (board_note_time && board_visit_time < *board_note_time) {
	int mr;

	setbfile(buf, currboard, fn_notes);
	mr = more(buf, NA);
	if(mr == -1)
            *board_note_time=0;
	else if (mr != READ_NEXT)
	    pressanykey();
    }
    i_read(READING, currdirect, readtitle, readdoent, read_comms,
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

    currstat = SELECT;
    if (do_select() == NEWDIRECT)
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
    do_select();
    return 0;
}

#ifdef HAVEMOBILE
void
mobile_message(const char *mobile, char *message)
{
    bsmtp(fpath, title, rcpt);
}
#endif
