/* $Id$ */
#include "bbs.h"

#ifdef ASSESS

/* do (*num) + n, n is integer. */
inline static void inc(unsigned char *num, int n)
{
    if (n >= 0 && UCHAR_MAX - *num <= n)
	(*num) = UCHAR_MAX;
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
	passwd_sync_update(uid, &xuser); \
	return xuser._attr; }\
    return 0;\
}

modify_column(badpost);  /* inc_badpost */

static char * const badpost_reason[] = {
    "�s�i", "�������", "�H������"
};

#define DIM(x)	(sizeof(x)/sizeof(x[0]))

int assign_badpost(const char *userid, fileheader_t *fhdr, 
	const char *newpath, const char *comment)
{
    char genbuf[STRLEN];
    char rptpath[PATHLEN];
    int i, tusernum = searchuser(userid, NULL);

    strcpy(rptpath, newpath);
    assert(tusernum > 0 && tusernum < MAX_USERS);
    move(b_lines - 2, 0);
    clrtobot();
    for (i = 0; i < DIM(badpost_reason); i++)
	prints("%d.%s ", i + 1, badpost_reason[i]);

    prints("%d.%s", i + 1, "��L");
    getdata(b_lines - 1, 0, "�п��[0:�����H��]:", genbuf, 3, LCECHO);
    i = genbuf[0] - '1';
    if (i < 0 || i > DIM(badpost_reason))
    {
	vmsg("�����]�w�H��C");
	return -1;
    }

    if (i < DIM(badpost_reason))
	sprintf(genbuf,"�H%s��h�^(%s)", comment ? "��" : "", badpost_reason[i]);
    else if(i==DIM(badpost_reason))
    {
	char *s = genbuf;
	strcpy(genbuf, comment ? "�H����h�^(" : "�H��h�^(");
	s += strlen(genbuf);
	getdata_buf(b_lines, 0, "�п�J��]", s, 50, DOECHO);
	// ��� comment �ثe�i�H���ӡA���Dcomment �媽���R���ҥH�S�k cancel
	if (!*s && comment)
	{
	    vmsg("�����]�w�H��C");
	    return -1;
	}
	strcat(genbuf,")");
    }

    assert(i >= 0 && i <= DIM(badpost_reason));
    if (fhdr) strncat(genbuf, fhdr->title, 64-strlen(genbuf)); 

#ifdef USE_COOLDOWN
    add_cooldowntime(tusernum, 60);
    add_posttimes(tusernum, 15); //Ptt: �ᵲ post for 1 hour
#endif

    if (!(inc_badpost(userid, 1) % 5)){
	userec_t xuser;
	post_violatelaw(userid, BBSMNAME " �t��ĵ��", 
		"�H��֭p 5 �g", "�@��@�i");
	mail_violatelaw(userid, BBSMNAME " �t��ĵ��", 
		"�H��֭p 5 �g", "�@��@�i");
	kick_all(userid);
	passwd_sync_query(tusernum, &xuser);
	xuser.money = moneyof(tusernum);
	xuser.vl_count++;
	xuser.userlevel |= PERM_VIOLATELAW;
	xuser.timeviolatelaw = now;  
	passwd_sync_update(tusernum, &xuser);
    }

#ifdef BAD_POST_RECORD
    // we also change rptpath because such record contains more information
    {
	int rpt_bid = getbnum(BAD_POST_RECORD);
	if (rpt_bid > 0) {
	    fileheader_t report_fh;
	    char rptdir[PATHLEN];
	    FILE *fp;

	    setbpath(rptpath, BAD_POST_RECORD);
	    stampfile(rptpath, &report_fh);

	    strcpy(report_fh.owner, "[" BBSMNAME "ĵ�]");
	    snprintf(report_fh.title, sizeof(report_fh.title),
		    "%s �O %s �O�D���� %s �@�g�H%s��",
		    currboard, cuser.userid, userid, comment ? "��" : "");
	    Copy(newpath, rptpath);
	    fp = fopen(rptpath, "at");

	    if (fp)
	    {
		fprintf(fp, "\n�H���]: %s\n", genbuf);
		if (comment)
		    fprintf(fp, "\n�Q�H���嶵��:\n%s", comment);
		fprintf(fp, "\n");
		fclose(fp);
	    }

	    setbdir(rptdir, BAD_POST_RECORD);
	    append_record(rptdir, &report_fh, sizeof(report_fh));

	    touchbtotal(rpt_bid);
	}
    }
#endif /* defined(BAD_POST_RECORD) */

    sendalert(userid,  ALERT_PWD_PERM);
    mail_id(userid, genbuf, rptpath, cuser.userid);

    return 0;
}

// ����ثe���]�p�@��ӴN�O�L�k�޲z�A
// ���L�b���g��M�u��^���t�Ϋe�]�O�S��k���ơA
// ���g�������檺�a
int
bad_comment(const char *fn)
{
    FILE *fp = NULL;
    char buf[ANSILINELEN];
    char uid[IDLEN+1];
    int done = 0;
    int i = 0, c;

    vs_hdr("�H����");
    usercomplete("�п�J�n�H���媺 ID: ", uid);
    if (!*uid)
	return -1;

    fp = fopen(fn, "rt");
    if (!fp)
	return -1;

    vs_hdr2(" �H���� ", uid);
    // search file for it
    while (fgets(buf, sizeof(buf), fp) && *buf)
    {
	if (strstr(buf, uid) == NULL)
	    continue;
	if (strstr(buf, ANSI_COLOR(33)) == NULL)
	    continue;

	// found something, let's show it
	move(2, 0); clrtobot();
	prints("�� %d ������:\n", ++i);
	outs(buf);

	move (5, 0);
	outs("�аݬO�n�H�o�ӱ���ܡH(Y:�T�w,N:��U��,Q:���}) [y/N/q]: ");
	c = vkey();
	if (isascii(c)) c = tolower(c);
	if (c == 'q')
	    break;
	if (c != 'y')
	    continue;

	if (assign_badpost(uid, NULL, fn, buf) != 0)
	    continue;

	done = 1;
	vmsg("�w�H����C");
	break;
    }
    fclose(fp);

    if (!done)
    {
	vmsgf("�䤣��䥦�u%s�v������F", uid);
	return -1;
    }

    return 0;
}

#endif // ASSESS
