/* $Id: boardlist.c,v 1.4 2003/07/21 09:27:16 in2 Exp $ */
/* �o�O�ΨӱN�𪬤�����X�� perl module (�i�H�����O man/ �ϥ�) */
#include "bbs.h"

static void
load_uidofgid(const int gid, const int type)
{
    boardheader_t  *bptr, *currbptr;
    int             n, childcount = 0;
    currbptr = &bcache[gid - 1];
    for (n = 0; n < numboards; ++n) {
	bptr = SHM->bsorted[type][n];
	if (bptr->brdname[0] == '\0')
	    continue;
	if (bptr->gid == gid) {
	    if (currbptr == &bcache[gid - 1])
		currbptr->firstchild[type] = bptr;
	    else {
		currbptr->next[type] = bptr;
		currbptr->parent = &bcache[gid - 1];
	    }
	    childcount++;
	    currbptr = bptr;
	}
    }
    bcache[gid - 1].childcount = childcount;
    if (currbptr == &bcache[gid - 1])
	currbptr->firstchild[type] = (boardheader_t *) ~ 0;
    else
	currbptr->next[type] = (boardheader_t *) ~ 0;
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

void dumpclass(int bid)
{
    boardheader_t  *bptr;
    char    BM[IDLEN * 3 + 3], *p;

    bptr = &bcache[bid];
    if (bptr->firstchild[0] == NULL || bptr->childcount <= 0)
	load_uidofgid(bid + 1, 0); /* �]���o�� bid�q 0�}�l, �ҥH�A +1 �^�� */
    printf("    %5d => [\n", bid);
    for (bptr = bptr->firstchild[0]; bptr != (boardheader_t *) ~ 0;
	 bptr = bptr->next[0]) {
	if( (bptr->brdattr & (BRD_HIDE | BRD_TOP)) ||
	    (bptr->level && !(bptr->brdattr & BRD_POSTMASK) &&
	     (bptr->level & 
	      ~(PERM_BASIC|PERM_CHAT|PERM_PAGE|PERM_POST|PERM_LOGINOK))) )
	    continue;

	printf("        [%5d, '%s', '%s', [",
	       (bptr->brdattr & BRD_GROUPBOARD) ? bptr - bcache : -1,
	       bptr->brdname, skipEscape(&bptr->title[7]));
	strlcpy(BM, bptr->BM, sizeof(BM));
	for( p = BM ; *p != 0 ; ++p )
	    if( !isalpha(*p) && !isdigit(*p) )
		*p = ' ';
	for( p = strtok(BM, " ") ; p != NULL ; p = strtok(NULL, " ") ){
	    printf("'%s',", p);
	}
	printf("]],\n");
    }
    printf("     ],\n");

    bptr = &bcache[bid];
    for (bptr = bptr->firstchild[0]; bptr != (boardheader_t *) ~ 0;
	 bptr = bptr->next[0]) {
	if( (bptr->brdattr & (BRD_HIDE | BRD_TOP)) ||
	    (bptr->level && !(bptr->brdattr & BRD_POSTMASK) &&
	     (bptr->level & 
	      ~(PERM_BASIC|PERM_CHAT|PERM_PAGE|PERM_POST|PERM_LOGINOK))) )
	    continue;

	printf("     '%d.title' => '%s',\n",
	       bptr - bcache, skipEscape(&bptr->title[7]));
	if( bptr->brdattr & BRD_GROUPBOARD )
	    dumpclass(bptr - bcache);
    }
}

int main(int argc, char **argv)
{
    attach_SHM();

    printf("# this is auto-generated perl module from boardlist.c\n"
	   "# please do NOT modify this directly\n"
	   "\n"
	   "package boardlist;\n"
	   "use Exporter;\n"
	   "$VERSION = '0.1';\n"
	   "use vars qw(%%brd);\n"
	   "\n"
	   "%%brd = (\n");
    dumpclass(0);
    printf(");\n"
	   "our(@ISA, @EXPORT);\n"
	   "@ISA = qw(Exporter);\n"
	   "@EXPORT = qw(%%brd);\n"
	   "\n"
	   "1;\n");
    return 0;
}
