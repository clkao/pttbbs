/* $Id$ */
#include "bbs.h"

#define SIDE_ROW          10
#define TURN_ROW          11
#define STEP_ROW          12
#define TIME_ROW          13
#define WARN_ROW          15
#define MYWIN_ROW         17
#define HISWIN_ROW        18

static char    *turn_str[2] = {"�ª�", "����"};

static char    *num_str[10] = {
    "", "�@", "�G", "�T", "�|", "��", "��", "�C", "�K", "�E"
};

static char    *chess_str[2][8] = {
    /* 0     1     2     3     4     5     6     7 */
    {"  ", "�N", "�h", "�H", "��", "��", "�]", "��"},
    {"  ", "��", "�K", "��", "��", "�X", "��", "�L"}
};

static char    *chess_brd[BRD_ROW * 2 - 1] = {
    /* 0   1   2   3   4   5   6   7   8 */
    "�z�w�s�w�s�w�s�w�s�w�s�w�s�w�s�w�{",	/* 0 */
    "�x  �x  �x  �x�@�x���x  �x  �x  �x",
    "�u�w�q�w�q�w�q�w�q�w�q�w�q�w�q�w�t",	/* 1 */
    "�x  �x  �x  �x���x�@�x  �x  �x  �x",
    "�u�w�q�w�q�w�q�w�q�w�q�w�q�w�q�w�t",	/* 2 */
    "�x  �x  �x  �x  �x  �x  �x  �x  �x",
    "�u�w�q�w�q�w�q�w�q�w�q�w�q�w�q�w�t",	/* 3 */
    "�x  �x  �x  �x  �x  �x  �x  �x  �x",
    "�u�w�r�w�r�w�r�w�r�w�r�w�r�w�r�w�t",	/* 4 */
    "�x  ��    �e          �~    ��  �x",
    "�u�w�s�w�s�w�s�w�s�w�s�w�s�w�s�w�t",	/* 5 */
    "�x  �x  �x  �x  �x  �x  �x  �x  �x",
    "�u�w�q�w�q�w�q�w�q�w�q�w�q�w�q�w�t",	/* 6 */
    "�x  �x  �x  �x  �x  �x  �x  �x  �x",
    "�u�w�q�w�q�w�q�w�q�w�q�w�q�w�q�w�t",	/* 7 */
    "�x  �x  �x  �x�@�x���x  �x  �x  �x",
    "�u�w�q�w�q�w�q�w�q�w�q�w�q�w�q�w�t",	/* 8 */
    "�x  �x  �x  �x���x�@�x  �x  �x  �x",
    "�|�w�r�w�r�w�r�w�r�w�r�w�r�w�r�w�}"	/* 9 */
};

static char    *hint_str[] = {
    "  q      �{�����}",
    "  p      �n�D�M��",
    "��V��   ���ʹC��",
    "Enter    ���/����"
};

void
chc_movecur(int r, int c)
{
    move(r * 2 + 3, c * 4 + 4);
}

#define BLACK_COLOR       "\033[1;36m"
#define RED_COLOR         "\033[1;31m"
#define BLACK_REVERSE     "\033[1;37;46m"
#define RED_REVERSE       "\033[1;37;41m"
#define TURN_COLOR        "\033[1;33m"

static void
showstep(board_t board)
{
    int             turn, fc, tc, eatten;
    char           *dir;

    turn = CHE_O(board[chc_from.r][chc_from.c]);
    fc = (turn == (chc_my ^ 1) ? chc_from.c + 1 : 9 - chc_from.c);
    tc = (turn == (chc_my ^ 1) ? chc_to.c + 1 : 9 - chc_to.c);
    if (chc_from.r == chc_to.r)
	dir = "��";
    else {
	if (chc_from.c == chc_to.c)
	    tc = chc_from.r - chc_to.r;
	if (tc < 0)
	    tc = -tc;

	if ((turn == (chc_my ^ 1) && chc_to.r > chc_from.r) ||
	    (turn == chc_my && chc_to.r < chc_from.r))
	    dir = "�i";
	else
	    dir = "�h";
    }
    prints("%s%s%s%s%s",
	   turn == 0 ? BLACK_COLOR : RED_COLOR,
	   chess_str[turn][CHE_P(board[chc_from.r][chc_from.c])],
	   num_str[fc], dir, num_str[tc]);
    eatten = board[chc_to.r][chc_to.c];
    if (eatten)
	prints("�G %s%s",
	       CHE_O(eatten) == 0 ? BLACK_COLOR : RED_COLOR,
	       chess_str[CHE_O(eatten)][CHE_P(eatten)]);
    prints("\033[m");
}

void
chc_drawline(board_t board, chcusr_t *user1, chcusr_t *user2, int line)
{
    int             i, j;

    move(line, 0);
    clrtoeol();
    if (line == 0) {
	prints("\033[1;46m   �H�ѹ��   \033[45m%30s VS %-30s\033[m",
	       user1->userid, user2->userid);
    } else if (line >= 3 && line <= 21) {
	outs("   ");
	for (i = 0; i < 9; i++) {
	    j = board[RTL(line)][i];
	    if ((line & 1) == 1 && j) {
		if (chc_selected &&
		    chc_select.r == RTL(line) && chc_select.c == i)
		    prints("%s%s\033[m",
			   CHE_O(j) == 0 ? BLACK_REVERSE : RED_REVERSE,
			   chess_str[CHE_O(j)][CHE_P(j)]);
		else
		    prints("%s%s\033[m",
			   CHE_O(j) == 0 ? BLACK_COLOR : RED_COLOR,
			   chess_str[CHE_O(j)][CHE_P(j)]);
	    } else
		prints("%c%c", chess_brd[line - 3][i * 4],
		       chess_brd[line - 3][i * 4 + 1]);
	    if (i != 8)
		prints("%c%c", chess_brd[line - 3][i * 4 + 2],
		       chess_brd[line - 3][i * 4 + 3]);
	}
	outs("        ");

	if (line >= 3 && line < 3 + (int)dim(hint_str)) {
	    outs(hint_str[line - 3]);
	} else if (line == SIDE_ROW) {
	    prints("\033[1m�A�O%s%s\033[m",
		   chc_my == 0 ? BLACK_COLOR : RED_COLOR,
		   turn_str[chc_my]);
	} else if (line == TURN_ROW) {
	    prints("%s%s\033[m",
		   TURN_COLOR,
		   chc_my == chc_turn ? "����A�U�ѤF" : "���ݹ��U��");
	} else if (line == STEP_ROW && !chc_firststep) {
	    showstep(board);
	} else if (line == TIME_ROW) {
	    prints("�Ѿl�ɶ� %d:%02d", chc_lefttime / 60, chc_lefttime % 60);
	} else if (line == WARN_ROW) {
	    outs(chc_warnmsg);
	} else if (line == MYWIN_ROW) {
	    prints("\033[1;33m%12.12s    "
		   "\033[1;31m%2d\033[37m�� "
		   "\033[34m%2d\033[37m�� "
		   "\033[36m%2d\033[37m�M\033[m",
		   user1->userid,
		   user1->win, user1->lose - 1, user1->tie);
	} else if (line == HISWIN_ROW) {
	    prints("\033[1;33m%12.12s    "
		   "\033[1;31m%2d\033[37m�� "
		   "\033[34m%2d\033[37m�� "
		   "\033[36m%2d\033[37m�M\033[m",
		   user2->userid,
		   user2->win, user2->lose - 1, user2->tie);
	}
    } else if (line == 2 || line == 22) {
	outs("   ");
	if (line == 2)
	    for (i = 1; i <= 9; i++)
		prints("%s  ", num_str[i]);
	else
	    for (i = 9; i >= 1; i--)
		prints("%s  ", num_str[i]);
    }
}

void
chc_redraw(chcusr_t *user1, chcusr_t *user2, board_t board)
{
    int             i;

    for (i = 0; i <= 22; i++)
	chc_drawline(board, user1, user2, i);
}
