/* $Id$ */
#include "bbs.h"

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
    char           *cc[2] = {"\033[44m            \033[m",
    "\033[1;33;41m     ��     \033[m"};

    while (1) {
	move(20, 0);
	clrtoeol();
	prints("%s%s%s%s%s", (*now == 0) ? cc[1] : cc[0],
	       (*now == 1) ? cc[1] : cc[0],
	       (*now == 2) ? cc[1] : cc[0],
	       (*now == 3) ? cc[1] : cc[0],
	       (*now == 4) ? cc[1] : cc[0]);
	switch (igetch()) {
	case 'Q':
	case 'q':
	    return 0;
	case '+':
	case ',':
	    return 1;
	case '\r':
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
	outs("�x\033[1;33;42m��������\033[m�x");
    else
	switch (cline) {
	case 1:
	    outs("�~�w�w�w�w��");
	    break;
	case 2:
	    prints("�x\033[1;%dm%s\033[m      �x", color, cn[number - 1]);
	    break;
	case 3:
	    if (flower == 1)
		prints("�x\033[1;%dm��������\033[m�x", color);
	    else
		prints("�x\033[1;%dm  ����  \033[m�x", color);
	    break;
	case 4:
	    if (flower == 1)
		prints("�x\033[1;%dm�i�i�i�i\033[m�x", color);
	    else if (flower == 3)
		prints("�x\033[1;%dm���i�i��\033[m�x", color);
	    else
		prints("�x\033[1;%dm���i�i��\033[m�x", color);
	    break;
	case 5:
	    if (flower == 0)
		prints("�x\033[1;%dm�i�i�i�i\033[m�x", color);
	    else if (flower == 3)
		prints("�x\033[1;%dm�i�����i\033[m�x", color);
	    else
		prints("�x\033[1;%dm���i�i��\033[m�x", color);
	    break;
	case 6:
	    if (flower == 0)
		prints("�x\033[1;%dm  ����  \033[m�x", color);
	    else if (flower == 3)
		prints("�x\033[1;%dm��������\033[m�x", color);
	    else
		prints("�x\033[1;%dm  ����  \033[m�x", color);
	    break;
	case 7:
	    prints("�x      \033[1;%dm%s\033[m�x", color, cn[number - 1]);
	    break;
	case 8:
	    outs("���w�w�w�w��");
	    break;
	}
}

static void
card_show(int cpu[], int c[], int me[], int m[])
{
    int             i, j;

    for (j = 0; j < 8; j++) {
	move(2 + j, 0);
	clrtoeol();
	for (i = 0; i < 5 && cpu[i] >= 0; i++)
	    card_display(j + 1, card_number(cpu[i]),
			 card_flower(cpu[i]), c[i]);
    }

    for (j = 0; j < 8; j++) {
	move(11 + j, 0);
	clrtoeol();
	for (i = 0; i < 5 && me[i] >= 0; i++)
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

    i=freecard[rand()%nfreecard];
    cards[i] = 1;
    return i;
}

static void
card_start(char name[])
{
    clear();
    stand_title(name);
    move(1, 0);
    outs("    \033[1;33;41m   �q  ��   \033[m");
    move(10, 0);
    outs("\033[1;34;44m���㡻�㡻�㡻�㡻�㡻�㡻�㡻�㡻�㡻�㡻�㡻��"
	   "���㡻�㡻�㡻�㡻�㡻�㡻�㡻\033[m");
    move(19, 0);
    outs("    \033[1;37;42m   ��  �v   \033[m");
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
card_99()
{
    int             i, j, turn;
    int             cpu[5], c[5], me[5], m[5];
    int             cards[52];
    int             count = 0;
    char           *ff[4] = {"\033[1;36m�®�", "\033[1;31m����",
    "\033[1;31m���", "\033[1;36m�ª�"};
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
    card_show(cpu, c, me, m);
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
	prints("[%d]�z�X %s%s\033[m �ثe \033[1;31m%d/\033[34m%d\033[m �I",
	       turn, ff[card_flower(me[j])],
	       cn[card_number(me[j]) - 1], count, 99 - count);
	me[j] = card_give(cards);
	turn++;
	if (count < 0)
	    count = 0;
	card_show(cpu, c, me, m);
	pressanykey();
	if (count > 99) {
	    move(22, 0);
	    clrtoeol();
	    prints("[%d]���G..YOU LOSS..�ثe \033[1;31m%d/\033[34m%d\033[m �I",
		   turn, count, 99 - count);
	    pressanykey();
	    return 0;
	}
	i = card_99_cpu(cpu, &count);
	move(21 + (turn / 2 + 1) % 2, 40);
	prints("[%d]�q���X %s%s\033[m �ثe \033[1;31m%d/\033[34m%d\033[m �I",
	       turn, ff[card_flower(cpu[i])],
	       cn[card_number(cpu[i]) - 1], count, 99 - count);
	cpu[i] = card_give(cards);
	turn++;
	if (count < 0)
	    count = 0;
	if (count > 99) {
	    move(22, 0);
	    clrtoeol();
	    prints("[%d]���G..YOU WIN!..�ثe \033[1;31m%d/\033[34m%d\033[m �I",
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

#define PMONEY     (10)
#define TEN_HALF   (5)		/* �Q�I�b��Ticket */
#define JACK      (10)		/* �³ǧJ��Ticket */
#define NINE99    (99)		/* 99    ��Ticket */

static int
game_log(int type, int money)
{
    FILE           *fp;

    if (money > 0)
	demoney(money);

    switch (type) {
    case JACK:
	fp = fopen(BBSHOME "/etc/card/jack.log", "a");
	if (!fp)
	    return 0;
	fprintf(fp, "%s win:%d\n", cuser.userid, money);
	fclose(fp);
	break;
    case TEN_HALF:
	fp = fopen(BBSHOME "/etc/card/tenhalf.log", "a");
	if (!fp)
	    return 0;
	fprintf(fp, "%s win:%d\n", cuser.userid, money);
	fclose(fp);
	break;
    }
    return 0;
}

static int
card_double_ask()
{
    char            buf[100], buf2[3];

    snprintf(buf, sizeof(buf),
	     "[ %s ]�z�{�b�@�� %d P��,  �{�b�n����(�[�� %d ��)��? [y/N]",
	     cuser.userid, cuser.money, JACK);
    reload_money();
    if (cuser.money < JACK)
	return 0;
    getdata(20, 0, buf, buf2, sizeof(buf2), LCECHO);
    if (buf2[0] == 'y' || buf2[0] == 'Y')
	return 1;
    return 0;
}

static int
card_ask()
{
    char            buf[100], buf2[3];

    snprintf(buf, sizeof(buf), "[ %s ]�z�{�b�@�� %d P��,  �٭n�[�P��? [y/N]",
	    cuser.userid, cuser.money);
    getdata(20, 0, buf, buf2, sizeof(buf2), LCECHO);
    if (buf2[0] == 'y' || buf2[0] == 'Y')
	return 1;
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
    card_show(cpu, c, me, m);

    /* black jack */
    if (card_isblackjack(me[0],me[1])) {
	if(card_isblackjack(cpu[0],cpu[1])) {
	    c[0]=1;
	    card_show(cpu, c, me, m);
	    game_log(JACK, JACK);
	    vmsg("�A��q��������³ǧJ, �h�� %d ��", JACK);
	    return 0;
	}
	game_log(JACK, JACK * 5/2);
	vmsg("�ܤ�����! (�³ǧJ!! �[ %d ��)", JACK * 5/2);
	return 0;
    } else if(card_isblackjack(cpu[0],cpu[1])) {
	c[0] = 1;
	card_show(cpu, c, me, m);
	game_log(JACK, 0);
	vmsg("�K�K...���n�N��....�³ǧJ!!");
	return 0;
    }

    /* double ��P */
    if ((card_number(me[0]) == card_number(me[1])) &&
	(card_double_ask())) {
	*db = me[1];
	me[1] = card_give(cards);
	card_show(cpu, c, me, m);
    }

    i = 2;
    while (i < 6 && card_ask()) {
	me[i] = card_give(cards);
	m[i] = 1;
	card_show(cpu, c, me, m);
	if (card_alls_lower(me) > 21) {
	    game_log(JACK, 0);
	    vmsg("���...�z���F!");
	    return 0;
	}
	i++;
    }
    if (i == 6) { /* �e���u���\���i�P, �]�������⪱�aĹ. �³ǧJ��ڤW�S�o�W�h */
	game_log(JACK, JACK * 10);
	vmsg("�n�F�`��! ���i�P�٨S�z! �[P�� %d ��!", 5 * JACK);
	return 0;
    }

    j = 2;
    c[0] = 1;
    while (j<6 && card_alls_upper(cpu)<=16) {
	cpu[j] = card_give(cards);
	c[j] = 1;
	if (card_alls_lower(cpu) > 21) {
	    card_show(cpu, c, me, m);
	    game_log(JACK, JACK * 2);
	    vmsg("����...�q���z���F! �AĹ�F! �i�oP�� %d ��", JACK * 2);
	    return 0;
	}
	j++;
    }
    card_show(cpu, c, me, m);
    if(card_alls_upper(cpu)==card_alls_upper(me)) {
	game_log(JACK, JACK);
	vmsg("�����A�h�^P�� %d ��!", JACK);
	return 0;
    }
    if(card_alls_upper(cpu)<card_alls_upper(me)) {
	game_log(JACK, JACK * 2);
	vmsg("����...�q������p! �AĹ�F! �i�oP�� %d ��", JACK * 2);
	return 0;
    }
    game_log(JACK, 0);
    vmsg("�z�z...�q��Ĺ�F!");
    return 0;
}

int
g_card_jack()
{
    int             db;
    char            buf[3];

    setutmpmode(JACK_CARD);
    while (1) {
	reload_money();
	if (cuser.money < JACK) {
	    outs("�z����������!�h�h�o��Ǧ��N�q���峹�A��~~~");
	    return 0;
	}
	getdata(b_lines - 1, 0, "�T�w�n���³ǧJ�� �@���Q����?(Y/N)?[N]",
		buf, 3, LCECHO);
	if ((*buf != 'y') && (*buf != 'Y'))
	    break;
	else {
	    db = -1;
	    vice(PMONEY, "�³ǧJ");
	    do {
		card_jack(&db);
	    } while(db>=0);
	}
    }
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
ten_helf()
{
    int             i, j;
    int             cpu[5], c[5], me[5], m[5];
    int             cards[52];

    card_start("�Q�I�b");
    card_new(cards);
    for (i = 0; i < 5; i++)
	cpu[i] = c[i] = me[i] = m[i] = -1;

    cpu[0] = card_give(cards);
    me[0] = card_give(cards);
    m[0] = 1;
    card_show(cpu, c, me, m);
    i = 1;
    while (i < 5 && card_ask()) {
	me[i] = card_give(cards);
	m[i] = 1;
	card_show(cpu, c, me, m);
	if (card_all(me) > 21) {
	    game_log(TEN_HALF, 0);
	    vmsg("���...�z���F!");
	    return 0;
	}
	i++;
    }
    if (i == 5) {		/* �L���� */
	game_log(TEN_HALF, PMONEY * 5);
	vmsg("�n�F�`��! �L������! �[P�� %d ��!", 5 * PMONEY);
	return 0;
    }
    j = 1;
    c[0] = 1;
    while (j < 5 && ((card_all(cpu) < card_all(me)) ||
		     (card_all(cpu) == card_all(me) && j < i))) {
	cpu[j] = card_give(cards);
	c[j] = 1;
	if (card_all(cpu) > 21) {
	    card_show(cpu, c, me, m);
	    game_log(TEN_HALF, PMONEY * 2);
	    vmsg("����...�q���z���F! �AĹ�F! �i�oP�� %d ��", PMONEY * 2);
	    return 0;
	}
	j++;
    }
    card_show(cpu, c, me, m);
    game_log(TEN_HALF, 0);
    vmsg("�z�z...�q��Ĺ�F!");
    return 0;
}

int
g_ten_helf()
{
    char            buf[3];

    setutmpmode(TENHALF);
    while (1) {
	reload_money();
	if (cuser.money < TEN_HALF) {
	    outs("�z����������!�h�h�o��Ǧ��N�q���峹�A��~~~");
	    return 0;
	}
	getdata(b_lines - 1, 0,
		"\033[1;37m�T�w�n���Q�I�b�� �@���Q����?(Y/N)?[N]\033[m",
		buf, 3, LCECHO);
	if (buf[0] != 'y' && buf[0] != 'Y')
	    return 0;
	else {
	    vice(PMONEY, "�Q�I�b");
	    ten_helf();
	}
    }
    return 0;
}
