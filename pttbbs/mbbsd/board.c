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

	if( num == 0 ) return ptr;

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

static char * const fn_boardrc = ".boardrc";

static inline void
brc_insert_record(const char* board, int num, int* list)
{
    char            *ptr, *endp, *tmpp = 0;
    char            tmp_buf[BRC_ITEMSIZE];
    char            tmp_name[BRC_STRLEN];
    int             tmp_list[BRC_MAXNUM];
    int             new_size, end_size, tmp_num;
    int             found = 0;

    ptr = brc_buf;
    endp = &brc_buf[brc_size];
    while (ptr < endp && (*ptr >= ' ' && *ptr <= 'z')) {
	/* for each available records */
	tmpp = brc_getrecord(ptr, tmp_name, &tmp_num, tmp_list);

	if ( tmpp > endp ){
	    /* dangling, ignore the trailing data */
	    brc_size = (int)(ptr - brc_buf);
	    break;
	}
	if ( strncmp(tmp_name, board, BRC_STRLEN) == 0 ){
	    found = 1;
	    break;
	}
	ptr = tmpp;
    }

    if( ! found ){
	/* put on the beginning */
	ptr = brc_putrecord(tmp_buf, board, num, list);
	new_size = (int)(ptr - tmp_buf);
	if( new_size ){
	    brc_size += new_size;
	    if ( brc_size > BRC_MAXSIZE )
		brc_size = BRC_MAXSIZE;
	    memmove(brc_buf + new_size, brc_buf, brc_size);
	    memmove(brc_buf, tmp_buf, new_size);
	}
    }else{
	/* ptr points to the old current brc list.
	 * tmpp is the end of it (exclusive).       */
	end_size = endp - tmpp;
	new_size = (int)(brc_putrecord(tmp_buf, board, num, list) - tmp_buf);
	if( new_size ){
	    brc_size += new_size - (tmpp - ptr);
	    if ( brc_size > BRC_MAXSIZE ){
		end_size -= brc_size - BRC_MAXSIZE;
		brc_size = BRC_MAXSIZE;
	    }
	    if ( end_size > 0 && ptr + new_size != tmpp )
		memmove(ptr + new_size, tmpp, end_size);
	    memmove(ptr, tmp_buf, new_size);
	}else
	    memmove(ptr, tmpp, brc_size - (tmpp - brc_buf));
    }

    brc_changed = 0;
}

void
brc_update(){
    if (brc_changed && cuser.userlevel && brc_num > 0)
	brc_insert_record(currboard, brc_num, brc_list);
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

void
brc_finalize(){
    char brcfile[STRLEN];
    int fd;
    brc_update();
    setuserfile(brcfile, fn_boardrc);
    if ((fd = open(brcfile, O_WRONLY | O_CREAT | O_TRUNC, 0644)) != -1) {
	write(fd, brc_buf, brc_size);
	close(fd);
    }
}

int
brc_read_record(const char* bname, int* num, int* list){
    char            *ptr;
    char            tmp_name[BRC_STRLEN];
    ptr = brc_buf;
    while (ptr < &brc_buf[brc_size] && (*ptr >= ' ' && *ptr <= 'z')) {
	/* for each available records */
	ptr = brc_getrecord(ptr, tmp_name, num, list);
	if (strncmp(tmp_name, bname, BRC_STRLEN) == 0)
	    return *num;
    }
    *num = list[0] = 1;
    return 0;
}

int
brc_initial(const char *boardname)
{
    if (strcmp(currboard, boardname) == 0) {
	return brc_num;
    }
    brc_update(); /* write back first */
    strlcpy(currboard, boardname, sizeof(currboard));
    currbid = getbnum(currboard);
    currbrdattr = bcache[currbid - 1].brdattr;

    return brc_read_record(boardname, &brc_num, brc_list);
}

static void
brc_trunc(const char* brdname, int ftime){
    brc_insert_record(brdname, 1, &ftime);
    if (strncmp(brdname, currboard, BRC_STRLEN) == 0){
	brc_num = 1;
	brc_list[0] = ftime;
	brc_changed = 0;
    }
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

#define BRD_NULL	0
#define BRD_FAV    	1
#define BRD_BOARD	2
#define BRD_LINE   	4
#define BRD_FOLDER	8
#define BRD_TAG        16
#define BRD_UNREAD     32


#define B_TOTAL(bptr)        (SHM->total[(bptr)->bid - 1])
#define B_LASTPOSTTIME(bptr) (SHM->lastposttime[(bptr)->bid - 1])
#define B_BH(bptr)           (&bcache[(bptr)->bid - 1])
typedef struct {
    int             bid;
    unsigned char   myattr;
} __attribute__ ((packed)) boardstat_t;

static boardstat_t *nbrd = NULL;
static char	choose_board_depth = 0;
static short    brdnum;
static char     yank_flag = 1;

void imovefav(int old)
{
    char buf[5];
    int new;
    
    getdata(b_lines - 1, 0, "請輸入新次序:", buf, sizeof(buf), DOECHO);
    new = atoi(buf) - 1;
    if (new < 0 || brdnum <= new){
	vmsg("輸入範圍有誤!");
	return;
    }
    move_in_current_folder(old, new);
}

void load_brdbuf(void)
{
    static  char    firsttime = 1;

    char    buf[128];
    setuserfile(buf, FAV4);
    if( !dashf(buf) ) {
	fav_v3_to_v4();
    }
    fav_load();
    updatenewfav(1);
    firsttime = 0;
}

void
init_brdbuf()
{
    brc_expire_time = login_start_time - 365 * 86400;
    read_brc_buf();
}

void
save_brdbuf(void)
{
    fav_save();
    fav_free();
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

    /* 祕密看板：核對首席板主的好友名單 */

    if (brdattr & BRD_HIDE) {	/* 隱藏 */
	if (hbflcheck((int)(bptr - bcache) + 1, currutmp->uid)) {
	    if (brdattr & BRD_POSTMASK)
		return 0;
	    else
		return 2;
	} else
	    return 1;
    }
    /* 限制閱讀權限 */
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
	     "正在搜尋作者[33m%s[m 看板:[1;33m%s[0m.....",
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
{				/* Ptt 改 */
    int             tbrc_list[BRC_MAXNUM], tbrc_num;
    time_t          ftime;

    ptr->myattr &= ~BRD_UNREAD;
    if (B_BH(ptr)->brdattr & BRD_GROUPBOARD)
	return 0;

    if (B_TOTAL(ptr) == 0)
	setbtotal(ptr->bid);
    if (B_TOTAL(ptr) == 0)
	return 0;
    ftime = B_LASTPOSTTIME(ptr);
    if (ftime > now)
	ftime = B_LASTPOSTTIME(ptr) = now - 1;

    if ( brc_read_record(B_BH(ptr)->brdname, &tbrc_num, tbrc_list) == 0 ||
	 brc_unread_time(ftime, tbrc_num, tbrc_list) )
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
    ptr->myattr = state;
    if ((B_BH(ptr)->brdattr & BRD_HIDE) && state == BRD_BOARD)
	B_BH(ptr)->brdattr |= BRD_POSTMASK;
    if (yank_flag != 0)
	ptr->myattr &= ~BRD_FAV;
    check_newpost(ptr);
    return ptr;
}

static int
cmpboardfriends(const void *brd, const void *tmp)
{
    return ((B_BH((boardstat_t*)tmp)->nuser) -
	    (B_BH((boardstat_t*)brd)->nuser));
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
	    fav_t *fav = get_current_fav();

	    nbrd = (boardstat_t *)malloc(sizeof(boardstat_t) * get_data_number(fav));
            for( i = 0 ; i < fav->DataTail; ++i ){
		int state;
		if (!(fav->favh[i].attr & FAVH_FAV))
		    continue;

		if ( !key[0] ){
		    if (get_item_type(&fav->favh[i]) == FAVT_LINE )
			state = BRD_LINE;
		    else if (get_item_type(&fav->favh[i]) == FAVT_FOLDER )
			state = BRD_FOLDER;
		    else {
			bptr = &bcache[ fav_getid(&fav->favh[i]) - 1];
			if( yank_flag == 0 )
			    state = BRD_BOARD;
			else
			    continue;
			if (is_set_attr(&fav->favh[i], FAVH_UNREAD))
			    state |= BRD_UNREAD;
		    }
		} else {
		    if (get_item_type(&fav->favh[i]) == FAVT_LINE )
			continue;
		    else if (get_item_type(&fav->favh[i]) == FAVT_FOLDER ){
			if( strcasestr(
			    get_folder_title(fav_getid(&fav->favh[i])),
			    key)
			)
			    state = BRD_FOLDER;
			else
			    continue;
		    }else{
			bptr = &bcache[ fav_getid(&fav->favh[i]) - 1];
			if( Ben_Perm(bptr) && strcasestr(bptr->title, key))
			    state = BRD_BOARD;
			else
			    continue;
			if (is_set_attr(&fav->favh[i], FAVH_UNREAD))
			    state |= BRD_UNREAD;
		    }
		}

		if (is_set_attr(&fav->favh[i], FAVH_TAG))
		    state |= BRD_TAG;
		if (is_set_attr(&fav->favh[i], FAVH_ADM_TAG))
		    state |= BRD_TAG;
		addnewbrdstat(fav_getid(&fav->favh[i]) - 1, BRD_FAV | state);
	    }
	    if (brdnum == 0)
		addnewbrdstat(0, 0);
	    byMALLOC = 0;
	    needREALLOC = (get_data_number(fav) != brdnum);
	}
#if HOTBOARDCACHE
	else if( class_bid == -1 ){
	    nbrd = (boardstat_t *)malloc(sizeof(boardstat_t) * SHM->nHOTs);
	    for( i = 0 ; i < SHM->nHOTs ; ++i )
		addnewbrdstat(SHM->HBcache[i] - SHM->bcache,
			      Ben_Perm(SHM->HBcache[i]));
	}
#endif
	else { // general case
	    nbrd = (boardstat_t *) MALLOC(sizeof(boardstat_t) * numboards);
	    for (i = 0; i < numboards; i++) {
		if ((bptr = SHM->bsorted[type][i]) == NULL)
		    continue;
		n = (int)(bptr - bcache);
		if (!bptr->brdname[0] ||
		    (bptr->brdattr & BRD_GROUPBOARD) ||
		    !((state = Ben_Perm(bptr)) || (currmode & MODE_MENU)) ||
		    (key[0] && !strcasestr(bptr->title, key))
#ifndef HOTBOARDCACHE
		    || (class_bid == -1 && bptr->nuser < 5)
#endif
		    )
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
#ifndef HOTBOARDCACHE
	if (class_bid == -1)
	    qsort(nbrd, brdnum, sizeof(boardstat_t), cmpboardfriends);
#endif
    } else { /* load boards of a subclass */
	int     childcount = bptr->childcount;
	nbrd = (boardstat_t *) malloc(childcount * sizeof(boardstat_t));
	for (bptr = bptr->firstchild[type]; bptr != NULL;
	     bptr = bptr->next[type]) {
	    n = (int)(bptr - bcache);
	    if (!((state = Ben_Perm(bptr)) || (currmode & MODE_MENU))
		|| (yank_flag == 0 && !(getbrdattr(n) & BRD_FAV)) ||
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
	if (yank_flag != 0 ||
	    (nbrd[num].myattr & BRD_BOARD && Ben_Perm(B_BH(&nbrd[num]))) )
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

static char
get_fav_type(boardstat_t *ptr)
{
    if (ptr->myattr & BRD_FOLDER)
	return FAVT_FOLDER;
    else if (ptr->myattr & BRD_BOARD)
	return FAVT_BOARD;
    else if (ptr->myattr & BRD_LINE)
	return FAVT_LINE;
    return 0;
}

static void
brdlist_foot()
{
    prints("\033[34;46m  選擇看板  \033[31;47m  (c)\033[30m新文章模式  "
	   "\033[31m(v/V)\033[30m標記已讀/未讀  \033[31m(y)\033[30m篩選%s"
	   "  \033[31m(m)\033[30m切換最愛  \033[m",
	   yank_flag == 0 ? "最愛" : yank_flag == 1 ? "部份" : "全部");
}

static void
show_brdlist(int head, int clsflag, int newflag)
{
    int             myrow = 2;
    if (class_bid == 1) {
	currstat = CLASS;
	myrow = 6;
	showtitle("分類看板", BBSName);
	movie(0);
	move(1, 0);
	outs(
	    "                                                              "
	    "◣  ╭—\033[33m●\n"
	    "                                                    �寣X  \033[m "
	    "◢█\033[47m☉\033[40m██◣�蔌n"
	    "  \033[44m   ︿︿︿︿︿︿︿︿                               "
	    "\033[33m�鱋033[m\033[44m ◣◢███▼▼▼�� \033[m\n"
	    "  \033[44m                                                  "
	    "\033[33m  \033[m\033[44m ◤◥███▲▲▲ �鱋033[m\n"
	    "                                  ︿︿︿︿︿︿︿︿    \033[33m"
	    "│\033[m   ◥████◤ �鱋n"
	    "                                                      \033[33m��"
	    "——\033[m  ◤      —＋\033[m");
    } else if (clsflag) {
	showtitle("看板列表", BBSName);
	prints("[←]主選單 [→]閱\讀 [↑↓]選擇 [y]載入 [S]排序 [/]搜尋 "
	       "[TAB]文摘•看板 [h]求助\n"
	       "\033[7m%-20s 類別 轉信%-31s人氣 板    主     \033[m",
	       newflag ? "總數 未讀 看  板" : "  編號  看  板",
	       "  中   文   敘   述");
	move(b_lines, 0);
	brdlist_foot();
    }
    if (brdnum > 0) {
	boardstat_t    *ptr;
	char    *color[8] = {"", "\033[32m",
	    "\033[33m", "\033[36m", "\033[34m", "\033[1m",
	"\033[1;32m", "\033[1;33m"};
	char    *unread[2] = {"\33[37m  \033[m", "\033[1;31mˇ\033[m"};

	if (yank_flag == 0 && get_fav_type(&nbrd[0]) == 0){
	    move(3, 0);
	    prints("        --- 空目錄 ---");
	    return;
	}

	while (++myrow < b_lines) {
	    move(myrow, 0);
	    clrtoeol();
	    if (head < brdnum) {
		ptr = &nbrd[head++];
		if (ptr->myattr & BRD_LINE){
		    if( !newflag )
			prints("%5d %c %s------------      ------------------------------------------\033[m",
				head,
				ptr->myattr & BRD_TAG ? 'D' : ' ',
				ptr->myattr & BRD_FAV ? "" : "\033[1;30m");
		    else
			prints("        %s------------      ------------------------------------------\033[m", ptr->myattr & BRD_FAV ? "" : "\033[1;30m");
		    continue;
		}
		else if (ptr->myattr & BRD_FOLDER){
		    char *title = get_folder_title(ptr->bid);
		    if( !newflag )
			prints("%5d %c %sMyFavFolder\033[m  目錄 □%-34s\033[m",
				head,
				ptr->myattr & BRD_TAG ? 'D' : ' ',
				!(cuser.uflag2 & FAVNOHILIGHT) ? "\033[1;36m" : "",
				title);
		    else
			prints("%6d  %sMyFavFolder\033[m  目錄 □%-34s\033[m",
				get_data_number(get_fav_folder(getfolder(ptr->bid))),
				!(cuser.uflag2 & FAVNOHILIGHT) ? "\033[1;36m" : "",
				title);
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
			   unread[ptr->myattr & BRD_UNREAD ? 1 : 0]);
		} else {
		    if (B_BH(ptr)->brdattr & BRD_GROUPBOARD)
			prints("        ");
		    else
			prints("%6d%s", (int)(B_TOTAL(ptr)),
				unread[ptr->myattr & BRD_UNREAD ? 1 : 0]);
		}
		if (class_bid != 1) {
		    if (!(currmode & MODE_MENU) && !Ben_Perm(B_BH(ptr))) {
			prints("Unknown??    隱板 ？這個板是隱板");
		    }
		    else {
			prints("%s%-13s\033[m%s%5.5s\033[0;37m%2.2s\033[m"
				"%-34.34s",
				((!(cuser.uflag2 & FAVNOHILIGHT) &&
				  getboard(ptr->bid) != NULL))? "\033[1;36m" : "",
				B_BH(ptr)->brdname,
				color[(unsigned int)
				(B_BH(ptr)->title[1] + B_BH(ptr)->title[2] +
				 B_BH(ptr)->title[3] + B_BH(ptr)->title[0]) & 07],
				B_BH(ptr)->title, B_BH(ptr)->title + 5, B_BH(ptr)->title + 7);

			if (B_BH(ptr)->brdattr & BRD_BAD)
			    prints(" X ");
			else if (B_BH(ptr)->nuser >= 5000)
			    prints("\033[1;34m爆!\033[m");
			else if (B_BH(ptr)->nuser >= 2000)
			    prints("\033[1;31m爆!\033[m");
			else if (B_BH(ptr)->nuser >= 1000)
			    prints("\033[1m爆!\033[m");
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
		    }
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
    "\0看板選單輔助說明",
    "\01基本指令",
    "(p)(↑)/(n)(↓)上一個看板 / 下一個看板",
    "(P)(^B)(PgUp)  上一頁看板",
    "(N)(^F)(PgDn)  下一頁看板",
    "($)/(s)/(/)    最後一個看板 / 搜尋看板 / 以中文搜尋看板關鍵字",
    "(數字)         跳至該項目",
    "\01進階指令",
    "(^W)           迷路了 我在哪裡",
    "(r)(→)/(q)(←)進入多功\能閱\讀選單 / 回到主選單",
    "(y/Z)          我的最愛,訂閱\看板,所有看板 / 訂閱\新開看板",
    "(L/g)          加入分隔線 / 目錄至我的最愛",
    "(K/T)          備份,清理我的最愛 / 修改我的最愛目錄名稱",
    "(v/V)          通通看完/全部未讀",
    "(S)            按照字母/分類排序",
    "(t/^T/^A/^D)   標記看板/取消所有標記/ 將已標記者加入/移出我的最愛",
    "(m/M)          把看板加入或移出我的最愛,移除分隔線/搬移我的最愛",
    "\01小組長指令",
    "(E/W/B)        設定看板/設定小組備忘/開新看板",
    "(^P)           移動已標記看板到此分類",
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


static void
choose_board(int newflag)
{
    static short    num = 0;
    boardstat_t    *ptr;
    int             head = -1, ch = 0, currmodetmp, tmp, tmp1, bidtmp;
    char            keyword[13] = "";

    setutmpmode(newflag ? READNEW : READBRD);
    if( get_current_fav() == NULL )
	load_brdbuf();
    ++choose_board_depth;
    brdnum = 0;
    if (!cuser.userlevel)	/* guest yank all boards */
	yank_flag = 2;

    do {
	if (brdnum <= 0) {
	    load_boards(keyword);
	    if (brdnum <= 0 && yank_flag > 0) {
		if (keyword[0] != 0) {
		    mprints(b_lines - 1, 0, "沒有任何看板標題有此關鍵字 "
			    "(板主應注意看板標題命名)");
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

	/* reset the cursor when out of range */
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
	    if (yank_flag == 0 && get_fav_type(&nbrd[0]) != 0) {
		fav_tag(ptr->bid, get_fav_type(ptr), 2);
	    }
	    else if (yank_flag != 0) {
		/* 站長管理用的 tag */
		if (ptr->myattr & BRD_TAG)
		    set_attr(getadmtag(ptr->bid), FAVH_ADM_TAG, 0);
		else
		    fav_add_admtag(ptr->bid);
	    }
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
	    getdata_buf(b_lines - 1, 0, "請輸入看板中文關鍵字:",
			keyword, sizeof(keyword), DOECHO);
	    brdnum = -1;
	    break;
	case 'S':
	    if(yank_flag == 0){
		char    input[4];
		move(b_lines - 2, 0);
		prints("重新排序看板 "
			"\033[1;33m(注意, 這個動作會覆寫原來設定)\033[m \n");
		getdata(b_lines - 1, 0,
			"排序方式 (1)按照板名排序 (2)按照類別排序 ==> [0]取消 ",
			input, sizeof(input), DOECHO);
		if( input[0] == '1' )
		    fav_sort_by_name();
		else if( input[0] == '2' )
		    fav_sort_by_class();
	    }
	    else
		cuser.uflag ^= BRDSORT_FLAG;
	    brdnum = -1;
	    break;
	case 'y':
	    if (get_current_fav() != NULL || yank_flag != 0)
	    yank_flag = (yank_flag + 1) % 2;
	    brdnum = -1;
	    break;
	case Ctrl('D'):
	    fav_remove_all_tagged_item();
	    brdnum = -1;
	    break;
	case Ctrl('A'):
	    fav_add_all_tagged_item();
	    brdnum = -1;
	    break;
	case Ctrl('T'):
	    fav_remove_all_tag();
	    brdnum = -1;
	    break;
	case Ctrl('P'):
	    if (class_bid != 0 &&
		(HAS_PERM(PERM_SYSOP) || (currmode & MODE_MENU))) {
		fav_t *fav = get_current_fav();
		for (tmp = 0; tmp < fav->DataTail; tmp++) {
		    short   bid = fav_getid(&fav->favh[tmp]);
		    boardheader_t  *bh = &bcache[ bid - 1 ];
		    if( !is_set_attr(&fav->favh[tmp], FAVH_ADM_TAG))
			continue;
		    set_attr(&fav->favh[tmp], FAVH_ADM_TAG, 0);
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
		if (fav_add_line() == NULL) {
		    vmsg("新增失敗，分隔線/總最愛 數量達最大值。");
		    break;
		}
		/* done move if it's the first item. */
		if (get_fav_type(&nbrd[0]) != 0)
		    move_in_current_folder(brdnum, num);
		brdnum = -1;
		head = 9999;
	    }
	    break;
	case 'm':
	    if (HAS_PERM(PERM_BASIC)) {
		ptr = &nbrd[num];
		if (yank_flag == 0) {
		    if (ptr->myattr & BRD_FAV) {
			if (getans("你確定刪除嗎? [N/y]") != 'y')
			    break;
			fav_remove_item(ptr->bid, get_fav_type(ptr));
			ptr->myattr &= ~BRD_FAV;
		    }
		}
		else {
		    if (getboard(ptr->bid) != NULL) {
			fav_remove_item(ptr->bid, FAVT_BOARD);
			ptr->myattr &= ~BRD_FAV;
		    }
		    else {
			if (fav_add_board(ptr->bid) == NULL)
			    vmsg("你的最愛太多了啦 真花心");
			else
			    ptr->myattr |= BRD_FAV;
		    }
		}
		brdnum = -1;
		head = 9999;
	    }
	    break;
	case 'M':
	    if (HAS_PERM(PERM_BASIC)){
		if (class_bid == 0 && yank_flag == 0){
		    imovefav(num);
		    brdnum = -1;
		    head = 9999;
		}
	    }
	    break;
	case 'g':
	    if (HAS_PERM(PERM_BASIC) && yank_flag == 0) {
		fav_type_t  *ft;
		if (fav_stack_full()){
		    vmsg("目錄已達最大層數!!");
		    break;
		}
		if ((ft = fav_add_folder()) == NULL) {
		    vmsg("新增失敗，目錄/總最愛 數量達最大值。");
		    break;
		}
		fav_set_folder_title(ft, "新的目錄");
		/* don't move if it's the first item */
		if (get_fav_type(&nbrd[0]) != 0)
		    move_in_current_folder(brdnum, num);
		brdnum = -1;
    		head = 9999;
	    }
	    break;
	case 'T':
	    if (HAS_PERM(PERM_BASIC) && nbrd[num].myattr & BRD_FOLDER) {
		char title[64];
		fav_type_t *ft = getfolder(nbrd[num].bid);
		strlcpy(title, get_item_title(ft), sizeof(title));
		getdata_buf(b_lines - 1, 0, "請輸入檔名:", title, sizeof(title), DOECHO);
		fav_set_folder_title(ft, title);
		brdnum = -1;
	    }
	    break;
	case 'K':
	    if (HAS_PERM(PERM_BASIC)) {
		char c, fname[80], genbuf[256];
		if (!current_fav_at_root()) {
		    vmsg("請到我的最愛最上層執行本功\能");
		    break;
		}

		c = getans("請選擇 1)清除不可見看板 2)備份我的最愛 3)取回最愛備份 [Q]");
		if(!c)
		    break;
		if(getans("確定嗎 [y/N] ") != 'y')
		    break;
		switch(c){
		    case '1':
			fav_clean_invisible();
			break;
		    case '2':
			fav_save();
			setuserfile(fname, FAV4);
			sprintf(genbuf, "%s.bak", fname);
                        Copy(fname, genbuf);
			break;
		    case '3':
			setuserfile(fname, FAV4);
			sprintf(genbuf, "%s.bak", fname);
			if (!dashf(genbuf)){
			    vmsg("你沒有備份你的最愛喔");
			    break;
			}
                        Copy(genbuf, fname);
			fav_free();
			load_brdbuf();
			break;
		}
		brdnum = -1;
	    }
	    break;
	case 'z':
	    if (HAS_PERM(PERM_BASIC))
		vmsg("嘿嘿 這個功\能已經被我的最愛取代掉了喔!");
	    break;
#ifdef DEBUG
	case 'A':
	    if (1) {
		char genbuf[200];
		sprintf(genbuf, "brdnum: %d  num: %d", brdnum, num);
		vmsg(genbuf);
	    }
	    break;
#endif
	case 'Z':
	    if (HAS_PERM(PERM_BASIC)) {
		char genbuf[256];
		sprintf(genbuf, "確定要 %s訂閱\ 新看板? [N/y] ", cuser.uflag2 & FAVNEW_FLAG ? "取消" : "");
		if (getans(genbuf) != 'y')
		    break;

		cuser.uflag2 ^= FAVNEW_FLAG;
		if (cuser.uflag2 & FAVNEW_FLAG) {
		    subscribe_newfav();
		    vmsg("切換為訂閱\新看板模式");
		}
		else
		    vmsg("取消訂閱\新看板");
	    }
	    break;
	case 'v':
	case 'V':
	    ptr = &nbrd[num];
	    if(nbrd[num].bid < 0 || !Ben_Perm(B_BH(ptr)))
		break;
	    if (ch == 'v') {
		ptr->myattr &= ~BRD_UNREAD;
		brc_trunc(B_BH(ptr)->brdname, now);
		setbrdtime(ptr->bid, now);
	    } else {
		brc_trunc(B_BH(ptr)->brdname, 1);
		setbrdtime(ptr->bid, 1);
		ptr->myattr |= BRD_UNREAD;
	    }
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
		mkdir(buf, 0755);	/* Ptt:開群組目錄 */
		b_note_edit_bname(class_bid);
		brdnum = -1;
	    }
	    break;

#ifdef DEBUG
	case 'w':
	    brc_finalize();
	    break;
#endif /* defined(DEBUG) */

	case KEY_RIGHT:
	case '\n':
	case '\r':
	case 'r':
	    {
		char            buf[STRLEN];

		ptr = &nbrd[num];
		if (yank_flag == 0 && get_fav_type(&nbrd[0]) == 0)
		    break;
		else if (ptr->myattr & BRD_LINE)
		    break;
		else if (ptr->myattr & BRD_FOLDER){
		    int t = num;
		    num = 0;
		    fav_folder_in(ptr->bid);
		    choose_board(0);
		    fav_folder_out();
		    num = t;
		    brdnum = -1;
		    head = 9999;
		    break;
		}

		if (!(B_BH(ptr)->brdattr & BRD_GROUPBOARD)) {	/* 非sub class */
		    if (Ben_Perm(B_BH(ptr))) {
			brc_initial(B_BH(ptr)->brdname);

			if (newflag) {
			    setbdir(buf, currboard);
			    tmp = unread_position(buf, ptr);
			    head = tmp - t_lines / 2;
			    getkeep(buf, head > 1 ? head : 1, tmp + 1);
			}
			board_visit_time = getbrdtime(ptr->bid);
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
			class_bid = -1;	/* 熱門群組用 */

		    if (!(currmode & MODE_MENU))	/* 如果還沒有小組長權限 */
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
		    currmode = currmodetmp;	/* 離開板板後就把權限拿掉喔 */
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
