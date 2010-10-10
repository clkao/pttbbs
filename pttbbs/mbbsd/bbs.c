
#include "bbs.h"

#ifdef EDITPOST_SMARTMERGE

#include "fnv_hash.h"
#define SMHASHLEN (64/8)

#endif // EDITPOST_SMARTMERGE

#define WHEREAMI_LEVEL	16

static int recommend(int ent, fileheader_t * fhdr, const char *direct);
static int do_add_recommend(const char *direct, fileheader_t *fhdr,
		 int ent, const char *buf, int type);
static int view_postinfo(int ent, const fileheader_t * fhdr, const char *direct, int crs_ln);

static int bnote_lastbid = -1; // �M�w�O�_�n��ܶi�O�e���� cache

// recommend/comment types
//  most people use recommendation just for one-line reply. 
//  so we may change default to (RECTYPE_ARROW)= comment only.
//  however, the traditional behavior (which does not have
//  BRC info for 'new comments available') uses RECTYPE_GOOD.
enum {
    RECTYPE_GOOD,
    RECTYPE_BAD,
    RECTYPE_ARROW,

    RECTYPE_SIZE,
    RECTYPE_MAX     = RECTYPE_SIZE-1,
    RECTYPE_DEFAULT = RECTYPE_GOOD, // match traditional user behavior
};

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
	char genbuf[PATHLEN];

	/* it is assumed that in MODE_SELECT, currboard is selected. */
	setbfile(genbuf, currboard, FN_DIR);
	get_record(genbuf, &hdr, sizeof(hdr), pfh->multi.refer.ref);
	pfh = &hdr;
    }

    if(pfh->filemode & INVALIDMONEY_MODES || pfh->multi.money > MAX_POST_MONEY)
	return -1;

    return pfh->multi.money;
}

// lite weight version to update dir files
static int 
modify_dir_lite(
	const char *direct, int ent, const char *fhdr_name,
	time4_t modified, const char *title, char recommend)
{
    // since we want to do 'modification'...
    int fd;
    off_t sz = dashs(direct);
    fileheader_t fhdr;

    // TODO lock?
    // PttLock(fd, offset, size, F_WRLCK);
    // write(fd, rptr, size);
    // PttLock(fd, offset, size, F_UNLCK);
    
    // prevent black holes
    if (sz < sizeof(fileheader_t) * (ent) ||
	    (fd = open(direct, O_RDWR)) < 0 )
	return -1;

    // also check if fhdr->filename is same.
    // let sz = base offset
    sz = (sizeof(fileheader_t) * (ent-1));
    if (lseek(fd, sz, SEEK_SET) < 0 ||
	read(fd, &fhdr, sizeof(fhdr)) != sizeof(fhdr) ||
	strcmp(fhdr.filename, fhdr_name) != 0)
    {
	close(fd);
	return -1;
    }

    // update records
    if (modified > 0)
	fhdr.modified = modified;

    if (title && *title)
	strcpy(fhdr.title, title);

    if (recommend) 
    {
	recommend += fhdr.recommend;
	if (recommend > MAX_RECOMMENDS) recommend = MAX_RECOMMENDS;
	else if (recommend < -MAX_RECOMMENDS) recommend = -MAX_RECOMMENDS;
	fhdr.recommend = recommend;
    }

    if (lseek(fd, sz, SEEK_SET) >= 0)
	write(fd, &fhdr, sizeof(fhdr));

    close(fd);

    return 0;
}

static void 
check_locked(fileheader_t *fhdr)
{
    boardheader_t *bp = NULL;

    if (currstat == RMAIL)
	return;
    if (!currboard[0] || currbid <= 0)
	return;
    bp = getbcache(currbid);
    if (!bp)
	return;
    if (!(fhdr->filemode & FILE_SOLVED))
	return;
    if (!(fhdr->filemode & FILE_MARKED))
	return;
    syncnow();
    bp->SRexpire = now;
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
    log_filef("etc/illegal_money",  LOG_CREAT,
             ANSI_COLOR(1;33;46) "%s "
             ANSI_COLOR(37;45) "cross post �峹 "
             ANSI_COLOR(37) " %s" ANSI_RESET "\n", 
             cuser.userid, Cdatelite(&now));
    post_violatelaw(cuser.userid, BBSMNAME "�t��ĵ��", 
	    "Cross-post", "�@��B��");
    pwcuViolateLaw();
    mail_id(cuser.userid, "Cross-Post�@��",
	    "etc/crosspost.txt", BBSMNAME "ĵ���");
    if ((now - cuser.firstlogin) / DAY_SECONDS < 14)
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
    vs_hdr("ú�@�椤��");

    // XXX reload lots of stuff here?
    pwcuReload();
    if (!(cuser.userlevel & PERM_VIOLATELAW)) {
	vmsg("�A�S���Q�}�@��~~");
	return 0;
    }

    day =  cuser.vl_count*3 - (now - cuser.timeviolatelaw)/DAY_SECONDS;
    if (day > 0) {
        vmsgf("�̷ӹH�W����, �A�ٻݭn�Ϭ� %d �Ѥ~��ú�@��", day);
	return 0;
    }
    reload_money();
    if (cuser.money < (int)cuser.vl_count * 1000) {
	snprintf(buf, sizeof(buf),
		 ANSI_COLOR(1;31) "�o�O�A�� %d ���H�ϥ����k�W"
		 "����ú�X %d ���F���A�ثe�u�� %d ��, ��������!!" ANSI_RESET,
           (int)cuser.vl_count, (int)cuser.vl_count * 1000, cuser.money);
	mvouts(22, 0, buf);
	pressanykey();
	return 0;
    }
    move(5, 0);
    prints("�o�O�A�� %d ���H�k ����ú�X %d ��\n\n", 
	    cuser.vl_count, cuser.vl_count * 1000);
    outs(ANSI_COLOR(1;37) "�A���D��? �]���A���H�k "
	   "�w�g�y���ܦh�H�����K" ANSI_RESET "\n");
    outs(ANSI_COLOR(1;37) "�A�O�_�T�w�H�ᤣ�|�A�ǤF�H" ANSI_RESET "\n");

    if (!getdata(10, 0, "�T�w�ܡH[y/N]:", ok, sizeof(ok), LCECHO) ||
	ok[0] != 'y') 
    {
	move(15, 0);
	outs( ANSI_COLOR(1;31) "���Q�I���ܡH �٬O�����D�n�� y �H\n"
	    "�оi���ݲM���t�ΰT�����n�ߺD�C\n"
	    "���A�Q�q�F�A�ӧa!! �ڬ۫H�A���|�������諸~~~" ANSI_RESET);
	pressanykey();
	return 0;
    }

    //Ptt:check one more time
    reload_money();
    if (cuser.money < (int)cuser.vl_count * 1000) 
    {
	log_filef("log/violation", LOG_CREAT,
		"%s %s pay-violation error: race-conditionn hack?\n", 
		Cdate(&now), cuser.userid);
	vmsg("����򩿵M�����F�H �չϴ��F�t�γQ�d��N��b���I");
	return 0; 
    }

    pay(1000 * (int)cuser.vl_count, "ú�I�@�� (#%d)", cuser.vl_count);
    pwcuSaveViolateLaw();
    log_filef("log/violation", LOG_CREAT,
	    "%s %s pay-violation: $%d complete.\n", 
	    Cdate(&now), cuser.userid, (int)cuser.vl_count*1000);

    vmsg("�@��w�I�A�Э��s�n�J�C");
    u_exit("save_violate");
    exit(0);
    return 0;
}

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
	!is_hidden_board_friend((int)(bp - bcache) + 1, currutmp->uid) )
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
	    char buf[PATHLEN];
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
        (HasUserPerm(PERM_ALLBOARD) ||
         is_BM_cache(currbid) ||
         (bp->BM[0] <= ' ' && GROUPOP()))) {
	currmode |= MODE_BOARD;
    }
}

// post �峹��������O
int IsFreeBoardName(const char *brdname)
{
    if (strcmp(brdname, BN_TEST) == 0)
	return 1;
    if (strcmp(brdname, BN_ALLPOST) == 0)
	return 1;
    // since DEFAULT_BOARD is allowed to post without
    // special permission, we should not give rewards.
    if (strcasecmp(brdname, DEFAULT_BOARD) == 0)
	return 1;
    return 0;
}

/* check post perm on demand, no double checks in current board
 * currboard MUST be defined!
 * XXX can we replace currboard with currbid ? */
int
CheckPostPerm(void)
{
    if (currmode & MODE_DIGEST)
	return 0;
    return CheckModifyPerm();
}

int
CheckModifyPerm(void)
{
    static time4_t last_chk_time = 0x0BAD0BB5; /* any magic number */
    static int last_board_index = 0; /* for speed up */
    int valid_index = 0;
    boardheader_t *bp = NULL;
    
    // check if my own permission is changed.
    if (ISNEWPERM(currutmp))
    {
#ifdef DEBUG
	log_filef("log/newperm.log", LOG_CREAT,
		"%-13s: reloaded perm %s\n",
		cuser.userid, Cdate(&now));
#endif
	currmode &= ~MODE_POSTCHECKED;
	pwcuReload();
	// XXX let pwcuReload handle NEWPERM?
	CLEAR_ALERT_NEWPERM(currutmp);
    }

    if (currmode & MODE_POSTCHECKED)
    {
        // if board is different, rebuild cache
        if (currbid != last_board_index) {
#ifdef DEBUG
            log_filef("log/reload_board_perm.log", LOG_CREAT,
                      "%-13s: reload board perm (curr=%d, last=%d)\n",
                      cuser.userid, currbid, last_board_index);
#endif
            currmode &= ~MODE_POSTCHECKED;
            last_board_index = 0;
        }

	/* checked? let's check if perm reloaded or board changed*/
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
    boardheader_t *bp;
    if (HasUserPerm(PERM_SYSOP))
	return 1;
    assert(0<=bid-1 && bid-1<MAX_BOARD);

    // XXX currmode �O�ثe�ݪO���O bid...
    if (is_BM_cache(bid))
	return 1;
    bp = getbcache(bid);

    if (cuser.firstlogin > (now - (time4_t)bp->post_limit_regtime * MONTH_SECONDS))
	return 0;
    if (cuser.numlogindays / 10 < (unsigned int)bp->post_limit_logins)
	return 0;
    // XXX numposts itself is an integer, but some records (by del post!?) may
    // create invalid records as -1... so we'd better make it signed for real
    // comparison.
    if ((int)(cuser.numposts  / 10) < (int)(unsigned int)bp->post_limit_posts)
	return 0;

#ifdef ASSESS
    if  (cuser.badpost > (255 - (unsigned int)bp->post_limit_badpost))
	return 0;
#endif

    return 1;
}

static void
readtitle(void)
{
    boardheader_t  *bp;
    char    *brd_title;
    char     buf[32];

    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
    bp = getbcache(currbid);

    if(bp->bvote != 2 && bp->bvote)
	brd_title = "���ݪO�i��벼��";
    else
	brd_title = bp->title + 7;

    showtitle(currBM, brd_title);
    outs("[��]���} [��]�\\Ū [Ctrl-P]�o��峹 [d]�R�� [z]��ذ� [i]�ݪO��T/�]�w [h]����\n");
    buf[0] = 0;

#ifdef USE_COOLDOWN
    if (!(bp->brdattr & BRD_COOLDOWN))
#endif
    {
	snprintf(buf, sizeof(buf), "�H��:%d ", SHM->bcache[currbid - 1].nuser);
    }

    vbarf(ANSI_REVERSE "   �s��    %s �@  ��       ��  ��  ��  �D\t%s ", 
	    IS_LISTING_MONEY ? listmode_desc[LISTMODE_MONEY] : listmode_desc[currlistmode],
	    buf);
}

static int
is_tn_announce(const char *title)
{
    return strncmp(title, TN_ANNOUNCE, strlen(TN_ANNOUNCE)) == 0;
}

static int
is_tn_allowed(const char *title)
{
#ifdef ALLOW_FREE_TN_ANNOUNCE
    return 1;
#else
    // TN_ANNOUNCE is prohibited for non-BMs
    if ((currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP) ||
	HasUserPerm(PERM_ACCOUNTS | PERM_BOARD | PERM_BBSADM | 
		    PERM_VIEWSYSOP| PERM_POLICE_MAN))
	return 1;

    // Note: ���� subgroup op ���P�w�ثe�]�O�@���V - �p�ժ��n�q�ۤv������
    // �i�h�~�|�� GROUPOP(). ���L�ϥ��p�ժ���s�ժ����H�S����h�A�N�}��L��
    // always �i�H�ϥ� TN_ANNOUNCE �a�C
    if (HasUserPerm(PERM_SYSSUPERSUBOP) ||
	HasUserPerm(PERM_SYSSUBOP))
	return 1;

    if (is_tn_announce(title))
	return 0;
    return 1;
#endif
}

static void
tn_safe_strip(char *title)
{
    if (is_tn_allowed(title))
	return;
    assert(is_tn_announce(title));
    memmove(title, title + strlen(TN_ANNOUNCE), 
	    strlen(title) - strlen(TN_ANNOUNCE)+1);
}

static void
readdoent(int num, fileheader_t * ent)
{
    int  type = ' ';
    char *mark, *title,
	 color, special = 0, isonline = 0, recom[8];
    char *typeattr = "";
    char isunread = 0, oisunread = 0;

#ifdef DETECT_SAFEDEL
    char iscorpse = (ent->owner[0] == '-') && (ent->owner[1] == 0);

    if (!iscorpse)
    {
#endif

    oisunread = isunread = 
	brc_unread(currbid, ent->filename, ent->modified);

    // modified tag
    if (isunread == 2)
    {
	// ignore unread, if user doesn't want to show it.
	if (HasUserFlag(UF_NO_MODMARK))
	{
	    oisunread = isunread = 0;
	}
	// if user wants colored marks, use 'read' marks
	else if (HasUserFlag(UF_COLORED_MODMARK))
	{
	    isunread = 0;
	    typeattr = ANSI_COLOR(36);
	}
    }

    // handle 'type'
    type = isunread ? '+' : ' ';
    if (isunread == 2) type = '~';

    if ((currmode & MODE_BOARD) && (ent->filemode & FILE_DIGEST))
    {
	type = (type == ' ') ? '*' : '#';
    }
    else if (ent->filemode & FILE_MARKED) // marks should be visible to everyone.
    {
	if(ent->filemode & FILE_SOLVED)
	    type = '!';
	else if (isunread == 0)
	    type = 'm';
	else if (isunread == 1)
	    type = 'M';
	else if (isunread == 2)
	    type = '=';
    }
    else if (ent->filemode & FILE_SOLVED)
    {
	type = (type == ' ') ? 's': 'S';
    }

    // tag should override everything
    if ((currmode & MODE_BOARD) || HasUserPerm(PERM_LOGINOK)) 
    {
	if (TagNum && !Tagger(atoi(ent->filename + 2), 0, TAG_NIN))
	    type = 'D';
    }

    // the only special case: ' ' with isunread == 2,
    // change to '+' with gray attribute.
    if (type == ' ' && oisunread == 2)
    {
	typeattr = ANSI_COLOR(1;30);
	type = '+';
    }
#ifdef DETECT_SAFEDEL
    } // if(!iscorpse)
    else {
	// quick display
	SOLVE_ANSI_CACHE();
	outs(ANSI_COLOR(1;30));
	prints("%7d    ", num);
	prints("%-6.5s", ent->date);
	prints("%-13.12s", ent->owner);
	prints("�� %-.*s" ANSI_RESET "\n",
		t_columns-34, ent->title);
	return;
    }
#endif

    title = ent->filename[0]!='L' ? subject(ent->title) : "<������w>";
    if (ent->filemode & FILE_VOTE)
	color = '2', mark = "��";
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

    // TN_ANNOUNCE: [���i]
    if (title[0] == '[' && is_tn_announce(title))
	special = 1;

    isonline = query_online(ent->owner);

    if(ent->recommend >= MAX_RECOMMENDS)
	  strcpy(recom,"1m�z");
    else if(ent->recommend>9)
	  sprintf(recom,"3m%2d",ent->recommend);
    else if(ent->recommend>0)
	  sprintf(recom,"2m%2d",ent->recommend);
    else if(ent->recommend <= -MAX_RECOMMENDS)
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
	prints(ANSI_COLOR(%d) "%-6.5s" ANSI_RESET,
		(ent->date[3] + ent->date[4]) % 7 + 31, ent->date);
#else
	prints("%-6.5s", ent->date);
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
	prints(ANSI_COLOR(1;3%c) "%s %s" ANSI_RESET "\n",
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

static int
cancelpost(const fileheader_t *fh, int by_BM, char *newpath)
{
    FILE           *fin, *fout;
    char           *ptr, *brd;
    fileheader_t    postfile;
    char            genbuf[200];
    char            nick[STRLEN], fn1[PATHLEN];
    int             len = 42-strlen(currboard);
    int		    ret = -1;

    if(!fh->filename[0]) return ret;
    setbfile(fn1, currboard, fh->filename);
    if ((fin = fopen(fn1, "r"))) {
	brd = by_BM ? BN_DELETED : BN_JUNK;

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
        log_filef(fn1,  LOG_CREAT, "\n�� Deleted by: %s (%s) %s",
                 cuser.userid, fromhost, Cdatelite(&now));
	ret = Rename(fn1, newpath);
	setbdir(genbuf, brd);
	append_record(genbuf, &postfile, sizeof(postfile));
	setbtotal(getbnum(brd));
    }
    return ret;
}

static void
do_deleteCrossPost(const fileheader_t *fh, char bname[])
{
    char bdir[PATHLEN], file[PATHLEN];
    fileheader_t newfh;
    boardheader_t  *bp;
    int i, bid;

    if(!bname || !fh) return;
    if(!fh->filename[0]) return;

    bid = getbnum(bname);
    if(bid <= 0) return;

    bp = getbcache(bid);
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
	        safe_article_delete(i, &newfh, bdir, NULL);
        else
#endif
                delete_record(bdir, sizeof(fileheader_t), i);
	setbtotal(bid);
	unlink(file);
    }
}

static void
deleteCrossPost(const fileheader_t *fh, char *bname)
{
    if(!fh || !fh->filename[0]) return;

    if(!strcmp(bname, BN_ALLPOST) || !strcmp(bname, "NEWIDPOST") ||
       !strcmp(bname, BN_ALLHIDPOST) || !strcmp(bname, BN_UNANONYMOUS))
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
	do_deleteCrossPost(fh, BN_ALLPOST);
    }
}

void
delete_allpost(const char *userid)
{
    fileheader_t fhdr;
    int     fd, i;
    char    bdir[PATHLEN], file[PATHLEN];

    if(!userid) return;

    setbdir(bdir, BN_ALLPOST);
    if( (fd = open(bdir, O_RDWR)) != -1) 
    {
       for(i=0; read(fd, &fhdr, sizeof(fileheader_t)) >0; i++){
           if(strcmp(fhdr.owner, userid))
             continue;
           deleteCrossPost(&fhdr, BN_ALLPOST);
	   setbfile(file, BN_ALLPOST, fhdr.filename);
	   unlink(file);

	   // usually delete_allpost are initiated by system,
	   // so don't set normal safedel.
	   strcpy(fhdr.filename, FN_SAFEDEL);
	   strcpy(fhdr.owner, "-");
	   snprintf(fhdr.title, sizeof(fhdr.title),
		   "%s", STR_SAFEDEL_TITLE);

           lseek(fd, sizeof(fileheader_t) * i, SEEK_SET);
           write(fd, &fhdr, sizeof(fileheader_t));
       }
       close(fd);
    }
}

/* ----------------------------------------------------- */
/* �o��B�^���B�s��B����峹                            */
/* ----------------------------------------------------- */
static int 
solveEdFlagByBoard(const char *bn, int flags)
{
    if (
#ifdef BN_BBSMOVIE
	    strcmp(bn, BN_BBSMOVIE) == 0 ||
#endif
#ifdef BN_TEST
	    strcmp(bn, BN_TEST) == 0 ||
#endif
	    0
	)
    {
	flags |= EDITFLAG_UPLOAD | EDITFLAG_ALLOWLARGE;
    }
    return flags;
}

void
do_reply_title(int row, const char *title, char result[STRLEN])
{
    char            genbuf[200];
    char            genbuf2[4];

    if (strncasecmp(title, str_reply, 4))
	snprintf(result, STRLEN, "Re: %s", title);
    else
	strlcpy(result, title, STRLEN);
    result[TTLEN - 1] = '\0';
    snprintf(genbuf, sizeof(genbuf), "�ĥέ���D�m%.60s�n��?[Y] ", result);
    getdata(row, 0, genbuf, genbuf2, 4, LCECHO);
    if (genbuf2[0] == 'n')
	getdata(++row, 0, "���D�G", result, TTLEN, DOECHO);
}

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

    if(!strcasecmp(brd, BN_UNANONYMOUS))
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

static int
do_general(int garbage)
{
    fileheader_t    postfile;
    char            fpath[PATHLEN], buf[STRLEN];
    int i, j;
    int             defanony, ifuseanony;
    int             money = 0;
    char            genbuf[PATHLEN];
    const char	    *owner;
    char            ctype[8][5] = {"���D", "��ĳ", "�Q��", "�߱o",
				   "����", "�Яq", "����", 
				   "���i" // TN_ANNOUNCE
				  };
    boardheader_t  *bp;
    int             islocal, posttype=-1, edflags = 0;
    char save_title[STRLEN];

    save_title[0] = '\0';

    ifuseanony = 0;
    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
    bp = getbcache(currbid);

    if( !CheckPostPerm()
#ifdef FOREIGN_REG
	// ���O�~�y�ϥΪ̦b PttForeign �O
	&& !(HasUserFlag(UF_FOREIGN) && 
	    strcmp(bp->brdname, BN_FOREIGN) == 0)
#endif
	) {
	vmsg("�藍�_�A�z�ثe�L�k�b���o��峹�I");
	return READ_REDRAW;
    }

    if ( !CheckPostRestriction(currbid) )
    {
	vmsg("�A������`��I (�i�� i �d�ݭ���)");
	return FULLUPDATE;
    }
#ifdef USE_COOLDOWN
   if(check_cooldown(bp))
       return READ_REDRAW;
#endif
    clear();

    setbfile(genbuf, currboard, FN_POST_NOTE);

    if (more(genbuf, NA) == -1) {
	more("etc/" FN_POST_NOTE, NA);
    }
    move(19, 0);
    prints("%s��i" ANSI_COLOR(33) " %s" ANSI_RESET " �j "
	   ANSI_COLOR(32) "%s" ANSI_RESET " �ݪO\n",
	   "�o��峹",
	   currboard, bp->title + 7);

    if (quote_file[0])
	do_reply_title(20, currtitle, save_title);
    else {
	char tmp_title[STRLEN]="";
	move(21,0);
	outs("�����G");
	for(i=0; i<8 && bp->posttype[i*4]; i++)
	    strlcpy(ctype[i],bp->posttype+4*i,5);
	if(i==0) i=8;
	for(j=0; j<i; j++)
	    prints("%d.%4.4s ", j+1, ctype[j]);
	sprintf(buf,"(1-%d�Τ���)",i);

	do {
	    getdata(21, 6+7*i, buf, tmp_title, 3, LCECHO); 
	    posttype = tmp_title[0] - '1';
	    if (posttype >= 0 && posttype < i)
	    {
		snprintf(tmp_title, sizeof(tmp_title),
			"[%s] ", ctype[posttype]);
	    } else {
		tmp_title[0] = '\0';
		posttype=-1;
	    }

	    if (!is_tn_allowed(tmp_title))
	    {
		// XXX the only case now is TN_ANNOUNCE.
		mvouts(22, 0, "��p�A" TN_ANNOUNCE " �O�d���O�D�ϥΡC\n");
		continue;
	    }
	    break;
	} while (1);

	do {
	    getdata_buf(22, 0, "���D�G", tmp_title, TTLEN, DOECHO);
	    strip_ansi(tmp_title, tmp_title, STRIP_ALL);

	    if (!is_tn_allowed(tmp_title))
	    {
		// XXX the only case now is TN_ANNOUNCE.
		mvouts(21, 0, "��p�A" TN_ANNOUNCE " �O�d���O�D�ϥΡC\n");
		tn_safe_strip(tmp_title);
		continue;
	    }
	    break;
	} while (1);

	strlcpy(save_title, tmp_title, sizeof(save_title));
    }
    if (save_title[0] == '\0')
	return FULLUPDATE;

    curredit &= ~EDIT_MAIL;
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
    if(posttype!=-1 && ((1<<posttype) & bp->posttype_f)) {
	setbnfile(genbuf, bp->brdname, "postsample", posttype);
	Copy(genbuf, fpath);
    }

    edflags = EDITFLAG_ALLOWTITLE;
    edflags = solveEdFlagByBoard(currboard, edflags);

#if defined(PLAY_ANGEL) && defined(BN_ANGELPRAY)
    // XXX �c�d�� code�C
    if (HasUserPerm(PERM_ANGEL) && strcmp(currboard, BN_ANGELPRAY) == 0)
    {
	currbrdattr |= BRD_ANONYMOUS;
	currbrdattr |= BRD_DEFAULTANONYMOUS;
    };
#endif
    
    // XXX I think the 'kind' determination looks really weird here.
    // However legacy BBS code here was a mass... so let's workaround it.
    edflags |= (quote_file[0] ?
	    EDITFLAG_KIND_REPLYPOST : EDITFLAG_KIND_NEWPOST);
    if (curredit & EDIT_BOTH)
	edflags |= EDITFLAG_KIND_SENDMAIL;

    money = vedit2(fpath, YEA, &islocal, save_title, edflags);
    if (money == EDIT_ABORTED) {
	unlink(fpath);
	pressanykey();
	return FULLUPDATE;
    }
    /* set owner to Anonymous for Anonymous board */

    // check TN_ANNOUNCE again for non-BMs...
    tn_safe_strip(save_title);

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

    // ---- BEGIN OF MONEY VERIFICATION ----

    // money verification
#ifdef MAX_POST_MONEY
    if (money > MAX_POST_MONEY)
	money = MAX_POST_MONEY;
#endif

    // drop money & numposts for free boards, or user without login information
    // (they may post on free boards like SYSOP)
    // including: special boards (e.g. TEST, ALLPOST), bad boards, no BM boards
    if (!HasUserPerm(PERM_LOGINOK) || 
	IsFreeBoardName(currboard) || 
	(currbrdattr&BRD_BAD) || 
	bp->BM[0] < ' ') // XXX note: BM may contain some (chinese) text that is not userid
    {
	money = 0;
    }

    // also drop for anonymos/bid posts
    if(ifuseanony) {
	money = 0;
	postfile.filemode |= FILE_ANONYMOUS;
	postfile.multi.anon_uid = currutmp->uid;
    }
    else
    {
	/* general article */
	postfile.modified = dasht(fpath);
	postfile.multi.money = money;
    }

    // ---- END OF MONEY VERIFICATION ----

    strlcpy(postfile.owner, owner, sizeof(postfile.owner));
    strlcpy(postfile.title, save_title, sizeof(postfile.title));
    if (islocal)		/* local save */
	postfile.filemode |= FILE_LOCAL;

    setbdir(buf, currboard);

    // Ptt: stamp file again to make it order
    //      fix the bug that search failure in getindex
    //      stampfile_u is used when you don't want to clear other fields
    strcpy(genbuf, fpath);
    setbpath(fpath, currboard);
    stampfile_u(fpath, &postfile);   

    if (append_record(buf, &postfile, sizeof(postfile)) == -1)
    {
        unlink(genbuf);
    }
    else
    {
	char addPost = 0;
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
	    if( ifuseanony )
		outgo_post(&postfile, currboard, owner, "Anonymous.");
	    else
		outgo_post(&postfile, currboard, cuser.userid, cuser.nickname);
	}
	brc_addlist(postfile.filename, postfile.modified);

        if( !bp->level || (currbrdattr & BRD_POSTMASK))
        {
	        if ((now - cuser.firstlogin) / DAY_SECONDS < 14)
            		do_crosspost("NEWIDPOST", &postfile, fpath, 0);

		if (!(currbrdattr & BRD_HIDE) )
            		do_crosspost(BN_ALLPOST, &postfile, fpath, 0);
	        else	
            		do_crosspost(BN_ALLHIDPOST, &postfile, fpath, 0);
	}
	outs("���Q�K�X�G�i�A");

	// Freeboard/BRD_BAD check was already done.
	if (!ifuseanony) 
	{
            if (money > 0)
	    {
                pay(-money, "%s �ݪO�o��Z�S: %s", currboard, postfile.title); 
		pwcuIncNumPost();
		addPost = 1;
		prints("�o�O�z���� %d �g���Ĥ峹�A��o�Z�S %d ��\n",
			cuser.numposts, money);
		prints("\n (�Y����ۦ�γQ�O�D�R��h������o�����Ĥ峹�ƤνZ�S�i��|�Q�^��)\n\n");
	    } else {
		// no money, no record.
		outs("���g���C�J�O���A�q�Х]�[�C");
	    }
	} 
	else 
	{
	    outs("���C�J�O���A�q�Х]�[�C");
	}
	clrtobot();

	/* �^�����@�̫H�c */

	if (curredit & EDIT_BOTH) {
	    char *str, *msg = "�^���ܧ@�̫H�c";

	    genbuf[0] = 0;
	    // XXX quote_user may contain invalid user, like '-' (deleted).
	    if (is_validuserid(quote_user))
	    {
		sethomepath(genbuf, quote_user);
		if (!dashd(genbuf))
		{
		    genbuf[0] = 0;
		    msg = err_uid;
		}
	    }

	    // now, genbuf[0] = "if user exists".
	    if (genbuf[0])
	    {
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
	    } else if ((str = strchr(quote_user, '.'))) {
#ifdef USE_LOG_INTERNETMAIL
                log_filef("log/internet_mail.log", LOG_CREAT, 
                        "%s [%s (%s)] %s -> %s: %s\n",
                        Cdatelite(&now), "do_general_reply_to_author",
                        currboard, cuser.userid, str + 1, save_title);
#endif
		if ( bsmtp(fpath, save_title, str + 1, NULL) < 0)
		    msg = "�@�̵L�k���H";
	    } else {
		// unknown user id
		msg = "�@�̵L�k���H";
	    }
	    outs(msg);
	    curredit ^= EDIT_BOTH;
	} // if (curredit & EDIT_BOTH)
	if (currbrdattr & BRD_ANONYMOUS)
            do_crosspost(BN_UNANONYMOUS, &postfile, fpath, 0);
#ifdef USE_COOLDOWN
        if(bp->nuser>30)
	{
	    if (cooldowntimeof(usernum)<now)
		add_cooldowntime(usernum, 5);
	}
	add_posttimes(usernum, 1);
#endif
	// Notify all logins
	if (addPost)
	{

	}
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

static void
do_generalboardreply(/*const*/ fileheader_t * fhdr)
{
    char            genbuf[3];
    
    assert(0<=currbid-1 && currbid-1<MAX_BOARD);

    if (!CheckPostRestriction(currbid))
    {
	getdata(b_lines - 1, 0,	
		ANSI_COLOR(1;31) "�� �L�k�^���ܬݪO�C " ANSI_RESET
		"��^���� (M)�@�̫H�c (Q)�����H[Q] ",
		genbuf, sizeof(genbuf), LCECHO);
	switch (genbuf[0]) {
	    case 'm':
		mail_reply(0, fhdr, 0);
		break;
	    default:
		break;
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
do_reply(/*const*/ fileheader_t * fhdr)
{
    boardheader_t  *bp;
    if (!fhdr || !fhdr->filename[0] || fhdr->owner[0] == '-')
	return DONOTHING;

    if (!CheckPostPerm() ) return DONOTHING;
    if (fhdr->filemode &FILE_SOLVED)
     {
       if(fhdr->filemode & FILE_MARKED)
         {
          vmsg("�ܩ�p, ���峹�w���רüаO, ���o�^��.");
          return FULLUPDATE;
         }
       if(vmsg("���g�峹�w����, �O�_�u���n�^��?(y/N)")!='y')
          return FULLUPDATE;
     }

    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
    bp = getbcache(currbid);
    if (bp->brdattr & BRD_NOREPLY) {
	// try to reply by mail.
	if (vans("�ܩ�p, ���O���}��^�Ф峹�A�n��^�H���@�̶ܡH [y/N]: ") == 'y')
	    return mail_reply(0, fhdr, 0);
	else
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

    Fnv64_t fnvseed = FNV1_64_INIT;
    assert(SMHASHLEN == sizeof(fnvseed));

    fd = open(path, O_RDONLY);
    if (fd < 0)
	return 1;

    while(  sz > 0 &&
	    (n = read(fd, buf, sizeof(buf))) > 0 )
    {
	if (n > sz) n = sz;
	fnvseed = fnv_64_buf(buf, (int) n, fnvseed);
	sz -= n;
    }
    close(fd);

    if (sz > 0) // file is different
	return 2;

    memcpy(output, (void*) &fnvseed, sizeof(fnvseed));
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
    // int		    recordTouched = 0;
    time4_t	    oldmt, newmt;
    off_t	    oldsz;
    int		    edflags = 0;
    char save_title[STRLEN];
    save_title[0] = '\0';

#ifdef EDITPOST_SMARTMERGE
    char	    canDoSmartMerge = 1;
#endif // EDITPOST_SMARTMERGE

#ifdef EXP_EDITPOST_TEXTONLY
	// experimental: "text only" editing
	edflags |= EXP_EDITPOST_TEXTONLY;
#endif

    assert(0<=currbid-1 && currbid-1<MAX_BOARD && bp);

    // special modes (plus MODE_DIGEST?)
    if( currmode & MODE_SELECT )
	return DONOTHING;

    // board check
    if (is_readonly_board(bp->brdname) ||
	(bp->brdattr & BRD_VOTEBOARD))
	return DONOTHING;

    // file check
    if (fhdr->filemode & FILE_VOTE)
	return DONOTHING;

#ifdef SAFE_ARTICLE_DELETE
    if( fhdr->filename[0] == '.' )
	return DONOTHING;
#endif

    // user and permission check
    // reason 1: BM may alter post restrictions to this board
    // reason 2: this account may be occupied by someone else.
    if (!HasUserPerm(PERM_BASIC) ||	// including guests
	!CheckPostPerm() ||		// edit needs post perm (cannot be changed in digest mode)
	!CheckPostRestriction(currbid)
	)   
	return DONOTHING;

    if (strcmp(fhdr->owner, cuser.userid) != EQUSTR)
    {
	if (!HasUserPerm(PERM_SYSOP))
	    return DONOTHING;

	// admin edit!
	log_filef("log/security", LOG_CREAT,
		"%d %s %d %s admin edit (board) file=%s\n", 
		(int)now, Cdate(&now), getpid(), cuser.userid, fpath);
    }

    edflags = EDITFLAG_ALLOWTITLE;
    edflags = solveEdFlagByBoard(bp->brdname, edflags);

    setutmpmode(REEDIT);


    // XXX ������ɰ_�A edit_post �w�g���|�� + ���F...
    // �������O Sysop Edit ����a�Φ��C
    // ���Ѧ��ŧ�ӤH�g�� mode �O��W edit �a
    //
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
    grayout(0, b_lines-2, GRAYOUT_DARK);
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


	if (vedit2(fpath, 0, NULL, save_title, edflags) == EDIT_ABORTED)
	    break;

	newmt = dasht(genbuf);

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

	    move(b_lines-7, 0);
	    clrtobot();
	    outs(ANSI_COLOR(1;33) "�� �ɮפw�Q�ק�L! ��" ANSI_RESET "\n\n");
	    outs("�i��۰ʦX�� [Smart Merge]...\n");

	    // smart merge
	    if (AppendTail(genbuf, fpath, oldsz) == 0)
	    {
		// merge ok
		oldmt = newmt;
		outs(ANSI_COLOR(1) 
		    "�X�֦��\\�A�s�ק�(�α���)�w�[�J�z���峹���C\n" 
		    ANSI_RESET "\n");
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

	    move(b_lines-7, 0);
	    clrtobot();
	    outs(ANSI_COLOR(1;31) "�� �ɮפw�Q�ק�L! ��" ANSI_RESET "\n\n");

	    outs("�i��O�z�b�s�誺�L�{�����H�i�����έפ�C\n"
	 	 "�z�i�H��ܪ����л\\�ɮ�(y)�B���(n)�A\n"
		 " �άO" ANSI_COLOR(1)"���s�s��" ANSI_RESET
		 "(�s��|�Q�K���s���ɮ׫᭱)(e)�C\n");
	    c = tolower(vans("�n�����л\\�ɮ�/����/���s�� [Y/n/e]�H"));

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

		    fprintf(fp, MSG_SEPARATOR "\n");
		    fprintf(fp, "�H�U���Q�ק�L���̷s���e: ");
		    fprintf(fp,
			    " (%s)\n",
			    Cdate_mdHM(&newmt));
		    fprintf(fp, MSG_SEPARATOR "\n");
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

	// piaip Wed Jan  9 11:11:33 CST 2008
	// in order to prevent calling system 'mv' all the
	// time, it is better to unlink() first, which
	// increased the chance of succesfully using rename().
	// WARNING: if genbuf and fpath are in different directory,
	// you should disable pre-unlinking
	unlink(genbuf);
        Rename(fpath, genbuf);

	fhdr->modified = dasht(genbuf);
	strlcpy(fhdr->title, save_title, sizeof(fhdr->title));

	if (fhdr->modified > 0)
	{
	    // substitute_ref_record(direct, fhdr, ent);
	    modify_dir_lite(direct, ent, fhdr->filename,
		    fhdr->modified, save_title, 0);

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
cp_IsHiddenBoard(const boardheader_t *bp)
{
    // rules: see HasBoardPerm().
    if ((bp->brdattr & BRD_HIDE) && (bp->brdattr & BRD_POSTMASK)) 
	return 1;
    if (bp->level && !(bp->brdattr & BRD_POSTMASK))
	return 1;
    return 0;
}

int
old_cross_post(int ent, fileheader_t * fhdr, const char *direct)
{
    vmsg("���F�קK�z�~���A�������w�אּ Ctrl-X");
    return  PARTUPDATE;
}

static int
cross_post(int ent, fileheader_t * fhdr, const char *direct)
{
    char            xboard[20], fname[PATHLEN], xfpath[PATHLEN], xtitle[80];
    char            inputbuf[10], genbuf[200], genbuf2[4];
    fileheader_t    xfile;
    FILE           *xptr;
    int             author, xbid, hashPost;
    boardheader_t  *bp;

    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
    bp = getbcache(currbid);

    if (bp && (bp->brdattr & BRD_VOTEBOARD) )
	return DONOTHING;

    // if file is SAFE_DELETED, skip it.
    if (fhdr->owner[0] == '-')
	return DONOTHING;

    setbfile(fname, currboard, fhdr->filename);
    if (!dashf(fname))
    {
	vmsg("�ɮפw���s�b�C");
	return FULLUPDATE;
    }

    // XXX TODO ���קK�H�k�ϥΪ̤j�q��ӶD�O���A���w�C���o��q�C
    if (HasUserPerm(PERM_VIOLATELAW))
    {
	static int violatecp = 0;
	if (violatecp++ >= MAX_CROSSNUM)
	    return DONOTHING;
    }

    // prompt user what he's going to do now.
    move(2, 0);
    if (is_BM_cache(currbid)) {
        SOLVE_ANSI_CACHE();
        clrtoeol();
        outs("�ǳƶi��峹����C�O�D�n�m���峹�Ч�� "
                ANSI_COLOR(1;31) "_" ANSI_RESET " (���� "
                ANSI_COLOR(1;36) "Shift" ANSI_RESET " �A�� " 
                ANSI_COLOR(1;36) "-" ANSI_RESET
                " )\n");
    }

#ifdef USE_AUTOCPLOG
    // anti-crosspost spammers
    //
    // some spammers try to cross-post to other boards without
    // restriction (see pakkei0712* events on 2007/12)
    // for (1) increase numpost (2) flood target board 
    // (3) flood original post
    // You must have post permission on current board
    //
    if( (bp->brdattr & BRD_CPLOG) && 
	(!CheckPostPerm() || !CheckPostRestriction(currbid)))
    {
	vmsg("�ѥ��O����峹�ݦ��o���v��(�i�� i �d�ݭ���)");
	return FULLUPDATE;
    }
#endif // USE_AUTOCPLOG

    if (postrecord.times > 1) {
	outs(ANSI_COLOR(1;31) 
	"�Ъ`�N: �Y�L�q��������N�����~�O�A�ɭP�Q�}�@�氱�v�C\n" ANSI_RESET
	"�Y���S�O�ݨD�Ь��U�O�D�A�ХL�����A���C\n\n");
    }

    move(1, 0);

    CompleteBoard("������峹��ݪO�G", xboard);
    if (*xboard == '\0')
	return FULLUPDATE;

    if (!haspostperm(xboard))
    {
	vmsg("�ݪO���s�b�θӬݪO�T��z�o��峹�I");
	return FULLUPDATE;
    }

    /* ���n�ɥ��ܼơA�O����S����ʡA�H���V�ê��N������� */

    // XXX cross-posting a series of articles should not be cross-post?
    // so let's use filename instead of title.
    // hashPost = StringHash(fhdr->title); // why use title?
    hashPost = StringHash(fhdr->filename); // let's try filename
    xbid = getbnum(xboard);
    assert(0<=xbid-1 && xbid-1<MAX_BOARD);

    if (xbid == currbid)
    {
	vmsg("�P�O��������C");
	return FULLUPDATE;
    }

    // check target postperm
    if (!CheckPostRestriction(xbid))
    {
	vmsg("�A������`��I (�i�b�ت��ݪO���� i �d�ݭ���)");
	return FULLUPDATE;
    }

    // quick check: if already cross-posted, reject.
    if (hashPost == postrecord.checksum[0])
    {
	if (xbid == postrecord.last_bid) 
	{
	    vmsg("�o�g�峹�w�g����L�F�C");
	    return FULLUPDATE;
	} 
	else if (postrecord.times >= MAX_CROSSNUM)
	{
	    anticrosspost();
	    return FULLUPDATE;
	}
    }

#ifdef USE_COOLDOWN
    if(check_cooldown(getbcache(xbid)))
    {
	vmsg("�ӬݪO�{�b�L�k����C");
	return FULLUPDATE;
    }
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
    if (genbuf2[0] == 'n') {
	if (getdata_str(2, 0, "���D�G", genbuf, TTLEN, DOECHO, xtitle))
	    strlcpy(xtitle, genbuf, sizeof(xtitle));
    }
    // FIXME �o�̥i��|���H�����ͥX�O�d���D(�p[���i])
    // ���L��F�A�����H�h�o�ؤH�����K
    // �ϥ����ӴN�u�O�Q�ѨM�u���p�ߡv�άO�u���ˤ��p�ߡv�Ψ쪺���ΡC
    // tn_safe_strip_board(xtitle, xboard);

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

	save_currboard = currboard;
	currboard = xboard;
	write_header(xptr, xfile.title);
	currboard = save_currboard;

	if (cp_IsHiddenBoard(bp))
	{
	    /* invisible board */
	    fprintf(xptr, "�� [��������۬Y���άݪO]\n\n");
	    b_suckinfile_invis(xptr, fname, currboard);
	} else {
	    /* public board */
	    // XXX we should add some string length checks here.
	    // maybe someday we will define the standard aidc length
	    // and helper functions to create aidc string with prefixes.
	    aidu_t aidu = 0;
	    char aidc[32] = {0};

	    aidu = fn2aidu((char *)fhdr->filename);
	    if (aidu > 0) {
		aidc[0] = ' '; aidc[1] = '#';
		aidu2aidc(aidc + strlen(aidc), aidu);
		// add trailing space
		strcat(aidc, " ");
	    }
	    fprintf(xptr, "�� [��������� %s �ݪO%s]\n\n", currboard, aidc);
	    b_suckinfile(xptr, fname);
	}

	addsignature(xptr, 0);
	fclose(xptr);

#ifdef USE_AUTOCPLOG
	/* add cp log. bp is currboard now. */
	if(bp->brdattr & BRD_CPLOG)
	{
	    char buf[PATHLEN], tail[STRLEN];
	    char bname[STRLEN] = "";
	    int maxlength = 51 +2 - 6;
	    int bid = getbnum(xboard);

	    assert(0<=bid-1 && bid-1<MAX_BOARD);
	    bp = getbcache(bid);
	    if (cp_IsHiddenBoard(bp))
	    {
		strcpy(bname, "�Y���άݪO");
	    } else {
		sprintf(bname, "�ݪO %s", xboard);
	    }

	    maxlength -= (strlen(cuser.userid) + strlen(bname));

#ifdef GUESTRECOMMEND
	    snprintf(tail, sizeof(tail),
		    "%15s %s",
		    FROMHOST, Cdate_md(&now));
#else
	    maxlength += (15 - 6);
	    snprintf(tail, sizeof(tail),
		    " %s",
		    Cdate_mdHM(&now));
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
	    /* now point bp to new bord */
	    // NOTE: xbid should be already fetched before.
	    assert(xbid == getbnum(xboard));
	    assert(0<=xbid-1 && xbid-1<MAX_BOARD);
	    bp = getbcache(xbid);
	}

	/*
	 * Cross fs�����D else { unlink(xfpath); link(fname, xfpath); }
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
	outs("�峹��������C(������W�[�峹�ơA�q�Х]�[)\n\n");

	// update crosspost record
	if (is_BM_cache(xbid)) {
	    // ignore BM for cross-posting.
	    outs(ANSI_COLOR(1;32) "���g���O�D����A���۰��ˬd�]���p�JCP" 
		 ANSI_RESET);
	} else if (hashPost == postrecord.checksum[0]) 
	    // && xbid != postrecord.last_bid)
	{
	    ++postrecord.times; // check will be done next time.

	    if (postrecord.times == MAX_CROSSNUM) 
	    {
		outs(ANSI_COLOR(1;31) " ĵ�i: �Y�N�F��������ƤW���A"
			"�U���N�}�@��!" ANSI_RESET);
	    }
	} else {
	    // reset cross-post record
	    postrecord.times = 0;
	    postrecord.last_bid = xbid;
	    postrecord.checksum[0] = hashPost;
	}

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

    if (fhdr->owner[0] == '-' || fhdr->filename[0] == 'L' || !fhdr->filename[0])
	return READ_SKIP;

    STATINC(STAT_READPOST);
    setdirpath(genbuf, direct, fhdr->filename);

    more_result = more(genbuf, YEA);

#ifdef LOG_CRAWLER
    {
        // kcwu: log crawler
	static int read_count = 0;
        extern Fnv32_t client_code;

        ++read_count;
        if (read_count % 1000 == 0) {
            time4_t t = time4(NULL);
            log_filef("log/read_alot", LOG_CREAT,
		    "%d %s %d %s %08x %d\n", t, Cdate(&t), getpid(),
		    cuser.userid, client_code, read_count);
        }
    }
#endif // LOG_CRAWLER

    {
	int posttime=atoi(fhdr->filename+2);
	if(posttime>now-12*3600)
	    STATINC(STAT_READPOST_12HR);
	else if(posttime>now-1*DAY_SECONDS)
	    STATINC(STAT_READPOST_1DAY);
	else if(posttime>now-3*DAY_SECONDS)
	    STATINC(STAT_READPOST_3DAY);
	else if(posttime>now-7*DAY_SECONDS)
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
	case RET_DOREPLY:
	case RET_DOREPLYALL:
	    do_reply(fhdr);
            return FULLUPDATE;
	case RET_DORECOMMEND:
            recommend(ent, fhdr, direct);
	    return FULLUPDATE;
	case RET_DOQUERYINFO:
	    view_postinfo(ent, fhdr, direct, b_lines-3);
	    return FULLUPDATE;
    }
    if(more_result)
	return more_result;
    return FULLUPDATE;
}

void
editLimits(unsigned char *pregtime, unsigned char *plogins,
	unsigned char *pposts, unsigned char *pbadpost)
{
    char genbuf[STRLEN];
    int  temp;

    // load var
    unsigned char 
	regtime = *pregtime, 
	logins  = *plogins, 
	posts   = *pposts, 
	badpost = *pbadpost;

    // query UI
    sprintf(genbuf, "%u", regtime);
    do {
	getdata_buf(b_lines - 1, 0, 
		"���U�ɶ����� (�H'��'�����A0~255)�G", genbuf, 4, NUMECHO);
	temp = atoi(genbuf);
    } while (temp < 0 || temp > 255);
    regtime = (unsigned char)temp;

    sprintf(genbuf, "%u", logins*10);
    do {
	move(b_lines-1, 0); clrtoeol();	// because previous prompt has same BIG5 prefix here
	getdata_buf(b_lines - 1, 0, 
		STR_LOGINDAYS "�U�� (0~2550,�H10�����,�Ӧ�Ʀr�N�۰ʱ˥h)�G", genbuf, 5, NUMECHO);
	temp = atoi(genbuf);
    } while (temp < 0 || temp > 2550);
    logins = (unsigned char)(temp / 10);

    sprintf(genbuf, "%u", posts*10);
    do {
	getdata_buf(b_lines - 1, 0, 
		"�峹�g�ƤU�� (0~2550,�H10�����,�Ӧ�Ʀr�N�۰ʱ˥h)�G", genbuf, 5, NUMECHO);
	temp = atoi(genbuf);
    } while (temp < 0 || temp > 2550);
    posts = (unsigned char)(temp / 10);

    sprintf(genbuf, "%u", 255 - badpost);
    do {
	getdata_buf(b_lines - 1, 0, 
		"�H��g�ƤW�� (0~255)�G", genbuf, 5, NUMECHO);
	temp = atoi(genbuf);
    } while (temp < 0 || temp > 255);
    badpost = (unsigned char)(255 - temp);

    // save var
    *pregtime = regtime;
    *plogins  = logins;
    *pposts   = posts;
    *pbadpost = badpost;
}

int
do_limitedit(int ent, fileheader_t * fhdr, const char *direct)
{
    char buf[STRLEN];
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
    buf[0] = vans(buf);

    if ((HasUserPerm(PERM_SYSOP) || (HasUserPerm(PERM_SYSSUPERSUBOP) && GROUPOP())) && buf[0] == 'a') {

	editLimits(
		&bp->post_limit_regtime,
		&bp->post_limit_logins,
		&bp->post_limit_posts,
		&bp->post_limit_badpost);

	assert(0<=currbid-1 && currbid-1<MAX_BOARD);
	substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
	log_usies("SetBoard", bp->brdname);
	vmsg("�ק粒���I");
	return FULLUPDATE;
    }
    else if (buf[0] == 'b') {

	editLimits(
		&bp->vote_limit_regtime,
		&bp->vote_limit_logins,
		&bp->vote_limit_posts,
		&bp->vote_limit_badpost);

	assert(0<=currbid-1 && currbid-1<MAX_BOARD);
	substitute_record(fn_board, bp, sizeof(boardheader_t), currbid);
	log_usies("SetBoard", bp->brdname);
	vmsg("�ק粒���I");
	return FULLUPDATE;
    }
    else if ((fhdr->filemode & FILE_VOTE) && buf[0] == 'c') {

	editLimits(
		&fhdr->multi.vote_limits.regtime,
		&fhdr->multi.vote_limits.logins,
		&fhdr->multi.vote_limits.posts,
		&fhdr->multi.vote_limits.badpost);

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
		  currbid, // getbnum(currboard)?
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
	veditfile(fn_ticket_end) < 0)
	return FULLUPDATE;

    clear();
    showtitle("�|���L", BBSNAME);
    setbfile(tmp, currboard, FN_TICKET_ITEMS ".tmp");

    //sprintf(genbuf, "%s/" FN_TICKET_ITEMS, direct);

    if (!(fp = fopen(tmp, "w")))
	return FULLUPDATE;
    do {
	getdata(2, 0, "��J�m������ (����:10-10000):", yn, 6, NUMECHO);
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

    snprintf(genbuf, sizeof(genbuf), TN_ANNOUNCE " %s �O �}�l���!", currboard);
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
    char            genbuf[200] = "";
    fileheader_t    tmpfhdr = *fhdr;
    int             dirty = 0;
    int allow = 0;

    // should we allow edit-title here?
    if (currstat == RMAIL)
	allow = 0;
    else if (HasUserPerm(PERM_SYSOP))
	allow = 2;
    else if (currmode & MODE_BOARD ||
	     strcmp(cuser.userid, fhdr->owner) == 0)
	allow = 1;

    if (!allow)
	return DONOTHING;

    if (fhdr && fhdr->title[0])
	strlcpy(genbuf, fhdr->title, TTLEN+1);

    if (getdata_buf(b_lines - 1, 0, "���D�G", genbuf, TTLEN, DOECHO)) {
	// check TN_ANNOUNCE again for non-BMs...
	tn_safe_strip(genbuf);
	strlcpy(tmpfhdr.title, genbuf, sizeof(tmpfhdr.title));
	dirty++;
    }

    if (allow >= 2) 
    {
	if (getdata(b_lines - 1, 0, "�@�̡G", genbuf, IDLEN + 2, DOECHO)) {
	    strlcpy(tmpfhdr.owner, genbuf, sizeof(tmpfhdr.owner));
	    dirty++;
	}
	if (getdata(b_lines - 1, 0, "����G", genbuf, 6, DOECHO)) {
	    snprintf(tmpfhdr.date, sizeof(tmpfhdr.date), "%.5s", genbuf);
	    dirty++;
	}
    }

    if (dirty)
    {
	getdata(b_lines - 1, 0, "�T�w(Y/N)?[n] ", genbuf, 3, DOECHO);
	if ((genbuf[0] == 'y' || genbuf[0] == 'Y') && dirty) {
	    // TODO verify if record is still valid
	    fileheader_t curr;
	    memset(&curr, 0, sizeof(curr));
	    if (get_record(direct, &curr, sizeof(curr), ent) < 0 ||
		strcmp(curr.filename, fhdr->filename) != 0)
	    {
		// modified...
		vmsg("��p�A�t�Φ��L���A�еy��A�աC");
		return FULLUPDATE;
	    }
	    *fhdr = tmpfhdr;
	    substitute_ref_record(direct, fhdr, ent);
	}
    }
    return FULLUPDATE;
}

static int
solve_post(int ent, fileheader_t * fhdr, const char *direct)
{
    if ((currmode & MODE_BOARD)) {
	fhdr->filemode ^= FILE_SOLVED;
        substitute_ref_record(direct, fhdr, ent);
	check_locked(fhdr);
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
#if defined(ASSESS) && defined(EXP_BAD_COMMENT)
    // supporting bad_comment
#if 0
    // XXX ����i��|�@���]�X�ӡA�ҥH...
    if (now - atoi(fhdr->filename + 2) > 2 * 7 * 24 * 60 * 60)
    {
	move(b_lines-2, 0); clrtoeol();
	outs("�W�L��g�A�T��H����C");
    } else 
#endif
    {
	getdata(b_lines - 1, 0, "�аݱz�n (1) �����k�s (2) �H���� [1/2]? ", yn, 3, LCECHO);
	if (yn[0] == '2')
	{
	    char fn[PATHLEN];
	    setbfile(fn, currboard, fhdr->filename);
	    bad_comment(fn);
	    return FULLUPDATE;
	} else if (yn[0] != '1')
	    return FULLUPDATE;
    }
#endif
    getdata(b_lines - 1, 0, "�T�w�n�����k�s[y/N]? ", yn, 3, LCECHO);
    if (yn[0] != 'y')
	return FULLUPDATE;
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
	vmsg("���˥���");
	return -1;
    }

    // XXX do lock some day!

    /* This is a solution to avoid most racing (still some), but cost four
     * system calls.                                                        */

    if(type == RECTYPE_GOOD && fhdr->recommend < MAX_RECOMMENDS )
          update = 1;
    else if(type == RECTYPE_BAD && fhdr->recommend > -MAX_RECOMMENDS)
          update = -1;
    fhdr->recommend += update;

    // since we want to do 'modification'...
    fhdr->modified = dasht(path);

    if (fhdr->modified > 0)
    {
	if (modify_dir_lite(direct, ent, fhdr->filename,
		fhdr->modified, NULL, update) < 0)
	    return -1;
	// mark my self as "read this file".
	brc_addlist(fhdr->filename, fhdr->modified);
    }
    
    return 0;
}

int
recommend(int ent, fileheader_t * fhdr, const char *direct)
{
    char            buf[PATHLEN], msg[STRLEN];
    const char	    *myid = cuser.userid;
    char	    aligncmt = 0;
    char	    mynick[IDLEN+1];
#ifndef OLDRECOMMEND
    static const char *ctype[RECTYPE_SIZE] = {
		       "��", "�N", "��", 
		   };
    static const char *ctype_attr[RECTYPE_SIZE] = {
		       ANSI_COLOR(1;33),
		       ANSI_COLOR(1;31),
		       ANSI_COLOR(1;37),
		   }, *ctype_attr2[RECTYPE_SIZE] = {
		       ANSI_COLOR(1;37),
		       ANSI_COLOR(1;31),
		       ANSI_COLOR(1;31),
		   }, *ctype_long[RECTYPE_SIZE] = {
		       "�ȱo����",
		       "�����N�n",
		       "�u�[������",
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

    if (!fhdr || !fhdr->filename[0])
	return DONOTHING;

    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
    bp = getbcache(currbid);

    if (bp->brdattr & BRD_NORECOMMEND || fhdr->filename[0] == 'L' || 
        ((fhdr->filemode & FILE_MARKED) && (fhdr->filemode & FILE_SOLVED))) {
	vmsg("��p, �T�����");
	return FULLUPDATE;
    }
    if (   !CheckPostPerm() || isGuest)
    {
	vmsg("�z�v������, �L�k����!"); //  "(�i���j�g I �d�ݭ���)"
	return FULLUPDATE;
    }

    // TODO ���ӥi�H�Ҽ{�@�����ﶵ�i�]�w�C
#ifdef BN_ONLY_OP_CAN_ADD_COMMENT
    if (  strcmp(bp->brdname, BN_ONLY_OP_CAN_ADD_COMMENT) == 0 &&
	!((currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP|PERM_SYSSUPERSUBOP|PERM_SYSSUBOP)) )
    {
	vmsg("���O���孭�w�޲z�H���ϥΡC");
	return FULLUPDATE;
    }
#endif

#ifdef SAFE_ARTICLE_DELETE
    if (fhdr->filename[0] == '.' || fhdr->owner[0] == '-') {
	vmsg("����w�R��");
	return FULLUPDATE;
    }
#endif

    if ((bp->brdattr & BRD_VOTEBOARD) || (fhdr->filemode & FILE_VOTE))
    {
	do_voteboardreply(fhdr);
	return FULLUPDATE;
    }

#ifndef DEBUG
    if (!CheckPostRestriction(currbid))
    {
	vmsg("�A������`��I (�i�� i �d�ݭ���)");
	return FULLUPDATE;
    }
#endif

    aligncmt = (bp->brdattr & BRD_ALIGNEDCMT) ? 1 : 0;
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

    type = RECTYPE_GOOD;

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
	type = RECTYPE_ARROW;
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
	type = RECTYPE_ARROW;
	move(ymsg--, 0); clrtoeol();
	outs("�ɶ��Ӫ�, �ϥ� �� �[���覡\n");
    }
#endif

#ifndef OLDRECOMMEND
    else
    {
	int i;

	move(b_lines, 0); clrtoeol();
	outs(ANSI_COLOR(1)  "�zı�o�o�g�峹 ");

	for (i = 0; i < RECTYPE_SIZE; i++)
	{
	    if (i == RECTYPE_BAD && (bp->brdattr & BRD_NOBOO))
		continue;
	    outs(ctype_attr[i]);
	    prints("%d.", i+1);
	    outs(ctype_long[i]);
	    outc(' ');
	}
	prints(ANSI_RESET "[%d]? ",
		RECTYPE_DEFAULT+1);

	type = vkey();

	if (!isascii(type) || !isdigit(type))
	{
	    type = RECTYPE_DEFAULT;
	} else {
	    type -= '1';
	    if (type < 0 || type > RECTYPE_MAX)
		type = RECTYPE_DEFAULT;
	}

	if( (bp->brdattr & BRD_NOBOO) && (type == RECTYPE_BAD))
	    type = RECTYPE_ARROW;
	assert(type >= 0 && type <= RECTYPE_MAX);

	move(b_lines, 0); clrtoeol();
    }
#endif

    // warn if article is outside post
    if (strchr(fhdr->owner, '.')  != NULL)
    {
	move(ymsg--, 0); clrtoeol();
	outs(ANSI_COLOR(1;31) 
	    "���o�g�峹�ӦۼʦW�O�Υ~����H�O�A��@�̥i��L�k�ݨ����C" 
	    ANSI_RESET "\n");
    }

    // warn if in non-standard mode
    {
	char *p = strrchr(direct, '/');
	// allow .DIR or .DIR.bottom
	if (!p || strncmp(p+1, FN_DIR, strlen(FN_DIR)) != 0)
	{
	    ymsg --;
	    move(ymsg--, 0); clrtoeol();
	    outs(ANSI_COLOR(1;33) 
	    "���z���b�j�M(���D�B�@��...)�Ψ䥦�S��C��Ҧ��A"
	    "����p�ƻP�ק�O���N�|���}�p��C" 
	    ANSI_RESET "\n"
	    "  �Y�Q���`�p�ƽХ�����h�^���`�C��Ҧ��C\n");
	}
    }

    if(type >  RECTYPE_MAX || type < 0)
	type = RECTYPE_ARROW;

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

#if defined(PLAY_ANGEL) && defined(BN_ANGELPRAY) && defined(ANGEL_ANONYMOUS_COMMENT)
    if (HasUserPerm(PERM_ANGEL) && currboard && strcmp(currboard, BN_ANGELPRAY) == 0 &&
	vans("�n�ϥΤp�ѨϼʦW����ܡH [Y/n]: ") != 'n')
    {
	// angel push
	mynick[0] = 0;
	angel_load_my_fullnick(mynick, sizeof(mynick));
	myid = mynick;
    }
#endif 

    if (aligncmt)
    {
	// left align, voted by LydiaWu and LadyNotorious
	snprintf(buf, sizeof(buf), "%-*s", IDLEN, myid);
	strlcpy(mynick, buf, sizeof(mynick));
	myid = mynick;
    }

#ifdef OLDRECOMMEND
    maxlength -= 2; /* '��' */
    maxlength -= strlen(myid);
    sprintf(buf, "%s %s:", "��" , myid);

#else // !OLDRECOMMEND
    maxlength -= strlen(myid);
    sprintf(buf, "%s%s%s %s:", 
	    ctype_attr[type], ctype[type], ANSI_RESET, myid);
#endif // !OLDRECOMMEND

    move(b_lines, 0);
    clrtoeol();

    if (!getdata(b_lines, 0, buf, msg, maxlength, DOECHO))
	return FULLUPDATE;

    // make sure to do modification
    {
	char ans[2];
	sprintf(buf+strlen(buf), 
		ANSI_REVERSE "%-*s" ANSI_RESET " �T�w[y/N]:", 
		maxlength, msg);
	move(b_lines, 0);
	clrtoeol();
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
		(cuser.numlogindays < 50 || (now - cuser.firstlogin) < DAY_SECONDS * 7)
		? 1 : 2;
	if( tolog == 1 ){
	    FILE   *fp;
	    if( (fp = fopen("log/push", "a")) != NULL ){
		fprintf(fp, "%s %d %s %s %s\n", cuser.userid, 
			(int)now, currboard, fhdr->filename, msg);
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
		    "%15s %s",
		    FROMHOST, 
		    Cdate_mdHM(&now));
	} else {
	    snprintf(tail, sizeof(tail),
		    " %s",
		    Cdate_mdHM(&now));
	}

#ifdef OLDRECOMMEND
	snprintf(buf, sizeof(buf),
	    ANSI_COLOR(1;31) "�� " ANSI_COLOR(33) "%s" 
	    ANSI_RESET ANSI_COLOR(33) ":%-*s" ANSI_RESET
	    "��%s\n",
	    myid, maxlength, msg, tail);
#else
	snprintf(buf, sizeof(buf),
	    "%s%s " ANSI_COLOR(33) "%s" ANSI_RESET ANSI_COLOR(33) 
	    ":%-*s" ANSI_RESET "%s\n",
             ctype_attr2[type], ctype[type], myid, 
	     maxlength, msg, tail);
#endif // OLDRECOMMEND
    }

    do_add_recommend(direct, fhdr,  ent, buf, type);
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
    substitute_ref_record(direct, fhdr, ent);
    check_locked(fhdr);
    return PART_REDRAW;
}

int
del_range(int ent, const fileheader_t *fhdr, const char *direct)
{
    char            num1[8], num2[8];
    int             inum1, inum2;
    boardheader_t  *bp = NULL;

    /* ���T�ر��p�|�i�o��, �H��, �ݪO, ��ذ� */

    if( direct[0] != 'h' && currbid) /* �H�󤣥� check */
    { 
	// �ܤ������O���@�جO�H��->mail_cite->��ذ�
        bp = getbcache(currbid);
	if (is_readonly_board(bp->brdname))
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
	    int ret = 0;
	    outmsg("�B�z��,�еy��...");
	    refresh();
#ifdef SAFE_ARTICLE_DELETE
	    if(bp && !(currmode & MODE_DIGEST) && bp->nuser > 30)
		ret = safe_article_delete_range(direct, inum1, inum2);
	    else
#endif
	    ret = delete_range(direct, inum1, inum2);
	    if (ret < 0)
	    {
		clear();
		vs_hdr("�R������");
		outs("\n\n�L�k�R���ɮסC�i��O�P�ɦ��䥦�H�]�b�i��R���C\n\n"
		     "�Y�����~����o�͡A�е����@�p�ɫ�A���աC\n\n"
		     "�Y��ɤ��L�k�R���A�Ш� " BN_SYSOP " �ݪO���i�C\n");
		vmsg("�L�k�R���C�i�঳�䥦�H���b�P�ɧR���C");
		return FULLUPDATE;
	    } else 
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
    char	    reason[PROPER_TITLE_LEN] = "";
    char            genbuf[100], newpath[PATHLEN];
    int             not_owned, is_anon, tusernum, del_ok = 0, as_badpost = 0;
    boardheader_t  *bp;

    assert(0<=currbid-1 && currbid-1<MAX_BOARD);
    bp = getbcache(currbid);

    if (is_readonly_board(bp->brdname))
	return DONOTHING;

    /* TODO recursive lookup */
    if (currmode & MODE_SELECT) { 
        vmsg("�Ц^��@��Ҧ��A�R���峹");
        return DONOTHING;
    }

    if ((fhdr->filemode & FILE_BOTTOM) || 
       (fhdr->filemode & FILE_MARKED) || (fhdr->filemode & FILE_DIGEST) ||
	(fhdr->owner[0] == '-'))
	return DONOTHING;

    is_anon = (fhdr->filemode & FILE_ANONYMOUS);
    if(is_anon)
	/* When the file is anonymous posted, fhdr->multi.anon_uid is author.
	 * see do_general() */
        tusernum = fhdr->multi.anon_uid;
    else
        tusernum = searchuser(fhdr->owner, NULL);

    not_owned = (tusernum == usernum ? 0: 1);
    if ((!(currmode & MODE_BOARD) && not_owned) ||
	((bp->brdattr & BRD_VOTEBOARD) && !HasUserPerm(PERM_SYSOP)) ||
	!strcmp(cuser.userid, STR_GUEST))
	return DONOTHING;

    // user and permission check
    // reason 1: BM may alter post restrictions to this board
    // reason 2: this account may be occupied by someone else.
    if (!HasUserPerm(PERM_BASIC) ||	// including guests
	!( (currmode & MODE_DIGEST) ? (currmode & MODE_BOARD) : CheckPostPerm() ) || // allow BM to delete posts in digest mode
	!CheckPostRestriction(currbid)
	)   
	return DONOTHING;

    if (fhdr->filename[0]=='L') fhdr->filename[0]='M';

#ifdef SAFE_ARTICLE_DELETE
    // query if user really wants to delete it
    if (not_owned && !is_anon && fhdr->owner[0])
    {
	// manager (bm, sysop, police)
	do {
	    getdata(1, 0, "�нT�w�R��(Y/N/R�[���z��)?[N]", genbuf, 3, LCECHO);

	    // for y/n, skip.
	    if (genbuf[0] != 'r')
		break;

	    // build reason string (based on STR_SAFEDEL_TITLE)
	    snprintf(reason, sizeof(reason), "(�w�Q%s�R��) <%s>", 
		    cuser.userid, fhdr->owner);
	    move(3, 0); clrtoeol();
	    getdata_str(2, 0, " >> �п�J�R����n��ܪ����D: �� ", 
		    reason, sizeof(reason), DOECHO, reason);

	    if (!reason[0])
	    {
		vmsg("����J�z�ѡA���R���C");
		genbuf[0] = 'n';
		break;
	    }

	    // confirm again!
	    move(4, 0); clrtoeol();
	    getdata(3, 0, "�ЦA���T�w�O�_�n�ΤW�z�z�ѧR��(Y/N)?[N]", 
		    genbuf, 3, LCECHO);

	    // since the default y/n is same to msg_del_ny, we reuse the genbuf[0] here.
	} while (0);
    } else 
#endif
    {
	getdata(1, 0, msg_del_ny, genbuf, 3, LCECHO);
    }
    if (genbuf[0] == 'y') {
	if(
#ifdef SAFE_ARTICLE_DELETE
	   ((reason[0] || bp->nuser > 30) && !(currmode & MODE_DIGEST) &&
            !safe_article_delete(ent, fhdr, direct, reason[0] ? reason : NULL)) ||
#endif
	   // XXX TODO delete_record is really really dangerous - 
	   // we should verify the header (maybe by filename) is the same.
	   // currently race condition is easily cause by 2 BMs
	   !delete_record(direct, sizeof(fileheader_t), ent)
	   ) {
            // do this immediately after .DIR change in case connection
            // was closed.
	    setbtotal(currbid);

	    del_ok = (cancelpost(fhdr, not_owned, newpath) == 0) ? 1 : 0;
            deleteCrossPost(fhdr, bp->brdname);

#ifdef ASSESS
	    // badpost assignment

	    // case one, self-owned, invalid author, or digest mode - should not give bad posts
	    if (!not_owned || tusernum <= 0 || (currmode & MODE_DIGEST) )
	    {
		// do nothing
	    } 
	    // case 2, got error in file deletion (already deleted, also skip badpost)
	    else if (!del_ok)
	    {
		move_ansi(1, 40); clrtoeol();
		outs("���ɤw�Q�O�H�R��(���L�H��]�w)");
		pressanykey();
	    }
	    // case 3, post older than one week (TODO use macro for the duration)
	    else if (now - atoi(fhdr->filename + 2) > 7 * 24 * 60 * 60)
	    {
		move_ansi(1, 40); clrtoeol();
		outs("�峹�W�L�@�g(���L�H��]�w)");
		pressanykey();
	    }
	    // case 4, can assign badpost
	    else 
	    {
		// TODO not_owned �ɤ]�n���� numpost?
		move_ansi(1, 40); clrtoeol();
                outs("�c�H�峹?(y/N) ");
                // FIXME ���O�D�|�b�o�̤��p���_���s�u�ҥH�n�p��...
                // ���n���Ƴ̦n�b�e���@���C
                vgets(genbuf, 3, VGET_LOWERCASE);

		if (genbuf[0]=='y') {
                    as_badpost = 1;
		    assign_badpost(getuserid(tusernum), fhdr, newpath, NULL);
                }
	    }
#endif // ASSESS

	    // �����e����峹�����d�M��
	    // freebn/brd_bad: should be done before, but let's make it safer.
	    // new rule: only articles with money need updating
	    // numpost (to solve deleting cross-posts).
	    // DIGEST mode ���κ�
	    // INVALIDMONEY_MODES (FILE_BID, FILE_ANONYMOUS, ...) �]�����Φ�
	    if (fhdr->multi.money < 0 || 
		IsFreeBoardName(currboard) || (currbrdattr & BRD_BAD) ||
		(currmode & MODE_DIGEST) ||
		(fhdr->filemode & INVALIDMONEY_MODES) ||
		0)
		fhdr->multi.money = 0;

	    // XXX also check MAX_POST_MONEY in case any error results in bad money...
	    if (fhdr->multi.money <= 0 || fhdr->multi.money > MAX_POST_MONEY)
	    {
		// no need to change user record
	    } 
	    else if (not_owned)
	    {
		// not owner case
		if (tusernum)
		{
		    userec_t xuser;
		    assert(tusernum != usernum);
		    // TODO we're doing redundant i/o here... merge and refine someday
		    if (passwd_sync_query(tusernum, &xuser) == 0) {
			if (xuser.numposts > 0)
			    xuser.numposts--;
			passwd_sync_update(tusernum, &xuser);
		    }

                    pay_as_uid(tusernum, fhdr->multi.money,  
                            "%s �ݪO �峹�u%s�v�Q%s�A�����Z�S%s %s",  
                            currboard,  fhdr->title,  
                            as_badpost ? "�H�h" : "�R��", 
                            reason[0] ? "�C��]:" : "", reason); 
                    sendalert_uid(tusernum, ALERT_PWD_PERM);
#ifdef USE_COOLDOWN
		    if (bp->brdattr & BRD_COOLDOWN)
			add_cooldowntime(tusernum, 15);
#endif
		}
	    } 
	    else
	    {
		// owner case
		pwcuDecNumPost();
                pay(fhdr->multi.money, "%s �ݪO �峹�ۧR�M��O: %s",  
                    currboard, fhdr->title); 
		sendalert(cuser.userid, ALERT_PWD_PERM);
		vmsgf("�z���峹� %d �g�A��I�M��O %d ��", 
			cuser.numposts, fhdr->multi.money);
	    }

	    return DIRCHANGED;
	} // delete_record
    } // genbuf[0] == 'y'
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
    char fn1[PATHLEN];
    char genbuf[PATHLEN] = "";
    int i;
    boardheader_t *bp = NULL;
    
    if (currstat == RMAIL)
	return DONOTHING;

    if (!(currmode & MODE_BOARD) && !HasUserPerm(PERM_SYSOP | PERM_POLICE))
	return DONOTHING;

    bp = getbcache(currbid);
    assert(bp);

    if (fhdr->filename[0]=='M') {
	if (!HasUserPerm(PERM_SYSOP | PERM_POLICE))
	    return DONOTHING;

	getdata(b_lines - 1, 0, "�п�J��w�z�ѡG", genbuf, 50, DOECHO);

	if (vans("�n�N�峹��w��(y/N)?") != 'y')
	    return FULLUPDATE;
        setbfile(fn1, currboard, fhdr->filename);
        fhdr->filename[0] = 'L';
	syncnow();
	bp->SRexpire = now;
    }
    else if (fhdr->filename[0]=='L') {
	if (vans("�n�N�峹��w�Ѱ���(y/N)?") != 'y')
	    return FULLUPDATE;
        fhdr->filename[0] = 'M';
        setbfile(fn1, currboard, fhdr->filename);
	syncnow();
	bp->SRexpire = now;
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
view_postinfo(int ent, const fileheader_t * fhdr, const char *direct, int crs_ln)
{
    aidu_t aidu = 0;
    int l = crs_ln + 3;  /* line of cursor */
    int area_l = l + 1;
#ifdef QUERY_ARTICLE_URL
    const int area_lines = 5;
#else
    const int area_lines = 4;
#endif

    if(!fhdr || fhdr->filename[0] == '.' || !fhdr->filename[0])
      return DONOTHING;

    if((area_l + area_lines > b_lines) ||  /* �U���񤣤U */
       (l >= (b_lines  * 2 / 3)))  /* ���W�L�e�� 2/3 */
      area_l -= (area_lines + 1);

    grayout(0, MIN(l - 1, area_l)-1, GRAYOUT_DARK);
    grayout(MAX(l + 1 + 1, area_l + area_lines), b_lines-1, GRAYOUT_DARK);
    grayout(l, l, GRAYOUT_BOLD);

    /* �M���峹���e�@��Ϋ�@�� */
    if(area_l > l)
      move(l - 1, 0);
    else
      move(l + 1, 0);
    clrtoeol();

    move(area_l-(area_l < l), 0);
    clrtoln(area_l -(area_l < l) + area_lines+1);
    outc(' '); outs(ANSI_CLRTOEND);
    move(area_l -(area_l < l) + area_lines, 0); 
    outc(' '); outs(ANSI_CLRTOEND);
    move(area_l, 0);

    // TODO XXX support wide terminal someday.

    prints("�z�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�{\n");

    aidu = fn2aidu((char *)fhdr->filename);
    if(aidu > 0)
    {
      char aidc[10];
      int y, x;
      
      aidu2aidc(aidc, aidu);
      prints("�x " AID_DISPLAYNAME ": " 
	  ANSI_COLOR(1) "#%s" ANSI_RESET " (%s) [%s] ", 
	  aidc, currboard && currboard[0] ? currboard : "����",
	  AID_HOSTNAME);
      getyx_ansi(&y, &x);
      x = 75 - x;
      if (x > 1)
	  prints("%.*s ", x, fhdr->title);
      outs("\n");
    }
    else
    {
      prints("�x\n");
    }

#ifdef QUERY_ARTICLE_URL
    {
	boardheader_t *bp = NULL;

	// XXX currbid should match currboard, right? can we use it?
	if (currboard && currboard[0])
	{
	    int bnum = getbnum(currboard);
	    if (bnum > 0)
	    {
		assert(0<=bnum-1 && bnum-1<MAX_BOARD);
		bp = getbcache(bnum);
	    }
	}

	if (!bp)
	{
	    prints("�x\n");
	} 
	else if ((bp->brdattr & (BRD_HIDE | BRD_OVER18)) ||
		 (bp->level && !(bp->brdattr & BRD_POSTMASK)) // !POSTMASK = restricted read
		)
	{
	    // over18 boards do not provide URL.
	    prints("�x ���ݪO�ثe������" URL_DISPLAYNAME " \n");
	}
	else
	{
	    prints("�x " URL_DISPLAYNAME ": " 
		    ANSI_COLOR(1) URL_PREFIX "/%s/%s.html" ANSI_RESET "\n",
		    currboard, fhdr->filename);
	}
    }
#endif

    if(fhdr->filemode & FILE_ANONYMOUS)
	/* When the file is anonymous posted, fhdr->multi.anon_uid is author.
	 * see do_general() */
	prints("�x �ΦW�޲z�s��: %u (�P�@�H���X�|�@��)",
	   (unsigned int)fhdr->multi.anon_uid + (unsigned int)currutmp->pid);
    else {
	int m = query_file_money(fhdr);

	if(m < 0)
	    prints("�x �S��峹�A�L����O��");
	else
	    prints("�x �o�@�g�峹�� %d ��", m);

    }
    prints("\n");
    prints("�|�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�}\n");

    /* �L��ܮت��k��� */
    {
      int i;

      for(i = 1; i < area_lines - 1; i ++)
      {
        move_ansi(area_l + i , 76);
        prints("�x");
      }
    }
    {
        int r = pressanykey();
        /* TODO: �h�[�@�� LISTMODE_AID�H */
	/* QQ: enable money listing mode */
	if (r == 'Q')
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
#ifdef TARQUEUE_SENDURL
    move (3,0); outs("�п�J�q���H�c (�w�]���� BBS �b���H�c): ");
    if (!getdata_str(4, 2, "",
		email, sizeof(email), DOECHO, cuser.userid))
	return FULLUPDATE;
    if (strstr(email, "@") == NULL)
    {
	strcat(email, ".bbs@");
	strcat(email, MYHOSTNAME);
    }
    move(4,0); clrtoeol();
    outs(email);
#else
    if (!getdata(4, 0, "�п�J�ت��H�c�G", email, sizeof(email), DOECHO))
	return FULLUPDATE;

    /* check email -.-"" */
    if (strstr(email, "@") == NULL || strstr(email, ".bbs@") != NULL) {
	move(6, 0);
	outs("�z���w���H�c�����T! ");
	pressanykey();
	return FULLUPDATE;
    }
#endif
    getdata(6, 0, "�n�ƥ��ݪO���e��(Y/N)?[Y]", ans, sizeof(ans), LCECHO);
    bakboard = (ans[0] == 'n') ? 0 : 1;
    getdata(7, 0, "�n�ƥ���ذϤ��e��(Y/N)?[N]", ans, sizeof(ans), LCECHO);
    bakman = (ans[0] == 'y') ? 1 : 0;
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
/* �ݪO�i�O�e���B��K�B��ذ�                              */
/* ----------------------------------------------------- */
int
b_note_edit_bname(int bid)
{
    char            buf[PATHLEN];
    int             aborted;
    boardheader_t  *fh = getbcache(bid);
    assert(0<=bid-1 && bid-1<MAX_BOARD);
    setbfile(buf, fh->brdname, fn_notes);
    aborted = veditfile(buf);
    if (aborted == -1) {
	clear();
	outs(msg_cancel);
	pressanykey();
    } else {
       // alert user our new b_note policy.
       char msg[STRLEN];
       clear();
       vs_hdr("�i�O�e����ܳ]�w");
       outs("\n"
       "\t�ШM�w�O�_�n�b�ϥΪ̭����i�J�ݪO����ܭ��x�s���i�O�e���C\n\n"
       "\t�Ъ`�N�Y�ϥΪ̳s�򭫽ƶi�X�P�@�ӬݪO�ɡA�i�O�e���u�|��ܤ@���C\n"
       "\t�����t�γ]�w�A�ëD�]�w���~�C\n\n"
       "\t(�ϥΪ��H�ɥi�� b �θg�Ѷi�X���P�ݪO�ӭ��s��ܶi�O�e��)\n");

       // �]�w������ĪG���ܦ��N���|�ʤF,�ҥH�ޱ�
       snprintf(msg, sizeof(msg), 
	       "�n�b�����i�J�ݪO����ܶi�O�e���ܡH (y/n) [%c]: ",
	       fh->bupdate ? 'Y' : 'N');
       getdata(10, 0, msg, buf, 3, LCECHO);

       switch(buf[0])
       {
           case 'y':
               // assign a large, max date.
               fh->bupdate = INT_MAX -1;
               break;
           case 'n':
               fh->bupdate = 0;
               break;
           default:
	       // do nothing
               break;
       }
       // expire BM's lastbid to help him verify settings.
       bnote_lastbid = -1;

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
	outs("���ݪO�|�L�i�O�e���C");
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
    char buf[PATHLEN];
    if ((currmode & MODE_DIGEST) || !(currmode & MODE_BOARD)
        || fhdr->filename[0]=='L')
        return DONOTHING;
    setbottomtotal(currbid);  // <- Ptt : will be remove when stable
    num = getbottomtotal(currbid);
    if (!(fhdr->filemode & FILE_BOTTOM))
    {
	move(b_lines-1, 0); clrtoeol();
	outs(ANSI_COLOR(1;33) "�����z�m���P���ثe�����s���A�R�����]�|�ɭP�m�������C" ANSI_RESET);
    }
    if( vans(fhdr->filemode & FILE_BOTTOM ?
	       "�����m�����i?(y/N)":
	       "�[�J�m�����i?(y/N)") != 'y' )
	return FULLUPDATE;
    if(!(fhdr->filemode & FILE_BOTTOM) ){
          snprintf(buf, sizeof(buf), "%s.bottom", direct);
          if(num >= 5){
              vmsg("���o�W�L 5 �g���n���i �к�²!");
              return FULLUPDATE;
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

    if(vans(fhdr->filemode & FILE_DIGEST ? 
              "�����ݪO��K?(Y/n)" : "���J�ݪO��K?(Y/n)") == 'n')
	return READ_REDRAW;

    if (fhdr->filemode & FILE_DIGEST) {
	fhdr->filemode = (fhdr->filemode & ~FILE_DIGEST);
	if (!strcmp(currboard, BN_NOTE) || 
#ifdef BN_ARTDSN	    
	    !strcmp(currboard, BN_ARTDSN) || 
#endif
	    !strcmp(currboard, BN_BUGREPORT) ||
	    !strcmp(currboard, BN_LAW)
	    ) 
	{
            int unum = searchuser(fhdr->owner, NULL); 
            if (unum > 0) { 
                pay_as_uid(unum, 1000, "���� %s �ݪO��K", currboard); 
            } 
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

#ifdef BN_DIGEST
	assert(0<=currbid-1 && currbid-1<MAX_BOARD);
	if(!(getbcache(currbid)->brdattr & BRD_HIDE)) { 
          getdata(1, 0, "�n��ȱo�X���������K?(N/y)", genbuf2, 3, LCECHO);
          if(genbuf2[0] == 'y')
	      do_crosspost(BN_DIGEST, &digest, genbuf, 1);
        }
#endif

	fhdr->filemode = (fhdr->filemode & ~FILE_MARKED) | FILE_DIGEST;
	if (!strcmp(currboard, BN_NOTE) || 
#ifdef BN_ARTDSN	    
	    !strcmp(currboard, BN_ARTDSN) || 
#endif
	    !strcmp(currboard, BN_BUGREPORT) ||
	    !strcmp(currboard, BN_LAW)
	    ) 
	{
            int unum = searchuser(fhdr->owner, NULL); 
            if (unum > 0) { 
                pay_as_uid(unum, -1000, "�Q��J %s �ݪO��K", currboard); 
            } 
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
	    vmsgf("�藍�_�A�z���峹�α��嶡�j�Ӫ��o�I (���� %d �� %d ��)", 
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
	if (vans("�ثe���Ť�, �n�}���(y/N)?") != 'y')
	    return FULLUPDATE;
	bp->brdattr &= ~BRD_COOLDOWN;
	outs("�j�a���i�H post �峹�F�C\n");
    } else {
	getdata(b_lines - 1, 0, "�п�J�N�R�z�ѡG", genbuf, 50, DOECHO);
	if (vans("�n���� post �W�v, ���Ŷ�(y/N)?") != 'y')
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

static int
b_moved_to_config()
{
    if ((currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP))
    {
	vmsg("�o�ӥ\\��w���J�ݪO�]�w (i) �h�F�I");
	return FULLUPDATE;
    }
    return DONOTHING;
}

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
    { 0, NULL }, // Ctrl('N')
    { 0, NULL }, // Ctrl('O') // BETTER NOT USE ^O - UNIX not work
    { 0, do_post }, // Ctrl('P')
    { 0, NULL }, // Ctrl('Q')
    { 0, NULL }, // Ctrl('R')
    { 0, NULL }, // Ctrl('S')
    { 0, NULL }, // Ctrl('T')
    { 0, NULL }, // Ctrl('U')
    { 0, do_post_vote }, // Ctrl('V')
    { 0, whereami }, // Ctrl('W')
    { 1, cross_post }, // Ctrl('X')
    { 0, NULL }, // Ctrl('Y')
    { 0, NULL }, // Ctrl('Z') 26 // �{�b�� ZA �ΡC
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
    { 0, b_config }, // 'B'
    { 1, do_limitedit }, // 'C'
    { 1, del_range }, // 'D'
    { 1, edit_post }, // 'E'
    { 0, NULL }, // 'F'
    { 0, NULL }, // 'G'
    { 0, b_moved_to_config }, // 'H'
    { 0, b_config }, // 'I'
#ifdef USE_COOLDOWN
    { 0, change_cooldown }, // 'J'
#else
    { 0, NULL }, // 'J'
#endif
    { 0, NULL }, // 'K'
    { 1, solve_post }, // 'L'
    { 0, NULL }, // 'M'
    { 0, NULL }, // 'N'
    { 0, NULL }, // 'O'
    { 0, NULL }, // 'P'
    { 1, view_postinfo }, // 'Q'
    { 0, b_results }, // 'R'
    { 0, NULL }, // 'S'
    { 1, edit_title }, // 'T'
    { 0, NULL }, // 'U'
    { 0, b_vote }, // 'V'
    { 0, b_notes_edit }, // 'W'
    { 1, recommend }, // 'X'
    { 1, recommend_cancel }, // 'Y'
    { 0, NULL }, // 'Z' 90
    { 0, NULL }, { 0, NULL }, { 0, NULL }, { 0, NULL }, 
    { 1, push_bottom }, // '_' 95
    { 0, NULL },
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
    { 0, b_config }, // 'i'
    { 0, NULL }, // 'j'
    { 0, NULL }, // 'k'
    { 0, NULL }, // 'l'
    { 1, mark_post }, // 'm'
    { 0, NULL }, // 'n'
    { 0, NULL }, // 'o'
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
    { 0, NULL }, // 'v'
    { 1, b_call_in }, // 'w'
    { 1, old_cross_post }, // 'x'
    { 1, reply_post }, // 'y'
    { 0, b_man }, // 'z' 122
};

int
Read(void)
{
    int             mode0 = currutmp->mode;
    int             stat0 = currstat, tmpbid = currutmp->brc_id;
    char            buf[PATHLEN];

    const char *bname = currboard[0] ? currboard : DEFAULT_BOARD;
    if (enter_board(bname) < 0)
	return 0;

    setutmpmode(READING);

    if (currbid != bnote_lastbid &&
	board_note_time && *board_note_time) {
	int mr;

	setbfile(buf, currboard, fn_notes);
	mr = more(buf, NA);
	if(mr == -1)
            *board_note_time=0;
	else if (mr != READ_NEXT)
	    pressanykey();
    }
    bnote_lastbid = currbid;
    i_read(READING, currdirect, readtitle, readdoent, read_comms,
	   currbid);
    currmode &= ~MODE_POSTCHECKED;
    brc_update();
    setutmpbid(tmpbid);
    currutmp->mode = mode0;
    currstat = stat0;
    return 0;
}

int
ReadSelect(void)
{
    int             mode0 = currutmp->mode;
    int             stat0 = currstat;
    int		    changed = 0;

    currstat = SELECT;
    if (do_select() == NEWDIRECT)
    {
	Read();
	changed = 1;
    }
    // need to set utmpbid here...
    // because Read() just restores settings.
    // and 's' directly calls here.
    // so if we don't reset then utmpbid will be out-of-sync.
    // fix someday...
    setutmpbid(0);
    currutmp->mode = mode0;
    currstat = stat0;
    return changed;
}

int
Select(void)
{
    return do_select();
}

