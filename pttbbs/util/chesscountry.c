/* $Id$ */
/*-------------------------------------------------------*/
/* copied from util/cchess.c    ( NTU FPG BBS Ver 1.00 ) */
/*-------------------------------------------------------*/
/* target : �Ѱ�]�w�ɦ۰ʧ�s�{��                       */
/*-------------------------------------------------------*/

#define CHESSCOUNTRY /* This tool is built with CHESSCOUNTRY defined */
#include "bbs.h"

/* Number of minutes of files' mtime before now will be rescanned. */
// #define UPDATE_FREQUENCY  (30)

void
f_suck6(FILE* fp, char* fname)
{
    FILE *sfp;
    int  count = 0;

    if ((sfp = fopen(fname, "r")))
    {
	char inbuf[256];

	while (fgets(inbuf, sizeof(inbuf), sfp) && count < 6)
	{
	    fputs(inbuf, fp);
	    count++;
	}
	fclose(sfp);
    }
    while (count++ < 6)
	fputc('\n', fp);
} 

int
main(void)
{
    FILE  *fp, *ftmp;
    int   i = 0, num;
    // char  *currboard[3] = {"CCK-CHUHEN", "CCK-GENERAL", "CCK-FREE"};
    // char  *kingdom[3] = {"���~�Ӵ�", "�N�ӫ��p", "�p������"};
    char  file1[80], file2[80], str[256];
    time_t dtime;
    boardheader_t brd;
    int brdfd;

    setgid(BBSGID);
    setuid(BBSUID);
    chdir(BBSHOME);  

    attach_SHM();

    time(&dtime);

    if ((brdfd = open(BBSHOME "/" FN_BOARD, O_RDONLY)) == -1){
	perror("open " BBSHOME "/" FN_BOARD);
	return 0;
    }
    
    while(read(brdfd, &brd, sizeof(brd)) == sizeof(brd))
    {
	const char* photo_fname = 0;
	const char* chess_name = 0;
	char kingdom_name[256];
	int bid;
	// struct stat st;

	switch(brd.chesscountry){
	    case CHESSCODE_FIVE:
		photo_fname = "photo_fivechess";
		chess_name = "���l��";
		break;
	    case CHESSCODE_CCHESS:
		photo_fname = "photo_cchess";
		chess_name = "�H��";
		break;
	    default:
		continue;
	}
	bid = getbnum(brd.brdname);

	setapath(str, brd.brdname);
	sprintf(file1, "%s/chess_list", str);

	printf("apath = %s\n", str);

	/*
	if (stat(file1, &st) == 0 && st.st_mtime > (dtime - UPDATE_FREQUENCY * 60))
	    continue;
	*/

	sprintf(file2, "%s/chess_list.tmp", str);
	if ((ftmp = fopen(file2, "w")) == NULL)
	    continue;

	if ((fp = fopen(file1, "r")))
	{
	    char  *p;
	    char userid[IDLEN + 1], buf[256], name[11];
	    char date[11], other[IDLEN + 1];
	    int  namelen;

	    fgets(kingdom_name, 256, fp);
	    fputs(kingdom_name, ftmp);
	    kingdom_name[strlen(kingdom_name) - 1] = 0;
	    while (fgets(buf, sizeof(buf), fp))
	    {
		i = 0;
		strcpy(str, buf);
		p = strtok(buf, " ");
		if (*p != '#' && searchuser(p))
		{
		    strlcpy(userid, p, sizeof(userid));
		    i = 1;

		    if ((p = strtok(NULL, " ")))
			strlcpy(name, p, sizeof(name));
		    else
			i = 0;

		    if ((p = strtok(NULL, " ")))
			strlcpy(date, p, sizeof(date));
		    else
			i = 0;

		    if ((p = strtok(NULL, " ")))
			strlcpy(other, p, sizeof(other));
		    else
			i = 0;
		}
		if (!strcmp("���W", name))
		{
		    sethomefile(buf, userid, photo_fname);
		    unlink(buf);
		    continue;
		}
		if (i == 0)
		{
		    fprintf(ftmp, "%s", str);
		    continue;
		}
		fprintf(ftmp, "#%s", str);
		namelen = strlen(name);

		setapath(str, brd.brdname);
		sprintf(buf, "%s/chess_photo/.DIR", str);
		num = get_num_records(buf, sizeof(fileheader_t));
		for (i = 1; i <= num; i++)
		{
		    fileheader_t item;
		    if (get_record(buf, &item, sizeof item, i) != -1)
		    {
			FILE *fp1;
			if (!strncmp(item.title + 3, name, namelen) &&
				(item.title[namelen + 3] == '\0' ||
				 item.title[namelen + 3] == ' ')
				)
			{
			    sethomefile(buf, userid, photo_fname);
			    if ((fp1 = fopen(buf, "w")))
			    {
				sprintf(buf, "%s/chess_photo/%s", str, item.filename);
				f_suck6(fp1, buf);
				fprintf(fp1, "%d\n", bid);
				if (strcmp("�R��", name))
				    fprintf(fp1, "<���ݤ���> %s (%s)\n", kingdom_name, chess_name);
				else
				    fprintf(fp1, "<�R������> %s\n", kingdom_name);
				fprintf(fp1, "<�{�b����> %s\n", name);
				fprintf(fp1, "<�[�J���> %s\n", date);
				if (strcmp("�R��", name))
				{
				    int level;
				    fprintf(fp1, "<���굥��> ");
				    level = atoi(other);
				    for (i = 0; i < level; i++)
					fprintf(fp1, "%s", "��");
				}
				else
				{
				    other[strlen(other) - 1] = 0;
				    fprintf(fp1, "<�Q�֫R��> %s", other);
				}
				fprintf(fp1, "\n<�ۧڻ���> \n");
				fclose(fp1);
			    }
			}
		    }
		}
	    }
	    fclose(fp);
	    fclose(ftmp);
	    rename(file2, file1);
	}
    }
    return 0;
}
