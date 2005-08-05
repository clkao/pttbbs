/* $Id$ */
#include "bbs.h"

// test
#define ACCOUNT_MAX_LINE        16
#define ADJUST_M        6	/* adjust back 5 minutes */

void
reset_garbage(void)
{
    if (SHM == NULL) {
	attach_SHM();
	if (SHM->Ptouchtime == 0)
	    SHM->Ptouchtime = 1;
    }
    /*
     * �����reload? for(n=0;n<=SHM->max_film;n++) printf("\n**%d**\n %s
     * \n",n,SHM->notes[n]);
     */
    SHM->Puptime = 0;
    reload_pttcache();

    printf("\n�ʺA�ݪO��[%d]\n", SHM->max_film);
    /*
     * for(n=0; n<MAX_MOVIE_SECTION; n++) printf("sec%d=> �_�I:%d
     * �U���n����:%d\n ",n,SHM->n_notes[n], SHM->next_refresh[n]);
     * printf("\n");
     */
}

void
keeplog(char *fpath, char *board, char *title, char *sym)
{
    fileheader_t    fhdr;
    int             bid;
    char            genbuf[256], buf[256];

    if (!board)
	board = "Record";

    sprintf(genbuf, BBSHOME "/boards/%c/%s", board[0], board);
    stampfile(genbuf, &fhdr);
    sprintf(buf, "mv %s %s", fpath, genbuf);
    system(buf);                                                              

    if( sym ){
        sprintf(buf, "log/%s", sym);
        unlink(buf);
        symlink(genbuf, buf);
    }
    /*
     * printf("keep record:[%s][%s][%s][%s]\n",fpath, board, title,genbuf);
     */
    strcpy(fhdr.title, title);
    strcpy(fhdr.owner, "[���v�Ѯv]");
    sprintf(genbuf, "boards/%c/%s/.DIR", board[0], board);
    append_record(genbuf, &fhdr, sizeof(fhdr));
    /* XXX: bid of cache.c's getbnum starts from 1 */
    if ((bid = getbnum(board)) > 0)
	touchbtotal(bid);
}


static void
my_outs(fp, buf, mode)
    FILE           *fp;
    char            buf[], mode;
{
    static char     state = '0';

    if (state != mode)
	fprintf(fp, "[3%cm", state = mode);
    if (buf[0]) {
	fprintf(fp, buf);
	buf[0] = 0;
    }
}

/* XXX: �ǩǪ�, �ݤ����b gzip() ����, �ӥB�䤤�� stamp �n�����öǶi�� */
void 
gzip(source, target, stamp)
    char           *source, *target, *stamp;
{
    char            buf[128];
    sprintf(buf, "gzip -f9n adm/%s%s", target, stamp);
    rename(source, &buf[14]);
    system(buf);
}

int 
main(int argc, char **argv)
{
    int             hour, max, item, total, i, j, mo, da, max_user = 0,
                    max_login = 0, max_reg = 0, mahour = 0, k, wday;
    char           *act_file = ".act";
    char           *log_file = "usies";
    char           *wday_str = "UMTWRFS";
    char            buf[256], buf1[256], *p;
    FILE           *fp, *fp1;
    int             act[27];	/* ����/�֭p�ɶ�/pointer */
    time4_t         now;
    struct tm      *ptime;
    int             per_hour_unit = 10;

    attach_SHM();
    nice(10);
    chdir(BBSHOME);
    now = time(NULL) - ADJUST_M * 60;	/* back to ancent */
    ptime = localtime4(&now);

    memset(act, 0, sizeof(act));
    printf("����/�֭p�ɶ�\n");
    if ((ptime->tm_hour != 0) && (fp = fopen(act_file, "r"))) {
	fread(act, sizeof(act), 1, fp);
	fclose(fp);
    }
    if ((fp = fopen(log_file, "r")) == NULL) {
	printf("cann't open usies\n");
	return 1;
    }
    if (act[26])
	fseek(fp, act[26], 0);
    while (fgets(buf, 256, fp)) {
	buf[11 + 2] = 0;
	hour = atoi(buf + 11);
	if (hour < 0 || hour > 23) {
	    continue;
	}
	//"09/06/1999 17:44:58 Mon "
	    // 012345678901234567890123
	    if (strstr(buf + 20, "ENTER")) {
	    act[hour]++;
	    continue;
	}
	if ((p = (char *)strstr(buf + 40, "Stay:"))) {
	    if ((hour = atoi(p + 5))) {
		act[24] += hour;
		act[25]++;
	    }
	    continue;
	}
    }
    act[26] = ftell(fp);
    fclose(fp);
    for (i = max = total = 0; i < 24; i++) {
	total += act[i];
	if (act[i] > max) {
	    max_user = max = act[i];
	    mahour = i;
	}
    }
    item = max / ACCOUNT_MAX_LINE + 1;

    if (!ptime->tm_hour) {
	keeplog("etc/today", "Record", "�W���H���έp", NULL);
	keeplog(FN_MONEY, "Security", "����������ӰO��", NULL);
	keeplog("etc/illegal_money", "Security", "����H�k�ȿ��O��", NULL);
	keeplog("etc/chicken", "Record", "�������i", NULL);
    }
    printf("�W���H���έp\n");
    if ((fp = fopen("etc/today", "w")) == NULL) {
	printf("cann't open etc/today\n");
	return 1;
    }
    fprintf(fp, "\t\t\t[1;33;46m �C�p�ɤW���H���έp [%02d/%02d/%02d] [40m\n\n", ptime->tm_year % 100, ptime->tm_mon + 1, ptime->tm_mday);
    for (i = ACCOUNT_MAX_LINE + 1; i > 0; i--) {
	strcpy(buf, "   ");
	for (j = 0; j < 24; j++) {
	    max = item * i;
	    hour = act[j];
	    if (hour && (max > hour) && (max - item <= hour)) {
		my_outs(fp, buf, '3');
		fprintf(fp, "%-3d", hour / per_hour_unit);
	    } else if (max <= hour) {
		my_outs(fp, buf, '4');
		fprintf(fp, "�i ");
	    } else
		strcat(buf, "   ");
	}
	fprintf(fp, "\n");
    }
    fprintf(fp, "   [32m"
	    "0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23\n\n"
	    "\t      [34m���: [37m%d[34m �H", per_hour_unit);
    fprintf(fp, "  �`�@�W���H���G[37m%-7d[34m�����ϥΤH�ơG[37m%d\n", total, total / 24);
    fclose(fp);

    if ((fp = fopen(act_file, "w"))) {
	fwrite(act, sizeof(act), 1, fp);
	fclose(fp);
    }
    /* -------------------------------------------------------------- */

    sprintf(buf, "-%02d%02d%02d",
	    ptime->tm_year % 100, ptime->tm_mon + 1, ptime->tm_mday);

    now += ADJUST_M * 60;	/* back to future */


    printf("���v�ƥ�B�z\n");
    /* Ptt ���v�ƥ�B�z */
    if ((fp = fopen("etc/history.data", "r"))) {	/* �̦h�P�ɤW�u */
	if (fscanf(fp, "%d %d %d %d", &max_login, &max, &max_reg, &k)) {
	    int             a;
	    resolve_fcache();
	    printf("���ɬq�̦h�P�ɤW�u:%d �L�h:%d\n", a = SHM->max_user, k);
	    fclose(fp);
	    if (a > k) {
		ptime = localtime4(&SHM->max_time);
		if ((fp1 = fopen("etc/history", "a"))) {
		    fprintf(fp1,
			    "�� �i%02d/%02d/%02d %02d:%02d�j"
		       "[32m�P�ɦb�{���H��[m�����F�� [1;36m%d[m �H��\n",
		    ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_year % 100,
			    ptime->tm_hour, ptime->tm_min, a);
		    fclose(fp1);
		}
		if ((fp = fopen("etc/history.data", "w"))) {
		    fprintf(fp, "%d %d %d %d", max_login, max, max_reg, a);
		    fclose(fp);
		}
	    }
	} else
	    fclose(fp);
    }
    ptime = localtime4(&now);

    if (ptime->tm_hour) {
	/* rotate one line in today_is */
	puts("�h�Ӹ`��B�z");
	if ((fp1 = fopen("etc/today_is", "r"))) {
	    char            tod[100][20];

	    i = 0;
	    while (i < 100 && fgets(tod[i], sizeof(tod[0]), fp1))
		i++;
	    fclose(fp1);

	    fp1 = fopen("etc/today_is", "w");
	    for (j = 0; j < i; j++)
		fputs(tod[j + 1 < i ? j + 1 : 0], fp1);
	    fclose(fp1);
	}
    }
    if (!ptime->tm_hour) {
	keeplog(".note", "Record", "�߱��d����", NULL);
	system("/bin/cp etc/today etc/yesterday");
	/* system("rm -f note.dat"); */
	/* Ptt */
	sprintf(buf1, "[���w���i] �ϥΪ̤W�u�ʱ� [%02d/%02d:%02d]",
		ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour);
	keeplog("usies", "Security", buf1, "usies");
	printf("[���w���i] �ϥΪ̤W�u�ʱ�\n");
	gzip(log_file, "usies", buf);
	printf("���Y�ϥΪ̤W�u�ʱ�\n");
	/* Ptt ���v�ƥ�B�z */
	now = time(NULL) - ADJUST_M * 60;	/* back to ancent */
	ptime = localtime4(&now);

	attach_SHM();
	if ((fp = fopen("etc/history.data", "r"))) {	/* ���̦h���H��,�P�ɤW�u
							 * ,���U */
	    if (fscanf(fp, "%d %d %d %d", &max_login, &max, &max_reg, &k)) {
		fp1 = fopen("etc/history", "r+");
		fseek(fp1, 0, 2);
		if (max_user > max) {
		    fprintf(fp1, "�� �i%02d/%02d/%02d %02d�j   "
		    "[1;32m��@�p�ɤW�u�H��[m�����F�� [1;35m%d[m �H�� \n"
			    ,ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_year % 100, mahour, max_user);
		    max = max_user;
		}
		if (total > max_login) {
		    fprintf(fp1, "�� �i%02d/%02d/%02d�j      "
		     "[1;32m���W�u�H��[m�����F��[1;33m %d[m �H��   \n"
			    ,ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_year % 100, total);
		    max_login = total;
		}
		if (SHM->number > max_reg + max_reg / 10) {
		    fprintf(fp1, "�� �i%02d/%02d/%02d�j      "
			    "[1;32m�`���U�H��[m���ɨ�[1;31m %d[m �H   \n"
			    ,ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_year % 100, SHM->number);
		    max_reg = SHM->number;
		}
		fclose(fp1);
	    }
	    fclose(fp);
	    fp = fopen("etc/history.data", "w");
	    fprintf(fp, "%d %d %d %d", max_login, max, max_reg, k);
	    fclose(fp);
	}
	now += ADJUST_M * 60;	/* back to future */
	ptime = localtime4(&now);

	/* Ptt �`��B�z */
	printf("�`��B�z\n");
	if ((fp1 = fopen("etc/today_is", "w"))) {
	    i = 0;
	    if ((fp = fopen("etc/feast", "r"))) {
		while (fgets(buf1, sizeof(buf1), fp)) {
		    if (buf[0] != '#' &&
			sscanf(buf1, "%d %c%c", &mo, buf, buf + 1) == 3) {
			if (isdigit(buf[0])) {
			    if (isdigit(buf[1])) {
				da = 10 * (buf[0] - '0') + (buf[1] - '0');
				if (ptime->tm_mday == da && ptime->tm_mon + 1 == mo) {
				    i = 1;
				    fprintf(fp1, "%-14.14s", &buf1[6]);
				}
			    } else {
				if (buf[0] - '0' <= 4) {
				    wday = 0;
				    buf[1] = toupper(buf[1]);
				    while (wday < 7 && buf[1] != *(wday_str + wday))
					wday++;
				    if (ptime->tm_mon + 1 == mo && ptime->tm_wday == wday &&
					(ptime->tm_mday - 1) / 7 + 1 == (buf[0] - '0')) {
					i = 1;
					fprintf(fp1, "%-14.14s", &buf1[6]);
				    }
				}
			    }
			}
		    }
		}
		fclose(fp);
	    }
	    printf("�`��B�z1\n");
	    if (i == 0) {
		if ((fp = fopen("etc/today_boring", "r"))) {
		    while (fgets(buf1, sizeof(buf1), fp))
			if (strlen(buf1) > 3)
			    fprintf(fp1, "%s", buf1);
		    fclose(fp);
		} else
		    fprintf(fp1, "����`��x�D��");
	    }
	    fclose(fp1);
	}
	/* Ptt �w��e���B�z */
	printf("�w��e���B�z\n");

	if ((fp = fopen("etc/Welcome.date", "r"))) {
	    char            temp[50];
	    while (fscanf(fp, "%d %d %s\n", &mo, &da, buf1) != EOF) {
		if (ptime->tm_mday == da && ptime->tm_mon + 1 == mo) {
		    strcpy(temp, buf1);
		    sprintf(buf1, "cp -f etc/Welcomes/%s etc/Welcome", temp);
		    system(buf1);
		    break;
		}
	    }
	    fclose(fp);
	}
	printf("�w��e���B�z\n");
	if (ptime->tm_wday == 0) {
	    keeplog("etc/week", "Record", "���g�������D", NULL);

	    gzip("bbslog", "bntplink", buf);
	    gzip("innd/bbslog", "innbbsd", buf);
	    gzip("etc/mailog", "mailog", buf);
	}
	if (ptime->tm_mday == 1)
	    keeplog("etc/month", "Record", "����������D", NULL);

	if (ptime->tm_yday == 1)
	    keeplog("etc/year", "Record", "�~�׼������D", NULL);
    } else if (ptime->tm_hour == 3 && ptime->tm_wday == 6) {
	char           *fn1 = "tmp";
	char           *fn2 = "suicide";
	rename(fn1, fn2);
	mkdir(fn1, 0755);
	sprintf(buf, "tar cfz adm/%s-%02d%02d%02d.tgz %s",
	 fn2, ptime->tm_year % 100, ptime->tm_mon + 1, ptime->tm_mday, fn2);
	system(buf);
	sprintf(buf, "/bin/rm -fr %s", fn2);
	system(buf);
    }
    /* Ptt reset Ptt's share memory */
    printf("���]Pttcache �Pfcache\n");

    SHM->Puptime = 0;
    resolve_fcache();
    reset_garbage();

    printf("�p��i���e����: ");
    for (i = 0; i < 5; ++i) {
	sprintf(buf, "etc/Welcome_login.%d", i);
	if (access(buf, 0) < 0)
	    break;
    }
    printf("%d\n", SHM->GV2.e.nWelcomes = i);
    return 0;
}
