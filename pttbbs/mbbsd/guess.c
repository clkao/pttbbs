/* $Id$ */
#include "bbs.h"
#define LOGPASS BBSHOME "/etc/winguess.log"

static void
show_table(char TABLE[], char ifcomputer)
{
    int             i;

    move(0, 35);
    outs("\033[1;44;33m  �i �q�Ʀr �j  \033[m");
    move(8, 1);
    outs("\033[1;44;36m��   �e   ��   �v\033[m\n");
    outs("\033[1;33m=================\033[m\n");
    if (ifcomputer) {
	outs("Ĺ�q��: 2 ��\n");
	outs("��q��: 0 ��\n");
    } else {
	for (i = 1; i <= 6; i++)
	    prints("��%d��, %02d��\n", i, TABLE[i]);
    }
    outs("\033[33m=================\033[m");
}

static int
get_money(void)
{
    int             money, i;
    char            data[20];

    move(1, 0);
    prints("�z�ثe��:%d Ptt$", cuser.money);
    do {
	getdata(2, 0, "�n��h��(5-10�Ϋ�q���}): ", data, 9, LCECHO);
	money = 0;
	if (data[0] == 'q' || data[0] == 'Q') {
	    unlockutmpmode();
	    return 0;
	}
	for (i = 0; data[i]; i++)
	    if (data[i] < '0' || data[i] > '9') {
		money = -1;
		break;
	    }
	if (money != -1) {
	    money = atoi(data);
	    reload_money();
	    if (money > cuser.money || money <= 4 || money > 10 ||
		money < 1)
		money = -1;
	}
    } while (money == -1);
    move(1, 0);
    clrtoeol();
    reload_money();
    prints("�z�ثe��:%d Ptt$", cuser.money - money);
    return money;
}

static int
check_data(char *str)
{
    int             i, j;

    if (strlen(str) != 4)
	return -1;
    for (i = 0; i < 4; i++)
	if (str[i] < '0' || str[i] > '9')
	    return -1;
    for (i = 0; i < 4; i++)
	for (j = i + 1; j < 4; j++)
	    if (str[i] == str[j])
		return -1;
    return 1;
}

static char    *
get_data(char data[5], int count)
{
    while (1) {
	getdata(6, 0, "��J�|��Ʀr(������): ", data, 5, LCECHO);
	if (check_data(data) == 1)
	    break;
    }
    return data;
}

static int
guess_play(char *data, char *answer, int count)
{
    int             A_num = 0, B_num = 0;
    int             i, j;

    for (i = 0; i < 4; i++) {
	if (data[i] == answer[i])
	    A_num++;
	for (j = 0; j < 4; j++)
	    if (i == j)
		continue;
	    else if (data[i] == answer[j]) {
		B_num++;
		break;
	    }
    }
    if (A_num == 4)
	return 1;
    move(count + 8, 55);
    prints("%s => \033[1;32m%dA %dB\033[m", data, A_num, B_num);
    return 0;
}

static int
result(int correct, int number)
{
    char            a = 0, b = 0, i, j;
    char            n1[5], n2[5];

    snprintf(n1, sizeof(n1), "%04d", correct);
    snprintf(n2, sizeof(n2), "%04d", number);
    for (i = 0; i < 4; i++)
	for (j = 0; j < 4; j++)
	    if (n1[(int)i] == n2[(int)j])
		b++;
    for (i = 0; i < 4; i++)
	if (n1[(int)i] == n2[(int)i]) {
	    b--;
	    a++;
	}
    return 10 * a + b;
}

static int
legal(int number)
{
    char            i, j;
    char            temp[5];

    snprintf(temp, sizeof(temp), "%04d", number);
    for (i = 0; i < 4; i++)
	for (j = i + 1; j < 4; j++)
	    if (temp[(int)i] == temp[(int)j])
		return 0;
    return 1;
}

static void
initcomputer(char flag[])
{
    int             i;

    for (i = 0; i < 10000; i++)
	if (legal(i))
	    flag[i] = 1;
	else
	    flag[i] = 0;
}

static int
computer(int correct, int total, char flag[], int n[])
{
    int             guess;
    static int      j;
    int             k, i;
    char            data[5];

    if (total == 1) {
	do {
	    guess = rand() % 10000;
	} while (!legal(guess));
    } else
	guess = n[rand() % j];
    k = result(correct, guess);
    if (k == 40) {
	move(total + 8, 25);
	snprintf(data, sizeof(data), "%04d", guess);
	prints("%s => �q���F!!", data);
	return 1;
    } else {
	move(total + 8, 25);
	snprintf(data, sizeof(data), "%04d", guess);
	prints("%s => \033[1;32m%dA %dB\033[m", data, k / 10, k % 10);
    }
    j = 0;
    for (i = 0; i < 10000; i++)
	if (flag[i]) {
	    if (result(i, guess) != k)
		flag[i] = 0;
	    else
		n[j++] = i;
	}
    return 0;
}

static void
Diff_Random(char *answer)
{
    register int    i = 0, j, k;

    while (i < 4) {
	k = rand() % 10 + '0';
	for (j = 0; j < i; j++)
	    if (k == answer[j])
		break;
	if (j == i) {
	    answer[j] = k;
	    i++;
	}
    }
    answer[4] = 0;
}

#define lockreturn0(unmode, state) if(lockutmpmode(unmode, state)) return 0

int
guess_main(void)
{
    char            data[5];
    int             money;
    char            computerwin = 0, youwin = 0;
    int             count = 0, c_count = 0;
    char            ifcomputer[2];
    char            answer[5];
    int            *n = NULL;
    char            yournum[5];
    char           *flag = NULL;
    char            TABLE[] = {0, 10, 8, 4, 2, 1, 0, 0, 0, 0, 0};
    FILE           *file;

    clear();
    showtitle("�q�Ʀr", BBSName);
    lockreturn0(GUESSNUM, LOCK_MULTI);

    reload_money();
    if (cuser.money < 5) {
	clear();
	move(12, 35);
	outs("�������� �ܤ֭n 5 Ptt$");
	unlockutmpmode();
	pressanykey();
	return 1;
    }
    if ((money = get_money()) == 0)
	return 1;
    vice(money, "�q�Ʀr");

    Diff_Random(answer);
    move(2, 0);
    clrtoeol();
    prints("�z�U�` :%lu Ptt$", money);

    getdata_str(4, 0, "�z�n�M�q�����ɶ�? <y/n>[y]:",
		ifcomputer, sizeof(ifcomputer), LCECHO, "y");
    if (ifcomputer[0] == 'y') {
	ifcomputer[0] = 1;
	show_table(TABLE, 1);
    } else {
	ifcomputer[0] = 0;
	show_table(TABLE, 0);
    }
    if (ifcomputer[0]) {
	do {
	    getdata(5, 0, "�п�J�z�n���q���q���Ʀr: ",
		    yournum, sizeof(yournum), LCECHO);
	} while (!legal(atoi(yournum)));
	move(8, 25);
	outs("�q���q");
	flag = malloc(sizeof(char) * 10000);
	n = malloc(sizeof(int) * 1500);
	initcomputer(flag);
    }
    move(8, 55);
    outs("�A�q");
    while (((!computerwin || !youwin) && count < 10 && (ifcomputer[0])) ||
	   (!ifcomputer[0] && count < 10 && !youwin)) {
	if (!computerwin && ifcomputer[0]) {
	    ++c_count;
	    if (computer(atoi(yournum), c_count, flag, n))
		computerwin = 1;
	}
	move(20, 55);
	prints("�� %d �����| ", count + 1);
	if (!youwin) {
	    ++count;
	    if (guess_play(get_data(data, count), answer, count))
		youwin = 1;
	}
    }
    move(17, 35);
    free(flag);
    free(n);
    if (ifcomputer[0]) {
	if (count > c_count) {
	    outs("�A�鵹�q���F");
	    move(18, 35);
	    prints("�A�ߤF %lu ", money);
	    if ((file = fopen(LOGPASS, "a"))) {
		fprintf(file, "�q����%d���q��, ", c_count);
		if (youwin)
		    fprintf(file, "%s ��%d���q��, ", cuser.userid, count);
		else
		    fprintf(file, "%s �S�q��, ", cuser.userid);
		fprintf(file, "�q���Ȩ��F%s %ld Ptt$\n", cuser.userid, money);
		fclose(file);
	    }
	} else if (count < c_count) {
	    outs("�u�F�`, ���A�Ȩ��o");
	    move(18, 35);
	    prints("�A�Ȩ��F %lu ", money * 2);
	    demoney(money * 2);
	    if ((file = fopen(LOGPASS, "a"))) {
		fprintf(file, "id: %s, ��%d���q��, �q����%d���q��, "
			"Ĺ�F�q�� %ld Ptt$\n", cuser.userid, count,
			c_count, money * 2);
		fclose(file);
	    }
	} else {
	    prints("�u�F�`, �M�q����������F, ���^����%lu\n", money);
	    demoney(money);
	    if ((file = fopen(LOGPASS, "a"))) {
		fprintf(file, "id: %s �M�q�������F����\n", cuser.userid);
		fclose(file);
	    }
	}
	unlockutmpmode();
	pressanykey();
	return 1;
    }
    if (youwin) {
	demoney(TABLE[count] * money);
	if (count < 5) {
	    outs("�u�F�`, ���Q�A�Ȩ��F");
	    if ((file = fopen(LOGPASS, "a"))) {
		fprintf(file, "id: %s, ��%d���q��, Ĺ�F %ld Ptt$\n",
			cuser.userid, count, TABLE[count] * money);
		fclose(file);
	    }
	} else if (count > 5) {
	    outs("��, �Ӧh���~�q�X�ӤF");
	    if ((file = fopen(LOGPASS, "a"))) {
		fprintf(file, "id: %s, ��%d���~�q��, �ߤF %ld Ptt$\n",
			cuser.userid, count, money);
		fclose(file);
	    }
	} else {
	    outs("�����q�X��, �٧A�����a");
	    move(18, 35);
	    clrtoeol();
	    prints("�A���^�F%lu Ptt$\n", money);
	    if ((file = fopen(LOGPASS, "a"))) {
		fprintf(file, "id: %s, ��%d���q��, ���^�F���� %lu Ptt$\n",
			cuser.userid, count, money);
		fclose(file);
	    }
	}
	unlockutmpmode();
	pressanykey();
	return 1;
    }
    move(17, 35);
    prints("�K�K �зǵ��׬O %s ", answer);
    move(18, 35);
    outs("�U���A�ӧa");
    if ((file = fopen(BBSHOME "/etc/loseguess.log", "a"))) {
	fprintf(file, "id: %s ��F %ld Ptt$\n", cuser.userid, money);
	fclose(file);
    }
    unlockutmpmode();
    pressanykey();
    return 1;
}
