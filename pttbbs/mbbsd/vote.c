/* $Id$ */
#include "bbs.h"

// XXX TODO bvote ���ڪ� vote control file ���ɷ|���P�B...
// XXX TODO �}���٬O���W�ߪ� utility �� daemon �h�]����n

#define MAX_VOTE_NR	(20)
#define MAX_VOTE_PAGE	(5)
#define ITEM_PER_PAGE	(30)

static const char * const STR_bv_control = "control";	/* �벼��� �ﶵ */
static const char * const STR_bv_desc    = "desc";	/* �벼�ت� */
static const char * const STR_bv_ballots = "ballots";	/* �몺�� (per byte) */
static const char * const STR_bv_flags   = "flags";
static const char * const STR_bv_comments= "comments";	/* �벼�̪���ĳ */
static const char * const STR_bv_limited = "limited";	/* �p�H�벼 */
static const char * const STR_bv_limits  = "limits";	/* �벼��歭�� */
static const char * const STR_bv_title   = "vtitle";

static const char * const STR_bv_results = "results";

// XXX TODO use macros to sync the length of strings above?
typedef struct {
    char control [sizeof("controlXX\0") ];
    char desc    [sizeof("descXX\0")    ];
    char ballots [sizeof("ballotsXX\0") ];
    char flags   [sizeof("flagsXX\0")   ];
    char comments[sizeof("commentsXX\0")];
    char limited [sizeof("limitedXX\0") ];
    char limits  [sizeof("limitsXX\0")  ];
    char title   [sizeof("vtitleXX\0")  ];
} vote_buffer_t;

static void
votebuf_init(vote_buffer_t *vbuf, int n)
{
    assert(vbuf);
    snprintf(vbuf->ballots, sizeof(vbuf->ballots), "%s%d", STR_bv_ballots, n);
    snprintf(vbuf->control, sizeof(vbuf->control), "%s%d", STR_bv_control, n);
    snprintf(vbuf->desc,    sizeof(vbuf->desc),    "%s%d", STR_bv_desc,    n);
    snprintf(vbuf->flags,   sizeof(vbuf->flags),   "%s%d", STR_bv_flags,   n);
    snprintf(vbuf->comments,sizeof(vbuf->comments),"%s%d", STR_bv_comments,n);
    snprintf(vbuf->limited, sizeof(vbuf->limited), "%s%d", STR_bv_limited, n);
    snprintf(vbuf->limits,  sizeof(vbuf->limits),  "%s%d", STR_bv_limits,  n);
    snprintf(vbuf->title,   sizeof(vbuf->title),   "%s%d", STR_bv_title,   n);
}

void
b_suckinfile(FILE * fp, const char *fname)
{
    FILE *sfp;
    char inbuf[256];

    if ((sfp = fopen(fname, "r"))) {

	while (fgets(inbuf, sizeof(inbuf), sfp))
	    fputs(inbuf, fp);
	fclose(sfp);
    }
}

void
b_suckinfile_invis(FILE * fp, const char *fname, const char *boardname)
{
    FILE           *sfp;

    if ((sfp = fopen(fname, "r"))) {
	char            inbuf[256];
	if(fgets(inbuf, sizeof(inbuf), sfp))
	{
	    /* first time, try if boardname revealed. */
	    char *post = strstr(inbuf, str_post1);
	    if(!post) post = strstr(inbuf, str_post2);
	    if(post) 
		post = strstr(post, boardname);
	    if(post) {
		/* found releaved stuff. */
		/*
		// mosaic method 1
		while(*boardname++)
		    *post++ = '?';
		    */
		// mosaic method 2
		strcpy(post, "(�Y���άݪO)\n");
	    }
	    fputs(inbuf, fp);
	    while (fgets(inbuf, sizeof(inbuf), sfp))
		fputs(inbuf, fp);
	}
	fclose(sfp);
    }
}

static void
b_count(const char *buf, int counts[], short item_num, int *total)
{
    short	    choice;
    int             fd;

    memset(counts, 0, item_num * sizeof(counts[0]));
    *total = 0;
    if ((fd = open(buf, O_RDONLY)) != -1) {
	flock(fd, LOCK_EX);	/* Thor: ����h�H�P�ɺ� */
	while (read(fd, &choice, sizeof(short)) == sizeof(short)) {
	    if (choice >= item_num)
		continue;
	    counts[choice]++;
	    (*total)++;
	}
	flock(fd, LOCK_UN);
	close(fd);
    }
}

static int
b_nonzeroNum(const char *buf)
{
    int             i = 0;
    char            inchar;
    int             fd;

    if ((fd = open(buf, O_RDONLY)) != -1) {
	while (read(fd, &inchar, 1) == 1)
	    if (inchar)
		i++;
	close(fd);
    }
    return i;
}

static void
vote_report(const char *bname, const char *fname, char *fpath)
{
    register char  *ip;
    time4_t         dtime;
    int             fd, bid;
    fileheader_t    header;

    ip = fpath;
    while (*(++ip));
    *ip++ = '/';

    /* get a filename by timestamp */

    dtime = now;
    for (;;) {
        /* TODO: extract record.c:stampfile_u() to common lib */
	sprintf(ip, "M.%d.A.%3.3X", (int)++dtime, (unsigned int)(random() & 0xFFF));
	fd = open(fpath, O_CREAT | O_EXCL | O_WRONLY, 0644);
	if (fd >= 0)
	    break;
	dtime++;
    }
    close(fd);

    /* XXX: FIXME: Possible race condition */
    unlink(fpath);
    link(fname, fpath);

    /* append record to .DIR */

    memset(&header, 0, sizeof(fileheader_t));
    strlcpy(header.owner, "[�������l]", sizeof(header.owner));
    snprintf(header.title, sizeof(header.title), "[%s] �ݪO �ﱡ����", bname);
    {
	struct tm ptime;

	localtime4_r(&dtime, &ptime);
	snprintf(header.date, sizeof(header.date),
		 "%2d/%02d", ptime.tm_mon + 1, ptime.tm_mday);
    }
    strlcpy(header.filename, ip, sizeof(header.filename));

    strcpy(ip, FN_DIR);
    if ((fd = open(fpath, O_WRONLY | O_CREAT, 0644)) >= 0) {
	flock(fd, LOCK_EX);
	lseek(fd, 0, SEEK_END);
	write(fd, &header, sizeof(fileheader_t));
	flock(fd, LOCK_UN);
	close(fd);
	if ((bid = getbnum(bname)) > 0)
	    setbtotal(bid);

    }
}

static void
b_result_one(const vote_buffer_t *vbuf, boardheader_t * fh, int *total)
{
    FILE           *cfp, *tfp, *frp, *xfp;
    char           *bname;
    char            inbuf[ANSILINELEN];
    int            *counts;
    int             people_num;
    short	    item_num, junk;
    char            b_control   [PATHLEN];
    char            b_newresults[PATHLEN];
    char            b_report    [PATHLEN];
    char            buf		[PATHLEN];
    time4_t         closetime;

    if (fh->bvote > 0)
	fh->bvote--;

    bname = fh->brdname;

    setbfile(buf, bname, vbuf->control);
    cfp = fopen(buf, "r");
    assert(cfp);
    fscanf(cfp, "%hd,%hd\n%d\n", &item_num, &junk, &closetime);
    fclose(cfp);

    // prevent death caused by a bug, it should be remove later.
    if (item_num <= 0)
	return;

    // XXX TODO backup vote files?

    counts = (int *)malloc(item_num * sizeof(int));

    setbfile(b_control, bname, "tmp");
    if (rename(buf, b_control) == -1)
	return;
    setbfile(buf, bname, vbuf->flags);
    people_num = b_nonzeroNum(buf);
    unlink(buf);
    setbfile(buf, bname, vbuf->ballots);
    b_count(buf, counts, item_num, total);
    unlink(buf);

    setbfile(b_newresults, bname, "newresults");
    if ((tfp = fopen(b_newresults, "w")) == NULL)
	return;

    setbfile(buf, bname, vbuf->title);

    if ((xfp = fopen(buf, "r"))) {
	fgets(inbuf, sizeof(inbuf), xfp);
	fprintf(tfp, "%s\n�� �벼�W��: %s\n\n", msg_separator, inbuf);
	fclose(xfp);
    }
    fprintf(tfp, "%s\n�� �벼�����: %s\n\n\n�� �����D�شy�z:\n\n",
	    msg_separator, Cdate(&closetime));
    fh->vtime = now;

    setbfile(buf, bname, vbuf->desc);

    b_suckinfile(tfp, buf);
    unlink(buf);

    if ((cfp = fopen(b_control, "r"))) {
	fgets(inbuf, sizeof(inbuf), cfp);
	fgets(inbuf, sizeof(inbuf), cfp);
	fprintf(tfp, "\n���벼���G:(�@�� %d �H�벼,�C�H�̦h�i�� %hd ��)\n",
		people_num, junk);
	fprintf(tfp, "    ��    ��                                   �`����  �o���v  �o������\n");
	for (junk = 0; junk < item_num; junk++) {
	    fgets(inbuf, sizeof(inbuf), cfp);
	    chomp(inbuf);
	    fprintf(tfp, "    %-42s %3d �� %6.2f%%  %6.2f%%\n", inbuf + 3, counts[junk],                
		    (float)(counts[junk] * 100) / (float)(people_num),
		    (float)(counts[junk] * 100) / (float)(*total));
	}
	fclose(cfp);
    }
    unlink(b_control);
    free(counts);

    fprintf(tfp, "%s\n�� �ϥΪ̫�ĳ�G\n\n", msg_separator);
    setbfile(buf, bname, vbuf->comments);
    b_suckinfile(tfp, buf);
    unlink(buf);

    fprintf(tfp, "%s\n�� �`���� = %d ��\n\n", msg_separator, *total);
    fclose(tfp);

    setbfile(b_report, bname, "report");
    if ((frp = fopen(b_report, "w"))) {
	b_suckinfile(frp, b_newresults);
	fclose(frp);
    }
    setbpath(inbuf, bname);
    vote_report(bname, b_report, inbuf);
    if (!(fh->brdattr & (BRD_NOCOUNT|BRD_HIDE))) { // from ptt2 local modification
	setbpath(inbuf, "Record");
	vote_report(bname, b_report, inbuf);
    }
    unlink(b_report);

    tfp = fopen(b_newresults, "a");
    setbfile(buf, bname, STR_bv_results);
    b_suckinfile(tfp, buf);
    fclose(tfp);
    Rename(b_newresults, buf);
}

static void
b_result(boardheader_t * fh)
{
    FILE           *cfp;
    time4_t         closetime;
    int             i, total;
    char            buf[STRLEN];
    char            temp[STRLEN];
    vote_buffer_t   vbuf;

    for (i = 0; i < MAX_VOTE_NR; i++) {
	votebuf_init(&vbuf, i);
	setbfile(buf, fh->brdname, vbuf.control);
	cfp = fopen(buf, "r");
	if (!cfp)
	    continue;
	fgets(temp, sizeof(temp), cfp);
	fscanf(cfp, "%d\n", &closetime);
	fclose(cfp);
	if (closetime < now)
	    b_result_one(&vbuf, fh, &total);
    }
}

static int
b_close(boardheader_t * fh)
{
    b_result(fh);
    return 1;
}

static int
b_closepolls(void)
{
    boardheader_t  *fhp;
    int             pos;

#ifndef BARRIER_HAS_BEEN_IN_SHM
    char    *fn_vote_polling = ".polling";
    unsigned long  last;
    FILE           *cfp;
    /* XXX necessary to lock ? */
    if ((cfp = fopen(fn_vote_polling, "r"))) {
	char timebuf[100];
	fgets(timebuf, sizeof(timebuf), cfp);
	sscanf(timebuf, "%lu", &last);
	fclose(cfp);
	if (last + 3600 >= (unsigned long)now)
	    return 0;
    }
    if ((cfp = fopen(fn_vote_polling, "w")) == NULL)
	return 0;
    fprintf(cfp, "%d\n%s\n\n", now, Cdate(&now));
    fclose(cfp);
#endif

    for (fhp = bcache, pos = 1; pos <= numboards; fhp++, pos++) {
	if (fhp->bvote && b_close(fhp)) {
	    if (substitute_record(fn_board, fhp, sizeof(*fhp), pos) == -1)
		outs(err_board_update);
	    else
		reset_board(pos);
	}
    }

    return 0;
}

void
auto_close_polls(void)
{
    /* �̦h�@�Ѷ}���@�� */
    if (now - SHM->close_vote_time > DAY_SECONDS) {
	b_closepolls();
	SHM->close_vote_time = now;
    }
}

static int
vote_view(const vote_buffer_t *vbuf, const char *bname)
{
    boardheader_t  *fhp;
    FILE           *fp;
    char            buf[STRLEN], genbuf[STRLEN], inbuf[STRLEN];
    short	    item_num, i;
    int             num = 0, pos, *counts, total;
    time4_t         closetime;

    setbfile(buf, bname, vbuf->ballots);
    if ((num = dashs(buf)) < 0) /* file size */
	num = 0;

    setbfile(buf, bname, vbuf->title);
    move(0, 0);
    clrtobot();

    if ((fp = fopen(buf, "r"))) {
	fgets(inbuf, sizeof(inbuf), fp);
	prints("\n�벼�W��: %s", inbuf);
	fclose(fp);
    }
    setbfile(buf, bname, vbuf->control);
    fp = fopen(buf, "r");

    assert(fp);
    fscanf(fp, "%hd,%hd\n%d\n", &item_num, &i, &closetime);
    counts = (int *)malloc(item_num * sizeof(int));

    prints("\n�� �w���벼����: �C�H�̦h�i�� %d ��,�ثe�@�� %d ��,\n"
	   "�����벼�N������ %s\n", atoi(inbuf), (int)(num / sizeof(short)),
	   Cdate(&closetime));

    /* Thor: �}�� ���� �w�� */
    setbfile(buf, bname, vbuf->flags);
    prints("�@�� %d �H�벼\n", b_nonzeroNum(buf));

    setbfile(buf, bname, vbuf->ballots);
    b_count(buf, counts, item_num, &total);

    total = 0;

    for (i = num = 0; i < item_num; i++, num++) {
	fgets(inbuf, sizeof(inbuf), fp);
	chomp(inbuf);
	inbuf[30] = '\0';	/* truncate */
	move(num % 15 + 6, num / 15 * 40);
	prints("  %-32s%3d ��", inbuf, counts[i]);
	total += counts[i];
	if (num == 29) {
	    num = -1;
	    pressanykey();
	    move(6, 0);
	    clrtobot();
	}
    }
    fclose(fp);
    free(counts);
    pos = getbnum(bname);
    assert(0<=pos-1 && pos-1<MAX_BOARD);
    fhp = bcache + pos - 1;
    move(t_lines - 3, 0);
    prints("�� �ثe�`���� = %d ��", total);
    getdata(b_lines - 1, 0, "(A)�����벼 (B)�����}�� (C)�~��H[C] ", genbuf,
	    4, LCECHO);
    if (genbuf[0] == 'a') {
	getdata(b_lines - 1, 0, "�ЦA���T�{�����벼 (Y/N) [N] ", genbuf,
		4, LCECHO);
	if (genbuf[0] == 'n')
	    return FULLUPDATE;

	setbfile(buf, bname, vbuf->control);
	unlink(buf);
	setbfile(buf, bname, vbuf->flags);
	unlink(buf);
	setbfile(buf, bname, vbuf->ballots);
	unlink(buf);
	setbfile(buf, bname, vbuf->desc);
	unlink(buf);
	setbfile(buf, bname, vbuf->limited);
	unlink(buf);
	setbfile(buf, bname, vbuf->limits);
	unlink(buf);
	setbfile(buf, bname, vbuf->title);
	unlink(buf);

	if (fhp->bvote > 0)
	    fhp->bvote--;

	if (substitute_record(fn_board, fhp, sizeof(*fhp), pos) == -1)
	    outs(err_board_update);
	reset_board(pos);
    } else if (genbuf[0] == 'b') {
	b_result_one(vbuf, fhp, &total);
	if (substitute_record(fn_board, fhp, sizeof(*fhp), pos) == -1)
	    outs(err_board_update);

	reset_board(pos);
    }
    return FULLUPDATE;
}

static int
vote_view_all(const char *bname)
{
    int             i;
    int             x = -1;
    FILE           *fp, *xfp;
    char            buf[STRLEN], genbuf[STRLEN];
    char            inbuf[80];
    vote_buffer_t   vbuf;

    move(0, 0);
    for (i = 0; i < MAX_VOTE_NR; i++) {
	votebuf_init(&vbuf, i);
	setbfile(buf, bname, vbuf.control);
	if ((fp = fopen(buf, "r"))) {
	    prints("(%d) ", i);
	    x = i;
	    fclose(fp);

	    setbfile(buf, bname, vbuf.title);
	    if ((xfp = fopen(buf, "r"))) {
		fgets(inbuf, sizeof(inbuf), xfp);
		fclose(xfp);
	    } else
		strlcpy(inbuf, "�L���D", sizeof(inbuf));
	    prints("%s\n", inbuf);
	}
    }

    if (x < 0)
	return FULLUPDATE;

    snprintf(buf, sizeof(buf), "�n�ݴX���벼 [%d] ", x);
    getdata(b_lines - 1, 0, buf, genbuf, 4, LCECHO);


    if (atoi(genbuf) < 0 || atoi(genbuf) > MAX_VOTE_NR)
	snprintf(genbuf, sizeof(genbuf), "%d", x);

    votebuf_init(&vbuf, atoi(genbuf));
    setbfile(buf, bname, vbuf.control);

    if (dashf(buf)) {
	return vote_view(&vbuf, bname);
    } else
	return FULLUPDATE;
}

static int
vote_maintain(const char *bname)
{
    FILE           *fp = NULL;
    char            inbuf[STRLEN], buf[STRLEN];
    int             num = 0, aborted, pos, x, i;
    time4_t         closetime;
    boardheader_t  *fhp;
    char            genbuf[4];
    vote_buffer_t   vbuf;

    if (!(currmode & MODE_BOARD))
	return 0;
    if ((pos = getbnum(bname)) <= 0)
	return 0;

    assert(0<=pos-1 && pos-1<MAX_BOARD);
    fhp = bcache + pos - 1;

    if (fhp->bvote != 0) {

	getdata(b_lines - 1, 0,
		"(V)�[��ثe�벼 (M)�|��s�벼 (A)�����Ҧ��벼 (Q)�~�� [Q]",
		genbuf, 4, LCECHO);
	if (genbuf[0] == 'v')
	    return vote_view_all(bname);
	else if (genbuf[0] == 'a') {
	    fhp->bvote = 0;

	    for (i = 0; i < MAX_VOTE_NR; i++) {
		int j;
		char buf2[64];
		const char *filename[] = {
		    STR_bv_ballots, STR_bv_control, STR_bv_desc, STR_bv_desc,
		    STR_bv_flags, STR_bv_comments, STR_bv_limited, STR_bv_limits,
		    STR_bv_title, NULL
		};
		for (j = 0; filename[j] != NULL; j++) {
		    snprintf(buf2, sizeof(buf2), "%s%d", filename[j], i);
		    setbfile(buf, bname, buf2);
		    unlink(buf);
		}
	    }
	    if (substitute_record(fn_board, fhp, sizeof(*fhp), pos) == -1)
		outs(err_board_update);

	    return FULLUPDATE;
	} else if (genbuf[0] != 'm') {
	    if (fhp->bvote >= MAX_VOTE_NR)
		vmsg("���o�|��L�h�벼");
	    return FULLUPDATE;
	}
    }

    x = 1;
    do {
	votebuf_init(&vbuf, x);
	setbfile(buf, bname, vbuf.control);
	x++;
    } while (dashf(buf) && x <= MAX_VOTE_NR);

    --x;
    if (x >= MAX_VOTE_NR)
	return FULLUPDATE;

    getdata(b_lines - 1, 0,
	    "�T�w�n�|��벼�ܡH [y/N]: ",
	    inbuf, 4, LCECHO);
    if (inbuf[0] != 'y')
	return FULLUPDATE;

    vs_hdr("�|��벼");
    votebuf_init(&vbuf, x);

    clear();
    move(0, 0);
    prints("�� %d ���벼\n", x);
    setbfile(buf, bname, vbuf.title);
    getdata(4, 0, "�п�J�벼�W��:", inbuf, 50, DOECHO);
    if (inbuf[0] == '\0')
	strlcpy(inbuf, "�����W��", sizeof(inbuf));
    fp = fopen(buf, "w");
    assert(fp);
    fputs(inbuf, fp);
    fclose(fp);

    vmsg("��������}�l�s�覹�� [�벼�v��]");
    setbfile(buf, bname, vbuf.desc);
    aborted = veditfile(buf);
    if (aborted == EDIT_ABORTED) {
	vmsg("���������벼");
	return FULLUPDATE;
    }
    aborted = 0;
    setbfile(buf, bname, vbuf.flags);
    unlink(buf);

    getdata(4, 0,
	    "�O�_���w�벼�̦W��G(y)�s��i�벼�H���W��[n]����H�ҥi�벼:[N]",
	    inbuf, 2, LCECHO);
    setbfile(buf, bname, vbuf.limited);
    if (inbuf[0] == 'y') {
	fp = fopen(buf, "w");
	assert(fp);
	//fprintf(fp, "�����벼�]��");
	fclose(fp);
	friend_edit(FRIEND_CANVOTE);
    } else {
	if (dashf(buf))
	    unlink(buf);
    }
    getdata(5, 0,
	    "�O�_���w�벼���G(y)����벼���[n]���]��:[N]",
	    inbuf, 2, LCECHO);
    setbfile(buf, bname, vbuf.limits);
    if (inbuf[0] == 'y') {
	fp = fopen(buf, "w");
	assert(fp);
	do {
	    getdata(6, 0, "���U�ɶ����� (�H'��'�����A0~120)�G", inbuf, 4, DOECHO);
	    closetime = atoi(inbuf);	// borrow variable
	} while (closetime < 0 || closetime > 120);
	fprintf(fp, "%d\n", now - (MONTH_SECONDS * closetime));
	closetime = 0;
#ifdef USE_LOGIN_LIMITS
	do {
	    getdata(6, 0, STR_LOGINDAYS "�U��", inbuf, 6, DOECHO);
	    closetime = atoi(inbuf);	// borrow variable
	} while (closetime < 0);
#endif
	fprintf(fp, "%d\n", closetime);
	do {
	    getdata(6, 0, "�峹�g�ƤU��", inbuf, 6, DOECHO);
	    closetime = atoi(inbuf);	// borrow variable
	} while (closetime < 0);
	fprintf(fp, "%d\n", closetime);
	fclose(fp);
    } else {
	if (dashf(buf))
	    unlink(buf);
    }
    clear();
    getdata(0, 0, "�����벼�i��X�� (�@��T�Q��)�H", inbuf, 4, DOECHO);

    closetime = atoi(inbuf);
    if (closetime <= 0)
	closetime = 1;
    else if (closetime > 30)
	closetime = 30;

    closetime = closetime * DAY_SECONDS + now;
    setbfile(buf, bname, vbuf.control);
    fp = fopen(buf, "w");
    assert(fp);
    fprintf(fp, "000,000\n%d\n", closetime);

    outs("\n�Ш̧ǿ�J�ﶵ, �� ENTER �����]�w");
    num = 0;
    x = 0;	/* x is the page number */
    while (!aborted) {
	if( num % 15 == 0 ){
	    for( i = num ; i < num + 15 ; ++i ){
		move((i % 15) + 2, (i / 15) * 40);
		prints(ANSI_COLOR(1;30) "%c)" ANSI_RESET " ", i + 'A');
	    }
	}
	snprintf(buf, sizeof(buf), "%c) ", num + 'A');
	getdata((num % 15) + 2, (num / 15) * 40, buf,
		inbuf, 37, DOECHO);
	if (*inbuf) {
	    fprintf(fp, "%1c) %s\n", (num + 'A'), inbuf);
	    num++;
	}
	if ((*inbuf == '\0' && num >= 1) || x == MAX_VOTE_PAGE)
	    aborted = 1;
	if (num == ITEM_PER_PAGE) {
	    x++;
	    num = 0;
	}
    }
    snprintf(buf, sizeof(buf), "�аݨC�H�̦h�i��X���H([1]��%d): ", x * ITEM_PER_PAGE + num);

    getdata(t_lines - 3, 0, buf, inbuf, 3, DOECHO);

    if (atoi(inbuf) <= 0 || atoi(inbuf) > (x * ITEM_PER_PAGE + num)) {
	inbuf[0] = '1';
	inbuf[1] = 0;
    }

    rewind(fp);
    fprintf(fp, "%3d,%3d\n", x * ITEM_PER_PAGE + num, MAX(1, atoi(inbuf)));
    fclose(fp);

    fhp->bvote++;

    if (substitute_record(fn_board, fhp, sizeof(*fhp), pos) == -1)
	outs(err_board_update);
    reset_board(pos);
    outs("�}�l�벼�F�I");

    return FULLUPDATE;
}

static int
vote_flag(const vote_buffer_t *vbuf, const char *bname, char val)
{
    char            buf[256], flag;
    int             fd, num, size;

    num = usernum - 1;
    setbfile(buf, bname, vbuf->flags);
    if ((fd = open(buf, O_RDWR | O_CREAT, 0600)) == -1)
	return -1;
    size = lseek(fd, 0, SEEK_END);
    memset(buf, 0, sizeof(buf));
    while (size <= num) {
	write(fd, buf, sizeof(buf));
	size += sizeof(buf);
    }
    lseek(fd, num, SEEK_SET);
    read(fd, &flag, 1);
    if (flag == 0 && val != 0) {
	lseek(fd, num, SEEK_SET);
	write(fd, &val, 1);
    }
    close(fd);
    return flag;
}

static int
user_vote_one(const vote_buffer_t *vbuf, const char *bname)
{
    FILE           *cfp;
    char            buf[STRLEN], redo;
    boardheader_t  *fhp;
    short	    count, tickets;
    short	    curr_page, item_num, max_page;
    char            inbuf[80], choices[ITEM_PER_PAGE+1], vote[4], *chosen;
    time4_t         closetime;

    // bid = boaord id, must be at least one int.
    // fd should be int.
    int		    bid = 0, i = 0;
    int		    fd = 0;

    // initialize board info
    if ((bid = getbnum(bname)) <= 0)
	return 0;
    assert(0<=bid-1 && bid-1<MAX_BOARD);
    fhp = getbcache(bid);

    setbfile(buf, bname, vbuf->limited);	/* Ptt */
    if (dashf(buf)) {
	setbfile(buf, bname, FN_CANVOTE);
	if (!file_exist_record(buf, cuser.userid)) {
	    vmsg("�藍�_! �o�O�p�H�벼..�A�èS�����ܭ�!");
	    return FULLUPDATE;
	} else {
	    vmsg("���ߧA���ܦ����p�H�벼.   <�˵��������ܦW��>");
	    more(buf, YEA);
	}
    }
    setbfile(buf, bname, vbuf->limits);
    if (dashf(buf)) {
	int limits_logins, limits_posts;
	FILE * lfp = fopen(buf, "r");
	assert(lfp);
	fscanf(lfp, "%d%d%d", &closetime, &limits_logins, &limits_posts);
	fclose(lfp);
	// XXX if this is a private vote (limited), I think we don't need to check limits?
	if (cuser.firstlogin > closetime || cuser.numposts < limits_posts ||
		cuser.numlogindays < limits_logins) {
	    vmsg("�A������`��I");
	    return FULLUPDATE;
	}
    }
    if (vote_flag(vbuf, bname, '\0')) {
	vmsg("�����벼�A�A�w��L�F�I");
	return FULLUPDATE;
    }
    setutmpmode(VOTING);
    setbfile(buf, bname, vbuf->desc);
    more(buf, YEA);

    vs_hdr("�벼�c");

    setbfile(buf, bname, vbuf->control);
    cfp = fopen(buf, "r");
    if (!cfp)
    {
	vmsg("��p�A���벼�w�����εL�ġC");
	return FULLUPDATE;
    }

    assert(cfp);
    fscanf(cfp, "%hd,%hd\n%d\n", &item_num, &tickets, &closetime);
    chosen = (char *)malloc(item_num+100); // XXX dirty fix �O�D�W�[�ﶵ�����D
    memset(chosen, 0, item_num+100);
    memset(choices, 0, sizeof(choices));
    max_page = (item_num - 1)/ ITEM_PER_PAGE + 1;

    outs("�벼�覡�G�T�w�n�z����ܫ�A��J��N�X(A, B, C...)�Y�i�C\n");
    prints("�����벼�A�i�H�� %1hd ���C�� 0 �����벼, 1 �����벼, "
	    "> �U�@��, < �W�@��\n�����벼�N������G%s \n\n",
	   tickets, Cdate(&closetime));

#define REDO_DRAW	1
#define REDO_SCAN	2
    redo = REDO_DRAW;
    curr_page = 0;
    while (1) {

	if (redo) {
	    int i, j;
	    move(5, 0);
	    clrtobot();

	    /* �Q����n��k �]�����Q���Ū�i memory
	     * �ӥB�j�������벼���|�W�L�@�� �ҥH�A�q�ɮ� scan */
	    /* XXX ���@�b�O�D�W�[�ﶵ�h chosen �Ӥp */
	    if (redo & REDO_SCAN) {
		for (i = 0; i < curr_page; i++)
		    for (j = 0; j < ITEM_PER_PAGE; j++)
			fgets(inbuf, sizeof(inbuf), cfp);
	    }

	    count = 0;
	    for (i = 0; i < ITEM_PER_PAGE && fgets(inbuf, sizeof(inbuf), cfp); i++) {
		chomp(inbuf);
		move((count % 15) + 5, (count / 15) * 40);
		prints("%c%s", chosen[curr_page * ITEM_PER_PAGE + i] ? '*' : ' ',
			inbuf);
		choices[count % ITEM_PER_PAGE] = inbuf[0];
		count++;
	    }
	    redo = 0;
	}

	vote[0] = vote[1] = '\0';
	move(t_lines - 2, 0);
	prints("�A�٥i�H�� %2d ��   [ �ثe�Ҧb���� %2d / �@ %2d �� (�i��J '<' '>' ����) ]", tickets - i, curr_page + 1, max_page);
	getdata(t_lines - 4, 0, "��J�z�����: ", vote, sizeof(vote), DOECHO);
	*vote = toupper(*vote);

#define CURRENT_CHOICE \
    chosen[curr_page * ITEM_PER_PAGE + vote[0] - 'A']
	if (vote[0] == '0' || (!vote[0] && !i)) {
	    outs("�O�o�A�ӧ��!!");
	    break;
	} else if (vote[0] == '1' && i);
	else if (!vote[0])
	    continue;
	else if (vote[0] == '<' && max_page > 1) {
	    curr_page = (curr_page + max_page - 1) % max_page;
    	    rewind(cfp);
	    fgets(inbuf, sizeof(inbuf), cfp);
    	    fgets(inbuf, sizeof(inbuf), cfp);
	    redo = REDO_DRAW | REDO_SCAN;
	    continue;
	}
	else if (vote[0] == '>' && max_page > 1) {
	    curr_page = (curr_page + 1) % max_page;
	    if (curr_page == 0) {
		rewind(cfp);
		fgets(inbuf, sizeof(inbuf), cfp);
		fgets(inbuf, sizeof(inbuf), cfp);
	    }
	    redo = REDO_DRAW;
	    continue;
	}
	else if (index(choices, vote[0]) == NULL)	/* �L�� */
	    continue;
	else if ( CURRENT_CHOICE ) { /* �w�� */
	    move(((vote[0] - 'A') % 15) + 5, (((vote[0] - 'A')) / 15) * 40);
	    outc(' ');
	    CURRENT_CHOICE = 0;
	    i--;
	    continue;
	} else {
	    if (i == tickets)
		continue;
	    move(((vote[0] - 'A') % 15) + 5, (((vote[0] - 'A')) / 15) * 40);
	    outc('*');
	    CURRENT_CHOICE = 1;
	    i++;
	    continue;
	}

	if (vote_flag(vbuf, bname, vote[0]) != 0)
	    outs("���Ƨ벼! �����p���C");
	else {
	    setbfile(buf, bname, vbuf->ballots);
	    if ((fd = open(buf, O_WRONLY | O_CREAT | O_APPEND, 0600)) == 0)
		outs("�L�k��J���o\n");
	    else {
		struct stat     statb;
		char            buf[3], mycomments[3][74], b_comments[80];

		for (i = 0; i < 3; i++)
		    strlcpy(mycomments[i], "\n", sizeof(mycomments[i]));

		flock(fd, LOCK_EX);
		for (count = 0; count < item_num; count++) {
		    if (chosen[count])
			write(fd, &count, sizeof(short));
		}
		flock(fd, LOCK_UN);
		fstat(fd, &statb);
		close(fd);
		getdata(b_lines - 2, 0,
			"�z��o���벼�������_�Q���N���ܡH(y/n)[N]",
			buf, 3, DOECHO);
		if (buf[0] == 'Y' || buf[0] == 'y') {
		    do {
			move(5, 0);
			clrtobot();
			outs("�аݱz��o���벼�������_�Q���N���H"
			     "�̦h�T��A��[Enter]����");
			for (i = 0; (i < 3) &&
			     getdata(7 + i, 0, "�G",
				     mycomments[i], sizeof(mycomments[i]),
				     DOECHO); i++);
			getdata(b_lines - 2, 0, "(S)�x�s (E)���s�ӹL "
				"(Q)�����H[S]", buf, 3, LCECHO);
		    } while (buf[0] == 'e');
		    if (buf[0] == 'q')
			break;
		    setbfile(b_comments, bname, vbuf->comments);
		    if (mycomments[0][0])
		    {
			FILE *fcm = fopen(b_comments, "a");
			if (fcm) {
			    fprintf(fcm,
				    ANSI_COLOR(36) "���ϥΪ�" ANSI_COLOR(1;36) " %s "
				    ANSI_COLOR(;36) "����ĳ�G" ANSI_RESET "\n",
				    cuser.userid);
			    for (i = 0; i < 3; i++)
				fprintf(fcm, "    %s\n", mycomments[i]);
			    fprintf(fcm, "\n");
			    fclose(fcm);
			}
		    }
		}
		move(b_lines - 1, 0);
		outs("�w�����벼�I\n");
	    }
	}
	break;
    }
    free(chosen);
    fclose(cfp);
    pressanykey();
    return FULLUPDATE;
}

static int
user_vote(const char *bname)
{
    int             bid;
    boardheader_t  *bh;
    char            buf[STRLEN];
    FILE           *fp;
    int             i, x = -1;
    char            genbuf[STRLEN];
    char            inbuf[80];
    vote_buffer_t   vbuf;

    if ((bid = getbnum(bname)) <= 0)
	return 0;

    assert(0<=bid-1 && bid-1<MAX_BOARD);
    bh = getbcache(bid);

    clear();

    if (bh->bvote == 0) {
	vmsg("�ثe�èS������벼�|��C");
	return FULLUPDATE;
    }
    if (!HasUserPerm(PERM_LOGINOK)) {
	vmsg("�藍�_! �z���������U�{��, �٨S���벼�v��!");
	return FULLUPDATE;
    }

    // XXX I think such loop is ineffective...
    // According to the creation code, the vote is ranged as [1..MAX_VOTE_NR]
    for (i = 1; i <= MAX_VOTE_NR; i++) {
	votebuf_init(&vbuf, i);
	setbfile(buf, bname, vbuf.control);
	if (!dashf(buf))
	    continue;

	x = i;
	prints("(%d) ", x);

	setbfile(buf, bname, vbuf.title);
	fp = fopen(buf, "r");
	if (fp) {
	    fgets(inbuf, sizeof(inbuf), fp);
	    fclose(fp);
	} else {
	    strlcpy(inbuf, "�L���D", sizeof(inbuf));
	}
	prints("%s\n", inbuf);
    }

    if (x < 0)
	return FULLUPDATE;

    snprintf(buf, sizeof(buf), "�n��X���벼 [%d] ", x);
    getdata(b_lines - 1, 0, buf, genbuf, 4, LCECHO);
    i = atoi(genbuf);

    // x: default (max), i: user selection
    if (i < 1 || i > MAX_VOTE_NR)
	i = x;

    votebuf_init(&vbuf, i);
    return user_vote_one(&vbuf, bname);
}

static int
vote_results(const char *bname)
{
    char            buf[STRLEN];

    setbfile(buf, bname, STR_bv_results);
    if (more(buf, YEA) == -1)
	vmsg("�ثe�S������벼�����G�C");
    return FULLUPDATE;
}

int
b_vote_maintain(void)
{
    return vote_maintain(currboard);
}

int
b_vote(void)
{
    return user_vote(currboard);
}

int
b_results(void)
{
    return vote_results(currboard);
}
