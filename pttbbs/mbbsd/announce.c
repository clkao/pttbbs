/* $Id$ */
#include "bbs.h"

#define PATHLEN     256

static char     copyfile[PATHLEN];
static char     copytitle[TTLEN + 1];
static char     copyowner[IDLEN + 2];

void
a_copyitem(char *fpath, char *title, char *owner, int mode)
{
    strcpy(copyfile, fpath);
    strcpy(copytitle, title);
    if (owner)
	strcpy(copyowner, owner);
    else
	*copyowner = 0;
    if (mode) {
	vmsg("�ɮ׼аO�����C[�`�N] ������~��R�����!");
    }
}

#define FHSZ            sizeof(fileheader_t)

static void
a_loadname(menu_t * pm)
{
    char            buf[PATHLEN];
    int             len;

    setadir(buf, pm->path);
    len = get_records(buf, pm->header, FHSZ, pm->page + 1, p_lines);
    if (len < p_lines)
	bzero(&pm->header[len], FHSZ * (p_lines - len));
}

static void
a_timestamp(char *buf, time4_t *time)
{
    struct tm      *pt = localtime4(time);

    sprintf(buf, "%02d/%02d/%02d", pt->tm_mon + 1, pt->tm_mday, (pt->tm_year + 1900) % 100);
}

static void
a_showmenu(menu_t * pm)
{
    char           *title, *editor;
    int             n;
    fileheader_t   *item;
    char            buf[PATHLEN];
    time4_t         dtime;

    showtitle("��ؤ峹", pm->mtitle);
    prints("   \033[1;36m�s��    ��      �D%56s\033[0m",
	   "�s    ��      ��    ��");

    if (pm->num) {
	setadir(buf, pm->path);
	a_loadname(pm);
	for (n = 0; n < p_lines && pm->page + n < pm->num; n++) {
	    item = &pm->header[n];
	    title = item->title;
	    editor = item->owner;
	    /*
	     * Ptt ��ɶ��אּ���ɮ׮ɶ� dtime = atoi(&item->filename[2]);
	     */
	    snprintf(buf, sizeof(buf), "%s/%s", pm->path, item->filename);
	    dtime = dasht(buf);
	    a_timestamp(buf, &dtime);
	    prints("\n%6d%c %-47.46s%-13s[%s]", pm->page + n + 1,
		   (item->filemode & FILE_BM) ? 'X' :
		   (item->filemode & FILE_HIDE) ? ')' : '.',
		   title, editor,
		   buf);
	}
    } else
	outs("\n  �m��ذϡn�|�b�l���Ѧa��������ؤ�... :)");

    move(b_lines, 1);
    outs(pm->level ?
	 "\033[34;46m �i�O  �D�j \033[31;47m  (h)\033[30m����  "
	 "\033[31m(q/��)\033[30m���}  \033[31m(n)\033[30m�s�W�峹  "
	 "\033[31m(g)\033[30m�s�W�ؿ�  \033[31m(e)\033[30m�s���ɮ�  \033[m" :
	 "\033[34;46m �i�\\����j \033[31;47m  (h)\033[30m����  "
	 "\033[31m(q/��)\033[30m���}  \033[31m(k��j��)\033[30m���ʴ��  "
	 "\033[31m(enter/��)\033[30mŪ�����  \033[m");
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
	    a_loadname(pm);
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
    outs("\033[36m�i " BBSNAME "���G��ϥλ��� �j\033[m\n\n"
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
	outs("\n\033[36m�i �O�D�M���� �j\033[m\n"
	     "[H]             ������ ���}/�|��/�O�D �~��\\Ū\n"
	     "[n/g/G]         ������ؤ峹/�}�P�ؿ�/�إ߳s�u\n"
	     "[m/d/D]         ����/�R���峹/�R���@�ӽd�򪺤峹\n"
	     "[f/T/e]         �s����D�Ÿ�/�ק�峹���D/���e\n"
	     "[c/p/a]         ����/�߶K/���[�峹\n"
	     "[^P/^A]         �߶K/���[�w��'t'�аO�峹\n");
    }
    if (level >= SYSOP) {
	outs("\n\033[36m�i �����M���� �j\033[m\n"
	     "[l]             �� symbolic link\n"
	     "[N]             �d���ɦW\n");
    }
    pressanykey();
}

static void
a_forward(char *path, fileheader_t * pitem, int mode)
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
a_additem(menu_t * pm, fileheader_t * myheader)
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
#define ADDLINK         2

static void
a_newitem(menu_t * pm, int mode)
{
    char    *mesg[3] = {
	"[�s�W�峹] �п�J���D�G",	/* ADDITEM */
	"[�s�W�ؿ�] �п�J���D�G",	/* ADDGROUP */
	"�п�J���D�G"		/* ADDLINK */
    };

    char            fpath[PATHLEN], buf[PATHLEN], lpath[PATHLEN];
    fileheader_t    item;
    int             d;

    strlcpy(fpath, pm->path, sizeof(fpath));

    switch (mode) {
    case ADDITEM:
	stampfile(fpath, &item);
	strlcpy(item.title, "�� ", sizeof(item.title));	/* A1BA */
	break;

    case ADDGROUP:
	stampdir(fpath, &item);
	strlcpy(item.title, "�� ", sizeof(item.title));	/* A1BB */
	break;
    case ADDLINK:
	stamplink(fpath, &item);
	if (!getdata(b_lines - 2, 1, "�s�W�s�u�G", buf, 61, DOECHO))
	    return;
	if (invalid_pname(buf)) {
	    unlink(fpath);
	    outs("�ت��a���|���X�k�I");
	    igetch();
	    return;
	}
	item.title[0] = 0;
	// XXX Is it alright?
	// ex: path= "PASSWD"
	for (d = 0; d <= 3; d++) {
	    switch (d) {
	    case 0:
		snprintf(lpath, sizeof(lpath), BBSHOME "/man/boards/%c/%s/%s",
			currboard[0], currboard, buf);
		break;
	    case 1:
		snprintf(lpath, sizeof(lpath), BBSHOME "/man/boards/%c/%s",
			buf[0], buf);
		break;
	    case 2:
		snprintf(lpath, sizeof(lpath), BBSHOME "/%s",
			buf);
		break;
	    case 3:
		snprintf(lpath, sizeof(lpath), BBSHOME "/etc/%s",
			buf);
		break;
	    }
	    if (dashf(lpath)) {
		strlcpy(item.title, "�� ", sizeof(item.title));	/* A1B3 */
		break;
	    } else if (dashd(lpath)) {
		strlcpy(item.title, "�� ", sizeof(item.title));	/* A1B4 */
		break;
	    }
	    if (!HAS_PERM(PERM_BBSADM) && d == 1)
		break;
	}

	if (!item.title[0]) {
	    unlink(fpath);
	    outs("�ت��a���|���X�k�I");
	    igetch();
	    return;
	}
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
	if (vedit(fpath, 0, NULL) == -1) {
	    unlink(fpath);
	    pressanykey();
	    return;
	}
	break;
    case ADDLINK:
	unlink(fpath);
	if (symlink(lpath, fpath) == -1) {
	    outs("�L�k�إ� symbolic link");
	    igetch();
	    return;
	}
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
    char            ans[2];
    int             i;
    fileheader_t    item;

    move(b_lines - 1, 1);
    if (copyfile[0]) {
	if (dashd(copyfile)) {
	    for (i = 0; copyfile[i] && copyfile[i] == pm->path[i]; i++);
	    if (!copyfile[i]) {
		vmsg("�N�ؿ����i�ۤv���l�ؿ����A�|�y���L�a�j��I");
		return;
	    }
	}
	if (mode) {
	    snprintf(buf, sizeof(buf),
		     "�T�w�n����[%s]��(Y/N)�H[N] ", copytitle);
	    getdata(b_lines - 1, 1, buf, ans, sizeof(ans), LCECHO);
	} else
	    ans[0] = 'y';
	if (ans[0] == 'y') {
	    strlcpy(newpath, pm->path, sizeof(newpath));

	    if (*copyowner) {
		char           *fname = strrchr(copyfile, '/');

		if (fname)
		    strcat(newpath, fname);
		else
		    return;
		if (access(pm->path, X_OK | R_OK | W_OK))
		    mkdir(pm->path, 0755);
		memset(&item, 0, sizeof(fileheader_t));
		strlcpy(item.filename, fname + 1, sizeof(item.filename));
		memcpy(copytitle, "��", 2);
		if (HAS_PERM(PERM_BBSADM))
		    Link(copyfile, newpath);
		else {
                    Copy(copyfile, newpath);
		}
	    } else if (dashf(copyfile)) {
		stampfile(newpath, &item);
		memcpy(copytitle, "��", 2);
                Copy(copyfile, newpath);
	    } else if (dashd(copyfile)) {
		stampdir(newpath, &item);
		memcpy(copytitle, "��", 2);
		copy_file(copyfile, newpath);
		system(buf);
	    } else {
		outs("�L�k�����I");
		igetch();
		return;
	    }
	    strlcpy(item.owner, *copyowner ? copyowner : cuser.userid,
		    sizeof(item.owner));
	    strlcpy(item.title, copytitle, sizeof(item.title));
	    a_additem(pm, &item);
	    copyfile[0] = '\0';
	}
    } else {
	outs("�Х����� copy �R�O��A paste");
	igetch();
    }
}

static void
a_appenditem(menu_t * pm, int isask)
{
    char            fname[PATHLEN];
    char            buf[ANSILINELEN];
    char            ans[2] = "y";
    FILE           *fp, *fin;

    move(b_lines - 1, 1);
    if (copyfile[0]) {
	if (dashf(copyfile)) {
	    snprintf(fname, sizeof(fname), "%s/%s", pm->path,
		    pm->header[pm->now - pm->page].filename);
	    if (dashf(fname)) {
		if (isask) {
		    snprintf(buf, sizeof(buf),
			     "�T�w�n�N[%s]���[�󦹶�(Y/N)�H[N] ", copytitle);
		    getdata(b_lines - 2, 1, buf, ans, sizeof(ans), LCECHO);
		}
		if (ans[0] == 'y') {
		    if ((fp = fopen(fname, "a+"))) {
			if ((fin = fopen(copyfile, "r"))) {
			    memset(buf, '-', 74);
			    buf[74] = '\0';
			    fprintf(fp, "\n> %s <\n\n", buf);
			    if (isask)
				getdata(b_lines - 1, 1,
					"�O�_����ñ�W�ɳ���(Y/N)�H[Y] ",
					ans, sizeof(ans), LCECHO);
			    while (fgets(buf, sizeof(buf), fin)) {
				if ((ans[0] == 'n') &&
				    !strcmp(buf, "--\n"))
				    break;
				fputs(buf, fp);
			    }
			    fclose(fin);
			    copyfile[0] = '\0';
			}
			fclose(fp);
		    }
		}
	    } else {
		outs("�ɮפ��o���[�󦹡I");
		igetch();
	    }
	} else {
	    outs("���o���[��ӥؿ����ɮ׫�I");
	    igetch();
	}
    } else {
	outs("�Х����� copy �R�O��A append");
	igetch();
    }
}

static int
a_pastetagpost(menu_t * pm, int mode)
{
    fileheader_t    fhdr;
    boardheader_t  *bh = NULL;
    int             ans = 0, ent = 0, tagnum;
    char            title[TTLEN + 1] = "��  ";
    char            dirname[200], buf[200];

    if (TagBoard == 0){
	sethomedir(dirname, cuser.userid);
    }
    else{
	bh = getbcache(TagBoard);
	setbdir(dirname, bh->brdname);
    }
    tagnum = TagNum;

    if (!tagnum)
	return ans;

    while (tagnum--) {
	EnumTagFhdr(&fhdr, dirname, ent++);
	if (TagBoard == 0) 
	    sethomefile(buf, cuser.userid, fhdr.filename);
	else
	    setbfile(buf, bh->brdname, fhdr.filename);

	if (dashf(buf)) {
	    strncpy(title + 3, fhdr.title, TTLEN - 3);
	    title[TTLEN] = '\0';
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
    char            newnum[4];
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
    char            fname[256];

    snprintf(fname, sizeof(fname), "%s/.DIR", pm->path);
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
	setapath(buf, "deleted");
	setadir(buf, buf);
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
a_newtitle(menu_t * pm)
{
    char            buf[PATHLEN];
    fileheader_t    item;

    memcpy(&item, &pm->header[pm->now - pm->page], FHSZ);
    strlcpy(buf, item.title + 3, sizeof(buf));
    if (getdata_buf(b_lines - 1, 1, "�s���D�G", buf, 60, DOECHO)) {
	strlcpy(item.title + 3, buf, sizeof(item.title) - 3);
	setadir(buf, pm->path);
	substitute_record(buf, &item, FHSZ, pm->now + 1);
    }
}
static void
a_hideitem(menu_t * pm)
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
a_editsign(menu_t * pm)
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
a_showname(menu_t * pm)
{
    char            buf[PATHLEN];
    int             len;
    int             i;
    int             sym;

    move(b_lines - 1, 1);
    snprintf(buf, sizeof(buf),
	     "%s/%s", pm->path, pm->header[pm->now - pm->page].filename);
    if (dashl(buf)) {
	prints("�� symbolic link �W�٬� %s\n",
	       pm->header[pm->now - pm->page].filename);
	if ((len = readlink(buf, buf, PATHLEN - 1)) >= 0) {
	    buf[len] = '\0';
	    for (i = 0; BBSHOME[i] && buf[i] == BBSHOME[i]; i++);
	    if (!BBSHOME[i] && buf[i] == '/') {
		if (HAS_PERM(PERM_BBSADM))
		    sym = 1;
		else {
		    sym = 0;
		    for (i++; BBSHOME "/man"[i] && buf[i] == BBSHOME "/man"[i];
			 i++);
		    if (!BBSHOME "/man"[i] && buf[i] == '/')
			sym = 1;
		}
		if (sym) {
		    vmsg("�� symbolic link ���V %s\n", &buf[i + 1]);
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

#if 0
static char    *a_title;

static void
atitle()
{
    showtitle("��ؤ峹", a_title);
    outs("[��]���} [��]�\\Ū [^P]�o��峹 [b]�Ƨѿ� [d]�R�� [q]��ذ� "
	 "[TAB]��K [h]elp\n\033[7m  �s��   �� ��  �@  ��       "
	 "��  ��  ��  �D\033[m");
}
#endif

static int
isvisible_man(menu_t * me)
{
    fileheader_t   *fhdr = &me->header[me->now - me->page];
    if (me->level < MANAGER && ((fhdr->filemode & FILE_BM) ||
				((fhdr->filemode & FILE_HIDE) &&
				 /* board friend only effact when
				  * in board reading mode             */
				 (currstat == ANNOUNCE ||
				  hbflcheck(currbid, currutmp->uid))
				)))
	return 0;
    return 1;
}
int
a_menu(char *maintitle, char *path, int lastlevel)
{
    static char     Fexit;	// �ΨӸ��X recursion
    menu_t          me;
    char            fname[PATHLEN];
    int             ch, returnvalue = FULLUPDATE;

    trans_buffer[0] = 0;

    Fexit = 0;
    me.header = (fileheader_t *) calloc(p_lines, FHSZ);
    me.path = path;
    strlcpy(me.mtitle, maintitle, sizeof(me.mtitle));
    setadir(fname, me.path);
    me.num = get_num_records(fname, FHSZ);

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

	if (me.now < me.page || me.now >= me.page + p_lines) {
	    me.page = me.now - ((me.page == 10000 && me.now > p_lines / 2) ?
				(p_lines / 2) : (me.now % p_lines));
	    a_showmenu(&me);
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
	    me.now = a_searchtitle(&me, ch == '?');
	    me.page = 9999;
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
		*quote_file = 0;
		if (vedit(fname, NA, NULL) != -1) {
		    char            fpath[200];
		    fileheader_t    fhdr;

		    strlcpy(fpath, path, sizeof(fpath));
		    stampfile(fpath, &fhdr);
		    unlink(fpath);
		    Rename(fname, fpath);
		    strlcpy(me.header[me.now - me.page].filename,
			    fhdr.filename,
			    sizeof(me.header[me.now - me.page].filename));
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
		if( !lastlevel && !HAS_PERM(PERM_SYSOP) &&
		    !is_BM_cache(currbid) && dashd(fname) )
		    vmsg("�u���O�D�~�i�H�����ؿ���!");
		else
		    a_copyitem(fname, me.header[me.now - me.page].title, 0, 1);
		me.page = 9999;
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
		vmsg("%s/%s", &path[11], fhdr->filename);;
#endif
		snprintf(fname, sizeof(fname), "%s/%s", path, fhdr->filename);
		if (*fhdr->filename == 'H' && fhdr->filename[1] == '.') {
		  vmsg("���A�䴩 gopher mode, �Шϥ��s���������s��");
		  vmsg("gopher://%s/1/",fhdr->filename+2);
		} else if (dashf(fname)) {
		    int             more_result;

		    while ((more_result = more(fname, YEA))) {
			/* Ptt �d�����F plugin */
			if (currstat == EDITEXP || currstat == OSONG) {
			    char            ans[4];

			    move(22, 0);
			    clrtoeol();
			    getdata(22, 1,
				    currstat == EDITEXP ?
				    "�n��d�� Plugin ��峹��?[y/N]" :
				    "�T�w�n�I�o���q��?[y/N]",
				    ans, sizeof(ans), LCECHO);
			    if (ans[0] == 'y') {
				strlcpy(trans_buffer,
					fname, sizeof(trans_buffer));
				Fexit = 1;
				if (currstat == OSONG) {
				    /* XXX: �u��q���I�q�i��Ʀ�] */
				    log_file(FN_USSONG, LOG_CREAT | LOG_VF,
					     "%s\n", fhdr->title);
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
		    a_menu(me.header[me.now - me.page].title, fname, me.level);
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
	    snprintf(fname, sizeof(fname),
		     "%s/%s", path, me.header[me.now - me.page].filename);
	    if (me.now < me.num && HAS_PERM(PERM_LOGINOK) && dashf(fname)) {
		a_forward(path, &me.header[me.now - me.page], ch /* == 'U' */ );
		/* By CharlieL */
	    } else
		vmsg("�L�k��H������");

	    me.page = 9999;
	    break;

#ifdef BLOG
	case 'b':
	    if( !HAS_PERM(PERM_SYSOP) && !is_BM_cache(currbid) )
		vmsg("�u���O�D�~�i�H�έ�!");
	    else{
		char    genbuf[128];
		snprintf(genbuf, sizeof(genbuf),
			 "bin/builddb.pl -f -n %d %s", me.now, currboard);
		system(genbuf);
		vmsg("��Ƨ�s����");
	    }
	    me.page = 9999;
	    break;

	case 'B':
	    if( !HAS_PERM(SYSOP) && !is_BM_cache(currbid) )
		vmsg("�u���O�D�~�i�H�έ�!");
	    else
		BlogMain(me.now);
	    me.page = 9999;

	    break;
#endif
	}

	if (me.level >= MANAGER) {
	    int             page0 = me.page;

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
	    default:
		me.page = page0;
		break;
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
		}
	}
	if (me.level == SYSOP) {
	    switch (ch) {
	    case 'l':
		a_newitem(&me, ADDLINK);
		me.page = 9999;
		break;
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

static char    *mytitle = BBSNAME "�G�i��";

int
Announce()
{
    setutmpmode(ANNOUNCE);
    a_menu(mytitle, "man",
	   ((HAS_PERM(PERM_SYSOP) ) ? SYSOP :
	    NOBODY));
    return 0;
}

#ifdef BLOG
#include <mysql/mysql.h>
void BlogMain(int num)
{
    int     oldmode = currutmp->mode;
    char    genbuf[128], exit = 0;

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
		if( !islower(hash[i]) && !isnumber(hash[i]) )
		    break;
	    if( i != 32 ){
		vmsg("��J���~");
		break;
	    }
	    if( hash[0] != 0 && 
		getans("�нT�w�R��(Y/N)?[N] ") == 'y' ){
		MYSQL   mysql;
		char    cmd[256];
		
		sprintf(cmd, "delete from comment where "
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
	    sprintf(fpath, "man/boards/%c/%s", currboard[0], currboard);
	    stampdir(fpath, &item);
	    strlcpy(item.title, "�� Blog", sizeof(item.title));
	    strlcpy(item.owner, cuser.userid, sizeof(item.owner));

	    sprintf(adir, "man/boards/%c/%s/.DIR", currboard[0], currboard);
	    append_record(adir, &item, FHSZ);

	    snprintf(buf, sizeof(buf),
		     "cp -R etc/Blog.Default/.DIR etc/Blog.Default/* %s/",
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
