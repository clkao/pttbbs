/* $Id$ */
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
    int             yes=0, no=0,*yn=NULL;
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
        if(yn)
           {
            if(!strncmp("----------", genbuf, 10))
                    yn=&no;
            else
                    *yn++;
           }
        else if (!strncmp("----------", genbuf, 10))
            yn=&yes;
   
	if (!strncmp(genbuf + 4, cuser.userid, len)) {
	    move(5, 10);
	    prints("�z�w�g�s�p�L���g�F");
	    getdata(7, 0, "�n�ק�z���e���s�p�ܡH(Y/N) [N]", opnion, 3, LCECHO);
            *yn--; /* ����L����s�p�����@������ */
	    if (opnion[0] != 'y') {
		fclose(fp);
		return;
	    }
	    strlcpy(reason, genbuf + 19, sizeof(reason));
            break;
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
    if(opnion[0]=='y')
       yes++;
    else
       no++;
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
        if (!strncmp("����H��:",genbuf,9))
            fprintf(fp, "����H��:%-9d�Ϲ�H��:%-9d", yes, no); 
	else if (!strncmp("----------", genbuf, 10))
	    break;
	else if (strncmp(genbuf + 4, cuser.userid, len))
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
do_voteboard(int type)
{
    fileheader_t    votefile;
    char            topic[100];
    char            title[80];
    char            genbuf[1024];
    char            fpath[80];
    FILE           *fp;
    int             temp;

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
    prints("���N���X�s�p�ת̡A�N�Q�C�J�����w��ϥΪ̳�\n");
    move(4, 0);
    clrtobot();
    prints("(1)���ʳs�p (2)�O�W����");
    if(type==0)
      prints("(3)�ӽзs�O (4)�o���ªO (5)�s�p�O�D \n(6)�}�K�O�D (7)�s�p�p�ժ� (8)�}�K�p�ժ� (9)�ӽзs�s��\n");

    do {
	getdata(6, 0, "�п�J�s�p���O [0:����]�G", topic, 3, DOECHO);
	temp = atoi(topic);
    } while (temp < 0 || temp > 9 || (type && temp>2));
    switch (temp) {
    case 0:
         return FULLUPDATE;
    case 1:
	if (!getdata(7, 0, "�п�J���ʥD�D�G", topic, 30, DOECHO))
	    return FULLUPDATE;
	snprintf(title, sizeof(title), "%s %s", "[���ʳs�p]", topic);
	snprintf(genbuf, sizeof(genbuf),
		 "%s\n\n%s%s\n", "���ʳs�p", "���ʥD�D: ", topic);
	strcat(genbuf, "\n���ʤ��e: \n");
	break;
    case 2:
	if (!getdata(7, 0, "�п�J����D�D�G", topic, 30, DOECHO))
	    return FULLUPDATE;
	snprintf(title, sizeof(title), "%s %s", "[�O�W����]", topic);
	snprintf(genbuf, sizeof(genbuf),
		 "%s\n\n%s%s\n", "�O�W����", "����D�D: ", topic);
	strcat(genbuf, "\n�����]: \n");
	break;
    case 3:
	do {
	    if (!getdata(7, 0, "�п�J�ݪO�^��W�١G", topic, IDLEN + 1, DOECHO))
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

	if (!getdata(8, 0, "�п�J�ݪO����W�١G", topic, 20, DOECHO))
	    return FULLUPDATE;
	strcat(genbuf, topic);
	strcat(genbuf, "\n�ݪO���O: ");
	if (!getdata(9, 0, "�п�J�ݪO���O�G", topic, 20, DOECHO))
	    return FULLUPDATE;
	strcat(genbuf, topic);
	strcat(genbuf, "\n�O�D�W��: ");
	getdata(10, 0, "�п�J�O�D�W��G", topic, IDLEN * 3 + 3, DOECHO);
	strcat(genbuf, topic);
	strcat(genbuf, "\n�ӽЭ�]: \n");
	break;
    case 4:
        generalnamecomplete("�п�J�ݪO�^��W�١G",
                            topic, IDLEN+1,
                            SHM->Bnumber,
                            completeboard_compar,
                            completeboard_permission,
                            completeboard_getname);
	snprintf(title, sizeof(title), "[�o���ªO] %s", topic);
	snprintf(genbuf, sizeof(genbuf),
		 "%s\n\n%s%s\n", "�o���ªO", "�^��W��: ", topic);
	strcat(genbuf, "\n�o����]: \n");
	break;
    case 5:
        generalnamecomplete("�п�J�ݪO�^��W�١G",
                            topic, IDLEN+1,
                            SHM->Bnumber,
                            completeboard_compar,
                            completeboard_permission,
                            completeboard_getname);
	snprintf(title, sizeof(title), "[�s�p�O�D] %s", topic);
	snprintf(genbuf, sizeof(genbuf), "%s\n\n%s%s\n%s%s", "�s�p�O�D", "�^��W��: ", topic, "�ӽ� ID : ", cuser.userid);
	strcat(genbuf, "\n�ӽЬF��: \n");
	break;
    case 6:
        generalnamecomplete("�п�J�ݪO�^��W�١G",
                            topic, IDLEN+1,
                            SHM->Bnumber,
                            completeboard_compar,
                            completeboard_permission,
                            completeboard_getname);
	snprintf(title, sizeof(title), "[�}�K�O�D] %s", topic);
	snprintf(genbuf, sizeof(genbuf),
		 "%s\n\n%s%s\n%s", "�}�K�O�D", "�^��W��: ",
		 topic, "�O�D ID : ");
        temp=getbnum(topic);
	do {
	    if (!getdata(7, 0, "�п�J�O�DID�G", topic, IDLEN + 1, DOECHO))
		return FULLUPDATE;
        }while (!userid_is_BM(topic, bcache[temp - 1].BM));
	strcat(genbuf, topic);
	strcat(genbuf, "\n�}�K��]: \n");
	break;
    case 7:
	if (!getdata(7, 0, "�п�J�p�դ��^��W�١G", topic, 30, DOECHO))
	    return FULLUPDATE;
	snprintf(title, sizeof(title), "[�s�p�p�ժ�] %s", topic);
	snprintf(genbuf, sizeof(genbuf),
		 "%s\n\n%s%s\n%s%s", "�s�p�p�ժ�", "�p�զW��: ",
		 topic, "�ӽ� ID : ", cuser.userid);
	strcat(genbuf, "\n�ӽЬF��: \n");
	break;
    case 8:
	if (!getdata(7, 0, "�п�J�p�դ��^��W�١G", topic, 30, DOECHO))
	    return FULLUPDATE;
	snprintf(title, sizeof(title), "[�}�K�p�ժ�] %s", topic);
	snprintf(genbuf, sizeof(genbuf), "%s\n\n%s%s\n%s",
		 "�}�K�p�ժ�", "�p�զW��: ", topic, "�p�ժ� ID : ");
	if (!getdata(8, 0, "�п�J�p�ժ�ID�G", topic, IDLEN + 1, DOECHO))
	    return FULLUPDATE;
	strcat(genbuf, topic);
	strcat(genbuf, "\n�}�K��]: \n");
	break;
    case 9:
	if (!getdata(7, 0, "�п�J�s�դ��^��W�١G", topic, 30, DOECHO))
	    return FULLUPDATE;
	snprintf(title, sizeof(title), "[�ӽзs�s��] %s", topic);
	snprintf(genbuf, sizeof(genbuf), "%s\n\n%s%s\n%s%s",
		 "�ӽиs��", "�s�զW��: ", topic, "�ӽ� ID : ", cuser.userid);
	strcat(genbuf, "\n�ӽЬF��: \n");
	break;
    default:
	return FULLUPDATE;
    }
    outs("�п�J²���άF��(�ܦh����)�A�n�M����g���M���|�֭��");
    for (temp = 11; temp < 16; temp++) {
	    if (!getdata(temp, 0, "�G", topic, 60, DOECHO))
		break;
	    strcat(genbuf, topic);
	    strcat(genbuf, "\n");
	}
    if (temp == 11)
	    return FULLUPDATE;
    strcat(genbuf, "�s�p�����ɶ�: ");
    now += 14 * 24 * 60 * 60;
    snprintf(topic, sizeof(topic), "(%ld)", now);
    strcat(genbuf, topic);
    strcat(genbuf, ctime(&now));
    now -= 14 * 24 * 60 * 60;
    strcat(genbuf, "����H��:0        �Ϲ�H��:0\n");
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
    votefile.filemode |= FILE_VOTE;
    setbdir(genbuf, currboard);
    if (append_record(genbuf, &votefile, sizeof(votefile)) != -1)
	setbtotal(currbid);
    do_voteboardreply(&votefile);
    return FULLUPDATE;
}
