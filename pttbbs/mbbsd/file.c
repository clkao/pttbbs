/* $Id$ */

#include "bbs.h"

/**
 * file.c �O�w��H"��"����쪺�ɮשҩw�q���@�� operation�C
 */

/**
 * �Ǧ^ file �ɪ����
 * @param file
 */
int file_count_line(const char *file)
{
    FILE           *fp;
    int             count = 0;
    char            buf[200];

    if ((fp = fopen(file, "r"))) {
	while (fgets(buf, sizeof(buf), fp)) {
	    if (strchr(buf, '\n') == NULL)
		continue;
	    count++;
	}
	fclose(fp);
    }
    return count;
}

/**
 * �N string append ���ɮ� file ���
 * @param file �n�Q append ����
 * @param string
 * @return ���\�Ǧ^ 0�A���ѶǦ^ -1�C
 */
int file_append_line(const char *file, const char *string)
{
    FILE *fp;
    if ((fp = fopen(file, "a")) == NULL)
	return -1;
    flock(fileno(fp), LOCK_EX);
    fputs(string, fp);
    flock(fileno(fp), LOCK_UN);
    fclose(fp);
    return 0;
}

#ifndef _BBS_UTIL_C_
/**
 * �q�ɮ� file ���R�� prefix �� string ���C�@��C(�p�� race)
 * @param file
 * @param string
 * @param case_sensitive �r����O�_ case sensitive
 */
int file_delete_line(const char *file, const char *string, int  case_sensitive)
{
    FILE           *fp, *nfp = NULL;
    char            fnew[80];
    char            genbuf[STRLEN + 1];

    sprintf(fnew, "%s.%3.3X", file, random() & 0xFFF);
    if ((fp = fopen(file, "r")) && (nfp = fopen(fnew, "w"))) {
	int             length = strlen(string);

	while (fgets(genbuf, sizeof(genbuf), fp))
	    if ((genbuf[0] > ' ')) {
		if (((case_sensitive && strncmp(genbuf, string, length)) ||
		    (!case_sensitive && strncasecmp(genbuf, string, length))))
    		    fputs(genbuf, nfp);
	    }
	Rename(fnew, file);
    }
    if(fp)
	fclose(fp);
    if(nfp)
	fclose(nfp);
    return 0;
}
#endif

/**
 * �Ǧ^�ɮ� file ���O�_�� string �o�Ӧr��C
 */
int file_exist_record(const char *file, const char *string)
{
    FILE           *fp;
    char            buf[STRLEN], *ptr;

    if ((fp = fopen(file, "r")) == NULL)
	return 0;

    while (fgets(buf, STRLEN, fp)) {
	if ((ptr = strtok(buf, str_space)) && !strcasecmp(ptr, string)) {
	    fclose(fp);
	    return 1;
	}
    }
    fclose(fp);
    return 0;
}

/**
 * ��C�@�� record �� func �o��ơC
 * @param file
 * @param func �B�z�C�� record �� handler�A���@ function pointer�C
 *        �Ĥ@�ӰѼƬO�ɮפ����@��A�ĤG�ӰѼƬ� info�C
 * @param info �@���B�~���ѼơC
 */
int file_foreach_entry(const char *file, int (*func)(char *, int), int info)
{
    char line[80];
    FILE *fp;

    if ((fp = fopen(file, "r")) == NULL)
	return -1;

    while (fgets(line, sizeof(line), fp)) {
	(*func)(line, info);
    }

    fclose(fp);
    return 0;
}
