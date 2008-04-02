/* $Id$ */
#include "bbs.h"

// XXX piaip 2007/12/29
// �̪�o�{�ܦh code �����b announce
// �]���i�ӭn�� lastlevel �ӫD currbid
// user �i��@�i BBS �����l��->mail_cite->�i��ذ�
// ��O�N�z�� 
// �P�z currboard �]���ӥ�
// �Ч�� me.bid (�`�N me.bid �i�ର 0, ��ܶi�Ӫ��D�ݪO�C)
//
// XXX 9999 �·зQ�Ӥ覡�ﱼ

// for max file size limitation here, see edit.c
#define MAX_FILE_SIZE (32768*1024)

/* copy temp queue operation -------------------------------------- */

/* TODO
 * change this to char* instead of char[]
 */
typedef struct {
    char     copyfile[PATHLEN];
    char     copytitle[TTLEN + 1];
    char     copyowner[IDLEN + 2];
} CopyQueue ;

#define COPYQUEUE_COMMON_SIZE (10)

static CopyQueue *copyqueue;
static int allocated_copyqueue = 0, used_copyqueue = 0, head_copyqueue = 0;

int copyqueue_testin(CopyQueue *pcq)
{
    int i = 0;
    for (i = head_copyqueue; i < used_copyqueue; i++)
	if (strcmp(pcq->copyfile, copyqueue[i].copyfile) == 0)
	    return 1;
    return 0;
}

int copyqueue_locate(CopyQueue *pcq)
{
    int i = 0;
    for (i = head_copyqueue; i < used_copyqueue; i++)
	if (strcmp(pcq->copyfile, copyqueue[i].copyfile) == 0)
	    return i;
    return -1;
}

int copyqueue_fileinqueue(const char *fn)
{
    int i = 0;
    for (i = head_copyqueue; i < used_copyqueue; i++)
	if (strcmp(fn, copyqueue[i].copyfile) == 0)
	    return 1;
    return 0;
}

void copyqueue_reset()
{
    allocated_copyqueue = 0;
    used_copyqueue = 0; 
    head_copyqueue = 0;
}

int copyqueue_append(CopyQueue *pcq)
{
    if(copyqueue_testin(pcq))
	return 0;
    if(head_copyqueue == used_copyqueue) 
    {
	// empty queue, happy happy reset
	if(allocated_copyqueue > COPYQUEUE_COMMON_SIZE)
	{
	    // let's reduce it
	    allocated_copyqueue = COPYQUEUE_COMMON_SIZE;
	    copyqueue = (CopyQueue*)realloc( copyqueue,
		      allocated_copyqueue * sizeof(CopyQueue));
	}
	head_copyqueue = used_copyqueue = 0;
    }
    used_copyqueue ++;

    if(used_copyqueue > allocated_copyqueue)
    {
	allocated_copyqueue = 
	    used_copyqueue + COPYQUEUE_COMMON_SIZE; // half page
	copyqueue = (CopyQueue*) realloc (copyqueue,
		sizeof(CopyQueue) * allocated_copyqueue);
	if(!copyqueue)
	{
	    vmsg("�O���餣���A��������");
	    // try to reset
	    copyqueue_reset();
	    if(copyqueue) free(copyqueue);
	    copyqueue = NULL;
	    return 0;
	}
    }
    memcpy(&(copyqueue[used_copyqueue-1]), pcq, sizeof(CopyQueue));
    return 1;
}

int copyqueue_toggle(CopyQueue *pcq)
{
    int i = copyqueue_locate(pcq);
    if(i >= 0)
    {
#if 0
	if (getans("�w�аO�L���ɡA�n�����аO�� [y/N]: ") != 'y')
	    return 1;
#endif
	/* remove it */
	used_copyqueue --;
	if(head_copyqueue > used_copyqueue)
	    head_copyqueue =used_copyqueue;
	if (i < used_copyqueue)
	{
	    memcpy(copyqueue + i, copyqueue+i+1, 
		    sizeof(CopyQueue) * (used_copyqueue - i));
	}
	return 0;
    } else {
	copyqueue_append(pcq);
    }
    return 1;
}

CopyQueue *copyqueue_gethead()
{
    if(	used_copyqueue <= 0 ||
	head_copyqueue >= used_copyqueue)
	return NULL;
    return &(copyqueue[head_copyqueue++]);
}

int copyqueue_querysize()
{
    if(	used_copyqueue <= 0 ||
	head_copyqueue >= used_copyqueue)
	return 0;
    return (used_copyqueue - head_copyqueue);
}

/* end copy temp queue operation ----------------------------------- */

void
a_copyitem(const char *fpath, const char *title, const char *owner, int mode)
{
    CopyQueue cq;
    static int flFirstAlert = 1;

    memset(&cq, 0, sizeof(CopyQueue));
    strcpy(cq.copyfile, fpath);
    strcpy(cq.copytitle, title);
    if (owner)
	strcpy(cq.copyowner, owner);

    //copyqueue_append(&cq);
    copyqueue_toggle(&cq);
    if (mode && flFirstAlert) {
#if 0
	move(b_lines-2, 0); clrtoeol();
	prints("�ثe�w�аO %d ���ɮסC[�`�N] ������~��R�����!",
		copyqueue_querysize());
#else
	vmsg("[�`�N] �����z�ƻs/�аO��n�K�W(p)�Ϊ��[(a)��~��R�����!");
	flFirstAlert = 0;
#endif
    }
}

#define FHSZ            sizeof(fileheader_t)

static int
a_loadname(menu_t * pm)
{
    char            buf[PATHLEN];
    int             len;

    if(p_lines != pm->header_size) {
	pm->header_size = p_lines;
	pm->header = (fileheader_t *) realloc(pm->header, pm->header_size*FHSZ);
	assert(pm->header);
    }

    setadir(buf, pm->path);
    len = get_records(buf, pm->header, FHSZ, pm->page + 1, pm->header_size); // XXX if get_records() return -1

    // if len < 0, the directory is not valid anymore.
    if (len < 0)
	return 0;

    if (len < pm->header_size)
	bzero(&pm->header[len], FHSZ * (pm->header_size - len));
    return 1;
}

static void
a_timestamp(char *buf, const time4_t *time)
{
    struct tm      *pt = localtime4(time);

    sprintf(buf, "%02d/%02d/%02d", pt->tm_mon + 1, pt->tm_mday, (pt->tm_year + 1900) % 100);
}

static int
a_showmenu(menu_t * pm)
{
    char           *title, *editor;
    int             n;
    fileheader_t   *item;
    time4_t         dtime;

    showtitle("��ؤ峹", pm->mtitle);
    prints("   " ANSI_COLOR(1;36) "�s��    ��      �D%56s" ANSI_COLOR(0),
	   "�s    ��      ��    ��");

    if (!pm->num)
    {
	outs("\n  �m��ذϡn�|�b�l���Ѧa��������ؤ�... :)");
    }
    else
    {
	char buf[PATHLEN];

	// determine if path is valid.
	if (!a_loadname(pm))
	    return 0;

	for (n = 0; n < p_lines && pm->page + n < pm->num; n++) {
	    int flTagged = 0;
	    item = &pm->header[n];
	    title = item->title;
	    editor = item->owner;
	    /*
	     * Ptt ��ɶ��אּ���ɮ׮ɶ� dtime = atoi(&item->filename[2]);
	     */
	    snprintf(buf, sizeof(buf), "%s/%s", pm->path, item->filename);
	    if(copyqueue_querysize() > 0 && copyqueue_fileinqueue(buf))
	    {
		flTagged = 1;
	    }
	    dtime = dasht(buf);
	    a_timestamp(buf, &dtime);
	    prints("\n%6d%c%c%-47.46s%-13s[%s]", pm->page + n + 1,
		   (item->filemode & FILE_BM) ? 'X' :
		   (item->filemode & FILE_HIDE) ? ')' : '.',
		   flTagged ? 'c' : ' ',
		   title, editor,
		   buf);
	}
    }

    move(b_lines, 0);
    if(copyqueue_querysize() > 0)
    {		// something in queue
	prints(
	 ANSI_COLOR(37;44) "�i�w�аO(�ƻs) %d ���j"
	 ANSI_COLOR(31;47) " (c)" ANSI_COLOR(30) "�аO/�ƻs "
		    , copyqueue_querysize());

	if(pm->level == 0)
	    outs(" - �L�޲z�v���A�L�k�K�W               " ANSI_RESET);
	else
	    outs(   ANSI_COLOR(31) "(p)" ANSI_COLOR(30) "�K�W/����/���]�аO "
		    ANSI_COLOR(31) "(a)" ANSI_COLOR(30) "���[�ܤ峹��    "
		    ANSI_RESET);
    } 
    else if(pm->level)
    {		// BM
	outs(
	 ANSI_COLOR(34;46) " �i�O  �D�j "
	 ANSI_COLOR(31;47) "  (h)" ANSI_COLOR(30) "����  "
	 ANSI_COLOR(31) "(q/��)" ANSI_COLOR(30) "���}  "
	 ANSI_COLOR(31) "(n)" ANSI_COLOR(30) "�s�W�峹  "
	 ANSI_COLOR(31) "(g)" ANSI_COLOR(30) "�s�W�ؿ�  "
	 ANSI_COLOR(31) "(e)" ANSI_COLOR(30) "�s���ɮ�  " ANSI_RESET
	 );
    }
    else
    {		// normal user
	outs(
	 ANSI_COLOR(34;46) " �i�\\����j "
	 ANSI_COLOR(31;47) "  (h)" ANSI_COLOR(30) "����  "
	 ANSI_COLOR(31) "(q/��)" ANSI_COLOR(30) "���}  "
	 ANSI_COLOR(31) "(k��j��)" ANSI_COLOR(30) "���ʴ��  "
	 ANSI_COLOR(31) "(enter/��)" ANSI_COLOR(30) "Ū�����  " ANSI_RESET);
    }
    return 1;
}

static int
a_searchtitle(menu_t * pm, int rev)
{
    static char     search_str[40] = "";
    int             pos;

    getdata(b_lines - 1, 1, "[�j�M]����r:", search_str, sizeof(search_str), DOECHO);

    if (!*search_str)
	return pm->now;

    str_lower(search_str, search_str);

    rev = rev ? -1 : 1;
    pos = pm->now;
    do {
	pos += rev;
	if (pos == pm->num)
	    pos = 0;
	else if (pos < 0)
	    pos = pm->num - 1;
	if (pos < pm->page || pos >= pm->page + p_lines) {
	    pm->page = pos - pos % p_lines;

	    if (!a_loadname(pm))
		return pm->now;
	}
	if (strcasestr(pm->header[pos - pm->page].title, search_str))
	    return pos;
    } while (pos != pm->now);
    return pm->now;
}

enum {
    NOBODY, MANAGER, SYSOP
};

static void
a_showhelp(int level)
{
    clear();
    outs(ANSI_COLOR(36) "�i " BBSNAME "���G��ϥλ��� �j" ANSI_RESET "\n\n"
	 "[��][q]         ���}��W�@�h�ؿ�\n"
	 "[��][k]         �W�@�ӿﶵ\n"
	 "[��][j]         �U�@�ӿﶵ\n"
	 "[��][r][enter]  �i�J�ؿ���Ū���峹\n"
	 "[^B][PgUp]      �W�����\n"
	 "[^F][PgDn][Spc] �U�����\n"
	 "[##]            ����ӿﶵ\n"
	 "[F][U]          �N�峹�H�^ Internet �l�c/"
	 "�N�峹 uuencode ��H�^�l�c\n");
    if (level >= MANAGER) {
	outs("\n" ANSI_COLOR(36) "�i �O�D�M���� �j" ANSI_RESET "\n"
	     "[H]             ������ ���}/�|��/�O�D �~��\\Ū\n"
	     "[n/g/G]         ������ؤ峹/�}�P�ؿ�/�إ߳s�u\n"
	     "[m/d/D]         ����/�R���峹/�R���@�ӽd�򪺤峹\n"
	     "[f/T/e]         �s����D�Ÿ�/�ק�峹���D/���e\n"
	     "[c/p/a]         ��ذϤ� �аO(�ƻs)/�K�W(�i�h�g)/���[��g�峹\n"
	     "[^P/^A]         �K�W/���[��ذϥ~�w��'t'�аO�峹\n");
    }
    if (level >= SYSOP) {
	outs("\n" ANSI_COLOR(36) "�i �����M���� �j" ANSI_RESET "\n"
	     "[l]             �� symbolic link\n"
	     "[N]             �d���ɦW\n");
    }
    pressanykey();
}

static void
a_forward(const char *path, const fileheader_t * pitem, int mode)
{
    fileheader_t    fhdr;

    strlcpy(fhdr.filename, pitem->filename, sizeof(fhdr.filename));
    strlcpy(fhdr.title, pitem->title, sizeof(fhdr.title));
    switch (doforward(path, &fhdr, mode)) {
    case 0:
	outmsg(msg_fwd_ok);
	break;
    case -1:
	outmsg(msg_fwd_err1);
	break;
    case -2:
	outmsg(msg_fwd_err2);
	break;
    }
}

static void
a_additem(menu_t * pm, const fileheader_t * myheader)
{
    char            buf[PATHLEN];

    setadir(buf, pm->path);
    if (append_record(buf, myheader, FHSZ) == -1)
	return;
    pm->now = pm->num++;

    if (pm->now >= pm->page + p_lines) {
	pm->page = pm->now - ((pm->page == 10000 && pm->now > p_lines / 2) ?
			      (p_lines / 2) : (pm->now % p_lines));
    }
    /* Ptt */
    strlcpy(pm->header[pm->now - pm->page].filename,
	    myheader->filename,
	    sizeof(pm->header[pm->now - pm->page].filename));
}

#define ADDITEM         0
#define ADDGROUP        1

static void
a_newitem(menu_t * pm, int mode)
{
    char    *mesg[3] = {
	"[�s�W�峹] �п�J���D�G",	/* ADDITEM */
	"[�s�W�ؿ�] �п�J���D�G",	/* ADDGROUP */
    };

    char            fpath[PATHLEN];
    fileheader_t    item;

    strlcpy(fpath, pm->path, sizeof(fpath));
    if (strlen(pm->path) + FNLEN*2 >= PATHLEN)
	return;

    switch (mode) {
    case ADDITEM:
	stampfile(fpath, &item);
	strlcpy(item.title, "�� ", sizeof(item.title));	/* A1BA */
	break;

    case ADDGROUP:
	stampdir(fpath, &item);
	strlcpy(item.title, "�� ", sizeof(item.title));	/* A1BB */
	break;
    }

    if (!getdata(b_lines - 1, 1, mesg[mode], &item.title[3], 55, DOECHO)) {
	if (mode == ADDGROUP)
	    rmdir(fpath);
	else
	    unlink(fpath);
	return;
    }
    switch (mode) {
    case ADDITEM:
	{
	    int edflags = 0;
# ifdef BN_BBSMOVIE
	    if (pm && pm->bid && 
		strcmp(getbcache(pm->bid)->brdname, 
			BN_BBSMOVIE) == 0)
	    {
		edflags |= EDITFLAG_UPLOAD;
		edflags |= EDITFLAG_ALLOWLARGE;
	    }
# endif // BN_BBSMOVIE
	    if (vedit2(fpath, 0, NULL, edflags) == -1) {
		unlink(fpath);
		pressanykey();
		return;
	    }
	}
	break;
    case ADDGROUP:
	// do nothing
	break;
    }

    strlcpy(item.owner, cuser.userid, sizeof(item.owner));
    a_additem(pm, &item);
}

void
a_pasteitem(menu_t * pm, int mode)
{
    char            newpath[PATHLEN];
    char            buf[PATHLEN];
    char            ans[2], skipAll = 0, multiple = 0;
    int             i, copied = 0;
    fileheader_t    item;

    CopyQueue *cq;

    move(b_lines - 1, 0);
    if(copyqueue_querysize() <= 0)
    {
	vmsg("�Х�����ƻs(copy)�R�O��A�K�W(paste)");
	return;
    }
    if(mode && copyqueue_querysize() > 1)
    {
	multiple = 1;
	move(b_lines-2, 0); clrtobot();
	outs("c: ��U���حӧO�T�{�O�_�n�K�W, z: �������K�A�P�ɭ��]�è��������аO\n");
	snprintf(buf, sizeof(buf),
		"�T�w�n�K�W�����@ %d �Ӷ��ض� (c/z/y/N)�H ", 
		copyqueue_querysize());
	getdata(b_lines - 1, 0, buf, ans, sizeof(ans), LCECHO);
	if(ans[0] == 'y')
	    skipAll = 1;
	else if(ans[0] == 'z')
	{
	    copyqueue_reset();
	    vmsg("�w���]�ƻs�O���C");
	    return;
	}
	else if (ans[0] != 'c')
	    return;
	clear();
    }
    while (copyqueue_querysize() > 0)
    {
	cq = copyqueue_gethead();
	if(!cq->copyfile[0])
	    continue;
	if(mode && multiple)
	{
	    scroll();
	    move(b_lines-2, 0); clrtobot();
	    prints("%d. %s\n", ++copied,cq->copytitle);

	}

	if (dashd(cq->copyfile)) {
	    for (i = 0; cq->copyfile[i] && cq->copyfile[i] == pm->path[i]; i++);
	    if (!cq->copyfile[i]) {
		vmsg("�N�ؿ����i�ۤv���l�ؿ����A�|�y���L�a�j��I");
		continue;
	    }
	}
	if (mode && !skipAll) {
	    snprintf(buf, sizeof(buf),
		     "�T�w�n����[%s]��(Y/N)�H[N] ", cq->copytitle);
	    getdata(b_lines - 1, 0, buf, ans, sizeof(ans), LCECHO);
	} else
	    ans[0] = 'y';
	if (ans[0] == 'y') {
	    strlcpy(newpath, pm->path, sizeof(newpath));

	    if (*cq->copyowner) {
		char           *fname = strrchr(cq->copyfile, '/');

		if (fname)
		    strcat(newpath, fname);
		else
		    return;
		if (access(pm->path, X_OK | R_OK | W_OK))
		    mkdir(pm->path, 0755);
		memset(&item, 0, sizeof(fileheader_t));
		strlcpy(item.filename, fname + 1, sizeof(item.filename));
		memcpy(cq->copytitle, "��", 2);
		Copy(cq->copyfile, newpath);
	    } else if (dashf(cq->copyfile)) {
		stampfile(newpath, &item);
		memcpy(cq->copytitle, "��", 2);
                Copy(cq->copyfile, newpath);
	    } else if (dashd(cq->copyfile)) {
		stampdir(newpath, &item);
		memcpy(cq->copytitle, "��", 2);
		copy_file(cq->copyfile, newpath);
	    } else {
		copyqueue_reset();
		vmsg("�L�k�����I");
		return;
	    }
	    strlcpy(item.owner, *cq->copyowner ? cq->copyowner : cuser.userid,
		    sizeof(item.owner));
	    strlcpy(item.title, cq->copytitle, sizeof(item.title));
	    a_additem(pm, &item);
	    cq->copyfile[0] = '\0';
	}
    }
}

static void
a_appenditem(const menu_t * pm, int isask)
{
    char            fname[PATHLEN];
    char            buf[ANSILINELEN];
    char            ans[2] = "y";
    FILE           *fp, *fin;

    move(b_lines - 1, 0);
    if(copyqueue_querysize() <= 0)
    {
	vmsg("�Х����� copy �R�O��A append");
	copyqueue_reset();
	return;
    } 
    else
    {
	CopyQueue *cq = copyqueue_gethead();
	off_t sz;

	if (!dashf(cq->copyfile)) {
	    vmsg("�ؿ����o���[���ɮ׫�I");
	    return;
	}

	snprintf(fname, sizeof(fname), "%s/%s", pm->path,
		pm->header[pm->now - pm->page].filename);

	// if same file, abort.
	if (!dashf(fname) || strcmp(fname, cq->copyfile) == 0) {
	    vmsg("�ɮפ��o���[�󦹡I");
	    return;
	}

	sz = dashs(fname);
	if (sz >= MAX_FILE_SIZE)
	{
	    vmsg("�ɮפw�W�L�̤j����A�L�k�A���[");
	    return;
	}

	if (isask) {
	    snprintf(buf, sizeof(buf),
		    "�T�w�n�N[%s]���[�󦹶�(Y/N)�H[N] ", cq->copytitle);
	    getdata(b_lines - 2, 1, buf, ans, sizeof(ans), LCECHO);
	}

	if (ans[0] != 'y' || !(fp = fopen(fname, "a+")))
	    return;

	if (!(fin = fopen(cq->copyfile, "r"))) {
	    fclose(fp);
	    return;
	}

	memset(buf, '-', 74);
	buf[74] = '\0';
	fprintf(fp, "\n> %s <\n\n", buf);
	if (isask)
	    getdata(b_lines - 1, 1,
		    "�O�_����ñ�W�ɳ���(Y/N)�H[Y] ",
		    ans, sizeof(ans), LCECHO);

	// XXX reported by Kinra, appending same file may cause endless loop here.
	// we check file name at prior steps and do file size check here.
	while (sz > 0 && fgets(buf, sizeof(buf), fin)) {
	    sz -= strlen(buf);
	    if ((ans[0] == 'n') &&
		    !strcmp(buf, "--\n"))
		break;
	    fputs(buf, fp);
	}
	fclose(fin);
	fclose(fp);
	cq->copyfile[0] = '\0';
    }
}

static int
a_pastetagpost(menu_t * pm, int mode)
{
    fileheader_t    fhdr;
    boardheader_t  *bh = NULL;
    int             ans = 0, ent = 0, tagnum;
    char            title[TTLEN + 1] = "��  ";
    char            dirname[PATHLEN], buf[PATHLEN];

    if (TagBoard == 0){
	sethomedir(dirname, cuser.userid);
    }
    else{
	bh = getbcache(TagBoard);
	setbdir(dirname, bh->brdname);
    }
    tagnum = TagNum;

    // prevent if anything wrong
    if (tagnum > MAXTAGS || tagnum < 0)
    {
	vmsg("�������~�C�Ч�A���i�檺����B�J�K�� "
		BN_BUGREPORT " �O�C");
	return ans;
    }

    if (tagnum < 1)
	return ans;

    /* since we use different tag features,
     * copyqueue is not required/used. */
    copyqueue_reset();

    while (tagnum-- > 0) {
	memset(&fhdr, 0, sizeof(fhdr));
	EnumTagFhdr(&fhdr, dirname, ent++);

	// XXX many process crashed here as fhdr.filename[0] == 0
	// let's workaround it? or trace?
	// if (!fhdr->filename[0])
        //   continue;
	
	if (!fhdr.filename[0])
	{
	    grayout(0, b_lines-2, GRAYOUT_DARK);
	    move(b_lines-1, 0); clrtobot();
	    prints("�� %d ���B�z�o�Ϳ��~�C �Ч�A���i�檺����B�J�K�� "
		    BN_BUGREPORT " �O�C\n", ent);
	    vmsg("�������~���~��i��C");
	    continue;
	}

	if (TagBoard == 0) 
	    sethomefile(buf, cuser.userid, fhdr.filename);
	else
	    setbfile(buf, bh->brdname, fhdr.filename);

	if (dashf(buf)) {
	    strlcpy(title + 3, fhdr.title, sizeof(title) - 3);
	    a_copyitem(buf, title, 0, 0);
	    if (mode) {
		mode--;
		a_pasteitem(pm, 0);
	    } else
		a_appenditem(pm, 0);
	    ++ans;
	    UnTagger(tagnum);
	}
    }

    return ans;
}

static void
a_moveitem(menu_t * pm)
{
    fileheader_t   *tmp;
    char            newnum[5];
    int             num, max, min;
    char            buf[PATHLEN];
    int             fail;

    snprintf(buf, sizeof(buf), "�п�J�� %d �ﶵ���s���ǡG", pm->now + 1);
    if (!getdata(b_lines - 1, 1, buf, newnum, sizeof(newnum), DOECHO))
	return;
    num = (newnum[0] == '$') ? 9999 : atoi(newnum) - 1;
    if (num >= pm->num)
	num = pm->num - 1;
    else if (num < 0)
	num = 0;
    setadir(buf, pm->path);
    min = num < pm->now ? num : pm->now;
    max = num > pm->now ? num : pm->now;
    tmp = (fileheader_t *) calloc(max + 1, FHSZ);

    fail = 0;
    if (get_records(buf, tmp, FHSZ, 1, min) != min)
	fail = 1;
    if (num > pm->now) {
	if (get_records(buf, &tmp[min], FHSZ, pm->now + 2, max - min) != max - min)
	    fail = 1;
	if (get_records(buf, &tmp[max], FHSZ, pm->now + 1, 1) != 1)
	    fail = 1;
    } else {
	if (get_records(buf, &tmp[min], FHSZ, pm->now + 1, 1) != 1)
	    fail = 1;
	if (get_records(buf, &tmp[min + 1], FHSZ, num + 1, max - min) != max - min)
	    fail = 1;
    }
    if (!fail)
	substitute_record(buf, tmp, FHSZ * (max + 1), 1);
    pm->now = num;
    free(tmp);
}

static void
a_delrange(menu_t * pm)
{
    char            fname[PATHLEN];

    snprintf(fname, sizeof(fname), "%s/" FN_DIR, pm->path);
    del_range(0, NULL, fname);
    pm->num = get_num_records(fname, FHSZ);
}

static void
a_delete(menu_t * pm)
{
    char            fpath[PATHLEN], buf[PATHLEN], cmd[PATHLEN];
    char            ans[4];
    fileheader_t    backup;

    snprintf(fpath, sizeof(fpath),
	     "%s/%s", pm->path, pm->header[pm->now - pm->page].filename);
    setadir(buf, pm->path);

    if (pm->header[pm->now - pm->page].filename[0] == 'H' &&
	pm->header[pm->now - pm->page].filename[1] == '.') {
	getdata(b_lines - 1, 1, "�z�T�w�n�R������ذϳs�u��(Y/N)�H[N] ",
		ans, sizeof(ans), LCECHO);
	if (ans[0] != 'y')
	    return;
	if (delete_record(buf, FHSZ, pm->now + 1) == -1)
	    return;
    } else if (dashl(fpath)) {
	getdata(b_lines - 1, 1, "�z�T�w�n�R���� symbolic link ��(Y/N)�H[N] ",
		ans, sizeof(ans), LCECHO);
	if (ans[0] != 'y')
	    return;
	if (delete_record(buf, FHSZ, pm->now + 1) == -1)
	    return;
	unlink(fpath);
    } else if (dashf(fpath)) {
	getdata(b_lines - 1, 1, "�z�T�w�n�R�����ɮ׶�(Y/N)�H[N] ", ans,
		sizeof(ans), LCECHO);
	if (ans[0] != 'y')
	    return;
	if (delete_record(buf, FHSZ, pm->now + 1) == -1)
	    return;

	setbpath(buf, "deleted");
	stampfile(buf, &backup);
	strlcpy(backup.owner, cuser.userid, sizeof(backup.owner));
	strlcpy(backup.title,
		pm->header[pm->now - pm->page].title + 2,
		sizeof(backup.title));

	snprintf(cmd, sizeof(cmd),
		 "mv -f %s %s", fpath, buf);
	system(cmd);
	setbdir(buf, "deleted");
	append_record(buf, &backup, sizeof(backup));
    } else if (dashd(fpath)) {
	getdata(b_lines - 1, 1, "�z�T�w�n�R����ӥؿ���(Y/N)�H[N] ", ans,
		sizeof(ans), LCECHO);
	if (ans[0] != 'y')
	    return;
	if (delete_record(buf, FHSZ, pm->now + 1) == -1)
	    return;

	setapath(buf, "deleted");
	stampdir(buf, &backup);

	snprintf(cmd, sizeof(cmd),
		 "rm -rf %s;/bin/mv -f %s %s", buf, fpath, buf);
	system(cmd);

	strlcpy(backup.owner, cuser.userid, sizeof(backup.owner));
        strcpy(backup.title, "��");
	strlcpy(backup.title + 2,
		pm->header[pm->now - pm->page].title + 2,
		sizeof(backup.title) - 3);

	/* merge setapath(buf, "deleted"); setadir(buf, buf); */
	snprintf(buf, sizeof(buf), "man/boards/%c/%s/" FN_DIR,
		 'd', "deleted");
	append_record(buf, &backup, sizeof(backup));
    } else {			/* Ptt �l�������� */
	getdata(b_lines - 1, 1, "�z�T�w�n�R�����l�������ض�(Y/N)�H[N] ",
		ans, sizeof(ans), LCECHO);
	if (ans[0] != 'y')
	    return;
	if (delete_record(buf, FHSZ, pm->now + 1) == -1)
	    return;
    }
    pm->num--;
}

static void
a_newtitle(const menu_t * pm)
{
    char            buf[PATHLEN];
    fileheader_t    item;

    memcpy(&item, &pm->header[pm->now - pm->page], FHSZ);
    strlcpy(buf, item.title + 3, sizeof(buf));
    if (getdata_buf(b_lines - 1, 0, "   �s���D: ", buf, 60, DOECHO)) {
	strlcpy(item.title + 3, buf, sizeof(item.title) - 3);
	setadir(buf, pm->path);
	substitute_record(buf, &item, FHSZ, pm->now + 1);
    }
}
static void
a_hideitem(const menu_t * pm)
{
    fileheader_t   *item = &pm->header[pm->now - pm->page];
    char            buf[PATHLEN];
    if (item->filemode & FILE_BM) {
	item->filemode &= ~FILE_BM;
	item->filemode &= ~FILE_HIDE;
    } else if (item->filemode & FILE_HIDE)
	item->filemode |= FILE_BM;
    else
	item->filemode |= FILE_HIDE;
    setadir(buf, pm->path);
    substitute_record(buf, item, FHSZ, pm->now + 1);
}
static void
a_editsign(const menu_t * pm)
{
    char            buf[PATHLEN];
    fileheader_t    item;

    memcpy(&item, &pm->header[pm->now - pm->page], FHSZ);
    snprintf(buf, sizeof(buf), "%c%c", item.title[0], item.title[1]);
    if (getdata_buf(b_lines - 1, 1, "�Ÿ�", buf, 5, DOECHO)) {
	item.title[0] = buf[0] ? buf[0] : ' ';
	item.title[1] = buf[1] ? buf[1] : ' ';
	item.title[2] = buf[2] ? buf[2] : ' ';
	setadir(buf, pm->path);
	substitute_record(buf, &item, FHSZ, pm->now + 1);
    }
}

static void
a_showname(const menu_t * pm)
{
    char            buf[PATHLEN];
    int             len;
    int             i;
    int             sym;

    move(b_lines - 1, 0);
    snprintf(buf, sizeof(buf),
	     "%s/%s", pm->path, pm->header[pm->now - pm->page].filename);
    if (dashl(buf)) {
	prints("�� symbolic link �W�٬� %s\n",
	       pm->header[pm->now - pm->page].filename);
	if ((len = readlink(buf, buf, PATHLEN - 1)) >= 0) {
	    buf[len] = '\0';
	    for (i = 0; BBSHOME[i] && buf[i] == BBSHOME[i]; i++);
	    if (!BBSHOME[i] && buf[i] == '/') {
		if (HasUserPerm(PERM_BBSADM))
		    sym = 1;
		else {
		    sym = 0;
		    for (i++; BBSHOME "/man"[i] && buf[i] == BBSHOME "/man"[i];
			 i++);
		    if (!BBSHOME "/man"[i] && buf[i] == '/')
			sym = 1;
		}
		if (sym) {
		    vmsgf("�� symbolic link ���V %s\n", &buf[i + 1]);
		}
	    }
	}
    } else if (dashf(buf))
	prints("���峹�W�٬� %s", pm->header[pm->now - pm->page].filename);
    else if (dashd(buf))
	prints("���ؿ��W�٬� %s", pm->header[pm->now - pm->page].filename);
    else
	outs("�����ؤw�l��, ��ĳ�N��R���I");
    pressanykey();
}
#ifdef CHESSCOUNTRY
static void
a_setchesslist(const menu_t * me)
{
    char buf[4];
    char buf_list[PATHLEN];
    char buf_photo[PATHLEN];
    char buf_this[PATHLEN];
    char buf_real[PATHLEN];
    int list_exist, photo_exist;
    fileheader_t* fhdr = me->header + me->now - me->page;
    int n;

    snprintf(buf_this,  sizeof(buf_this),  "%s/%s", me->path, fhdr->filename);
    if((n = readlink(buf_this, buf_real, sizeof(buf_real) - 1)) == -1)
	strcpy(buf_real, fhdr->filename);
    else
	// readlink doesn't garentee zero-ended
	buf_real[n] = 0;

    if (strcmp(buf_real, "chess_list") == 0
	    || strcmp(buf_real, "chess_photo") == 0) {
	vmsg("���ݭ��]�I");
	return;
    }

    snprintf(buf_list,  sizeof(buf_list),  "%s/chess_list",  me->path);
    snprintf(buf_photo, sizeof(buf_photo), "%s/chess_photo", me->path);

    list_exist = dashf(buf_list);
    photo_exist = dashd(buf_photo);

    if (!list_exist && !photo_exist) {
	vmsg("���ݪO�D�Ѱ�I");
	return;
    }

    getdata(b_lines, 0, "�N�����س]�w�� (1) �Ѱ�W�� (2) �Ѱ�Ӥ��ɥؿ��G",
	    buf, sizeof(buf), 1);
    if (buf[0] == '1') {
	if (list_exist)
	    getdata(b_lines, 0, "�즳���Ѱ�W��N�Q���N�A�нT�{ (y/N)",
		    buf, sizeof(buf), 1);
	else
	    buf[0] = 'y';

	if (buf[0] == 'y' || buf[0] == 'Y') {
	    Rename(buf_this, buf_list);
	    symlink("chess_list", buf_this);
	}
    } else if (buf[0] == '2') {
	if (photo_exist)
	    getdata(b_lines, 0, "�즳���Ѱ�Ӥ��N�Q���N�A�нT�{ (y/N)",
		    buf, sizeof(buf), 1);
	else
	    buf[0] = 'y';

	if (buf[0] == 'y' || buf[0] == 'Y') {
	    if(strncmp(buf_photo, "man/boards/", 11) == 0 && // guarding
		    buf_photo[11] && buf_photo[12] == '/' && // guarding
		    snprintf(buf_list, sizeof(buf_list), "rm -rf %s", buf_photo)
		    == strlen(buf_photo) + 7)
		system(buf_list);
	    Rename(buf_this, buf_photo);
	    symlink("chess_photo", buf_this);
	}
    }
}
#endif /* defined(CHESSCOUNTRY) */

static int
isvisible_man(const menu_t * me)
{
    fileheader_t   *fhdr = &me->header[me->now - me->page];
    /* board friend only effact when in board reading mode */
    if (me->level >= MANAGER)
	return 1;
    if (fhdr->filemode & FILE_BM)
	return 0;
    if (fhdr->filemode & FILE_HIDE)
    {
	if (currstat == ANNOUNCE ||
	    !is_hidden_board_friend(me->bid, currutmp->uid))
	    return 0;
    }
    return 1;
}
int
a_menu(const char *maintitle, const char *path, 
	int lastlevel, int lastbid,
	char *trans_buffer)
{
    static char     Fexit;	// �ΨӸ��X recursion
    menu_t          me;
    char            fname[PATHLEN];
    int             ch, returnvalue = FULLUPDATE;

    // prevent deep resursive directories
    if (strlen(path) + FNLEN >= PATHLEN)
    {
	// it is not save to enter such directory.
	return returnvalue;
    }

    if(trans_buffer)
	trans_buffer[0] = '\0';

    memset(&me, 0, sizeof(me));
    Fexit = 0;
    me.header_size = p_lines;
    me.header = (fileheader_t *) calloc(me.header_size, FHSZ);
    me.path = path;
    strlcpy(me.mtitle, maintitle, sizeof(me.mtitle));
    setadir(fname, me.path);
    me.num = get_num_records(fname, FHSZ);
    me.bid = lastbid;

    /* ��ذ�-tree ���������c�ݩ� cuser ==> BM */

    if (!(me.level = lastlevel)) {
	char           *ptr;

	if ((ptr = strrchr(me.mtitle, '[')))
	    me.level = is_BM(ptr + 1);
    }
    me.page = 9999;
    me.now = 0;
    for (;;) {
	if (me.now >= me.num)
	    me.now = me.num - 1;
	if (me.now < 0)
	    me.now = 0;

	if (me.now < me.page || me.now >= me.page + me.header_size) {
	    me.page = me.now - ((me.page == 10000 && me.now > p_lines / 2) ?
				(p_lines / 2) : (me.now % p_lines));
	    if (!a_showmenu(&me))
	    {
		// some directories are invalid, restart!
		Fexit = 1;
		break;
	    }
	}
	ch = cursor_key(2 + me.now - me.page, 0);

	if (ch == 'q' || ch == 'Q' || ch == KEY_LEFT)
	    break;

	if (ch >= '1' && ch <= '9') {
	    if ((ch = search_num(ch, me.num)) != -1)
		me.now = ch;
	    me.page = 10000;
	    continue;
	}
	switch (ch) {
	case KEY_UP:
	case 'k':
	    if (--me.now < 0)
		me.now = me.num - 1;
	    break;

	case KEY_DOWN:
	case 'j':
	    if (++me.now >= me.num)
		me.now = 0;
	    break;

	case KEY_PGUP:
	case Ctrl('B'):
	    if (me.now >= p_lines)
		me.now -= p_lines;
	    else if (me.now > 0)
		me.now = 0;
	    else
		me.now = me.num - 1;
	    break;

	case ' ':
	case KEY_PGDN:
	case Ctrl('F'):
	    if (me.now < me.num - p_lines)
		me.now += p_lines;
	    else if (me.now < me.num - 1)
		me.now = me.num - 1;
	    else
		me.now = 0;
	    break;

	case '0':
	    me.now = 0;
	    break;
	case '?':
	case '/':
	    if(me.num) {
		me.now = a_searchtitle(&me, ch == '?');
		me.page = 9999;
	    }
	    break;
	case '$':
	    me.now = me.num - 1;
	    break;
	case 'h':
	    a_showhelp(me.level);
	    me.page = 9999;
	    break;

	case Ctrl('I'):
	    t_idle();
	    me.page = 9999;
	    break;

	case 'e':
	case 'E':
	    snprintf(fname, sizeof(fname),
		     "%s/%s", path, me.header[me.now - me.page].filename);
	    if (dashf(fname) && me.level >= MANAGER) {
		int edflags = 0;
		*quote_file = 0;

# ifdef BN_BBSMOVIE
		if (me.bid && strcmp(getbcache(me.bid)->brdname, 
			    BN_BBSMOVIE) == 0)
		{
		    edflags |= EDITFLAG_UPLOAD;
		    edflags |= EDITFLAG_ALLOWLARGE;
		}
# endif // BN_BBSMOVIE

		if (vedit2(fname, NA, NULL, edflags) != -1) {
		    char            fpath[PATHLEN];
		    fileheader_t    fhdr;
		    strlcpy(fpath, path, sizeof(fpath));
		    stampfile(fpath, &fhdr);
		    unlink(fpath);
		    strlcpy(fhdr.filename,
			    me.header[me.now - me.page].filename,
			    sizeof(fhdr.filename));
		    strlcpy(me.header[me.now - me.page].owner,
			    cuser.userid,
			    sizeof(me.header[me.now - me.page].owner));
		    setadir(fpath, path);
		    substitute_record(fpath, me.header + me.now - me.page,
				      sizeof(fhdr), me.now + 1);

		}
		me.page = 9999;
	    }
	    break;

	case 't':
	case 'c':
	    if (me.now < me.num) {
		if (!isvisible_man(&me))
		    break;

		snprintf(fname, sizeof(fname), "%s/%s", path,
			 me.header[me.now - me.page].filename);

		/* XXX: dirty fix
		   ���ӭn�令�p�G�o�{�ӥؿ��̭������Υؿ����ܤ~�ڵ�.
		   ���L�o�˪��ܶ��n��ӷj�@�M, �ӥB�ثe�P�_�Ӹ�ƬO�ؿ�
		   �٬O�ɮ׳��M�O�� fstat(2) �Ӥ��O�����s�b .DIR �� |||b
		   �����Ӹ�Ƽg�J .DIR ���A implement�~���Ĳv.
		 */
		if( !lastlevel && !HasUserPerm(PERM_SYSOP) &&
		    (me.bid==0 || !is_BM_cache(me.bid)) && dashd(fname) )
		    vmsg("�u���O�D�~�i�H�����ؿ���!");
		else
		    a_copyitem(fname, me.header[me.now - me.page].title, 0, 1);
		me.page = 9999;
		/* move down */
		if (++me.now >= me.num)
		    me.now = 0;
		break;
	    }
	case '\n':
	case '\r':
	case KEY_RIGHT:
	case 'r':
	    if (me.now < me.num) {
		fileheader_t   *fhdr = &me.header[me.now - me.page];
		if (!isvisible_man(&me))
		    break;
#ifdef DEBUG
		vmsgf("%s/%s", &path[11], fhdr->filename);;
#endif
		snprintf(fname, sizeof(fname), "%s/%s", path, fhdr->filename);
		if (*fhdr->filename == 'H' && fhdr->filename[1] == '.') {
		  vmsg("���A�䴩 gopher mode, �Шϥ��s���������s��");
		  vmsgf("gopher://%s/1/",fhdr->filename+2);
		} else if (dashf(fname)) {
		    int             more_result;

		    while ((more_result = more(fname, YEA))) {
			/* Ptt �d�����F plugin */
			if (trans_buffer && 
				(currstat == EDITEXP || currstat == OSONG)) {
			    char            ans[4];

			    move(22, 0);
			    clrtoeol();
			    getdata(22, 1,
				    currstat == EDITEXP ?
				    "�n��d�� Plugin ��峹��?[y/N]" :
				    "�T�w�n�I�o���q��?[y/N]",
				    ans, sizeof(ans), LCECHO);
			    if (ans[0] == 'y') {
				strlcpy(trans_buffer, fname, PATHLEN);
				Fexit = 1;
				if (currstat == OSONG) {
				    log_filef(FN_USSONG, LOG_CREAT, "%s\n", fhdr->title);
				}
				free(me.header);
				return FULLUPDATE;
			    }
			}
			if (more_result == READ_PREV) {
			    if (--me.now < 0) {
				me.now = 0;
				break;
			    }
			} else if (more_result == READ_NEXT) {
			    if (++me.now >= me.num) {
				me.now = me.num - 1;
				break;
			    }
			    /* we only load me.header_size pages */
			    if (me.now - me.page >= me.header_size)
				break;
			} else
			    break;
			if (!isvisible_man(&me))
			    break;
			snprintf(fname, sizeof(fname), "%s/%s", path,
				 me.header[me.now - me.page].filename);
			if (!dashf(fname))
			    break;
		    }
		} else if (dashd(fname)) {
		    a_menu(me.header[me.now - me.page].title, fname, 
			    me.level, me.bid, trans_buffer);
		    /* Ptt  �j�O���Xrecursive */
		    if (Fexit) {
			free(me.header);
			return FULLUPDATE;
		    }
		}
		me.page = 9999;
	    }
	    break;

	case 'F':
	case 'U':
	    if (me.now < me.num) {
                fileheader_t   *fhdr = &me.header[me.now - me.page];
                if (!isvisible_man(&me))
                    break;
		snprintf(fname, sizeof(fname),
			 "%s/%s", path, fhdr->filename);
		if (HasUserPerm(PERM_LOGINOK) && dashf(fname)) {
		    a_forward(path, fhdr, ch /* == 'U' */ );
		    /* By CharlieL */
		} else
		    vmsg("�L�k��H������");
		me.page = 9999;
	    }

	    break;

	}

	if (me.level >= MANAGER) {
	    switch (ch) {
	    case 'n':
		a_newitem(&me, ADDITEM);
		me.page = 9999;
		break;
	    case 'g':
		a_newitem(&me, ADDGROUP);
		me.page = 9999;
		break;
	    case 'p':
		a_pasteitem(&me, 1);
		me.page = 9999;
		break;
	    case 'f':
		a_editsign(&me);
		me.page = 9999;
		break;
	    case Ctrl('P'):
		a_pastetagpost(&me, -1);
		returnvalue = DIRCHANGED;
		me.page = 9999;
		break;
	    case Ctrl('A'):
		a_pastetagpost(&me, 1);
		returnvalue = DIRCHANGED;
		me.page = 9999;
		break;
	    case 'a':
		a_appenditem(&me, 1);
		me.page = 9999;
		break;
#ifdef BLOG
	    case 'b':
		if (me.bid)
		{
		    char    genbuf[128];
		    char *bname = getbcache(me.bid)->brdname;
		    snprintf(genbuf, sizeof(genbuf),
			    "bin/builddb.pl -f -n %d %s", me.now, bname);
		    system(genbuf);
		    vmsg("��Ƨ�s����");
		}
		me.page = 9999;
		break;

	    case 'B':
		if (me.bid && me.bid == currbid)
		{
		    BlogMain(me.now);
		};
		me.page = 9999;
		break;
#endif
	    }

	    if (me.num)
		switch (ch) {
		case 'm':
		    a_moveitem(&me);
		    me.page = 9999;
		    break;

		case 'D':
		    /* Ptt me.page = -1; */
		    a_delrange(&me);
		    me.page = 9999;
		    break;
		case 'd':
		    a_delete(&me);
		    me.page = 9999;
		    break;
		case 'H':
		    a_hideitem(&me);
		    me.page = 9999;
		    break;
		case 'T':
		    a_newtitle(&me);
		    me.page = 9999;
		    break;
#ifdef CHESSCOUNTRY
		case 'L':
		    a_setchesslist(&me);
		    break;
#endif
		}
	}
	if (me.level >= SYSOP) {
	    switch (ch) {
	    case 'N':
		a_showname(&me);
		me.page = 9999;
		break;
	    }
	}
    }
    free(me.header);
    return returnvalue;
}

int
Announce(void)
{
    setutmpmode(ANNOUNCE);
    a_menu(BBSNAME "�G�i��", "man",
	   ((HasUserPerm(PERM_SYSOP) ) ? SYSOP : NOBODY), 
	   0,
	   NULL);
    return 0;
}

#ifdef BLOG
#include <mysql/mysql.h>
void BlogMain(int num)
{
    int     oldmode = currutmp->mode;
    char    genbuf[128], exit = 0;

    // WARNING: �n�T�{ currboard/currbid �w���T�]�w�~��Φ�API�C
    
    //setutmpmode(BLOGGING); /* will crash someone using old program  */
    sprintf(genbuf, "%s��������", currboard);
    showtitle("������", genbuf);
    while( !exit ){
	move(1, 0);
	outs("�п�ܱz�n���檺���@:\n"
	       "0.�^��W�@�h\n"
	       "1.�s�@������˪O�榡\n"
	       "  �ϥηs�� config �ؿ��U�˪O���\n"
	       "  �q�`�b�Ĥ@���ϥγ�����μ˪O��s���ɭԨϥ�\n"
	       "\n"
	       "2.���s�s�@������\n"
	       "  �u�b�������ƾ�Ӷñ����ɭԤ~�ϥ�\n"
	       "\n"
	       "3.�N����[�J������\n"
	       "  �N��ЩҦb��m���峹�[�J������\n"
	       "\n"
	       "4.�R���j�T\n"
	       "\n"
	       "5.�R���@�g������\n"
	       "\n"
	       "C.�إ߳�����ؿ� (�z�u���Ĥ@���ɻݭn)\n"
	       );
	switch( getans("�п��(0-5,C)�H[0]") ){
	case '1':
	    snprintf(genbuf, sizeof(genbuf),
		     "bin/builddb.pl -c %s", currboard);
	    system(genbuf);
	    break;
	case '2':
	    snprintf(genbuf, sizeof(genbuf),
		     "bin/builddb.pl -a %s", currboard);
	    system(genbuf);
	    break;
	case '3':
	    snprintf(genbuf, sizeof(genbuf),
		     "bin/builddb.pl -f -n %d %s", num, currboard);
	    system(genbuf);
	    break;
	case '4':{
	    char    hash[33];
	    int     i;
	    getdata(16, 0, "�п�J�ӽg�������: ",
		    hash, sizeof(hash), DOECHO);
	    for( i = 0 ; hash[i] != 0 ; ++i ) /* �e���� getdata() �O�Ҧ� \0 */
		if( !islower(hash[i]) && !isdigit(hash[i]) )
		    break;
	    if( i != 32 ){
		vmsg("��J���~");
		break;
	    }
	    if( hash[0] != 0 && 
		getans("�нT�w�R��(Y/N)?[N] ") == 'y' ){
		MYSQL   mysql;
		char    cmd[256];
		
		snprintf(cmd, sizeof(cmd), "delete from comment where "
			"hash='%s'&&brdname='%s'", hash, currboard);
#ifdef DEBUG
		vmsg(cmd);
#endif
		if( !(!mysql_init(&mysql) ||
		      !mysql_real_connect(&mysql, BLOGDB_HOST, BLOGDB_USER,
					  BLOGDB_PASSWD, BLOGDB_DB, 
					  BLOGDB_PORT, BLOGDB_SOCK, 0) ||
		      mysql_query(&mysql, cmd)) )
		    vmsg("��ƧR������");
		else
		    vmsg(
#ifdef DEBUG
			 mysql_error(&mysql)
#else
			 "database internal error"
#endif
			 );
		mysql_close(&mysql);
	    }
	}
	    break;
	    
	case '5': {
	    char    date[9];
	    int     i;
	    getdata(16, 0, "�п�J�ӽg�����(yyyymmdd): ",
		    date, sizeof(date), DOECHO);
	    for( i = 0 ; i < 9 ; ++i )
		if( !isdigit(date[i]) )
		    break;
	    if( i != 8 ){
		vmsg("��J���~");
		break;
	    }
	    snprintf(genbuf, sizeof(genbuf),
		     "bin/builddb.pl -D %s %s", date, currboard);
	    system(genbuf);
	}
	    break;

	case 'C': case 'c': {
	    fileheader_t item;
	    char    fpath[PATHLEN], adir[PATHLEN], buf[256];
	    setapath(fpath, currboard);
	    stampdir(fpath, &item);
	    strlcpy(item.title, "�� Blog", sizeof(item.title));
	    strlcpy(item.owner, cuser.userid, sizeof(item.owner));

	    setapath(adir, currboard);
	    strcat(adir, "/" FN_DIR);
	    append_record(adir, &item, FHSZ);

	    snprintf(buf, sizeof(buf),
		     "cp -R etc/Blog.Default/" FN_DIR " etc/Blog.Default/* %s/",
		     fpath);
	    system(buf);

	    more("etc/README.BLOG", YEA);
	}
	    break;

	default:
	    exit = 1;
	    break;
	}
	if( !exit )
	    vmsg("�����槹��");
    }
    currutmp->mode = oldmode;
    pressanykey();
}
#endif
