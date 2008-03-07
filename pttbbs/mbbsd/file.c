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
 * �N string append ���ɮ� file ��� (���[����)
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

/**
 * �N "$key\n" append ���ɮ� file ���
 * @param file �n�Q append ����
 * @param key �S�����檺�r��
 * @return ���\�Ǧ^ 0�A���ѶǦ^ -1�C
 */
int file_append_record(const char *file, const char *key)
{
    FILE *fp;
    if (!key || !*key) return -1;
    if ((fp = fopen(file, "a")) == NULL)
	return -1;
    flock(fileno(fp), LOCK_EX);
    fputs(key, fp);
    fputs("\n", fp);
    flock(fileno(fp), LOCK_UN);
    fclose(fp);
    return 0;
}

/**
 * �Ǧ^�ɮ� file �� key �Ҧb���
 */
int file_find_record(const char *file, const char *key)
{
    FILE           *fp;
    char            buf[STRLEN], *ptr;
    int i = 0;

    if ((fp = fopen(file, "r")) == NULL)
	return 0;

    while (fgets(buf, STRLEN, fp)) {
	char *strtok_pos;
	i++;
	if ((ptr = strtok_r(buf, str_space, &strtok_pos)) && !strcasecmp(ptr, key)) {
	    fclose(fp);
	    return i;
	}
    }
    fclose(fp);
    return 0;
}

/**
 * �Ǧ^�ɮ� file ���O�_�� key
 */
int file_exist_record(const char *file, const char *key)
{
    return file_find_record(file, key) > 0 ? 1 : 0;
}

/**
 * �R���ɮ� file ���H string �}�Y����
 * @param file �n�B�z���ɮ�
 * @param string �M�䪺 key name
 * @param case_sensitive �O�_�n�B�z�j�p�g
 * @return ���\�Ǧ^ 0�A���ѶǦ^ -1�C
 */
int
file_delete_record(const char *file, const char *string, int case_sensitive)
{
    // TODO nfp �� tmpfile() ����n�H ���L Rename �|�ܺC...
    FILE *fp = NULL, *nfp = NULL;
    char fnew[PATHLEN];
    char buf[STRLEN + 1];
    int ret = -1, i = 0;
    const size_t toklen = strlen(string);

    if (!toklen)
	return 0;

    do {
	snprintf(fnew, sizeof(fnew), "%s.%3.3X", file, (unsigned int)(random() & 0xFFF));
	if (access(fnew, 0) != 0)
	    break;
    } while (i++ < 10); // max tries = 10

    if (access(fnew, 0) == 0) return -1;    // cannot create temp file.

    i = 0;
    if ((fp = fopen(file, "r")) && (nfp = fopen(fnew, "w"))) {
	while (fgets(buf, sizeof(buf), fp))
	{
	    size_t klen = strcspn(buf, str_space);
	    if (toklen == klen)
	    {
		if (((case_sensitive && strncmp(buf, string, toklen) == 0) ||
		    (!case_sensitive && strncasecmp(buf, string, toklen) == 0)))
		{
		    // found line. skip it.
		    i++;
		    continue;
		}
	    }
	    // other wise, keep the line.
	    fputs(buf, nfp);
	}
	fclose(nfp); nfp = NULL;
	if (i > 0)
	{
	    if(Rename(fnew, file) < 0)
		ret = -1;
	    else
		ret = 0;
	} else {
	    unlink(fnew);
	    ret = 0;
	}
    }
    if(fp)
	fclose(fp);
    if(nfp)
	fclose(nfp);
    return ret;
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
