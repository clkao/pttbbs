# $Id$
# �w�q�򥻪��
BBSHOME?=	$(HOME)
BBSHOME?=	/home/bbs

OS!=		uname
OS_MAJOR_VER!=	uname -r|cut -d . -f 1
OS_MINOR_VER!=	uname -r|cut -d . -f 2
OSTYPE?=	$(OS)

CC=		gcc
CCACHE!=	which ccache|sed -e 's/^.*\///'
PTT_CFLAGS=	-Wall -pipe -DBBSHOME='"$(BBSHOME)"' -I../include
PTT_LDFLAGS=	-pipe -Wall -L/usr/local/lib
PTT_LIBS=	-lhz

# enable assert()
#PTT_CFLAGS+=	-DNDEBUG 

# FreeBSD�S��������
CFLAGS_FreeBSD=	-DHAVE_SETPROCTITLE -DFreeBSD -I/usr/local/include
LDFLAGS_FreeBSD=
LIBS_FreeBSD=	-lkvm -liconv

# Linux�S��������
# CFLAGS_linux=   -DHAVE_DES_CRYPT -DLinux
CFLAGS_Linux=	
LDFLAGS_Linux=	-pipe -Wall 
LIBS_Linux=	

# SunOS�S��������
CFLAGS_Solaris= -DSolaris -DHAVE_DES_CRYPT -I/usr/local/include 
LDFLAGS_Solaris= -L/usr/local/lib -L/usr/lib/
LIBS_Solaris= -lnsl -lsocket -liconv -lkstat

OS_FLAGS=	-D__OS_MAJOR_VERSION__="$(OS_MAJOR_VER)" \
		-D__OS_MINOR_VERSION__="$(OS_MINOR_VER)"

# CFLAGS, LDFLAGS, LIBS �[�J OS �����Ѽ�
PTT_CFLAGS+=	$(CFLAGS_$(OSTYPE)) $(OS_FLAGS)
PTT_LDFLAGS+=	$(LDFLAGS_$(OSTYPE))
PTT_LIBS+=	$(LIBS_$(OSTYPE))


# �Y���w�q PROFILING
.if defined(PROFILING)
PTT_CFLAGS+=	-pg
PTT_LDFLAGS+=	-pg
NO_OMITFP=	yes
NO_FORK=	yes
.endif

.if defined(USE_ICC)
CC=		icc
CFLAGS=		$(PTT_CFLAGS) -O1 -tpp6 -mcpu=pentiumpro -march=pentiumiii \
		-ip -ipo
LDFLAGS+=	-O1 -tpp6 -mcpu=pentiumpro -march=pentiumiii -ip -ipo \
		$(PTT_LDFLAGS) $(PTT_LIBS)
.else
# �Y���w�q GDB�� DEBUG, �h�[�J -g , �_�h�� -O
.if defined(GDB) || defined(DEBUG)
CFLAGS=		-g $(PTT_CFLAGS)
LDFLAGS=	-g $(PTT_LDFLAGS) $(PTT_LIBS)
.else
CFLAGS+=	-Os -fstrength-reduce \
		-fthread-jumps -fexpensive-optimizations \
		$(PTT_CFLAGS) $(EXT_CFLAGS)
LDFLAGS+=	-Os $(PTT_LDFLAGS) $(PTT_LIBS)

.if !defined(NO_OMITFP)
CFLAGS+=	-fomit-frame-pointer
.endif
.endif
.endif

# �Y���w�q DEBUG, �h�b CFLAGS���w�q DEBUG
.if defined(DEBUG)
CFLAGS+=	-DDEBUG
.endif

# �Y���w�q NO_FORK, �h�b CFLAGS���w�q NO_FORK
.if defined(NO_FORK)
CFLAGS+=	-DNO_FORK
.endif
