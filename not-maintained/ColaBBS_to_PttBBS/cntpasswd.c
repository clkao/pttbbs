/* $Id$ */
#include "bbs.h"
/* usage: ./cntpasswd < ColaBBS/.PASSWDS > ~/.PASSWDS */

typedef struct {
    char    userid[IDLEN + 1];
    char    pad0[1];
    char    passwd[PASSLEN];
    char    nick[24];
    char    pad1[16];
    char    name[20];
    char    pad2[424];
} fuserect_t;

int main(int argc, char **argv)
{
    fuserect_t  fu;
    userec_t    u;
    memset(&u, 0, sizeof(u));
    while( read(0, &fu, sizeof(fu)) == sizeof(fu) ){
#if 0
	printf("(%s), (%s), (%s), (%s)\n",
	       fu.userid, fu.passwd, fu.nick, fu.name);
#endif
	if( !fu.userid[0] )
	    continue;

	memset(&u, 0, sizeof(u));
	strlcpy(u.userid, fu.userid, sizeof(u.userid));
	strlcpy(u.passwd, fu.passwd, sizeof(u.passwd));
	strlcpy(u.realname, fu.name, sizeof(u.realname));
	strlcpy(u.username, fu.nick, sizeof(u.username));
	write(1, &u, sizeof(u));
    }
    return 0;
}
