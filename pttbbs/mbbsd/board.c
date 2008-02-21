/* $Id$ */
#include "bbs.h"

/* personal board state
 * �۹��ݪO�� attr (BRD_* in ../include/pttstruct.h),
 * �o�ǬO�Φb user interface �� flag */
#define NBRD_FAV    	 1
#define NBRD_BOARD	 2
#define NBRD_LINE   	 4
#define NBRD_FOLDER	 8
#define NBRD_TAG	16
#define NBRD_UNREAD     32
#define NBRD_SYMBOLIC   64

#define TITLE_MATCH(bptr, key)	((key)[0] && !strcasestr((bptr)->title, (key)))


#define B_TOTAL(bptr)        (SHM->total[(bptr)->bid - 1])
#define B_LASTPOSTTIME(bptr) (SHM->lastposttime[(bptr)->bid - 1])
#define B_BH(bptr)           (&bcache[(bptr)->bid - 1])

#define HasFavEditPerm() HasUserPerm(PERM_BASIC)

typedef struct {
    int             bid;
    unsigned char   myattr;
} __attribute__ ((packed)) boardstat_t;

/**
 * class_bid ���N�q
 *   class_bid < 0   �����ݪO
 *   class_bid = 0   �ڪ��̷R
 *   class_bid = 1   �����ݪO
 *   class_bid > 1   ��L�ؿ�
 */
#define IN_HOTBOARD()	(class_bid < 0)
#define IN_FAVORITE()	(class_bid == 0)
#define IN_CLASSROOT()	(class_bid == 1)
#define IN_SUBCLASS()	(class_bid > 1)
#define IN_CLASS()	(class_bid > 0)
static int      class_bid = 0;

static int nbrdsize = 0;
static boardstat_t *nbrd = NULL;
static char	choose_board_depth = 0;
static int      brdnum;
static char     yank_flag = 1;

static time4_t   last_save_fav_and_brc;

/* These are all the states yank_flag may be. */
// XXX IS_LISTING_FAV() does not mean we are in favorite.
// That is controlled by IN_FAVORITE().
#define LIST_FAV()         (yank_flag = 0)
#define LIST_BRD()         (yank_flag = 1)
#define IS_LISTING_FAV()   (yank_flag == 0)
#define IS_LISTING_BRD()   (yank_flag == 1)

inline int getbid(const boardheader_t *fh)
{
    return (fh - bcache);
}
inline boardheader_t *getparent(const boardheader_t *fh)
{
    if(fh->parent>0)
	return getbcache(fh->parent);
    else
	return NULL;
}

/**
 * @param[in]	boardname	board name, case insensitive
 * @return	0	if success
 * 		-1	if not found
 * 		-2	permission denied
 * 		-3	error
 * @note enter board:
 * 	1. setup brc (currbid, currboard, currbrdattr)
 * 	2. set currbid, currBM, currmode, currdirect
 * 	3. utmp brc_id
 */
int enter_board(const char *boardname)
{
    boardheader_t  *bh;
    int bid;
    char bname[IDLEN+1];
    char bpath[60];
    struct stat     st;

    /* checking ... */
    if (boardname[0] == '\0' || !(bid = getbnum(boardname)))
	return -1;
    assert(0<=bid-1 && bid-1<MAX_BOARD);
    bh = getbcache(bid);
    if (!HasBoardPerm(bh))
	return -2;

    strlcpy(bname, bh->brdname, sizeof(bname));
    if (bname[0] == '\0')
	return -3;

    setbpath(bpath, bname);
    if (stat(bpath, &st) == -1) {
	return -3;
    }

    /* really enter board */
    brc_update();
    brc_initial_board(bname);
    setutmpbid(currbid);

    set_board();
    setbdir(currdirect, currboard);
    curredit &= ~EDIT_MAIL;

    return 0;
}


void imovefav(int old)
{
    char buf[5];
    int new;
    
    getdata(b_lines - 1, 0, "�п�J�s����:", buf, sizeof(buf), DOECHO);
    new = atoi(buf) - 1;
    if (new < 0 || brdnum <= new){
	vmsg("��J�d�򦳻~!");
	return;
    }
    move_in_current_folder(old, new);
}

void
init_brdbuf(void)
{
    if (brc_initialize())
	return;
}

void
save_brdbuf(void)
{
    fav_save();
    fav_free();
}

int
HasBoardPerm(boardheader_t *bptr)
{
    register int    level, brdattr;

    level = bptr->level;
    brdattr = bptr->brdattr;

    if (HasUserPerm(PERM_SYSOP))
	return 1;

    /* �Q�K�T�ݪO */
    if( (brdattr & BRD_OVER18) && !over18 )
	return 0;

    /* �O�D */
    if( is_BM_cache(bptr - bcache + 1) ) /* XXXbid */
	return 1;

    /* ���K�ݪO�G�ֹﭺ�u�O�D���n�ͦW�� */
    if (brdattr & BRD_HIDE) {	/* ���� */
	if (!is_hidden_board_friend((int)(bptr - bcache) + 1, currutmp->uid)) {
	    if (brdattr & BRD_POSTMASK)
		return 0;
	    else
		return 2;
	} else
	    return 1;
    }

    /* ����\Ū�v�� */
    if (level && !(brdattr & BRD_POSTMASK) && !HasUserPerm(level))
	return 0;

    return 1;
}

// board configuration utilities

static int
b_post_note(void)
{
    char            buf[200], yn[3];

    // if(!(currmode & MODE_BOARD)) return DONOTHING;
    stand_title("�ۭq�`�N�ƶ�");

    setbfile(buf, currboard, FN_POST_NOTE);
    move(b_lines-2, 0); clrtobot();

    if (more(buf, NA) == -1)
	more("etc/" FN_POST_NOTE, NA);
    getdata(b_lines - 2, 0, "�O�_�n�Φۭq�o��`�N�ƶ�? [y/N]",
	    yn, sizeof(yn), LCECHO);
    if (yn[0] == 'y')
	vedit(buf, NA, NULL);
    else
	unlink(buf);

    setbfile(buf, currboard, FN_POST_BID);
    if (more(buf, NA) == -1)
	more("etc/" FN_POST_BID, NA);
    getdata(b_lines - 2, 0, "�O�_�n�Φۭq�v�Ф峹�`�N�ƶ�? [y/N]",
	    yn, sizeof(yn), LCECHO);
    if (yn[0] == 'y')
	vedit(buf, NA, NULL);
    else
	unlink(buf);

    return FULLUPDATE;
}

static int
b_posttype()
{
   boardheader_t  *bp;
   int i, aborted;
   char filepath[PATHLEN], genbuf[60], title[5], posttype_f, posttype[33]="";

   // if(!(currmode & MODE_BOARD)) return DONOTHING;
   
   assert(0<=currbid-1 && currbid-1<MAX_BOARD);
   bp = getbcache(currbid);
   stand_title("�]�w�峹���O");

   move(2,0);
   clrtobot();
   posttype_f = bp->posttype_f;
   for( i = 0 ; i < 8 ; ++i ){
       move(2+i,0);
       outs("�峹����:       ");
       strlcpy(genbuf, bp->posttype + i * 4, 5);
       sprintf(title, "%d.", i + 1);
       if( !getdata_buf(2+i, 11, title, genbuf, 5, DOECHO) )
	   break;
       sprintf(posttype + i * 4, "%-4.4s", genbuf); 
       if( posttype_f & (1<<i) ){
	   if( getdata(2+i, 20, "�]�w�d���榡�H(Y/n)", genbuf, 3, LCECHO) &&
	       genbuf[0]=='n' ){
	       posttype_f &= ~(1<<i);
	       continue;
	   }
       }
       else if ( !getdata(2+i, 20, "�]�w�d���榡�H(y/N)", genbuf, 3, LCECHO) ||
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

// integrated board config
int
b_config(void)
{
    boardheader_t   *bp=NULL;
    int touched = 0, finished = 0;
    bp = getbcache(currbid); 
    int i = 0, attr = 0, ipostres;
    char isBM = (currmode & MODE_BOARD) || HasUserPerm(PERM_SYSOP);

#define LNBOARDINFO (17)
#define LNPOSTRES   (12)
#define COLPOSTRES  (50)

    int ytitle = b_lines - LNBOARDINFO;

#ifdef OLDRECOMMEND
    ytitle ++;
#endif  // OLDRECOMMEND

    grayout(0, ytitle-2, GRAYOUT_DARK);

    // available hotkeys yet:
    // a b d j k p q z
    // 2 3 4 5 6 7 9
    // better not: l 0

    while(!finished) {
	move(ytitle-1, 0); clrtobot();
	// outs(MSG_SEPERATOR); // deprecated by grayout
	move(ytitle, 0);
	outs(ANSI_COLOR(7) " " ); outs(bp->brdname); outs(" �ݪO�]�w");
	i = t_columns - strlen(bp->brdname) - strlen("  �ݪO�]�w") - 2;
	for (; i>0; i--)
	    outc(' ');
	outs(ANSI_RESET);

	move(ytitle +2, 0);
	clrtobot();

	prints(" ����ԭz: %s\n", bp->title);
	prints(" �O�D�W��: %s\n", (bp->BM[0] > ' ')? bp->BM : "(�L)");

	outs(" \n"); // at least one character, for move_ansi.

	prints( " " ANSI_COLOR(1;36) "h" ANSI_RESET 
		" - ���}���A(�O�_����): %s " ANSI_RESET "\n", 
		(bp->brdattr & BRD_HIDE) ? 
		ANSI_COLOR(1)"����":"���}");

	prints( " " ANSI_COLOR(1;36) "g" ANSI_RESET 
		" - ���O�� %s �i�J�Q�j�Ʀ�]" ANSI_RESET "\n", 
		(bp->brdattr & BRD_BMCOUNT) ? 
		ANSI_COLOR(1)"�i�H" ANSI_RESET:
		"���i");

	prints( " " ANSI_COLOR(1;36) "r" ANSI_RESET 
		" - %s " ANSI_RESET "���ˤ峹\n", 
		(bp->brdattr & BRD_NORECOMMEND) ? 
		ANSI_COLOR(31)"���i":"�i�H");

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
		" - ����峹 %s " ANSI_RESET "�۰ʰO���A�B %s " 
		ANSI_RESET "�o���v��\n", 
		(bp->brdattr & BRD_CPLOG) ? 
		ANSI_COLOR(1)"�|" : "���|" ,
		(bp->brdattr & BRD_CPLOG) ? 
		ANSI_COLOR(1)"�ݭn" : "����"
		);
#endif

	prints( " " ANSI_COLOR(1;36) "L" ANSI_RESET 
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
		" �^��\n",
		(bp->brdattr & BRD_NOREPLY) ? 
		ANSI_COLOR(1)"���i�H" : "�i�H" );

	prints( " " ANSI_COLOR(1;36) "e" ANSI_RESET 
		" - �o���v��: %s" ANSI_RESET "\n", 
		(bp->brdattr & BRD_RESTRICTEDPOST) ? 
		ANSI_COLOR(1)"�u���O�ͤ~�i�o��" : "�L�S�O�]�w" );

	ipostres = b_lines - LNPOSTRES;
	move_ansi(ipostres++, COLPOSTRES-2);
	outs(ANSI_COLOR(1;32) "�o�孭��" ANSI_RESET);

#define POSTRESTRICTION(msg,utag) \
	prints(msg, attr ? ANSI_COLOR(1) : "", i, attr ? ANSI_RESET : "")

	move_ansi(ipostres++, COLPOSTRES);
	i = (int)bp->post_limit_logins * 10;
	attr = (cuser.numlogins < i) ? 1 : 0;
	if (attr) outs(ANSI_COLOR(31));
	prints("�W������ %d ���H�W", i);
	if (attr) outs(ANSI_RESET);

	move_ansi(ipostres++, COLPOSTRES);
	i = (int)bp->post_limit_posts * 10;
	attr = (cuser.numposts < i) ? 1 : 0;
	if (attr) outs(ANSI_COLOR(31));
	prints("�峹�g�� %d �g�H�W", i);
	if (attr) outs(ANSI_RESET);

	move_ansi(ipostres++, COLPOSTRES);
	i = bp->post_limit_regtime;
	attr = (cuser.firstlogin > 
		(now - (time4_t)bp->post_limit_regtime * 2592000)) ? 1 : 0;
	if (attr) outs(ANSI_COLOR(31));
	prints("���U�ɶ� %d �Ӥ�H�W",i);
	if (attr) outs(ANSI_RESET);

	move_ansi(ipostres++, COLPOSTRES);
	i = 255 - bp->post_limit_badpost;
	attr = (cuser.badpost > i) ? 1 : 0;
	if (attr) outs(ANSI_COLOR(31));
	prints("�H��g�� %d �g�H�U", i);
	if (attr) outs(ANSI_RESET);

	if (bp->brdattr & BRD_POSTMASK)
	{
	    // see haspostperm()
	    unsigned int permok = bp->level & ~PERM_POST;
	    permok = permok ? HasUserPerm(permok) : 1;
	    move_ansi(ipostres++, COLPOSTRES);
	    prints("�ϥΪ̵���: %s���w(%s�n�D)%s\n", 
		    permok ? "" : ANSI_COLOR(31),
		    permok ? "�w�F" : "���F",
		    permok ? "" : ANSI_RESET
		    );
	}

	{
	    const char *aCat = ANSI_COLOR(1;32);
	    const char *aHot = ANSI_COLOR(1;36);
	    const char *aRst = ANSI_RESET;

	    if (!isBM)
	    {
		aCat = ANSI_COLOR(1;30;40);
		aHot = "";
		aRst = "";
	    }

	    ipostres ++;
	    move_ansi(ipostres++, COLPOSTRES-2);
	    outs(aCat);
	    outs("�W��s��P�䥦:");
	    if (!isBM) outs(" (�ݪO�D�v��)");
	    outs(aRst);
	    move_ansi(ipostres++, COLPOSTRES);
	    prints("%sv%s)�i���W�� %sw%s)����W�� ", 
		    aHot, aRst, aHot, aRst);
	    move_ansi(ipostres++, COLPOSTRES);
	    prints("%sm%s)�|��벼 %so%s)�벼�W�� ",
		    aHot, aRst, aHot, aRst);
	    move_ansi(ipostres++, COLPOSTRES);
	    prints("%sc%s)�峹���O %sn%s)�o��`�N�ƶ� ",
		    aHot, aRst, aHot, aRst);
	    outs(ANSI_RESET);
	}

	move(b_lines, 0);
	if (!isBM)
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
	    case 'l':
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
		    hbflreload(currbid);
		} else {
		    bp->brdattr |= BRD_HIDE;
		    bp->brdattr |= BRD_POSTMASK;
		}
		touched = 1;
		break;

	    case 'g':
#ifndef BMCHS
		if (!HasUserPerm(PERM_SYSOP))
		{
		    vmsg("�����]�w�ݭn�����v��");
		    break;
		}
#endif
		bp->brdattr ^= BRD_BMCOUNT;
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

	    case 'v':
		clear();
		friend_edit(BOARD_VISABLE);
		assert(0<=currbid-1 && currbid-1<MAX_BOARD);
		hbflreload(currbid);
		clear();
		break;

	    case 'w':
		clear();
		friend_edit(BOARD_WATER);
		clear();
		break;

	    case 'o':
		clear();
		friend_edit(FRIEND_CANVOTE);
		clear();

	    case 'm':
		clear();
		b_vote_maintain();
		clear();
		break;

	    case 'n':
		clear();
		b_post_note();
		clear();
		break;

	    case 'c':
		clear();
		b_posttype();
		clear();
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
	log_usies("SetBoard", bp->brdname);
	vmsg("�w�x�s�s�]�w");
    }
    else
	vmsg("�����ܥ���]�w");

    return FULLUPDATE;
}

static int
check_newpost(boardstat_t * ptr)
{				/* Ptt �� */
    time4_t         ftime;

    ptr->myattr &= ~NBRD_UNREAD;
    if (B_BH(ptr)->brdattr & (BRD_GROUPBOARD | BRD_SYMBOLIC))
	return 0;

    if (B_TOTAL(ptr) == 0)
       {
	setbtotal(ptr->bid);
	setbottomtotal(ptr->bid);
       }
    if (B_TOTAL(ptr) == 0)
	return 0;
    ftime = B_LASTPOSTTIME(ptr);

    /* ���� util, �ר�O innbbsd, �|�Ψ����s�� time stamp,
     * �u�n���Ӹرi�N ok */
    if (ftime > now + 10) 
	ftime = B_LASTPOSTTIME(ptr) = now - 1;

    if ( brc_unread_time(ptr->bid, ftime, 0) )
	ptr->myattr |= NBRD_UNREAD;
    
    return 1;
}

static void
load_uidofgid(const int gid, const int type)
{
    boardheader_t  *bptr, *currbptr, *parent;
    int             bid, n, childcount = 0;
    assert(0<=type && type<2);
    assert(0<= gid-1 && gid-1<MAX_BOARD);
    currbptr = parent = &bcache[gid - 1];
    assert(0<=numboards && numboards<=MAX_BOARD);
    for (n = 0; n < numboards; ++n) {
	bid = SHM->bsorted[type][n]+1;
	if( bid<=0 || !(bptr = getbcache(bid)) 
		|| bptr->brdname[0] == '\0' )
	    continue;
	if (bptr->gid == gid) {
	    if (currbptr == parent)
		currbptr->firstchild[type] = bid;
	    else {
		currbptr->next[type] = bid;
		currbptr->parent = gid;
	    }
	    childcount++;
	    currbptr = bptr;
	}
    }
    parent->childcount = childcount;
    if (currbptr == parent) // no child
	currbptr->firstchild[type] = -1;
    else // the last child
	currbptr->next[type] = -1;
}

static boardstat_t *
addnewbrdstat(int n, int state)
{
    boardstat_t    *ptr;

    // ptt 2 local modification 
    // XXX maybe some developer discovered signed short issue?
    assert(n != -32769);

    assert(0<=n && n<MAX_BOARD);
    assert(0<=brdnum && brdnum<nbrdsize);
    ptr = &nbrd[brdnum++];
    //boardheader_t  *bptr = &bcache[n];
    //ptr->total = &(SHM->total[n]);
    //ptr->lastposttime = &(SHM->lastposttime[n]);
    
    ptr->bid = n + 1;
    ptr->myattr = state;
    if ((B_BH(ptr)->brdattr & BRD_HIDE) && state == NBRD_BOARD)
	B_BH(ptr)->brdattr |= BRD_POSTMASK;
    if (!IS_LISTING_FAV())
	ptr->myattr &= ~NBRD_FAV;
    check_newpost(ptr);
    return ptr;
}

#if !HOTBOARDCACHE
static int
cmpboardfriends(const void *brd, const void *tmp)
{
#ifdef USE_COOLDOWN
    if ((B_BH((boardstat_t*)tmp)->brdattr & BRD_COOLDOWN) &&
	    (B_BH((boardstat_t*)brd)->brdattr & BRD_COOLDOWN))
	return 0;
    else if ( B_BH((boardstat_t*)tmp)->brdattr & BRD_COOLDOWN ) {
	if (B_BH((boardstat_t*)brd)->nuser == 0)
	    return 0;
	else
	    return 1;
    }
    else if ( B_BH((boardstat_t*)brd)->brdattr & BRD_COOLDOWN ) {
	if (B_BH((boardstat_t*)tmp)->nuser == 0)
	    return 0;
	else
	    return -1;
    }
#endif
    return ((B_BH((boardstat_t*)tmp)->nuser) -
	    (B_BH((boardstat_t*)brd)->nuser));
}
#endif

static void
load_boards(char *key)
{
    int             type = cuser.uflag & BRDSORT_FLAG ? 1 : 0;
    int             i;
    int             state;

    brdnum = 0;
    if (nbrd) {
        free(nbrd);
	nbrdsize = 0;
	nbrd = NULL;
    }
    if (!IN_CLASS()) {
	if(IS_LISTING_FAV()){
	    fav_t   *fav = get_current_fav();
	    int     nfav = get_data_number(fav);
	    if( nfav == 0 ) {
		nbrdsize = 1;
		nbrd = (boardstat_t *)malloc(sizeof(boardstat_t) * 1);
		addnewbrdstat(0, 0); // dummy
    		return;
	    }
	    nbrdsize = nfav;
	    nbrd = (boardstat_t *)malloc(sizeof(boardstat_t) * nfav);
            for( i = 0 ; i < fav->DataTail; ++i ){
		int state;
		if (!(fav->favh[i].attr & FAVH_FAV))
		    continue;

		if ( !key[0] ){
		    if (get_item_type(&fav->favh[i]) == FAVT_LINE )
			state = NBRD_LINE;
		    else if (get_item_type(&fav->favh[i]) == FAVT_FOLDER )
			state = NBRD_FOLDER;
		    else {
			state = NBRD_BOARD;
			if (is_set_attr(&fav->favh[i], FAVH_UNREAD))
			    state |= NBRD_UNREAD;
		    }
		} else {
		    if (get_item_type(&fav->favh[i]) == FAVT_LINE )
			continue;
		    else if (get_item_type(&fav->favh[i]) == FAVT_FOLDER ){
			if( strcasestr(
			    get_folder_title(fav_getid(&fav->favh[i])),
			    key)
			)
			    state = NBRD_FOLDER;
			else
			    continue;
		    }else{
			boardheader_t *bptr = getbcache(fav_getid(&fav->favh[i]));
			assert(0<=fav_getid(&fav->favh[i])-1 && fav_getid(&fav->favh[i])-1<MAX_BOARD);
			if (strcasestr(bptr->title, key))
			    state = NBRD_BOARD;
			else
			    continue;
			if (is_set_attr(&fav->favh[i], FAVH_UNREAD))
			    state |= NBRD_UNREAD;
		    }
		}

		if (is_set_attr(&fav->favh[i], FAVH_TAG))
		    state |= NBRD_TAG;
		if (is_set_attr(&fav->favh[i], FAVH_ADM_TAG))
		    state |= NBRD_TAG;
		// ���ǤH �Y�� bid < 0 Orzz // ptt2 local modification
		if (fav_getid(&fav->favh[i]) < 1)
		    continue;
		addnewbrdstat(fav_getid(&fav->favh[i]) - 1, NBRD_FAV | state);
	    }
	}
#if HOTBOARDCACHE
	else if(IN_HOTBOARD()){
	    nbrdsize = SHM->nHOTs;
	    if(nbrdsize == 0) {
		nbrdsize = 1;
		nbrd = (boardstat_t *)malloc(sizeof(boardstat_t) * 1);
		addnewbrdstat(0, 0); // dummy
		return;
	    }
	    assert(0<nbrdsize);
	    nbrd = (boardstat_t *)malloc(sizeof(boardstat_t) * nbrdsize);
	    for( i = 0 ; i < nbrdsize; ++i ) {
		if(SHM->HBcache[i] == -1)
		    continue;
		addnewbrdstat(SHM->HBcache[i], HasBoardPerm(&bcache[SHM->HBcache[i]]));
	    }
	}
#endif
	else { // general case
	    nbrdsize = numboards;
	    assert(0<nbrdsize && nbrdsize<=MAX_BOARD);
	    nbrd = (boardstat_t *) malloc(sizeof(boardstat_t) * nbrdsize);
	    for (i = 0; i < nbrdsize; i++) {
		int n = SHM->bsorted[type][i];
		boardheader_t *bptr;
		if (n < 0)
		    continue;
		bptr = &bcache[n];
		if (bptr == NULL)
		    continue;
		if (!bptr->brdname[0] ||
		    (bptr->brdattr & (BRD_GROUPBOARD | BRD_SYMBOLIC)) ||
		    !((state = HasBoardPerm(bptr)) || GROUPOP()) ||
		    TITLE_MATCH(bptr, key)
#if ! HOTBOARDCACHE
		    || (IN_HOTBOARD() && bptr->nuser < 5)
#endif
		    )
		    continue;
		addnewbrdstat(n, state);
	    }
	}
#if ! HOTBOARDCACHE
	if (IN_HOTBOARD())
	    qsort(nbrd, brdnum, sizeof(boardstat_t), cmpboardfriends);
#endif
    } else { /* load boards of a subclass */
	boardheader_t  *bptr = getbcache(class_bid);
	int childcount; 
	int bid;

	assert(0<=class_bid-1 && class_bid-1<MAX_BOARD);
	if (bptr->firstchild[type] == 0 || bptr->childcount==0)
	    load_uidofgid(class_bid, type);

        childcount = bptr->childcount;  // Ptt: child count after load_uidofgid

	nbrdsize = childcount + 5;
	nbrd = (boardstat_t *) malloc((childcount+5) * sizeof(boardstat_t));
        // �w�d��ӥH�K�j�q�}�O�ɱ���
	for (bid = bptr->firstchild[type]; bid > 0 && 
		brdnum < childcount+5; bid = bptr->next[type]) {
	    assert(0<=bid-1 && bid-1<MAX_BOARD);
            bptr = getbcache(bid);
	    state = HasBoardPerm(bptr);
	    if ( !(state || GROUPOP()) || TITLE_MATCH(bptr, key) )
		continue;

	    if (bptr->brdattr & BRD_SYMBOLIC) {
		/* Only SYSOP knows a board is symbolic */
		if (HasUserPerm(PERM_SYSOP) || HasUserPerm(PERM_SYSSUPERSUBOP))
		    state |= NBRD_SYMBOLIC;
		else {
		    bid = BRD_LINK_TARGET(bptr);
		    if (bcache[bid - 1].brdname[0] == 0) {
			vmsg("�s���w�l���A�Ц� SYSOP �^�������D�C");
			continue;
		    }
		}
	    }
	    assert(0<=bid-1 && bid-1<MAX_BOARD);
	    addnewbrdstat(bid-1, state);
	}
        if(childcount < brdnum) {
	    //Ptt: dirty fix fix soon 
	    getbcache(class_bid)->childcount = 0;
	}
           
                 
    }
}

static int
search_board(void)
{
    int             num;
    char            genbuf[IDLEN + 2];
    struct NameList namelist;

    move(0, 0);
    clrtoeol();
    NameList_init(&namelist);
    assert(brdnum<=nbrdsize);
    NameList_resizefor(&namelist, brdnum);
    for (num = 0; num < brdnum; num++)
	if (!IS_LISTING_FAV() ||
	    (nbrd[num].myattr & NBRD_BOARD && HasBoardPerm(B_BH(&nbrd[num]))) )
	    NameList_add(&namelist, B_BH(&nbrd[num])->brdname);
    namecomplete2(&namelist, MSG_SELECT_BOARD, genbuf);
    NameList_delete(&namelist);

#ifdef DEBUG
    vmsg(genbuf);
#endif

    for (num = 0; num < brdnum; num++)
	if (!strcasecmp(B_BH(&nbrd[num])->brdname, genbuf))
	    return num;
    return -1;
}

static int
unread_position(char *dirfile, boardstat_t * ptr)
{
    fileheader_t    fh;
    char            fname[FNLEN];
    register int    num, fd, step, total;

    total = B_TOTAL(ptr);
    num = total + 1;
    if ((ptr->myattr & NBRD_UNREAD) && (fd = open(dirfile, O_RDWR)) > 0) {
	if (!brc_initial_board(B_BH(ptr)->brdname)) {
	    num = 1;
	} else {
	    num = total - 1;
	    step = 4;
	    while (num > 0) {
		lseek(fd, (off_t) (num * sizeof(fh)), SEEK_SET);
		if (read(fd, fname, FNLEN) <= 0 ||
		    !brc_unread(ptr->bid, fname, 0))
		    break;
		num -= step;
		if (step < 32)
		    step += step >> 1;
	    }
	    if (num < 0)
		num = 0;
	    while (num < total) {
		lseek(fd, (off_t) (num * sizeof(fh)), SEEK_SET);
		if (read(fd, fname, FNLEN) <= 0 ||
		    brc_unread(ptr->bid, fname, 0))
		    break;
		num++;
	    }
	}
	close(fd);
    }
    if (num < 0)
	num = 0;
    return num;
}

static char
get_fav_type(boardstat_t *ptr)
{
    if (ptr->myattr & NBRD_FOLDER)
	return FAVT_FOLDER;
    else if (ptr->myattr & NBRD_BOARD)
	return FAVT_BOARD;
    else if (ptr->myattr & NBRD_LINE)
	return FAVT_LINE;
    return 0;
}

static void
brdlist_foot(void)
{
    outs(   ANSI_COLOR(34;46) "  ��ܬݪO  " 
	    ANSI_COLOR(31;47) "  (c)" ANSI_COLOR(30) "�s�峹�Ҧ�  " 
	    ANSI_COLOR(31) "(v/V)" ANSI_COLOR(30) "�Ь��wŪ/��Ū  " 
	    ANSI_COLOR(31) "(y)" ANSI_COLOR(30));
    if(IS_LISTING_FAV())
	outs("�C�X����");
    else if (IS_LISTING_BRD())
	outs("�z��C��");
    else outs("�z��C��"); // never reach here?

    outslr("  " ANSI_COLOR(31) "(m)" ANSI_COLOR(30) "�����̷R",
	    73, ANSI_RESET, 0);
}


static inline char * 
make_class_color(char *name)
{
    /* 34 is too dark */
    char    *colorset[8] = {"", ANSI_COLOR(32),
	ANSI_COLOR(33), ANSI_COLOR(36), ANSI_COLOR(1;34), 
	ANSI_COLOR(1), ANSI_COLOR(1;32), ANSI_COLOR(1;33)};

    return colorset[(unsigned int)
	(name[0] + name[1] +
	 name[2] + name[3]) & 0x7];
}

#define HILIGHT_COLOR	ANSI_COLOR(1;36)

static void
show_brdlist(int head, int clsflag, int newflag)
{
    int             myrow = 2;
    if (unlikely(IN_CLASSROOT())) {
	currstat = CLASS;
	myrow = 6;
	showtitle("�����ݪO", BBSName);
	movie(0);
	move(1, 0);
	// TODO remove ascii art here
	outs(
	    "                                                              "
	    "��  �~�X" ANSI_COLOR(33) "��\n"
	    "                                                    ��X  " ANSI_RESET " "
	    "���i" ANSI_COLOR(47) "��" ANSI_COLOR(40) "�i�i����\n"
	    "  " ANSI_COLOR(44) "   �s�s�s�s�s�s�s�s                               "
	    ANSI_COLOR(33) "��" ANSI_RESET ANSI_COLOR(44) " �����i�i�i�������� " ANSI_RESET "\n"
	    "  " ANSI_COLOR(44) "                                                  "
	    ANSI_COLOR(33) "  " ANSI_RESET ANSI_COLOR(44) " �����i�i�i������ ��" ANSI_RESET "\n"
	    "                                  �s�s�s�s�s�s�s�s    " ANSI_COLOR(33)
	    "�x" ANSI_RESET "   ���i�i�i�i�� ��\n"
	    "                                                      " ANSI_COLOR(33) "��"
	    "�X�X" ANSI_RESET "  ��      �X��" ANSI_RESET);
    } else if (clsflag) {
	showtitle("�ݪO�C��", BBSName);
	// [m]�[�J�β��X�ڪ��̷R 
	outs("[��][q]�D��� [��][r]�\\Ū [����]��� [PgUp][PgDn]½�� [S]�Ƨ� [/]�j�M  [h]�D�U\n");
	outs(ANSI_COLOR(7));

	// boards in Ptt series are very, very large.
	// let's create more space for board numbers,
	// and less space for BM.
	//
	// newflag is not so different now because we use all 5 digits.

	outs( newflag ?  "   �`��" : "   �s��");
	outs("   ��  �O       ���O ��H  ��   ��   ��   �z           �H�� �O   �D");
	outslr("", 74, ANSI_RESET, 0);
	move(b_lines, 0);
	brdlist_foot();
    }
    if (brdnum > 0) {
	boardstat_t    *ptr;
 	char    *unread[2] = {ANSI_COLOR(37) "  " ANSI_RESET, ANSI_COLOR(1;31) "��" ANSI_RESET};
 
	if (IS_LISTING_FAV() && brdnum == 1 && get_fav_type(&nbrd[0]) == 0) {

	    // (a) or (i) needs HasUserPerm(PERM_LOGINOK)).
	    // 3 = first line of empty area
	    if (!HasFavEditPerm())
	    {
		// TODO actually we cannot use 's' (for PTT)...
		mouts(3, 10, 
		"--- ���U���ϥΪ̤~��s�W�ݪO�� (�i�� s ��ʿ��) ---");
	    } else {
		// normal user. tell him what to do.
		mouts(3, 10, 
		"--- �ťؿ��A�Ы� a �s�W�Υ� y �C�X�����ݪO��� z �W�R ---");
	    }
	    return;
	}

	while (++myrow < b_lines) {

	    move(myrow, 0);
	    clrtoeol();

	    if (head < brdnum) {
		assert(0<=head && head<nbrdsize);
		ptr = &nbrd[head++];
		if (ptr->myattr & NBRD_LINE){
		    if( !newflag )
			prints("%7d %c ", head, ptr->myattr & NBRD_TAG ? 'D' : ' ');
		    else
			prints("%7s   ", "");
				
		    if (!(ptr->myattr & NBRD_FAV))
			outs(ANSI_COLOR(1;30));

		    outs("------------" 
			    "      "
			    // "------"
			    "------------------------------------------" 
			    ANSI_RESET "\n");
		    continue;
		}
		else if (ptr->myattr & NBRD_FOLDER){
		    char *title = get_folder_title(ptr->bid);
		    prints("%7d %c ", 
			    newflag ? 
			    get_data_number(get_fav_folder(getfolder(ptr->bid))) :
			    head, ptr->myattr & NBRD_TAG ? 'D' : ' ');

		    // well, what to print with myfav folders?
		    // this style is too long and we don't want to 
		    // fight with users... 
		    // think about new way some otherday.
		    prints("%sMyFavFolder" ANSI_RESET "  �ؿ� ��%-34s", 
			    !(cuser.uflag2 & FAVNOHILIGHT)?HILIGHT_COLOR  : "",
			    title); 
		    /*
		    if (!(cuser.uflag2 & FAVNOHILIGHT))
			outs(HILIGHT_COLOR);
		    prints("%-12s", "[Folder]");
		    outs(ANSI_RESET);
		    prints(" �ؿ� �U%-34s", title);
		    */
		    /*
		    outs(ANSI_COLOR(0;36));
		    prints("�U%-70.70s", title);
		    outs(ANSI_RESET);
		    */
		    continue;
		}

		if (IN_CLASSROOT())
		    outs("          ");
		else {
		    if (!GROUPOP() && !HasBoardPerm(B_BH(ptr))) {
			if (newflag) prints("%7s", "");
			else prints("%7d", head);
			prints(" %c Unknown??    ���O �H�o�ӪO�O���O",
				ptr->myattr & NBRD_TAG ? 'D' : ' ');
			continue;
		    }
		}

		if (newflag && B_BH(ptr)->brdattr & BRD_GROUPBOARD)
		    outs("          ");
		else
		    prints("%7d%c%s", 
			    newflag ? (int)(B_TOTAL(ptr)) : head,
			    !(B_BH(ptr)->brdattr & BRD_HIDE) ? ' ' :
			    (B_BH(ptr)->brdattr & BRD_POSTMASK) ? ')' : '-',
			    (ptr->myattr & NBRD_TAG) ? "D " :
			    (B_BH(ptr)->brdattr & BRD_GROUPBOARD) ? "  " :
			    unread[ptr->myattr & NBRD_UNREAD ? 1 : 0]);

		if (!IN_CLASSROOT()) {
		    prints("%s%-13s" ANSI_RESET "%s%5.5s" ANSI_COLOR(0;37) 
			    "%2.2s" ANSI_RESET "%-34.34s",
			    ((!(cuser.uflag2 & FAVNOHILIGHT) &&
			      getboard(ptr->bid) != NULL))? HILIGHT_COLOR : "",
			    B_BH(ptr)->brdname,
			    make_class_color(B_BH(ptr)->title),
			    B_BH(ptr)->title, B_BH(ptr)->title + 5, B_BH(ptr)->title + 7);

#ifdef USE_COOLDOWN
		    if (B_BH(ptr)->brdattr & BRD_COOLDOWN)
			outs("�R ");
		    else if (B_BH(ptr)->brdattr & BRD_BAD)
#else
		    if (B_BH(ptr)->brdattr & BRD_BAD)
#endif
			outs(" X ");

		    else if (B_BH(ptr)->nuser <= 0)
			prints(" %c ", B_BH(ptr)->bvote ? 'V' : ' ');
		    else if (B_BH(ptr)->nuser <= 10)
			prints("%2d ", B_BH(ptr)->nuser);
		    else if (B_BH(ptr)->nuser <= 50)
			prints(ANSI_COLOR(1;33) "%2d" ANSI_RESET " ", B_BH(ptr)->nuser);
#ifdef EXTRA_HOTBOARD_COLORS
		    // piaip 2008/02/04: new colors
		    else if (B_BH(ptr)->nuser >= 100000)
			outs(ANSI_COLOR(1;35) "�z!" ANSI_RESET);
		    else if (B_BH(ptr)->nuser >= 60000)
			outs(ANSI_COLOR(1;33) "�z!" ANSI_RESET);
		    else if (B_BH(ptr)->nuser >= 30000)
			outs(ANSI_COLOR(1;32) "�z!" ANSI_RESET);
		    else if (B_BH(ptr)->nuser >= 10000)
			outs(ANSI_COLOR(1;36) "�z!" ANSI_RESET);
#endif
		    else if (B_BH(ptr)->nuser >= 5000)
			outs(ANSI_COLOR(1;34) "�z!" ANSI_RESET);
		    else if (B_BH(ptr)->nuser >= 2000)
			outs(ANSI_COLOR(1;31) "�z!" ANSI_RESET);
		    else if (B_BH(ptr)->nuser >= 1000)
			outs(ANSI_COLOR(1) "�z!" ANSI_RESET);
		    else if (B_BH(ptr)->nuser >= 100)
			outs(ANSI_COLOR(1) "HOT" ANSI_RESET);
		    else //if (B_BH(ptr)->nuser > 50)
			prints(ANSI_COLOR(1;31) "%2d" ANSI_RESET " ", B_BH(ptr)->nuser);
		    prints("%.*s" ANSI_CLRTOEND, t_columns - 68, B_BH(ptr)->BM);
		} else {
		    prints("%-40.40s %.*s", B_BH(ptr)->title + 7,
			   t_columns - 68, B_BH(ptr)->BM);
		}
	    }
	    clrtoeol();
	}
    }
}

static void
set_menu_BM(char *BM)
{
    if (!HasUserPerm(PERM_NOCITIZEN) && (HasUserPerm(PERM_ALLBOARD) || is_BM(BM))) {
	currmode |= MODE_GROUPOP;
	cuser.userlevel |= PERM_SYSSUBOP;
    }
}

static void replace_link_by_target(boardstat_t *board)
{
    assert(0<=board->bid-1 && board->bid-1<MAX_BOARD);
    board->bid = BRD_LINK_TARGET(getbcache(board->bid));
    board->myattr &= ~NBRD_SYMBOLIC;
}
static int
paste_taged_brds(int gid)
{
    fav_t *fav;
    int  bid, tmp;

    if (gid == 0  || ! (HasUserPerm(PERM_SYSOP) || GROUPOP()) ||
        getans("�K�W�аO���ݪO?(y/N)")!='y') return 0;
    fav = get_fav_root();
    for (tmp = 0; tmp < fav->DataTail; tmp++) {
	    boardheader_t  *bh;
	    bid = fav_getid(&fav->favh[tmp]);
	    assert(0<=bid-1 && bid-1<MAX_BOARD);
	    bh = getbcache(bid);
	    if( !is_set_attr(&fav->favh[tmp], FAVH_ADM_TAG))
		continue;
	    set_attr(&fav->favh[tmp], FAVH_ADM_TAG, FALSE);
	    if (bh->gid != gid) {
		bh->gid = gid;
		assert(0<=bid-1 && bid-1<MAX_BOARD);
		substitute_record(FN_BOARD, bh,
				  sizeof(boardheader_t), bid);
		reset_board(bid);
		log_usies("SetBoardGID", bh->brdname);
	    }
	}
    sort_bcache();
    return 1;
}

static void
choose_board(int newflag)
{
    static int      num = 0;
    boardstat_t    *ptr;
    int             head = -1, ch = 0, currmodetmp, tmp, tmp1, bidtmp;
    char            keyword[13] = "", buf[PATHLEN];

    setutmpmode(newflag ? READNEW : READBRD);
    if( get_fav_root() == NULL )
	fav_load();
    ++choose_board_depth;
    brdnum = 0;
    if (!cuser.userlevel)	/* guest yank all boards */
	LIST_BRD();

    do {
	if (brdnum <= 0) {
	    load_boards(keyword);
	    if (brdnum <= 0) {
		if (keyword[0] != 0) {
		    vmsg("�S������ݪO���D��������r");
		    keyword[0] = 0;
		    brdnum = -1;
		    continue;
		}
		if (IS_LISTING_BRD()) {
		    if (HasUserPerm(PERM_SYSOP) || GROUPOP()) {
			if (paste_taged_brds(class_bid) || 
    			    m_newbrd(class_bid, 0) == -1)
			    break;
			brdnum = -1;
			continue;
		    } else
			break;
		}
	    }
	    head = -1;
	}

	/* reset the cursor when out of range */
	if (num < 0)
	    num = 0;
	else if (num >= brdnum)
	    num = brdnum - 1;

	if (head < 0) {
	    if (newflag) {
		tmp = num;
		assert(brdnum<=nbrdsize);
		while (num < brdnum) {
		    ptr = &nbrd[num];
		    if (ptr->myattr & NBRD_UNREAD)
			break;
		    num++;
		}
		if (num >= brdnum)
		    num = tmp;
	    }
	    head = (num / p_lines) * p_lines;
	    show_brdlist(head, 1, newflag);
	} else if (num < head || num >= head + p_lines) {
	    head = (num / p_lines) * p_lines;
	    show_brdlist(head, 0, newflag);
	}
	if (IN_CLASSROOT())
	    ch = cursor_key(7 + num - head, 10);
	else
	    ch = cursor_key(3 + num - head, 0);

	switch (ch) {

		///////////////////////////////////////////////////////
		// General Hotkeys
		///////////////////////////////////////////////////////
		
	case 'h':
	    show_helpfile(fn_boardlisthelp);
	    show_brdlist(head, 1, newflag);
	    break;
	case Ctrl('W'):
	    whereami();
	    head = -1;
	    break;

	case 'c':
	    show_brdlist(head, 1, newflag ^= 1);
	    break;
	case Ctrl('I'):
	    t_idle();
	    show_brdlist(head, 1, newflag);
	    break;

	case 'e':
	case KEY_LEFT:
	case EOF:
	    ch = 'q';
	case 'q':
	    if (keyword[0]) {
		keyword[0] = 0;
		brdnum = -1;
		ch = ' ';
	    }
	    break;
	case KEY_PGUP:
	case 'P':
	case 'b':
	case Ctrl('B'):
	    if (num) {
		num -= p_lines;
		break;
	    }
	case KEY_END:
	case '$':
	    num = brdnum - 1;
	    break;
	case ' ':
	case KEY_PGDN:
	case 'N':
	case Ctrl('F'):
	    if (num == brdnum - 1)
		num = 0;
	    else
		num += p_lines;
	    break;
	case KEY_UP:
	case 'p':
	case 'k':
	    if (num-- <= 0)
		num = brdnum - 1;
	    break;
	case '*':
	    if (IS_LISTING_FAV()) {
		int i = 0;
		assert(brdnum<=nbrdsize);
		for (i = 0; i < brdnum; i++)
		{
		    ptr = &nbrd[i];
		    if (IS_LISTING_FAV()){
			assert(nbrdsize>0);
			if(get_fav_type(&nbrd[0]) != 0)
			    fav_tag(ptr->bid, get_fav_type(ptr), 2);
		    }
		    ptr->myattr ^= NBRD_TAG;
		}
		head = 9999;
	    }
	    break;
	case 't':
	    assert(0<=num && num<nbrdsize);
	    ptr = &nbrd[num];
	    if (IS_LISTING_FAV()){
		assert(nbrdsize>0);
		if(get_fav_type(&nbrd[0]) != 0)
		    fav_tag(ptr->bid, get_fav_type(ptr), EXCH);
	    }
	    else if (HasUserPerm(PERM_SYSOP) ||
		     HasUserPerm(PERM_SYSSUPERSUBOP) ||
		     HasUserPerm(PERM_SYSSUBOP) ||
		     HasUserPerm(PERM_BOARD)) {
		/* �����޲z�Ϊ� tag */
		if (ptr->myattr & NBRD_TAG)
		    set_attr(getadmtag(ptr->bid), FAVH_ADM_TAG, FALSE);
		else
		    fav_add_admtag(ptr->bid);
	    }
	    ptr->myattr ^= NBRD_TAG;
	    head = 9999;
	case KEY_DOWN:
	case 'n':
	case 'j':
	    if (++num < brdnum)
		break;
	case '0':
	case KEY_HOME:
	    num = 0;
	    break;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	    if ((tmp = search_num(ch, brdnum)) >= 0)
		num = tmp;
	    brdlist_foot();
	    break;

	case '/':
	    getdata_buf(b_lines - 1, 0, "�п�J�ݪO��������r:",
			keyword, sizeof(keyword), DOECHO);
	    brdnum = -1;
	    break;
	case 'S':
	    if(IS_LISTING_FAV()){
		move(b_lines - 2, 0);
		outs("���s�ƧǬݪO "
			ANSI_COLOR(1;33) "(�`�N, �o�Ӱʧ@�|�мg��ӳ]�w)" ANSI_RESET " \n");
		tmp = getans("�ƧǤ覡 (1)���ӪO�W�Ƨ� (2)�������O�Ƨ� ==> [0]���� ");
		if( tmp == '1' )
		    fav_sort_by_name();
		else if( tmp == '2' )
		    fav_sort_by_class();
	    }
	    else
		cuser.uflag ^= BRDSORT_FLAG;
	    brdnum = -1;
	    break;


	case 'v':
	case 'V':
	    assert(0<=num && num<nbrdsize);
	    ptr = &nbrd[num];
	    if(nbrd[num].bid < 0 || !HasBoardPerm(B_BH(ptr)))
		break;
	    if (ch == 'v') {
		ptr->myattr &= ~NBRD_UNREAD;
		brc_toggle_all_read(ptr->bid, 1);
	    } else {
		brc_toggle_all_read(ptr->bid, 0);
		ptr->myattr |= NBRD_UNREAD;
	    }
	    show_brdlist(head, 0, newflag);
	    break;
	case 's':
	    if ((tmp = search_board()) == -1) {
		show_brdlist(head, 1, newflag);
		break;
	    }
	    head = -1;
	    num = tmp;
	    break;

	case KEY_RIGHT:
	case '\n':
	case '\r':
	case 'r':
	    {
		if (IS_LISTING_FAV()) {
		    assert(nbrdsize>0);
		    if (get_fav_type(&nbrd[0]) == 0)
			break;
		    assert(0<=num && num<nbrdsize);
		    ptr = &nbrd[num];
		    if (ptr->myattr & NBRD_LINE)
			break;
		    if (ptr->myattr & NBRD_FOLDER){
			int t = num;
			num = 0;
			fav_folder_in(ptr->bid);
			choose_board(0);
			fav_folder_out();
			num = t;
			LIST_FAV(); // XXX press 'y' in fav makes yank_flag = LIST_BRD
			brdnum = -1;
			head = 9999;
			break;
		    }
		} else {
		    assert(0<=num && num<nbrdsize);
		    ptr = &nbrd[num];
		    if (ptr->myattr & NBRD_SYMBOLIC) {
			replace_link_by_target(ptr);
		    }
		}

		assert(0<=ptr->bid-1 && ptr->bid-1<MAX_BOARD);
		if (!(B_BH(ptr)->brdattr & BRD_GROUPBOARD)) {	/* �Dsub class */
		    if (HasBoardPerm(B_BH(ptr))) {
			brc_initial_board(B_BH(ptr)->brdname);

			if (newflag) {
			    setbdir(buf, currboard);
			    tmp = unread_position(buf, ptr);
			    head = tmp - t_lines / 2;
			    getkeep(buf, head > 1 ? head : 1, tmp + 1);
			}
			Read();
			check_newpost(ptr);
			head = -1;
			setutmpmode(newflag ? READNEW : READBRD);
		    }
		} else {	/* sub class */
		    move(12, 1);
		    bidtmp = class_bid;
		    currmodetmp = currmode;
		    tmp1 = num;
		    num = 0;
		    if (!(B_BH(ptr)->brdattr & BRD_TOP))
			class_bid = ptr->bid;
		    else
			class_bid = -1;	/* �����s�ե� */

		    if (!GROUPOP())	/* �p�G�٨S���p�ժ��v�� */
			set_menu_BM(B_BH(ptr)->BM);

		    if (now < B_BH(ptr)->bupdate) {
			int mr = 0;

			setbfile(buf, B_BH(ptr)->brdname, fn_notes);
			mr = more(buf, NA);
			if (mr != -1 && mr != READ_NEXT)
			    pressanykey();
		    }
		    tmp = currutmp->brc_id;
		    setutmpbid(ptr->bid);
		    free(nbrd);
		    nbrd = NULL;
		    nbrdsize = 0;
	    	    if (IS_LISTING_FAV()) {
			LIST_BRD();
			choose_board(0);
			LIST_FAV();
    		    }
		    else
			choose_board(0);
		    currmode = currmodetmp;	/* ���}�O�O��N���v�������� */
		    num = tmp1;
		    class_bid = bidtmp;
		    setutmpbid(tmp);
		    brdnum = -1;
		}
	    }
	    break;
		///////////////////////////////////////////////////////
		// MyFav Functionality (Require PERM_BASIC)
		///////////////////////////////////////////////////////
	case 'y':
	    if (HasFavEditPerm() && !(IN_CLASS())) {
		if (get_current_fav() != NULL || !IS_LISTING_FAV()){
		    yank_flag ^= 1; /* FAV <=> BRD */
		}
		brdnum = -1;
	    }
	    break;
	case Ctrl('D'):
	    if (HasFavEditPerm()) {
		if (getans("�R���Ҧ��аO[N]?") == 'y'){
		    fav_remove_all_tagged_item();
		    brdnum = -1;
		}
	    }
	    break;
	case Ctrl('A'):
	    if (HasFavEditPerm()) {
		fav_add_all_tagged_item();
		brdnum = -1;
	    }
	    break;
	case Ctrl('T'):
	    if (HasFavEditPerm()) {
		fav_remove_all_tag();
		brdnum = -1;
	    }
	    break;
	case Ctrl('P'):
            if (paste_taged_brds(class_bid))
                brdnum = -1;
            break;

	case 'L':
	    if ((HasUserPerm(PERM_SYSOP) ||
			(HasUserPerm(PERM_SYSSUPERSUBOP) && GROUPOP())) && IN_CLASS()) {
		// TODO XXX why need symlink here? Can we remove it?
		if (make_symbolic_link_interactively(class_bid) < 0)
		    break;
		brdnum = -1;
		head = 9999;
	    }
	    else if (HasFavEditPerm() && IS_LISTING_FAV()) {
		if (fav_add_line() == NULL) {
		    vmsg("�s�W���ѡA���j�u/�`�̷R �ƶq�F�̤j�ȡC");
		    break;
		}
		/* done move if it's the first item. */
		assert(nbrdsize>0);
		if (get_fav_type(&nbrd[0]) != 0)
		    move_in_current_folder(brdnum, num);
		brdnum = -1;
		head = 9999;
	    }
	    break;

	case 'z':
	case 'm':
	    if (HasFavEditPerm()) {
		assert(0<=num && num<nbrdsize);
		ptr = &nbrd[num];
		if (IS_LISTING_FAV()) {
		    if (ptr->myattr & NBRD_FAV) {
			if (getans("�A�T�w�R����? [N/y]") != 'y')
			    break;
			fav_remove_item(ptr->bid, get_fav_type(ptr));
			ptr->myattr &= ~NBRD_FAV;
		    }
		}
		else {
		    if (getboard(ptr->bid) != NULL) {
			fav_remove_item(ptr->bid, FAVT_BOARD);
			ptr->myattr &= ~NBRD_FAV;
		    }
		    else {
			if (fav_add_board(ptr->bid) == NULL)
			    vmsg("�A���̷R�Ӧh�F�� �u���");
			else
			    ptr->myattr |= NBRD_FAV;
		    }
		}
		brdnum = -1;
		head = 9999;
	    }
	    break;
	case 'M':
	    if (HasFavEditPerm()){
		if (IN_FAVORITE() && IS_LISTING_FAV()){
		    imovefav(num);
		    brdnum = -1;
		    head = 9999;
		}
	    }
	    break;
	case 'g':
	    if (HasFavEditPerm() && IS_LISTING_FAV()) {
		fav_type_t  *ft;
		if (fav_stack_full()){
		    vmsg("�ؿ��w�F�̤j�h��!!");
		    break;
		}
		if ((ft = fav_add_folder()) == NULL) {
		    vmsg("�s�W���ѡA�ؿ�/�`�̷R �ƶq�F�̤j�ȡC");
		    break;
		}
		fav_set_folder_title(ft, "�s���ؿ�");
		/* don't move if it's the first item */
		assert(nbrdsize>0);
		if (get_fav_type(&nbrd[0]) != 0)
		    move_in_current_folder(brdnum, num);
		brdnum = -1;
    		head = 9999;
	    }
	    break;
	case 'T':
	    assert(0<=num && num<nbrdsize);
	    if (HasFavEditPerm() && nbrd[num].myattr & NBRD_FOLDER) {
		fav_type_t *ft = getfolder(nbrd[num].bid);
		strlcpy(buf, get_item_title(ft), sizeof(buf));
		getdata_buf(b_lines-1, 0, "�Эק�W��: ", buf, BTLEN+1, DOECHO);
		fav_set_folder_title(ft, buf);
		brdnum = -1;
	    }
	    break;
	case 'K':
	    if (HasFavEditPerm()) {
		char c, fname[80];
		if (get_current_fav() != get_fav_root()) {
		    vmsg("�Ш�ڪ��̷R�̤W�h���楻�\\��");
		    break;
		}

		c = getans("�п�� 2)�ƥ��ڪ��̷R 3)���^�̷R�ƥ� [Q]");
		if(!c)
		    break;
		if(getans("�T�w�� [y/N] ") != 'y')
		    break;
		switch(c){
		    case '2':
			fav_save();
			setuserfile(fname, FAV);
			sprintf(buf, "%s.bak", fname);
                        Copy(fname, buf);
			break;
		    case '3':
			setuserfile(fname, FAV);
			sprintf(buf, "%s.bak", fname);
			if (!dashf(buf)){
			    vmsg("�A�S���ƥ��A���̷R��");
			    break;
			}
                        Copy(buf, fname);
			fav_free();
			fav_load();
			break;
		}
		brdnum = -1;
	    }
	    break;

	case 'a':
	case 'i':
	    if(IN_FAVORITE() && HasFavEditPerm()){
		char         bname[IDLEN + 1];
		int          bid;
		move(0, 0);
		clrtoeol();
		/* use CompleteBoard or CompleteBoardAndGroup ? */
		CompleteBoard(ANSI_COLOR(7) "�i �W�[�ڪ��̷R �j" ANSI_RESET "\n"
			"�п�J���[�J���ݪO�W��(���ť���۰ʷj�M)�G",
			bname);

		if (bname[0] && (bid = getbnum(bname)) &&
			HasBoardPerm(getbcache(bid))) {
		    fav_type_t * ptr = getboard(bid);
		    if (ptr != NULL) { // already in fav list
			// move curser to item
			for (num = 0; num<nbrdsize && bid != nbrd[num].bid; ++num);
			assert(bid==nbrd[num].bid);
		    } else {
			ptr = fav_add_board(bid);

			if (ptr == NULL)
			    vmsg("�A���̷R�Ӧh�F�� �u���");
			else {
			    ptr->attr |= NBRD_FAV;

			    if (ch == 'i' && get_data_number(get_current_fav()) > 1)
				move_in_current_folder(brdnum, num);
			    else
				num = brdnum;
			}
		    }
		}
	    }
	    brdnum = -1;
	    head = 9999;
	    break;

	case 'w':
	    /* allowing save BRC/fav once per 10 minutes */
	    if (now - last_save_fav_and_brc > 10 * 60) {
		fav_save();
		brc_finalize();

		last_save_fav_and_brc = now;
	    }
	    break;

		///////////////////////////////////////////////////////
		// Administrator Only
		///////////////////////////////////////////////////////

	case 'F':
	case 'f':
	    if (HasUserPerm(PERM_SYSOP)) {
		getbcache(class_bid)->firstchild[cuser.uflag & BRDSORT_FLAG ? 1 : 0] = 0;
		brdnum = -1;
	    }
	    break;
	case 'D':
	    if (HasUserPerm(PERM_SYSOP) ||
		    (HasUserPerm(PERM_SYSSUPERSUBOP) &&	GROUPOP())) {
		assert(0<=num && num<nbrdsize);
		ptr = &nbrd[num];
		if (ptr->myattr & NBRD_SYMBOLIC) {
		    if (getans("�T�w�R���s���H[N/y]") == 'y')
			delete_symbolic_link(getbcache(ptr->bid), ptr->bid);
		}
		brdnum = -1;
	    }
	    break;
	case 'E':
	    if (HasUserPerm(PERM_SYSOP | PERM_BOARD) || GROUPOP()) {
		assert(0<=num && num<nbrdsize);
		ptr = &nbrd[num];
		move(1, 1);
		clrtobot();
		m_mod_board(B_BH(ptr)->brdname);
		brdnum = -1;
	    }
	    break;
	case 'R':
	    if (HasUserPerm(PERM_SYSOP) || GROUPOP()) {
		m_newbrd(class_bid, 1);
		brdnum = -1;
	    }
	    break;
	case 'B':
	    if (HasUserPerm(PERM_SYSOP) || GROUPOP()) {
		m_newbrd(class_bid, 0);
		brdnum = -1;
	    }
	    break;
	case 'W':
	    if (IN_SUBCLASS() &&
		(HasUserPerm(PERM_SYSOP) || GROUPOP())) {
		setbpath(buf, getbcache(class_bid)->brdname);
		mkdir(buf, 0755);	/* Ptt:�}�s�եؿ� */
		b_note_edit_bname(class_bid);
		brdnum = -1;
	    }
	    break;

	}
    } while (ch != 'q');
    free(nbrd);
    nbrd = NULL;
    nbrdsize = 0;
    --choose_board_depth;
}

int
Class(void)
{
    init_brdbuf();
    class_bid = 1;
    LIST_BRD();
    choose_board(0);
    return 0;
}

int
Favorite(void)
{
    init_brdbuf();
    class_bid = 0;
    LIST_FAV();
    choose_board(0);
    return 0;
}


int
New(void)
{
    int             mode0 = currutmp->mode;
    int             stat0 = currstat;

    class_bid = 0;
    init_brdbuf();
    choose_board(1);
    currutmp->mode = mode0;
    currstat = stat0;
    return 0;
}
