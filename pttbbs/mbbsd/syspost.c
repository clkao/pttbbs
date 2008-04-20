/* $Id$ */
#include "bbs.h"

int 
post_msg_fpath(const char* bname, const char* title, const char *msg, const char* author, char *fname)
{
    FILE           *fp;
    int             bid;
    fileheader_t    fhdr;
    char	    dirfn[PATHLEN];

    // fname should be lengthed in PATHLEN
    assert(fname);

    /* �b bname �O�o��s�峹 */
    setbpath(fname, bname);
    stampfile(fname, &fhdr);
    fp = fopen(fname, "w");

    if (!fp)
	return -1;

    fprintf(fp, "�@��: %s �ݪO: %s\n���D: %s \n", author, bname, title);
    fprintf(fp, "�ɶ�: %s\n", ctime4(&now));

    /* �峹�����e */
    fputs(msg, fp);
    fclose(fp);

    /* �N�ɮץ[�J�C�� */
    strlcpy(fhdr.title, title, sizeof(fhdr.title));
    strlcpy(fhdr.owner, author, sizeof(fhdr.owner));
    setbdir(dirfn, bname);
    if (append_record(dirfn, &fhdr, sizeof(fhdr)) != -1)
	if ((bid = getbnum(bname)) > 0)
	    setbtotal(bid);
    return 0;
}

int 
post_msg(const char* bname, const char* title, const char *msg, const char* author)
{
    char fname[PATHLEN];
    return post_msg_fpath(bname, title, msg, author, fname);
}

int
post_file(const char *bname, const char *title, const char *filename, const char *author)
{
    int             size = dashs(filename);
    char           *msg;
    FILE           *fp;

    if (size <= 0)
	return -1;
    if (!(fp = fopen(filename, "r")))
	return -1;
    msg = (char *)malloc(size + 1);
    size = fread(msg, 1, size, fp);
    msg[size] = 0;
    size = post_msg(bname, title, msg, author);
    fclose(fp);
    free(msg);
    return size;
}

void
post_change_perm(int oldperm, int newperm, const char *sysopid, const char *userid)
{
    char genbuf[(NUMPERMS+1) * STRLEN*2] = "", reason[30] = "";
    char title[TTLEN+1];
    char *s = genbuf;
    int  i, flag = 0;

    // generate log (warning: each line should <= STRLEN*2)
    for (i = 0; i < NUMPERMS; i++) {
	if (((oldperm >> i) & 1) != ((newperm >> i) & 1)) {
	    sprintf(s, "   ����" ANSI_COLOR(1;32) "%s%s%s%s" ANSI_RESET "���v��\n",
		    sysopid,
	       (((oldperm >> i) & 1) ? ANSI_COLOR(1;33) "����" : ANSI_COLOR(1;33) "�}��"),
		    userid, str_permid[i]);
	    s += strlen(s);
	    flag++;
	}
    }

    if (!flag) // nothing changed
	return;

    clrtobot();
    clear();
    while (!getdata(5, 0, "�п�J�z�ѥH�ܭt�d�G",
		reason, sizeof(reason), DOECHO));
    sprintf(s, "\n   " ANSI_COLOR(1;37) "����%s�ק��v���z�ѬO�G%s\n" ANSI_RESET,
	    cuser.userid, reason);

    snprintf(title, sizeof(title), "[���w���i] ����%s�ק�%s�v�����i",
	    cuser.userid, userid);

    post_msg(BN_SECURITY, title, genbuf, "[�t�Φw����]");
}

void
post_violatelaw(const char *crime, const char *police, const char *reason, const char *result)
{
    char title[TTLEN+1];
    char msg[ANSILINELEN];

    snprintf(title, sizeof(title), "[���i] %s:%-*s �P�M", crime,
	    (int)(37 - strlen(reason) - strlen(crime)), reason);

    snprintf(msg, sizeof(msg), 
	    ANSI_COLOR(1;32) "%s" ANSI_RESET "�P�M�G\n"
	    "     " ANSI_COLOR(1;32) "%s" ANSI_RESET "�]" ANSI_COLOR(1;35) "%s" ANSI_RESET "�欰�A\n"
	    "�H�ϥ������W�A�B�H" ANSI_COLOR(1;35) "%s" ANSI_RESET "�A�S�����i\n",
	    police, crime, reason, result);

    if (!strstr(police, "ĵ��")) {
	post_msg("PoliceLog", title, msg, "[" BBSMNAME "�k�|]");

	snprintf(msg, sizeof(msg), 
		ANSI_COLOR(1;32) "%s" ANSI_RESET "�P�M�G\n"
		"     " ANSI_COLOR(1;32) "%s" ANSI_RESET "�]" ANSI_COLOR(1;35) "%s" ANSI_RESET "�欰�A\n"
		"�H�ϥ������W�A�B�H" ANSI_COLOR(1;35) "%s" ANSI_RESET "�A�S�����i\n",
		"����ĵ��", crime, reason, result);
    }

    post_msg("ViolateLaw", title, msg, "[" BBSMNAME "�k�|]");
}

void
post_newboard(const char *bgroup, const char *bname, const char *bms)
{
    char            genbuf[ANSILINELEN], title[TTLEN+1];

    snprintf(title, sizeof(title), "[�s�O����] %s", bname);
    snprintf(genbuf, sizeof(genbuf),
	     "%s �}�F�@�ӷs�O %s : %s\n\n�s���O�D�� %s\n\n����*^_^*\n",
	     cuser.userid, bname, bgroup, bms);

    post_msg("Record", title, genbuf, "[�t��]");
}

void
post_policelog(const char *bname, const char *atitle, const char *action, const char *reason, const int toggle)
{
    char            genbuf[ANSILINELEN], title[TTLEN+1];

    snprintf(title, sizeof(title), "[%s][%s] %s by %s", action, toggle ? "�}��" : "����", bname, cuser.userid);
    snprintf(genbuf, sizeof(genbuf),
	     "%s (%s) %s %s �ݪO %s �\\��\n��] : %s\n%s%s\n",
	     cuser.userid, fromhost, toggle ? "�}��" : "����", bname, action,
	     reason, atitle ? "�峹���D : " : "", atitle ? atitle : "");

    post_msg("PoliceLog", title, genbuf, "[�t��]");
}
