/* $Id$ */
/*-------------------------------------------------------*/
/* go.c                         ( NTU FPG BBS Ver 1.00 ) */
/*-------------------------------------------------------*/
/* target : 圍棋                                         */
/* create : 01/09/00                                     */
/*-------------------------------------------------------*/

#include "bbs.h"
#include <sys/socket.h> 

#define BBLANK 	(0)  	/* 空白 */
#define BBLACK 	(1)  	/* 黑子 */
#define BWHITE 	(2)  	/* 白子 */

#define redoln() (void)0

#define BRDSIZ 	(19) 	/* 棋盤單邊大小 */

#define BGOTO(x, y)	move(20 - y , x * 2 + 4)

static unsigned char	go[BRDSIZ][BRDSIZ];
static unsigned char	l[BRDSIZ][BRDSIZ];
static unsigned char	ml[BRDSIZ][BRDSIZ];
static int	lib, mik, mjk, hik, hjk, mk, hk;
static float	win;

//extern 	char	*bw_chess[];	/* 和五子棋共用 */

static char	*locE = "ABCDEFGHJKLMNOPQRST";

static char	AB[41];

static unsigned char 	me, he, hand;

static Horder_t	*v, pool[500];

static void
GO_init(void)
{
    memset(pool, 0, sizeof(pool));
    v = pool;

    memset(go, 0, sizeof(go));
    memset( l, 0, sizeof(go));
    memset(ml, 0, sizeof(go));
}

static void
GO_add(Horder_t *mv)
{
    if (v < pool + 500) 
	*v++ = *mv;
}

static int
GO_sethand(int i)
{
    int	j;
    char	tmp[5];

    if (i > 0 && i < 10)
    {
	move(1, 71);
	prints("讓子：%d", i);   

	win = 0;
	go[3][3] = BBLACK;    
	if (i > 1)
	{
	    go[15][15] = BBLACK;
	    if (i > 2)
	    {
		go[3][15] = BBLACK;
		if (i > 3)
		{
		    go[15][3] = BBLACK;
		    if (i == 5)
			go[9][9] = BBLACK;
		    else
			if (i > 5)
			{
			    go[9][15] = BBLACK;
			    go[9][3] = BBLACK;
			    if (i == 7)
				go[9][9] = BBLACK;
			    else
				if (i > 7)
				{
				    go[15][9] = BBLACK;
				    go[3][9] = BBLACK;
				    if (i > 8) 
					go[9][9] = BBLACK;
				}
			}
		}
	    }
	}
    }
    else
	return 0;

    hand = i;
    *AB = 0;
    for (i = 0;i < 19;i++)
	for (j = 0;j < 19;j++)
	    if (go[i][j])
	    {
		BGOTO(i, j);
		outs(bw_chess[go[i][j] - 1]);
		redoln(); 
		sprintf(tmp, "[%c%c]", 'a' + i, 's' - j);
		strcat(AB, tmp);
	    }
    return 1;
}


static void
GO_count(int x, int y, int color)
{
    ml[x][y] = 0;

    if (x != 0)
    {
	if ((go[x - 1][y] == BBLANK) && ml[x - 1][y])
	{
	    ++lib;
	    ml[x - 1][y] = 0;
	}
	else
	    if ((go[x - 1][y] == color) && ml[x - 1][y])
		GO_count(x - 1, y, color);
    }
    if (x != 18)
    {
	if ((go[x + 1][y] == BBLANK) && ml[x + 1][y])
	{
	    ++lib;
	    ml[x + 1][y] = 0;
	}
	else
	    if ((go[x + 1][y] == color) && ml[x + 1][y])
		GO_count(x + 1, y, color);
    }
    if (y != 0)
    {
	if ((go[x][y - 1] == BBLANK) && ml[x][y - 1])
	{
	    ++lib;
	    ml[x][y - 1] = 0; 
	}
	else
	    if ((go[x][y - 1] == color) && ml[x][y - 1])
		GO_count(x, y - 1, color);
    }
    if (y != 18)
    {
	if ((go[x][y + 1] == BBLANK) && ml[x][y + 1])
	{
	    ++lib; 
	    ml[x][y + 1] = 0;
	}
	else
	    if ((go[x][y + 1] == color) && ml[x][y + 1])
		GO_count(x, y + 1, color);
    }
}

static void
GO_countlib(int x, int y, char color)
{
    int i, j;

    for (i = 0; i < 19; i++)
	for (j = 0; j < 19; j++)
	    ml[i][j] = 1;

    GO_count(x, y, color);
}

static void 
GO_eval(char color)
{
    int i, j;

    for (i = 0; i < 19; i++)
	for (j = 0; j < 19; j++)
	    if (go[i][j] == color)
	    {
		lib = 0;
		GO_countlib(i, j, color);
		l[i][j] = lib;
	    }
}

static int 
GO_check(Horder_t *mv)
{
    int m, n, k;

    lib = 0;
    GO_countlib(mv->x, mv->y, me);

    if (lib == 0)
    {
	go[(int)mv->x][(int)mv->y] = me;

	GO_eval(he);
	k = 0;

	for (m = 0; m < 19; m++)
	    for (n = 0; n < 19; n++)
		if ((go[m][n] == he) && !l[m][n]) ++k;

	if ((k == 0) || (k == 1 && ((mv->x == mik) && (mv->y == mjk))))
	{
	    go[(int)mv->x][(int)mv->y] = BBLANK;   /* restore to open */
	    return 0;
	}
	else
	    return 1;
    }
    else
	return 1;
}

static void
GO_blank(char x, char y)
{
    char *str = "┌┬┐├┼┤└┴┘";
    int n1, n2, loc;

    BGOTO(x, y);
    n1 = (x == 0) ? 0 : (x == 18) ? 2 : 1;
    n2 = (y == 18) ? 0 : (y == 0) ? 2 : 1;
    loc= 2 * (n2 * 3 + n1);
    prints("%.2s", str + loc);
    redoln();
} 

static void 
GO_examboard(char color)
{
    int i, j, n;

    GO_eval(color);

    if (color == he)
    {
	hik = -1;
	hjk = -1;
    }
    else
    {
	mik = -1;
	mjk = -1;
    }
    n = 0;

    for (i = 0; i < 19; i++)
	for (j = 0; j < 19; j++)
	    if ((go[i][j] == color) && (l[i][j] == 0))
	    {
		go[i][j] = BBLANK;
		GO_blank(i, j);
		if (color == he)
		{
		    hik = i;
		    hjk = j;
		    ++hk;
		}
		else
		{
		    mik = i;
		    mjk = j;
		    ++mk;
		}
		++n;
	    }

    if (color == he && n > 1)
    {
	hik = -1;
	hjk = -1;
    }
    else if ( n > 1 )
    {
	mik = -1;
	mjk = -1;
    }
}

static void
GO_clean(int fd, int x, int y, int color)
{
    Horder_t tmp;

    ml[x][y] = 0;

    go[x][y] = BBLANK;
    GO_blank(x, y);

    tmp.x = x;
    tmp.y = y;

    if (send(fd, &tmp, sizeof(Horder_t), 0) != sizeof(Horder_t))
	return;

    if (x != 0)
    {
	if ((go[x - 1][y] == color) && ml[x - 1][y])
	    GO_clean(fd, x - 1, y, color);
    }
    if (x != 18)
    {
	if ((go[x + 1][y] == color) && ml[x + 1][y])
	    GO_clean(fd, x + 1, y, color);
    }
    if (y != 0)
    {
	if ((go[x][y - 1] == color) && ml[x][y - 1])
	    GO_clean(fd, x, y - 1, color);
    }
    if (y != 18)
    { 
	if ((go[x][y + 1] == color) && ml[x][y + 1])
	    GO_clean(fd, x, y + 1, color);
    }
}

static void
GO_cleandead(int fd, char x, char y, char color)
{
    int i, j;

    for (i = 0; i < 19; i++)
	for (j = 0; j < 19; j++)
	    ml[i][j] = 1;

    GO_clean(fd, x, y, color);
}

static void
GO_log(char *userid)
{
    fileheader_t mymail;
    char         title[128], buf[80];
    int          i = 0;
    FILE        *fp;
    Horder_t *ptr = pool;
    //extern screenline *big_picture;

    sethomepath(buf, cuser.userid);
    stampfile(buf, &mymail);

    fp = fopen(buf, "w");
    for(i = 1; i < 21; i++)
	fprintf(fp, "%.*s\n", big_picture[i].len, big_picture[i].data + 1);

    i = 0;

    fprintf(fp, "\n");

    do
    {
	if (ptr->x == -1 || ptr->y == -1)
	    fprintf(fp, "[%3d]%s =>    %c", i + 1, win ? bw_chess[i % 2] : bw_chess[(i + 1) % 2],
		    (i % 5) == 4 ? '\n' : '\t');
	else
	    fprintf(fp, "[%3d]%s => %.1s%d%c", i + 1, win ? bw_chess[i % 2] : bw_chess[(i + 1) % 2],
		    locE + ptr->x, ptr->y + 1, (i % 5) == 4 ? '\n' : '\t');
	i++;

    } while (++ptr < v);

    fprintf(fp, "\n\n《以下為 sgf 格式棋譜》\n\n(;GM[1]");
    if (userid == NULL)
	fprintf(fp, "GN[Gobot-Gobot FPG]\n");
    else
	fprintf(fp, "GN[%s-%s(%c) FPG]\n", userid, cuser.userid, me == BBLACK ? 'B' : 'W');
    fprintf(fp, "SZ[19]HA[%d]", hand);
    if (userid == NULL)
    {
	fprintf(fp, "PB[Gobot]PW[Gobot]\n");
    }
    else
    {
	if (me == BBLACK)
	    fprintf(fp, "PB[%s]PW[%s]\n", cuser.userid, userid);
	else
	    fprintf(fp, "PB[%s]PW[%s]\n", userid, cuser.userid);
    }
    fprintf(fp, "PC[FPG BBS/Ptt BBS: ptt.cc]\n");

    if (win)
	i = 0;
    else
    {
	i = 1;
	fprintf(fp, "AB%s\n", AB);
    }
    ptr = pool;
    do
    {
	if (ptr->x == -1 || ptr->y == -1)
	    fprintf(fp, ";%c[]%c", i % 2 ? 'W' : 'B', (i % 10) == 9 ? '\n' : ' ');
	else
	    fprintf(fp, ";%c[%c%c]%c", i % 2 ? 'W' : 'B', 'a' + ptr->x, 's' - ptr->y, (i % 10) == 9 ? '\n' : ' ');

	i++;
    } while (++ptr < v);

    fprintf(fp, ";)\n\n");
    fclose(fp);

    mymail.filemode = FILE_READ;

    strcpy(mymail.owner, "[備.忘.錄]");
    if (userid == NULL)
    {
	strcpy(mymail.title, "圍棋打譜");
    }
    else
	snprintf(mymail.title, sizeof(mymail.title),
		"\033[37;41m棋譜\033[m %s VS %s", cuser.userid, userid);

    sethomedir(title, cuser.userid);
    append_record(title, &mymail, sizeof(mymail));
}

static int
go_key(int fd, int ch, Horder_t *mv)
{
    if (ch >= 'a' && ch <= 's' && ch != 'i')
    {
	char pbuf[4];
	int  vx, vy;

	pbuf[0] = ch;
	pbuf[1] = 0;

	if (fd) add_io(0, 0);
	getdata(21, 9, "直接指定位置(1 - 19)：", pbuf, 4, DOECHO);
	if (fd) add_io(fd, 0);
	move(21, 9);
	clrtoeol();
	outs("                               ");

	vx = ch - 'a';
	if (ch > 'i')
	    vx--;
	vy = atoi(pbuf) - 1;
	if( vx >= 0 && vx < BRDSIZ && vy >= 0 && vy < BRDSIZ && go[vx][vy] == BBLANK)
	{
	    mv->x = vx;
	    mv->y = vy;
	    return 1;
	}
    }
    else
    {
	switch(ch)
	{
	    case KEY_RIGHT:
		mv->x = (mv->x == BRDSIZ - 1) ? mv->x : mv->x + 1;
		break;
	    case KEY_LEFT:
		mv->x = (mv->x == 0 ) ? 0 : mv->x - 1;
		break;
	    case KEY_UP:
		mv->y = (mv->y == BRDSIZ - 1) ? mv->y : mv->y + 1;
		break;
	    case KEY_DOWN:
		mv->y = (mv->y == 0 ) ? 0 : mv->y - 1;
		break;
	    case ' ':
	    case '\r':
	    case '\n':
		if (go[(int)mv->x][(int)mv->y] == BBLANK && GO_check(mv))
		    return 1;
	}
    }
    return 0;
}

static unsigned char 
GO_findcolor(int x, int y)
{
    int k, result = 0, color[4];

    if (go[x][y] != BBLANK)
	return 0;

    if (x > 0)
    {
	k = x;
	do --k;
	while ((go[k][y] == BBLANK) && (k > 0));
	color[0] = go[k][y];
    }
    else 
	color[0] = go[x][y];

    if (x < 18)
    {
	k = x;
	do ++k;
	while ((go[k][y] == BBLANK) && (k < 18));
	color[1] = go[k][y];
    }
    else
	color[1] = go[x][y];

    if (y > 0)
    {
	k = y;
	do --k;
	while ((go[x][k] == BBLANK) && (k > 0));
	color[2] = go[x][k];
    }
    else color[2] = go[x][y];

    if (y < 18)
    {
	k = y;
	do ++k;
	while ((go[x][k] == BBLANK) && (k < 18));
	color[3] = go[x][k];
    }
    else
	color[3] = go[x][y];

    for (k = 0;k < 4;k++)
    {
	if (color[k] == BBLANK)
	    continue;
	else
	{
	    result = color[k];
	    break;
	}
    }

    for (k = 0;k < 4;k++)
    {
	if ((color[k] != BBLANK) && (color[k] != result))
	    return 0;
    }

    return result;
}

static int
GO_result(void)
{
    int i, j, result;
    float count[2];
    char    *chessresult[] = { "•", "。" }; 

    count[0] = count[1] = 0;

    for (i = 0; i < 19; i++)
	for (j = 0; j < 19; j++)
	    if (go[i][j] == BBLANK)
	    {
		result = GO_findcolor(i, j);
		BGOTO(i, j);
		if (result)
		{
		    outs(chessresult[result - 1]);
		    count[result - 1]++;
		}
		else
		{
		    outs("×");
		    win -= 0.5;	  
		}
		redoln();
	    }
	    else
		count[go[i][j] - 1]++;

    if (me == BBLACK)
    {
	move(5, 46);
	prints("%s 方目數：%-3.1f       ", bw_chess[me - 1], count[0]);
	move(6, 46);
	prints("%s 方目數：%-3.1f       ", bw_chess[he - 1], count[1]);
	move(21, 46);
	clrtoeol();
	move(8, 46);

	if (count[0] > 181 + win)
	    return 1;
    }
    else
    {
	move(5, 46);
	prints("%s 方目數：%-3.1f       ", bw_chess[me - 1], count[1]);
	move(6, 46);
	prints("%s 方目數：%-3.1f       ", bw_chess[he - 1], count[0]);
	move(21, 46);
	clrtoeol();
	move(8, 46);

	if (count[0] <= 181 + win)
	    return 1;
    }
    return 0;
}

void
GO_cleantable()
{
    int i, j;
    for(i = 0; i < BRDSIZ; ++i)
	for(j = 0; j < BRDSIZ; ++j)
	    GO_blank(i, j);
}

int
gochess(int fd)
{
    Horder_t    mv;
    userinfo_t *my = currutmp;
    time_t     mtime, htime, btime;
    int        i, j, ch = 0, passflag, endflag, totalgo, timeflag, is_view;
    unsigned char    mhand, hhand;

    GO_init();

    hk = mk = 0;
    totalgo = 0;
    hik = hjk = -1;
    mik = mjk = -1;
    mhand = hhand = 250;	/* 不太可能一分鐘內下這麼多手 :p */
    mtime = htime = 60;		/* 一開始的一分鐘內不計手 */
    win = 3.5;

    me = !(my->turn) + 1;
    he = my->turn + 1;
    if (me > 2) me = 2;
    if (he > 2) he = 2;

    endflag = passflag = timeflag = 0;
    is_view = 1;

    setutmpmode(GO);

    clear();

    /* 觀戰用程式碼, 與 ptt 不相容
    if (me == BWHITE)
    {
	VIEWCHESS	vs;

	memset(&vs, 0, sizeof(VIEWCHESS));     
	strcpy(vs.userid[0], cuser.userid);
	strcpy(vs.userid[1], my->mateid);
	if (cshm_new(&vs) == -1)
	    is_view = 0;
    }
    */

    prints("\033[1;46m  圍棋對戰  \033[45m%31s VS %-31s\033[m",
	    cuser.userid, my->mateid);
    GO_cleantable();

    /* film_out(FILM_GO, 1); */

    add_io(fd, 0);

    mv.x = mv.y = 9;
    btime = now;

    for(;;)
    {
	if (endflag || timeflag)
	{
	    move(5, 46);
	    prints("%s 方時間：----- --", bw_chess[me - 1]);
	    move(6, 46);
	    prints("%s 方時間：----- --", bw_chess[he - 1]);
	}
	else
	{
	    if (my->turn)
	    {
		mtime -= now - btime;
		if (mtime < 0)
		{
		    if (mhand > 25)		/* 第一分鐘過後，要在 12 分鐘內下 25 手 */
		    {
			mtime = 12 * 60;
			mhand = 25;
		    }
		    else
		    {
			mv.x = mv.y = -8;
			if (send(fd, &mv, sizeof(Horder_t), 0) != sizeof(Horder_t))
			    break;
			move(8, 46);
			outs("未在時間內完成 25 手，裁定敗");
			break;
		    }
		}
	    }
	    else
	    {
		htime -= now - btime;
		if (htime < 0)
		{
		    if (hhand > 25)         /* 第一分鐘過後，要在 12 分鐘內下 25 手 */
		    {
			htime = 12 * 60;
			hhand = 25;
		    }
		    else
			htime = 0;
		}
	    }
	    move(5, 46);
	    prints("%s 方時間：%02ld:%02ld ", bw_chess[me - 1], mtime / 60, mtime % 60);
	    if (mhand <= 25)
		prints("%2d 手", 25 - mhand);
	    move(6, 46);
	    prints("%s 方時間：%02ld:%02ld ", bw_chess[he - 1], htime / 60, htime % 60);
	    if (hhand <= 25)
		prints("%2d 手", 25 - hhand); 
	}
	btime = now;
	move(2, 46);
	prints("%s 方提子數：%3d", bw_chess[me - 1], hk);
	move(3, 46);
	prints("%s 方提子數：%3d", bw_chess[he - 1], mk);

	move(8, 46);
	clrtoeol();
	if (endflag)
	    outs("請清除死子，以便計算勝負");
	else if (my->turn)
	    prints("輪到自己下了.... 我是 %s", bw_chess[me - 1]);
	else
	    outs("等待對方下子....");


	move(10, 46);
	if (totalgo > 0)
	{
	    if (pool[totalgo - 1].x == -1 || pool[totalgo - 1].y == -1)
		prints("%s #%-3d PASS 上一手    ", win ? bw_chess[(totalgo - 1) % 2] : bw_chess[totalgo % 2], totalgo);
	    else
		prints("%s #%-3d %.1s%-2d  上一手    ", win ? bw_chess[(totalgo - 1) % 2] : bw_chess[totalgo % 2], totalgo, locE + pool[totalgo - 1].x, pool[totalgo - 1].y + 1);
	}

	for (i = totalgo - 1;i > 0 && totalgo - i <= 10;i--)
	{
	    move(10 + totalgo - i, 46);
	    if (pool[i - 1].x == -1 || pool[i - 1].y == -1)
		prints("%s #%-3d PASS", win ? bw_chess[(i - 1) & 1] : bw_chess[i & 1], i);
	    else
		prints("%s #%-3d %.1s%-2d ", win ? bw_chess[(i - 1) & 1] : bw_chess[i & 1], i, locE + pool[i - 1].x, pool[i - 1].y + 1);
	}

	move(21, 46);

	if (v == pool)
	{
	    if (me == BWHITE && win != 0)
		outs("\033[1;33m按 x 讓子 y 不限時 Ctrl-C 中止棋局\033[m");
	    else
		outs("\033[1;33m按 Ctrl-C 中止棋局\033[m");
	}
	else if (passflag && my->turn)
	{
	    if (endflag)
		outs("\033[1;33m對方 DONE，己方 DONE 就計算結果\033[m");
	    else
		outs("\033[1;33m對方 PASS，己方 PASS 就結束棋局\033[m");
	}
	else if (v > pool)
	    clrtoeol();

	if (endflag)
	    outmsg("\033[1;33;42m 下棋 \033[;31;47m (←↑↓→)\033[30m移動 \033[31m(空白鍵/ENTER)\033[30m下子 " /*\033[31m(v)\033[30m傳訊 */ "\033[31m(z)\033[30m投降 \033[31m(w)\033[30mDONE \033[31m(u)\033[30m回復      \033[m");
	else
	    outmsg("\033[1;33;42m 下棋 \033[;31;47m (←↑↓→)\033[30m移動 \033[31m(空白鍵/ENTER)\033[30m下子 " /*\033[31m(v)\033[30m傳訊 */ "\033[31m(z)\033[30m投降 \033[31m(w)\033[30mPASS              \033[m");

	BGOTO(mv.x, mv.y);

	/*
	if (ch == -1 && is_view)
	    cshm_update(mv.x, mv.y);
	*/

	ch = igetch();

	/* 對戰中丟水球, 暫不支援
	if (ch == 'v')
	{
	    //extern screenline *big_picture;
	    screenline* screen0 = calloc(t_lines, sizeof(screenline));
	    int y, x;

	    memcpy(screen0, big_picture, t_lines * sizeof(screenline));
	    add_io(0, 0);
	    getyx(&y, &x);
	    if (ch == 'v')
	    {
		extern char   watermode;

		watermode = 2;
		my_write(my->destuip->pid, "丟水球過去：");
		watermode = 0;
	    }
	    else
	    {
		show_last_call_in();
		my_write(currutmp->msgs[0].last_pid, "水球丟回去：");
	    }
	    move(y, x);
	    memcpy(big_picture, screen0, t_lines * sizeof(screenline));
	    free(screen0);
	    redoscr();
	    add_io(fd, 0);
	    continue;
	} else */ if (ch == 'x') {
	    if (v == pool && me == BWHITE && win != 0)
	    {
		char buf[4];

		add_io(0, 0);
		getdata(21, 46, "要讓多少子呢(1 - 9)？ ", buf, 4, DOECHO);
		add_io(fd, 0);

		if (GO_sethand(atoi(buf)))
		{
		    mv.x = -9;
		    mv.y = atoi(buf);
		    if (send(fd, &mv, sizeof(Horder_t), 0) != sizeof(Horder_t))
			break;
		    mv.x = mv.y = 9; 
		    my->turn = 1;
		    ch = -1;
		}
		continue;
	    }
	}
	else if (ch == 'y')
	{
	    if (v == pool && me == BWHITE)
	    {
		timeflag = 1;
		mv.x = mv.y = -10;
		if (send(fd, &mv, sizeof(Horder_t), 0) != sizeof(Horder_t))
		    break;
		mv.x = mv.y = 9;
		ch = -1; 
	    }
	}
	else if (ch == 'w')
	{
	    if (endflag)
	    {
		Horder_t tmp;

		if (passflag && my->turn)
		{
		    tmp.x = tmp.y = -3;
		    if (send(fd, &tmp, sizeof(Horder_t), 0) != sizeof(Horder_t))
			break;

		    if (GO_result())
			outs("恭禧，您獲得勝利....      ");
		    else
			outs("輸了，再接再勵呦....      ");

		    if (me == BWHITE)
			ch = -1;

		    break;
		}
		else
		{
		    Horder_t tmp;

		    passflag = 1;
		    tmp.x = tmp.y = -6;
		    if (send(fd, &tmp, sizeof(Horder_t), 0) != sizeof(Horder_t))
			break;
		    my->turn = 0;

		    if (me == BWHITE)
			ch = -1;

		    continue;
		}
	    }
	    else
	    {
		if (my->turn)
		{
		    if (passflag)
		    { 
			endflag = 1;
			passflag = 0;
			mv.x = mv.y = -2;
			if (send(fd, &mv, sizeof(Horder_t), 0) != sizeof(Horder_t))
			    break;
			mv.x = mv.y = 9;
			memcpy(l, go, sizeof(l));	/* 備份 */

			if (me == BWHITE)
			    ch = -1;
			continue;
		    }
		    else
		    {
			passflag = 1;
			mv.x = mv.y = -1;
			if (send(fd, &mv, sizeof(Horder_t), 0) != sizeof(Horder_t))
			    break;
			GO_add(&mv);
			totalgo++;
			mv.x = mv.y = 9; 
			my->turn = 0;

			if (me == BWHITE)
			    ch = -1;
			continue;
		    }
		}
	    }
	}
	else if (ch == Ctrl('C')/* && v == pool */)
	{
	    mv.x = mv.y = -5;
	    if (send(fd, &mv, sizeof(Horder_t), 0) != sizeof(Horder_t))
		break;
	    break;
	}
	else if (ch == 'u')
	{
	    if (endflag)
	    {
		memcpy(go, l, sizeof(go));

		for(i = 0;i < 19;i++)
		    for(j = 0;j < 19;j++)
		    {
			BGOTO(i, j);
			if (go[i][j])
			{
			    outs(bw_chess[go[i][j] - 1]);
			    redoln();   	      
			}
			else
			    GO_blank(i, j);
		    }
		mv.x = mv.y = -7;
		if (send(fd, &mv, sizeof(Horder_t), 0) != sizeof(Horder_t))
		    break;
		mv = *(v - 1);

		if (me == BWHITE)
		    ch = -1;
	    }	
	}
	else if (ch == 'z')
	{
	    mv.x = mv.y = -4;
	    if (send(fd, &mv, sizeof(Horder_t), 0) != sizeof(Horder_t))
		break;
	    move(8, 46);
	    outs("您認輸了....             ");
	    break;
	}

	if (ch == I_OTHERDATA)
	{
	    ch = recv(fd, &mv, sizeof(Horder_t), 0);

	    if (ch <= 0)
	    {
		move(8, 46);
		outs("對方斷線....            ");
		break;
	    }

	    if (ch != sizeof(Horder_t))
	    {
		continue;
	    }

	    if (mv.x == -9 && mv.y > 0)
	    {
		GO_sethand(mv.y);
		mv.x = mv.y = 9;
		my->turn = 0;
		continue;
	    }

	    if (mv.x == -10 && mv.y == -10)
	    {
		timeflag = 1;
		mv.x = mv.y = 9;
		continue;
	    }

	    if (mv.x == -5 && mv.y == -5)
		break;

	    if (mv.x == -4 && mv.y == -4)
	    {
		move(8, 46);
		outs("對方認輸....      ");
		break;
	    }

	    if (mv.x == -8 && mv.y == -8)
	    {
		move(8, 46);
		outs("對方未在時間內完成 25 手，裁定勝");
		break;
	    } 

	    if (mv.x == -3 && mv.y == -3)
	    {
		if (GO_result())
		    outs("恭禧，您獲得勝利....      ");
		else
		    outs("輸了，再接再勵呦....      "); 
		break;

		if (me == BWHITE)
		    ch = -1;
	    }

	    if (mv.x == -2 && mv.y == -2)
	    {
		endflag = 1;
		passflag = 0;
		mv.x = mv.y = 9;
		my->turn = 1;
		memcpy(l, go, sizeof(l));	/* 備份 */

		if (me == BWHITE)
		    ch = -1;
		continue;
	    } 

	    if (mv.x == -7 && mv.y == -7)
	    {
		memcpy(go, l, sizeof(go));

		for(i = 0;i < 19;i++)
		    for(j = 0;j < 19;j++)
		    {
			BGOTO(i, j);
			if (go[i][j])
			{
			    outs(bw_chess[go[i][j] - 1]);
			    redoln();
			}
			else
			    GO_blank(i, j);
		    }
		mv = *(v - 1);

		if (me == BWHITE)
		    ch = -1;

		continue;
	    }

	    if (mv.x == -6 && mv.y == -6)
	    {
		passflag = 1;
		mv = *(v - 1);
		my->turn = 1;

		if (me == BWHITE)
		    ch = -1;

		continue;
	    }

	    if (mv.x == -1 && mv.y == -1)
	    {
		GO_add(&mv);
		totalgo++;
		mv.x = mv.y = 9;

		passflag = 1;
		my->turn = 1;

		if (me == BWHITE)
		    ch = -1;

		continue;
	    }

	    if (endflag)
	    {
		int y, x;

		getyx(&y, &x);

		go[(int)mv.x][(int)mv.y] = BBLANK;
		GO_blank(mv.x, mv.y);
		/*
		   hk++;
		   */
		mv.x = (x - 4) / 2;
		mv.y = 20 - y;
		BGOTO(mv.x, mv.y);

		if (me == BWHITE)
		    ch = -1;	
		continue;
	    }

	    if (!my->turn)
	    {
		GO_add(&mv);

		totalgo++;
		if (--hhand <= 0)
		{
		    htime = 12 * 60;
		    hhand = 25;
		}
		go[(int)mv.x][(int)mv.y] = he;
		BGOTO(mv.x, mv.y);
		outs(bw_chess[he - 1]);
		redoln();

		GO_examboard(me);
		my->turn = 1;
		htime -= time(0) - btime;
		btime = time(0);
		passflag = 0;

		if (me == BWHITE)
		    ch = -1;
	    }
	    continue;
	}

	if (endflag == 1 && (ch == ' ' || ch == '\r' || ch == '\n')) {
	    if (go[(int)mv.x][(int)mv.y] == me)
		GO_cleandead(fd, mv.x, mv.y, me);

	    if (me == BWHITE)
		ch = -1;
	    continue;
	}

	if (my->turn)
	{
	    if (go_key(fd, ch, &mv))
		my->turn = 0;
	    else
		continue;

	    if (!my->turn)
	    {
		GO_add(&mv);

		totalgo++;
		mtime -= time(0) - btime;
		btime = time(0);
		if (--mhand <= 0)
		{
		    mtime = 12 * 60;
		    mhand = 25; 
		}
		BGOTO(mv.x, mv.y);
		outs(bw_chess[me - 1]);
		redoln();
		go[(int)mv.x][(int)mv.y] = me;
		GO_examboard(he);
		if (send(fd, &mv, sizeof(Horder_t), 0) != sizeof(Horder_t))
		    break;
		passflag = 0;

		if (me == BWHITE)
		    ch = -1;
	    }
	}
    }

    add_io(0, 0);
    close(fd);

    igetch();

    /*
    if (me == BWHITE && is_view)
	cshm_free();
    */

    if (endflag)
    {
	for(i = 0;i < 19;i++)
	    for(j = 0;j < 19;j++)
	    {
		BGOTO(i, j);
		if (l[i][j])
		{
		    outs(bw_chess[l[i][j] - 1]);
		    redoln();
		}
		else
		    GO_blank(i, j);
	    } 
    }

    if (v > pool + 10)
    {
	char ans[3];

	getdata(b_lines - 1, 0, "要存成棋譜嗎(Y/N)？[Y] ", ans, 3, LCECHO);

	if (*ans != 'n')
	    GO_log(my->mateid);
    }

    return 0;
}

int
GoBot(void)
{
    Horder_t mv;
    int i, ch, totalgo;
    unsigned char  tmp_go[2][BRDSIZ][BRDSIZ];
    unsigned char  tmp_l[2][BRDSIZ][BRDSIZ];
    unsigned char  tmp_ml[2][BRDSIZ][BRDSIZ];
    int     tmp_lib[2], tmp_mik[2], tmp_mjk[2], tmp_hik[2], tmp_hjk[2];

    GO_init();

    memset(&tmp_go, 0, sizeof(tmp_go));
    memset(&tmp_l, 0, sizeof(tmp_l));
    memset(&tmp_ml, 0, sizeof(tmp_ml));
    for (i = 0;i < 2;i++)
    {
	tmp_lib[i] = 0;
	tmp_mik[i] = tmp_mjk[i] = tmp_hik[i] = tmp_hjk[i] = -1;
    }


    me = BBLACK;
    he = BWHITE;
    hand = 0;
    totalgo = 0;
    hik = hjk = -1;
    mik = mjk = -1; 

    setutmpmode(GO);

    clear();

    prints("\033[1;46m  圍棋打譜  \033[45m%66s\033[m", " ");
    GO_cleantable();

    /* film_out(FILM_GO, 1); */

    mv.x = mv.y = 9;

    for(;;)
    {
	move(8, 46);
	prints("輪到 %s 方下了....", bw_chess[me - 1]);

	move(10, 46);
	if (totalgo > 0)
	{
	    if (pool[totalgo - 1].x == -1 || pool[totalgo - 1].y == -1)
		prints("%s #%-3d PASS 上一手    ", bw_chess[(totalgo - 1) & 1], totalgo);
	    else
		prints("%s #%-3d %.1s%-2d  上一手    ", bw_chess[(totalgo - 1) & 1], totalgo, locE + pool[totalgo - 1].x, pool[totalgo - 1].y + 1);
	}

	for (i = totalgo - 1;i > 0 && totalgo - i <= 10;i--)
	{
	    move(10 + totalgo - i, 46);
	    if (pool[i - 1].x == -1 || pool[i - 1].y == -1)
		prints("%s #%-3d PASS", bw_chess[(i - 1) % 2], i);
	    else
		prints("%s #%-3d %.1s%-2d ", bw_chess[(i - 1) % 2], i, locE + pool[i - 1].x, pool[i - 1].y + 1);
	}

	outmsg("[1;33;42m 打譜 [;31;47m (←↑↓→)[30m移動 [31m(空白鍵/ENTER)[30m下子 [30m[31m(u)[30m回上一步 \033[31m(z)\033[30m離開                  [m");    

	BGOTO(mv.x, mv.y);

	ch = igetch();

	if (ch == 'z')
	    break;
	else if (ch == 'u')
	{
	    memset(go, 0, sizeof(go));
	    memset( l, 0, sizeof(go));
	    memset(ml, 0, sizeof(go));
	    memset(&tmp_go, 0, sizeof(tmp_go));
	    memset(&tmp_l, 0, sizeof(tmp_l));
	    memset(&tmp_ml, 0, sizeof(tmp_ml));
	    for (i = 0;i < 2;i++)
	    {
		tmp_lib[i] = 0;
		tmp_mik[i] = tmp_mjk[i] = tmp_hik[i] = tmp_hjk[i] = -1;
	    }
	    me = BBLACK;
	    he = BWHITE; 
	    hik = hjk = -1;
	    mik = mjk = -1; 

	    /* film_out(FILM_GO, 1); */
	    totalgo--;
	    GO_cleantable();
	    for (i = 0;i < totalgo;i++)
	    {
		BGOTO(pool[i].x, pool[i].y);
		outs(bw_chess[me - 1]);
		redoln();
		go[(int)pool[i].x][(int)pool[i].y] = me;
		GO_examboard(he);
		memcpy(&tmp_go[me - 1], &go, sizeof(l));
		memcpy(&tmp_l[me - 1], &l, sizeof(l));
		memcpy(&tmp_ml[me - 1], &ml, sizeof(ml));
		tmp_lib[me - 1] = lib;
		tmp_mik[me - 1] = mik;
		tmp_mjk[me - 1] = mjk;
		tmp_hik[me - 1] = hik;
		tmp_hjk[me - 1] = hjk;
		me = he;
		he = 3 - me;
		memcpy(&go, &tmp_go[me - 1], sizeof(l));
		memcpy(&l, &tmp_l[me - 1], sizeof(l));
		memcpy(&ml, &tmp_ml[me - 1], sizeof(ml));
		lib = tmp_lib[me - 1];
		mik = tmp_mik[me - 1];
		mjk = tmp_mjk[me - 1];
		hik = tmp_hik[me - 1];
		hjk = tmp_hjk[me - 1];
		go[(int)pool[i].x][(int)pool[i].y] = he;
		GO_examboard(me); 
	    }	
	    mv = *(--v);
	}
	else 
	{
	    if (go_key(0, ch, &mv))
	    {
		GO_add(&mv);
		totalgo++;
		BGOTO(mv.x, mv.y);
		outs(bw_chess[me - 1]); 
		redoln();
		go[(int)mv.x][(int)mv.y] = me;
		GO_examboard(he);
		memcpy(&tmp_go[me - 1], &go, sizeof(l));
		memcpy(&tmp_l[me - 1], &l, sizeof(l));
		memcpy(&tmp_ml[me - 1], &ml, sizeof(ml));
		tmp_lib[me - 1] = lib;
		tmp_mik[me - 1] = mik;
		tmp_mjk[me - 1] = mjk;
		tmp_hik[me - 1] = hik;
		tmp_hjk[me - 1] = hjk;
		me = he;
		he = 3 - me;
		memcpy(&go, &tmp_go[me - 1], sizeof(l));
		memcpy(&l, &tmp_l[me - 1], sizeof(l));
		memcpy(&ml, &tmp_ml[me - 1], sizeof(ml));
		lib = tmp_lib[me - 1];
		mik = tmp_mik[me - 1];
		mjk = tmp_mjk[me - 1];
		hik = tmp_hik[me - 1];
		hjk = tmp_hjk[me - 1];
		go[(int)mv.x][(int)mv.y] = he;
		GO_examboard(me);
	    }
	}
    }

    if (v > pool)
    {
	char ans[3];

	getdata(b_lines - 1, 0, "要存成棋譜嗎(Y/N)？[Y] ", ans, 3, LCECHO);

	if (*ans != 'n')
	    GO_log(NULL);
    } 

    return 0;
}
