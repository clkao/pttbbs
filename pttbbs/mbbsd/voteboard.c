/* $Id$ */
#include "bbs.h"

#define VOTEBOARD "NewBoard"
void
do_voteboardreply(const fileheader_t * fhdr)
{
    char            genbuf[256];
    char            reason[36]="";
    char            fpath[80];
    char            oldfpath[80];
    char            opnion[10];
    char           *ptr;
    FILE           *fo,*fi;
    fileheader_t    votefile;
    int             yes=0, no=0, len;
    int             fd;
    unsigned long   endtime=0;


    clear();
    if (!CheckPostPerm()||HasUserPerm(PERM_NOCITIZEN)) {
	move(5, 10);
	vmsg("�藍�_�A�z�ثe�L�k�b���o��峹�I");
	return;
    }
    if ( cuser.numlogins < ((unsigned int)(fhdr->multi.vote_limits.logins) * 10) ||
	    cuser.numposts < ((unsigned int)(fhdr->multi.vote_limits.posts) * 10) ) {
	move(5, 10);
	vmsg("�A���W����/�峹�Ƥ�����I");
	return;
    }
    setbpath(fpath, currboard);
    stampfile(fpath, &votefile);

    setbpath(oldfpath, currboard);

    strcat(oldfpath, "/");
    strcat(oldfpath, fhdr->filename);

    fi = fopen(oldfpath, "r");
    assert(fi);

    while (fgets(genbuf, sizeof(genbuf), fi)) {

        if (yes>=0)
           {
            if (!strncmp(genbuf, "----------",10))
               {yes=-1; continue;}
            else 
                yes++;
           }
        if (yes>3) outs(genbuf);

	if (!strncmp(genbuf, "�s�p�����ɶ�", 12)) {
	    ptr = strchr(genbuf, '(');
	    assert(ptr);
	    sscanf(ptr + 1, "%lu", &endtime);
	    if (endtime < (unsigned long)now) {
		vmsg("�s�p�ɶ��w�L");
		fclose(fi);
		return;
	    }
	}
        if(yes>=0) continue; 

        strtok(genbuf+4," \n");
	if (!strncmp(genbuf + 4, cuser.userid, IDLEN)) {
	    move(5, 10);
	    outs("�z�w�g�s�p�L���g�F");
	    getdata(17, 0, "�n�ק�z���e���s�p�ܡH(Y/N) [N]", opnion, 3, LCECHO);
	    if (opnion[0] != 'y') {
		fclose(fi);
		return;
	    }
	    strlcpy(reason, genbuf + 19, 34);
            break;
	}
    }
    fclose(fi);
    do {
	if (!getdata(19, 0, "�аݱz (Y)��� (N)�Ϲ� �o��ĳ�D�G", opnion, 3, LCECHO)) {
	    return;
	}
    } while (opnion[0] != 'y' && opnion[0] != 'n');
    sprintf(genbuf, "�аݱz�P�o��ĳ�D�����Y��%s�z�Ѭ���G",
	    opnion[0] == 'y' ? "���" : "�Ϲ�");
    if (!getdata_buf(20, 0, genbuf, reason, 35, DOECHO)) {
	return;
    }
    if ((fd = open(oldfpath, O_RDONLY)) == -1)
	return;
    if(flock(fd, LOCK_EX)==-1 )
       {close(fd); return;}
    if(!(fi = fopen(oldfpath, "r")))
       {flock(fd, LOCK_UN); close(fd); return;}
     
    if(!(fo = fopen(fpath, "w")))
       {
        flock(fd, LOCK_UN);
        close(fd);
        fclose(fi);
	return;
       }

    while (fgets(genbuf, sizeof(genbuf), fi)) {
        if (!strncmp("----------", genbuf, 10))
	    break;
	fputs(genbuf, fo);
    }
    if (!endtime) {
	now += 14 * 24 * 60 * 60;
	fprintf(fo, "�s�p�����ɶ�: (%d)%s\n", now, ctime4(&now));
	now -= 14 * 24 * 60 * 60;
    }
    fputs(genbuf, fo);
    len = strlen(cuser.userid); 
    for(yes=0; fgets(genbuf, sizeof(genbuf), fi);) {
	if (!strncmp("----------", genbuf, 10))
	    break;
	if (strlen(genbuf)<30 || (genbuf[4+len]==' ' && !strncmp(genbuf + 4, cuser.userid, len)))
            continue;
	fprintf(fo, "%3d.%s", ++yes, genbuf + 4);
      }
    if (opnion[0] == 'y')
	fprintf(fo, "%3d.%-15s%-34s �ӷ�:%s\n", ++yes, cuser.userid, reason, cuser.lasthost);
    fputs(genbuf, fo);

    for(no=0; fgets(genbuf, sizeof(genbuf), fi);) {
	if (!strncmp("----------", genbuf, 10))
	    break;
	if (strlen(genbuf)<30 || (genbuf[4+len]==' ' && !strncmp(genbuf + 4, cuser.userid, len)))
            continue;
	fprintf(fo, "%3d.%s", ++no, genbuf + 4);
    }
    if (opnion[0] == 'n')
	fprintf(fo, "%3d.%-15s%-34s �ӷ�:%s\n", ++no, cuser.userid, reason, cuser.lasthost);
    fprintf(fo, "----------�`�p----------\n");
    fprintf(fo, "����H��:%-9d�Ϲ�H��:%-9d\n", yes, no);
    fprintf(fo, "\n--\n�� �o�H�� :" BBSNAME "(" MYHOSTNAME
                ") \n�� From: �s�p�峹\n");

    flock(fd, LOCK_UN);
    close(fd);
    fclose(fi);
    fclose(fo);
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
    if (!CheckPostPerm()) {
	move(5, 10);
	vmsg("�藍�_�A�z�ثe�L�k�b���o��峹�I");
	return FULLUPDATE;
    }
    if ( cuser.firstlogin > (now - (time4_t)bcache[currbid - 1].vote_limit_regtime * 2592000) ||
	    cuser.numlogins < ((unsigned int)(bcache[currbid - 1].vote_limit_logins) * 10) ||
	    cuser.numposts < ((unsigned int)(bcache[currbid - 1].vote_limit_posts) * 10) ) {
	move(5, 10);
	vmsg("�A������`��I");
	return FULLUPDATE;
    }
    move(0, 0);
    clrtobot();
    outs("�z���b�ϥ� PTT ���s�p�t��\n");
    outs("���s�p�t�αN�߰ݱz�@�ǰ��D�A�Фp�ߦ^���~��}�l�s�p\n");
    outs("���N���X�s�p�ת̡A�N�Q�C�J�����w��ϥΪ̳�\n");
    move(4, 0);
    clrtobot();
    outs("(1)���ʳs�p (2)�O�W���� ");
    if(type==0)
      outs("(3)�ӽзs�O (4)�o���ªO (5)�s�p�O�D \n(6)�}�K�O�D (7)�s�p�p�ժ� (8)�}�K�p�ժ� (9)�ӽзs�s��\n");

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
        move(1,0); clrtobot();
        generalnamecomplete("�п�J�ݪO�^��W�١G",
                            topic, IDLEN+1,
                            SHM->Bnumber,
                            &completeboard_compar,
                            &completeboard_permission,
                            &completeboard_getname);
	snprintf(title, sizeof(title), "[�o���ªO] %s", topic);
	snprintf(genbuf, sizeof(genbuf),
		 "%s\n\n%s%s\n", "�o���ªO", "�^��W��: ", topic);
	strcat(genbuf, "\n�o����]: \n");
	break;
    case 5:
        move(1,0); clrtobot();
        generalnamecomplete("�п�J�ݪO�^��W�١G",
                            topic, IDLEN+1,
                            SHM->Bnumber,
                            &completeboard_compar,
                            &completeboard_permission,
                            &completeboard_getname);
	snprintf(title, sizeof(title), "[�s�p�O�D] %s", topic);
	snprintf(genbuf, sizeof(genbuf), "%s\n\n%s%s\n%s%s", "�s�p�O�D", "�^��W��: ", topic, "�ӽ� ID : ", cuser.userid);
	strcat(genbuf, "\n�ӽЬF��: \n");
	break;
    case 6:
        move(1,0); clrtobot();
        generalnamecomplete("�п�J�ݪO�^��W�١G",
                            topic, IDLEN+1,
                            SHM->Bnumber,
                            &completeboard_compar,
                            &completeboard_permission,
                            &completeboard_getname);
	snprintf(title, sizeof(title), "[�}�K�O�D] %s", topic);
	snprintf(genbuf, sizeof(genbuf),
		 "%s\n\n%s%s\n%s", "�}�K�O�D", "�^��W��: ",
		 topic, "�O�D ID : ");
        temp=getbnum(topic);
	assert(0<=temp-1 && temp-1<MAX_BOARD);
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
    outs("�п�J²���άF��(�ܦh����)�A�n�M����g");
    for (temp = 12; temp < 17; temp++) {
	    if (!getdata(temp, 0, "�G", topic, 60, DOECHO))
		break;
	    strcat(genbuf, topic);
	    strcat(genbuf, "\n");
	}
    if (temp == 11)
	    return FULLUPDATE;
    strcat(genbuf, "�s�p�����ɶ�: ");
    now += 14 * 24 * 60 * 60;
    snprintf(topic, sizeof(topic), "(%d)", now);
    strcat(genbuf, topic);
    strcat(genbuf, ctime4(&now));
    strcat(genbuf, "\n");
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
    fprintf(fp, "%s%s %s%s\n%s%s\n%s%s\n", "�@��: ", cuser.userid,
	    "�ݪO: ", currboard,
	    "���D: ", title,
	    "�ɶ�: ", ctime4(&now));
    fprintf(fp, "%s\n", genbuf);
    fclose(fp);
    strlcpy(votefile.owner, cuser.userid, sizeof(votefile.owner));
    strlcpy(votefile.title, title, sizeof(votefile.title));
    votefile.filemode |= FILE_VOTE;
    /* use lower 16 bits of 'money' to store limits */
    /* lower 8 bits are posts, higher 8 bits are logins */
    votefile.multi.vote_limits.regtime = bcache[currbid - 1].vote_limit_regtime;
    votefile.multi.vote_limits.logins = bcache[currbid - 1].vote_limit_logins;
    votefile.multi.vote_limits.posts = bcache[currbid - 1].vote_limit_posts;
    setbdir(genbuf, currboard);
    if (append_record(genbuf, &votefile, sizeof(votefile)) != -1)
	setbtotal(currbid);
    do_voteboardreply(&votefile);
    return FULLUPDATE;
}
