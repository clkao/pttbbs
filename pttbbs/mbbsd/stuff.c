/* $Id$ */
#include "bbs.h"

/* ----------------------------------------------------- */
/* set file path for boards/user home                    */
/* ----------------------------------------------------- */
static char    *str_home_file = "home/%c/%s/%s";
static char    *str_board_file = "boards/%c/%s/%s";
static char    *str_board_n_file = "boards/%c/%s/%s.%d";

#define STR_DOTDIR  ".DIR"
static char    *str_dotdir = STR_DOTDIR;

void
sethomepath(char *buf, char *userid)
{
    sprintf(buf, "home/%c/%s", userid[0], userid);
}

void
sethomedir(char *buf, char *userid)
{
    sprintf(buf, str_home_file, userid[0], userid, str_dotdir);
}

void
sethomeman(char *buf, char *userid)
{
    sprintf(buf, str_home_file, userid[0], userid, "man");
}


void
sethomefile(char *buf, char *userid, char *fname)
{
    sprintf(buf, str_home_file, userid[0], userid, fname);
}

void
setuserfile(char *buf, char *fname)
{
    sprintf(buf, str_home_file, cuser.userid[0], cuser.userid, fname);
}

void
setapath(char *buf, char *boardname)
{
    sprintf(buf, "man/boards/%c/%s", boardname[0], boardname);
}

void
setadir(char *buf, char *path)
{
    sprintf(buf, "%s/%s", path, str_dotdir);
}

void
setbpath(char *buf, char *boardname)
{
    sprintf(buf, "boards/%c/%s", boardname[0], boardname);
}

void
setbdir(char *buf, char *boardname)
{
    sprintf(buf, str_board_file, boardname[0], boardname,
	    (currmode & MODE_DIGEST ? fn_mandex : str_dotdir));
}

void
setbfile(char *buf, char *boardname, char *fname)
{
    sprintf(buf, str_board_file, boardname[0], boardname, fname);
}

void
setbnfile(char *buf, char *boardname, char *fname, int n)
{
    sprintf(buf, str_board_n_file, boardname[0], boardname, fname, n);
}
/*
 * input	direct
 * output	buf: copy direct
 * 		fname: direct ���ɦW����
 */
void
setdirpath(char *buf, char *direct, char *fname)
{
    strcpy(buf, direct);
    direct = strrchr(buf, '/');
    assert(direct);
    strcpy(direct + 1, fname);
}

char           *
subject(char *title)
{
    if (!strncasecmp(title, str_reply, 3)) {
	title += 3;
	if (*title == ' ')
	    title++;
    }
    return title;
}

/* ----------------------------------------------------- */
/* �r���ഫ�ˬd���                                      */
/* ----------------------------------------------------- */
int
str_checksum(char *str)
{
    int             n = 1;
    if (strlen(str) < 6)
	return 0;
    while (*str)
	n += *(str++) * (n);
    return n;
}

void
str_lower(char *t, char *s)
{
    register unsigned char ch;

    do {
	ch = *s++;
	*t++ = char_lower(ch);
    } while (ch);
}

int
strstr_lower(char *str, char *tag)
{
    char            buf[STRLEN];

    str_lower(buf, str);
    return (int)strstr(buf, tag);
}

void
trim(char *buf)
{				/* remove trailing space */
    char           *p = buf;

    while (*p)
	p++;
    while (--p >= buf) {
	if (*p == ' ')
	    *p = '\0';
	else
	    break;
    }
}

/* ----------------------------------------------------- */
/* �r���ˬd��ơG�^��B�Ʀr�B�ɦW�BE-mail address        */
/* ----------------------------------------------------- */

int
invalid_pname(char *str)
{
    char           *p1, *p2, *p3;

    p1 = str;
    while (*p1) {
	if (!(p2 = strchr(p1, '/')))
	    p2 = str + strlen(str);
	if (p1 + 1 > p2 || p1 + strspn(p1, ".") == p2)
	    return 1;
	for (p3 = p1; p3 < p2; p3++)
	    if (not_alnum(*p3) && !strchr("@[]-._", *p3))
		return 1;
	p1 = p2 + (*p2 ? 1 : 0);
    }
    return 0;
}

int
valid_ident(char *ident)
{
    char    *invalid[] = {"unknown@", "root@", "gopher@", "bbs@",
    "@bbs", "guest@", "@ppp", "@slip", NULL};
    char            buf[128];
    int             i;

    str_lower(buf, ident);
    for (i = 0; invalid[i]; i++)
	if (strstr(buf, invalid[i]))
	    return 0;
    return 1;
}

int
is_uBM(char *list, char *id)
{
    register int    len;

    if (list[0] == '[')
	list++;
    if (list[0] > ' ') {
	len = strlen(id);
	do {
	    if (!strncasecmp(list, id, len)) {
		list += len;
		if ((*list == 0) || (*list == '/') ||
		    (*list == ']') || (*list == ' '))
		    return 1;
	    }
	    if ((list = strchr(list, '/')) != NULL)
		list++;
	    else
		break;
	} while (1);
    }
    return 0;
}

int
is_BM(char *list)
{
    if (is_uBM(list, cuser.userid)) {
	cuser.userlevel |= PERM_BM;	/* Ptt �۰ʥ[�WBM���v�Q */
	return 1;
    }
    return 0;
}

int
userid_is_BM(char *userid, char *list)
{
    register int    ch, len;

    ch = list[0];
    if ((ch > ' ') && (ch < 128)) {
	len = strlen(userid);
	do {
	    if (!strncasecmp(list, userid, len)) {
		ch = list[len];
		if ((ch == 0) || (ch == '/') || (ch == ']'))
		    return 1;
	    }
	    while ((ch = *list++)) {
		if (ch == '/')
		    break;
	    }
	} while (ch);
    }
    return 0;
}

/* ----------------------------------------------------- */
/* �ɮ��ˬd��ơG�ɮסB�ؿ��B�ݩ�                        */
/* ----------------------------------------------------- */
off_t
dashs(char *fname)
{
    struct stat     st;

    if (!stat(fname, &st))
	return st.st_size;
    else
	return -1;
}

long
dasht(char *fname)
{
    struct stat     st;

    if (!stat(fname, &st))
	return st.st_mtime;
    else
	return -1;
}

int
dashl(char *fname)
{
    struct stat     st;

    return (lstat(fname, &st) == 0 && S_ISLNK(st.st_mode));
}

int
dashf(char *fname)
{
    struct stat     st;

    return (stat(fname, &st) == 0 && S_ISREG(st.st_mode));
}

int
dashd(char *fname)
{
    struct stat     st;

    return (stat(fname, &st) == 0 && S_ISDIR(st.st_mode));
}

int
belong(char *filelist, char *key)
{
    FILE           *fp;
    int             rc = 0;

    if ((fp = fopen(filelist, "r"))) {
	char            buf[STRLEN], *ptr;

	while (fgets(buf, STRLEN, fp)) {
	    if ((ptr = strtok(buf, str_space)) && !strcasecmp(ptr, key)) {
		rc = 1;
		break;
	    }
	}
	fclose(fp);
    }
    return rc;
}

#ifndef _BBS_UTIL_C_ /* getdata_buf */
time_t
gettime(int line, time_t dt, char*head)
{
    char            yn[7];
    int i;
    struct tm      *ptime = localtime(&dt), endtime;

    memcpy(&endtime, ptime, sizeof(struct tm));
    snprintf(yn, sizeof(yn), "%4d", ptime->tm_year + 1900);
    move(line, 0); prints("%s",head);
    i=strlen(head);
    do {
	getdata_buf(line, i, " �褸�~:", yn, 5, LCECHO);
    } while ((endtime.tm_year = atoi(yn) - 1900) < 0 || endtime.tm_year > 200);
    snprintf(yn, sizeof(yn), "%d", ptime->tm_mon + 1);
    do {
	getdata_buf(line, i+15, "��:", yn, 3, LCECHO);
    } while ((endtime.tm_mon = atoi(yn) - 1) < 0 || endtime.tm_mon > 11);
    snprintf(yn, sizeof(yn), "%d", ptime->tm_mday);
    do {
	getdata_buf(line, i+24, "��:", yn, 3, LCECHO);
    } while ((endtime.tm_mday = atoi(yn)) < 1 || endtime.tm_mday > 31);
    snprintf(yn, sizeof(yn), "%d", ptime->tm_hour);
    do {
	getdata_buf(line, i+33, "��(0-23):", yn, 3, LCECHO);
    } while ((endtime.tm_hour = atoi(yn)) < 0 || endtime.tm_hour > 23);
    return mktime(&endtime);
}
#endif

char           *
Cdate(time_t * clock)
{
    static char     foo[32];
    struct tm      *mytm = localtime(clock);

    strftime(foo, 32, "%m/%d/%Y %T %a", mytm);
    return foo;
}

char           *
Cdatelite(time_t * clock)
{
    static char     foo[32];
    struct tm      *mytm = localtime(clock);

    strftime(foo, 32, "%m/%d/%Y %T", mytm);
    return foo;
}

char           *
Cdatedate(time_t * clock)
{
    static char     foo[32];
    struct tm      *mytm = localtime(clock);

    strftime(foo, 32, "%m/%d/%Y", mytm);
    return foo;
}

#ifndef _BBS_UTIL_C_
/* �o�@�ϳ��O������e���B�z��, �G _BBS_UTIL_C_ �����n */
static void
capture_screen()
{
    char            fname[200];
    FILE           *fp;
    int             i;

    getdata(b_lines - 2, 0, "��o�ӵe�����J��Ȧs�ɡH[y/N] ",
	    fname, 4, LCECHO);
    if (fname[0] != 'y')
	return;

    setuserfile(fname, ask_tmpbuf(b_lines - 1));
    if ((fp = fopen(fname, "w"))) {
	for (i = 0; i < scr_lns; i++)
	    fprintf(fp, "%.*s\n", big_picture[i].len, big_picture[i].data);
	fclose(fp);
    }
}

int
vmsg_lines(const int lines, const char msg[])
{
    int             ch;

    move(lines, 0);
    clrtoeol();

    if (msg)
        outs((char *)msg);
    else
        outs("\033[45;1m                        \033[37m"
	     "\033[200m\033[1431m\033[506m�� �Ы� \033[33m(Space/Return)\033[37m �~�� ��\033[201m     (^T) ����Ȧs��   \033[m");

    do {
	if( (ch = igetch()) == Ctrl('T') )
	    capture_screen();
    } while( ch == 0 );

    move(lines, 0);
    clrtoeol();
    return ch;
}

#ifdef PLAY_ANGEL
void
pressanykey_or_callangel(){
    int             ch;

    outmsg("\033[37;45;1m  \033[33m(h)\033[37m �I�s�p�Ѩ�        "
	   "�� �Ы� \033[33m(Space/Return)\033[37m �~�� ��"
	   "       \033[33m(^T)\033[37m �s�Ȧs��   \033[m");
    do {
	ch = igetch();

	if (ch == Ctrl('T')) {
	    capture_screen();
	    break;
	}else if (ch == 'h' || ch == 'H'){
	    CallAngel();
	    break;
	}
    } while ((ch != ' ') && (ch != KEY_LEFT) && (ch != '\r') && (ch != '\n'));
    move(b_lines, 0);
    clrtoeol();
    refresh();
}
#endif

char
getans(const char *fmt,...)
{
    char   msg[256];
    char   ans[5];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg , 128, fmt, ap);
    va_end(ap);

    getdata(b_lines, 0, msg, ans, sizeof(ans), LCECHO);
    return ans[0];
}

int
getkey(const char *fmt,...)
{
    char   msg[256], i;
    va_list ap;
    va_start(ap, fmt);
    i = vsnprintf(msg , 128, fmt, ap);
    va_end(ap);
    return vmsg_lines(b_lines, msg);
}

int
vmsg(const char *fmt,...)
{
    char   msg[256] = "\033[1;36;44m �� ", i;
    va_list ap;
    va_start(ap, fmt);
    i = vsnprintf(msg + 14, 128, fmt, ap);
    va_end(ap);
    for(i = i + 14; i < 71; i++) 
	msg[(int)i] = ' ';
    strcat(msg + 71,
	   "\033[33;46m \033[200m\033[1431m\033[506m[�����N���~��]\033[201m \033[m");
    return vmsg_lines(b_lines, msg);
}


void
bell()
{
    char            c;

    c = Ctrl('G');
    write(1, &c, 1);
}

int
search_num(int ch, int max)
{
    int             clen = 1;
    int             x, y;
    char            genbuf[10];

    outmsg("\033[7m ���ܲĴX���G\033[m");
    outc(ch);
    genbuf[0] = ch;
    getyx(&y, &x);
    x--;
    while ((ch = igetch()) != '\r') {
	if (ch == 'q' || ch == 'e')
	    return -1;
	if (ch == '\n')
	    break;
	if (ch == '\177' || ch == Ctrl('H')) {
	    if (clen == 0) {
		bell();
		continue;
	    }
	    clen--;
	    move(y, x + clen);
	    outc(' ');
	    move(y, x + clen);
	    continue;
	}
	if (!isdigit(ch)) {
	    bell();
	    continue;
	}
	if (x + clen >= scr_cols || clen >= 6) {
	    bell();
	    continue;
	}
	genbuf[clen++] = ch;
	outc(ch);
    }
    genbuf[clen] = '\0';
    move(b_lines, 0);
    clrtoeol();
    if (genbuf[0] == '\0')
	return -1;
    clen = atoi(genbuf);
    if (clen == 0)
	return 0;
    if (clen > max)
	return max;
    return clen - 1;
}

void
stand_title(char *title)
{
    clear();
    prints("\033[1;37;46m�i %s �j\033[m\n", title);
}

void
cursor_show(int row, int column)
{
    move(row, column);
    outs(STR_CURSOR);
    move(row, column + 1);
}

void
cursor_clear(int row, int column)
{
    move(row, column);
    outs(STR_UNCUR);
}

int
cursor_key(int row, int column)
{
    int             ch;

    cursor_show(row, column);
    ch = igetch();
    move(row, column);
    outs(STR_UNCUR);
    return ch;
}

void
printdash(char *mesg)
{
    int             head = 0, tail;

    if (mesg)
	head = (strlen(mesg) + 1) >> 1;

    tail = head;

    while (head++ < 38)
	outch('-');

    if (tail) {
	outch(' ');
	outs(mesg);
	outch(' ');
    }
    while (tail++ < 38)
	outch('-');
    outch('\n');
}

int
log_user(const char *fmt, ...)
{
    char msg[256], filename[256];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(msg , 128, fmt, ap);
    va_end(ap);

    sethomefile(filename, cuser.userid, "USERLOG");
    return log_file(filename, LOG_CREAT | LOG_VF,
		    "%s: %s %s", cuser.userid, msg,  Cdate(&now));
}

int
log_file(char *fn, int flag, const char *fmt,...)
{
    int        fd;
    char       msg[256];
    const char *realmsg;
    if( !(flag & LOG_VF) ){
	realmsg = fmt;
    }
    else{
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(msg , 128, fmt, ap);
	va_end(ap);
	realmsg = msg;
    }

    if( (fd = open(fn, O_APPEND | O_WRONLY | ((flag & LOG_CREAT)? O_CREAT : 0),
		   ((flag & LOG_CREAT) ? 0664 : 0))) < 0 )
	return -1;
    if( write(fd, realmsg, strlen(realmsg)) < 0 ){
	close(fd);
	return -1;
    }
    close(fd);
    return 0;
}

void
show_help(char *helptext[])
{
    char           *str;
    int             i;

    clear();
    for (i = 0; (str = helptext[i]); i++) {
	if (*str == '\0')
	    prints("\033[1m�i %s �j\033[0m\n", str + 1);
	else if (*str == '\01')
	    prints("\n\033[36m�i %s �j\033[m\n", str + 1);
	else
	    prints("        %s\n", str);
    }
#ifdef PLAY_ANGEL
    pressanykey_or_callangel();
#else
    pressanykey();
#endif
}
#endif // _BBS_UTIL_C_

/* ----------------------------------------------------- */
/* use mmap() to malloc large memory in CRITICAL_MEMORY  */
/* ----------------------------------------------------- */
#ifdef CRITICAL_MEMORY
void *MALLOC(int size)
{
    int     *p;
    p = (int *)mmap(NULL, (size + 4), PROT_READ | PROT_WRITE, MAP_ANON, -1, 0);
    p[0] = size;
#if defined(DEBUG) && !defined(_BBS_UTIL_C_)
    vmsg("critical malloc %d bytes", size);
#endif
    return (void *)&p[1];
}

void FREE(void *ptr)
{
    int     size = ((int *)ptr)[-1];
    munmap((void *)(&(((int *)ptr)[-1])), size);
#if defined(DEBUG) && !defined(_BBS_UTIL_C_)
    vmsg("critical free %d bytes", size);
#endif
}
#endif

unsigned
StringHash(unsigned char *s)
{
    unsigned int    v = 0;
    while (*s) {
	v = (v << 8) | (v >> 24);
	v ^= toupper(*s++);	/* note this is case insensitive */
    }
    return (v * 2654435769UL) >> (32 - HASH_BITS);
}

inline int *intbsearch(int key, int *base0, int nmemb)
{
    /* ��� /usr/src/lib/libc/stdlib/bsearch.c ,
       �M���j int array �Ϊ�, ���z�L compar function �G���֨� */
    const   char *base = (char *)base0;
    size_t  lim;
    int     *p;

    for (lim = nmemb; lim != 0; lim >>= 1) {
	p = (int *)(base + (lim >> 1) * 4);
	if( key == *p )
	    return p;
	if( key > *p ){/* key > p: move right */
	    base = (char *)p + 4;
	    lim--;
	}               /* else move left */
    }
    return (NULL);
}

int qsort_intcompar(const void *a, const void *b)
{
    return *(int *)a - *(int *)b;
}

#ifdef OUTTACACHE
#include <err.h>
int tobind(int port)
{
    int     sockfd, val;
    struct  sockaddr_in     servaddr;

    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	err(1, NULL);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
	       (char *)&val, sizeof(val));
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    if( bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 )
	err(1, NULL);
    if( listen(sockfd, 5) < 0 )
	err(1, NULL);

    return sockfd;
}

int toconnect(char *host, int port)
{
    int    sock;
    struct sockaddr_in serv_name;
    if( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){
        perror("socket");
        return -1;
    }

    serv_name.sin_family = AF_INET;
    serv_name.sin_addr.s_addr = inet_addr(host);
    serv_name.sin_port = htons(port);
    if( connect(sock, (struct sockaddr*)&serv_name, sizeof(serv_name)) < 0 ){
        close(sock);
        return -1;
    }
    return sock;
}

int toread(int fd, void *buf, int len)
{
    int     l;
    for( l = 0 ; len > 0 ; )
	if( (l = read(fd, buf, len)) <= 0 )
	    return -1;
	else{
	    buf += l;
	    len -= l;
	}
    return l;
}

int towrite(int fd, void *buf, int len)
{
    int     l;
    for( l = 0 ; len > 0 ; )
	if( (l = write(fd, buf, len)) <= 0 )
	    return -1;
	else{
	    buf += l;
	    len -= l;
	}
    return l;
}
#endif
