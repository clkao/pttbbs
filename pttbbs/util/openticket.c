/* $Id$ */
/* �}���� utility */
#define _UTIL_C_
#include "bbs.h"

static char *betname[8] = {"Ptt", "Jaky",  "Action",  "Heat",
			   "DUNK", "Jungo", "waiting", "wofe"};

#define MAX_DES 7		/* �̤j�O�d���� */

int Link(const char *src, const char *dst)
{
    char cmd[200];

    if (link(src, dst) == 0)
	return 0;

    sprintf(cmd, "/bin/cp -R %s %s", src, dst);
    return system(cmd);
}

int main(int argc, char **argv)
{
    int money, bet, n, total = 0, ticket[8] =
    {0, 0, 0, 0, 0, 0, 0, 0};
    FILE *fp;
    time4_t now = (time4_t)time(NULL);
    char des[MAX_DES][200] =
    {"", "", "", ""};

    nice(10);
    attach_SHM();
    if(passwd_init())
	exit(1);
    
    rename(BBSHOME "/etc/" FN_TICKET_RECORD,
           BBSHOME "/etc/" FN_TICKET_RECORD ".tmp");
    rename(BBSHOME "/etc/" FN_TICKET_USER,
           BBSHOME "/etc/" FN_TICKET_USER ".tmp");

    if (!(fp = fopen(BBSHOME "/etc/"FN_TICKET_RECORD ".tmp", "r")))
	return 0;
    fscanf(fp, "%9d %9d %9d %9d %9d %9d %9d %9d\n",
	   &ticket[0], &ticket[1], &ticket[2], &ticket[3],
	   &ticket[4], &ticket[5], &ticket[6], &ticket[7]);
    for (n = 0; n < 8; n++)
	total += ticket[n];
    fclose(fp);

    if (!total)
	return 0;

    if((fp = fopen(BBSHOME "/etc/" FN_TICKET , "r")))
    {
	for (n = 0; n < MAX_DES && fgets(des[n], 200, fp); n++) ;
	fclose(fp);
    }

    /* �{�b�}�����X�èS�Ψ� random function.
     * �p���� UTMPnumber �i�����w��, �B UTMPnumber �w�]�@��~��s�@��
     * �}���@�q�ɶ����}�� pid ���ӵL�k�w��.
     * �Y�O�p�����}���e�}��, �h���Q�q�����i�� */
    attach_SHM();
    bet = (SHM->UTMPnumber+getpid()) % 8;



    money = ticket[bet] ? total * 95 / ticket[bet] : 9999999;

    if((fp = fopen(BBSHOME "/etc/" FN_TICKET, "w")))
    {
	if (des[MAX_DES - 1][0])
	    n = 1;
	else
	    n = 0;

	for (; n < MAX_DES && des[n][0] != 0; n++)
	{
	    fprintf(fp, des[n]);
	}

	printf("\n\n�}���ɶ��G %s \n\n"
	       "�}�����G�G %s \n\n"
	       "�U�`�`���B�G %d00 �� \n"
	       "������ҡG %d�i/%d�i  (%f)\n"
	       "�C�i�����m���i�o %d �T" MONEYNAME "�� \n\n",
	       Cdatelite(&now), betname[bet], total, ticket[bet], total,
	       (float) ticket[bet] / total, money);

	fprintf(fp, "%s ��L�}�X:%s �`���B:%d00 �� ����/�i:%d �� ���v:%1.2f\n",
		Cdatelite(&now), betname[bet], total, money,
		(float) ticket[bet] / total);
	fclose(fp);

    }

    if (ticket[bet] && (fp = fopen(BBSHOME "/etc/" FN_TICKET_USER ".tmp", "r")))
    {
	int mybet, num, uid;
	char userid[20], genbuf[200];
	fileheader_t mymail;

	while (fscanf(fp, "%s %d %d\n", userid, &mybet, &num) != EOF)
	{
	    if (mybet == bet)
	    {
		printf("���� %-15s�R�F%9d �i %s, ��o %d �T" MONEYNAME "��\n"
		       ,userid, num, betname[mybet], money * num);
                if((uid=searchuser(userid, userid))==0) continue;
		deumoney(uid, money * num);
		sprintf(genbuf, BBSHOME "/home/%c/%s", userid[0], userid);
		stampfile(genbuf, &mymail);
		strcpy(mymail.owner, BBSNAME);
		sprintf(mymail.title, "[%s] �����o! $ %d", Cdatelite(&now), money * num);
		unlink(genbuf);
		Link(BBSHOME "/etc/ticket", genbuf);
		sprintf(genbuf, BBSHOME "/home/%c/%s/.DIR", userid[0], userid);
		append_record(genbuf, &mymail, sizeof(mymail));
	    }
	}
    }
    unlink(BBSHOME "/etc/" FN_TICKET_RECORD ".tmp");
    unlink(BBSHOME "/etc/" FN_TICKET_USER ".tmp");
    return 0;
}
