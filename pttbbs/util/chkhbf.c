/* $Id$ */
#define _UTIL_C_
#include "bbs.h"

struct {
    char    userid[IDLEN + 1];
    time_t  lastlogin, expire;
} explist[MAX_FRIEND];

void usage(void)
{
    fprintf(stderr, "�q�����O�O�D�O�_���O�͹L��/�w�g�L��\n"
	    "usage: chkhbf [-a] [board name [board name]]\n");
}

int mailalertuid(int tuid)
{
    userinfo_t *uentp=NULL;
    if(tuid>0 && (uentp = (userinfo_t *)search_ulist(tuid)) )
	    uentp->alerts |=ALERT_NEW_MAIL;
    return 0;
}      

char *CTIMEx(char *buf, time_t t)
{
    strcpy(buf, ctime(&t));
    buf[strlen(buf) - 1] = 0;
    return buf;
}

void informBM(char *userid, boardheader_t *bptr, int nEXP)
{
    int     uid, i;
    char    filename[256], buf[64];
    fileheader_t mymail;
    FILE    *fp;
    if( !(uid = searchuser(userid, userid)) )
	return;
    sprintf(filename, BBSHOME "/home/%c/%s", userid[0], userid);
    stampfile(filename, &mymail);
    if( (fp = fopen(filename, "w")) == NULL )
	return;

    printf("brdname: %s, BM: %s\n", bptr->brdname, userid);
    fprintf(fp,
	    "�@��: �t�γq��.\n"
	    "���D: ĵ�i: �Q�O�O�ͧY�N�L��/�w�g�L��\n"
	    "�ɶ�: %s\n"
	    " %s ���O�D�z�n: \n"
	    "    �U�C�Q�O�O�ͧY�N�L���Τw�g�L��:\n"
	    "------------------------------------------------------------\n",
	    CTIMEx(buf, now), bptr->brdname);
    for( i = 0 ; i < nEXP ; ++i )
	if( explist[i].expire == -1 )
	    fprintf(fp, "%-15s  �w�g�L��\n", explist[i].userid);
	else
	    fprintf(fp, "%-15s  �Y�N�b %s �L��\n",
		    explist[i].userid, CTIMEx(buf, explist[i].expire));
    fprintf(fp,
	    "------------------------------------------------------------\n"
	    "����:\n"
	    "    ���F�`�٨t�θ귽, �t�αN�۰ʲM�����W�L�|�Ӥ를�W��\n"
	    "���ϥΪ�. ���ɭY���Y��z���{�Ѫ��ϥΪ̫�n���U�F�ӱb��,\n"
	    "�N�|�����Q�O�O�ͦө��i�J.\n"
	    "    ��ĳ�z�ȮɱN�o�ǨϥΪ̦۶Q�O���O�ͦW�椤����.\n"
	    "\n"
	    "    �o�ʫH��O�ѵ{���۰ʵo�X, �Ф��n�����^�гo�ʫH. �Y\n"
	    "�z���������D�г·ЦܬݪO SYSOP, �άO�����P�ݪO�`���pô. :)\n"
	    "\n"
	    BBSNAME "�����s�q�W"
	    );
    fclose(fp);

    strcpy(mymail.title, "ĵ�i: �Q�O�O�ͧY�N�L��/�w�g�L��");
    strcpy(mymail.owner, "�t�γq��.");
    sprintf(filename, BBSHOME "/home/%c/%s/.DIR", userid[0], userid);
    mailalertuid(uid);
    append_record(filename, &mymail, sizeof(mymail));
}

void chkhbf(boardheader_t *bptr)
{
    char    fn[256], chkuser[256];
    int     i, nEXP = 0;
    FILE    *fp;
    userec_t xuser;

    sprintf(fn, "boards/%c/%s/visable", bptr->brdname[0], bptr->brdname);
    if( (fp = fopen(fn, "rt")) == NULL )
	return;
    while( fgets(chkuser, sizeof(chkuser), fp) != NULL ){
	for( i = 0 ; chkuser[i] != 0 && i < sizeof(chkuser) ; ++i )
	    if( !isalnum(chkuser[i]) ){
		chkuser[i] = 0;
		break;
	    }
	if( passwd_load_user(chkuser, &xuser) < 1 || strcasecmp(chkuser, STR_GUEST) == 0 ){
	    strcpy(explist[nEXP].userid, chkuser);
	    explist[nEXP].expire = -1;
	    ++nEXP;
	}
	else if( (xuser.lastlogin < now - 86400 * 90) &&
		 !(xuser.userlevel & PERM_XEMPT) ){
	    strcpy(explist[nEXP].userid, chkuser);
	    explist[nEXP].lastlogin = xuser.lastlogin;
	    explist[nEXP].expire = xuser.lastlogin + 86400 * 120;
	    ++nEXP;
	}
    }
    fclose(fp);
    if( nEXP ){
	char    BM[IDLEN * 3 + 3], *p;
	strlcpy(BM, bptr->BM, sizeof(BM));
	for( p = BM ; *p != 0 ; ++p )
	    if( !isalpha(*p) && !isdigit(*p) )
		*p = ' ';
	for( i = 0, p = strtok(BM, " ") ; p != NULL ;
	     ++i, p = strtok(NULL, " ") )
	    informBM(p, bptr, nEXP);
    }
}

int main(int argc, char **argv)
{
    int     ch, allboards = 0, i;
    boardheader_t  *bptr;
    while( (ch = getopt(argc, argv, "ah")) != -1 )
	switch( ch ){
	case 'a':
	    allboards = 1;
	    break;
	case 'h':
	    usage();
	    return 0;
	}

    chdir(BBSHOME);
    attach_SHM();
    argc -= optind;
    argv += optind;
    now = time(NULL);
    if( allboards ){
	for( i = 0 ; i < MAX_BOARD ; ++i ){
	    bptr = &bcache[i];
	    if( bptr->brdname[0] &&
		!(bptr->brdattr & (BRD_TOP | BRD_GROUPBOARD)) &&
		bptr->brdattr & BRD_HIDE )
		chkhbf(bptr);
	}
    }
    else if( argc > 0 ){
	int     bid;
	for( i = 0 ; i < argc ; ++i )
	    if( (bid = getbnum(argv[i])) != 0 ) // XXX: bid start 1
		chkhbf(getbcache(bid));
	    else
		fprintf(stderr, "%s not found\n", argv[i]);
    }
    else
	usage();

    return 0;
}
