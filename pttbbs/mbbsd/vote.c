/* $Id$ */
#include "bbs.h"

static char     STR_bv_control[] = "control";	/* �벼��� �ﶵ */
static char     STR_bv_desc[] = "desc";	/* �벼�ت� */
static char     STR_bv_ballots[] = "ballots";
static char     STR_bv_flags[] = "flags";
static char     STR_bv_comments[] = "comments";	/* �벼�̪���ĳ */
static char     STR_bv_limited[] = "limited";	/* �p�H�벼 */
static char     STR_bv_title[] = "vtitle";

static char     STR_bv_results[] = "results";

static char     STR_new_control[] = "control0\0";	/* �벼��� �ﶵ */
static char     STR_new_desc[] = "desc0\0";	/* �벼�ت� */
static char     STR_new_ballots[] = "ballots0\0";
static char     STR_new_flags[] = "flags0\0";
static char     STR_new_comments[] = "comments0\0";	/* �벼�̪���ĳ */
static char     STR_new_limited[] = "limited0\0";	/* �p�H�벼 */
static char     STR_new_title[] = "vtitle0\0";

void
b_suckinfile(FILE * fp, char *fname)
{
    FILE           *sfp;

    if ((sfp = fopen(fname, "r"))) {
	char            inbuf[256];

	while (fgets(inbuf, sizeof(inbuf), sfp))
	    fputs(inbuf, fp);
	fclose(sfp);
    }
}

static void
b_count(char *buf, int counts[], int *total)
{
    char            inchar;
    int             fd;

    memset(counts, 0, 31 * sizeof(counts[0]));
    *total = 0;
    if ((fd = open(buf, O_RDONLY)) != -1) {
	flock(fd, LOCK_EX);	/* Thor: ����h�H�P�ɺ� */
	while (read(fd, &inchar, 1) == 1) {
	    counts[(int)(inchar - 'A')]++;
	    (*total)++;
	}
	flock(fd, LOCK_UN);
	close(fd);
    }
}


static int
b_nonzeroNum(char *buf)
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
vote_report(char *bname, char *fname, char *fpath)
{
    register char  *ip;
    time_t          dtime;
    int             fd, bid;
    fileheader_t    header;

    ip = fpath;
    while (*(++ip));
    *ip++ = '/';

    /* get a filename by timestamp */

    dtime = now;
    for (;;) {
	sprintf(ip, "M.%ld.A", ++dtime);
	fd = open(fpath, O_CREAT | O_EXCL | O_WRONLY, 0644);
	if (fd >= 0)
	    break;
	dtime++;
    }
    close(fd);

    unlink(fpath);
    link(fname, fpath);

    /* append record to .DIR */

    memset(&header, 0, sizeof(fileheader_t));
    strlcpy(header.owner, "[�������l]", sizeof(header.owner));
    snprintf(header.title, sizeof(header.title), "[%s] �ݪO �ﱡ����", bname);
    {
	register struct tm *ptime = localtime(&dtime);

	snprintf(header.date, sizeof(header.date),
		 "%2d/%02d", ptime->tm_mon + 1, ptime->tm_mday);
    }
    strlcpy(header.filename, ip, sizeof(header.filename));

    strcpy(ip, ".DIR");
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
b_result_one(boardheader_t * fh, int ind, int *total)
{
    FILE           *cfp, *tfp, *frp, *xfp;
    char           *bname;
    char            buf[STRLEN];
    char            inbuf[80];
    int             counts[31];
    int             num, pnum;
    int             junk;
    char            b_control[64];
    char            b_newresults[64];
    char            b_report[64];
    time_t          closetime;

    fh->bvote--;

    if (fh->bvote == 0)
	fh->bvote = 2;
    else if (fh->bvote == 2)
	fh->bvote = 1;

    if (ind) {
	snprintf(STR_new_ballots, sizeof(STR_new_ballots), "%s%d", STR_bv_ballots, ind);
	snprintf(STR_new_control, sizeof(STR_new_control),"%s%d", STR_bv_control, ind);
	snprintf(STR_new_desc, sizeof(STR_new_desc), "%s%d", STR_bv_desc, ind);
	snprintf(STR_new_flags, sizeof(STR_new_flags), "%s%d", STR_bv_flags, ind);
	snprintf(STR_new_comments, sizeof(STR_new_comments), "%s%d", STR_bv_comments, ind);
	snprintf(STR_new_limited, sizeof(STR_new_limited), "%s%d", STR_bv_limited, ind);
	snprintf(STR_new_title, sizeof(STR_new_title), "%s%d", STR_bv_title, ind);
    } else {
	strlcpy(STR_new_ballots, STR_bv_ballots, sizeof(STR_new_ballots));
	strlcpy(STR_new_control, STR_bv_control, sizeof(STR_new_control));
	strlcpy(STR_new_desc, STR_bv_desc, sizeof(STR_new_desc));
	strlcpy(STR_new_flags, STR_bv_flags, sizeof(STR_new_flags));
	strlcpy(STR_new_comments, STR_bv_comments, sizeof(STR_new_comments));
	strlcpy(STR_new_limited, STR_bv_limited, sizeof(STR_new_limited));
	strlcpy(STR_new_title, STR_bv_title, sizeof(STR_new_title));
    }

    bname = fh->brdname;

    setbfile(buf, bname, STR_new_control);
    cfp = fopen(buf, "r");
    fscanf(cfp, "%d\n%lu\n", &junk, &closetime);
    fclose(cfp);

    setbfile(b_control, bname, "tmp");
    if (rename(buf, b_control) == -1)
	return;
    setbfile(buf, bname, STR_new_flags);
    pnum = b_nonzeroNum(buf);
    unlink(buf);
    setbfile(buf, bname, STR_new_ballots);
    b_count(buf, counts, total);
    unlink(buf);

    setbfile(b_newresults, bname, "newresults");
    if ((tfp = fopen(b_newresults, "w")) == NULL)
	return;

    setbfile(buf, bname, STR_new_title);

    if ((xfp = fopen(buf, "r"))) {
	fgets(inbuf, sizeof(inbuf), xfp);
	fprintf(tfp, "%s\n�� �벼�W��: %s\n\n", msg_seperator, inbuf);
	fclose(xfp);
    }
    fprintf(tfp, "%s\n�� �벼�����: %s\n\n�� �����D�شy�z:\n\n",
	    msg_seperator, ctime(&closetime));
    fh->vtime = now;

    setbfile(buf, bname, STR_new_desc);

    b_suckinfile(tfp, buf);
    unlink(buf);

    if ((cfp = fopen(b_control, "r"))) {
	fgets(inbuf, sizeof(inbuf), cfp);
	fgets(inbuf, sizeof(inbuf), cfp);
	fprintf(tfp, "\n���벼���G:(�@�� %d �H�벼,�C�H�̦h�i�� %d ��)\n",
		pnum, junk);
	fprintf(tfp, "    ��    ��                                   �`���� �o���v   �o������\n");
	while (fgets(inbuf, sizeof(inbuf), cfp)) {
	    inbuf[(strlen(inbuf) - 1)] = '\0';
	    num = counts[inbuf[0] - 'A'];
	    fprintf(tfp, "    %-42s %3d ��   %02.2f%%   %02.2f%%\n", inbuf + 3, num,                
		    (float)(num * 100) / (float)(pnum),
		    (float)(num * 100) / (float)(*total));
	}
	fclose(cfp);
    }
    unlink(b_control);

    fprintf(tfp, "%s\n�� �ϥΪ̫�ĳ�G\n\n", msg_seperator);
    setbfile(buf, bname, STR_new_comments);
    b_suckinfile(tfp, buf);
    unlink(buf);

    fprintf(tfp, "%s\n�� �`���� = %d ��\n\n", msg_seperator, *total);
    fclose(tfp);

    setbfile(b_report, bname, "report");
    if ((frp = fopen(b_report, "w"))) {
	b_suckinfile(frp, b_newresults);
	fclose(frp);
    }
    snprintf(inbuf, sizeof(inbuf), "boards/%c/%s", bname[0], bname);
    vote_report(bname, b_report, inbuf);
    if (!(fh->brdattr & BRD_NOCOUNT)) {
	snprintf(inbuf, sizeof(inbuf), "boards/%c/%s", 'R', "Record");
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
    time_t          closetime;
    int             i, total;
    char            buf[STRLEN];
    char            temp[STRLEN];

    for (i = 0; i < 20; i++) {
	if (i)
	    snprintf(STR_new_control, sizeof(STR_new_control), "%s%d", STR_bv_control, i);
	else
	    strlcpy(STR_new_control, STR_bv_control, sizeof(STR_new_control));

	setbfile(buf, fh->brdname, STR_new_control);
	cfp = fopen(buf, "r");
	if (!cfp)
	    continue;
	fgets(temp, sizeof(temp), cfp);
	fscanf(cfp, "%lu\n", &closetime);
	fclose(cfp);
	if (closetime < now)
	    b_result_one(fh, i, &total);
    }
}

static int
b_close(boardheader_t * fh)
{

    if (fh->bvote == 2) {
	if (fh->vtime < now - 3 * 86400) {
	    fh->bvote = 0;
	    return 1;
	} else
	    return 0;
    }
    b_result(fh);
    return 1;
}

int
b_closepolls()
{
    char    *fn_vote_polling = ".polling";
    boardheader_t  *fhp;
    FILE           *cfp;
    int             pos, dirty;
    time_t          last;
    char            timebuf[100];

    /* Edited by CharlieL for can't auto poll bug */

    if ((cfp = fopen(fn_vote_polling, "r"))) {
	fgets(timebuf, 100 * sizeof(char), cfp);
	sscanf(timebuf, "%lu", &last);
	fclose(cfp);
	if (last + 3600 >= now)
	    return 0;
    }
    if ((cfp = fopen(fn_vote_polling, "w")) == NULL)
	return 0;
    fprintf(cfp, "%lu\n%s\n", now, ctime(&now));
    fclose(cfp);

    dirty = 0;
    for (fhp = bcache, pos = 1; pos <= numboards; fhp++, pos++) {
	if (fhp->bvote && b_close(fhp)) {
	    if (substitute_record(fn_board, fhp, sizeof(*fhp), pos) == -1)
		outs(err_board_update);
	    dirty = 1;
	}
    }
    if (dirty)			/* vote flag changed */
	reset_board(pos);

    return 0;
}

static int
vote_view(char *bname, int index)
{
    boardheader_t  *fhp;
    FILE           *fp;
    char            buf[STRLEN], genbuf[STRLEN], inbuf[STRLEN];
    struct stat     stbuf;
    int             fd, num = 0, i, pos, counts[31], total;
    time_t          closetime;

    if (index) {
	snprintf(STR_new_ballots, sizeof(STR_new_ballots),"%s%d", STR_bv_ballots, index);
	snprintf(STR_new_control, sizeof(STR_new_control), "%s%d", STR_bv_control, index);
	snprintf(STR_new_desc, sizeof(STR_new_desc), "%s%d", STR_bv_desc, index);
	snprintf(STR_new_flags, sizeof(STR_new_flags), "%s%d", STR_bv_flags, index);
	snprintf(STR_new_comments, sizeof(STR_new_comments), "%s%d", STR_bv_comments, index);
	snprintf(STR_new_limited, sizeof(STR_new_limited), "%s%d", STR_bv_limited, index);
	snprintf(STR_new_title, sizeof(STR_new_title), "%s%d", STR_bv_title, index);
    } else {
	strlcpy(STR_new_ballots, STR_bv_ballots, sizeof(STR_new_ballots));
	strlcpy(STR_new_control, STR_bv_control, sizeof(STR_new_control));
	strlcpy(STR_new_desc, STR_bv_desc, sizeof(STR_new_desc));
	strlcpy(STR_new_flags, STR_bv_flags, sizeof(STR_new_flags));
	strlcpy(STR_new_comments, STR_bv_comments, sizeof(STR_new_comments));
	strlcpy(STR_new_limited, STR_bv_limited, sizeof(STR_new_limited));
	strlcpy(STR_new_title, STR_bv_title, sizeof(STR_new_title));
    }

    setbfile(buf, bname, STR_new_ballots);
    if ((fd = open(buf, O_RDONLY)) > 0) {
	fstat(fd, &stbuf);
	close(fd);
    } else
	stbuf.st_size = 0;

    setbfile(buf, bname, STR_new_title);
    move(0, 0);
    clrtobot();

    if ((fp = fopen(buf, "r"))) {
	fgets(inbuf, sizeof(inbuf), fp);
	prints("\n�벼�W��: %s", inbuf);
	fclose(fp);
    }
    setbfile(buf, bname, STR_new_control);
    fp = fopen(buf, "r");
    fgets(inbuf, sizeof(inbuf), fp);
    fscanf(fp, "%lu\n", &closetime);

    prints("\n�� �w���벼����: �C�H�̦h�i�� %d ��,�ثe�@�� %d ��,\n"
	   "�����벼�N������ %s", atoi(inbuf), (int)stbuf.st_size,
	   ctime(&closetime));

    /* Thor: �}�� ���� �w�� */
    setbfile(buf, bname, STR_new_flags);
    num = b_nonzeroNum(buf);

    setbfile(buf, bname, STR_new_ballots);
    b_count(buf, counts, &total);

    prints("�@�� %d �H�벼\n", num);
    total = 0;

    while (fgets(inbuf, sizeof(inbuf), fp)) {
	inbuf[(strlen(inbuf) - 1)] = '\0';
	inbuf[30] = '\0';	/* truncate */
	i = inbuf[0] - 'A';
	num = counts[i];
	move(i % 15 + 6, i / 15 * 40);
	prints("  %-32s%3d ��", inbuf, num);
	total += num;
    }
    fclose(fp);
    pos = getbnum(bname);
    fhp = bcache + pos - 1;
    move(t_lines - 3, 0);
    prints("�� �ثe�`���� = %d ��", total);
    getdata(b_lines - 1, 0, "(A)�����벼 (B)�����}�� (C)�~��H[C] ", genbuf,
	    4, LCECHO);
    if (genbuf[0] == 'a') {
	setbfile(buf, bname, STR_new_control);
	unlink(buf);
	setbfile(buf, bname, STR_new_flags);
	unlink(buf);
	setbfile(buf, bname, STR_new_ballots);
	unlink(buf);
	setbfile(buf, bname, STR_new_desc);
	unlink(buf);
	setbfile(buf, bname, STR_new_limited);
	unlink(buf);
	setbfile(buf, bname, STR_new_title);
	unlink(buf);

	if (fhp->bvote)
	    fhp->bvote--;
	if (fhp->bvote == 2)
	    fhp->bvote = 1;

	if (substitute_record(fn_board, fhp, sizeof(*fhp), pos) == -1)
	    outs(err_board_update);
	reset_board(pos);
    } else if (genbuf[0] == 'b') {
	b_result_one(fhp, index, &total);
	if (substitute_record(fn_board, fhp, sizeof(*fhp), pos) == -1)
	    outs(err_board_update);

	reset_board(pos);
    }
    return FULLUPDATE;
}

static int
vote_view_all(char *bname)
{
    int             i;
    int             x = -1;
    FILE           *fp, *xfp;
    char            buf[STRLEN], genbuf[STRLEN];
    char            inbuf[80];

    strlcpy(STR_new_control, STR_bv_control, sizeof(STR_new_control));
    strlcpy(STR_new_title, STR_bv_title, sizeof(STR_new_title));
    setbfile(buf, bname, STR_new_control);
    move(0, 0);
    if ((fp = fopen(buf, "r"))) {
	prints("(0) ");
	x = 0;
	fclose(fp);

	setbfile(buf, bname, STR_new_title);
	if ((xfp = fopen(buf, "r"))) {
	    fgets(inbuf, sizeof(inbuf), xfp);
	    fclose(xfp);
	} else
	    strlcpy(inbuf, "�L���D", sizeof(inbuf));
	prints("%s\n", inbuf);
    }
    for (i = 1; i < 20; i++) {
	snprintf(STR_new_control, sizeof(STR_new_control),
		 "%s%d", STR_bv_control, i);
	snprintf(STR_new_title, sizeof(STR_new_title),
		 "%s%d", STR_bv_title, i);
	setbfile(buf, bname, STR_new_control);
	if ((fp = fopen(buf, "r"))) {
	    prints("(%d) ", i);
	    x = i;
	    fclose(fp);

	    setbfile(buf, bname, STR_new_title);
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


    if (atoi(genbuf) < 0 || atoi(genbuf) > 20)
	snprintf(genbuf, sizeof(genbuf), "%d", x);
    if (genbuf[0] != '0')
	snprintf(STR_new_control, sizeof(STR_new_control),
		 "%s%d", STR_bv_control, atoi(genbuf));
    else
	strlcpy(STR_new_control, STR_bv_control, sizeof(STR_new_control));

    setbfile(buf, bname, STR_new_control);

    if ((fp = fopen(buf, "r"))) { // TODO try access()
	fclose(fp);
	return vote_view(bname, atoi(genbuf));
    } else
	return FULLUPDATE;
}

static int
vote_maintain(char *bname)
{
    FILE           *fp = NULL;
    char            inbuf[STRLEN], buf[STRLEN];
    int             num = 0, aborted, pos, x, i;
    time_t          closetime;
    boardheader_t  *fhp;
    char            genbuf[4];

    if (!(currmode & MODE_BOARD))
	return 0;
    if ((pos = getbnum(bname)) <= 0)
	return 0;

    stand_title("�|��벼");
    fhp = bcache + pos - 1;

    /* CharlieL */
    if (fhp->bvote != 2 && fhp->bvote != 0) {
	getdata(b_lines - 1, 0,
		"(V)�[��ثe�벼 (M)�|��s�벼 (A)�����Ҧ��벼 (Q)�~�� [Q]",
		genbuf, 4, LCECHO);
	if (genbuf[0] == 'v')
	    return vote_view_all(bname);
	else if (genbuf[0] == 'a') {
	    fhp->bvote = 0;

	    setbfile(buf, bname, STR_bv_control);
	    unlink(buf);
	    setbfile(buf, bname, STR_bv_flags);
	    unlink(buf);
	    setbfile(buf, bname, STR_bv_ballots);
	    unlink(buf);
	    setbfile(buf, bname, STR_bv_desc);
	    unlink(buf);
	    setbfile(buf, bname, STR_bv_limited);
	    unlink(buf);
	    setbfile(buf, bname, STR_bv_title);
	    unlink(buf);

	    for (i = 1; i < 20; i++) {
		snprintf(STR_new_ballots, sizeof(STR_new_ballots),
			 "%s%d", STR_bv_ballots, i);
		snprintf(STR_new_control, sizeof(STR_new_control),
			 "%s%d", STR_bv_control, i);
		snprintf(STR_new_desc, sizeof(STR_new_desc),
			 "%s%d", STR_bv_desc, i);
		snprintf(STR_new_flags, sizeof(STR_new_flags),
			 "%s%d", STR_bv_flags, i);
		snprintf(STR_new_comments, sizeof(STR_new_comments),
			 "%s%d", STR_bv_comments, i);
		snprintf(STR_new_limited, sizeof(STR_new_limited),
			 "%s%d", STR_bv_limited, i);
		snprintf(STR_new_title, sizeof(STR_new_title),
			 "%s%d", STR_bv_title, i);

		setbfile(buf, bname, STR_new_control);
		unlink(buf);
		setbfile(buf, bname, STR_new_flags);
		unlink(buf);
		setbfile(buf, bname, STR_new_ballots);
		unlink(buf);
		setbfile(buf, bname, STR_new_desc);
		unlink(buf);
		setbfile(buf, bname, STR_new_limited);
		unlink(buf);
		setbfile(buf, bname, STR_new_title);
		unlink(buf);
	    }
	    if (substitute_record(fn_board, fhp, sizeof(*fhp), pos) == -1)
		outs(err_board_update);

	    return FULLUPDATE;
	} else if (genbuf[0] != 'm' || fhp->bvote >= 20)
	    return FULLUPDATE;
    }
    strlcpy(STR_new_control, STR_bv_control, sizeof(STR_new_control));
    setbfile(buf, bname, STR_new_control);
    x = 0;
    while (x < 20 && (fp = fopen(buf, "r")) != NULL) { // TODO try access()
	fclose(fp);
	x++;
	snprintf(STR_new_control, sizeof(STR_new_control),
		 "%s%d", STR_bv_control, x);
	setbfile(buf, bname, STR_new_control);
    }
    if (x >= 20)
	return FULLUPDATE;
    if (x) {
	snprintf(STR_new_ballots, sizeof(STR_new_ballots), "%s%d", STR_bv_ballots, x);
	snprintf(STR_new_control, sizeof(STR_new_control), "%s%d", STR_bv_control, x);
	snprintf(STR_new_desc, sizeof(STR_new_desc), "%s%d", STR_bv_desc, x);
	snprintf(STR_new_flags, sizeof(STR_new_flags), "%s%d", STR_bv_flags, x);
	snprintf(STR_new_comments, sizeof(STR_new_comments), "%s%d", STR_bv_comments, x);
	snprintf(STR_new_limited, sizeof(STR_new_limited), "%s%d", STR_bv_limited, x);
	snprintf(STR_new_title, sizeof(STR_new_title), "%s%d", STR_bv_title, x);
    } else {
	strlcpy(STR_new_ballots, STR_bv_ballots, sizeof(STR_new_ballots));
	strlcpy(STR_new_control, STR_bv_control, sizeof(STR_new_control));
	strlcpy(STR_new_desc, STR_bv_desc, sizeof(STR_new_desc));
	strlcpy(STR_new_flags, STR_bv_flags, sizeof(STR_new_flags));
	strlcpy(STR_new_comments, STR_bv_comments, sizeof(STR_new_comments));
	strlcpy(STR_new_limited, STR_bv_limited, sizeof(STR_new_limited));
	strlcpy(STR_new_title, STR_bv_title, sizeof(STR_new_title));
    }
    clear();
    move(0, 0);
    prints("�� %d ���벼\n", x);
    setbfile(buf, bname, STR_new_title);
    getdata(4, 0, "�п�J�벼�W��:", inbuf, 50, LCECHO);
    if (inbuf[0] == '\0')
	strlcpy(inbuf, "�����W��", sizeof(inbuf));
    fp = fopen(buf, "w");
    assert(fp);
    fprintf(fp, "%s", inbuf);
    fclose(fp);

    prints("��������}�l�s�覹�� [�벼�v��]");
    pressanykey();
    setbfile(buf, bname, STR_new_desc);
    aborted = vedit(buf, NA, NULL);
    if (aborted == -1) {
	clear();
	outs("���������벼");
	pressanykey();
	return FULLUPDATE;
    }
    aborted = 0;
    setbfile(buf, bname, STR_new_flags);
    unlink(buf);

    getdata(4, 0,
	    "�O�_���w�벼�̦W��G(y)�s��i�벼�H���W��[n]����H�ҥi�벼:[N]",
	    inbuf, 2, LCECHO);
    setbfile(buf, bname, STR_new_limited);
    if (inbuf[0] == 'y') {
	fp = fopen(buf, "w");
	assert(fp);
	fprintf(fp, "�����벼�]��");
	fclose(fp);
	friend_edit(FRIEND_CANVOTE);
    } else {
	if (dashf(buf))
	    unlink(buf);
    }
    clear();
    getdata(0, 0, "�����벼�i��X�� (�@��Q��)�H", inbuf, 4, DOECHO);

    closetime = atoi(inbuf);
    if (closetime <= 0)
	closetime = 1;
    else if (closetime > 10)
	closetime = 10;

    closetime = closetime * 86400 + now;
    setbfile(buf, bname, STR_new_control);
    fp = fopen(buf, "w");
    assert(fp);
    fprintf(fp, "00\n%lu\n", closetime);

    outs("\n�Ш̧ǿ�J�ﶵ, �� ENTER �����]�w");
    num = 0;
    while (!aborted) {
	if( num % 15 == 0 ){
	    for( i = num ; i < num + 15 ; ++i ){
		snprintf(buf, sizeof(buf), "\033[1;30m%c)\033[m ", i + 'A');
		move((i % 15) + 2, (i / 15) * 40);
		prints(buf);
	    }
	    refresh();
	}
	snprintf(buf, sizeof(buf), "%c) ", num + 'A');
	getdata((num % 15) + 2, (num / 15) * 40, buf,
		inbuf, 37, DOECHO);
	if (*inbuf) {
	    fprintf(fp, "%1c) %s\n", (num + 'A'), inbuf);
	    num++;
	}
	if ((*inbuf == '\0' && num >= 1) || num == 30)
	    aborted = 1;
    }
    snprintf(buf, sizeof(buf), "�аݨC�H�̦h�i��X���H([1]��%d): ", num);

    getdata(t_lines - 3, 0, buf, inbuf, 3, DOECHO);

    if (atoi(inbuf) <= 0 || atoi(inbuf) > num)
	strlcpy(inbuf, "1", sizeof(inbuf));

    rewind(fp);
    fprintf(fp, "%2d\n", MAX(1, atoi(inbuf)));
    fclose(fp);

    if (fhp->bvote == 2)
	fhp->bvote = 0;
    else if (fhp->bvote == 1)
	fhp->bvote = 2;
    else if (fhp->bvote == 2)
	fhp->bvote = 1;

    fhp->bvote++;

    if (substitute_record(fn_board, fhp, sizeof(*fhp), pos) == -1)
	outs(err_board_update);
    reset_board(pos);
    outs("�}�l�벼�F�I");

    return FULLUPDATE;
}

static int
vote_flag(char *bname, int index, char val)
{
    char            buf[256], flag;
    int             fd, num, size;

    if (index)
	snprintf(STR_new_flags, sizeof(STR_new_flags), "%s%d", STR_bv_flags, index);
    else
	strlcpy(STR_new_flags, STR_bv_flags, sizeof(STR_new_flags));

    num = usernum - 1;
    setbfile(buf, bname, STR_new_flags);
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
same(char compare, char list[], int num)
{
    int             n;
    int             rep = 0;

    for (n = 0; n < num; n++) {
	if (compare == list[n])
	    rep = 1;
	if (rep == 1)
	    list[n] = list[n + 1];
    }
    return rep;
}

static int
user_vote_one(char *bname, int ind)
{
    FILE           *cfp, *fcm;
    char            buf[STRLEN];
    boardheader_t  *fhp;
    int             pos = 0, i = 0, count = 0, tickets, fd;
    char            inbuf[80], choices[31], vote[4], chosen[31];
    time_t          closetime;

    if (ind) {
	snprintf(STR_new_ballots, sizeof(STR_new_ballots), "%s%d", STR_bv_ballots, ind);
	snprintf(STR_new_control, sizeof(STR_new_control), "%s%d", STR_bv_control, ind);
	snprintf(STR_new_desc, sizeof(STR_new_desc), "%s%d", STR_bv_desc, ind);
	snprintf(STR_new_flags, sizeof(STR_new_flags),"%s%d", STR_bv_flags, ind);
	snprintf(STR_new_comments, sizeof(STR_new_comments), "%s%d", STR_bv_comments, ind);
	snprintf(STR_new_limited, sizeof(STR_new_limited), "%s%d", STR_bv_limited, ind);
    } else {
	strlcpy(STR_new_ballots, STR_bv_ballots, sizeof(STR_new_ballots));
	strlcpy(STR_new_control, STR_bv_control, sizeof(STR_new_control));
	strlcpy(STR_new_desc, STR_bv_desc, sizeof(STR_new_desc));
	strlcpy(STR_new_flags, STR_bv_flags, sizeof(STR_new_flags));
	strlcpy(STR_new_comments, STR_bv_comments, sizeof(STR_new_comments));
	strlcpy(STR_new_limited, STR_bv_limited, sizeof(STR_new_limited));
    }

    setbfile(buf, bname, STR_new_control);
    cfp = fopen(buf, "r");
    if (!cfp)
	return FULLUPDATE;

    setbfile(buf, bname, STR_new_limited);	/* Ptt */
    if (dashf(buf)) {
	setbfile(buf, bname, FN_CANVOTE);
	if (!belong(buf, cuser.userid)) {
	    fclose(cfp);
	    outs("\n\n�藍�_! �o�O�p�H�벼..�A�èS�����ܭ�!");
	    pressanykey();
	    return FULLUPDATE;
	} else {
	    outs("\n\n���ߧA���ܦ����p�H�벼....<�����N���˵��������ܦW��>");
	    pressanykey();
	    more(buf, YEA);
	}
    }
    if (vote_flag(bname, ind, '\0')) {
	outs("\n\n�����벼�A�A�w��L�F�I");
	pressanykey();
	return FULLUPDATE;
    }
    setutmpmode(VOTING);
    setbfile(buf, bname, STR_new_desc);
    more(buf, YEA);

    stand_title("�벼�c");
    if ((pos = getbnum(bname)) <= 0)
	return 0;

    fhp = bcache + pos - 1;
    fgets(inbuf, sizeof(inbuf), cfp);
    tickets = atoi(inbuf);
    fscanf(cfp, "%lu\n", &closetime);

    prints("�벼�覡�G�T�w�n�z����ܫ�A��J��N�X(A, B, C...)�Y�i�C\n"
	   "�����벼�A�i�H�� %1d ���C"
	   "�� 0 �����벼 , 1 �����벼\n"
	   "�����벼�N������G%s \n",
	   tickets, ctime(&closetime));
    move(5, 0);
    memset(choices, 0, sizeof(choices));
    memset(chosen, 0, sizeof(chosen));

    while (fgets(inbuf, sizeof(inbuf), cfp)) {
	move((count % 15) + 5, (count / 15) * 40);
	prints(" %s", strtok(inbuf, "\n\0"));
	choices[count++] = inbuf[0];
    }
    fclose(cfp);

    while (1) {
	vote[0] = vote[1] = '\0';
	move(t_lines - 2, 0);
	prints("�A�٥i�H�� %2d ��", tickets - i);
	getdata(t_lines - 4, 0, "��J�z�����: ", vote, sizeof(vote), DOECHO);
	*vote = toupper(*vote);
	if (vote[0] == '0' || (!vote[0] && !i)) {
	    outs("�O���A�ӧ��!!");
	    break;
	} else if (vote[0] == '1' && i);
	else if (!vote[0])
	    continue;
	else if (index(choices, vote[0]) == NULL)	/* �L�� */
	    continue;
	else if (same(vote[0], chosen, i)) {
	    move(((vote[0] - 'A') % 15) + 5, (((vote[0] - 'A')) / 15) * 40);
	    prints(" ");
	    i--;
	    continue;
	} else {
	    if (i == tickets)
		continue;
	    chosen[i] = vote[0];
	    move(((vote[0] - 'A') % 15) + 5, (((vote[0] - 'A')) / 15) * 40);
	    prints("*");
	    i++;
	    continue;
	}

	if (vote_flag(bname, ind, vote[0]) != 0)
	    prints("���Ƨ벼! �����p���C");
	else {
	    setbfile(buf, bname, STR_new_ballots);
	    if ((fd = open(buf, O_WRONLY | O_CREAT | O_APPEND, 0600)) == 0)
		outs("�L�k��J���o\n");
	    else {
		struct stat     statb;
		char            buf[3], mycomments[3][74], b_comments[80];

		for (i = 0; i < 3; i++)
		    strlcpy(mycomments[i], "\n", sizeof(mycomments[i]));

		flock(fd, LOCK_EX);
		for (count = 0; count < 31; count++) {
		    if (chosen[count])
			write(fd, &chosen[count], 1);
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
		    } while (buf[0] == 'E' || buf[0] == 'e');
		    if (buf[0] == 'Q' || buf[0] == 'q')
			break;
		    setbfile(b_comments, bname, STR_new_comments);
		    if (mycomments[0])
			if ((fcm = fopen(b_comments, "a"))) {
			    fprintf(fcm,
				    "\033[36m���ϥΪ�\033[1;36m %s "
				    "\033[;36m����ĳ�G\033[m\n",
				    cuser.userid);
			    for (i = 0; i < 3; i++)
				fprintf(fcm, "    %s\n", mycomments[i]);
			    fprintf(fcm, "\n");
			    fclose(fcm);
			}
		}
		move(b_lines - 1, 0);
		prints("�w�����벼�I\n");
	    }
	}
	break;
    }
    pressanykey();
    return FULLUPDATE;
}

static int
user_vote(char *bname)
{
    int             pos;
    boardheader_t  *fhp;
    char            buf[STRLEN];
    FILE           *fp, *xfp;
    int             i, x = -1;
    char            genbuf[STRLEN];
    char            inbuf[80];

    if ((pos = getbnum(bname)) <= 0)
	return 0;

    fhp = bcache + pos - 1;

    move(0, 0);
    clrtobot();

    if (fhp->bvote == 2 || fhp->bvote == 0) {
	outs("\n\n�ثe�èS������벼�|��C");
	pressanykey();
	return FULLUPDATE;
    }
    if (!HAS_PERM(PERM_LOGINOK)) {
	outs("\n�藍�_! �z�����G�Q��, �٨S���벼�v��!");
	pressanykey();
	return FULLUPDATE;
    }
    strlcpy(STR_new_control, STR_bv_control, sizeof(STR_new_control));
    strlcpy(STR_new_title, STR_bv_title, sizeof(STR_new_title));
    setbfile(buf, bname, STR_new_control);
    move(0, 0);
    if ((fp = fopen(buf, "r"))) {
	prints("(0) ");
	x = 0;
	fclose(fp);

	setbfile(buf, bname, STR_new_title);
	if ((xfp = fopen(buf, "r"))) {
	    fgets(inbuf, sizeof(inbuf), xfp);
	    fclose(xfp);
	} else
	    strlcpy(inbuf, "�L���D", sizeof(inbuf));
	prints("%s\n", inbuf);
    }
    for (i = 1; i < 20; i++) {
	snprintf(STR_new_control, sizeof(STR_new_control),
		 "%s%d", STR_bv_control, i);
	snprintf(STR_new_title, sizeof(STR_new_title),
		 "%s%d", STR_bv_title, i);
	setbfile(buf, bname, STR_new_control);
	if ((fp = fopen(buf, "r"))) {
	    prints("(%d) ", i);
	    x = i;
	    fclose(fp);

	    setbfile(buf, bname, STR_new_title);
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

    snprintf(buf, sizeof(buf), "�n��X���벼 [%d] ", x);

    getdata(b_lines - 1, 0, buf, genbuf, 4, LCECHO);

    if (atoi(genbuf) < 0 || atoi(genbuf) > 20)
	snprintf(genbuf, sizeof(genbuf), "%d", x);

    if (genbuf[0] != '0')
	snprintf(STR_new_control, sizeof(STR_new_control),
		 "%s%d", STR_bv_control, atoi(genbuf));
    else
	strlcpy(STR_new_control, STR_bv_control, sizeof(STR_new_control));

    setbfile(buf, bname, STR_new_control);

    if ((fp = fopen(buf, "r"))) { // TODO try access()
	fclose(fp);

	return user_vote_one(bname, atoi(genbuf));
    } else
	return FULLUPDATE;
}

static int
vote_results(char *bname)
{
    char            buf[STRLEN];

    setbfile(buf, bname, STR_bv_results);
    if (more(buf, YEA) == -1)
	outs("\n�ثe�S������벼�����G�C");
    return FULLUPDATE;
}

int
b_vote_maintain()
{
    return vote_maintain(currboard);
}

int
b_vote()
{
    return user_vote(currboard);
}

int
b_results()
{
    return vote_results(currboard);
}
