/* $Id$ */
#include "bbs.h"
/*
    usage: apply "mv %1/.DIR %1/.DIR.colabbs; 
                  ./cntDIR < %1/.DIR.colabbs > %1/.DIR;
		  rm %1/.DIR.colabbs"
           ~/boards/ * / *
                    ^ ^ ^ <- please ignore the spaces
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

    memset(&fh, 0, sizeof(fh));
    while( read(0, &ffh, sizeof(ffh)) == sizeof(ffh) ){
	if( !ffh.filename[0] )
	    continue;
	strlcpy(fh.filename, ffh.filename, sizeof(fh.filename));
	strlcpy(fh.title,    ffh.title,    sizeof(fh.title));
	strlcpy(fh.owner,    ffh.owner,    sizeof(fh.owner));

	t = atoi(&fh.filename[2]);
	tm = localtime(&t);
	sprintf(fh.date, "%2d/%02d", tm->tm_mon + 1, tm->tm_mday);
	write(1, &fh, sizeof(fh));
    }
    return 0;
}
