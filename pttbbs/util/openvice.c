/* $Id: openvice.c,v 1.3 2003/07/20 00:55:34 in2 Exp $ */
/* 發票開獎小程式 */

#include "bbs.h"

#define VICE_SHOW  BBSHOME "/etc/vice.show1"
#define VICE_BINGO BBSHOME "/etc/vice.bingo"
#define VICE_NEW   "vice.new"
#define VICE_DATA  "vice.data"
#define MAX_BINGO  99999999

int main(int argc, char **argv)
{
    char *TABLE[5] = {"一", "二", "三", "四", "五"};
    int i = 0, bingo, base = 0;


    FILE *fp = fopen(VICE_SHOW, "w"), *fb = fopen(VICE_BINGO, "w");

    // XXX: resolve_utmp();
    attach_SHM();

    srand(SHM->number);

    if (!fp || !fb )
	perror("error open file");


    bingo = rand() % MAX_BINGO;
    fprintf(fp, "%1c[1;33m統一發票中獎號碼[m\n", ' ');
    fprintf(fp, "%1c[1;37m================[m\n", ' ');
    fprintf(fp, "%1c[1;31m特別獎[m: [1;31m%08d[m\n\n", ' ', bingo);
    fprintf(fb, "%d\n", bingo);

    while (i < 5)
    {
	bingo = (base + rand()) % MAX_BINGO;
	fprintf(fp, "%1c[1;36m第%s獎[m: [1;37m%08d[m\n", ' ', TABLE[i], bingo);
	fprintf(fb, "%08d\n", bingo);
	i++;
    }
    fclose(fp);
    fclose(fb);
    return 0;
}
