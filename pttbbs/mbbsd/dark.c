/* $Id$ */
#include "bbs.h"

#define RED   1
#define BLACK 0
typedef short int sint;

typedef struct item {
    short int       color, value, die, out;
}               item;

typedef struct cur {
    short int       y, x, end;
}               cur;

struct DarkData {
    item     brd[4][8];
    cur      curr;
    sint     rcount, bcount, cont, fix;	/* cont:�O�_�i�s�Y */
    sint     my, mx, mly, mlx;		/* ���ʪ��y�� �� */

    sint     cur_eaty, cur_eatx;	/* �Y������l���q�X�y�� */
};

static char    * const rname[] = {"�L", "��", "�X", "��", "��", "�K", "��"};
static char    * const bname[] = {"��", "�]", "��", "��", "�H", "�h", "�N"};

static const sint     cury[] = {3, 5, 7, 9}, curx[] = {5, 9, 13, 17, 21, 25, 29, 33};

static void
brdswap(struct DarkData *dd, sint y, sint x, sint ly, sint lx)
{
    memcpy(&dd->brd[y][x], &dd->brd[ly][lx], sizeof(item));
    dd->brd[ly][lx].die = 1;
    dd->brd[ly][lx].color = -1;	/* �S�o��color */
    dd->brd[ly][lx].value = -1;
}

static          sint
Is_win(struct DarkData *dd, item att, item det, sint y, sint x, sint ly, sint lx)
{
    sint            i, c = 0, min, max;
    if (att.value == 1) {	/* �� */
	if (y != ly && x != lx)
	    return 0;
	if ((abs(ly - y) == 1 && dd->brd[y][x].die == 0) ||
	    (abs(lx - x) == 1 && dd->brd[y][x].die == 0))
	    return 0;
	if (y == ly) {
	    if (x > lx) {
		max = x;
		min = lx;
	    } else {
		max = lx;
		min = x;
	    }
	    for (i = min + 1; i < max; i++)
		if (dd->brd[y][i].die == 0)
		    c++;
	} else if (x == lx) {
	    if (y > ly) {
		max = y;
		min = ly;
	    } else {
		max = ly;
		min = y;
	    }
	    for (i = min + 1; i < max; i++)
		if (dd->brd[i][x].die == 0)
		    c++;
	}
	if (c != 1)
	    return 0;
	if (det.die == 1)
	    return 0;
	return 1;
    }
    /* �D�� */
    if (((abs(ly - y) == 1 && x == lx) || (abs(lx - x) == 1 && ly == y)) && dd->brd[y][x].out == 1) {
	if (att.value == 0 && det.value == 6)
	    return 1;
	else if (att.value == 6 && det.value == 0)
	    return 0;
	else if (att.value >= det.value)
	    return 1;
	else
	    return 0;
    }
    return 0;
}

static          sint
Is_move(struct DarkData *dd, sint y, sint x, sint ly, sint lx)
{
    if (dd->brd[y][x].die == 1 && ((abs(ly - y) == 1 && x == lx) || (abs(lx - x) == 1 && ly == y)))
	return 1;
    return 0;
}

static void
brd_rand(struct DarkData *dd)
{
    sint            y, x, index;
    sint            tem[32];
    sint            value[32] = {0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6,
    0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6};

    bzero(dd->brd, sizeof(dd->brd));
    bzero(tem, sizeof(tem));
    bzero(&dd->curr, sizeof(dd->curr));
    for (y = 0; y < 4; y++)
	for (x = 0; x < 8; x++)
	    while (1) {
		index = random() % 32;
		if (tem[index])
		    continue;
		dd->brd[y][x].color = (index > 15) ? 0 : 1;
		dd->brd[y][x].value = value[index];
		tem[index] = 1;
		break;
	    }
}

static void
brd_prints(void)
{
    clear();
    move(1, 0);
    outs("\n"
	 "   [43;30m�~�w�s�w�s�w�s�w�s�w�s�w�s�w�s�w��[m\n"
	 "   [43;30m�x���x���x���x���x���x���x���x���x[m\n"
	 "   [43;30m�u�w�q�w�q�w�q�w�q�w�q�w�q�w�q�w�t[m\n"
	 "   [43;30m�x���x���x���x���x���x���x���x���x[m\n"
	 "   [43;30m�u�w�q�w�q�w�q�w�q�w�q�w�q�w�q�w�t[m\n"
	 "   [43;30m�x���x���x���x���x���x���x���x���x[m\n"
	 "   [43;30m�u�w�q�w�q�w�q�w�q�w�q�w�q�w�q�w�t[m\n"
	 "   [43;30m�x���x���x���x���x���x���x���x���x[m\n"
	 "   [43;30m���w�r�w�r�w�r�w�r�w�r�w�r�w�r�w��[m\n"
	 "   ");
}

static void
draw_line(struct DarkData *dd, sint y, sint f)
{
    sint            i;
    char            buf[1024], tmp[256];

    *buf = 0;
    *tmp = 0;
    strlcpy(buf, "\033[43;30m", sizeof(buf));
    for (i = 0; i < 8; i++) {
	if (dd->brd[y][i].die == 1)
	    snprintf(tmp, sizeof(tmp), "�x  ");
	else if (dd->brd[y][i].out == 0)
	    snprintf(tmp, sizeof(tmp), "�x��");
	else {
	    snprintf(tmp, sizeof(tmp), "�x\033[%s1;%dm%s\033[m\033[43;30m",
		     (f == i) ? "1;47;" : "", (dd->brd[y][i].color) ? 31 : 34,
		     (dd->brd[y][i].color) ? rname[dd->brd[y][i].value] :
		     bname[dd->brd[y][i].value]);
	}
	strcat(buf, tmp);
    }
    strcat(buf, "�x\033[m");

    move(cury[y], 3);
    clrtoeol();
    outs(buf);
}

static void
redraw(struct DarkData *dd)
{
    sint            i = 0;
    for (; i < 4; i++)
	draw_line(dd, i, -1);
}

static          sint
playing(struct DarkData *dd, sint fd, sint color, sint ch, sint * b, userinfo_t * uin)
{
    dd->curr.end = 0;
    move(cury[dd->my], curx[dd->mx]);

    if (dd->fix) {
	if (ch == 's') {
	    dd->fix = 0;
	    *b = 0;
	    return 0;
	} else {
	    draw_line(dd, dd->mly, -1);
	}
    }
    switch (ch) {
    case KEY_LEFT:
	if (dd->mx == 0)
	    dd->mx = 7;
	else
	    dd->mx--;
	move(cury[dd->my], curx[dd->mx]);
	*b = -1;
	break;
    case KEY_RIGHT:
	if (dd->mx == 7)
	    dd->mx = 0;
	else
	    dd->mx++;
	move(cury[dd->my], curx[dd->mx]);
	*b = -1;
	break;
    case KEY_UP:
	if (dd->my == 0)
	    dd->my = 3;
	else
	    dd->my--;
	move(cury[dd->my], curx[dd->mx]);
	*b = -1;
	break;
    case KEY_DOWN:
	if (dd->my == 3)
	    dd->my = 0;
	else
	    dd->my++;
	move(cury[dd->my], curx[dd->mx]);
	*b = -1;
	break;
    case 'q':
    case 'Q':
	if (!color)
	    dd->bcount = 0;
	else
	    dd->rcount = 0;
	*b = 0;
	return -2;
    case 'p':
    case 'P':
	return -3;
    case 'c':
	return -4;
    case 'g':
	return -5;
    case 's':			/* ½�}�Ѥl �άO��ܴѤl */
	/* ��ܴѤl */
	if (dd->brd[dd->my][dd->mx].out == 1) {
	    if (dd->brd[dd->my][dd->mx].color != color) {
		*b = -1;
		break;
	    }
	    if (dd->mly < 0) {	/* �i�H��� */
		dd->mly = dd->my;
		dd->mlx = dd->mx;
		draw_line(dd, dd->my, dd->mx);
		*b = -1;
		break;
	    } else if (dd->mly == dd->my && dd->mlx == dd->mx) {	/* ����F */
		dd->mly = -1;
		dd->mlx = -1;
		draw_line(dd, dd->my, -1);
	    } else {
		draw_line(dd, dd->mly, -1);
		dd->mly = dd->my;
		dd->mlx = dd->mx;
		if (dd->brd[dd->mly][dd->mlx].value == 1)
		    dd->fix = 1;
		draw_line(dd, dd->my, dd->mx);
	    }
	    *b = -1;
	    break;
	}
	/* ½�}�Ѥl */
	if (dd->mly >= 0) {
	    *b = -1;
	    break;
	}			/* ���ӴN�O½�}�� */
	/* �M�w�@�}�l���C�� */
	if (currutmp->color == '.') {
	    if (uin->color != '1' && uin->color != '0')
		currutmp->color = (dd->brd[dd->my][dd->mx].color) ? '1' : '0';
	    else
		currutmp->color = (uin->color == '0') ? '1' : '0';
	}
	dd->brd[dd->my][dd->mx].out = 1;
	draw_line(dd, dd->my, -1);
	move(cury[dd->my], curx[dd->mx]);
	*b = 0;
	break;
    case 'u':
	move(0, 0);
	clrtoeol();
	prints("%s��%s cont=%d", (dd->brd[dd->my][dd->mx].color == RED) ? "��" : "��", rname[dd->brd[dd->my][dd->mx].value], dd->cont);
	*b = -1;
	break;
    case '\r':			/* �Y or ����  ly��lx�����j��0 */
    case '\n':
	if (
	    dd->mly >= 0		/* �n����l */
	    &&
	    dd->brd[dd->mly][dd->mlx].color != dd->brd[dd->my][dd->mx].color	/* �P�⤣�ಾ�ʤ]����Y */
	    &&
	    (Is_move(dd, dd->my, dd->mx, dd->mly, dd->mlx) || Is_win(dd, dd->brd[dd->mly][dd->mlx], dd->brd[dd->my][dd->mx], dd->my, dd->mx, dd->mly, dd->mlx))
	    ) {
	    if (dd->fix && dd->brd[dd->my][dd->mx].value < 0) {
		*b = -1;
		return 0;
	    }
	    if (dd->brd[dd->my][dd->mx].value >= 0 && dd->brd[dd->my][dd->mx].die == 0) {
		if (!color)
		    dd->bcount--;
		else
		    dd->rcount--;
		move(dd->cur_eaty, dd->cur_eatx);
		outs((color) ? bname[dd->brd[dd->my][dd->mx].value] : rname[dd->brd[dd->my][dd->mx].value]);
		if (dd->cur_eatx >= 26) {
		    dd->cur_eatx = 5;
		    dd->cur_eaty++;
		} else
		    dd->cur_eatx += 3;
	    }
	    brdswap(dd, dd->my, dd->mx, dd->mly, dd->mlx);
	    draw_line(dd, dd->mly, -1);
	    draw_line(dd, dd->my, -1);
	    if (dd->fix == 1)
		*b = -1;
	    else {
		dd->mly = -1;
		dd->mlx = -1;
		*b = 0;
	    }
	} else
	    *b = -1;
	break;
    default:
	*b = -1;
    }

    if (!dd->rcount)
	return -1;
    else if (!dd->bcount)
	return -1;
    if (*b == -1)
	return 0;
    dd->curr.y = dd->my;
    dd->curr.x = dd->mx;
    dd->curr.end = (!*b) ? 1 : 0;
    send(fd, &dd->curr, sizeof(dd->curr), 0);
    send(fd, &dd->brd, sizeof(dd->brd), 0);
    return 0;
}

int
main_dark(int fd, userinfo_t * uin)
{
    sint            end = 0, ch = 1, i = 0;
    char            buf[16];
    struct DarkData dd;

    memset(&dd, 0, sizeof(dd));
    dd.my=dd.mx=0;
    dd.mly=dd.mlx=-1;

    *buf = 0;
    dd.fix = 0;
    currutmp->color = '.';
    /* '.' ����٨S�M�w�C�� */
    dd.rcount = 16;
    dd.bcount = 16;
    //initialize
    dd.cur_eaty = 18, dd.cur_eatx = 5;
    setutmpmode(DARK);
    brd_prints();
    if (currutmp->turn) {
	brd_rand(&dd);
	send(fd, &dd.brd, sizeof(dd.brd), 0);
	mouts(21, 0, "   [1;37m[1;33m��[1;37m�A�O����[m");
	mouts(22, 0, "   [1;33m��[5;35m����A�U�F[m");
    } else {
	recv(fd, &dd.brd, sizeof(dd.brd), 0);
	mouts(21, 0, "   [1;33m��[1;37m�A�O���[m");
    }
    move(12, 3);
    prints("%s[0��0��][5;31m����[1;37m.[m%s[0��0��]", currutmp->userid, currutmp->mateid);
    outs("\n"
	 "                                                [1;36m����[1;31m�\\���[1;36m������������[m\n"
	 "                                                [1;36m��[1;33m ��������[1;37m: [1;35m����[m\n"
	 "                                                [1;36m��[1;33m ��[1;37m: [1;35m      ��l,½�l[m\n"
	 "                                                [1;36m��[1;33m enter[1;37m: [1;35m   �Y��,���[m\n"
	 "�@[1;33m�w�g�ѨM��[1;37m:[1;36m�@�@                               ��[1;33m ��[1;37m: [1;35m      �X��[m\n"
	 "                                       �@�@     [1;36m��[1;33m ��[1;37m: [1;35m      �{��[m\n"
	 "                                                [1;36m��[1;33m ��[1;37m: [1;35m      ����[m");

    if (currutmp->turn)
	move(cury[0], curx[0]);

    add_io(fd, 0);
    while (end <= 0) {
	if (uin->turn == 'w' || currutmp->turn == 'w') {
	    end = -1;
	    break;
	}
	ch = igetch();
	if (ch == I_OTHERDATA) {
	    ch = recv(fd, &dd.curr, sizeof(dd.curr), 0);
	    if (ch != sizeof(dd.curr)) {
		if (uin->turn == 'e') {
		    end = -3;
		    break;
		} else if (uin->turn != 'w') {
		    end = -1;
		    currutmp->turn = 'w';
		    break;
		}
		end = -1;
		break;
	    }
	    if (dd.curr.end == -3)
		mouts(23, 30, "\033[33m�n�D�X��\033[m");
	    else if (dd.curr.end == -4)
		mouts(23, 30, "\033[33m�n�D����\033[m");
	    else if (dd.curr.end == -5)
		mouts(23, 30, "\033[33m�n�D�s�Y\033[m");
	    else
		mouts(23, 30, "");

	    recv(fd, &dd.brd, sizeof(dd.brd), 0);
	    dd.my = dd.curr.y;
	    dd.mx = dd.curr.x;
	    redraw(&dd);
	    if (dd.curr.end)
		mouts(22, 0, "   [1;33m��[5;35m����A�U�F[m");
	    move(cury[dd.my], curx[dd.mx]);
	} else {
	    if (currutmp->turn == 'p') {
		if (ch == 'y') {
		    end = -3;
		    currutmp->turn = 'e';
		    break;
		} else {
		    mouts(23, 30, "");
		    *buf = 0;
		    currutmp->turn = (uin->turn) ? 0 : 1;
		}
	    } else if (currutmp->turn == 'c') {
		if (ch == 'y') {
		    currutmp->color = (currutmp->color == '1') ? '0' : '1';
		    uin->color = (uin->color == '1') ? '0' : '1';
		    mouts(21, 0, (currutmp->color == '1') ? "   \033[1;33m��[1;31m�A�������\033[m" : "   \033[1;33m��[1;36m�A���¦��\033[m");
		} else {
		    mouts(23, 30, "");
		    currutmp->turn = (uin->turn) ? 0 : 1;
		}
	    } else if (currutmp->turn == 'g') {
		if (ch == 'y') {
		    dd.cont = 1;
		    mouts(21, 0, "   \033[1;33m��[1;31m�A�������\033[m �i�s�Y");
		} else {
		    mouts(23, 30, "");
		    currutmp->turn = (uin->turn) ? 0 : 1;
		}
	    }
	    if (currutmp->turn == 1) {
	        sint go_on;
		if (uin->turn == 'g') {
		    dd.cont = 1;
		    uin->turn = (currutmp->turn) ? 0 : 1;
		    mouts(21, 10, "�i�s�Y");
		}
		end = playing(&dd, fd, currutmp->color - '0', ch, &go_on, uin);

		if (end == -1) {
		    currutmp->turn = 'w';
		    break;
		} else if (end == -2) {
		    uin->turn = 'w';
		    break;
		} else if (end == -3) {
		    uin->turn = 'p';
		    dd.curr.end = -3;
		    send(fd, &dd.curr, sizeof(dd.curr), 0);
		    send(fd, &dd.brd, sizeof(buf), 0);
		    continue;
		} else if (end == -4) {
		    if (currutmp->color != '1' && currutmp->color != '0')
			continue;
		    uin->turn = 'c';
		    i = 0;
		    dd.curr.end = -4;
		    send(fd, &dd.curr, sizeof(dd.curr), 0);
		    send(fd, &dd.brd, sizeof(buf), 0);
		    continue;
		} else if (end == -5) {
		    uin->turn = 'g';
		    dd.curr.end = -5;
		    send(fd, &dd.curr, sizeof(dd.curr), 0);
		    send(fd, &dd.brd, sizeof(buf), 0);
		    continue;
		}
		if (!i && currutmp->color == '1') {
		    mouts(21, 0, "   \033[1;33m��[1;31m�A�������\033[m");
		    i++;
		    move(cury[dd.my], curx[dd.mx]);
		}
		if (!i && currutmp->color == '0') {
		    mouts(21, 0, "   \033[1;33m��[1;36m�A���¦��\033[m");
		    i++;
		    move(cury[dd.my], curx[dd.mx]);
		}
		if (uin->turn == 'e') {
		    end = -3;
		    break;
		}
		if (go_on < 0)
		    continue;

		move(22, 0);
		clrtoeol();
		prints("   [1;33m��[1;37m����%s�U �O�ȧO�� �L��ԣ��[m", currutmp->mateid);
		currutmp->turn = 0;
		uin->turn = 1;
	    } else {
		if (ch == 'q') {
		    uin->turn = 'w';
		    break;
		}
		move(22, 0);
		clrtoeol();
		prints("   [1;33m��[1;37m����%s�U �O�ȧO�� �L��ԣ��[m", currutmp->mateid);
	    }
	}
    }

    switch (end) {
    case -1:
    case -2:
	if (currutmp->turn == 'w') {
	    move(22, 0);
	    clrtoeol();
	    outs("[1;31m�AĹ�F.. �u�O����~~[m");
	} else {
	    move(22, 0);
	    clrtoeol();
	    outs("[1;31m�鱼�F��.....�U�����L�n��!![m");
	}
	break;
    case -3:
	mouts(22, 0, "[1;31m�X�ѭ�!! �U���b�����U�a ^_^[m");
	break;
    default:
	add_io(0, 0);
	close(fd);
	pressanykey();
	return 0;
    }
    add_io(0, 0);
    close(fd);
    pressanykey();
    return 0;
}
