/* $Id: chc_net.c,v 1.4 2002/07/21 09:26:02 in2 Exp $ */
#include "bbs.h"
typedef struct drc_t {
    rc_t            from, to;
}               drc_t;

int
chc_recvmove(int s)
{
    drc_t           buf;

    if (read(s, &buf, sizeof(buf)) != sizeof(buf))
	return 1;
    chc_from = buf.from, chc_to = buf.to;
    return 0;
}

void
chc_sendmove(int s)
{
    drc_t           buf;

    buf.from = chc_from, buf.to = chc_to;
    write(s, &buf, sizeof(buf));
}
