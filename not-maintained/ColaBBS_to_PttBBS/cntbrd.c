/* $Id$ */
#include "bbs.h"
/* usage: ./cntbrd < ColaBBS/.BOARDS > ~/.BRD */

typedef struct {
    char    brdname[13];
    char    pad[148];
    char    title[49];
    char    pad2[46];
} fbrd_t;

int main(int argc, char **argv)
{
    fbrd_t        fbh;
    boardheader_t bh;
#if 0
    int     i;
    read(0, &fbh, sizeof(fbh));
    for( i = 0 ; i < sizeof(fbh) ; ++i )
	printf("%03d: %d(%c)\n", i, fbh.pad[i], fbh.pad[i]);
    return 0;
#endif

    memset(&bh, 0, sizeof(bh));
    strcpy(bh.brdname, "SYSOP");
    strcpy(bh.title, "�T�� �������n!");
    bh.brdattr = BRD_POSTMASK | BRD_NOTRAN | BRD_NOZAP;
    bh.level = 0;
    bh.gid = 2;
    write(1, &bh, sizeof(bh));

    strcpy(bh.brdname, "1...........");
    strcpy(bh.title, ".... �U�����F��  �m�����M�I,�D�H�i�ġn");
    bh.brdattr = BRD_GROUPBOARD;
    bh.level = PERM_SYSOP;
    bh.gid = 1;
    write(1, &bh, sizeof(bh));
	
    strcpy(bh.brdname, "junk");
    strcpy(bh.title, "�o�q �����C���K���U��");
    bh.brdattr = BRD_NOTRAN;
    bh.level = PERM_SYSOP;
    bh.gid = 2;
    write(1, &bh, sizeof(bh));
    
    strcpy(bh.brdname, "Security");
    strcpy(bh.title, "�o�q �������t�Φw��");
    bh.brdattr = BRD_NOTRAN;
    bh.level = PERM_SYSOP;
    bh.gid = 2;
    write(1, &bh, sizeof(bh));
	
    strcpy(bh.brdname, "2...........");
    strcpy(bh.title, ".... �U�����s��     ���i  ����  ���I");
    bh.brdattr = BRD_GROUPBOARD;
    bh.level = 0;
    bh.gid = 1;
    write(1, &bh, sizeof(bh));
	
    strcpy(bh.brdname, "ALLPOST");
    strcpy(bh.title, "�T�� ����O��LOCAL�s�峹");
    bh.brdattr = BRD_POSTMASK | BRD_NOTRAN;
    bh.level = PERM_SYSOP;
    bh.gid = 5;
    write(1, &bh, sizeof(bh));
	
    strcpy(bh.brdname, "deleted");
    strcpy(bh.title, "�T�� ���귽�^����");
    bh.brdattr = BRD_NOTRAN;
    bh.level = PERM_BM;
    bh.gid = 5;
    write(1, &bh, sizeof(bh));
	
    strcpy(bh.brdname, "Note");
    strcpy(bh.title, "�T�� ���ʺA�ݪO�κq����Z");
    bh.brdattr = BRD_NOTRAN;
    bh.level = 0;
    bh.gid = 5;
    write(1, &bh, sizeof(bh));
	
    strcpy(bh.brdname, "Record");
    strcpy(bh.title, "�T�� ���ڭ̪����G");
    bh.brdattr = BRD_NOTRAN | BRD_POSTMASK;
    bh.level = 0;
    bh.gid = 5;
    write(1, &bh, sizeof(bh));
	
    strcpy(bh.brdname, "WhoAmI");
    strcpy(bh.title, "�T�� �������A�q�q�ڬO�֡I");
    bh.brdattr = BRD_NOTRAN;
    bh.level = 0;
    bh.gid = 5;
    write(1, &bh, sizeof(bh));
	
    strcpy(bh.brdname, "EditExp");
    strcpy(bh.title, "�T�� ���d�����F��Z��");
    bh.brdattr = BRD_NOTRAN;
    bh.level = 0;
    bh.gid = 5;
    write(1, &bh, sizeof(bh));

    while( read(0, &fbh, sizeof(fbh)) == sizeof(fbh) ){
	if( !fbh.brdname[0] || strcasecmp(fbh.brdname, "sysop") == 0 )
	    continue;
	memset(&bh, 0, sizeof(bh));
	strlcpy(bh.brdname, fbh.brdname, sizeof(bh.brdname));
	strlcpy(bh.title, "�ഫ ��", sizeof(bh.title));
	strlcpy(&bh.title[7], fbh.title, sizeof(bh.title));
	bh.brdattr = BRD_NOTRAN;
	bh.level = 0;
	bh.gid = 2;
	write(1, &bh, sizeof(bh));
    }
    return 0;
}
