/* $Id$ */
#include "bbs.h"

int
post_msg(char *bname, char *title, char *msg, char *author)
{
    FILE           *fp;
    int             bid;
    fileheader_t    fhdr;
    char            genbuf[256];

    /* �b bname �O�o��s�峹 */
    snprintf(genbuf, sizeof(genbuf), "boards/%c/%s", bname[0], bname);
    stampfile(genbuf, &fhdr);
    fp = fopen(genbuf, "w");

    if (!fp)
	return -1;

    fprintf(fp, "�@��: %s �ݪO: %s\n���D: %s \n", author, bname, title);
    fprintf(fp, "�ɶ�: %s\n", ctime(&now));

    /* �峹�����e */
    fputs(msg, fp);
    fclose(fp);

    /* �N�ɮץ[�J�C�� */
    strlcpy(fhdr.title, title, sizeof(fhdr.title));
    strlcpy(fhdr.owner, author, sizeof(fhdr.owner));
    setbdir(genbuf, bname);
    if (append_record(genbuf, &fhdr, sizeof(fhdr)) != -1)
	if ((bid = getbnum(bname)) > 0)
	    setbtotal(bid);
    return 0;
}

int
post_file(char *bname, char *title, char *filename, char *author)
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
post_change_perm(int oldperm, int newperm, char *sysopid, char *userid)
{
    FILE           *fp;
    fileheader_t    fhdr;
    char            genbuf[200], reason[30];
    int             i, flag = 0;

    strlcpy(genbuf, "boards/S/Security", sizeof(genbuf));
    stampfile(genbuf, &fhdr);
    if (!(fp = fopen(genbuf, "w")))
	return;

    fprintf(fp, "�@��: [�t�Φw����] �ݪO: Security\n"
	    "���D: [���w���i] �����ק��v�����i\n"
	    "�ɶ�: %s\n", ctime(&now));
    for (i = 5; i < NUMPERMS; i++) {
	if (((oldperm >> i) & 1) != ((newperm >> i) & 1)) {
	    fprintf(fp, "   ����\033[1;32m%s%s%s%s\033[m���v��\n",
		    sysopid,
	       (((oldperm >> i) & 1) ? "\033[1;33m����" : "\033[1;33m�}��"),
		    userid, str_permid[i]);
	    flag++;
	}
    }

    if (flag) {
	clrtobot();
	clear();
	while (!getdata_str(5, 0, "�п�J�z�ѥH�ܭt�d�G",
			    reason, sizeof(reason), DOECHO, "�ݪO�O�D:"));
	fprintf(fp, "\n   \033[1;37m����%s�ק��v���z�ѬO�G%s\033[m",
		cuser.userid, reason);
	fclose(fp);

	snprintf(fhdr.title, sizeof(fhdr.title),
		 "[���w���i] ����%s�ק�%s�v�����i",
		 cuser.userid, userid);
	strlcpy(fhdr.owner, "[�t�Φw����]", sizeof(fhdr.owner));
	append_record("boards/S/Security/.DIR", &fhdr, sizeof(fhdr));
    } else
	fclose(fp);
}

void
post_violatelaw(char *crime, char *police, char *reason, char *result)
{
    char            genbuf[200];
    fileheader_t    fhdr;
    FILE           *fp;
/*
    strlcpy(genbuf, "boards/S/Security", sizeof(genbuf));
    stampfile(genbuf, &fhdr);
    if (!(fp = fopen(genbuf, "w")))
	return;
    fprintf(fp, "�@��: [Ptt�k�|] �ݪO: Security\n"
	    "���D: [���i] %-20s �H�k�P�M���i\n"
	    "�ɶ�: %s\n"
	    "\033[1;32m%s\033[m�P�M�G\n     \033[1;32m%s\033[m"
	    "�]\033[1;35m%s\033[m�欰�A\n�H�ϥ������W�A�B�H\033[1;35m%s\033[m�A�S�����i",
	    crime, ctime(&now), police, crime, reason, result);
    fclose(fp);
    snprintf(fhdr.title, sizeof(fhdr.title),
	     "[���i] %-20s �H�k�P�M���i", crime);
    strlcpy(fhdr.owner, "[Ptt�k�|]", sizeof(fhdr.owner));
    append_record("boards/S/Security/.DIR", &fhdr, sizeof(fhdr));

*/
    strlcpy(genbuf, "boards/V/ViolateLaw", sizeof(genbuf));
    stampfile(genbuf, &fhdr);
    if (!(fp = fopen(genbuf, "w")))
	return;
    fprintf(fp, "�@��: [Ptt�k�|] �ݪO: ViolateLaw\n"
	    "���D: [���i] %-20s �H�k�P�M���i\n"
	    "�ɶ�: %s\n"
	    "\033[1;32m%s\033[m�P�M�G\n     \033[1;32m%s\033[m"
	    "�]\033[1;35m%s\033[m�欰�A\n�H�ϥ������W�A�B�H\033[1;35m%s\033[m�A�S�����i",
	    crime, ctime(&now), police, crime, reason, result);
    fclose(fp);
    snprintf(fhdr.title, sizeof(fhdr.title),
	     "[���i] %s: %s �P�M���i", reason, crime);
    strlcpy(fhdr.owner, "[Ptt�k�|]", sizeof(fhdr.owner));

    append_record("boards/V/ViolateLaw/.DIR", &fhdr, sizeof(fhdr));

}

void
post_newboard(char *bgroup, char *bname, char *bms)
{
    char            genbuf[256], title[128];
    snprintf(title, sizeof(title), "[�s�O����] %s", bname);
    snprintf(genbuf, sizeof(genbuf),
	     "%s �}�F�@�ӷs�O %s : %s\n\n�s���O�D�� %s\n\n����*^_^*\n",
	     cuser.userid, bname, bgroup, bms);
    post_msg("Record", title, genbuf, "[�t��]");
}

void
give_money_post(char *userid, int money)
{
    FILE           *fp;
    fileheader_t    fhdr;
    time_t          now = time(0);
    char            genbuf[200];

    strlcpy(genbuf, "boards/S/Security", sizeof(genbuf));
    stampfile(genbuf, &fhdr);
    if (!(fp = fopen(genbuf, "w")))
	return;
    fprintf(fp, "�@��: [�t�Φw����] �ݪO: Security\n"
	    "���D: [���w���i] ����%s�ϥά��]�����i\n"
	    "�ɶ�: %s\n", cuser.userid, ctime(&now));
    clrtobot();
    clear();
    fprintf(fp, "\n   ����\033[1;32m%s\033[m��\033[1;33m%s %d ��\033[m",
	    cuser.userid, userid, money);

    fclose(fp);
    snprintf(fhdr.title, sizeof(fhdr.title), "[���w���i] ����%s�ϥά��]�����i",
	     cuser.userid);
    strlcpy(fhdr.owner, "[�t�Φw����]", sizeof(fhdr.owner));
    append_record("boards/S/Security/.DIR", &fhdr, sizeof(fhdr));
}
