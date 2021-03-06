/*     壽星程式               96 10/11            */

#define _UTIL_C_
#include "bbs.h"

#define OUTFILE    BBSHOME "/etc/birth.today"

struct userec_t user;

int bad_user_id(const char *userid) {
    register char ch;
    int j;
    if (strlen(user.userid) < 2 || !isalpha(user.userid[0]))
	return 1;
    if (user.numlogindays == 0 || user.numlogindays > 15000)
	return 1;
    if (user.numposts > 15000)
	return 1;
    for (j = 1; (ch = user.userid[j]); j++)
    {
	if (!isalnum(ch))
	    return 1;
    }
    return 0;
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

    printf("*製表\n");
    fp1 = fopen(OUTFILE, "w");

    fprintf(fp1, "\n                      "
	    "[1m★[35m★[34m★[33m★[32m★[31m★[45;33m 壽星大觀 "
	    "[40m★[32m★[33m★[34m★[35m★[1m★[m \n\n");
    fprintf(fp1, "[33m【[1;45m本日壽星[40;33m】[m \n");
    int horoscope = getHoroscope(ptime->tm_mon + 1, ptime->tm_mday);
    char path[PATHLEN];
    snprintf(path, sizeof(path), BBSHOME "/etc/Welcome_birth.%d", horoscope);
    for(j = 1; j <= MAX_USERS; j++) {
	passwd_query(j, &user);
	if (bad_user_id(NULL))
	    continue;
	if (user.month == ptime->tm_mon + 1 && user.day == ptime->tm_mday) {
	    char genbuf[200];
	    sprintf(genbuf, BBSHOME "/home/%c/%s", user.userid[0], user.userid);
	    stampfile(genbuf, &mymail);
	    strcpy(mymail.owner, BBSNAME);
	    strcpy(mymail.title, "!! 生日快樂 !!");
	    unlink(genbuf);
	    Link(path, genbuf);
	    sprintf(genbuf, BBSHOME "/home/%c/%s/.DIR", user.userid[0], user.userid);
	    append_record(genbuf, &mymail, sizeof(mymail));
	    if ((user.numlogindays + user.numposts) < 20)
		continue;

	    fprintf(fp1,
		    "   [33m[%2d/%-2d] %-14s[0m %-24s login:%-5d post:%-5d\n",
		    ptime->tm_mon + 1, ptime->tm_mday, user.userid,
		    user.nickname, user.numlogindays, user.numposts);
	}
    }
    fclose(fp1);
    return 0;
}
