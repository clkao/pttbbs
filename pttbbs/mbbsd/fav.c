/* $Id$ */
#include "bbs.h"

/**
 * Structure
 * =========
 * fav4 ���D�n�[�c�p�U�G
 * 
 *   fav_t - �ΨӸ˦U�� entry(fav_type_t) �� directory
 *     �i�J�ڪ��̷R�ɡA�ݨ쪺�F��N�O�ھ� fav_t �ͥX�Ӫ��C
 *     �̭������̡A�o�@�� level �����h�֭ӬݪO�B�ؿ��B���j�u�C(favh)
 *     �O�@�� array (with pre-allocated buffer)
 * 
 *   fav_type_t - fav entry �� base class
 *     �s���ɳz�L type �ܼƨӱo�����T�����A�C
 * 
 *   fav_board_t / fav_line_t / fav_folder_t - derived class
 *     �Բӱ��νаѦ� fav.h �����w�q�C
 *     �H cast_(board|line|folder)_t �ӱN�@�� fav_type_t �@�ʺA�૬�C
 * 
 * Policy
 * ======
 * ���F�קK�L�ת���Ʒh���A��N�@�� item �q�ڪ��̷R�������ɡA�u�N�L��
 * FAVH_FAV flag �����C�ӨS���o�� flag �� item �]���Q�����ڪ��̷R�C
 * 
 * �ڪ��̷R���A�S�] FAVH_FAV ����ơA�N�b�Y�ǮɭԡA�p�g�J�ɮ׮ɡA�I�s
 * rebuild_fav �M�����b�C
 * 
 * Others
 * ======
 * �����h���ݪO�ҥΪ� t �A�]������u�s�b nbrd �̭��A�S���M�A�˥X�B�~���Ŷ��A
 * �ҥH�������b�ڪ��̷R���F t �A�|��o�ӰO���Ȧs�b fav ��
 * (FAVH_ADM_TAG == 1, FAVH_FAV == 0)�C
 */

#ifdef MEM_CHECK
static int	memcheck;
#endif

/* the total number of items, every level. */
static int 	fav_number;

/* definition of fav stack, the top one is in use now. */
static int	fav_stack_num = 0;
static fav_t   *fav_stack[FAV_MAXDEPTH] = {0};

/* fav_tmp is for recordinge while copying, moving, etc. */
static fav_t   *fav_tmp;
//static int	fav_tmp_snum; /* the sequence number in favh in fav_t */


/**
 * cast_(board|line|folder) �@�ڥΩ�N base class �@�૬
 * (���ˬd��� data type)
 */
inline static fav_board_t *cast_board(fav_type_t *p){
    return (fav_board_t *)p->fp;
}

inline static fav_line_t *cast_line(fav_type_t *p){
    return (fav_line_t *)p->fp;
}

inline static fav_folder_t *cast_folder(fav_type_t *p){
    return (fav_folder_t *)p->fp;
}

/**
 * �Ǧ^���w�� fp(dir) ���� fp->DataTail, �Ĥ@�ӨS�ιL����m�� index
 */
inline static int get_data_tail(fav_t *fp){
    return fp->DataTail;
}

/**
 * �Ǧ^���w dir ���ҥΪ� entry ���`�� (�u��u���b�̭��A�Ӥ���w�Q������)
 */
inline int get_data_number(fav_t *fp){
    return fp->nBoards + fp->nLines + fp->nFolders;
}

/**
 * �Ǧ^�ثe�Ҧb�� dir pointer
 */
inline fav_t *get_current_fav(void){
    if (fav_stack_num == 0)
	return NULL;
    return fav_stack[fav_stack_num - 1];
}

/**
 * �N ft(entry) cast ���@�� dir
 */
inline fav_t *get_fav_folder(fav_type_t *ft){
    return cast_folder(ft)->this_folder;
}

inline int get_item_type(fav_type_t *ft){
    return ft->type;
}

/**
 * �N�@�ӫ��w�� dir pointer �s�U�ӡA����i�� fav_get_tmp_fav �Ӧs��
 */
inline static void fav_set_tmp_folder(fav_t *fp){
    fav_tmp = fp;
}

inline static fav_t *fav_get_tmp_fav(void){
    return fav_tmp;
}

/**
 * �N fp(dir) �O���ƶq���A�����@��� ft(entry)
 */
static void fav_decrease(fav_t *fp, fav_type_t *ft){
    switch (get_item_type(ft)){
	case FAVT_BOARD:
	    fp->nBoards--;
	    break;
	case FAVT_LINE:
	    fp->nLines--;
	    break;
	case FAVT_FOLDER:
	    fp->nFolders--;
	    break;
    }
    fav_number--;
}

/**
 * �N fp(dir) �O���ƶq���A�W�[�@��� ft(entry)
 */
static void fav_increase(fav_t *fp, fav_type_t *ft)
{
    switch (get_item_type(ft)){
	case FAVT_BOARD:
	    fp->nBoards++;
	    break;
	case FAVT_LINE:
	    fp->nLines++;
	    cast_line(ft)->lid = ++fp->lineID;
	    break;
	case FAVT_FOLDER:
	    fp->nFolders++;
	    cast_folder(ft)->fid = ++fp->folderID;
	    break;
    }
    fav_number++;
    fp->DataTail++;
}

inline static int get_folder_num(fav_t *fp) {
    return fp->nFolders;
}

/**
 * get_(folder|line)_id �Ǧ^ fp ���@�ӷs�� folder/line id
 */
inline static int get_folder_id(fav_t *fp) {
    return fp->folderID;
}

inline static int get_line_id(fav_t *fp) {
    return fp->lineID;
}

inline static int get_line_num(fav_t *fp) {
    return fp->nLines;
}

/**
 * �]�w�Y�� flag�C
 * @bit: �ثe�Ҧ� flags ��: FAVH_FAV, FAVH_TAG, FAVH_UNREAD, FAVH_ADM_TAG
 * @param bool: FALSE: unset, TRUE: set, EXCH: opposite
 */
void set_attr(fav_type_t *ft, int bit, char bool){
    if (bool == EXCH)
	ft->attr ^= bit;
    else if (bool == TRUE)
	ft->attr |= bit;
    else
	ft->attr &= ~bit;
}

inline int is_set_attr(fav_type_t *ft, char bit){
    return ft->attr & bit;
}

char *get_item_title(fav_type_t *ft)
{
    switch (get_item_type(ft)){
	case FAVT_BOARD:
	    return bcache[cast_board(ft)->bid - 1].brdname;
	case FAVT_FOLDER:
	    return cast_folder(ft)->title;
	case FAVT_LINE:
	    return "----";
    }
    return NULL;
}

static char *get_item_class(fav_type_t *ft)
{
    switch (get_item_type(ft)){
	case FAVT_BOARD:
	    return bcache[cast_board(ft)->bid - 1].title;
	case FAVT_FOLDER:
	    return "�ؿ�";
	case FAVT_LINE:
	    return "----";
    }
    return NULL;
}

#ifdef MEM_CHECK
inline static void fav_set_memcheck(int n) {
    memcheck = n;
}

inline static int fav_memcheck(void) {
    return memcheck;
}
#endif
/* ---*/

static int get_type_size(int type)
{
    switch (type){
	case FAVT_BOARD:
	    return sizeof(fav_board_t);
	case FAVT_FOLDER:
	    return sizeof(fav_folder_t);
	case FAVT_LINE:
	    return sizeof(fav_line_t);
    }
    return 0;
}

inline static void* fav_malloc(int size){
    void *p = (void *)malloc(size);
    assert(p);
    memset(p, 0, size);
    return p;
}

/**
 * allocate header(fav_type_t, base class) �� fav_*_t (derived class)
 * @return �s allocate ���Ŷ�
 */
static fav_type_t *fav_item_allocate(int type)
{
    int size = 0;
    fav_type_t *ft = (fav_type_t *)fav_malloc(sizeof(fav_type_t));

    size = get_type_size(type);
    if (size) {
	ft->fp = fav_malloc(size);
	ft->type = type;
    }
    return ft;
}

/**
 * �u�ƻs fav_type_t
 */
inline static void
fav_item_copy(fav_type_t *target, const fav_type_t *source){
    target->type = source->type;
    target->attr = source->attr;
    target->fp = source->fp;
}

inline fav_t *get_fav_root(void){
    return fav_stack[0];
}

/**
 * �O�_�����Ī� entry
 */
inline int valid_item(fav_type_t *ft){
    return ft->attr & FAVH_FAV;
}

/**
 * �M�� fp(dir) ���L�Ī� entry/dir�A�p�G clean_invisible == true�A�� user
 * �ݤ������ݪO�]�|�Q�M���C
 */
static void rebuild_fav(fav_t *fp, int clean_invisible)
{
    int i, j, nData;
    boardheader_t *bp;
    fav_type_t *ft;

    fav_number = 0;
    fp->lineID = fp->folderID = 0;
    fp->nLines = fp->nFolders = fp->nBoards = 0;
    nData = fp->DataTail;

    for (i = 0, j = 0; i < nData; i++){
	if (!(fp->favh[i].attr & FAVH_FAV))
	    continue;

	ft = &fp->favh[i];
	switch (get_item_type(ft)){
	    case FAVT_BOARD:
		bp = &bcache[cast_board(ft)->bid - 1];
		if (!bp->brdname[0])
		    continue;
		if ( clean_invisible && !HasPerm(bp))
		    continue;
		break;
	    case FAVT_LINE:
		break;
	    case FAVT_FOLDER:
		rebuild_fav(get_fav_folder(&fp->favh[i]), clean_invisible);
		break;
	    default:
		continue;
	}
	fav_increase(fp, &fp->favh[i]);
	if (i != j)
	    fav_item_copy(&fp->favh[j], &fp->favh[i]);
	j++;
    }
    fp->DataTail = get_data_number(fp);
}

inline void fav_cleanup(void)
{
    rebuild_fav(get_fav_root(), 0);
}

void fav_clean_invisible(void)
{
    rebuild_fav(get_fav_root(), 1);
}

/* sort the fav */
static int favcmp_by_name(const void *a, const void *b)
{
    return strcasecmp(get_item_title((fav_type_t *)a), get_item_title((fav_type_t *)b));
}

void fav_sort_by_name(void)
{
    rebuild_fav(get_current_fav(), 0);
    qsort(get_current_fav()->favh, get_data_number(get_current_fav()), sizeof(fav_type_t), favcmp_by_name);
}

static int favcmp_by_class(const void *a, const void *b)
{
    fav_type_t *f1, *f2;
    int cmp;

    f1 = (fav_type_t *)a;
    f2 = (fav_type_t *)b;
    if (get_item_type(f1) == FAVT_FOLDER)
	return -1;
    if (get_item_type(f2) == FAVT_FOLDER)
	return 1;
    if (get_item_type(f1) == FAVT_LINE)
	return 1;
    if (get_item_type(f2) == FAVT_LINE)
	return -1;

    cmp = strncasecmp(get_item_class(f1), get_item_class(f2), 4);
    if (cmp)
	return cmp;
    return strcasecmp(get_item_title(f1), get_item_title(f2));
}

void fav_sort_by_class(void)
{
    rebuild_fav(get_current_fav(), 0);
    qsort(get_current_fav()->favh, get_data_number(get_current_fav()), sizeof(fav_type_t), favcmp_by_class);
}

/**
 * The following is the movement operations in the user interface.
 */

/**
 * �ؿ��h�ƬO�_�F��̤j�� FAV_MAXDEPTH
 */
inline int fav_stack_full(void){
    return fav_stack_num >= FAV_MAXDEPTH;
}

inline static int fav_stack_push_fav(fav_t *fp){
    if (fav_stack_full())
	return -1;
    fav_stack[fav_stack_num++] = fp;
    return 0;
}

inline static int fav_stack_push(fav_type_t *ft){
//    if (ft->type != FAVT_FOLDER)
//	return -1;
    return fav_stack_push_fav(get_fav_folder(ft));
}

inline static void fav_stack_pop(void){
    fav_stack[--fav_stack_num] = NULL;
}

void fav_folder_in(short fid)
{
    fav_type_t *tmp = getfolder(fid);
    if (get_item_type(tmp) == FAVT_FOLDER){
	fav_stack_push(tmp);
    }
}

void fav_folder_out(void)
{
    fav_stack_pop();
}

static void read_favrec(int fd, fav_t *fp)
{
    int i;
    fav_type_t *ft;
    read(fd, &fp->nBoards, sizeof(fp->nBoards));
    read(fd, &fp->nLines, sizeof(fp->nLines));
    read(fd, &fp->nFolders, sizeof(fp->nFolders));
    fp->DataTail = get_data_number(fp);
    fp->nAllocs = fp->DataTail + FAV_PRE_ALLOC;
    fp->lineID = fp->folderID = 0;
    fp->favh = (fav_type_t *)fav_malloc(sizeof(fav_type_t) * fp->nAllocs);

    for(i = 0; i < fp->DataTail; i++){
	read(fd, &fp->favh[i].type, sizeof(fp->favh[i].type));
	read(fd, &fp->favh[i].attr, sizeof(fp->favh[i].attr));
	fp->favh[i].fp = (void *)fav_malloc(get_type_size(fp->favh[i].type));
	read(fd, fp->favh[i].fp, get_type_size(fp->favh[i].type));
    }

    for(i = 0; i < fp->DataTail; i++){
	ft = &fp->favh[i];
	switch (ft->type) {
	    case FAVT_FOLDER: {
		fav_t *p = (fav_t *)fav_malloc(sizeof(fav_t));
		read_favrec(fd, p);
		cast_folder(ft)->this_folder = p;
		cast_folder(ft)->fid = ++(fp->folderID);
	    	break;
	    }
	    case FAVT_LINE:
		cast_line(ft)->lid = ++(fp->lineID);
		break;
	}
    }
}

/**
 * �q�O���ɤ� load �X�ڪ��̷R�C
 */
int fav_load(void)
{
    int fd;
    char buf[128];
    fav_t *fp;
    if (fav_stack_num > 0)
	return -1;
    setuserfile(buf, FAV4);

    if (!dashf(buf)) {
	fp = (fav_t *)fav_malloc(sizeof(fav_t));
	fav_stack_push_fav(fp);
#ifdef MEM_CHECK
	fav_set_memcheck(MEM_CHECK);
#endif
	return 0;
    }

    if ((fd = open(buf, O_RDONLY)) < 0)
	return -1;
    fp = (fav_t *)fav_malloc(sizeof(fav_t));
    read_favrec(fd, fp);
    fav_stack_push_fav(fp);
    close(fd);
#ifdef MEM_CHECK
    fav_set_memcheck(MEM_CHECK);
#endif
    return 0;
}

/* write to the rec file */
static void write_favrec(int fd, fav_t *fp)
{
    int i;
    if (fp == NULL)
	return;
    write(fd, &fp->nBoards, sizeof(fp->nBoards));
    write(fd, &fp->nLines, sizeof(fp->nLines));
    write(fd, &fp->nFolders, sizeof(fp->nFolders));
    fp->DataTail = get_data_number(fp);
    
    for(i = 0; i < fp->DataTail; i++){
	write(fd, &fp->favh[i].type, sizeof(fp->favh[i].type));
	write(fd, &fp->favh[i].attr, sizeof(fp->favh[i].attr));
	write(fd, fp->favh[i].fp, get_type_size(fp->favh[i].type));
    }
    
    for(i = 0; i < fp->DataTail; i++){
	if (fp->favh[i].type == FAVT_FOLDER)
	    write_favrec(fd, get_fav_folder(&fp->favh[i]));
    }
}

/**
 * ��O���� save �i�ڪ��̷R�C
 * @note fav_cleanup() �|���Q�I�s�C
 */
int fav_save(void)
{
    int fd;
    char buf[64], buf2[64];
    fav_t *fp = get_fav_root();
#ifdef MEM_CHECK
    if (fav_memcheck() != MEM_CHECK)
	return -1;
#endif
    if (fp == NULL)
	return -1;
    fav_cleanup();
    setuserfile(buf, FAV4".tmp");
    setuserfile(buf2, FAV4);
    fd = open(buf, O_CREAT | O_WRONLY, 0600);
    if (fd < 0)
	return -1;
    write_favrec(fd, fp);
    close(fd);

    Rename(buf, buf2);
    return 0;
}

/**
 * remove ft (�]�� invalid�A��ڤW�|���� save �ɤ~�M��)
 */
static inline void fav_free_item(fav_type_t *ft)
{
    set_attr(ft, 0xFFFF, FALSE);
}

/**
 * delete ft from fp
 */
static int fav_remove(fav_t *fp, fav_type_t *ft)
{
    fav_free_item(ft);
    fav_decrease(fp, ft);
    return 0;
}

/**
 * free the mem of fp recursively
 */
static void fav_free_branch(fav_t *fp)
{
    int i;
    fav_type_t *ft;
    if (fp == NULL)
	return;
    for(i = 0; i < fp->DataTail; i++){
	ft = &fp->favh[i];
	switch(get_item_type(ft)){
	    case FAVT_FOLDER:
		fav_free_branch(cast_folder(ft)->this_folder);
		break;
	    case FAVT_BOARD:
	    case FAVT_LINE:
		if (ft->fp)
		    free(ft->fp);
		break;
	}
	fav_remove(fp, ft);
    }
    free(fp->favh);
    free(fp);
    fp = NULL;
}

/**
 * free the mem of the whole fav tree recursively
 */
void fav_free(void)
{
    fav_free_branch(get_fav_root());

    /* reset the stack */
    fav_stack_num = 0;
}

/**
 * �q�ثe�� dir ����X�S�w���O (type)�Bid �� id �� entry�C
 * �䤣��Ǧ^ NULL
 */
static fav_type_t *get_fav_item(short id, int type)
{
    int i;
    fav_type_t *ft;
    fav_t *fp = get_current_fav();

    for(i = 0; i < fp->DataTail; i++){
	ft = &fp->favh[i];
	if (!valid_item(ft) || get_item_type(ft) != type)
	    continue;
	if (fav_getid(ft) == id)
	    return ft;
    }
    return NULL;
}

/**
 * �q�ثe�� dir �� remove �S�w���O (type)�Bid �� id �� entry�C
 */
void fav_remove_item(short id, char type)
{
    fav_remove(get_current_fav(), get_fav_item(id, type));
}

/**
 * get*(bid) �Ǧ^�ثe�� dir �������O id == bid �� entry�C
 */
fav_type_t *getadmtag(short bid)
{
    int i;
    fav_t *fp = get_fav_root();
    fav_type_t *ft;
    for (i = 0; i < fp->DataTail; i++) {
	ft = &fp->favh[i];
	if (cast_board(ft)->bid == bid && is_set_attr(ft, FAVH_ADM_TAG))
	    return ft;
    }
    return NULL;
}

fav_type_t *getboard(short bid)
{
    return get_fav_item(bid, FAVT_BOARD);
}

fav_type_t *getfolder(short fid)
{
    return get_fav_item(fid, FAVT_FOLDER);
}

char *get_folder_title(int fid)
{
    return get_item_title(get_fav_item(fid, FAVT_FOLDER));
}


char getbrdattr(short bid)
{
    fav_type_t *fb = getboard(bid);
    if (!fb)
	return 0;
    return fb->attr;
}

time_t getbrdtime(short bid)
{
    fav_type_t *fb = getboard(bid);
    if (!fb)
	return 0;
    return cast_board(fb)->lastvisit;
}

void setbrdtime(short bid, time_t t)
{
    fav_type_t *fb = getboard(bid);
    if (fb)
	cast_board(fb)->lastvisit = t;
}

int fav_getid(fav_type_t *ft)
{
    switch(get_item_type(ft)){
	case FAVT_FOLDER:
	    return cast_folder(ft)->fid;
	case FAVT_LINE:
	    return cast_line(ft)->lid;
	case FAVT_BOARD:
	    return cast_board(ft)->bid;
    }
    return -1;
}

inline static int is_maxsize(void){
    return fav_number >= MAX_FAV;
}

/**
 * �C���@�� dir ���ɡA�o�� function �|�N buffer �j�p�դj FAV_PRE_ALLOC �ӡA
 * ����W������C
 */
static int enlarge_if_full(fav_t *fp)
{
    fav_type_t * p;
    /* enlarge the volume if need. */
    if (is_maxsize())
	return -1;
    if (fp->DataTail < fp->nAllocs)
	return 1;

    /* realloc and clean the tail */
    p = (fav_type_t *)realloc(fp->favh, sizeof(fav_type_t) * (fp->nAllocs + FAV_PRE_ALLOC));
    if( p == NULL )
	return -1;

    fp->favh = p;
    memset(fp->favh + fp->nAllocs, 0, sizeof(fav_type_t) * FAV_PRE_ALLOC);
    fp->nAllocs += FAV_PRE_ALLOC;
    return 0;
}

/**
 * �N�@�ӷs�� entry item �[�J fp ��
 */
static int fav_add(fav_t *fp, fav_type_t *item)
{
    if (enlarge_if_full(fp) < 0)
	return -1;
    fav_item_copy(&fp->favh[fp->DataTail], item);
    fav_increase(fp, item);
    return 0;
}

static void move_in_folder(fav_t *fav, int src, int dst)
{
    int i, count;
    fav_type_t tmp;

    /* Find real locations of src and dst in fav->favh[] */
    for(count = i = 0; count <= src; i++)
	if (valid_item(&fav->favh[i]))
	    count++;
    src = i - 1;
    for(count = i = 0; count <= dst; i++)
	if (valid_item(&fav->favh[i]))
	    count++;
    dst = i - 1;

    fav_item_copy(&tmp, &fav->favh[src]);

    if (src < dst) {
	for(i = src; i < dst; i++)
	    fav_item_copy(&fav->favh[i], &fav->favh[i + 1]);
    }
    else { // dst < src
	for(i = src; i > dst; i--)
	    fav_item_copy(&fav->favh[i], &fav->favh[i - 1]);
    }
    fav_item_copy(&fav->favh[dst], &tmp);
}

/**
 * �N�ثe�ؿ����� src �� entry ���� dst�C
 * @note src/dst �O user ��ڤW�ݨ쪺��m�A�]�N�O���]�t invalid entry�C
 */
void move_in_current_folder(int from, int to)
{
    move_in_folder(get_current_fav(), from, to);
}

/**
 * the following defines the interface of add new fav_XXX
 */

/**
 * allocate �@�� folder entry
 */
inline static fav_t *alloc_folder_item(void){
    fav_t *fp = (fav_t *)fav_malloc(sizeof(fav_t));
    fp->nAllocs = FAV_PRE_ALLOC;
    fp->favh = (fav_type_t *)fav_malloc(sizeof(fav_type_t) * FAV_PRE_ALLOC);
    return fp;
}

static fav_type_t *init_add(fav_t *fp, int type)
{
    fav_type_t *ft;
    if (is_maxsize())
	return NULL;
    ft = fav_item_allocate(type);
    set_attr(ft, FAVH_FAV, TRUE);
    fav_add(fp, ft);
    return ft;
}

/**
 * �s�W�@���j�u
 * @return �[�J�� entry ����
 */
fav_type_t *fav_add_line(void)
{
    fav_t *fp = get_current_fav();
    if (get_line_num(fp) >= MAX_LINE)
	return NULL;
    return init_add(fp, FAVT_LINE);
}

/**
 * �s�W�@�ؿ�
 * @return �[�J�� entry ����
 */
fav_type_t *fav_add_folder(void)
{
    fav_t *fp = get_current_fav();
    fav_type_t *ft;
    if (fav_stack_full())
	return NULL;
    if (get_folder_num(fp) >= MAX_FOLDER)
	return NULL;
    ft = init_add(fp, FAVT_FOLDER);
    if (ft == NULL)
	return NULL;
    cast_folder(ft)->this_folder = alloc_folder_item();
    return ft;
}

/**
 * �N���w�ݪO�[�J�ثe���ؿ��C
 * @return �[�J�� entry ����
 * @note �����\�P�@�ӪO�Q�[�J�⦸
 */
fav_type_t *fav_add_board(int bid)
{
    fav_t *fp = get_current_fav();
    fav_type_t *ft = getboard(bid); 
    if (ft != NULL)
	return ft;
    ft = init_add(fp, FAVT_BOARD);
    if (ft == NULL)
	return NULL;
    cast_board(ft)->bid = bid;
    return ft;
}

/**
 * for administrator to move/administrate board
 */
fav_type_t *fav_add_admtag(int bid)
{   
    fav_t *fp = get_fav_root();
    fav_type_t *ft;
    if (is_maxsize())
	return NULL;
    ft = fav_item_allocate(FAVT_BOARD);
    if (ft == NULL)
	return NULL; 
    // turn on  FAVH_ADM_TAG
    set_attr(ft, FAVH_ADM_TAG, TRUE);
    fav_add(fp, ft);
    cast_board(ft)->bid = bid;
    return ft; 
}   


/* everything about the tag in fav mode.
 * I think we don't have to implement the function 'cross-folder' tag.*/

/**
 * �N�ثe�ؿ��U�A�� type & id ���w�� entry �ФW/���� tag
 * @param bool �P set_attr
 * @note �Y�P�@�ӥؿ��������P�˪��F��A�u���Ĥ@�ӷ|�@�ΡC
 */
void fav_tag(short id, char type, char bool) {
    fav_type_t *ft = get_fav_item(id, type);
    if (ft != NULL)
	set_attr(ft, FAVH_TAG, bool);
}

static void fav_dosomething_tagged_item(fav_t *fp, int (*act)(fav_t *, fav_type_t *))
{
    int i;
    for(i = 0; i < fp->DataTail; i++){
	if (is_set_attr(&fp->favh[i], FAVH_FAV) && is_set_attr(&fp->favh[i], FAVH_TAG))
	    if ((*act)(fp, &fp->favh[i]) < 0)
		break;
    }
}

static int remove_tagged_item(fav_t *fp, fav_type_t *ft)
{
    int i;
    for(i = 0; i < FAV_MAXDEPTH && fav_stack[i] != NULL; i++)
	if (fav_stack[i] == get_fav_folder(ft)){
	    set_attr(ft, FAVH_TAG, FALSE);
	    return 0;
	}
    return fav_remove(fp, ft);
}

inline static int fav_remove_tagged_item(fav_t *fp){
    fav_dosomething_tagged_item(fp, remove_tagged_item);
    return 0;
}

/* add an item into a fav_t.
 * here we must give the line and foler a new id to prevent an old one is exist.
 */
static int add_and_remove_tag(fav_t *fp, fav_type_t *ft)
{
    int i;
    fav_type_t *tmp;
    for(i = 0; i < FAV_MAXDEPTH && fav_stack[i] != NULL; i++)
	if (fav_stack[i] == get_fav_folder(ft)){
	    set_attr(ft, FAVH_TAG, FALSE);
	    return 0;
	}
    tmp = fav_malloc(sizeof(fav_type_t));
    fav_item_copy(tmp, ft);
    /* since fav_item_copy is symbolic link, we disable removed link to
     * prevent double-free when doing fav_clean(). */
    ft->fp = NULL;
    set_attr(tmp, FAVH_TAG, FALSE);

    if (fav_add(fav_get_tmp_fav(), tmp) < 0)
	return -1;
    fav_remove(fp, ft);
    return 0;
}

inline static int fav_add_tagged_item(fav_t *fp){
    if (fp == fav_get_tmp_fav())
	return -1;
    fav_dosomething_tagged_item(fp, add_and_remove_tag);
    return 0;
}

static void fav_do_recursively(fav_t *fp, int (*act)(fav_t *))
{
    int i;
    fav_type_t *ft;
    for(i = 0; i < fp->DataTail; i++){
	ft = &fp->favh[i];
	if (!valid_item(ft))
	    continue;
	if (get_item_type(ft) == FAVT_FOLDER && get_fav_folder(ft) != NULL){
	    fav_do_recursively(get_fav_folder(ft), act);
	}
    }
    (*act)(fp);
}

static void fav_dosomething_all_tagged_item(int (*act)(fav_t *))
{
    fav_do_recursively(get_fav_root(), act);
}

/**
 * fav_*_all_tagged_item �b��ӧڪ��̷R�W��w�ФW tag �� entry ���Y��ơC
 */
void fav_remove_all_tagged_item(void)
{
    fav_dosomething_all_tagged_item(fav_remove_tagged_item);
}

void fav_add_all_tagged_item(void)
{
    fav_set_tmp_folder(get_current_fav());
    fav_dosomething_all_tagged_item(fav_add_tagged_item);
}

inline static int remove_tag(fav_t *fp, fav_type_t  *ft)
{
    set_attr(ft, FAVH_TAG, FALSE);
    return 0;
}

inline static int remove_tags(fav_t *fp)
{
    fav_dosomething_tagged_item(fp, remove_tag);
    return 0;
}

/**
 * �����ڪ��̷R�Ҧ��� tags
 */
void fav_remove_all_tag(void)
{
    fav_dosomething_all_tagged_item(remove_tags);
}

/**
 * �]�w folder ������W��
 */
void fav_set_folder_title(fav_type_t *ft, char *title)
{
    if (get_item_type(ft) != FAVT_FOLDER)
	return;
    strlcpy(cast_folder(ft)->title, title, sizeof(cast_folder(ft)->title));
}

#define BRD_OLD 0
#define BRD_NEW 1
#define BRD_END 2
/**
 * �p�G user �}�� FAVNEW_FLAG ���\��:
 *  mode == 1: update �ݪO�A�ñN�s�ݪO�[�J�ڪ��̷R�C
 *  mode == 0: update ��T�����[�J�C
 */
void updatenewfav(int mode)
{
    /* mode: 0: don't write to fav  1: write to fav */
    int i, fd, brdnum;
    char fname[80], *brd;

    if(!(cuser.uflag2 & FAVNEW_FLAG))
	return;

    setuserfile(fname, FAVNB);

    if( (fd = open(fname, O_RDWR | O_CREAT, 0600)) != -1 ){

	brdnum = numboards; /* avoid race */

	brd = (char *)malloc((brdnum + 1) * sizeof(char));
	memset(brd, 0, (brdnum + 1) * sizeof(char));

	i = read(fd, brd, (brdnum + 1) * sizeof(char));
	if (i < 0) {
	    vmsg("favorite subscription error");
	    return;
	}

	/* if it's a new file, no BRD_END is in it. */
	brd[i] = BRD_END;
	
	for(i = 0; i < brdnum + 1 && brd[i] != BRD_END; i++){
	    if(brd[i] == BRD_NEW){
		/* check the permission if the board exsits */
		if(bcache[i].brdname[0] && HasPerm(&bcache[i])){
		    if(mode && !(bcache[i].brdattr & BRD_SYMBOLIC))
			fav_add_board(i + 1);
		    brd[i] = BRD_OLD;
		}
	    }
	    else{
		if(!bcache[i].brdname[0])
		    brd[i] = BRD_NEW;
	    }
	}
	if( i < brdnum) // the board number may change
	    for(i-- ; i < brdnum; i++){
		if(bcache[i].brdname[0] && HasPerm(&bcache[i])){
		    if(mode && !(bcache[i].brdattr & BRD_SYMBOLIC))
			fav_add_board(i + 1);
		    brd[i] = BRD_OLD;
		}
		else
		    brd[i] = BRD_NEW;
	    }

	brd[i] = BRD_END;
	
	lseek(fd, 0, SEEK_SET);
	write(fd, brd, (brdnum + 1 ) * sizeof(char));
	free(brd);
	close(fd);
    }
}

void subscribe_newfav(void)
{
    updatenewfav(0);
}

#ifdef NOT_NECESSARY_NOW
/** backward compatible **/

/* old struct */
#define BRD_UNREAD      1
#define BRD_FAV         2
#define BRD_LINE        4
#define BRD_TAG         8 
#define BRD_GRP_HEADER 16

typedef struct {
  short           bid;
  char            attr;
  time_t          lastvisit;
} fav3_board_t;

typedef struct {
    short           nDatas;
    short           nAllocs;
    char            nLines;
    fav_board_t    *b;
} fav3_t;

int fav_v3_to_v4(void)
{
    int i, fd, fdw;
    char buf[128];
    short nDatas;
    char nLines;
    fav_t *fav4;
    fav3_board_t *brd;

    setuserfile(buf, FAV3);
    if (!dashf(buf))
	return -1;

    setuserfile(buf, FAV4);
    if (dashf(buf))
	return 0;
    fdw = open(buf, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fdw < 0)
	return -1;
    setuserfile(buf, FAV3);
    fd = open(buf, O_RDONLY);
    if (fd < 0) {
	close(fdw);
	return -1;
    }

    fav4 = (fav_t *)fav_malloc(sizeof(fav_t));

    read(fd, &nDatas, sizeof(nDatas));
    read(fd, &nLines, sizeof(nLines));

    fav4->DataTail = nDatas;
    //fav4->nBoards = nDatas - (-nLines);
    //fav4->nLines = -nLines;
    fav4->nBoards = fav4->nFolders = fav4->nLines = 0;
    fav4->favh = (fav_type_t *)fav_malloc(sizeof(fav_type_t) * fav4->DataTail);

    brd = (fav3_board_t *)fav_malloc(sizeof(fav3_board_t) * nDatas);
    read(fd, brd, sizeof(fav3_board_t) * nDatas);

    for(i = 0; i < fav4->DataTail; i++){
	fav4->favh[i].type = brd[i].attr & BRD_LINE ? FAVT_LINE : FAVT_BOARD;

	if (brd[i].attr & BRD_UNREAD)
	    fav4->favh[i].attr |= FAVH_UNREAD;
	if (brd[i].attr & BRD_FAV)
	    fav4->favh[i].attr |= FAVH_FAV;
	if (brd[i].attr & BRD_TAG)
	    fav4->favh[i].attr |= FAVH_TAG;

	fav4->favh[i].fp = (void *)fav_malloc(get_type_size(fav4->favh[i].type));
	if (brd[i].attr & BRD_LINE){
	    fav4->favh[i].type = FAVT_LINE;
	    cast_line(&fav4->favh[i])->lid = ++fav4->nLines;
	}
	else{
	    fav4->favh[i].type = FAVT_BOARD;
	    cast_board(&fav4->favh[i])->bid = brd[i].bid;
	    cast_board(&fav4->favh[i])->lastvisit = brd[i].lastvisit;
	    fav4->nBoards++;
	}
    }

    write_favrec(fdw, fav4);
    fav_free_branch(fav4);
    free(brd);
    close(fd);
    close(fdw);
    return 0;
}
#endif
