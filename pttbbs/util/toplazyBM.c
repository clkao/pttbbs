/* $Id$ */
#define _UTIL_C_
#include "bbs.h"
#define OUTFILE  BBSHOME "/etc/toplazyBM"
#define FIREFILE BBSHOME "/etc/firelazyBM"
extern boardheader_t *bcache;
extern int numboards;

#ifndef LAZY_BM_LIMIT_DAYS  
#define LAZY_BM_LIMIT_DAYS  (90)
#endif

boardheader_t allbrd[MAX_BOARD];
typedef struct lostbm {
    char  bmname[IDLEN + 1];
    char *title;
    char *ctitle;
    int lostdays;
} lostbm;
lostbm lostbms[MAX_BOARD];

typedef struct BMarray{
    char bmname[IDLEN + 1];
    int  flag;
}  BMArray;
BMArray bms[5];


int bmlostdays_cmp(const void *va, const void *vb)
{
    lostbm *a=(lostbm *)va, *b=(lostbm *)vb;
    if (a->lostdays > b->lostdays) return -1;
    else if (a->lostdays == b->lostdays) return 0;
    else return 1;
}

int main(int argc, char *argv[])
{
    int bmid, i, j=0;
    FILE *inf, *firef;
    time4_t now=time(NULL); 
    attach_SHM();
    resolve_boards();

    if(passwd_init())
	exit(1);      

    memcpy(allbrd,bcache,numboards*sizeof(boardheader_t));

    /* write out the target file */
    inf   = fopen(OUTFILE, "w+");
    if (inf == NULL) {
	printf("open file error : %s\n", OUTFILE);
	exit(1);
    }

    firef = fopen(FIREFILE, "w+");
    if (firef == NULL) {
	printf("open file error : %s\n", FIREFILE);
	exit(1);
    }

    fprintf(inf, "警告: 板主若超過(不包含) %d天未上站,將予於免職\n",
            LAZY_BM_LIMIT_DAYS);
    fprintf(inf,
	    "看板名稱                                           "
	    "    板主        幾天沒來啦\n"
	    "---------------------------------------------------"
	    "--------------------------\n");

    fprintf(firef, "免職板主\n");
    fprintf(firef,
	    "看板名稱                                           "
	    "    板主        幾天沒來啦\n"
	    "---------------------------------------------------"
	    "--------------------------\n"); 


    j = 0;
    for (i = 0; i < numboards; i++) {
	char *p, bmbuf[IDLEN * 3 + 3];
	int   index = 0, flag = 0, k, n;
	userec_t xuser;
	p = allbrd[i].BM;

	if (*p == '[') p++;
	if (allbrd[i].brdname[0] == '\0' ||
		!isalpha(allbrd[i].brdname[0])
	   ) continue;

	p = strtok(p,"/ ]");
	for(index=0; p && index<5; index++) {
	    int diff;
	    // XXX what if bmid is invalid?
	    if(!p[0] || (bmid = passwd_load_user(p, &xuser)) < 1) {
		index--;
		p=strtok(NULL,"/ ]");
		continue;
	    }
	    strlcpy(bms[index].bmname, p, sizeof(bms[index].bmname));
	    bms[index].flag = 0;

	    diff = now - xuser.lastlogin;
	    if (diff < 0)
		diff = 0;

	    if (diff >= 45 * 86400
		    && !(xuser.userlevel & PERM_SYSOPHIDE)
		    && !(xuser.userlevel & PERM_SYSOP)) {
		strlcpy(lostbms[j].bmname, p, sizeof(bms[index].bmname));
		lostbms[j].title = allbrd[i].brdname;
		lostbms[j].ctitle = allbrd[i].title;
		lostbms[j].lostdays = diff / 86400;

		//超過 LAZY_BM_LIMIT_DAYS 天 免職
		if (lostbms[j].lostdays > LAZY_BM_LIMIT_DAYS) {
		    xuser.userlevel &= ~PERM_BM;
		    bms[index].flag = 1;
		    flag = 1;
                    // NOTE: 好像不改也無所謂，目前拔 BM 是自動的。
		    passwd_update(bmid, &xuser);
		}
		j++;
	    }
	    p = strtok(NULL,"/ ]");
	}

	if (flag == 1) {
            boardheader_t *bp = getbcache(i+1);

            // 確認我們沒搞錯 cache. 如果 cache 炸了就別用了
            if (strcmp(bp->brdname, allbrd[i].brdname) != 0) {
                printf("ERROR: unmatched cache!!! (%s - %s)\n",
                        bp->brdname, allbrd[i].brdname);
                bp = NULL;
                exit(1);
                // sync to latest
                memcpy(&allbrd[i], bp, sizeof(boardheader_t));
            }

	    bmbuf[0] = '\0';
	    for (k = 0, n = 0; k < index; k++) {
		if (!bms[k].flag) {
		    if (n++ != 0) strcat(bmbuf, "/");
		    strcat(bmbuf, bms[k].bmname);
		}
	    }
	    strcpy(allbrd[i].BM, bmbuf);
            printf("board %s: %s -> %s\n",
                    allbrd[i].brdname, bp->BM, allbrd[i].BM);
            strcpy(bp->BM, allbrd[i].BM);
	    if (substitute_record(BBSHOME"/"FN_BOARD, &allbrd[i], 
			sizeof(boardheader_t), i+1) == -1) {
		printf("Update Board Failed: %s\n", allbrd[i].brdname);
	    }
	    reset_board(i+1);
	}
    }
    qsort(lostbms, j, sizeof(lostbm), bmlostdays_cmp);

    //write to the etc/toplazyBM
    for (i = 0; i < j; i++) {
	if (lostbms[i].lostdays > LAZY_BM_LIMIT_DAYS) {
	    fprintf(firef, "%-*.*s%-*.*s%-*.*s%3d天沒上站\n",
		    IDLEN, IDLEN, lostbms[i].title, BTLEN-10,
		    BTLEN-10, lostbms[i].ctitle, IDLEN,IDLEN,
		    lostbms[i].bmname,lostbms[i].lostdays);
	} else {
	    fprintf(inf, "%-*.*s%-*.*s%-*.*s%3d天沒上站\n",
		    IDLEN, IDLEN, lostbms[i].title, BTLEN-10,
		    BTLEN-10, lostbms[i].ctitle, IDLEN,IDLEN,
		    lostbms[i].bmname,lostbms[i].lostdays);
	}
    }
    fclose(inf);
    fclose(firef);

    //printf("Total %d boards.\n", count);

    //mail to the users
    for (i=0; i<j; i++) {
	fileheader_t mymail;
	char	genbuf[200];
	int	lostdays;

	lostdays = lostbms[i].lostdays;

	if (lostdays != LAZY_BM_LIMIT_DAYS/2 &&
            lostdays != LAZY_BM_LIMIT_DAYS*2/3 &&
            lostdays != LAZY_BM_LIMIT_DAYS*5/6 &&
            lostdays <= LAZY_BM_LIMIT_DAYS)
	    continue;

	sprintf(genbuf, BBSHOME "/home/%c/%s", 
		lostbms[i].bmname[0], lostbms[i].bmname);
	stampfile(genbuf, &mymail);

	strcpy(mymail.owner, "[" BBSMNAME "警察局]");
	if (lostdays <= LAZY_BM_LIMIT_DAYS)
	    sprintf(mymail.title,
		    ANSI_COLOR(32) "版主通知" ANSI_RESET " %s版版主%s",
		    lostbms[i].title, lostbms[i].bmname);
	else
	    sprintf(mymail.title,
		    ANSI_COLOR(32) "版主自動免職通知" ANSI_RESET " %s 版主 %s",
		    lostbms[i].title, lostbms[i].bmname);

	unlink(genbuf);
	if (lostdays <= LAZY_BM_LIMIT_DAYS)
	    Link(OUTFILE, genbuf);
	else
	    Link(FIREFILE, genbuf);

	sprintf(genbuf, BBSHOME "/home/%c/%s/.DIR", 
		lostbms[i].bmname[0], lostbms[i].bmname);
	append_record(genbuf, &mymail, sizeof(mymail)); 	
    }
    return 0;
}
