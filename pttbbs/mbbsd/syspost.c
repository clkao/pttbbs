/* $Id: syspost.c,v 1.13 2002/07/21 08:18:41 in2 Exp $ */
#include "bbs.h"

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

	sprintf(fhdr.title, "[���w���i] ����%s�ק�%s�v�����i",
		cuser.userid, userid);
	strlcpy(fhdr.owner, "[�t�Φw����]", sizeof(fhdr.owner));
	append_record("boards/S/Security/.DIR", &fhdr, sizeof(fhdr));
    }
}

void 
post_violatelaw(char *crime, char *police, char *reason, char *result)
{
    char            genbuf[200];
    fileheader_t    fhdr;
    FILE           *fp;
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
    sprintf(fhdr.title, "[���i] %-20s �H�k�P�M���i", crime);
    strlcpy(fhdr.owner, "[Ptt�k�|]", sizeof(fhdr.owner));
    append_record("boards/S/Security/.DIR", &fhdr, sizeof(fhdr));

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
    sprintf(fhdr.title, "[���i] %-20s �H�k�P�M���i", crime);
    strlcpy(fhdr.owner, "[Ptt�k�|]", sizeof(fhdr.owner));

    append_record("boards/V/ViolateLaw/.DIR", &fhdr, sizeof(fhdr));

}

void 
post_newboard(char *bgroup, char *bname, char *bms)
{
    char            genbuf[256], title[128];
    sprintf(title, "[�s�O����] %s", bname);
    sprintf(genbuf, "%s �}�F�@�ӷs�O %s : %s\n\n�s���O�D�� %s\n\n����*^_^*\n",
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
    sprintf(fhdr.title, "[���w���i] ����%s�ϥά��]�����i",
	    cuser.userid);
    strlcpy(fhdr.owner, "[�t�Φw����]", sizeof(fhdr.owner));
    append_record("boards/S/Security/.DIR", &fhdr, sizeof(fhdr));
}
