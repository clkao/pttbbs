/* $Id$ */
#include "bbs.h"

//////////////////////////////////////////////////////
//
// we don't use real user money anymore, because there
// are too many bots...
//
// if you want to associate card games with real money,
// override the functions below.
//
//////////////////////////////////////////////////////

static int card_money = 0;
static time4_t card_time  = 0;

#define CARD_MONEY_DEFAULT  (100)
#define PMONEY     (10)	// �C����l�Ҧ����ƥ�

int card_reset_money()
{
    //  initialize
    card_money = CARD_MONEY_DEFAULT;

    // alert user for the new money change
    clear();
    move(t_lines/2-2, 0);
    prints( "    �Ъ`�N: ���C�ֳ��ثe�w�אּ�ϥλP BBS �t�Ϊ����L�����w�X�A\n\n"
	    "    �C���C���}�l�z���|���m�� %u �T�w�X�A���}��۰ʥ��ġC\n\n"
	    "    (P.S: �C���C���i�J�ɷ|���۰ʦ��� %u �T, �ҥH������ܼƥج� %u )\n"
	    , card_money, PMONEY, card_money - PMONEY);
    vmsg(NULL);

    card_time  = now;
    return card_money;
}

int card_get_money()
{
    return card_money;
}

int card_add_money(int delta)
{
    if (card_money + delta < 0)
	return (card_money = 0);
    card_money += delta;
    return card_money;
}

void card_end_game()
{
    clear();
    vs_hdr("�C�ֳ�");
    if (card_get_money())
    {
	int m = card_get_money();
	prints("\n    �C�������A�z�������ԪG�� %d - %d = " 
		ANSI_COLOR(1;3%d) "%d" ANSI_RESET " �T�w�X�C\n\n",
		m, CARD_MONEY_DEFAULT, 
		m >= CARD_MONEY_DEFAULT ? 3 : 1,
		m - CARD_MONEY_DEFAULT);
    } else {
	prints("\n    ��p�A�z�w�g����F�C\n\n");
    }
    card_time = now - card_time;
    prints( "    �����C���ɶ��� %d �p�� %d �� %d ��C\n\n"
	    "    �P�±z�����{�I\n\n",
	     card_time / 3600,
	    (card_time % 3600) / 60,
	     card_time % 60);
    vmsg(NULL);
}

void card_anti_bot_sleep()
{
    // sleep 0.2 sec
    usleep(200000);     
}

//////////////////////////////////////////////////////

enum CardSuit {
  Spade, Heart, Diamond, Club
};

static int
card_remain(int cards[])
{
    int             i, temp = 0;

    for (i = 0; i < 52; i++)
	temp += cards[i];
    if (temp == 52)
	return 1;
    return 0;
}

static enum CardSuit
card_flower(int card)
{
    return (card / 13);
}

/* 1...13 */
static int
card_number(int card)
{
    return (card % 13 + 1);
}

static int
card_isblackjack(int card1, int card2)
{
    return 
	(card_number(card1)==1 && (10<=card_number(card2) && card_number(card2)<=13)) ||
	(card_number(card2)==1 && (10<=card_number(card1) && card_number(card1)<=13));
}

static int
card_select(int *now)
{
    char           *cc[2] = {ANSI_COLOR(44) "            " ANSI_RESET,
    ANSI_COLOR(1;33;41) "     ��     " ANSI_RESET};

    while (1) {
	move(20, 0);
	clrtoeol();
	prints("%s%s%s%s%s", (*now == 0) ? cc[1] : cc[0],
	       (*now == 1) ? cc[1] : cc[0],
	       (*now == 2) ? cc[1] : cc[0],
	       (*now == 3) ? cc[1] : cc[0],
	       (*now == 4) ? cc[1] : cc[0]);
	switch (vkey()) {
	case 'Q':
	case 'q':
	    return 0;
	case '+':
	case ',':
	    return 1;
	case KEY_ENTER:
	    return -1;
	case KEY_LEFT:
	    *now = (*now + 4) % 5;
	    break;
	case KEY_RIGHT:
	    *now = (*now + 1) % 5;
	    break;
	case '1':
	    *now = 0;
	    break;
	case '2':
	    *now = 1;
	    break;
	case '3':
	    *now = 2;
	    break;
	case '4':
	    *now = 3;
	    break;
	case '5':
	    *now = 4;
	    break;
	}
    }
}

static void
card_display(int cline, int number, enum CardSuit flower, int show)
{
    int             color = 31;
    char           *cn[13] = {"��", "��", "��", "��", "��", "��",
    "��", "��", "��", "10", "��", "��", "��"};
    if (flower == 0 || flower == 3)
	color = 36;
    if ((show < 0) && (cline > 1 && cline < 8))
	outs("�x" ANSI_COLOR(1;33;42) "��������" ANSI_RESET "�x");
    else
	switch (cline) {
	case 1:
	    outs("�~�w�w�w�w��");
	    break;
	case 2:
	    prints("�x" ANSI_COLOR(1;%d) "%s" ANSI_RESET "      �x", color, cn[number - 1]);
	    break;
	case 3:
	    if (flower == 1)
		prints("�x" ANSI_COLOR(1;%d) "��������" ANSI_RESET "�x", color);
	    else
		prints("�x" ANSI_COLOR(1;%d) "  ����  " ANSI_RESET "�x", color);
	    break;
	case 4:
	    if (flower == 1)
		prints("�x" ANSI_COLOR(1;%d) "�i�i�i�i" ANSI_RESET "�x", color);
	    else if (flower == 3)
		prints("�x" ANSI_COLOR(1;%d) "���i�i��" ANSI_RESET "�x", color);
	    else
		prints("�x" ANSI_COLOR(1;%d) "���i�i��" ANSI_RESET "�x", color);
	    break;
	case 5:
	    if (flower == 0)
		prints("�x" ANSI_COLOR(1;%d) "�i�i�i�i" ANSI_RESET "�x", color);
	    else if (flower == 3)
		prints("�x" ANSI_COLOR(1;%d) "�i�����i" ANSI_RESET "�x", color);
	    else
		prints("�x" ANSI_COLOR(1;%d) "���i�i��" ANSI_RESET "�x", color);
	    break;
	case 6:
	    if (flower == 0)
		prints("�x" ANSI_COLOR(1;%d) "  ����  " ANSI_RESET "�x", color);
	    else if (flower == 3)
		prints("�x" ANSI_COLOR(1;%d) "��������" ANSI_RESET "�x", color);
	    else
		prints("�x" ANSI_COLOR(1;%d) "  ����  " ANSI_RESET "�x", color);
	    break;
	case 7:
	    prints("�x      " ANSI_COLOR(1;%d) "%s" ANSI_RESET "�x", color, cn[number - 1]);
	    break;
	case 8:
	    outs("���w�w�w�w��");
	    break;
	}
}

static void
card_show(int maxncard, int cpu[], int c[], int me[], int m[])
{
    int             i, j;

    for (j = 0; j < 8; j++) {
	move(2 + j, 0);
	clrtoeol();
	for (i = 0; i < maxncard && cpu[i] >= 0; i++)
	    card_display(j + 1, card_number(cpu[i]),
			 card_flower(cpu[i]), c[i]);
    }

    for (j = 0; j < 8; j++) {
	move(11 + j, 0);
	clrtoeol();
	for (i = 0; i < maxncard && me[i] >= 0; i++)
	    card_display(j + 1, card_number(me[i]), card_flower(me[i]), m[i]);
    }
}
static void
card_new(int cards[])
{
    memset(cards, 0, sizeof(int) * 52);
}

static int
card_give(int cards[])
{
    int i;
    int	freecard[52];
    int nfreecard = 0;

    for(i=0; i<52; i++)
	if(cards[i]==0)
	    freecard[nfreecard++]=i;

    assert(nfreecard>0); /* caller �n�t�d�T�O�٦��ѵP */

    i=freecard[random()%nfreecard];
    cards[i] = 1;
    return i;
}

static void
card_start(char name[])
{
    clear();
    vs_hdr(name);
    move(1, 0);
    outs("    " ANSI_COLOR(1;33;41) "   �q  ��   " ANSI_RESET);
    move(10, 0);
    outs(ANSI_COLOR(1;34;44) "���㡻�㡻�㡻�㡻�㡻�㡻�㡻�㡻�㡻�㡻�㡻��"
	   "���㡻�㡻�㡻�㡻�㡻�㡻�㡻" ANSI_RESET);
    move(19, 0);
    outs("    " ANSI_COLOR(1;37;42) "   ��  �v   " ANSI_RESET);
}

static int
card_99_add(int i, int aom, int count)
{
    if (i == 4 || i == 5 || i == 11)
	return count;
    else if (i == 12)
	return count + 20 * aom;
    else if (i == 10)
	return count + 10 * aom;
    else if (i == 13)
	return 99;
    else
	return count + i;
}

static int
card_99_cpu(int cpu[], int *count)
{
    int             stop = -1;
    int             twenty = -1;
    int             ten = -1;
    int             kill = -1;
    int             temp, num[10];
    int             other = -1;
    int             think = 99 - (*count);
    int             i, j;

    for (i = 0; i < 10; i++)
	num[i] = -1;
    for (i = 0; i < 5; i++) {
	temp = card_number(cpu[i]);
	if (temp == 4 || temp == 5 || temp == 11)
	    stop = i;
	else if (temp == 12)
	    twenty = i;
	else if (temp == 10)
	    ten = i;
	else if (temp == 13)
	    kill = i;
	else {
	    other = i;
	    num[temp] = i;
	}
    }
    for (j = 9; j > 0; j--)
	if (num[j] >= 0 && j != 4 && j != 5 && think >= j) {
	    (*count) += j;
	    return num[j];
	}
    if ((think >= 20) && (twenty >= 0)) {
	(*count) += 20;
	return twenty;
    } else if ((think >= 10) && (ten >= 0)) {
	(*count) += 10;
	return ten;
    } else if (stop >= 0)
	return stop;
    else if (kill >= 0) {
	(*count) = 99;
	return kill;
    } else if (ten >= 0) {
	(*count) -= 10;
	return ten;
    } else if (twenty >= 0) {
	(*count) -= 20;
	return twenty;
    } else {
	(*count) += card_number(cpu[0]);
	return 0;
    }
}

int
card_99(void)
{
    int             i, j, turn;
    int             cpu[5], c[5], me[5], m[5];
    int             cards[52];
    int             count = 0;
    char           *ff[4] = {ANSI_COLOR(1;36) "�®�", ANSI_COLOR(1;31) "����",
    ANSI_COLOR(1;31) "���", ANSI_COLOR(1;36) "�ª�"};
    char           *cn[13] = {"��", "��", "��", "��", "��", "��",
    "��", "��", "��", "10", "��", "��", "��"};
    for (i = 0; i < 5; i++)
	cpu[i] = c[i] = me[i] = m[i] = -1;
    setutmpmode(CARD_99);
    card_start("�Ѫ��a�[");
    card_new(cards);
    for (i = 0; i < 5; i++) {
	cpu[i] = card_give(cards);
	me[i] = card_give(cards);
	m[i] = 1;
    }
    card_show(5, cpu, c, me, m);
    j = 0;
    turn = 1;
    move(21, 0);
    clrtoeol();
    prints("[0]�ثe %d , �� %d �I\n", count, 99 - count);
    outs("���k�䲾�ʴ��, [Enter]�T�w, [ + ]��[�G�Q(�[�Q), [Q/q]���C��");
    while (1) {
	i = card_select(&j);
	if (i == 0)		/* ���C�� */
	    return 0;
	count = card_99_add(card_number(me[j]), i, count);
	move(21 + (turn / 2) % 2, 0);
	clrtoeol();
	prints("[%d]�z�X %s%s" ANSI_RESET " �ثe " ANSI_COLOR(1;31) "%d/" ANSI_COLOR(34) "%d" ANSI_RESET " �I",
	       turn, ff[card_flower(me[j])],
	       cn[card_number(me[j]) - 1], count, 99 - count);
	me[j] = card_give(cards);
	turn++;
	if (count < 0)
	    count = 0;
	card_show(5, cpu, c, me, m);
	pressanykey();
	if (count > 99) {
	    move(22, 0);
	    clrtoeol();
	    prints("[%d]���G..YOU LOSS..�ثe " ANSI_COLOR(1;31) "%d/" ANSI_COLOR(34) "%d" ANSI_RESET " �I",
		   turn, count, 99 - count);
	    pressanykey();
	    return 0;
	}
	i = card_99_cpu(cpu, &count);
	move(21 + (turn / 2 + 1) % 2, 40);
	prints("[%d]�q���X %s%s" ANSI_RESET " �ثe " ANSI_COLOR(1;31) "%d/" ANSI_COLOR(34) "%d" ANSI_RESET " �I",
	       turn, ff[card_flower(cpu[i])],
	       cn[card_number(cpu[i]) - 1], count, 99 - count);
	cpu[i] = card_give(cards);
	turn++;
	if (count < 0)
	    count = 0;
	if (count > 99) {
	    move(22, 0);
	    clrtoeol();
	    prints("[%d]���G..YOU WIN!..�ثe " ANSI_COLOR(1;31) "%d/" ANSI_COLOR(34) "%d" ANSI_RESET " �I",
		   turn, count, 99 - count);
	    pressanykey();
	    return 0;
	}
	if (!card_remain(cards)) {
	    card_new(cards);
	    for (i = 0; i < 5; i++) {
		cards[me[i]] = 1;
		cards[cpu[i]] = 1;
	    }
	}
    }
}

#define TEN_HALF   (5)		/* �Q�I�b��Ticket */
#define JACK      (10)		/* �³ǧJ��Ticket */
#define NINE99    (99)		/* 99    ��Ticket */

static int
game_log(int type, int money)
{
    if (money > 0)
	card_add_money(money);

    return 0;
}

static int
card_double_ask(void)
{
    char            buf[100], buf2[3];

    snprintf(buf, sizeof(buf),
	     "[ %s ]�z�{�b�@�� %d �T�w�X,  �{�b�n����(�[�� %d �T)��? [y/N]",
	     cuser.userid, card_get_money(), JACK);
    if (card_get_money() < JACK)
	return 0;
    getdata(20, 0, buf, buf2, sizeof(buf2), LCECHO);
    if (buf2[0] == 'y')
	return 1;
    return 0;
}

static int
card_ask(void)
{
    char            buf[100], buf2[3];

    move(21, 0); clrtoeol();
    outs("< �Y�Q���}�п�J q >");
    snprintf(buf, sizeof(buf), "[ %s ]�z�{�b�@�� %d �T�w�X�A�٭n�[�P��? [y/N/q]: ",
	    cuser.userid, card_get_money());
    getdata(20, 0, buf, buf2, sizeof(buf2), LCECHO);

    // peek one more byte
    if (buf2[0] == ' ')
	buf2[0] = buf2[1];

    if (buf2[0] == 'y')
	return 1;
    if (buf2[0] == 'q')
	return -1;
    return 0;
}

static int
card_alls_lower(int all[])
{
    int             i, count = 0;
    for (i = 0; i < 6 && all[i] >= 0; i++)
	if (card_number(all[i]) <= 10)
	    count += card_number(all[i]);
	else
	    count += 10;
    return count;
}

static int
card_alls_upper(int all[])
{
    int             i, count;

    count = card_alls_lower(all);
    for (i = 0; i < 6 && all[i] >= 0 && count <= 11; i++)
	if (card_number(all[i]) == 1)
	    count += 10;
    return count;
}

static int
card_jack(int *db)
{
    int             i, j;
    int             cpu[6], c[6], me[6], m[6];
    int             cards[52];
    int             r = 0;

    for (i = 0; i < 6; i++)
	cpu[i] = c[i] = me[i] = m[i] = -1;

    if ((*db) < 0) {
	card_new(cards);
	card_start("�³ǧJ");
	for (i = 0; i < 2; i++) {
	    cpu[i] = card_give(cards);
	    me[i] = card_give(cards);
	}
    } else {
	card_new(cards);
	cards[*db]=1;
	card_start("�³ǧJDOUBLE�l�[��");
	cpu[0] = card_give(cards);
	cpu[1] = card_give(cards);
	me[0] = *db;
	me[1] = card_give(cards);
	*db = -1;
    }
    c[1] = m[0] = m[1] = 1;
    card_show(6, cpu, c, me, m);

    /* black jack */
    if (card_isblackjack(me[0],me[1])) {
	if(card_isblackjack(cpu[0],cpu[1])) {
	    c[0]=1;
	    card_show(6, cpu, c, me, m);
	    game_log(JACK, JACK);
	    vmsgf("�A��q��������³ǧJ, �h�� %d �T�w�X", JACK);
	    return 0;
	}
	game_log(JACK, JACK * 5/2);
	vmsgf("�ܤ�����! (�³ǧJ!! �[ %d �T�w�X)", JACK * 5/2);
	return 0;
    } else if(card_isblackjack(cpu[0],cpu[1])) {
	c[0] = 1;
	card_show(6, cpu, c, me, m);
	game_log(JACK, 0);
	vmsg("�K�K...���n�N��....�³ǧJ!!");
	return 0;
    }

    /* double ��P */
    if ((card_number(me[0]) == card_number(me[1])) &&
	(card_double_ask())) {
	*db = me[1];
	me[1] = card_give(cards);
	card_show(6, cpu, c, me, m);
    }

    i = 2;
    while (i < 6 && (r = card_ask()) > 0) {
	me[i] = card_give(cards);
	m[i] = 1;
	card_show(6, cpu, c, me, m);
	if (card_alls_lower(me) > 21) {
	    game_log(JACK, 0);
	    vmsg("���...�z���F!");
	    return 0;
	}
	i++;
    }

    // user quit
    if (r < 0)
	return -1;

    if (i == 6) { /* �e���u���\���i�P, �]�������⪱�aĹ. �³ǧJ��ڤW�S�o�W�h */
	game_log(JACK, JACK * 10);
	vmsgf("�n�F�`��! ���i�P�٨S�z! �[ %d �T�w�X!", 5 * JACK);
	return 0;
    }

    j = 2;
    c[0] = 1;
    while (j<6 && card_alls_upper(cpu)<=16) {
	cpu[j] = card_give(cards);
	c[j] = 1;
	if (card_alls_lower(cpu) > 21) {
	    card_show(6, cpu, c, me, m);
	    game_log(JACK, JACK * 2);
	    vmsgf("����...�q���z���F! �AĹ�F! �i�o %d �T�w�X", JACK * 2);
	    return 0;
	}
	j++;
    }
    card_show(6, cpu, c, me, m);
    if(card_alls_upper(cpu)==card_alls_upper(me)) {
	game_log(JACK, JACK);
	vmsgf("�����A�h�^ %d �T�w�X!", JACK);
	return 0;
    }
    if(card_alls_upper(cpu)<card_alls_upper(me)) {
	game_log(JACK, JACK * 2);
	vmsgf("����...�q������p! �AĹ�F! �i�o %d �T�w�X", JACK * 2);
	return 0;
    }
    game_log(JACK, 0);
    vmsg("�z�z...�q��Ĺ�F!");
    return 0;
}

static int
card_all(int all[])
{
    int             i, count = 0;

    for (i = 0; i < 5 && all[i] >= 0; i++)
	if (card_number(all[i]) <= 10)
	    count += 2 * card_number(all[i]);
	else
	    count += 1;
    return count;
}

static int
ten_helf(void)
{
    int             i, j;
    int             cpu[5], c[5], me[5], m[5];
    int             cards[52];
    int             r = 0;

    card_start("�Q�I�b");
    card_new(cards);
    for (i = 0; i < 5; i++)
	cpu[i] = c[i] = me[i] = m[i] = -1;

    cpu[0] = card_give(cards);
    me[0] = card_give(cards);
    m[0] = 1;
    card_show(5, cpu, c, me, m);
    i = 1;
    while (i < 5 && (r = card_ask()) > 0) {
	me[i] = card_give(cards);
	m[i] = 1;
	card_show(5, cpu, c, me, m);
	if (card_all(me) > 21) {
	    game_log(TEN_HALF, 0);
	    vmsg("���...�z���F!");
	    return 0;
	}
	i++;
    }

    // user quit
    if (r < 0)
	return -1;

    if (i == 5) {		/* �L���� */
	game_log(TEN_HALF, PMONEY * 5);
	vmsgf("�n�F�`��! �L������! �[ %d �T�w�X!", 5 * PMONEY);
	return 0;
    }
    j = 1;
    c[0] = 1;
    while (j < 5 && ((card_all(cpu) < card_all(me)) ||
		     (card_all(cpu) == card_all(me) && j < i))) {
	cpu[j] = card_give(cards);
	c[j] = 1;
	if (card_all(cpu) > 21) {
	    card_show(5, cpu, c, me, m);
	    game_log(TEN_HALF, PMONEY * 2);
	    vmsgf("����...�q���z���F! �AĹ�F! �i�o %d �T�w�X", PMONEY * 2);
	    return 0;
	}
	j++;
    }
    card_show(5, cpu, c, me, m);
    game_log(TEN_HALF, 0);
    vmsg("�z�z...�q��Ĺ�F!");
    return 0;
}

// Main Game Loops

int
g_card_jack(void)
{
    int db;
    int bQuit=0;

    setutmpmode(JACK_CARD);
    card_reset_money();

    while (!bQuit && card_get_money() >= PMONEY) 
    {
	db = -1;
	card_add_money(-PMONEY);
	do {
	    if (card_jack(&db) < 0)
	    {
		card_add_money(PMONEY);
		bQuit = 1;
		break;
	    }
	} while(db>=0);

	card_anti_bot_sleep();
    }
    card_end_game();
    return 0;
}

int
g_ten_helf(void)
{
    setutmpmode(TENHALF);
    card_reset_money();

    while (card_get_money() >= PMONEY) 
    {
	card_add_money(-PMONEY);
	if (ten_helf() < 0)
	{
	    card_add_money(+PMONEY);
	    break;
	}
	card_anti_bot_sleep();
    }
    card_end_game();
    return 0;
}
