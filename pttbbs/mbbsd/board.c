/* $Id$ */
#include "bbs.h"
#define BRC_STRLEN 15		/* Length of board name */
#define BRC_MAXSIZE     24576   /* Effective size of brc rc file */
#define BRC_ITEMSIZE    (BRC_STRLEN + 1 + BRC_MAXNUM * sizeof( int ))
    /* Maximum size of each record */
#define BRC_MAXNUM      80      /* Upper bound of brc_num, size of brc_list  */

/* brc rc file form:
 * board_name     15 bytes
 * brc_num         1 byte, binary integer
 * brc_list       brc_num * sizeof(int) bytes, brc_num binary integer(s) */

static char    *
brc_getrecord(char *ptr, char *name, int *pnum, int *list)
{
    int             num;
    char           *tmp;

    strncpy(name, ptr, BRC_STRLEN); /* board_name */
    ptr += BRC_STRLEN;
    num = (*ptr++) & 0xff;          /* brc_num */
    tmp = ptr + num * sizeof(int);  /* end of this record */
    if (num > BRC_MAXNUM) /* FIXME if this happens... may crash next time. */
	num = BRC_MAXNUM;
    *pnum = num;
    memcpy(list, ptr, num * sizeof(int)); /* brc_list */
    return tmp;
}

static time_t   brc_expire_time;
 /* Will be set to the time one year before login. All the files created
  * before then will be recognized read. */

static char    *
brc_putrecord(char *ptr, const char *name, int num, const int *list)
{
    if (num > 0 && list[0] > brc_expire_time) {
	if (num > BRC_MAXNUM)
	    num = BRC_MAXNUM;

	while (num > 1 && list[num - 1] < brc_expire_time)
	    num--; /* not to write the times before brc_expire_time */

	strncpy(ptr, name, BRC_STRLEN);  /* write in board_name */
	ptr += BRC_STRLEN;
	*ptr++ = num;                    /* write in brc_num */
	memcpy(ptr, list, num * sizeof(int)); /* write in brc_list */
	ptr += num * sizeof(int);
    }
    return ptr;
}

static int      brc_changed = 0;
/* The below two will be filled by read_brc_buf() and brc_update() */
static char     brc_buf[BRC_MAXSIZE];
static int      brc_size;
//static char     brc_name[BRC_STRLEN]; /* board name of the brc data */
static char * const fn_boardrc = ".boardrc";
/* unused variable
char *brc_buf_addr=brc_buf;
*/

void
brc_update()
{
    if (brc_changed && cuser.userlevel) {
	char            dirfile[STRLEN], *ptr;
	char            tmp_buf[BRC_MAXSIZE - BRC_ITEMSIZE], *tmp;
	char            tmp_name[BRC_STRLEN];
	int             tmp_list[BRC_MAXNUM], tmp_num;
	int             fd, tmp_size;

	ptr = brc_buf;
	if (brc_num > 0)
	    ptr = brc_putrecord(ptr, currboard, brc_num, brc_list);

	setuserfile(dirfile, fn_boardrc);
	if ((fd = open(dirfile, O_RDONLY)) != -1) {
	    tmp_size = read(fd, tmp_buf, sizeof(tmp_buf));
	    close(fd);
	} else {
	    tmp_size = 0;
	}

	tmp = tmp_buf;
	while (tmp < &tmp_buf[tmp_size] && (*tmp >= ' ' && *tmp <= 'z')) {
	    /* for each available records */
	    tmp = brc_getrecord(tmp, tmp_name, &tmp_num, tmp_list);
	    if (strncmp(tmp_name, currboard, BRC_STRLEN))
		/* not overwrite the currend record */
		ptr = brc_putrecord(ptr, tmp_name, tmp_num, tmp_list);
	}
	brc_size = (int)(ptr - brc_buf);

	if ((fd = open(dirfile, O_WRONLY | O_CREAT, 0644)) != -1) {
	    ftruncate(fd, 0);
	    write(fd, brc_buf, brc_size);
	    close(fd);
	}
	brc_changed = 0;
    }
}

static void
read_brc_buf()
{
    char            dirfile[STRLEN];
    int             fd;

    if (brc_buf[0] == '\0') {
	setuserfile(dirfile, fn_boardrc);
	if ((fd = open(dirfile, O_RDONLY)) != -1) {
	    brc_size = read(fd, brc_buf, sizeof(brc_buf));
	    close(fd);
	} else {
	    brc_size = 0;
	}
    }
}

int
brc_initial(const char *boardname)
{
    char           *ptr;
    char            tmp_name[BRC_STRLEN];
    if (strcmp(currboard, boardname) == 0) {
	return brc_num;
    }
    brc_update(); /* write back first */
    strlcpy(currboard, boardname, sizeof(currboard));
    currbid = getbnum(currboard);
    currbrdattr = bcache[currbid - 1].brdattr;
    read_brc_buf();

    ptr = brc_buf;
    while (ptr < &brc_buf[brc_size] && (*ptr >= ' ' && *ptr <= 'z')) {
	/* for each available records */
	ptr = brc_getrecord(ptr, tmp_name, &brc_num, brc_list);
	if (strncmp(tmp_name, currboard, BRC_STRLEN) == 0)
	    return brc_num;
    }
    strncpy(tmp_name, boardname, BRC_STRLEN);
    brc_num = brc_list[0] = 1;
    /* We don't have to set brc_changed to 0 here, since brc_update() already
     * did that. */
    return 0;
}

void
brc_addlist(const char *fname)
{
    int             ftime, n, i;

    if (!cuser.userlevel)
	return;

    ftime = atoi(&fname[2]);
    if (ftime <= brc_expire_time /* too old, don't do any thing  */
	 /* || fname[0] != 'M' || fname[1] != '.' */ ) {
	return;
    }
    if (brc_num <= 0) { /* uninitialized */
	brc_list[0] = ftime;
	brc_num = 1;
	brc_changed = 1;
	return;
    }
    if ((brc_num == 1) && (ftime < brc_list[0])) /* most when after 'v' */
	return;
    for (n = 0; n < brc_num; n++) { /* using linear search */
	if (ftime == brc_list[n]) {
	    return;
	} else if (ftime > brc_list[n]) {
	    if (brc_num < BRC_MAXNUM)
		brc_num++;
	    /* insert ftime in to brc_list */
	    for (i = brc_num - 1; --i >= n; brc_list[i + 1] = brc_list[i]);
	    brc_list[n] = ftime;
	    brc_changed = 1;
	    return;
	}
    }
    /* (by scw) These lines are no used. Since if it reachs here, this file
     * is already been labeled read.
    if (brc_num < BRC_MAXNUM) {
	brc_list[brc_num++] = ftime;
	brc_changed = 1;
    }
    */
}

static int
brc_unread_time(time_t ftime, int bnum, const int *blist)
{
    int             n;

    if (ftime <= brc_expire_time) /* too old */
	return 0;

    if (brc_num <= 0)
	return 1;
    for (n = 0; n < bnum; n++) { /* using linear search */
	if (ftime > blist[n])
	    return 1;
	else if (ftime == blist[n])
	    return 0;
    }
    return 0;
}

int
brc_unread(const char *fname, int bnum, const int *blist)
{
    int             ftime, n;

    ftime = atoi(&fname[2]); /* this will get the time of the file created */

    if (ftime <= brc_expire_time) /* too old */
	return 0;

    if (brc_num <= 0)
	return 1;
    for (n = 0; n < bnum; n++) { /* using linear search */
	if (ftime > blist[n])
	    return 1;
	else if (ftime == blist[n])
	    return 0;
    }
    return 0;
}

#define BRD_UNREAD 	1
#define BRD_FAV    	2
#define BRD_LINE   	4
#define BRD_TAG    	8
#define BRD_GRP_HEADER 16

#define MAX_GRP_BRD    16
#define MAX_GRP

#define FAVNB      ".favnb"
#define FAV3       ".fav3"

#define B_TOTAL(bptr)        (SHM->total[(bptr)->bid - 1])
#define B_LASTPOSTTIME(bptr) (SHM->lastposttime[(bptr)->bid - 1])
#define B_BH(bptr)           (&bcache[(bptr)->bid - 1])
typedef struct {
    int             bid;
    unsigned char   myattr;
} __attribute__ ((packed)) boardstat_t;

typedef struct {
    short           bid;
    char            attr;
    time_t          lastvisit;
} fav_board_t;

typedef struct {
#ifdef MEM_CHECK
    int             memcheck;
#endif
    short           nDatas;
    short           nAllocs;
    char            nLines;
    fav_board_t  b[0];
} fav_t;
    
static boardstat_t *nbrd = NULL;
char   favchange = 0, choose_board_depth = 0;
fav_t *fav;
static short    brdnum;
static char     yank_flag = 1;

int cmpfav(const void *a, const void *b)
{
    if( *(short *)a > ((fav_board_t *)b)->bid )
	return 1;
    else if( *(short *)a == ((fav_board_t *)b)->bid )
	return 0;
    return -1;
}

fav_board_t *getfav(short bid)
{
    int i;
    for(i = 0; i < fav->nDatas; i++)
	if(fav->b[i].bid == bid)
	    break;
    return i == fav->nDatas ? NULL : &fav->b[i];
}

char getfavattr(short bid)
{
    fav_board_t *ptr = getfav(bid);
    if( ptr == NULL )
	return 0; // default here
    else
	return ptr->attr;
}

time_t getfavtime(short bid)
{
    fav_board_t *ptr = getfav(bid);
    if( ptr == NULL )
	return login_start_time; // default here
    else
	return ptr->lastvisit;
}

void basemovefav(int src, int des)
{
    int i;
    fav_board_t tmp = fav->b[src];

    if(src < des){
	for(i = src; i < des; i++)
	    fav->b[i] = fav->b[i + 1];
    }
    else{ // des < src
	for(i = src; i > des; i--)
	    fav->b[i] = fav->b[i - 1];
    }
    fav->b[des] = tmp;

    brdnum = -1;
}

void movefav(int old, int new)
{
    int i, src = -1, des = -1;
    favchange = 1;

    for(i = 0; i < fav->nDatas; i++){
	if(nbrd[old].bid == fav->b[i].bid)
	    src = i;
	if(nbrd[new].bid == fav->b[i].bid)
	    des = i;
    }

    if(src == -1 || des == -1)
	return;
    basemovefav(src, des);
}

void delfavline(int bid, int num)
{
    int i;

    movefav(num, --brdnum);
    fav->nLines++;
    fav->nDatas--;

    for(i = 0; i < fav->nDatas; i++)
	if(fav->b[i].bid < bid)
	    fav->b[i].bid++;
    for(i = 0; i < brdnum; i++)
	if(nbrd[i].bid < bid)
	    nbrd[i].bid++;
}

void setfav(short bid, char attr, char mode, time_t t)
{
    /* mode: 0: �]�� off, 1: �]�� on, 2: �Ϭ� */
    fav_board_t *ptr = getfav(bid);

    favchange = 1;

    if( ptr != NULL ){
	if( mode == 2 )
	    ptr->attr ^= attr;
	else if( mode )
	    ptr->attr |= attr;
	else
	    ptr->attr &= ~attr;
	if( t )
	    ptr->lastvisit = t;
    }
    else{
	int     where;
	if( fav->nDatas == fav->nAllocs ){
#ifdef DEBUG
	    vmsg("realloc fav");
#endif
	    fav = realloc(fav,
		    sizeof(fav_t) +
		    sizeof(fav_board_t) *
		    (16 + fav->nAllocs));
	    memset(&fav->b[fav->nDatas], 0, sizeof(fav_board_t) * 16);
	    fav->nAllocs += 16;
	}

	where = fav->nDatas;

	if( attr & BRD_LINE ){
	    fav->b[where].bid = --(fav->nLines);
	    fav->b[where].attr = mode ? (BRD_LINE | BRD_FAV) : 0;
	}
	else{
	    fav->b[where].bid = bid ;
	    fav->b[where].attr = mode ? attr : 0;
	}
	
	fav->b[where].lastvisit = t ? t : login_start_time;
	fav->nDatas++;
    }
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
    movefav(old, new);
}

#define BRD_OLD 0
#define BRD_NEW 1
#define BRD_END 2

void updatenewfav(int mode)
{
    /* mode: 0: don't write to fav  1: write to fav */
    int i, fd;
    char fname[80], *brd;

    if(!(cuser.uflag2 & FAVNEW_FLAG))
	return;

    setuserfile(fname, FAVNB);

    if( (fd = open(fname, O_RDWR, 0600)) != -1 ){

	brd = (char *)malloc((numboards + 1) * sizeof(char));
	read(fd, brd, (numboards + 1) * sizeof(char));
	
	for(i = 0; i < numboards + 1 && brd[i] != BRD_END; i++){
	    if(brd[i] == BRD_NEW){
		if(bcache[i].brdname[0] && Ben_Perm(&bcache[i])){ // check the permission if the board exsits
		    if(mode)
			setfav(i + 1, BRD_FAV, 1, 0);
		    brd[i] = BRD_OLD;
		}
	    }
	    else{
		if(!bcache[i].brdname[0])
		    brd[i] = BRD_NEW;
	    }
	}
	if( i < numboards) // the board number may change
	    for(i-- ; i < numboards; i++){
		if(bcache[i].brdname[0] && Ben_Perm(&bcache[i])){
		    if(mode)
			setfav(i + 1, BRD_FAV, 1, 0);
		    brd[i] = BRD_OLD;
		}
		else
		    brd[i] = BRD_NEW;
	    }

	brd[i] = BRD_END;
	
	lseek(fd, 0, SEEK_SET);
	write(fd, brd, (numboards + 1 ) * sizeof(char));
	free(brd);
	close(fd);
    }
}

void favclean(fav_t *fav){
    int i;
    boardheader_t *bptr;

    for(i = 0; i < fav->nDatas; i++){
	if(fav->b[i].attr & BRD_LINE)
	    continue;
	bptr = &bcache[ fav->b[i].bid - 1 ];
	if(!(fav->b[i].attr & BRD_FAV) || !Ben_Perm(bptr)){
	    basemovefav(i, fav->nDatas--);
	    continue;
	}
    }
}

void freefav(fav_t *fav){
    free(fav->b);
    free(fav);
    fav = NULL;
}

void load_brdbuf(void)
{
    static  char    firsttime = 1;
    int     fd;
    char    fname[80];

    setuserfile(fname, FAV3);
    if( (fd = open(fname, O_RDONLY)) == -1 ){
	// not found.
	int i;
	char favrec;
	fav = (fav_t *)malloc(sizeof(fav_t) +
				    sizeof(fav_board_t) * 16);
	fav->nDatas = 0;
	fav->nAllocs = 16;
	fav->nLines = 0;
	favchange = 1;

	setuserfile(fname, ".fav2");

	if( (fd = open(fname, O_RDONLY)) != -1 ){
	    for( i = 1; i <= numboards; i++){
		if(read(fd, &favrec, sizeof(char)) < 0)
		    break;
		if( (favrec & BRD_FAV) )
		    setfav(i, BRD_FAV, 1, 0);
	    }
	    close(fd);
	}
    }
    else{
	short   nDatas, i;
	char    nLines;
	read(fd, &nDatas, sizeof(nDatas));
	read(fd, &nLines, sizeof(nLines));
	fav = (fav_t *)malloc(sizeof(fav_t) +
				    sizeof(fav_board_t) * (nDatas + 16));
	fav->nDatas = nDatas;
	fav->nAllocs = nDatas + 16;
	fav->nLines = nLines;
	read(fd, fav->b, sizeof(fav_board_t) * fav->nDatas);
	for(i = 0; i < fav->nDatas; i++){
	    if( !(fav->b[i].attr & BRD_LINE) && !bcache[fav->b[i].bid - 1].brdname[0])
		fav->b[i].attr ^= BRD_FAV;
	}
	close(fd);
    }
    updatenewfav(1);
#ifdef MEM_CHECK
	fav->memcheck = MEM_CHECK;
#endif
    firsttime = 0;
}

void
init_brdbuf()
{
    brc_expire_time = login_start_time - 365 * 86400;
}

void
save_brdbuf(void)
{
    int     r, w, fd;
    char    fname[80];
    if( !fav )
	return;

    if( !favchange )
	return;
    
#ifdef MEM_CHECK
    if( fav->memcheck != MEM_CHECK )
	return;
#endif

    fav->nLines = 0;
    for( r = w = 0 ; r < fav->nDatas ; ++r ){
	if( ( fav->b[r].attr & BRD_LINE ) ||
	     (fav->b[r].attr & BRD_FAV && bcache[fav->b[r].bid - 1].brdname[0])){
	    if(fav->b[r].attr & BRD_LINE)
		fav->nLines--;
	    fav->b[w++] = fav->b[r];
	}
    }
    fav->nDatas = w;
    setuserfile(fname, FAV3);
    if( (fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0600)) != -1 ){
	int i;
	write(fd, &fav->nDatas, sizeof(short));
	write(fd, &fav->nLines, sizeof(char));
	for(i = 0; i < fav->nDatas; i++)
	    if(fav->b[i].attr & BRD_FAV)
		write(fd, &fav->b[i], sizeof(fav_board_t));
	close(fd);
    }
    freefav(fav);
}

int
Ben_Perm(boardheader_t * bptr)
{
    register int    level, brdattr;

    level = bptr->level;
    brdattr = bptr->brdattr;

    if (HAS_PERM(PERM_SYSOP))
	return 1;

    if( is_BM_cache(bptr - bcache + 1) ) /* XXXbid */
	return 1;

    /* ���K�ݪO�G�ֹﭺ�u�O�D���n�ͦW�� */

    if (brdattr & BRD_HIDE) {	/* ���� */
	if (hbflcheck((int)(bptr - bcache) + 1, currutmp->uid)) {
	    if (brdattr & BRD_POSTMASK)
		return 0;
	    else
		return 2;
	} else
	    return 1;
    }
    /* ����\Ū�v�� */
    if (level && !(brdattr & BRD_POSTMASK) && !HAS_PERM(level))
	return 0;

    return 1;
}

#if 0
static int
have_author(char *brdname)
{
    char            dirname[100];

    snprintf(dirname, sizeof(dirname),
	     "���b�j�M�@��[33m%s[m �ݪO:[1;33m%s[0m.....",
	     currauthor, brdname);
    move(b_lines, 0);
    clrtoeol();
    outs(dirname);
    refresh();

    setbdir(dirname, brdname);
    str_lower(currowner, currauthor);

    return search_rec(dirname, cmpfowner);
}
#endif

static int
check_newpost(boardstat_t * ptr)
{				/* Ptt �� */
    int             tbrc_list[BRC_MAXNUM], tbrc_num;
    char            bname[BRC_STRLEN];
    char           *po;
    time_t          ftime;

    ptr->myattr &= ~BRD_UNREAD;
    if (B_BH(ptr)->brdattr & BRD_GROUPBOARD)
	return 0;

    if (B_TOTAL(ptr) == 0)
	setbtotal(ptr->bid);
    if (B_TOTAL(ptr) == 0)
	return 0;
    ftime = B_LASTPOSTTIME(ptr);
    if( ftime > now )
	ftime = B_LASTPOSTTIME(ptr) = now - 1;
    read_brc_buf();
    po = brc_buf;
    while (po < &brc_buf[brc_size] && (*po >= ' ' && *po <= 'z')) {
	po = brc_getrecord(po, bname, &tbrc_num, tbrc_list);
	if (strncmp(bname, B_BH(ptr)->brdname, BRC_STRLEN) == 0) {
	    if (brc_unread_time(ftime, tbrc_num, tbrc_list)) {
		ptr->myattr |= BRD_UNREAD;
	    }
	    return 1;
	}
    }

    ptr->myattr |= BRD_UNREAD;
    return 1;
}

static void
load_uidofgid(const int gid, const int type)
{
    boardheader_t  *bptr, *currbptr;
    int             n, childcount = 0;
    currbptr = &bcache[gid - 1];
    for (n = 0; n < numboards; ++n) {
	bptr = SHM->bsorted[type][n];
	if (bptr->brdname[0] == '\0')
	    continue;
	if (bptr->gid == gid) {
	    if (currbptr == &bcache[gid - 1])
		currbptr->firstchild[type] = bptr;
	    else {
		currbptr->next[type] = bptr;
		currbptr->parent = &bcache[gid - 1];
	    }
	    childcount++;
	    currbptr = bptr;
	}
    }
    bcache[gid - 1].childcount = childcount;
    if (currbptr == &bcache[gid - 1])
	currbptr->firstchild[type] = NULL;
    else
	currbptr->next[type] = NULL;
}

static boardstat_t *
addnewbrdstat(int n, int state)
{
    boardstat_t    *ptr;

    ptr = &nbrd[brdnum++];
    //boardheader_t  *bptr = &bcache[n];
    //ptr->total = &(SHM->total[n]);
    //ptr->lastposttime = &(SHM->lastposttime[n]);
    
    ptr->bid = n + 1;
    ptr->myattr = getfavattr(ptr->bid);
    if ((B_BH(ptr)->brdattr & BRD_HIDE) && state == 1)
	B_BH(ptr)->brdattr |= BRD_POSTMASK;
    check_newpost(ptr);
    return ptr;
}

static int
cmpboardfriends(const void *brd, const void *tmp)
{
    return ((B_BH((boardstat_t*)tmp)->nuser) -
	    (B_BH((boardstat_t*)brd)->nuser));
}

static int
favcmpboardname(const void *brd, const void *tmp)
{
    int     b1 = ((fav_board_t *)brd)->bid;
    int     b2 = ((fav_board_t *)tmp)->bid;

    if( b1 <= 0 || b2 <= 0 ) // ���j�u
	return 1;

    return strcasecmp(bcache[b1 - 1].brdname, bcache[b2 - 1].brdname);
}

static int
favcmpboardclass(const void *brd, const void *tmp)
{
    int     b1 = ((fav_board_t *)brd)->bid;
    int     b2 = ((fav_board_t *)tmp)->bid;
    int     cmp;

    if( b1 <= 0 || b2 <= 0 ) // ���j�u
	return 1;

    cmp = strncasecmp(bcache[b1 - 1].title, bcache[b2 - 1].title, 4);
    if( cmp )
	return cmp;
    else
	return strcasecmp(bcache[b1 - 1].brdname, bcache[b2 - 1].brdname);
}

static void
load_boards(char *key)
{
    boardheader_t  *bptr = NULL;
    int             type = cuser.uflag & BRDSORT_FLAG ? 1 : 0;
    int             i, n;
    int             state;
    char            byMALLOC = 0, needREALLOC = 0;

    if (class_bid > 0) {
	bptr = &bcache[class_bid - 1];
	if (bptr->firstchild[type] == NULL || bptr->childcount <= 0)
	    load_uidofgid(class_bid, type);
    }
    brdnum = 0;
    if (nbrd) {
        free(nbrd);
	nbrd = NULL;
    }
    if (class_bid <= 0) {
	if( yank_flag == 0 ){ // fav mode
	    nbrd = (boardstat_t *)malloc(sizeof(boardstat_t) * fav->nDatas);
            for( i = 0 ; i < fav->nDatas ; ++i ){
		if( fav->b[i].attr & BRD_FAV ){
		    if( fav->b[i].attr & BRD_LINE && !key[0])
			addnewbrdstat(fav->b[i].bid - 1, BRD_FAV | BRD_LINE);
		    else{
			bptr = &bcache[ fav->b[i].bid - 1 ];
			if( (state = Ben_Perm(bptr)) && (!key[0] || strcasestr(bptr->title, key)))
			    addnewbrdstat(fav->b[i].bid - 1, state);
    		    }
		}
	    }
	    byMALLOC = 0;
	    needREALLOC = (fav->nDatas != brdnum);
	}
	else{ // general case
	    nbrd = (boardstat_t *) MALLOC(sizeof(boardstat_t) * numboards);
	    for (i = 0; i < numboards; i++) {
		if ((bptr = SHM->bsorted[type][i]) == NULL)
		    continue;
		n = (int)(bptr - bcache);
		if (!bptr->brdname[0] ||
			(bptr->brdattr & BRD_GROUPBOARD) ||
			!((state = Ben_Perm(bptr)) || (currmode & MODE_MENU)) ||
			(key[0] && !strcasestr(bptr->title, key)) ||
			(class_bid == -1 && bptr->nuser < 5))
		    continue;
		addnewbrdstat(n, state);
	    }
#ifdef CRITICAL_MEMORY
	    byMALLOC = 1;
#else
	    byMALLOC = 0;
#endif
	    needREALLOC = 1;
	}
	if (class_bid == -1)
	    qsort(nbrd, brdnum, sizeof(boardstat_t), cmpboardfriends);

    } else {
	int     childcount = bptr->childcount;
	nbrd = (boardstat_t *) malloc(childcount * sizeof(boardstat_t));
	for (bptr = bptr->firstchild[type]; bptr != NULL;
	     bptr = bptr->next[type]) {
	    n = (int)(bptr - bcache);
	    if (!((state = Ben_Perm(bptr)) || (currmode & MODE_MENU))
		|| (yank_flag == 0 && !(getfavattr(n) & BRD_FAV)) ||
		(key[0] && !strcasestr(bptr->title, key)))
		continue;
	    addnewbrdstat(n, state);
	}
	byMALLOC = 0;
	needREALLOC = (childcount != brdnum);
    }

    if( needREALLOC ){
	if( byMALLOC ){
	    boardstat_t *newnbrd;
	    newnbrd = (boardstat_t *)malloc(sizeof(boardstat_t) * brdnum);
	    memcpy(newnbrd, nbrd, sizeof(boardstat_t) * brdnum);
	    FREE(nbrd);
	    nbrd = newnbrd;
	}
	else {
	    nbrd = (boardstat_t *)realloc(nbrd, sizeof(boardstat_t) * brdnum);
	}
    }
}

static int
search_board()
{
    int             num;
    char            genbuf[IDLEN + 2];
    move(0, 0);
    clrtoeol();
    CreateNameList();
    for (num = 0; num < brdnum; num++)
	AddNameList(B_BH(&nbrd[num])->brdname);
    namecomplete(MSG_SELECT_BOARD, genbuf);
    FreeNameList();
    toplev = NULL;

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
    if ((ptr->myattr & BRD_UNREAD) && (fd = open(dirfile, O_RDWR)) > 0) {
	if (!brc_initial(B_BH(ptr)->brdname)) {
	    num = 1;
	} else {
	    num = total - 1;
	    step = 4;
	    while (num > 0) {
		lseek(fd, (off_t) (num * sizeof(fh)), SEEK_SET);
		if (read(fd, fname, FNLEN) <= 0 ||
		    !brc_unread(fname, brc_num, brc_list))
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
		    brc_unread(fname, brc_num, brc_list))
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

static void
brdlist_foot()
{
    prints("\033[34;46m  ��ܬݪO  \033[31;47m  (c)\033[30m�s�峹�Ҧ�  "
	   "\033[31m(v/V)\033[30m�аO�wŪ/��Ū  \033[31m(y)\033[30m�z��%s"
	   "  \033[31m(m)\033[30m�����̷R  \033[m",
	   yank_flag == 0 ? "�̷R" : yank_flag == 1 ? "����" : "����");
}

static void
show_brdlist(int head, int clsflag, int newflag)
{
    int             myrow = 2;
    if (class_bid == 1) {
	currstat = CLASS;
	myrow = 6;
	showtitle("�����ݪO", BBSName);
	movie(0);
	move(1, 0);
	outs(
	    "                                                              "
	    "��  ���X\033[33m��\n"
	    "                                                    ��X  \033[m "
	    "���i\033[47m��\033[40m�i�i����\n"
	    "  \033[44m   �s�s�s�s�s�s�s�s                               "
	    "\033[33m��\033[m\033[44m �����i�i�i�������� \033[m\n"
	    "  \033[44m                                                  "
	    "\033[33m  \033[m\033[44m �����i�i�i������ ��\033[m\n"
	    "                                  �s�s�s�s�s�s�s�s    \033[33m"
	    "�x\033[m   ���i�i�i�i�� ��\n"
	    "                                                      \033[33m��"
	    "�X�X\033[m  ��      �X��\033[m");
    } else if (clsflag) {
	showtitle("�ݪO�C��", BBSName);
	prints("[��]�D��� [��]�\\Ū [����]��� [y]���J [S]�Ƨ� [/]�j�M "
	       "[TAB]��K�E�ݪO [h]�D�U\n"
	       "\033[7m%-20s ���O ��H%-31s�H�� �O    �D     \033[m",
	       newflag ? "�`�� ��Ū ��  �O" : "  �s��  ��  �O",
	       "  ��   ��   ��   �z");
	move(b_lines, 0);
	brdlist_foot();
    }
    if (brdnum > 0) {
	boardstat_t    *ptr;
	char    *color[8] = {"", "\033[32m",
	    "\033[33m", "\033[36m", "\033[34m", "\033[1m",
	"\033[1;32m", "\033[1;33m"};
	char    *unread[2] = {"\33[37m  \033[m", "\033[1;31m��\033[m"};

	while (++myrow < b_lines) {
	    move(myrow, 0);
	    clrtoeol();
	    if (head < brdnum) {
		ptr = &nbrd[head++];
		if(ptr->myattr & BRD_LINE){
		    prints("%5d %c ------------      ------------------------------------------", head, ptr->myattr & BRD_TAG ? 'D' : ' ');
		    continue;
		}
		if (class_bid == 1)
		    prints("          ");
		if (!newflag) {
		    prints("%5d%c%s", head,
			   !(B_BH(ptr)->brdattr & BRD_HIDE) ? ' ' :
			   (B_BH(ptr)->brdattr & BRD_POSTMASK) ? ')' : '-',
			   (ptr->myattr & BRD_TAG) ? "D " :
			   (B_BH(ptr)->brdattr & BRD_GROUPBOARD) ? "  " :
			   unread[ptr->myattr & BRD_UNREAD]);
		} else {
		    if (newflag) {
			if ((B_BH(ptr)->brdattr & BRD_GROUPBOARD))
			    prints("        ");
			else
			    prints("%6d%s", (int)(B_TOTAL(ptr)),
				    unread[ptr->myattr & BRD_UNREAD]);
		    }
		}
		if (class_bid != 1) {
		    prints("%s%-13s\033[m%s%5.5s\033[0;37m%2.2s\033[m"
			   "%-34.34s",
			   ((ptr->myattr & BRD_FAV) &&
			    !(cuser.uflag2 & FAVNOHILIGHT))? "\033[1;36m" : "",
			    B_BH(ptr)->brdname,
			    color[(unsigned int)
			    (B_BH(ptr)->title[1] + B_BH(ptr)->title[2] +
			     B_BH(ptr)->title[3] + B_BH(ptr)->title[0]) & 07],
			    B_BH(ptr)->title, B_BH(ptr)->title + 5, B_BH(ptr)->title + 7);

		    if (B_BH(ptr)->brdattr & BRD_BAD)
			prints(" X ");
		    else if (B_BH(ptr)->nuser >= 1000)
			prints("\033[1m�z!\033[m");
		    else if (B_BH(ptr)->nuser >= 100)
			prints("\033[1mHOT\033[m");
		    else if (B_BH(ptr)->nuser > 50)
			prints("\033[1;31m%2d\033[m ", B_BH(ptr)->nuser);
		    else if (B_BH(ptr)->nuser > 10)
			prints("\033[1;33m%2d\033[m ", B_BH(ptr)->nuser);
		    else if (B_BH(ptr)->nuser > 0)
			prints("%2d ", B_BH(ptr)->nuser);
		    else
			prints(" %c ", B_BH(ptr)->bvote ? 'V' : ' ');
		    prints("%.*s\033[K", t_columns - 67, B_BH(ptr)->BM);
		} else {
		    prints("%-40.40s %.*s", B_BH(ptr)->title + 7,
			   t_columns - 67, B_BH(ptr)->BM);
		}
	    }
	    clrtoeol();
	}
    }
}

static char    *choosebrdhelp[] = {
    "\0�ݪO��滲�U����",
    "\01�򥻫��O",
    "(p)(��)/(n)(��)�W�@�ӬݪO / �U�@�ӬݪO",
    "(P)(^B)(PgUp)  �W�@���ݪO",
    "(N)(^F)(PgDn)  �U�@���ݪO",
    "($)/(s)/(/)    �̫�@�ӬݪO / �j�M�ݪO / �H����j�M�ݪO����r",
    "(�Ʀr)         ���ܸӶ���",
    "\01�i�����O",
    "(^W)           �g���F �ڦb����",
    "(r)(��)(Rtn)   �i�J�h�\\��\\Ū���",
    "(q)(��)        �^��D���",
    "(y/Z)          �ڪ��̷R,�Ҧ��ݪO/�q�\\�s�}�ݪO",
    "(L/K)          �[�J���j�u�ܧڪ��̷R / �ƥ�,�M�z�ڪ��̷R",
    "(v/V)          �q�q�ݧ�/������Ū",
    "(S)            ���Ӧr��/�����Ƨ�",
    "(t/^T/^A/^D)   �аO�ݪO/�����Ҧ��аO/ �N�w�аO�̥[�J/���X�ڪ��̷R",
    "(m/M)          ��ݪO�[�J�β��X�ڪ��̷R,�������j�u/�h���ڪ��̷R",
    "\01�p�ժ����O",
    "(E/W/B)        �]�w�ݪO/�]�w�p�ճƧ�/�}�s�ݪO",
    "(^P)           ���ʤw�аO�ݪO�즹����",
    NULL
};


static void
set_menu_BM(char *BM)
{
    if (HAS_PERM(PERM_ALLBOARD) || is_BM(BM)) {
	currmode |= MODE_MENU;
	cuser.userlevel |= PERM_SYSSUBOP;
    }
}

static char    *privateboard =
"\n\n\n\n         �藍�_ ���O�ثe�u��ݪO�n�Ͷi�J  �Х��V�O�D�ӽФJ�ҳ\\�i";

static void
choose_board(int newflag)
{
    static int      num = 0;
    boardstat_t    *ptr;
    int             head = -1, ch = 0, currmodetmp, tmp, tmp1, bidtmp;
    char            keyword[13] = "";

    setutmpmode(newflag ? READNEW : READBRD);
    if( fav == NULL )
	load_brdbuf();
    ++choose_board_depth;
    brdnum = 0;
    if (!cuser.userlevel)	/* guest yank all boards */
	yank_flag = 2;

    do {
	if (brdnum <= 0) {
	    load_boards(keyword);
	    if (brdnum <= 0) {
		if (keyword[0] != 0) {
		    mprints(b_lines - 1, 0, "�S������ݪO���D��������r "
			    "(�O�D���`�N�ݪO���D�R�W)");
		    pressanykey();
		    keyword[0] = 0;
		    brdnum = -1;
		    continue;
		}
		if (yank_flag < 2) {
		    brdnum = -1;
		    yank_flag++;
		    continue;
		}
		if (HAS_PERM(PERM_SYSOP) || (currmode & MODE_MENU)) {
		    if (m_newbrd(0) == -1)
			break;
		    brdnum = -1;
		    continue;
		} else
		    break;
	    }
	    head = -1;
	}
	if (num < 0)
	    num = 0;
	else if (num >= brdnum)
	    num = brdnum - 1;

	if (head < 0) {
	    if (newflag) {
		tmp = num;
		while (num < brdnum) {
		    ptr = &nbrd[num];
		    if (ptr->myattr & BRD_UNREAD)
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
	if (class_bid == 1)
	    ch = cursor_key(7 + num - head, 10);
	else
	    ch = cursor_key(3 + num - head, 0);

	switch (ch) {
	case Ctrl('W'):
	    whereami(0, NULL, NULL);
	    head = -1;
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
	case 'c':
	    show_brdlist(head, 1, newflag ^= 1);
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
	case Ctrl('C'):
	    cal();
	    show_brdlist(head, 1, newflag);
	    break;
	case Ctrl('I'):
	    t_idle();
	    show_brdlist(head, 1, newflag);
	    break;
	case KEY_UP:
	case 'p':
	case 'k':
	    if (num-- <= 0)
		num = brdnum - 1;
	    break;
	case 't':
	    ptr = &nbrd[num];
	    setfav(ptr->bid, BRD_TAG, 2, 0);
	    ptr->myattr ^= BRD_TAG;
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
	case 'F':
	case 'f':
	    if (class_bid>0 && HAS_PERM(PERM_SYSOP)) {
		bcache[class_bid - 1].firstchild[cuser.uflag & BRDSORT_FLAG ? 1 : 0]
		    = NULL;
		brdnum = -1;
	    }
	    break;
	case 'h':
	    show_help(choosebrdhelp);
	    show_brdlist(head, 1, newflag);
	    break;
	case '/':
	    getdata_buf(b_lines - 1, 0, "�п�J�ݪO��������r:",
			keyword, sizeof(keyword), DOECHO);
	    brdnum = -1;
	    break;
	case 'S':
	    if(yank_flag == 0){
		char    input[4];
		move(b_lines - 2, 0);
		prints("���s�ƧǬݪO "
			"\033[1;33m(�`�N, �o�Ӱʧ@�|�мg��ӳ]�w)\033[m \n");
		getdata(b_lines - 1, 0,
			"�ƧǤ覡 (1)���ӪO�W�Ƨ� (2)�������O�Ƨ� ==> [0]���� ",
			input, sizeof(input), DOECHO);
		if( input[0] == '1' )
		    qsort(&fav->b, fav->nDatas, sizeof(fav_board_t), 
			    favcmpboardname);
		else if( input[0] == '2' )
		    qsort(&fav->b, fav->nDatas, sizeof(fav_board_t), 
			    favcmpboardclass);
	    }
	    else
		cuser.uflag ^= BRDSORT_FLAG;
	    favchange = 1;
	    brdnum = -1;
	    break;
	case 'y':
	    yank_flag = (yank_flag + 1) % 2;
	    brdnum = -1;
	    break;
	case Ctrl('D'):
	    for (tmp = 0; tmp < fav->nDatas; tmp++) {
		if (fav->b[tmp].attr & BRD_TAG) {
		    favchange = 1;
		    fav->b[tmp].attr &= ~BRD_FAV;
		    fav->b[tmp].attr &= ~BRD_TAG;
		}
	    }
	    brdnum = -1;
	    break;
	case Ctrl('A'):
	    for (tmp = 0; tmp < fav->nDatas; tmp++) {
		if (fav->b[tmp].attr & BRD_TAG) {
		    favchange = 1;
		    fav->b[tmp].attr |= BRD_FAV;
		    fav->b[tmp].attr &= ~BRD_TAG;
		}
	    }
	    brdnum = -1;
	    break;
	case Ctrl('T'):
	    for (tmp = 0; tmp < fav->nDatas; tmp++)
		fav->b[tmp].attr &= ~BRD_TAG;
	    favchange = 1;
	    brdnum = -1;
	    break;
	case Ctrl('P'):
	    if (class_bid != 0 &&
		(HAS_PERM(PERM_SYSOP) || (currmode & MODE_MENU))) {
		for (tmp = 0; tmp < fav->nDatas; tmp++) {
		    short   bid = fav->b[tmp].bid;
		    boardheader_t  *bh = &bcache[ bid - 1 ];
		    /*
		    if (!(fav->b[tmp].attr & BRD_TAG) || bh->gid == class_bid)
			continue;
		    */
		    if( !(fav->b[tmp].attr & BRD_TAG) )
			continue;
		    favchange = 1;
		    fav->b[tmp].attr &= ~BRD_TAG;
		    if (bh->gid != class_bid) {
			bh->gid = class_bid;
			substitute_record(FN_BOARD, bh,
					  sizeof(boardheader_t), bid);
			reset_board(bid);
			log_usies("SetBoardGID", bh->brdname);
		    }
		}
		brdnum = -1;
	    }
	    break;
	case 'L':
	    if (HAS_PERM(PERM_BASIC)) {
		if(fav->nDatas > FAVMAX || fav->nLines <= -127){
		    vmsg("�A���̷R�Ӧh�F�� �u���");
		    break;
		}
		setfav(0, BRD_FAV | BRD_LINE, 1, 0);
		nbrd[brdnum].bid = fav->nLines;
		movefav(brdnum, num);
		brdnum = -1;
		head = 9999;
	    }
	    break;
	case 'm':
	    if (HAS_PERM(PERM_BASIC)) {
		if(nbrd[num].myattr & BRD_LINE){
		    delfavline(nbrd[num].bid, num);
		    brdnum = -1;
		}
		else if(!(nbrd[num].myattr & BRD_FAV) && (fav->nDatas > FAVMAX)){
		    vmsg("�A���̷R�Ӧh�F�� �u���");
		    break;
		}
		else{
		    setfav(nbrd[num].bid, BRD_FAV, 2, 0);
		    nbrd[num].myattr ^= BRD_FAV;
		}
		head = 9999;
	    }
	    break;
	case 'M':
	    if (HAS_PERM(PERM_BASIC) && class_bid == 0 && yank_flag == 0){
		imovefav(num);
    		head = 9999;
	    }
	    break;
	case 'K':
	    if (HAS_PERM(PERM_BASIC)) {
		char c, fname[80], genbuf[256];
		int fd;
		c = getans("�п�� 1)�M�����i���ݪO 2)�ƥ��ڪ��̷R 3)���^�̷R�ƥ� [Q]");
		if(!c)
		    break;
		if(getans("�T�w�� [y/N] ") != 'y')
		    break;
		switch(c){
		    case '1':
			favclean(fav);
			break;
		    case '2':
			setuserfile(fname, FAV3);
			sprintf(genbuf, "cp -f %s %s.bak", fname, fname);
			system(genbuf);
			break;
		    case '3':
			setuserfile(fname, FAV3);
			sprintf(genbuf, "%s.bak", fname);
			if((fd = open(genbuf, O_RDONLY)) < 0){
			    vmsg("�A�S���ƥ��A���̷R��");
			    break;
			}
			close(fd);
			sprintf(genbuf, "cp -f %s.bak %s", fname, fname);
			system(genbuf);
			freefav(fav);
			load_brdbuf();
			favchange = 1;
			break;
		}
		brdnum = -1;
	    }
	    break;
	case 'z':
	    if (HAS_PERM(PERM_BASIC))
		vmsg("�K�K �o�ӥ\\��w�g�Q�ڪ��̷R���N���F��!");
	    break;
	case 'Z':
	    if (!HAS_PERM(PERM_BASIC))
		break;
	    cuser.uflag2 ^= FAVNEW_FLAG;
	    if(cuser.uflag2 & FAVNEW_FLAG){
		char fname[80];

		setuserfile(fname, FAVNB);

		if( (tmp = open(fname, O_RDONLY, 0600)) != -1 ){
		    close(tmp);
		    updatenewfav(0);
		}
		else{
		    char stat;
		    if( (tmp = open(fname, O_WRONLY | O_CREAT, 0600)) != -1 ){
			for(tmp1 = 0; tmp1 < numboards; tmp1++){
			    if(bcache[tmp1].brdname[0] && Ben_Perm(&bcache[tmp1]))
				stat = BRD_OLD;
			    else
				stat = BRD_NEW;
			    write(tmp, &stat, sizeof(char));
			}
			stat = BRD_END;
			write(tmp, &stat, sizeof(char));
			close(tmp);
		    }
		}
	    }
	    vmsg((cuser.uflag2 & FAVNEW_FLAG) ? "�������q�\\�s�ݪO�Ҧ�" : "���������`�Ҧ�");
	    break;
	case 'v':
	case 'V':
	    if(nbrd[num].bid < 0)
		break;
	    ptr = &nbrd[num];
	    brc_initial(B_BH(ptr)->brdname);
	    if (ch == 'v') {
		ptr->myattr &= ~BRD_UNREAD;
		brc_list[0] = now;
		setfav(ptr->bid, 0, 0, now);
	    } else {
		brc_list[0] = 1;
		setfav(ptr->bid, 0, 0, 1);
		ptr->myattr |= BRD_UNREAD;
	    }
	    brc_num = brc_changed = 1;
	    brc_update();
	    show_brdlist(head, 0, newflag);
	    break;
	case 's':
	    if ((tmp = search_board()) == -1) {
		show_brdlist(head, 1, newflag);
		break;
	    }
	    num = tmp;
	    break;
	case 'E':
	    if (HAS_PERM(PERM_SYSOP) || (currmode & MODE_MENU)) {
		ptr = &nbrd[num];
		move(1, 1);
		clrtobot();
		m_mod_board(B_BH(ptr)->brdname);
		brdnum = -1;
	    }
	    break;
	case 'R':
	    if (HAS_PERM(PERM_SYSOP) || (currmode & MODE_MENU)) {
		m_newbrd(1);
		brdnum = -1;
	    }
	    break;
	case 'B':
	    if (HAS_PERM(PERM_SYSOP) || (currmode & MODE_MENU)) {
		m_newbrd(0);
		brdnum = -1;
	    }
	    break;
	case 'W':
	    if (class_bid > 0 &&
		(HAS_PERM(PERM_SYSOP) || (currmode & MODE_MENU))) {
		char            buf[128];
		setbpath(buf, bcache[class_bid - 1].brdname);
		mkdir(buf, 0755);	/* Ptt:�}�s�եؿ� */
		b_note_edit_bname(class_bid);
		brdnum = -1;
	    }
	    break;
	case KEY_RIGHT:
	case '\n':
	case '\r':
	case 'r':
	    {
		char            buf[STRLEN];

		ptr = &nbrd[num];
		if(ptr->myattr & BRD_LINE)
		    break;

		if (!(B_BH(ptr)->brdattr & BRD_GROUPBOARD)) {	/* �Dsub class */
		    if (!(B_BH(ptr)->brdattr & BRD_HIDE) ||
			(B_BH(ptr)->brdattr & BRD_POSTMASK)) {
			brc_initial(B_BH(ptr)->brdname);

			if (newflag) {
			    setbdir(buf, currboard);
			    tmp = unread_position(buf, ptr);
			    head = tmp - t_lines / 2;
			    getkeep(buf, head > 1 ? head : 1, tmp + 1);
			}
			board_visit_time = getfavtime(ptr->bid);
			Read();
			check_newpost(ptr);
			head = -1;
			setutmpmode(newflag ? READNEW : READBRD);
		    } else {
			setbfile(buf, B_BH(ptr)->brdname, FN_APPLICATION);
			if (more(buf, YEA) == -1) {
			    move(1, 0);
			    clrtobot();
			    outs(privateboard);
			    pressanykey();
			}
			head = -1;
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

		    if (!(currmode & MODE_MENU))	/* �p�G�٨S���p�ժ��v�� */
			set_menu_BM(B_BH(ptr)->BM);

		    if (now < B_BH(ptr)->bupdate) {
			setbfile(buf, B_BH(ptr)->brdname, fn_notes);
			if (more(buf, NA) != -1)
			    pressanykey();
		    }
		    tmp = currutmp->brc_id;
		    setutmpbid(ptr->bid);
		    free(nbrd);
		    nbrd = NULL;
			if (yank_flag == 0 ) {
			    yank_flag = 1;
			    choose_board(0);
			    yank_flag = 0;
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
	}
    } while (ch != 'q');
    free(nbrd);
    nbrd = NULL;
    --choose_board_depth;
}

int
root_board()
{
    class_bid = 1;
    yank_flag = 1;
    choose_board(0);
    return 0;
}

int
Boards()
{
    class_bid = 0;
    yank_flag = 0;
    choose_board(0);
    return 0;
}


int
New()
{
    int             mode0 = currutmp->mode;
    int             stat0 = currstat;

    class_bid = 0;
    choose_board(1);
    currutmp->mode = mode0;
    currstat = stat0;
    return 0;
}
