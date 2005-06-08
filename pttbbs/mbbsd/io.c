/* $Id$ */
#include "bbs.h"

#define OBUFSIZE  2048
#define IBUFSIZE  128

/* realXbuf is Xbuf+3 because hz convert library requires buf[-2]. */
static unsigned char real_outbuf[OBUFSIZE+6] = "   ", real_inbuf[IBUFSIZE+6] = "   ";
static unsigned char *outbuf = real_outbuf + 3, *inbuf = real_inbuf + 3;

static int      obufsize = 0, ibufsize = 0;
static int      icurrchar = 0;

/* ----------------------------------------------------- */
/* convert routines                                      */
/* ----------------------------------------------------- */
#ifdef CONVERT

extern read_write_type write_type;
extern read_write_type read_type;
extern convert_type    input_type;

inline static ssize_t input_wrapper(void *buf, ssize_t count) {
    /* input_wrapper is a special case.
     * because we may do nothing,
     * a if-branch is better than a function-pointer call.
     */
    if(input_type) return (*input_type)(buf, count);
    else return count;
}

inline static int read_wrapper(int fd, void *buf, size_t count) {
    return (*read_type)(fd, buf, count);
}

inline static int write_wrapper(int fd, void *buf, size_t count) {
    return (*write_type)(fd, buf, count);
}
#endif

/* ----------------------------------------------------- */
/* output routines                                       */
/* ----------------------------------------------------- */
void
oflush(void)
{
    if (obufsize) {
	STATINC(STAT_SYSWRITESOCKET);
#ifdef CONVERT
	write_wrapper(1, outbuf, obufsize);
#else
	write(1, outbuf, obufsize);
#endif
	obufsize = 0;
    }
}

void
init_buf(void)
{

    memset(inbuf, 0, IBUFSIZE);
}
void
output(const char *s, int len)
{
    /* Invalid if len >= OBUFSIZE */

    assert(len<OBUFSIZE);
    if (obufsize + len > OBUFSIZE) {
	STATINC(STAT_SYSWRITESOCKET);
#ifdef CONVERT
	write_wrapper(1, outbuf, obufsize);
#else
	write(1, outbuf, obufsize);
#endif
	obufsize = 0;
    }
    memcpy(outbuf + obufsize, s, len);
    obufsize += len;
}

int
ochar(int c)
{
    if (obufsize > OBUFSIZE - 1) {
	/* suppose one byte data doesn't need to be converted. */
	write(1, outbuf, obufsize);
	obufsize = 0;
    }
    outbuf[obufsize++] = c;
    return 0;
}

/* ----------------------------------------------------- */
/* input routines                                        */
/* ----------------------------------------------------- */

static int      i_newfd = 0;
static struct timeval i_to, *i_top = NULL;
static int      (*flushf) () = NULL;

void
add_io(int fd, int timeout)
{
    i_newfd = fd;
    if (timeout) {
	i_to.tv_sec = timeout;
	i_to.tv_usec = 16384;	/* Ptt: �令16384 �קK������for loop�Ycpu
				 * time 16384 ���C��64�� */
	i_top = &i_to;
    } else
	i_top = NULL;
}

int
num_in_buf(void)
{
    return icurrchar - ibufsize;
}

/*
 * dogetch() is not reentrant-safe. SIGUSR[12] might happen at any time, and
 * dogetch() might be called again, and then ibufsize/icurrchar/inbuf might
 * be inconsistent. We try to not segfault here...
 */

static int
dogetch(void)
{
    ssize_t         len;
    static time4_t  lastact;
    if (ibufsize <= icurrchar) {

	if (flushf)
	    (*flushf) ();

	refresh();

	if (i_newfd) {

	    struct timeval  timeout;
	    fd_set          readfds;

	    if (i_top)
		timeout = *i_top;	/* copy it because select() might
					 * change it */

	    FD_ZERO(&readfds);
	    FD_SET(0, &readfds);
	    FD_SET(i_newfd, &readfds);

	    /* jochang: modify first argument of select from FD_SETSIZE */
	    /* since we are only waiting input from fd 0 and i_newfd(>0) */

	    STATINC(STAT_SYSSELECT);
	    while ((len = select(i_newfd + 1, &readfds, NULL, NULL,
			    i_top ? &timeout : NULL)) < 0) {
		if (errno != EINTR)
		    abort_bbs(0);
		/* raise(SIGHUP); */
	    }

	    if (len == 0){
#ifdef OUTTA_TIMER
		now = SHM->GV2.e.now;
#else
		now = time(0);
#endif
		return I_TIMEOUT;
	    }

	    if (i_newfd && FD_ISSET(i_newfd, &readfds)){
#ifdef OUTTA_TIMER
		now = SHM->GV2.e.now;
#else
		now = time(0);
#endif
		return I_OTHERDATA;
	    }
	}

#ifdef NOKILLWATERBALL
	if( currutmp && currutmp->msgcount && !reentrant_write_request )
	    write_request(1);
#endif

	STATINC(STAT_SYSREADSOCKET);

	do {
	    len = tty_read(inbuf, IBUFSIZE);
	    /* tty_read will handle abort_bbs.
	     * len <= 0: read more */
#ifdef CONVERT
	    if(len > 0)
		len = input_wrapper(inbuf, len);
#endif
	} while (len <= 0);

	ibufsize = len;
	icurrchar = 0;
    }

    if (currutmp) {
#ifdef OUTTA_TIMER
	now = SHM->GV2.e.now;
#else
	now = time(0);
#endif
	/* 3 ���W�L�� byte �~�� active, anti-antiidle.
	 * ���L��V�䵥�զX�䤣�� 1 byte */
	if (now - lastact < 3)
	    currutmp->lastact = now;
	lastact = now;
    }
    return (unsigned char)inbuf[icurrchar++];
}

static int      water_which_flag = 0;

int
igetch(void)
{
    register int ch, mode = 0, last = 0;
    while (1) {
	ch = dogetch();

        if (mode == 0 && ch == KEY_ESC)           // here is state machine for 2 bytes key
	    mode = 1;
        else if (mode == 1) { /* Escape sequence */
            if (ch == '[' || ch == 'O')
                mode = 2;
            else if (ch == '1' || ch == '4')
		{ mode = 3; last = ch; }
            else {
                KEY_ESC_arg = ch;
                return KEY_ESC;
	    }
        } else if (mode == 2 && ch >= 'A' && ch <= 'D')  /* Cursor key */
	    return  KEY_UP + (ch - 'A');
	else  if (mode == 2 && ch >= '1' && ch <= '6')
	    { mode = 3; last = ch; }
	else if (mode == 3 && ch == '~') { /* Ins Del Home End PgUp PgDn */
	    return KEY_HOME + (last - '1');
        }
        else                                       //  here is switch for default keys
	switch (ch) { // XXX: indent error
#ifdef DEBUG
	case Ctrl('Q'):{
	    struct rusage ru;
	    getrusage(RUSAGE_SELF, &ru);
	    vmsg("sbrk: %d KB, idrss: %d KB, isrss: %d KB",
		 ((int)sbrk(0) - 0x8048000) / 1024,
		 (int)ru.ru_idrss, (int)ru.ru_isrss);
	}
	    continue;
#endif
	case Ctrl('L'):
	    redoscr();
	    continue;
	case Ctrl('U'):
	    if (currutmp != NULL && currutmp->mode != EDITING
		&& currutmp->mode != LUSERS && currutmp->mode) {

		void *screen0;
		int		oldroll = roll;
		int             y, x, my_newfd;

		// FIXME reveal heap data or crash if screen resize
		getyx(&y, &x);
		screen0=malloc(screen_backupsize(t_lines, big_picture));
		screen_backup(t_lines, big_picture, screen0);
		my_newfd = i_newfd;
		i_newfd = 0;

		t_users();

		i_newfd = my_newfd;
		screen_restore(t_lines, big_picture, screen0);
		free(screen0);
		roll = oldroll;
		move(y, x);
		redoscr();
		continue;
	    } 
            return ch;
	case KEY_TAB:
	    if (WATERMODE(WATER_ORIG) || WATERMODE(WATER_NEW))
		if (currutmp != NULL && watermode > 0) {
		    watermode = (watermode + water_which->count)
			% water_which->count + 1;
		    t_display_new();
		    continue;
		}
            return ch;

	case Ctrl('R'):
	    if (currutmp == NULL)
		return (ch);

	    if (currutmp->msgs[0].pid &&
		WATERMODE(WATER_OFO) && wmofo == NOTREPLYING) {
		int             y, x, my_newfd;
		void *screen0;

		screen0=malloc(screen_backupsize(t_lines, big_picture));
		screen_backup(t_lines, big_picture, screen0);
		getyx(&y, &x);

		my_newfd = i_newfd;
		i_newfd = 0;
		my_write2();
		screen_restore(t_lines, big_picture, screen0);
		free(screen0);
		i_newfd = my_newfd;
		move(y, x);
		redoscr();
		continue;
	    } else if (!WATERMODE(WATER_OFO)) {
		if (watermode > 0) {
		    watermode = (watermode + water_which->count)
			% water_which->count + 1;
		    t_display_new();
		    continue;
		} else if (currutmp->mode == 0 &&
		    (currutmp->chatid[0] == 2 || currutmp->chatid[0] == 3) &&
			   water_which->count != 0 && watermode == 0) {
		    /* �ĤG���� Ctrl-R */
		    watermode = 1;
		    t_display_new();
		    continue;
		} else if (watermode == -1 && currutmp->msgs[0].pid) {
		    /* �Ĥ@���� Ctrl-R (�������Q��L���y) */
		    void *screen0;
		    int             y, x, my_newfd;
		    getyx(&y, &x);
		    screen0=malloc(screen_backupsize(t_lines, big_picture));
		    screen_backup(t_lines, big_picture, screen0);

		    /* �p�G���btalk���ܥ����B�z���e�L�Ӫ��ʥ] (���hselect) */
		    my_newfd = i_newfd;
		    i_newfd = 0;
		    show_call_in(0, 0);
		    watermode = 0;
#ifndef PLAY_ANGEL
		    my_write(currutmp->msgs[0].pid, "���y��L�h�G ",
			    currutmp->msgs[0].userid, WATERBALL_GENERAL, NULL);
#else
		    switch (currutmp->msgs[0].msgmode) {
			case MSGMODE_TALK:
			case MSGMODE_WRITE:
			    my_write(currutmp->msgs[0].pid, "���y��L�h�G ",
				    currutmp->msgs[0].userid, WATERBALL_GENERAL, NULL);
			    break;
			case MSGMODE_FROMANGEL:
			    my_write(currutmp->msgs[0].pid, "�A�ݥL�@���G ",
				    currutmp->msgs[0].userid, WATERBALL_ANGEL, NULL);
			    break;
			case MSGMODE_TOANGEL:
			    my_write(currutmp->msgs[0].pid, "�^���p�D�H�G ",
				    currutmp->msgs[0].userid, WATERBALL_ANSWER, NULL);
			    break;
		    }
#endif
		    i_newfd = my_newfd;

		    /* �٭�ù� */
		    screen_restore(t_lines, big_picture, screen0);
		    free(screen0);
		    move(y, x);
		    redoscr();
		    continue;
		}
	    }
	    return ch;

	case Ctrl('T'):
	    if (WATERMODE(WATER_ORIG) || WATERMODE(WATER_NEW)) {
		if (watermode > 0) {
		    if (watermode > 1)
			watermode--;
		    else
			watermode = water_which->count;
		    t_display_new();
		    continue;
		}
	    }
            return ch;

	case Ctrl('F'):
	    if (WATERMODE(WATER_NEW)) {
		if (watermode > 0) {
		    if (water_which_flag == (int)water_usies)
			water_which_flag = 0;
		    else
			water_which_flag =
			    (water_which_flag + 1) % (int)(water_usies + 1);
		    if (water_which_flag == 0)
			water_which = &water[0];
		    else
			water_which = swater[water_which_flag - 1];
		    watermode = 1;
		    t_display_new();
		    continue;
		}
	    }
            return ch;

	case Ctrl('G'):
	    if (WATERMODE(WATER_NEW)) {
		if (watermode > 0) {
		    water_which_flag = (water_which_flag + water_usies) % (water_usies + 1);
		    if (water_which_flag == 0)
			water_which = &water[0];
		    else
			water_which = swater[water_which_flag - 1];
		    watermode = 1;
		    t_display_new();
		    continue;
		}
	    }
	    return ch;

	case Ctrl('J'):  /* Ptt �� \n ���� */
#ifdef PLAY_ANGEL
	    /* Seams some telnet client still send CR LF when changing lines.
	    CallAngel();
	    */
#endif
	    continue;

	default:
            return ch;
	}
    }
    // should not reach here. just to make compiler happy.
    return 0;
}

/**
 * �ھ� mode �� strip �r�� str�A�ç⵲�G�s�� buf
 * @param buf
 * @param str
 * @param mode enum {STRIP_ALL = 0, ONLY_COLOR, NO_RELOAD};
 *             STRIP_ALL: ??
 *             ONLY_COLOR: ??
 *             NO_RELOAD: �� strip (?)
 */
int
strip_ansi(char *buf, const char *str, int mode)
{
    register int    count = 0;
    static const char EscapeFlag[] = {
	/*  0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 10 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0, 0, 0, 0, 0,
	/* 20 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 30 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, /* 0~9 ;= */
	/* 40 */ 0, 2, 2, 2, 2, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, /* ABCDHIJK */
	/* 50 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 60 */ 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 2, 2, 0, 0, /* fhlm */
	/* 70 */ 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* su */
	/* 80 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 90 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* A0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* B0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* C0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* D0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* E0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* F0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };
#define isEscapeParam(X) (EscapeFlag[(int)(X)] & 1)
#define isEscapeCommand(X) (EscapeFlag[(int)(X)] & 2)

    for(; *str; ++str)
	if( *str != ESC_CHR ){
	    if( buf )
		*buf++ = *str;
	    ++count;
	}else{
	    const char* p = str + 1;
	    if( *p != '[' ){
		++str;
		continue;
	    }
	    while(isEscapeParam(*++p));
	    if( (mode == NO_RELOAD && isEscapeCommand(*p)) ||
		(mode == ONLY_COLOR && *p == 'm' )){
		register int len = p - str + 1;
		if( buf ){
		    strncpy(buf, str, len);
		    buf += len;
		}
		count += len;
	    }
	    str = p;
	}
    if( buf )
	*buf = 0;
    return count;
}

void
strip_nonebig5(unsigned char *str, int maxlen)
{
  int i;
  int len=0;
  for(i=0;i<maxlen && str[i];i++) {
    if(32<=str[i] && str[i]<128)
      str[len++]=str[i];
    else if(str[i]==255) {
      if(i+1<maxlen)
	if(251<=str[i+1] && str[i+1]<=254) {
	  i++;
	  if(i+1<maxlen && str[i+1])
	    i++;
	}
      continue;
    } else if(str[i]&0x80) {
      if(i+1<maxlen)
	if((0x40<=str[i+1] && str[i+1]<=0x7e) ||
	   (0xa1<=str[i+1] && str[i+1]<=0xfe)) {
	  str[len++]=str[i];
	  str[len++]=str[i+1];
	  i++;
	}
    }
  }
  if(len<maxlen)
    str[len]='\0';
}

#ifdef DBCSAWARE_GETDATA

#define ISDBCSAWARE() (!(cuser.uflag & RAWDBCS_FLAG))

int getDBCSstatus(unsigned char *s, int pos)
{
    int sts = DBCS_ASCII;
    while(pos >= 0)
    {
	if(sts == DBCS_LEADING)
	    sts = DBCS_TRAILING;
	else if (*s >= 0x80)
	{
	    sts = DBCS_LEADING;
	} else {
	    sts = DBCS_ASCII;
	}
	s++, pos--;
    }
    return sts;
}

#else

#define dbcs_off (1)

#endif

int
oldgetdata(int line, int col, const char *prompt, char *buf, int len, int echo)
{
    register int    ch, i;
    int             clen;
    int             x = col, y = line;
#define MAXLASTCMD 12
    static char     lastcmd[MAXLASTCMD][80];

    strip_ansi(buf, buf, STRIP_ALL);

    if (prompt) {
	move(line, col);
	clrtoeol();
	outs(prompt);
	x += strip_ansi(NULL, prompt, STRIP_ALL);
    }

    if (!echo) {
	len--;
	clen = 0;
	while ((ch = igetch()) != '\r') {
	    if (ch == '\177' || ch == Ctrl('H')) {
		if (!clen) {
		    bell();
		    continue;
		}
		clen--;
		continue;
	    }
	    if (!isprint(ch)) {
		continue;
	    }
	    if (clen >= len) {
		continue;
	    }
	    buf[clen++] = ch;
	}
	buf[clen] = '\0';
	outc('\n');
	oflush();
    } else {
	int             cmdpos = 0;
	int             currchar = 0;

	standout();
	for(i=0; i<len; i++)
	    outc(' ');
	standend();
	len--;
	buf[len] = '\0';
	move(y, x);
	edit_outs(buf);
	clen = currchar = strlen(buf);

	while (move(y, x + currchar), (ch = igetch()) != '\r') {
	    switch (ch) {
	    case KEY_DOWN: case Ctrl('N'):
	    case KEY_UP:   case Ctrl('P'):
		strlcpy(lastcmd[cmdpos], buf, sizeof(lastcmd[0]));
		if (ch == KEY_UP || ch == Ctrl('P'))
		    cmdpos++;
		else
		    cmdpos += MAXLASTCMD - 1;
		cmdpos %= MAXLASTCMD;
		strlcpy(buf, lastcmd[cmdpos], len+1);

		move(y, x);	/* clrtoeof */
		for (i = 0; i <= clen; i++)
		    outc(' ');
		move(y, x);
		edit_outs(buf);
		clen = currchar = strlen(buf);
		break;
	    case KEY_LEFT:
		if (currchar > 0)
		{
		    --currchar;
#ifdef DBCSAWARE_GETDATA
		    if(currchar > 0 && 
			    ISDBCSAWARE() &&
			    getDBCSstatus(buf, currchar) == DBCS_TRAILING)
			currchar --;
#endif
		}
		break;
	    case KEY_RIGHT:
		if (buf[currchar])
		{
		    ++currchar;
#ifdef DBCSAWARE_GETDATA
		    if(buf[currchar] &&
			    ISDBCSAWARE() &&
			    getDBCSstatus(buf, currchar) == DBCS_TRAILING)
			currchar++;
#endif
		}
		break;
	    case '\177':
	    case Ctrl('H'):
		if (currchar) {
#ifdef DBCSAWARE_GETDATA
		    int dbcs_off = 1;
		    if (ISDBCSAWARE() && 
			    getDBCSstatus(buf, currchar-1) == DBCS_TRAILING)
			dbcs_off = 2;
#endif
		    currchar -= dbcs_off;
		    clen -= dbcs_off;
		    for (i = currchar; i <= clen; i++)
			buf[i] = buf[i + dbcs_off];
		    move(y, x + clen);
		    outc(' ');
#ifdef DBCSAWARE_GETDATA
		    if(dbcs_off > 1) outc(' ');
#endif
		    move(y, x);
		    edit_outs(buf);
		}
		break;
	    case Ctrl('Y'):
		currchar = 0;
	    case Ctrl('K'):
		/* we shoud be able to avoid DBCS issues in ^K mode */
		buf[currchar] = '\0';
		move(y, x + currchar);
		for (i = currchar; i < clen; i++)
		    outc(' ');
		clen = currchar;
		break;
	    case Ctrl('D'):
	    case KEY_DEL:
		if (buf[currchar]) {
#ifdef DBCSAWARE_GETDATA
		    int dbcs_off = 1;
		    if (ISDBCSAWARE() && buf[currchar+1] && 
			    getDBCSstatus(buf, currchar+1) == DBCS_TRAILING)
		       dbcs_off = 2;
#endif
		    clen -= dbcs_off;
		    for (i = currchar; i <= clen; i++)
			buf[i] = buf[i + dbcs_off];
		    move(y, x + clen);
		    outc(' ');
#ifdef DBCSAWARE_GETDATA
		    if(dbcs_off > 1) outc(' ');
#endif
		    move(y, x);
		    edit_outs(buf);
		}
		break;
	    case Ctrl('A'):
	    case KEY_HOME:
		currchar = 0;
		break;
	    case Ctrl('E'):
	    case KEY_END:
		currchar = clen;
		break;
	    default:
		if (isprint2(ch) && clen < len && x + clen < scr_cols) {
		    for (i = clen + 1; i > currchar; i--)
			buf[i] = buf[i - 1];
		    buf[currchar] = ch;
		    move(y, x + currchar);
		    edit_outs(buf + currchar);
		    currchar++;
		    clen++;
		}
		break;
	    }			/* end case */
	}			/* end while */

	if (clen > 1) {
	    strlcpy(lastcmd[0], buf, sizeof(lastcmd[0]));
	    memmove(lastcmd+1, lastcmd, (MAXLASTCMD-1)*sizeof(lastcmd[0]));
	}
	outc('\n');
	refresh();
    }
    if ((echo == LCECHO) && isupper((int)buf[0]))
	buf[0] = tolower(buf[0]);
    return clen;
}

/* Ptt */
int
getdata_buf(int line, int col, const char *prompt, char *buf, int len, int echo)
{
    return oldgetdata(line, col, prompt, buf, len, echo);
}


int
getdata_str(int line, int col, const char *prompt, char *buf, int len, int echo, const char *defaultstr)
{
    strlcpy(buf, defaultstr, len);

    return oldgetdata(line, col, prompt, buf, len, echo);
}

int
getdata(int line, int col, const char *prompt, char *buf, int len, int echo)
{
    buf[0] = 0;
    return oldgetdata(line, col, prompt, buf, len, echo);
}

/* vim:sw=4
 */
