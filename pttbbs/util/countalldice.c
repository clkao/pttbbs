/* $Id$ */

/**********************************************/
/*�o�ӵ{���O�Ψӭp����l�ȱo����ߪ������{�� */
/*�Ϊk�N�O������ countalldice �N�i�H�w��Ҧ��H */
/*�ӭp��L�`�@�ȤF�h�� �ߤF�h��............... */
/*�@��:Heat ��1997/10/2                       */
/**********************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "config.h"

#define DICE_WIN  BBSHOME "/etc/windice.log"
#define DICE_LOST BBSHOME "/etc/lostdice.log"

int total = 0;

typedef struct dice
{
    char id[14];
    int win;
    int lost;
}
dice;

dice table[1024];

int find(char *name)
{
    int i = 0;
    if (total == 0)
    {
	total++;
	return 0;
    }
    for (i = 0; i < total; i++)
	if (!strcmp(name, table[i].id))
	    return i;
    memset(&table[total++], 0, sizeof(dice));
    return total - 1;
}

int main() {
    int index, win = 0, lost = 0;
    FILE *fpwin, *fplost;
    char buf[256], *ptr, buf0[256], *name = (char *) malloc(15), *mon = (char *) malloc(5);

    fpwin = fopen(DICE_WIN, "r");
    fplost = fopen(DICE_LOST, "r");

    if (!fpwin || !fplost)
	perror("error open file");

    while (fgets(buf, 255, fpwin))
    {
	strcpy(buf0, buf);
	name = strtok(buf, " ");
	mon = strstr(buf0, "�b��:");
	if ((ptr = strchr(mon, '\n')))
	    *ptr = 0;
	index = find(name);
	strcpy(table[index].id, name);
	table[index].win += atoi(mon + 5);
    }
    fclose(fpwin);

    while (fgets(buf, 255, fplost))
    {
	strcpy(buf0, buf);
	name = strtok(buf, " ");
	mon = strstr(buf0, "��F ");
	if ((ptr = strchr(mon, '\n')))
	    *ptr = 0;
	if ((index = find(name)) == total - 1)
	    strcpy(table[index].id, name);
	table[index].lost += atoi(mon + 5);
    }

    for (index = 0; index < total; index++)
    {
	printf("%-15s Ĺ�F %-8d �����A �鱼 %-8d ����\n", table[index].id
	       ,table[index].win, table[index].lost);
	win += table[index].win;
	lost += table[index].lost;
    }
    index = win + lost;
    printf("\n�H��: %d\n�`Ĺ��=%d �`���=%d �`���B:%d\n", total, win, lost, index);
    printf("Ĺ�����:%f �骺���:%f\n", (float) win / index, (float) lost / index);
    printf("\n�Ƶ��G��Ĺ�O�H�ϥΪ̪��[�I�Ӭ�\n");
    fclose(fplost);
    return 0;
}
