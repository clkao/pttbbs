/*     �جP�{��               96 10/11            */

#define _UTIL_C_
#include "bbs.h"

#define OUTFILE    BBSHOME "/etc/birth.today"

struct userec_t cuser;

int bad_user_id(char *userid) {
    register char ch;
    int j;
    if (strlen(cuser.userid) < 2 || !isalpha(cuser.userid[0]))
	return 1;
    if (cuser.numlogins == 0 || cuser.numlogins > 15000)
	return 1;
    if (cuser.numposts > 15000)
	return 1;
    for (j = 1; (ch = cuser.userid[j]); j++)
    {
	if (!isalnum(ch))
	    return 1;
    }
    return 0;
}

int Link(char *src, char *dst) {
    char cmd[200];

    if (link(src, dst) == 0)
	return 0;

    sprintf(cmd, "/bin/cp -R %s %s", src, dst);
    return system(cmd);
}

int main(argc, argv)
    int argc;
    char **argv;
{
    FILE *fp1;
    fileheader_t mymail;
    //int i, day = 0;
    time_t now;
    struct tm *ptime;
    int j;

    attach_SHM();
    now = time(NULL);		/* back to ancent */
    ptime = localtime(&now);

    if(passwd_init())
	exit(1);

    printf("*�s��\n");
    fp1 = fopen(OUTFILE, "w");

    fprintf(fp1, "\n                      "
	    "[1m��[35m��[34m��[33m��[32m��[31m��[45;33m �جP�j�[ "
	    "[40m��[32m��[33m��[34m��[35m��[1m��[m \n\n");
    fprintf(fp1, "[33m�i[1;45m����جP[40;33m�j[m \n");
    for(j = 1; j <= MAX_USERS; j++) {
	passwd_query(j, &cuser);
	if (bad_user_id(NULL))
	    continue;
	if (cuser.month == ptime->tm_mon + 1)
	{
	    if (cuser.day == ptime->tm_mday)
	    {
		char genbuf[200];
		sprintf(genbuf, BBSHOME "/home/%c/%s", cuser.userid[0], cuser.userid);
		stampfile(genbuf, &mymail);
		strcpy(mymail.owner, BBSNAME);
		strcpy(mymail.title, "!! �ͤ�ּ� !!");
		unlink(genbuf);
		Link(BBSHOME "/etc/Welcome_birth", genbuf);
		sprintf(genbuf, BBSHOME "/home/%c/%s/.DIR", cuser.userid[0], cuser.userid);
		append_record(genbuf, &mymail, sizeof(mymail));
		if ((cuser.numlogins + cuser.numposts) < 20)
		    continue;

                fprintf(fp1,
		 "   [33m[%2d/%-2d] %-14s[0m %-24s login:%-5d post:%-5d\n",
		    ptime->tm_mon + 1, ptime->tm_mday, cuser.userid,
			cuser.username, cuser.numlogins, cuser.numposts);
	    }
	}
    }
    fclose(fp1);
    return 0;
}
