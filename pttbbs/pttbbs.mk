# $Id: pttbbs.mk,v 1.8 2003/07/22 03:51:43 in2 Exp $
# �w�q�򥻪��
BBSHOME?=	$(HOME)
BBSHOME?=	/home/bbs
OSTYPE?=	FreeBSD
CC?=		gcc
CCACHE!=	which ccache|sed -e 's/^.*\///'
PTT_CFLAGS=	-Wall -pipe -DBBSHOME='"$(BBSHOME)"' -I../include
PTT_LDFLAGS=	-pipe -Wall
PTT_LIBS=	-lcrypt

# enable assert()
#PTT_CFLAGS+=	-DNDEBUG 

# FreeBSD�S��������
CFLAGS_FreeBSD=	-DHAVE_SETPROCTITLE -DFreeBSD
LDFLAGS_FreeBSD=
LIBS_FreeBSD=	-lkvm

# Linux�S��������
# CFLAGS_linux=   -DHAVE_DES_CRYPT -DLinux
CFLAGS_linux=	-DLinux
LDFLAGS_linux=	-pipe -Wall 
LIBS_linux=	

# CFLAGS, LDFLAGS, LIBS �[�J OS �����Ѽ�
PTT_CFLAGS+=	$(CFLAGS_$(OSTYPE))
PTT_LDFLAGS+=	$(LDFLAGS_$(OSTYPE))
PTT_LIBS+=	$(LIBS_$(OSTYPE))

# �Y���w�q GDB�� DEBUG, �h�[�J -g , �_�h�� -O
.if defined(GDB) || defined(DEBUG)
CFLAGS=		-g $(PTT_CFLAGS)
LDFLAGS=	-g $(PTT_LDFLAGS) $(PTT_LIBS)
.else
CFLAGS+=	-O2 -Os -fomit-frame-pointer -fstrength-reduce \
		-fthread-jumps -fexpensive-optimizations \
		$(PTT_CFLAGS)
LDFLAGS+=	-O2 $(PTT_LDFLAGS) $(PTT_LIBS)
.endif

# �Y���w�q DEBUG, �h�b CFLAGS���w�q DEBUG
.if defined(DEBUG)
CFLAGS+=	-DDEBUG
.endif

# �Y���w�q NO_FORK, �h�b CFLAGS���w�q NO_FORK
.if defined(NO_FORK)
CFLAGS+=	-DNO_FORK
.endif

.if defined(FreeBSD5)
CFLAGS+=	-DFreeBSD5
.endif
