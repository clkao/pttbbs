/* $Id: announce.c,v 1.12 2002/06/19 13:20:27 lwms Exp $ */
#include "bbs.h"

static void g_showmenu(gmenu_t *pm) {
    static char *mytype = "�s    ��     ��������";
    char *title, ch;
    int n, max;
    item_t *item;

    showtitle("��ؤ峹", pm->mtitle);
    prints("  \033[1;36m�s��    ��      �D%56s\033[m", mytype);
    
    if(pm->num) {
	n = pm->page;
	max = n + p_lines;
	if(max > pm->num)
	    max = pm->num;
	while(n < max) {
	    item = pm->item[n++];
	    title = item->title;
	    ch = title[1];
	    prints("\n%5d. %-72.71s", n, title);
	}
    } else
	outs("\n  �m��ذϡn�|�b�l���Ѧa��������� :)");

    move(b_lines, 1);
    outs(pm->level ?
	 "\033[34;46m �i�O  �D�j \033[31;47m  (h)\033[30m����  "
	 "\033[31m(q/��)\033[30m���}  \033[31m(n)\033[30m�s�W�峹  "
	 "\033[31m(g)\033[30m�s�W�ؿ�  \033[31m(e)\033[30m�s���ɮ�  \033[m" :
	 "\033[34;46m �i�\\����j \033[31;47m  (h)\033[30m����  "
	 "\033[31m(q/��)\033[30m���}  \033[31m(k��j��)\033[30m���ʴ��  "
	 "\033[31m(enter/��)\033[30mŪ�����  \033[m");
}

static FILE *go_cmd(item_t *node, int *sock) {
    struct sockaddr_in sin;
    struct hostent *host;
    char *site;
    FILE *fp;
    
    *sock = socket(AF_INET, SOCK_STREAM, 0);
    
    if(*sock < 0) {
	syslog(LOG_ERR, "socket(): %m");
	return NULL;
    }
    memset((char *)&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(node->X.G.port);
    
    host = gethostbyname(site = node->X.G.server);
    if(host == NULL)
	sin.sin_addr.s_addr = inet_addr(site);
    else
	memcpy(&sin.sin_addr.s_addr, host->h_addr, host->h_length);
    
    if(connect(*sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
	syslog(LOG_ERR, "connect(): %m");
	return NULL;
    }
    fp = fdopen(*sock, "r+");
    if(fp != NULL) {
	setbuf(fp, (char *) 0);
	fprintf(fp, "%s\r\n", node->X.G.path);
	fflush(fp);
    } else
	close(*sock);
    return fp;
}

static char *nextfield(char *data, char *field) {
    register int ch;
    
    while((ch = *data)) {
	data++;
	if((ch == '\t') || (ch == '\r' && *data == '\n'))
	    break;
	*field++ = ch;
    }
    *field = '\0';
    return data;
}

static FILE* my_open(char* path) {
    FILE* ans = 0;
    char buf[80];
    struct stat st;

    if(stat(path, &st) == 0 && st.st_mtime < now - 3600 * 24 * 7) {
	return fopen(path, "w");
    }
    
    if((ans = fopen(path, "r+"))) {
	fclose(ans);
	return 0;
	/*
	  return directly due to currutmp->pager > 1 mode (real copy)
	*/
	fgets(buf, 80, ans);
	if(!strncmp(buf, str_author1, strlen(str_author1)) ||
	   *buf == '0' || *buf == '1') {
	    fclose(ans);
	    return 0;
	}

	rewind(ans);
    } else
	ans = fopen(path, "w");
    return ans;
}

static jmp_buf jbuf;

static void isig(int sig) {
    longjmp(jbuf, 1);
}

#define PROXY_HOME      "proxy/"

static void go_proxy(char* fpath, item_t *node, int update) {
    char *ptr, *str, *server;
    int ch;
    static FILE *fo;
    
    strcpy(fpath, PROXY_HOME);
    ptr = fpath + sizeof(PROXY_HOME) - 1;
    str = server = node->X.G.server;
    while((ch = *str)) {
	str++;
	if(ch == '.') {
	    if(!strcmp(str, "edu.tw"))
		break;
	} else if(ch >= 'A' && ch <= 'Z') {
	    ch |= 0x20;
	}
	*ptr++ = ch;
    }
    *ptr = '\0';
    mkdir(fpath, 0755);
    
    *ptr++ = '/';
    str = node->X.G.path;
    while((ch = *str)) {
	str++;
	if(ch == '/') {
	    ch = '.';
	}
	*ptr++ = ch;
    }
    *ptr = '\0';
    
    /* expire proxy data */
    
    if((fo = update ? fopen(fpath, "w") : my_open(fpath))) {
	FILE *fp;
	char buf[512];
	int sock;
	
	if(fo == NULL)
	    return;
	
	outmsg("�� �إ� proxy ��Ƴs�u�� ... ");
	refresh();
	
	sock = -1;
	if(setjmp(jbuf)) {
	    if(sock != -1)
		close(sock);
	    fseek(fo, 0, SEEK_SET);
	    fwrite("", 0, 0, fo);
	    fclose(fo);
	    alarm(0);
	    return;
	}

	signal(SIGALRM, isig);
	alarm(5);
	fp = go_cmd(node, &sock);
	alarm(0);

	str = node->title;
	ch = str[1];
	if(ch == (char) 0xbc &&
	   !(HAS_PERM(PERM_SYSOP) && currutmp->pager > 1)) {
	    fprintf(fo, "�@��: %s (�s�u��ذ�)\n���D: %s\n�ɶ�: %s\n",
		    server, str + 3, ctime(&now)
		);
	}

	while(fgets(buf, 511, fp)) {
	    if(!strcmp(buf, ".\r\n"))
		break;
	    if((ptr = strstr(buf, "\r\n")))
		strcpy(ptr, "\n");
	    fputs(buf, fo);
	}
	fclose(fo);
	fclose(fp);
    }
}

static void g_additem(gmenu_t *pm, item_t *myitem) {
    if(pm->num < MAX_ITEMS) {
	item_t *newitem = (item_t *)malloc(sizeof(item_t));
	
	memcpy(newitem, myitem, sizeof(item_t));
	pm->item[(pm->num)++] = newitem;
    }
}

static void go_menu(gmenu_t *pm, item_t *node, int update) {
    FILE *fp;
    char buf[512], *ptr, *title;
    item_t item;
    int ch;
    
    go_proxy(buf, node, update);
    pm->num = 0;
    if((fp = fopen(buf, "r"))) {
	title = item.title;
	while(fgets(buf, 511, fp)) {
	    ptr = buf;
	    ch = *ptr++;
	    if(ch != '0' && ch != '1')
		continue;
	    
	    strcpy(title, "�� ");
	    if(ch == '1')
		title[1] = (char) 0xbd;

	    ptr = nextfield(ptr, title + 3);
	    if(!*ptr)
		continue;
	    title[sizeof(item.title) - 1] = '\0';

	    ptr = nextfield(ptr, item.X.G.path);
	    if(!*ptr)
		continue;

	    ptr = nextfield(ptr, item.X.G.server);
	    if(!*ptr)
		continue;

	    nextfield(ptr, buf);
	    item.X.G.port = atoi(buf);

	    g_additem(pm, &item);
	}
	fclose(fp);
    }
}

static int g_searchtitle(gmenu_t* pm, int rev) {
    static char search_str[30] = "";
    int pos;
    
    if(getdata(b_lines - 1, 1,"[�j�M]����r:", search_str, sizeof(search_str), DOECHO))
	if(!*search_str)
	    return pm->now;

    str_lower(search_str, search_str);

    rev = rev ? -1 : 1;
    pos = pm->now;
    do {
	pos += rev;
	if(pos == pm->num)
	    pos = 0;
	else if(pos < 0)
	    pos = pm->num - 1;
	if(strstr_lower(pm->item[pos]->title, search_str))
	    return pos;
    } while(pos != pm->now);
    return pm->now;
}

static void g_showhelp() {
    clear();
    outs("\033[36m�i " BBSNAME "�s�u��ذϨϥλ��� �j\033[m\n\n"
	 "[��][q]         ���}��W�@�h�ؿ�\n"
	 "[��][k]         �W�@�ӿﶵ\n"
	 "[��][j]         �U�@�ӿﶵ\n"
	 "[��][r][enter]  �i�J�ؿ���Ū���峹\n"
	 "[b][PgUp]       �W�����\n"
	 "[^F][PgDn][Spc] �U�����\n"
	 "[##]            ����ӿﶵ\n"
	 "[^S]            �N�峹�s��H�c\n"
	 "[R]             ��s���\n"
	 "[N]             �d���ɦW\n"
	 "[c][C][^C]      �����峹/�ø��ܤW���K�峹���a��/"
	 "�����K��W���K���a��\n");
    pressanykey();
}

#define PATHLEN     256

static char paste_fname[200];

static void load_paste() {
    struct stat st;
    FILE *fp;
    
    if(!*paste_fname)
	setuserfile(paste_fname, "paste_path");
    if(stat(paste_fname, &st) == 0 && st.st_mtime > paste_time &&
       (fp = fopen(paste_fname, "r"))) {
	int i;
	fgets(paste_path, PATHLEN, fp);
	i = strlen(paste_path) - 1;
	if(paste_path[i] == '\n')
	    paste_path[i] = 0;
	fgets(paste_title, STRLEN, fp);
	i = strlen(paste_title) - 1;
	if(paste_title[i] == '\n')
	    paste_title[i] = 0;
	fscanf(fp, "%d", &paste_level);
	paste_time = st.st_mtime;
	fclose(fp);
    }
}

static char copyfile[PATHLEN];
static char copytitle[TTLEN+1];
static char copyowner[IDLEN + 2];

void a_copyitem(char* fpath, char* title, char* owner, int mode) {
    strcpy(copyfile, fpath);
    strcpy(copytitle, title);
    if(owner)
	strcpy(copyowner, owner);
    else
	*copyowner = 0;
    if(mode) {
	outmsg("�ɮ׼аO�����C[�`�N] ������~��R�����!");
	igetch();
    }
}

#define FHSZ            sizeof(fileheader_t)

static void a_loadname(menu_t *pm) {
    char buf[PATHLEN];
    int len;
    
    setadir(buf, pm->path);
    len = get_records(buf, pm->header, FHSZ, pm->page+1, p_lines);
    if(len < p_lines)
	bzero(&pm->header[len], FHSZ*(p_lines-len));
}

static void a_timestamp(char *buf, time_t *time) {
    struct tm *pt = localtime(time);
    
    sprintf(buf, "%02d/%02d/%02d", pt->tm_mon + 1, pt->tm_mday, (pt->tm_year + 1900) % 100);
}

static void a_showmenu(menu_t *pm) {
    char *title, *editor;
    int n;
    fileheader_t *item;
    char buf[PATHLEN];
    time_t dtime;

    showtitle("��ؤ峹", pm->mtitle);
    prints("   \033[1;36m�s��    ��      �D%56s\033[0m",
	   "�s    ��      ��    ��");

    if(pm->num) {
	setadir(buf, pm->path);
	a_loadname(pm);
	for(n = 0; n < p_lines && pm->page + n < pm->num; n++) {
	    item = &pm->header[n];
	    title = item->title;
	    editor = item->owner;
	    /*  Ptt ��ɶ��אּ���ɮ׮ɶ�
		dtime = atoi(&item->filename[2]);
	    */
	    sprintf(buf,"%s/%s",pm->path,item->filename);
	    dtime = dasht(buf);
	    a_timestamp(buf, &dtime);
	    prints("\n%6d%c %-47.46s%-13s[%s]", pm->page+n+1, 
			    (item->filemode & FILE_BM) ?'X':
			    (item->filemode & FILE_HIDE) ?')':'.',
			    title, editor,
		   buf);
	}
    } else
	outs("\n  �m��ذϡn�|�b�l���Ѧa��������ؤ�... :)");

    move(b_lines, 1);
    outs(pm->level ?
	 "\033[34;46m �i�O  �D�j \033[31;47m  (h)\033[30m����  "
	 "\033[31m(q/��)\033[30m���}  \033[31m(n)\033[30m�s�W�峹  "
	 "\033[31m(g)\033[30m�s�W�ؿ�  \033[31m(e)\033[30m�s���ɮ�  \033[m" :
	 "\033[34;46m �i�\\����j \033[31;47m  (h)\033[30m����  "
	 "\033[31m(q/��)\033[30m���}  \033[31m(k��j��)\033[30m���ʴ��  "
	 "\033[31m(enter/��)\033[30mŪ�����  \033[m");
}

static int a_searchtitle(menu_t *pm, int rev) {
    static char search_str[40] = "";
    int pos;
    
    getdata(b_lines - 1, 1, "[�j�M]����r:", search_str, sizeof(search_str), DOECHO);
    
    if(!*search_str)
	return pm->now;
    
    str_lower(search_str, search_str);
    
    rev = rev ? -1 : 1;
    pos = pm->now;
    do {
	pos += rev;
	if(pos == pm->num)
	    pos = 0;
	else if(pos < 0)
	    pos = pm->num - 1;
	if(pos < pm->page || pos >= pm->page + p_lines) {
	    pm->page = pos - pos % p_lines;
	    a_loadname(pm);
	}
	if(strstr_lower(pm->header[pos - pm->page].title, search_str))
	    return pos;
    } while(pos != pm->now);
    return pm->now;
}

enum {NOBODY, MANAGER, SYSOP};

static void a_showhelp(int level) {
    clear();
    outs("\033[36m�i " BBSNAME "���G��ϥλ��� �j\033[m\n\n"
	 "[��][q]         ���}��W�@�h�ؿ�\n"
	 "[��][k]         �W�@�ӿﶵ\n"
	 "[��][j]         �U�@�ӿﶵ\n"
	 "[��][r][enter]  �i�J�ؿ���Ū���峹\n"
	 "[^B][PgUp]      �W�����\n"
	 "[^F][PgDn][Spc] �U�����\n"
	 "[##]            ����ӿﶵ\n"
	 "[F][U]          �N�峹�H�^ Internet �l�c/"
	 "�N�峹 uuencode ��H�^�l�c\n");
    if(level >= MANAGER) {
	outs("\n\033[36m�i �O�D�M���� �j\033[m\n"
             "[H]             ������ ���}/�|��/�O�D �~��\\Ū\n"
	     "[n/g/G]         ������ؤ峹/�}�P�ؿ�/�إ߳s�u\n"
	     "[m/d/D]         ����/�R���峹/�R���@�ӽd�򪺤峹\n"
	     "[f/T/e]         �s����D�Ÿ�/�ק�峹���D/���e\n"
	     "[c/p/a]         ����/�߶K/���[�峹\n"
	     "[^P/^A]         �߶K/���[�w��'t'�аO�峹\n");
    }

    if(level >= SYSOP) {
	outs("\n\033[36m�i �����M���� �j\033[m\n"
	     "[l]             �� symbolic link\n"
	     "[N]             �d���ɦW\n");
    }
    pressanykey();
}

static int AnnounceSelect() {
    static char xboard[20];
    char buf[20];
    char fpath[256];
    boardheader_t *bp;

    move(2, 0);
    clrtoeol();
    move(3, 0);
    clrtoeol();
    move(1, 0);
    generalnamecomplete("��ܺ�ذϬݪO�G", buf, sizeof(buf),
			SHM->Bnumber,
			completeboard_compar,
			completeboard_permission,
			completeboard_getname);
    if(*buf)
	strcpy(xboard, buf);
    if(*xboard && (bp = getbcache(getbnum(xboard)))) {
	setapath(fpath, xboard);
	setutmpmode(ANNOUNCE);
	a_menu(xboard, fpath,
	       (HAS_PERM(PERM_ALLBOARD) ||
		(HAS_PERM(PERM_BM) && is_BM(bp->BM))) ? 1 : 0);
    }
    return FULLUPDATE;
}

void gem(char* maintitle, item_t* path, int update) {
    gmenu_t me;
    int ch;
    char fname[PATHLEN];
    
    strncpy(me.mtitle, maintitle, 40);
    me.mtitle[40] = 0;
    go_menu(&me, path, update);
    
    /* ��ذ�-tree ���������c�ݩ� cuser ==> BM */
    
    me.level = 0;
    me.page = 9999;
    me.now = 0;
    for(;;) {
	if(me.now >= me.num && me.num > 0)
	    me.now = me.num - 1;
	else if(me.now < 0)
	    me.now = 0;
	
	if(me.now < me.page || me.now >= me.page + p_lines) {
	    me.page = me.now - (me.now % p_lines);
	    g_showmenu(&me);
	}
	ch = cursor_key(2 + me.now - me.page, 0);
	if(ch == 'q' || ch == 'Q' || ch == KEY_LEFT)
	    break;
	
	if(ch >= '0' && ch <= '9') {
	    if((ch = search_num(ch, me.num)) != -1)
		me.now = ch;
	    me.page = 9999;
	    continue;
	}
	
	switch(ch) {
	case KEY_UP:
	case 'k':
	    if(--me.now < 0)
		me.now = me.num - 1;
	    break;
	case KEY_DOWN:
	case 'j':
	    if(++me.now >= me.num)
		me.now = 0;
	    break;
	case KEY_PGUP:
	case 'b':
	    if(me.now >= p_lines)
		me.now -= p_lines;
	    else if(me.now > 0)
		me.now = 0;
	    else
		me.now = me.num - 1;
	    break;
	case ' ':
	case KEY_PGDN:
	case Ctrl('F'):
	    if(me.now < me.num - p_lines)
		me.now += p_lines;
	    else if(me.now < me.num - 1)
		me.now = me.num - 1;
	    else
		me.now = 0;
	    break;
	case 'h':
	    g_showhelp();
	    me.page = 9999;
	    break;
	case '?':
	case '/':
	    me.now = g_searchtitle(&me, ch == '?');
	    me.page = 9999;
	    break;
	case 'N':
	    if(HAS_PERM(PERM_SYSOP)) {
		go_proxy(fname, me.item[me.now], 0);
		move(b_lines - 1, 0);
		outs(fname);
		pressanykey();
		me.page = 9999;
	    }
	    break;
	case 'c':
	case 'C':
	case Ctrl('C'):
	    if(me.now < me.num) {
		item_t *node = me.item[me.now];
		char *title = node->title;
		int mode = title[1];

		load_paste();
		if(mode == (char) 0xbc || ch == Ctrl('C')) {
		    if(mode == (char) 0xbc)
			go_proxy(fname, node, 0);
		    if(ch == Ctrl('C') && *paste_path && paste_level) {
			char newpath[PATHLEN];
			fileheader_t item;

			strcpy(newpath, paste_path);
			if(mode == (char) 0xbc) {
			    stampfile(newpath, &item);
			    unlink(newpath);
			    Link(fname, newpath);
			} else
			    stampdir(newpath, &item);
			strcpy(item.owner, cuser.userid);
			sprintf(item.title, "%s%.72s",
				(currutmp->pager > 1) ? "" :
				(mode == (char) 0xbc) ? "�� " : "�� ",
				title + 3);
			strcpy(strrchr(newpath, '/') + 1, ".DIR");
			append_record(newpath, &item, FHSZ);
			if(++me.now >= me.num)
			    me.now = 0;
			break;
		    }
		    if(mode == (char) 0xbc) {
			a_copyitem(fname,
				   title + ((currutmp->pager > 1) ? 3 : 0),
				   0, 1);
			if(ch == 'C' && *paste_path) {
			    setutmpmode(ANNOUNCE);
			    a_menu(paste_title, paste_path, paste_level);
			}
			me.page = 9999;
		    } else
			bell();
		}
	    }
	    break;
	case Ctrl('B'):
	    m_read();
	    me.page = 9999;
	    break;
	case Ctrl('I'):
	    t_idle();
	    me.page = 9999;
	    break;
	case 's':
	    AnnounceSelect();
	    me.page = 9999;
	    break;
	case '\n':
	case '\r':
	case KEY_RIGHT:
	case 'r':
	case 'R':
	    if(me.now < me.num) {
		item_t *node = me.item[me.now];
		char *title = node->title;
		int mode = title[1];
		int update = (ch == 'R') ? 1 : 0;

		title += 3;

		if(mode == (char) 0xbc) {
		    int more_result;

		    go_proxy(fname, node, update);
		    strcpy(vetitle, title);
		    while((more_result = more(fname, YEA))) {
			if(more_result == 1) {
			    if(--me.now < 0) {
				me.now = 0;
				break;
			    }
			} else if(more_result == 3) {
			    if(++me.now >= me.num) {
				me.now = me.num - 1;
				break;
			    }
			} else
			    break;
			node = me.item[me.now];
			if(node->title[1] != (char) 0xbc)
			    break;
			go_proxy(fname, node, update);
			strcpy(vetitle, title);
		    }
		} else if(mode == (char) 0xbd) {
		    gem(title, node, update);
		}
		me.page = 9999;
	    }
	    break;
	}
    }
    for(ch = 0; ch < me.num; ch++)
	free(me.item[ch]);
}

static void a_forward(char *path, fileheader_t *pitem, int mode) {
    fileheader_t fhdr;
    
    strcpy(fhdr.filename, pitem->filename);
    strcpy(fhdr.title, pitem->title);
    switch(doforward(path, &fhdr, mode)) {
    case 0:
	outmsg(msg_fwd_ok);
	break;
    case -1:
	outmsg(msg_fwd_err1);
	break;
    case -2:
	outmsg(msg_fwd_err2);
	break;
    }
}

static void a_additem(menu_t *pm, fileheader_t *myheader) {
    char buf[PATHLEN];
    
    setadir(buf, pm->path);
    if(append_record(buf, myheader, FHSZ) == -1)
	return;
    pm->now = pm->num++;
    
    if(pm->now >= pm->page + p_lines) {
	pm->page = pm->now - ((pm->page == 10000 && pm->now > p_lines / 2) ?
			      (p_lines / 2) : (pm->now % p_lines));
    }      
    
    /*Ptt*/  
    strcpy(pm->header[pm->now - pm->page].filename, myheader->filename);
}

#define ADDITEM         0
#define ADDGROUP        1
#define ADDGOPHER       2
#define ADDLINK         3

static void a_newitem(menu_t *pm, int mode) {
    static char *mesg[4] = {
	"[�s�W�峹] �п�J���D�G",      /* ADDITEM */
	"[�s�W�ؿ�] �п�J���D�G",      /* ADDGROUP */
	"[�s�W�s�u] �п�J���D�G",      /* ADDGOPHER */
	"�п�J���D�G"                  /* ADDLINK */
    };

    char fpath[PATHLEN], buf[PATHLEN], lpath[PATHLEN];
    fileheader_t item;
    int d;

    strcpy(fpath, pm->path);

    switch(mode) {
    case ADDITEM:
	stampfile(fpath, &item);
	strcpy(item.title, "�� ");  /* A1BA */
	break;
	
    case ADDGROUP:
	stampdir(fpath, &item);
	strcpy(item.title, "�� ");  /* A1BB */
	break;
    case ADDGOPHER:
	bzero(&item, sizeof(item));
	strcpy(item.title, "�� ");  /* A1BB */
	if(!getdata(b_lines - 2, 1, "��JURL��}�G",
		    item.filename+2,61, DOECHO))
	    return;
	break;
    case ADDLINK:
	stamplink(fpath, &item);
	if (!getdata(b_lines - 2, 1, "�s�W�s�u�G", buf, 61, DOECHO))
	    return;
	if(invalid_pname(buf)) {
	    unlink(fpath);
	    outs("�ت��a���|���X�k�I");
	    igetch();
	    return;
	}

	item.title[0] = 0;
	for(d = 0; d <= 3; d++) {
	    switch(d) {
	    case 0:
		sprintf(lpath, BBSHOME "/man/boards/%c/%s/%s",
			currboard[0], currboard, buf);
		break;
	    case 1:
		sprintf(lpath, "%s%s/%c/%s",
			BBSHOME, "/man/boards/" , buf[0], buf);
		break;
	    case 2:
		sprintf(lpath, "%s%s%s",
			BBSHOME, "/" , buf);
		break;
	    case 3:
		sprintf(lpath, "%s%s%s",
			BBSHOME, "/etc/" , buf);
		break;
	    }
	    if(dashf(lpath)) {
		strcpy(item.title, "�� ");        /* A1B3 */
		break;
	    } else if (dashd(lpath)) {
		strcpy(item.title, "�� ");        /* A1B4 */
		break;
	    }
	    if(!HAS_PERM(PERM_BBSADM) && d==1)
		break;
	}
	
	if(!item.title[0]) {
	    unlink(fpath);
	    outs("�ت��a���|���X�k�I");
	    igetch();
	    return;
	}
    }

    if(!getdata(b_lines - 1, 1, mesg[mode], &item.title[3], 55, DOECHO)) {
	if(mode == ADDGROUP)
	    rmdir(fpath);
	else if(mode != ADDGOPHER)
	    unlink(fpath);
	return;
    }

    switch(mode) {
    case ADDITEM:
	if(vedit(fpath, 0, NULL) == -1) {
	    unlink(fpath);
	    pressanykey();
	    return;
	}
	break;
    case ADDLINK:
	unlink(fpath);
	if(symlink(lpath, fpath) == -1) {
	    outs("�L�k�إ� symbolic link");
	    igetch();
	    return;
	}
	break;
    case ADDGOPHER:
	strcpy(item.date, "70");
	strncpy(item.filename, "H.",2);
	break;
    }

    strcpy(item.owner, cuser.userid);
    a_additem(pm, &item);
}

static void a_pasteitem(menu_t *pm, int mode) {
    char newpath[PATHLEN];
    char buf[PATHLEN];
    char ans[2];
    int i;
    fileheader_t item;

    move(b_lines - 1, 1);
    if(copyfile[0]) {
	if(dashd(copyfile)) {
	    for(i = 0; copyfile[i] && copyfile[i] == pm->path[i]; i++);
	    if(!copyfile[i]) {
		outs("�N�ؿ����i�ۤv���l�ؿ����A�|�y���L�a�j��I");
		igetch();
		return;
	    }
	}
	if(mode) {
	    sprintf(buf, "�T�w�n����[%s]��(Y/N)�H[N] ", copytitle);
	    getdata(b_lines - 1, 1, buf, ans, sizeof(ans), LCECHO);
	} else
	    ans[0]='y';
	if(ans[0] == 'y') {
	    strcpy(newpath, pm->path);

	    if(*copyowner) {
		char* fname = strrchr(copyfile, '/');

		if(fname)
		    strcat(newpath, fname);
		else
		    return;
		if(access(pm->path, X_OK | R_OK | W_OK))
		    mkdir(pm->path, 0755);
		memset(&item, 0, sizeof(fileheader_t));
		strcpy(item.filename, fname + 1);
		memcpy(copytitle, "��", 2);
		if(HAS_PERM(PERM_BBSADM))
		    Link(copyfile, newpath);
		else {
		    sprintf(buf, "/bin/cp %s %s", copyfile, newpath);
		    system(buf);
		}
	    } else if(dashf(copyfile)) {
		stampfile(newpath, &item);
		memcpy(copytitle, "��", 2);
		sprintf(buf, "/bin/cp %s %s", copyfile, newpath);
	    } else if(dashd(copyfile)) {
		stampdir(newpath, &item);
		memcpy(copytitle, "��", 2);
		sprintf(buf, "/bin/cp -r %s/* %s/.D* %s", copyfile, copyfile,
			newpath);
	    } else {
		outs("�L�k�����I");
		igetch();
		return;
	    }
	    strcpy(item.owner, *copyowner ? copyowner : cuser.userid);
	    strcpy(item.title, copytitle);
	    if(!*copyowner)
		system(buf);
	    a_additem(pm, &item);
	    copyfile[0] = '\0';
	}
    } else {
	outs("�Х����� copy �R�O��A paste");
	igetch();
    }
}

static void a_appenditem(menu_t *pm, int isask) {
    char fname[PATHLEN];
    char buf[ANSILINELEN];
    char ans[2] = "y";
    FILE *fp, *fin;

    move(b_lines - 1, 1);
    if(copyfile[0]) {
	if(dashf(copyfile)) {
	    sprintf(fname, "%s/%s", pm->path,
		    pm->header[pm->now-pm->page].filename);
	    if(dashf(fname)) {
		if(isask) {
		    sprintf(buf, "�T�w�n�N[%s]���[�󦹶�(Y/N)�H[N] ",
			    copytitle);
		    getdata(b_lines - 2, 1, buf, ans, sizeof(ans), LCECHO);
		}
		if(ans[0] == 'y') {
		    if((fp = fopen(fname, "a+"))) {
			if((fin = fopen(copyfile, "r"))) {
			    memset(buf, '-', 74);
			    buf[74] = '\0';
			    fprintf(fp, "\n> %s <\n\n", buf);
			    if(isask)
				getdata(b_lines - 1, 1,
					"�O�_����ñ�W�ɳ���(Y/N)�H[Y] ",
					ans, sizeof(ans), LCECHO);
			    while(fgets(buf, sizeof(buf), fin)) {
				if((ans[0] == 'n' ) &&
				   !strcmp(buf, "--\n"))
				    break;
				fputs(buf, fp);
			    }
			    fclose(fin);
			    copyfile[0] = '\0';
			}
			fclose(fp);
		    }
		}
	    } else {
		outs("�ɮפ��o���[�󦹡I");
		igetch();
	    }
	} else {
	    outs("���o���[��ӥؿ����ɮ׫�I");
	    igetch();
	}
    } else {
	outs("�Х����� copy �R�O��A append");
	igetch();
    }
}

static int a_pastetagpost(menu_t *pm, int mode) {
    fileheader_t fhdr;
    int ans = 0, ent=0, tagnum;
    char title[TTLEN + 1]=  "��  ";
    char dirname[200],buf[200];

    setbdir(dirname, currboard);
    tagnum = TagNum;

    if (!tagnum) return ans;

    while (tagnum--)
    {
      EnumTagFhdr (&fhdr, dirname, ent++);
      setbfile (buf, currboard, fhdr.filename);

      if (dashf (buf))
      {
        strncpy(title+3, fhdr.title, TTLEN-3);
        title[TTLEN] = '\0';
        a_copyitem(buf, title, 0, 0);
        if(mode) 
        {
  	  mode--;     
	  a_pasteitem(pm,0);
        } 
        else a_appenditem(pm, 0);
        ++ans;
        UnTagger (tagnum);
      }

    };

    return ans;
}          

static void a_moveitem(menu_t *pm) {
    fileheader_t *tmp;
    char newnum[4];
    int num, max, min;
    char buf[PATHLEN];
    int fail;

    sprintf(buf, "�п�J�� %d �ﶵ���s���ǡG", pm->now + 1);
    if(!getdata(b_lines - 1, 1, buf, newnum, sizeof(newnum), DOECHO))
	return;
    num = (newnum[0] == '$') ? 9999 : atoi(newnum) - 1;
    if(num >= pm->num)
	num = pm->num - 1;
    else if(num < 0)
	num = 0;
    setadir(buf, pm->path);
    min = num < pm->now ? num : pm->now;
    max = num > pm->now ? num : pm->now;
    tmp = (fileheader_t *) calloc(max + 1, FHSZ);

    fail = 0;
    if(get_records(buf, tmp, FHSZ, 1, min) != min)
	fail = 1;
    if(num > pm->now) {
	if(get_records(buf, &tmp[min], FHSZ, pm->now+2, max-min) != max-min)
	    fail = 1;
	if(get_records(buf, &tmp[max], FHSZ, pm->now+1, 1) != 1)
	    fail = 1;
    } else {
	if(get_records(buf, &tmp[min], FHSZ, pm->now+1, 1) != 1)
	    fail = 1;
	if(get_records(buf, &tmp[min+1], FHSZ, num+1, max-min) != max-min)
	    fail = 1;
    }
    if(!fail)
	substitute_record(buf, tmp, FHSZ * (max + 1), 1);
    pm->now = num;
    free(tmp);
}

static void a_delrange(menu_t *pm) {
    char fname[256];

    sprintf(fname,"%s/.DIR",pm->path);
    del_range(0, NULL, fname);
    pm->num = get_num_records(fname, FHSZ);
}

static void a_delete(menu_t *pm) {
    char fpath[PATHLEN], buf[PATHLEN], cmd[PATHLEN];
    char ans[4];
    fileheader_t backup;

    sprintf(fpath, "%s/%s", pm->path, pm->header[pm->now - pm->page].filename);
    setadir(buf, pm->path);
        
    if(pm->header[pm->now - pm->page].filename[0] == 'H' && 
       pm->header[pm->now - pm->page].filename[1] == '.') {
	getdata(b_lines - 1, 1, "�z�T�w�n�R������ذϳs�u��(Y/N)�H[N] ",
		ans, sizeof(ans), LCECHO);
	if(ans[0] != 'y')
	    return;
	if(delete_record(buf, FHSZ, pm->now + 1) == -1)
	    return;
    } else if (dashl(fpath)) {
	getdata(b_lines - 1, 1, "�z�T�w�n�R���� symbolic link ��(Y/N)�H[N] ",
		ans, sizeof(ans), LCECHO);
	if(ans[0] != 'y')
	    return;
	if(delete_record(buf, FHSZ, pm->now + 1) == -1)
	    return;
	unlink(fpath);
    } else if(dashf(fpath)) {
	getdata(b_lines - 1, 1, "�z�T�w�n�R�����ɮ׶�(Y/N)�H[N] ", ans,
		sizeof(ans), LCECHO);
	if(ans[0] != 'y')
	    return;
	if(delete_record(buf, FHSZ, pm->now + 1) == -1)
	    return; 

	setbpath(buf, "deleted");
	stampfile(buf, &backup);
	strcpy(backup.owner, cuser.userid);
	strcpy(backup.title,pm->header[pm->now - pm->page].title + 2);

	sprintf(cmd, "mv -f %s %s", fpath,buf);
	system(cmd);
	setbdir(buf, "deleted"); 
	append_record(buf, &backup, sizeof(backup)); 
    } else if (dashd(fpath)) {
	getdata(b_lines - 1, 1, "�z�T�w�n�R����ӥؿ���(Y/N)�H[N] ", ans, 
		sizeof(ans), LCECHO);
	if(ans[0] != 'y')
	    return;
	if(delete_record(buf, FHSZ, pm->now + 1) == -1)
	    return;

	setapath(buf, "deleted"); 
	stampdir(buf, &backup); 

	sprintf(cmd, "rm -rf %s;/bin/mv -f %s %s",buf,fpath,buf); 
	system(cmd); 

	strcpy(backup.owner, cuser.userid); 
	strcpy(backup.title,pm->header[pm->now - pm->page].title +2); 
	setapath(buf, "deleted");  
	setadir(buf,buf);
	append_record(buf, &backup, sizeof(backup));  
    } else { /* Ptt �l�������� */
	getdata(b_lines - 1, 1, "�z�T�w�n�R�����l�������ض�(Y/N)�H[N] ",
		ans, sizeof(ans), LCECHO);
	if(ans[0] != 'y')
	    return;
	if(delete_record(buf, FHSZ, pm->now + 1) == -1)
	    return;
    }
    pm->num--;
}

static void a_newtitle(menu_t *pm) {
    char buf[PATHLEN];
    fileheader_t item;

    memcpy(&item, &pm->header[pm->now - pm->page], FHSZ);
    strcpy(buf,item.title + 3);
    if(getdata_buf(b_lines - 1, 1, "�s���D�G", buf, 60, DOECHO)) {
	strcpy(item.title + 3, buf);
	setadir(buf, pm->path);
	substitute_record(buf, &item, FHSZ, pm->now + 1);
    }
}
static void a_hideitem(menu_t *pm) {
    fileheader_t *item=&pm->header[pm->now - pm->page];
    char buf[PATHLEN];
    if(item->filemode&FILE_BM)
	{
          item->filemode &= ~FILE_BM;
	  item->filemode &= ~FILE_HIDE;
        }
    else if(item->filemode&FILE_HIDE)
          item->filemode |= FILE_BM;
    else item->filemode |= FILE_HIDE;
    setadir(buf, pm->path);
    substitute_record(buf, item, FHSZ, pm->now + 1);
}
static void a_editsign(menu_t *pm) {
    char buf[PATHLEN];
    fileheader_t item;

    memcpy(&item, &pm->header[pm->now - pm->page], FHSZ);
    sprintf(buf, "%c%c", item.title[0], item.title[1]);
    if(getdata_buf(b_lines - 1, 1, "�Ÿ�", buf, 5, DOECHO)) {
	item.title[0] = buf[0] ? buf[0] : ' ';
	item.title[1] = buf[1] ? buf[1] : ' ';
	item.title[2] = buf[2] ? buf[2] : ' ';
	setadir(buf, pm->path);
	substitute_record(buf, &item, FHSZ, pm->now + 1);
    }
}

static void a_showname(menu_t *pm) {
    char buf[PATHLEN];
    int len;
    int i;
    int sym;

    move(b_lines - 1, 1);
    sprintf(buf, "%s/%s", pm->path, pm->header[pm->now - pm->page].filename);
    if(dashl(buf)) {
	prints("�� symbolic link �W�٬� %s\n",
	       pm->header[pm->now - pm->page].filename);
	if((len = readlink(buf, buf, PATHLEN-1)) >= 0) {
	    buf[len] = '\0';
	    for(i = 0; BBSHOME[i] && buf[i] == BBSHOME[i]; i++);
	    if(!BBSHOME[i] && buf[i] == '/') {
		if(HAS_PERM(PERM_BBSADM))
		    sym = 1;
		else {
		    sym = 0;
		    for(i++; BBSHOME "/man"[i] && buf[i] == BBSHOME "/man"[i];
			i++);
		    if(!BBSHOME "/man"[i] && buf[i] == '/')
			sym = 1;
		}
		if(sym) {
		    pressanykey();
		    move(b_lines - 1, 1);
		    prints("�� symbolic link ���V %s\n", &buf[i+1]);
		}
	    }
	}
    } else if(dashf(buf))
	prints("���峹�W�٬� %s", pm->header[pm->now - pm->page].filename);
    else if(dashd(buf))
	prints("���ؿ��W�٬� %s", pm->header[pm->now - pm->page].filename);
    else
	outs("�����ؤw�l��, ��ĳ�N��R���I");
    pressanykey();
}

#if 0
static char *a_title;

static void atitle() {
    showtitle("��ؤ峹", a_title);
    outs("[��]���} [��]�\\Ū [^P]�o��峹 [b]�Ƨѿ� [d]�R�� [q]��ذ� "
	 "[TAB]��K [h]elp\n\033[7m  �s��   �� ��  �@  ��       "
	 "��  ��  ��  �D\033[m");
}
#endif

static int isvisible_man(menu_t  *me)
{
  fileheader_t *fhdr = &me->header[me->now-me->page];
  if( me->level<MANAGER && ((fhdr->filemode & FILE_BM) ||
        ((fhdr->filemode & FILE_HIDE) && 
        hbflcheck(currbid, currutmp->uid)))) 
	   return 0;
  return 1;
}
int a_menu(char *maintitle, char *path, int lastlevel) {
    static char Fexit;
    menu_t me;
    char fname[PATHLEN];
    int ch, returnvalue = FULLUPDATE;
    
    trans_buffer[0] = 0;
    
    Fexit = 0;
    me.header = (fileheader_t *)calloc(p_lines, FHSZ);
    me.path = path;
    strcpy(me.mtitle, maintitle);
    setadir(fname, me.path);
    me.num = get_num_records(fname, FHSZ);
    
    /* ��ذ�-tree ���������c�ݩ� cuser ==> BM */
    
    if(!(me.level = lastlevel)) {
	char *ptr;
	
	if((ptr = strrchr(me.mtitle, '[')))
	    me.level = is_BM(ptr + 1);
    }
    
    me.page = 9999;
    me.now = 0;
    for(;;) {
	if(me.now >= me.num)
	    me.now = me.num - 1;
	if(me.now < 0)
	    me.now = 0;

	if(me.now < me.page || me.now >= me.page + p_lines) {
	    me.page = me.now - ((me.page == 10000 && me.now > p_lines / 2) ?
				(p_lines / 2) : (me.now % p_lines));
	    a_showmenu(&me);
	}
	
	ch = cursor_key(2 + me.now - me.page, 0);
	
	if(ch == 'q' || ch == 'Q' || ch == KEY_LEFT)
	    break;
	
	if(ch >= '1' && ch <= '9') {
	    if((ch = search_num(ch, me.num)) != -1)
		me.now = ch;
	    me.page = 10000;
	    continue;
	}
	
	switch(ch) {
	case KEY_UP:
	case 'k':
	    if(--me.now < 0)
		me.now = me.num - 1;
	    break;
	    
	case KEY_DOWN:
	case 'j':
	    if(++me.now >= me.num)
		me.now = 0;
	    break;

	case KEY_PGUP:
	case Ctrl('B'):
	    if(me.now >= p_lines)
		me.now -= p_lines;
	    else if (me.now > 0)
		me.now = 0;
	    else
		me.now = me.num - 1;
	    break;

	case ' ':
	case KEY_PGDN:
	case Ctrl('F'):
	    if(me.now < me.num - p_lines)
		me.now += p_lines;
	    else if(me.now < me.num - 1)
		me.now = me.num - 1;
	    else
		me.now = 0;
	    break;

	case '0':
	    me.now = 0;
	    break;
	case '?':
	case '/':
	    me.now = a_searchtitle(&me, ch == '?');
	    me.page = 9999;
	    break;
	case '$':
	    me.now = me.num -1;
	    break;
	case 'h':
	    a_showhelp(me.level);
	    me.page = 9999;
	    break;
	case Ctrl('C'):
	    cal();
	    me.page = 9999;
	    break;

	case Ctrl('I'):
	    t_idle();
	    me.page = 9999;
	    break;

	case 's':
	    AnnounceSelect();
	    me.page = 9999;
	    break;

	case 'e':
	case 'E':
	    sprintf(fname, "%s/%s", path, me.header[me.now-me.page].filename);
	    if(dashf(fname) && me.level >= MANAGER) {
		*quote_file = 0;
		if(vedit(fname, NA, NULL) != -1) {
		    char fpath[200];
		    fileheader_t fhdr;

		    strcpy(fpath, path);
		    stampfile(fpath, &fhdr);
		    unlink(fpath);
		    Rename(fname, fpath);
		    strcpy(me.header[me.now-me.page].filename, fhdr.filename);
		    strcpy(me.header[me.now-me.page].owner, cuser.userid);
		    setadir(fpath, path);
		    substitute_record(fpath, me.header+me.now-me.page,
				      sizeof(fhdr), me.now  + 1);
		}
		me.page = 9999;
	    }
	    break;

	case 'c':
	    if(me.now < me.num) {
		if(!isvisible_man(&me)) break;
		sprintf(fname, "%s/%s", path,
			me.header[me.now-me.page].filename);
		a_copyitem(fname, me.header[me.now-me.page].title, 0, 1);
		me.page = 9999;
		break;
	    }

	case '\n':
	case '\r':
	case KEY_RIGHT:
	case 'r':
	    if(me.now < me.num) {
		fileheader_t *fhdr = &me.header[me.now-me.page];
		if(!isvisible_man(&me)) break;
		sprintf(fname, "%s/%s", path, fhdr->filename);
		if(*fhdr->filename == 'H' && fhdr->filename[1] == '.') {
		    item_t item;
		    strcpy(item.X.G.server, fhdr->filename + 2);
		    strcpy(item.X.G.path, "1/");
		    item.X.G.port = 70;
		    gem(fhdr->title, &item, (ch == 'R') ? 1 : 0);
		} else if (dashf(fname)) {
		    int more_result;

		    while((more_result = more(fname, YEA))) {
			/* Ptt �d�����F plugin */
			if(currstat == EDITEXP || currstat == OSONG) {
			    char ans[4];

			    move(22, 0);
			    clrtoeol();
			    getdata(22, 1, 
				    currstat == EDITEXP ? 
				    "�n��d�� Plugin ��峹��?[y/N]":
				    "�T�w�n�I�o���q��?[y/N]",
				    ans, sizeof(ans), LCECHO);
			    if(ans[0]=='y') {
				strcpy(trans_buffer,fname);
				Fexit = 1;
				if(currstat == OSONG){				
				    log_file(FN_USSONG,fhdr->title);
				}
				free(me.header);
				return FULLUPDATE;
			    }
			}
			if(more_result == 1) {
			    if(--me.now < 0) {
				me.now = 0;
				break;
			    }
			} else if(more_result == 3) {
			    if(++me.now >= me.num) {
				me.now = me.num - 1;
				break;
			    }
			} else
			    break;
			if(!isvisible_man(&me)) break;
			sprintf(fname, "%s/%s", path,
				me.header[me.now-me.page].filename);
			if(!dashf(fname))
			    break;
		    }
		} else if(dashd(fname)) {
		    a_menu(me.header[me.now-me.page].title, fname, me.level);
		    /* Ptt  �j�O���Xrecursive */
		    if(Fexit) {
			free(me.header);
			return FULLUPDATE;
		    }
		}
		me.page = 9999;
	    }
	    break;

	case 'F':
	case 'U':
	    sprintf(fname, "%s/%s", path, me.header[me.now-me.page].filename);
	    if(me.now < me.num && HAS_PERM(PERM_BASIC) && dashf(fname)) {
		    a_forward(path, &me.header[me.now-me.page], ch /*== 'U'*/);
		/*By CharlieL*/
	    } else
		outmsg("�L�k��H������");

	    me.page = 9999;
	    refresh();
	    sleep(1);
	    break;
	}

	if(me.level >= MANAGER) {
	    int page0 = me.page;

	    switch(ch) {
	    case 'n':
		a_newitem(&me, ADDITEM);
		me.page = 9999;
		break;
	    case 'g':
		a_newitem(&me, ADDGROUP);
		me.page = 9999;
		break;
	    case 'G':
		a_newitem(&me, ADDGOPHER);
		me.page = 9999;
		break;
	    case 'p':
		a_pasteitem(&me,1);
		me.page = 9999;
		break;
	    case 'f':
		a_editsign(&me);
		me.page = 9999;
		break;
	    case Ctrl('P'):
		a_pastetagpost(&me, -1);
		returnvalue = DIRCHANGED;  
		me.page = 9999;
		break;
	    case Ctrl('A'):
		a_pastetagpost(&me, 1);
		returnvalue = DIRCHANGED;
		me.page = 9999;
		break;
	    case 'a':
		a_appenditem(&me, 1);
		me.page = 9999;
		break;
	    default:
		me.page = page0;
		break;
	    }
	    
	    if(me.num)
		switch(ch) {
		case 'm':
		    a_moveitem(&me);
		    me.page = 9999;
		    break;

		case 'D':
		    /* Ptt me.page = -1;*/
		    a_delrange(&me);
		    me.page = 9999;
		    break;
		case 'd':
		    a_delete(&me);
		    me.page = 9999;
		    break;
                case 'H':
		    a_hideitem(&me);
		    me.page = 9999;
		    break;
		case 'T':
		    a_newtitle(&me);
		    me.page = 9999;
		    break;
		}
	}

	if(me.level == SYSOP) {
	    switch(ch) {
	    case 'l':
		a_newitem(&me, ADDLINK);
		me.page = 9999;
		break;
	    case 'N':
		a_showname(&me);
		me.page = 9999;
		break;
	    }
	}
    }
    free(me.header);
    return returnvalue;
}

static char *mytitle = BBSNAME "�G�i��";

int Announce() {
    setutmpmode(ANNOUNCE);
    a_menu(mytitle, "man",
	   ((HAS_PERM(PERM_SYSOP) || HAS_PERM(PERM_ANNOUNCE)) ? SYSOP :
	    NOBODY));
    return 0;
}
