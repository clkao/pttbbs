#include "bbs.h"

// warning: if you changed userec_t, you must use the old version.

typedef struct {
    unsigned int    version;	/* version number of this sturcture, we
    				 * use revision number of project to denote.*/

    char    userid[IDLEN + 1];	/* ID */
    char    realname[20];	/* �u��m�W */
    char    nickname[24];	/* �ʺ� */
    char    passwd[PASSLEN];	/* �K�X */
    char    padx;
    unsigned int    uflag;	/* �ߺD1 , see uflags.h */
    unsigned int    uflag2;	/* �ߺD2 , see uflags.h */
    unsigned int    userlevel;	/* �v�� */
    unsigned int    numlogins;	/* �W������ */
    unsigned int    numposts;	/* �峹�g�� */
    time4_t firstlogin;		/* ���U�ɶ� */
    time4_t lastlogin;		/* �̪�W���ɶ� */
    char    lasthost[16];	/* �W���W���ӷ� */
    int     money;		/* Ptt�� */
    char    remoteuser[3];	/* �O�d �ثe�S�Ψ쪺 */
    char    proverb;		/* �y�k�� (unused) */
    char    email[50];		/* Email */
    char    address[50];	/* ��} */
    char    justify[REGLEN + 1];    /* �f�ָ�� */
    unsigned char   month;	/* �ͤ� �� */
    unsigned char   day;	/* �ͤ� �� */
    unsigned char   year;	/* �ͤ� �~ */
    unsigned char   sex;	/* �ʧO */
    unsigned char   state;	/* TODO unknown (unused ?) */
    unsigned char   pager;	/* �I�s�����A */
    unsigned char   invisible;	/* ���Ϊ��A */
    char    padxx[2];
    unsigned int    exmailbox;	/* �ʶR�H�c�� TODO short �N���F */
    chicken_t       mychicken;	/* �d�� */
    time4_t lastsong;		/* �W���I�q�ɶ� */
    unsigned int    loginview;	/* �i���e�� */
    unsigned char   channel;	/* TODO unused */
    char    padxxx;
    unsigned short  vl_count;	/* �H�k�O�� ViolateLaw counter */
    unsigned short  five_win;	/* ���l�Ѿ��Z �� */
    unsigned short  five_lose;	/* ���l�Ѿ��Z �� */
    unsigned short  five_tie;	/* ���l�Ѿ��Z �M */
    unsigned short  chc_win;	/* �H�Ѿ��Z �� */
    unsigned short  chc_lose;	/* �H�Ѿ��Z �� */
    unsigned short  chc_tie;	/* �H�Ѿ��Z �M */
    int     mobile;		/* ������X */
    char    mind[4];		/* �߱� not a null-terminate string */
    unsigned short  go_win;	/* ��Ѿ��Z �� */
    unsigned short  go_lose;	/* ��Ѿ��Z �� */
    unsigned short  go_tie;	/* ��Ѿ��Z �M */
    char    pad0[5];		/* �q�e�� ident �����Ҧr���A�{�b�i�H���Ӱ��O���ƤF�A
				   ���L�̦n�O�o�n���M�� 0 */
    unsigned char   signature;	/* �D��ñ�W�� */

    unsigned char   goodpost;	/* �������n�峹�� */
    unsigned char   badpost;	/* �������a�峹�� */
    unsigned char   goodsale;	/* �v�� �n������  */
    unsigned char   badsale;	/* �v�� �a������  */
    char    myangel[IDLEN+1];	/* �ڪ��p�Ѩ� */
    char    pad2;
    unsigned short  chess_elo_rating;	/* �H�ѵ��Ť� */
    unsigned int    withme;	/* �ڷQ��H�U�ѡA���.... */
    time4_t timeremovebadpost;  /* �W���R���H��ɶ� */
    time4_t timeviolatelaw; /* �Q�}�@��ɶ� */
    char    pad[28];
} old_userec_t;

int main()
{
    FILE *fp = fopen(FN_PASSWD, "rb"), *fp2 = NULL;
    char fn[PATHLEN];
    old_userec_t u;
    time4_t now=0;
    int i, cusr;

    if (!fp) {
	printf("cannot load password file. abort.\n");
	return -1;
    }
    now = time(NULL);

    i = 0; cusr = 0;
    while (fread(&u, sizeof(u), 1, fp) > 0)
    {
	/*
	cusr ++;
	if (cusr % (MAX_USERS / 100) == 0)
	{
	    fprintf(stderr, "%3d%%\r", cusr/(MAX_USERS/100));
	}
	*/
	if (!u.userid[0])
	    continue;

	// if dead and no records, // not possible to revive, 
	// then abort.
	if (!u.mychicken.name[0] &&
	     u.mychicken.cbirth == 0)
	     // u.lastvisit + 86400*7 < now
	    continue;

	// now, write this data to user home.
	// sethomefile(fn, u.userid, FN_CHICKEN);
	sprintf(fn, BBSHOME "/home/%c/%s/" FN_CHICKEN,
		u.userid[0], u.userid);

	// ignore created entries (if you are running live upgrade)
	if (access(fn, R_OK) == 0)//dashf(fn))
	{
	    // printf("\nfound created entry, ignore: %s\n", u.userid);
	    continue;
	}

	/*
	i++;
	continue;
	*/

	fp2 = fopen(fn, "wb");
	if (!fp2)
	{
	    printf("ERROR: cannot create chicken data: %s.\n", u.userid);
	    continue;
	}
	if (fwrite(&u.mychicken, sizeof(u.mychicken), 1, fp2) < 1)
	{
	    printf("ERROR: cannot write chicken data: %s.\n", u.userid);
	    unlink(fn);
	}
	else
	{
	    // printf("Transferred chicken data OK: %s.\n", u.userid);
	    i++;
	}
	fclose(fp2); fp2 = NULL;
    }
    fclose(fp);
    printf("\ntotal %d users updated.\n", i);
    return 0;
}
