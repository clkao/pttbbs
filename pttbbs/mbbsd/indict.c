/* $Id: indict.c,v 1.13 2003/06/28 08:48:36 kcwu Exp $ */
#include "bbs.h"

#define REFER "etc/dicts"

static void
addword(char word[])
{
    char            buf[150], temp[150], a[3];
    FILE           *fp = fopen(database, "r+");

    if (fp == NULL) {
	vmsg("database error");
	return;
    }
    fgets(buf, 130, fp);
    fseek(fp, 0, 2);
    if (HAVE_PERM(PERM_LOGINOK)) {
	clear();
	move(4, 0);
	outs(" \033[31mĵ�i\033[m:�Y�W�N��g����ƱN\033[36m��id\033[m�B��\n");
	snprintf(temp, sizeof(temp), "\n��J�d��\n:\033[33m%s\033[m", buf);
	outs(temp);
	outs("\n�Ш̤W�C�d�ҿ�J�@����(����enter���)\n");
	getdata(10, 0, ":", buf, 65, DOECHO);
	if (buf[0]) {
	    getdata(13, 0, "�T�w�s�W?(Y/n)", a, sizeof(a), LCECHO);
	    if (a[0] != 'n')
		fprintf(fp, "%-65s[%s]\n", buf, cuser.userid);
	}
    }
    fclose(fp);
    clear();
}

static int
choose_dict(void)
{
    int             c;
    FILE           *fp;
    char            buf[10][21], data[10][21], cho[130];

    move(12, 0);
    clrtobot();
    outs("                        "
	 "�� \033[45;33m�r��� �� �n�d���@���H\033[m ��");

    if ((fp = fopen(REFER, "r"))) {
	for (c = 0; fscanf(fp, "%s %s", buf[c], data[c]) != EOF; c++) {
	    snprintf(cho, sizeof(cho), "\n                     "
		    "(\033[36m%d\033[m) %-20s�j�r��", c + 1, buf[c]);
	    outs(cho);
	}
	fclose(fp);

	getdata(22, 14, "          �� �п�ܡA[Enter]���}�G", cho, 3, LCECHO);
	cho[0] -= '1';
	if (cho[1])
	    cho[0] = (cho[0] + 1) * 10 + (cho[1] - '1');

	if (cho[0] >= 0 && cho[0] < c) {
	    strlcpy(dict, buf[(int)cho[0]], sizeof(dict));
	    strlcpy(database, data[(int)cho[0]], sizeof(database));
	    return 1;
	} else
	    return 0;
    }
    return 0;
}

int
use_dict()
{
    FILE           *fp;
    char            lang[150], word[80] = "";
    char            j, f, buf[120], sys[] = "|\033[31me\033[m:�s��r��";
    int             i = 0;

    setutmpmode(DICT);
    if (!HAS_PERM(PERM_SYSOP))
	sys[0] = 0;

    clear();

    snprintf(buf, sizeof(buf),
	     "\033[45m                           ��\033[1;44;33m"
	     "  %-14s\033[3;45m ��                              ", dict);
    strlcpy(&buf[100], "\033[m\n", sizeof(buf) - 100);
    for (;;) {
	move(0, 0);
	snprintf(lang, sizeof(lang),
		 "  �п�J����r��(%s) �Ϋ��O(h,t,a)\n", dict);
	outs(lang);
	snprintf(lang, sizeof(lang),
		 "[\033[32m<����r>\033[m|\033[32mh\033[m:help|\033[32m"
		 "t\033[m:�Ҧ����|\033[32ma\033[m:�s�W���%s]\n:", sys);
	outs(lang);
	getdata(2, 0, ":", word, 18, DOECHO);
	outs("��Ʒj�M���еy��....");
	str_lower(word, word);
	if (word[0] == 0)
	    return 0;
	clear();
	move(4, 0);
	outs(buf);
	if (strlen(word) == 1) {
	    if (word[0] == 'a') {
		clear();
		move(4, 0);
		outs(buf);
		addword(word);
		continue;
	    } else if (word[0] == 't')
		word[0] = 0;
	    else if (word[0] == 'h') {
		more("etc/dict.hlp", YEA);
		clear();
		continue;
	    } else if (word[0] == 'e' && HAS_PERM(PERM_SYSOP)) {
		vedit(database, NA, NULL);
		clear();
		continue;
	    } else {
		outs("�r��ӵu,�п�J�h�@�I����r");
		continue;
	    }
	}
	i = 0;
	if ((fp = fopen(database, "r"))) {
	    while (fgets(lang, sizeof(lang), fp) != NULL) {
		if (lang[65] == '[') {
		    lang[65] = 0;
		    f = 1;
		} else
		    f = 0;
		if (strstr_lower(lang, word)) {
		    if (f == 1)
			lang[65] = '[';
		    outs(lang);
		    i++;
		    if (!((i + 1) % 17)) {
			move(23, 0);
			outs("\033[45m                               "
			   "���N���~��  Q:���}                             "
			     "\033[m ");
			j = igetch();
			if (j == 'q')
			    break;
			else {
			    clear();
			    move(4, 0);
			    outs(buf);
			}
		    }
		}
	    }
	    fclose(fp);
	}
	if (i == 0) {
	    getdata(5, 0, "�S�o�Ӹ�ƭC,�s�W��?(y/N)", lang, 3, LCECHO);
	    if (lang[0] == 'y') {
		clear();
		move(4, 0);
		outs(buf);
		addword(word);
	    }
	}
    }
}

int
x_dict()
{
    if (choose_dict())
	use_dict();
    return 0;
}
