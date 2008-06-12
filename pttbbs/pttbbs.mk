# $Id$
# �w�q�򥻪��
BBSHOME?=	$(HOME)
BBSHOME?=	/home/bbs

SRCROOT?=	.

OSTYPE!=	uname

CC=		gcc
CCACHE!=	which ccache|sed -e 's/^.*\///'

PTT_CFLAGS:=	-Wall -pipe -DBBSHOME='"$(BBSHOME)"' -I$(SRCROOT)/include
PTT_LDFLAGS=
PTT_LDLIBS=	-lhz

# enable assert()
#PTT_CFLAGS+=	-DNDEBUG 

.if ${OSTYPE} == "FreeBSD"
# FreeBSD�S��������
PTT_CFLAGS+=  -I/usr/local/include
PTT_LDFLAGS+= -L/usr/local/lib
PTT_LDLIBS+=   -lkvm -liconv
.endif

# �Y���w�q PROFILING
.if defined(PROFILING)
PTT_CFLAGS+=	-pg
PTT_LDFLAGS+=	-pg
NO_OMITFP=	yes
NO_FORK=	yes
.endif

# �Y���w�q DEBUG, �h�b CFLAGS���w�q DEBUG
.if defined(DEBUG)
GDB=		1
#CFLAGS+=	-DDEBUG
PTT_CFLAGS+=	-DDEBUG 
.endif

.if defined(GDB)
CFLAGS:=	-g -O0 $(PTT_CFLAGS)
LDFLAGS:=	-O0 $(PTT_LDFLAGS)
LDLIBS:=	$(PTT_LDLIBS)
.else
CFLAGS:=	-g -Os $(PTT_CFLAGS) $(EXT_CFLAGS)
LDFLAGS:=	-Os $(PTT_LDFLAGS)
LDLIBS:=	$(PTT_LDLIBS)

.if defined(OMITFP)
CFLAGS+=	-fomit-frame-pointer
.endif
.endif


# �Y���w�q NO_FORK, �h�b CFLAGS���w�q NO_FORK
.if defined(NO_FORK)
CFLAGS+=	-DNO_FORK
.endif
