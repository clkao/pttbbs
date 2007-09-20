/* $Id$ */
#include "bbs.h"
#define DATA "etc/lovepaper.dat"

int
x_love(void)
{
    char            buf1[200], title[TTLEN + 1];
    char            receiver[61], path[STRLEN] = "home/";
    int             x, y = 0, tline = 0, poem = 0;
    FILE           *fp, *fpo;
    struct tm      *gtime;
    fileheader_t    mhdr;

    setutmpmode(LOVE);
    gtime = localtime4(&now);
    snprintf(buf1, sizeof(buf1), "%c/%s/love%d%d",
	    cuser.userid[0], cuser.userid, gtime->tm_sec, gtime->tm_min);
    strcat(path, buf1);
    move(1, 0);
    clrtobot();

    outs("\n�w��ϥα��Ѳ��;� v0.00 �O \n");
    outs("�������H�Ҿ�����,��Ѩt�����A���a.\n������ : �ݱ����Ǫk.\n");

    if (!getdata(7, 0, "���H�H�G", receiver, sizeof(receiver), DOECHO))
	return 0;
    if (receiver[0] && !(searchuser(receiver, receiver) &&
			 getdata(8, 0, "�D  �D�G", title,
				 sizeof(title), DOECHO))) {
	move(10, 0);
	vmsg("���H�H�ΥD�D�����T,���ѵL�k�ǻ�");
	return 0;
    }
    fpo = fopen(path, "w");
    assert(fpo);
    fprintf(fpo, "\n");
    if ((fp = fopen(DATA, "r"))) {
	while (fgets(buf1, 100, fp)) {
	    switch (buf1[0]) {
	    case '#':
		break;
	    case '@':
		if (!strncmp(buf1, "@begin", 6) || !strncmp(buf1, "@end", 4))
		    tline = 3;
		else if (!strncmp(buf1, "@poem", 5)) {
		    poem = 1;
		    tline = 1;
		    fprintf(fpo, "\n\n");
		} else
		    tline = 2;
		break;
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
		sscanf(buf1, "%d", &x);
		y = (random() % (x - 1)) * tline;
		break;
	    default:
		if (!poem) {
		    if (y > 0)
			y = y - 1;
		    else {
			if (tline > 0) {
			    fputs(buf1, fpo);
			    tline--;
			}
		    }
		} else {
		    if (buf1[0] == '$')
			y--;
		    else if (y == 0)
			fputs(buf1, fpo);
		}
	    }

	}

	fclose(fp);
	fclose(fpo);
	if (vedit(path, YEA, NULL) == -1) {
	    unlink(path);
	    clear();
	    outs("\n\n ���H����\n");
	    pressanykey();
	    return -2;
	}
	sethomepath(buf1, receiver);
	stampfile(buf1, &mhdr);
	Rename(path, buf1);
	strlcpy(mhdr.title, title, sizeof(mhdr.title));
	strlcpy(mhdr.owner, cuser.userid, sizeof(mhdr.owner));
	sethomedir(path, receiver);
	if (append_record(path, &mhdr, sizeof(mhdr)) == -1)
	    return -1;
	hold_mail(buf1, receiver);
	return 1;
    }
    fclose(fpo);
    return 0;
}
