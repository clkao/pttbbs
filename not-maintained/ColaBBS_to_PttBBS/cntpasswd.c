/* $Id$ */
#include "bbs.h"
// usage: cd home/; cntpasswd */*/USERDATA.DAT > ~/.PASSWDS ; tunepasswd

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
    int     fd, i;

    for( i = 1 ; i < argc ; ++i ){
	if( (fd = open(argv[i], O_RDONLY)) > 0 &&
	    read(fd, &fu, sizeof(fu)) == sizeof(fu) &&
	    fu.userid[0] ){

	    memset(&u, 0, sizeof(u));
	    strlcpy(u.userid, fu.userid, sizeof(u.userid));
	    strlcpy(u.passwd, fu.passwd, sizeof(u.passwd));
	    strlcpy(u.realname, fu.name, sizeof(u.realname));
	    strlcpy(u.username, fu.nick, sizeof(u.username));
	    write(1, &u, sizeof(u));
	}
    }
    return 0;
}
