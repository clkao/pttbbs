/* $Id: voteboard.c,v 1.18 2003/06/28 08:47:45 kcwu Exp $ */
#include "bbs.h"

#define VOTEBOARD "NewBoard"

void
do_voteboardreply(fileheader_t * fhdr)
{
    char            genbuf[1024];
    char            reason[50];
    char            fpath[80];
    char            oldfpath[80];
    char            opnion[10];
    char           *ptr;
    FILE           *fp;
    fileheader_t    votefile;
    int             len;
    int             i, j;
    int             fd;
    time_t          endtime;
    int             hastime = 0;


    clear();
    if (!(currmode & MODE_POST)) {
	move(5, 10);
	outs("�藍�_�A�z�ثe�L�k�b���o��峹�I");
	pressanykey();
	return;
    }
    setbpath(fpath, currboard);
    stampfile(fpath, &votefile);

    setbpath(oldfpath, currboard);

    strcat(oldfpath, "/");
    strcat(oldfpath, fhdr->filename);

    fp = fopen(oldfpath, "r");
    assert(fp);

    len = strlen(cuser.userid);

    while (fgets(genbuf, sizeof(genbuf), fp)) {
	if (!strncmp(genbuf, "�s�p�����ɶ�", 12)) {
	    hastime = 1;
	    ptr = strchr(genbuf, '(');
	    assert(ptr);
	    sscanf(ptr + 1, "%ld", &endtime);
	    if (endtime < now) {
		prints("�s�p�ɶ��w�L");
		pressanykey();
		fclose(fp);
		return;
	    }
	}
	if (!strncmp(genbuf + 4, cuser.userid, len)) {
	    move(5, 10);
	    prints("�z�w�g�s�p�L���g�F");
	    opnion[0] = 'n';
	    getdata(7, 0, "�n�ק�z���e���s�p�ܡH(Y/N) [N]", opnion, 3, LCECHO);
	    if (opnion[0] != 'y') {
		fclose(fp);
		return;
	    }
	    strlcpy(reason, genbuf + 19, sizeof(reason));
	}
    }
    fclose(fp);

    if ((fd = open(oldfpath, O_RDONLY)) == -1)
	return;

    fp = fopen(fpath, "w");

    if (!fp)
	return;
    i = 0;
    while (fp) {
	j = 0;
	do {
	    if (read(fd, genbuf + j, 1) <= 0) {
		flock(fd, LOCK_UN);
		close(fd);
		fclose(fp);
		unlink(fpath);
		return;
	    }
	    j++;
	} while (genbuf[j - 1] != '\n');
	genbuf[j] = '\0';
	i++;
	if (!strncmp("----------", genbuf, 10))
	    break;
	if (i > 3)
	    prints(genbuf);
	fprintf(fp, "%s", genbuf);
    }
    if (!hastime) {
	now += 14 * 24 * 60 * 60;
	fprintf(fp, "�s�p�����ɶ�: (%ld)%s", now, ctime(&now));
	now -= 14 * 24 * 60 * 60;
    }
    fprintf(fp, "%s", genbuf);

    do {
	if (!getdata(18, 0, "�аݱz (Y)��� (N)�Ϲ� �o��ĳ�D�G", opnion, 3, LCECHO)) {
	    flock(fd, LOCK_UN);
	    close(fd);
	    fclose(fp);
	    unlink(fpath);
	    return;
	}
    } while (opnion[0] != 'y' && opnion[0] != 'n');

    if (!getdata(20, 0, "�аݱz�P�o��ĳ�D�����Y�γs�p�z�Ѭ���G",
		 reason, sizeof(reason), DOECHO)) {
	flock(fd, LOCK_UN);
	close(fd);
	fclose(fp);
	unlink(fpath);
	return;
    }
    flock(fd, LOCK_EX);
    i = 0;

    while (fp) {
	i++;
	j = 0;
	do {
	    if (read(fd, genbuf + j, 1) <= 0) {
		flock(fd, LOCK_UN);
		close(fd);
		fclose(fp);
		unlink(fpath);
		return;
	    }
	    j++;
	} while (genbuf[j - 1] != '\n');
	genbuf[j] = '\0';
	if (!strncmp("----------", genbuf, 10))
	    break;
	if (strncmp(genbuf + 4, cuser.userid, len))
	    fprintf(fp, "%3d.%s", i, genbuf + 4);
	else
	    i--;
    }
    if (opnion[0] == 'y')
	fprintf(fp, "%3d.%-15s%-34s �ӷ�:%s\n", i, cuser.userid, reason, cuser.lasthost);
    i = 0;
    fprintf(fp, "%s", genbuf);
    while (fp) {
	i++;
	j = 0;
	do {
	    if (!read(fd, genbuf + j, 1))
		break;
	    j++;
	} while (genbuf[j - 1] != '\n');
	genbuf[j] = '\0';
	if (j <= 3)
	    break;
	if (strncmp(genbuf + 4, cuser.userid, len))
	    fprintf(fp, "%3d.%s", i, genbuf + 4);
	else
	    i--;
    }
    if (opnion[0] == 'n')
	fprintf(fp, "%3d.%-15s%-34s �ӷ�:%s\n", i, cuser.userid, reason, cuser.lasthost);
    flock(fd, LOCK_UN);
    close(fd);
    fclose(fp);
    unlink(oldfpath);
    rename(fpath, oldfpath);
}

int
do_voteboard()
{
    fileheader_t    votefile;
    char            topic[100];
    char            title[80];
    char            genbuf[1024];
    char            fpath[80];
    FILE           *fp;
    int             temp, i;

    clear();
    if (!(currmode & MODE_POST)) {
	move(5, 10);
	outs("�藍�_�A�z�ثe�L�k�b���o��峹�I");
	pressanykey();
	return FULLUPDATE;
    }
    move(0, 0);
    clrtobot();
    prints("�z���b�ϥ� PTT ���s�p�t��\n");
    prints("���s�p�t�αN�߰ݱz�@�ǰ��D�A�Фp�ߦ^���~��}�l�s�p\n");
    prints("���N���X�s�p�ת̡A�N�Q�C�J���t�Τ����w��ϥΪ̳�\n");
    pressanykey();
    move(0, 0);
    clrtobot();
    prints("(1)�ӽзs�O (2)�o���ªO (3)�s�p�O�D (4)�}�K�O�D\n");
    if (!strcmp(currboard, VOTEBOARD))
	prints("(5)�s�p�p�ժ� (6)�}�K�p�ժ� ");
    if (!strcmp(currboard, VOTEBOARD) && HAS_PERM(PERM_SYSOP))
	prints("(7)��������");
    prints("(8)�ӽзs�s��");

    do {
	getdata(3, 0, "�п�J�s�p���O�G", topic, 3, DOECHO);
	temp = atoi(topic);
    } while (temp <= 0 && temp >= 9);

    switch (temp) {
    case 1:
	do {
	    if (!getdata(4, 0, "�п�J�ݪO�^��W�١G", topic, IDLEN + 1, DOECHO))
		return FULLUPDATE;
	    else if (invalid_brdname(topic))
		outs("���O���T���ݪO�W��");
	    else if (getbnum(topic) > 0)
		outs("���W�٤w�g�s�b");
	    else
		break;
	} while (temp > 0);
	snprintf(title, sizeof(title), "[�ӽзs�O] %s", topic);
	snprintf(genbuf, sizeof(genbuf),
		 "%s\n\n%s%s\n%s", "�ӽзs�O", "�^��W��: ", topic, "����W��: ");

	if (!getdata(5, 0, "�п�J�ݪO����W�١G", topic, 20, DOECHO))
	    return FULLUPDATE;
	strcat(genbuf, topic);
	strcat(genbuf, "\n�ݪO���O: ");
	if (!getdata(6, 0, "�п�J�ݪO���O�G", topic, 20, DOECHO))
	    return FULLUPDATE;
	strcat(genbuf, topic);
	strcat(genbuf, "\n�O�D�W��: ");
	getdata(7, 0, "�п�J�O�D�W��G", topic, IDLEN * 3 + 3, DOECHO);
	strcat(genbuf, topic);
	strcat(genbuf, "\n�ӽЭ�]: \n");
	outs("�п�J�ӽЭ�](�ܦh����)�A�n�M����g���M���|�֭��");
	for (i = 9; i < 13; i++) {
	    if (!getdata(i, 0, "�G", topic, 60, DOECHO))
		break;
	    strcat(genbuf, topic);
	    strcat(genbuf, "\n");
	}
	if (i == 9)
	    return FULLUPDATE;
	break;
    case 2:
	do {
	    if (!getdata(4, 0, "�п�J�ݪO�^��W�١G", topic, IDLEN + 1, DOECHO))
		return FULLUPDATE;
	    else if (getbnum(topic) <= 0)
		outs("���W�٨ä��s�b");
	    else
		break;
	} while (temp > 0);
	snprintf(title, sizeof(title), "[�o���ªO] %s", topic);
	snprintf(genbuf, sizeof(genbuf),
		 "%s\n\n%s%s\n", "�o���ªO", "�^��W��: ", topic);
	strcat(genbuf, "\n�o����]: \n");
	outs("�п�J�o����](�ܦh����)�A�n�M����g���M���|�֭��");
	for (i = 8; i < 13; i++) {
	    if (!getdata(i, 0, "�G", topic, 60, DOECHO))
		break;
	    strcat(genbuf, topic);
	    strcat(genbuf, "\n");
	}
	if (i == 8)
	    return FULLUPDATE;

	break;
    case 3:
	do {
	    if (!getdata(4, 0, "�п�J�ݪO�^��W�١G", topic, IDLEN + 1, DOECHO))
		return FULLUPDATE;
	    else if (getbnum(topic) <= 0)
		outs("���W�٨ä��s�b");
	    else
		break;
	} while (temp > 0);
	snprintf(title, sizeof(title), "[�s�p�O�D] %s", topic);
	snprintf(genbuf, sizeof(genbuf), "%s\n\n%s%s\n%s%s", "�s�p�O�D", "�^��W��: ", topic, "�ӽ� ID : ", cuser.userid);
	strcat(genbuf, "\n�ӽЬF��: \n");
	outs("�п�J�ӽЬF��(�ܦh����)�A�n�M����g���M���|�֭��");
	for (i = 8; i < 13; i++) {
	    if (!getdata(i, 0, "�G", topic, 60, DOECHO))
		break;
	    strcat(genbuf, topic);
	    strcat(genbuf, "\n");
	}
	if (i == 8)
	    return FULLUPDATE;
	break;
    case 4:
	do {
	    if (!getdata(4, 0, "�п�J�ݪO�^��W�١G", topic, IDLEN + 1, DOECHO))
		return FULLUPDATE;
	    else if ((i = getbnum(topic)) <= 0)
		outs("���W�٨ä��s�b");
	    else
		break;
	} while (temp > 0);
	snprintf(title, sizeof(title), "[�}�K�O�D] %s", topic);
	snprintf(genbuf, sizeof(genbuf),
		 "%s\n\n%s%s\n%s", "�}�K�O�D", "�^��W��: ",
		 topic, "�O�D ID : ");
	do {
	    if (!getdata(6, 0, "�п�J�O�DID�G", topic, IDLEN + 1, DOECHO))
		return FULLUPDATE;
	    else if (!userid_is_BM(topic, bcache[i - 1].BM))
		outs("���O�ӪO���O�D");
	    else
		break;
	} while (temp > 0);
	strcat(genbuf, topic);
	strcat(genbuf, "\n�}�K��]: \n");
	outs("�п�J�}�K��](�ܦh����)�A�n�M����g���M���|�֭��");
	for (i = 8; i < 13; i++) {
	    if (!getdata(i, 0, "�G", topic, 60, DOECHO))
		break;
	    strcat(genbuf, topic);
	    strcat(genbuf, "\n");
	}
	if (i == 8)
	    return FULLUPDATE;
	break;
    case 5:
	if (!getdata(4, 0, "�п�J�p�դ��^��W�١G", topic, 30, DOECHO))
	    return FULLUPDATE;
	snprintf(title, sizeof(title), "[�s�p�p�ժ�] %s", topic);
	snprintf(genbuf, sizeof(genbuf),
		 "%s\n\n%s%s\n%s%s", "�s�p�p�ժ�", "�p�զW��: ",
		 topic, "�ӽ� ID : ", cuser.userid);
	strcat(genbuf, "\n�ӽЬF��: \n");
	outs("�п�J�ӽЬF��(�ܦh����)�A�n�M����g���M���|�֭��");
	for (i = 8; i < 13; i++) {
	    if (!getdata(i, 0, "�G", topic, 60, DOECHO))
		break;
	    strcat(genbuf, topic);
	    strcat(genbuf, "\n");
	}
	if (i == 8)
	    return FULLUPDATE;
	break;
    case 6:

	if (!getdata(4, 0, "�п�J�p�դ��^��W�١G", topic, 30, DOECHO))
	    return FULLUPDATE;
	snprintf(title, sizeof(title), "[�}�K�p�ժ�] %s", topic);
	snprintf(genbuf, sizeof(genbuf), "%s\n\n%s%s\n%s",
		 "�}�K�p�ժ�", "�p�զW��: ", topic, "�p�ժ� ID : ");
	if (!getdata(6, 0, "�п�J�p�ժ�ID�G", topic, IDLEN + 1, DOECHO))
	    return FULLUPDATE;
	strcat(genbuf, topic);
	strcat(genbuf, "\n�}�K��]: \n");
	outs("�п�J�}�K��](�ܦh����)�A�n�M����g���M���|�֭��");
	for (i = 8; i < 13; i++) {
	    if (!getdata(i, 0, "�G", topic, 60, DOECHO))
		break;
	    strcat(genbuf, topic);
	    strcat(genbuf, "\n");
	}
	if (i == 8)
	    return FULLUPDATE;
	break;
    case 7:
	if (!HAS_PERM(PERM_SYSOP))
	    return FULLUPDATE;
	if (!getdata(4, 0, "�п�J����D�D�G", topic, 30, DOECHO))
	    return FULLUPDATE;
	snprintf(title, sizeof(title), "%s %s", "[��������]", topic);
	snprintf(genbuf, sizeof(genbuf),
		 "%s\n\n%s%s\n", "��������", "����D�D: ", topic);
	strcat(genbuf, "\n�����]: \n");
	outs("�п�J�����](�ܦh����)�A�n�M����g���M���|�֭��");
	for (i = 8; i < 13; i++) {
	    if (!getdata(i, 0, "�G", topic, 60, DOECHO))
		break;
	    strcat(genbuf, topic);
	    strcat(genbuf, "\n");
	}
	if (i == 8)
	    return FULLUPDATE;
	break;
    case 8:
	if (!getdata(4, 0, "�п�J�s�դ��^��W�١G", topic, 30, DOECHO))
	    return FULLUPDATE;
	snprintf(title, sizeof(title), "[�ӽзs�s��] %s", topic);
	snprintf(genbuf, sizeof(genbuf), "%s\n\n%s%s\n%s%s",
		 "�ӽиs��", "�s�զW��: ", topic, "�ӽ� ID : ", cuser.userid);
	strcat(genbuf, "\n�ӽЬF��: \n");
	outs("�п�J�ӽЬF��(�ܦh����)�A�n�M����g���M���|�֭��");
	for (i = 8; i < 13; i++) {
	    if (!getdata(i, 0, "�G", topic, 60, DOECHO))
		break;
	    strcat(genbuf, topic);
	    strcat(genbuf, "\n");
	}
	if (i == 8)
	    return FULLUPDATE;
	break;
    default:
	return FULLUPDATE;
    }
    strcat(genbuf, "�s�p�����ɶ�: ");
    now += 14 * 24 * 60 * 60;
    snprintf(topic, sizeof(topic), "(%ld)", now);
    strcat(genbuf, topic);
    strcat(genbuf, ctime(&now));
    now -= 14 * 24 * 60 * 60;
    strcat(genbuf, "----------���----------\n");
    strcat(genbuf, "----------�Ϲ�----------\n");
    outs("�}�l�s�p��");
    setbpath(fpath, currboard);
    stampfile(fpath, &votefile);

    if (!(fp = fopen(fpath, "w"))) {
	outs("�}�ɥ��ѡA�еy�ԭ��Ӥ@��");
	return FULLUPDATE;
    }
    fprintf(fp, "%s%s %s%s\n%s%s\n%s%s", "�@��: ", cuser.userid,
	    "�ݪO: ", currboard,
	    "���D: ", title,
	    "�ɶ�: ", ctime(&now));
    fprintf(fp, "%s\n", genbuf);
    fclose(fp);
    strlcpy(votefile.owner, cuser.userid, sizeof(votefile.owner));
    strlcpy(votefile.title, title, sizeof(votefile.title));
    setbdir(genbuf, currboard);
    if (append_record(genbuf, &votefile, sizeof(votefile)) != -1)
	setbtotal(currbid);
    do_voteboardreply(&votefile);
    return FULLUPDATE;
}
