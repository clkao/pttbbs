#include "bbs.h"
/* please run cnthomedir.pl first!
   cd home/; apply "mv %1/mail/* %1/" * / *
             apply "mv %1/mail/.DIR %1/.DIR.colabbs" * / *
             apply "rmdir %1/mail" * / *
	     apply "cnthomeDIR < %1/.DIR.colabbs > %1/.DIR" * / *
*/

typedef struct {
    char    filename[FNLEN];         /* M.9876543210.A */
    char    pad0[80-FNLEN];
    char    owner[IDLEN + 2];        /* uid[.] */
    char    pad1[66];
    char    title[65];
    char    pad2[31];
} ffh_t;
int main(int argc, char **argv)
{
    ffh_t   ffh;
    fileheader_t fh;

    time_t  t;
    struct  tm      *tm;
    int     i;

    memset(&fh, 0, sizeof(fh));
    while( read(0, &ffh, sizeof(ffh)) == sizeof(ffh) ){
	if( !ffh.filename[0] )
	    continue;
	strlcpy(fh.filename, ffh.filename, sizeof(fh.filename));
	strlcpy(fh.title,    ffh.title,    sizeof(fh.title));
	strlcpy(fh.owner,    ffh.owner,    sizeof(fh.owner));
	for( i = 0 ; i < sizeof(fh.owner) ; ++i )
	    if( fh.owner[i] == ' ' ){
		fh.owner[i] = 0;
		break;
	    }

	t = atoi(&fh.filename[2]);
	tm = localtime(&t);
	sprintf(fh.date, "%2d/%02d", tm->tm_mon + 1, tm->tm_mday);
	write(1, &fh, sizeof(fh));
    }
    return 0;
}
