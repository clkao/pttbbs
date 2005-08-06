/* $Id$ */
/* �o�O�ΨӱN�𪬤�����X�� perl module (�i�H�����O man/ �ϥ�) */
#include "bbs.h"
/* ���� hash �����e�p�U:

   $db{'tobid.BRDNAME'}  �� BRDNAME(�j�p�g�ݥ��T) �d bid
   $db{'parent.BID'}     �q BID �d parent �� bid
   $db{'tobrdname.BID'}  �q BID �d�^��ݪO�W��
   $db{'look.SBRDNAME'}  �q�������O�p�g�� SBRDNAME �d���T���ݪO�j�p�g
   $db{'BID.isboard'}    �� BID �O�ݪO(1)�θs��(0)
   $db{'BID.brdname'}    �q BID �d brdname
   $db{'BID.title'}      �d BID ������O�W
   $db{'BID.BM.0'} .. $db{'BID.BM.4'}
                         �ӪO�O�D ID
 */
int parent[MAX_BOARD];

static void
load_uidofgid(const int gid, const int type)
{
    boardheader_t  *bptr, *currbptr, *parent;
    int             bid, n, childcount = 0;
    currbptr = parent = &bcache[gid - 1];
    for (n = 0; n < numboards; ++n) {
	bid = SHM->bsorted[type][n]+1;
	if( bid<=0 || !(bptr = &bcache[bid-1])
		|| bptr->brdname[0] == '\0' )
	    continue;
	if (bptr->gid == gid) {
	    if (currbptr == parent)
		currbptr->firstchild[type] = bid;
	    else {
		currbptr->next[type] = bid;
		currbptr->parent = gid;
	    }
	    childcount++;
	    currbptr = bptr;
	}
    }
    parent->childcount = childcount;
    if (currbptr == parent) // no child
	currbptr->firstchild[type] = -1;
    else // the last child
	currbptr->next[type] = -1;
}

char *skipEscape(char *s)
{
    static  char    buf[TTLEN * 2 + 1];
    int     r, w;
    for( w = r = 0 ; s[r] != 0 ; ++r ){
	if( s[r] == '\'' || s[r] == '\\' )
	    buf[w++] = '\\';
	buf[w++] = s[r];
    }
    buf[w++] = 0;
    return buf;
}

void dumpdetail(void)
{
    int     i, k, bid;
    boardheader_t  *bptr;
    char    BM[IDLEN * 3 + 3], *p;
    char    smallbrdname[IDLEN + 1];
    for( i = 0 ; i < MAX_BOARD ; ++i ){
	bptr = &bcache[i];
	
	if( !bptr->brdname[0] ||
	    (bptr->brdattr & (BRD_HIDE | BRD_TOP)) ||
	    (bptr->level && !(bptr->brdattr & BRD_POSTMASK) &&
	     (bptr->level & 
	      ~(PERM_BASIC|PERM_CHAT|PERM_PAGE|PERM_POST|PERM_LOGINOK))) )
	    continue;

	for( k = 0 ; bptr->brdname[k] ; ++k )
	    smallbrdname[k] = (isupper(bptr->brdname[k]) ? 
			       tolower(bptr->brdname[k]) :
			       bptr->brdname[k]);
	smallbrdname[k] = 0;

	bid = bptr - bcache + 1;
	printf("$db{'tobid.%s'} = %d;\n", bptr->brdname, bid);
	printf("$db{'parent.%d'} = %d;\n", bid, parent[bid]);
	printf("$db{'tobrdname.%d'} = '%s';\n", bid, bptr->brdname);
	printf("$db{'look.%s'} = '%s';\n", smallbrdname, bptr->brdname);
	printf("$db{'%d.isboard'} = %d;\n", bid,
	       (bptr->brdattr & BRD_GROUPBOARD) ? 0 : 1);
	printf("$db{'%d.brdname'} = '%s';\n", bid, bptr->brdname);
	printf("$db{'%d.title'} = '%s';\n", bid, skipEscape(&bptr->title[7]));
	strlcpy(BM, bptr->BM, sizeof(BM));
	for( p = BM ; *p != 0 ; ++p )
	    if( !isalpha(*p) && !isdigit(*p) )
		*p = ' ';
	for( k = 0, p = strtok(BM, " ") ; p != NULL ; ++k, p = strtok(NULL, " ") )
	    printf("$db{'%d.BM.%d'} = '%s';\n", bid, k, p);
    }
}

void dumpclass(int gid)
{
    boardheader_t  *bptr;
    int bid;
    bptr = getbcache(gid);
    if (bptr->firstchild[0] == 0 || bptr->childcount <= 0)
	load_uidofgid(gid, 0);
    printf("$db{'class.%d'} = $serializer->serialize([", gid);
    for( bid = bptr->firstchild[0] ; bid > 0 ; bid = bptr->next[0] ) {
	bptr = getbcache(bid);
	if( (bptr->brdattr & (BRD_HIDE | BRD_TOP)) ||
	    (bptr->level && !(bptr->brdattr & BRD_POSTMASK) &&
	     (bptr->level & 
	      ~(PERM_BASIC|PERM_CHAT|PERM_PAGE|PERM_POST|PERM_LOGINOK))) )
	    continue;

	printf("%5d,\t", bid);
	parent[bid] = gid;
    }
    printf("]);\n");

    bptr = getbcache(gid);
    for( bid = bptr->firstchild[0] ; bid > 0 ; bid = bptr->next[0] ) {
	bptr = getbcache(bid);
	if( (bptr->brdattr & (BRD_HIDE | BRD_TOP)) ||
	    (bptr->level && !(bptr->brdattr & BRD_POSTMASK) &&
	     (bptr->level & 
	      ~(PERM_BASIC|PERM_CHAT|PERM_PAGE|PERM_POST|PERM_LOGINOK))) )
	    continue;
	if( bptr->brdattr & BRD_GROUPBOARD )
	    dumpclass(bid);
    }
}

void dumpallbrdname(void)
{
    int     i;
    boardheader_t  *bptr;
    FILE    *fp;
    
    if( !(fp = fopen("boardlist.all", "wt")) )
	return;

    for( i = 0 ; i < MAX_BOARD ; ++i ){
	bptr = &bcache[i];
	
	if( !bptr->brdname[0] ||
	    (bptr->brdattr & (BRD_HIDE | BRD_TOP | BRD_GROUPBOARD)) ||
	    (bptr->level && !(bptr->brdattr & BRD_POSTMASK) &&
	     (bptr->level & 
	      ~(PERM_BASIC|PERM_CHAT|PERM_PAGE|PERM_POST|PERM_LOGINOK))) )
	    continue;
	fprintf(fp, "%s\n", bptr->brdname);
    }
    fclose(fp);
}

int main(int argc, char **argv)
{
    attach_SHM();

    printf("#!/usr/bin/perl\n"
           "# this is auto-generated perl module from boardlist.c\n"
           "# please do NOT modify this directly!\n"
           "# usage: make boardlist; ./boardlist | perl\n"
           "# Id of boardlist.c: $Id$\n"
           "use DB_File;\n"
	   "use Data::Serializer;\n"
	   "\n"
	   "unlink 'boardlist.db', 'boardlist.list';\n"
	   "$serializer = Data::Serializer->new(serializer => 'Storable', digester => 'MD5',compress => 0,);\n"
           "tie %%db, 'DB_File', 'boardlist.db', (O_RDWR | O_CREAT), 0666, $DB_HASH;\n"
	   );
    dumpclass(1);
    dumpdetail();
    dumpallbrdname();
    printf("untie %%db;\n");
    return 0;
}
