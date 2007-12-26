/* $Id$ */
#include "bbs.h"

#ifdef ASSESS

/* do (*num) + n, n is integer. */
inline static void inc(unsigned char *num, int n)
{
    if (n >= 0 && SALE_MAXVALUE - *num <= n)
	(*num) = SALE_MAXVALUE;
    else if (n < 0 && *num < -n)
	(*num) = 0;
    else
	(*num) += n;
}

#define modify_column(_attr) \
int inc_##_attr(const char *userid, int num) \
{ \
    userec_t xuser; \
    int uid = getuser(userid, &xuser);\
    if( uid > 0 ){ \
	inc(&xuser._attr, num); \
	passwd_update(uid, &xuser); \
	return xuser._attr; }\
    return 0;\
}

modify_column(goodpost); /* inc_goodpost */
modify_column(badpost);  /* inc_badpost */
modify_column(goodsale); /* inc_goodsale */
modify_column(badsale);  /* inc_badsale */

#if 0 //unused function
void set_assess(const char *userid, unsigned char num, int type)
{
    userec_t xuser;
    int uid = getuser(userid, &xuser);
    if(uid<=0) return;
    switch (type){
	case GOODPOST:
	    xuser.goodpost = num;
	    break;
	case BADPOST:
	    xuser.badpost = num;
	    break;
	case GOODSALE:
	    xuser.goodsale = num;
	    break;
	case BADSALE:
	    xuser.badsale = num;
	    break;
    }
    passwd_update(uid, &xuser);
}
#endif

// how long is AID? see read.c...
#ifndef AIDC_LEN
#define AIDC_LEN (20)
#endif // AIDC_LEN

#define MAXGP (100)

int 
u_fixgoodpost(void)
{
    char endinput = 0, newgp = 0;
    int bid;
    char bname[IDLEN+1];
    char xaidc[AIDC_LEN+1];

    aidu_t gpaids[MAXGP+1];
    int    gpbids[MAXGP+1];
    int    cgps = 0;

    clear();
    stand_title("�۰��u��ץ��{��");
    
    outs("�}�l�ץ��u�大�e�A���ǥ\\�ҭn�·бz���d�n�G\n\n"
	 "�Х����A�Ҧ����u��峹���ݪ��P" AID_DISPLAYNAME "\n"
	 AID_DISPLAYNAME "���d�ߤ�k�O�b�ӽg�峹�e�����U�j�g Q �C\n"
	 "�d�n��Ч�o�Ǹ�Ʃ�b����A���U�|�бz��J�C\n\n");
    outs("�p�G�@�����ǳƦn�F�A�Ы��U y �}�l�A�Ψ䥦���N����X�C\n\n");
    if (getans("�u�媺��Ƴ��d�n�F�ܡH") != 'y')
    {
	vmsg("���X�ץ��{���C");
	return 0;
    }
    while (!endinput && newgp < MAXGP)
    {
	int y, x = 0;
	boardheader_t *bh = NULL;

	move(1, 0); clrtobot();
	outs("�Ш̧ǿ�J�u���T�A����������� ENTER �Y�i����C\n");

	move(b_lines-2, 0); clrtobot();
	prints("�ثe�w�T�{�u��ƥ�: %d" ANSI_RESET "\n\n", newgp);

	if (!getdata(5, 0, "�п�J�u��峹�Ҧb�ݪO�W��: ", 
		    bname, sizeof(bname), DOECHO))
	{
	    move(5, 0); 
	    if (getans(ANSI_COLOR(1;33)"�T�w������J�����F�ܡH "
			ANSI_RESET "[y/N]: ") != 'y')
		continue;
	    endinput = 1; break;
	}
	move (6, 0);
	outs("�T�{�ݪO... ");
	if (bname[0] == '\0' || !(bid = getbnum(bname)))
	{
	    outs(ANSI_COLOR(1;31) "�ݪO���s�b�I");
	    vmsg("�Э��s��J�C");
	    continue;
	}
	assert(0<=bid-1 && bid-1<MAX_BOARD);
	bh = getbcache(bid);
	strlcpy(bname, bh->brdname, sizeof(bname));
	prints("�w���ݪO --> %s\n", bname);
	getyx(&y, &x);

	// loop AID query
	while (newgp < MAXGP)
	{
	    int n;
	    int fd;
	    char dirfile[PATHLEN];
	    char *sp;
	    aidu_t aidu = 0;
	    fileheader_t fh;

	    move(y, 0); clrtobot();
	    move(b_lines-2, 0); clrtobot();
	    prints("�ثe�w�T�{�u��ƥ�: %d" ANSI_RESET "\n\n", newgp);

	    if (getdata(y, 0, "�п�J" AID_DISPLAYNAME ": #",
		    xaidc, AIDC_LEN, DOECHO) == 0)
		break;

	    sp = xaidc;
	    while(*sp == ' ') sp ++;
	    if(*sp == '#') sp ++;

	    if((aidu = aidc2aidu(sp)) <= 0)
	    {
		outs(ANSI_COLOR(1;31) AID_DISPLAYNAME "�榡�����T�I");
		vmsg("�Э��s��J�C");
		continue;
	    }

	    // check repeated input of same board+AID.
	    for (n = 0; n < cgps; n++)
	    {
		if (gpaids[n] == aidu && gpbids[n] == bid)
		{
		    vmsg("�z�w��J�L���u��F�A�Э��s��J�C");
		    aidu = 0;
		    break;
		}
	    }

	    if (aidu <= 0)
		continue;
	    
	    // find aidu in board
	    n = -1;
	    // see read.c, search .bottom first.
	    if (n < 0)
	    {
		outs("�j�M�m���峹...");
		setbfile(dirfile, bname, FN_DIR ".bottom");
		n = search_aidu(dirfile, aidu);
	    }
	    if (n < 0) {
		// search board
		outs("�����C\n�j�M�ݪO�峹..");
		setbfile(dirfile, bname, FN_DIR);
		n = search_aidu(dirfile, aidu);
	    }
	    if (n < 0)
	    {
		// search digest
		outs("�����C\n�j�M��K..");
		setbfile(dirfile, currboard, fn_mandex);
		n = search_aidu(dirfile, aidu);
	    } 
	    if (n < 0) 
	    {
		// search failed...
		outs("�����\n" ANSI_COLOR(1;31) "�䤣��峹�I");
		vmsg("�нT�{�᭫�s��J�C");
		continue;
	    }

	    // found something
	    fd = open(dirfile, O_RDONLY);
	    if (fd < 0)
	    {
		outs(ANSI_COLOR(1;31) "�t�ο��~�C �еy�ԦA���աC\n");
		vmsg("�Y����o�ͽЦ�" GLOBAL_BUGREPORT "���i�C");
		continue;
	    }

	    lseek(fd, n*sizeof(fileheader_t), SEEK_SET);
	    memset(&fh, 0, sizeof(fh));
	    read(fd, &fh, sizeof(fh));
	    outs("\n�}�l�ֹ���...\n");
	    n = 1;
	    if (strcmp(fh.owner, cuser.userid) != 0)
		n = 0;
	    prints("�@��: %s (%s)\n", fh.owner, n ? "���T" : 
		    ANSI_COLOR(1;31) "���~" ANSI_RESET);
	    if (!(fh.filemode & FILE_MARKED))
		n = 0;
	    prints("�ۤ�: %s\n", (fh.filemode & FILE_MARKED) ? "���T" : 
		    ANSI_COLOR(1;31) "���~" ANSI_RESET);
	    prints("����: %d\n", fh.recommend);
	    close(fd);
	    if (!n)
	    {
		vmsg("��J���峹�ëD�u��A�Э��s��J�C");
		continue;
	    }
	    n = fh.recommend / 10;
	    prints("�p���u��ƭ�: %+d\n", n);

	    if (n > 0)
	    {
		// log new data
		newgp += n;
		gpaids[cgps] = aidu;
		gpbids[cgps] = bid;
		cgps ++;
	    }

	    clrtobot();


	    vmsg("�u��w�T�{�C�Y�n��J�䥦�ݪO�峹�ЦbAID��ťի� ENTER");
	}
	vmsgf("%s �ݪO��J�����C", bname);
    }
    if (newgp <= cuser.goodpost)
    {
	vmsg("�T�{�u��ƥاC��w���u��ơA���վ�C");
    } else {
	if (newgp > MAXGP)
	    newgp = MAXGP;
	log_filef("log/fixgoodpost.log", LOG_CREAT,
	        "%s %s �۰ʭץ��u���: �� %d �ܬ� %d\n", Cdate(&now), cuser.userid,
		cuser.goodpost, newgp);
	cuser.goodpost = newgp;
	// update passwd file here?
	passwd_update(usernum, &cuser);
	vmsgf("��s�u��ƥج�%d�C", newgp);
    }

    return 0;
}

#endif
