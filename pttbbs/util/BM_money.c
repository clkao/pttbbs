/* $Id$ */

/* ���O�D�����{�� */

#define _UTIL_C_
#include "bbs.h"

#define FUNCTION    (2100 - c*5)

extern int numboards;
extern boardheader_t *bcache;
extern struct UCACHE *uidshm;

int c, n;



int Link(const char *src, const char *dst) {
    char cmd[200];

    if (link(src, dst) == 0)
	return 0;

    sprintf(cmd, "/bin/cp -R %s %s", src, dst);
    return system(cmd);
}


int main(int argc, char **argv)
{
    FILE *fp = fopen(BBSHOME "/etc/topboardman", "r");
    char buf[201], bname[20], BM[90], *ch;
    boardheader_t *bptr = NULL;
    int nBM;

    attach_SHM();
    resolve_boards();
    if(passwd_init())
	exit(1);
    if (!fp)
	return 0;

    c = 0;
    fgets(buf, 200, fp);	/* �Ĥ@�殳�� */

    printf(
	"              \033[1;44m  ���y�u�}�O�D �C�g���~ �̺�ذϱƦW���t  \033[m\n\n"
	"\033[33m                 (�ƦW�ӫ᭱�δX�G�S����ذϪ̤��C�J)\033[m\n"
	"  �w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w\n"
	"\n\n");

    while (fgets(buf, 200, fp) != NULL)
    {
	buf[24] = 0;
	sscanf(&buf[9], "%s", bname);
	for (n = 0; n < numboards; n++)
	{
	    bptr = &bcache[n];
	    if (!strcmp(bptr->brdname, bname))
		break;
	}
	if (n == numboards)
	    continue;
	strcpy(BM, bptr->BM);
	printf("        (%d) %-15.15s %s \n", c + 1, bptr->brdname, bptr->title);

	if (BM[0] == 0 || BM[0] == ' ')
	    continue;

	ch = BM;
	for (nBM = 1; (ch = strchr(ch, '/')) != NULL; nBM++)
	{
	    ch++;
	};
	ch = BM;

	if (FUNCTION <= 0)
	    break;

	printf("             ���� \033[32m%6d \033[m     ���� \033[33m%s\033[m \n",
	       FUNCTION, bptr->BM);

	for (n = 0; n < nBM; n++)
	{
	    fileheader_t mymail;
	    char *ch1,uid	;
	    if((ch1 = strchr(ch, '/')))
		*ch1 = 0;
            if ((uid=searchuser(ch, ch))!=0)
	    {
            
		char genbuf[200];
                deumoney(uid,FUNCTION / nBM);
		sprintf(genbuf, BBSHOME "/home/%c/%s", ch[0], ch);
		stampfile(genbuf, &mymail);

		strcpy(mymail.owner, "[�~���U]");
		sprintf(mymail.title,
			"\033[32m %s \033[m�O���~�� �C\033[33m%d\033![m", bptr->brdname, FUNCTION / nBM);
		unlink(genbuf);
		Link(BBSHOME "/etc/BM_money", genbuf);
		sprintf(genbuf, BBSHOME "/home/%c/%s/.DIR", ch[0], ch);
		append_record(genbuf, &mymail, sizeof(mymail));
	    }
	    ch = ch1 + 1;
	}
	c++;
    }
    return 0;
}
