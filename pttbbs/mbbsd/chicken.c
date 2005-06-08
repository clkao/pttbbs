/* $Id$ */
#include "bbs.h"

#define NUM_KINDS   15		/* ���h�ֺذʪ� */

static const char * const cage[17] = {
    "�ϥ�", "�g��", "���~", "�֦~", "�C�K", "�C�~",
    "�C�~", "���O", "���~", "���~", "���~", "���~",
    "���~", "�Ѧ~", "�Ѧ~", "������", "�j��"};
static const char * const chicken_type[NUM_KINDS] = {
    "�p��", "���֤k", "�i�h", "�j��",
    "���s", "���N", "��", "�����p�s",
    "����", "�c�]", "�Ԫ�", "����",
    "���^�E", "�N�i�H", "ù��"};
static const char * const chicken_food[NUM_KINDS] = {
    "���}��", "��i�p��", "���ƫK��", "������",
    "����", "�p��", "�߻氮", "�p���氮",
    "�_��", "�F��", "����", "�K��",
    "���L", "���ܤ峹", "���G�F��"};
static          const int egg_price[NUM_KINDS] = {
    5, 25, 30, 40,
    80, 50, 15, 35,
    17, 100, 85, 200,
    200, 100, 77};
static          const int food_price[NUM_KINDS] = {
    4, 6, 8, 10,
    12, 12, 5, 6,
    5, 20, 15, 23,
    23, 10, 19};
static const char * const attack_type[NUM_KINDS] = {
    "��", "�@��", "�l", "�r",
    "����", "��", "��", "��",
    "�r", "�U�N", "�t��", "�ҥ�",
    "�C��", "�N����u", "���k�@�T"};

static const char * const damage_degree[] = {
    "�A�l����", "���o����", "�p�O��", "���L��",
    "���I�k��", "�ϤO��", "�ˤH��", "������",
    "�ϥ��O��", "�c������", "�M�I��", "�ƨg��",
    "�r�P��", "�g���ɫB����", "��Ѱʦa��",
    "�P�R��", NULL};

enum {
    OO, FOOD, WEIGHT, CLEAN, RUN, ATTACK, BOOK, HAPPY, SATIS,
    TEMPERAMENT, TIREDSTRONG, SICK, HP_MAX, MM_MAX
};

static          const short time_change[NUM_KINDS][14] =
/* �ɫ~ ���� �魫 ���b �ӱ� �����O ���� �ּ� ���N ��� �h�� �f�� ���� ���k */
{
    /* �� */
    {1, 1, 30, 3, 8, 3, 3, 40, 9, 1, 7, 3, 30, 1},
    /* ���֤k */
    {1, 1, 110, 1, 4, 7, 41, 20, 9, 25, 25, 7, 110, 15},
    /* �i�h */
    {1, 1, 200, 5, 4, 10, 33, 20, 15, 10, 27, 1, 200, 9},
    /* �j�� */
    {1, 1, 10, 5, 8, 1, 1, 5, 3, 1, 4, 1, 10, 30},
    /* ���s */
    {1, 1, 1000, 9, 1, 13, 4, 12, 3, 1, 200, 1, 1000, 3},
    /* ���N */
    {1, 1, 90, 7, 10, 7, 4, 12, 3, 30, 20, 5, 90, 20},
    /* �� */
    {1, 1, 30, 5, 5, 6, 4, 8, 3, 15, 7, 4, 30, 21},
    /* �����p�s */
    {1, 1, 100, 9, 7, 7, 20, 50, 10, 8, 24, 4, 100, 9},
    /* �� */
    {1, 1, 45, 8, 7, 9, 3, 40, 20, 3, 9, 5, 45, 1},
    /* �c�] */
    {1, 1, 45, 10, 11, 11, 5, 21, 11, 1, 9, 5, 45, 25},
    /* �Ԫ� */
    {1, 1, 45, 2, 12, 10, 25, 1, 1, 10, 9, 5, 45, 26},
    /* ���� */
    {1, 1, 150, 4, 8, 13, 95, 25, 7, 10, 25, 5, 175, 85},
    /* ���^�E */
    {1, 1, 147, 2, 10, 10, 85, 20, 4, 25, 25, 5, 145, 95},
    /* �N�i�H */
    {1, 1, 200, 3, 15, 15, 50, 50, 10, 5, 10, 2, 300, 0},
    /* ù�Q */
    {1, 1, 80, 2, 9, 10, 2, 5, 7, 8, 12, 1, 135, 5},
};

int
reload_chicken(void)
{
    userec_t xuser;
    chicken_t *mychicken = &cuser.mychicken;

    passwd_query(usernum, &xuser);
    memcpy(mychicken, &xuser.mychicken, sizeof(chicken_t));
    if (!mychicken->name[0])
	return 0;
    else
	return 1;
}

#define CHICKENLOG  "etc/chicken"

static int
new_chicken(void)
{
    chicken_t *mychicken = &cuser.mychicken;
    int             price, i;

    clear();
    move(2, 0);
    outs("�w����{ " ANSI_COLOR(33) "��" ANSI_COLOR(37;44) " Ptt�d������ " ANSI_COLOR(33;40) "��" ANSI_RESET ".. "
	 "�ثe�J���G\n"
	 "(a)�p�� $5   (b)���֤k $25  (c)�i�h    $30  (d)�j��  $40  "
	 "(e)���s $80\n"
	 "(f)���N $50  (g)��     $15  (h)�����p�s$35  (i)����  $17  "
	 "(j)�c�] $100\n"
	 "(k)�Ԫ� $85  (l)����   $200 (m)���^�E  $200 (n)�N�i�H$100 "
	 "[o]ù�� $77\n"
	 "[0]�ۤv $0\n");
    i = getans("�п�ܧA�n�i���ʪ��G");

    i -= 'a';
    if (i < 0 || i > NUM_KINDS - 1)
	return 0;

    mychicken->type = i;

    reload_money();
    price = egg_price[(int)mychicken->type];
    if (cuser.money < price) {
	vmsg("�������R�J�J,�J�J�n %d ��", price);
	return 0;
    }
    vice(price, "�d���J");
    while (strlen(mychicken->name) < 3)
	getdata(8, 0, "���e���Ӧn�W�r�G", mychicken->name,
		sizeof(mychicken->name), DOECHO);

    log_file(CHICKENLOG, LOG_CREAT | LOG_VF,
              ANSI_COLOR(31) "%s " ANSI_RESET "�i�F�@���s" ANSI_COLOR(33) " %s " ANSI_RESET "�� "
              ANSI_COLOR(32) "%s" ANSI_RESET "  �� %s\n", cuser.userid,
              mychicken->name, chicken_type[(int)mychicken->type], ctime4(&now));
    mychicken->lastvisit = mychicken->birthday = mychicken->cbirth = now;
    mychicken->food = 0;
    mychicken->weight = time_change[(int)mychicken->type][WEIGHT] / 3;
    mychicken->clean = 0;
    mychicken->run = time_change[(int)mychicken->type][RUN];
    mychicken->attack = time_change[(int)mychicken->type][ATTACK];
    mychicken->book = time_change[(int)mychicken->type][BOOK];
    mychicken->happy = time_change[(int)mychicken->type][HAPPY];
    mychicken->satis = time_change[(int)mychicken->type][SATIS];
    mychicken->temperament = time_change[(int)mychicken->type][TEMPERAMENT];
    mychicken->tiredstrong = 0;
    mychicken->sick = 0;
    mychicken->hp = time_change[(int)mychicken->type][WEIGHT];
    mychicken->hp_max = time_change[(int)mychicken->type][WEIGHT];
    mychicken->mm = 0;
    mychicken->mm_max = 0;
    return 1;
}

static void
show_chicken_stat(const chicken_t * thechicken, int age)
{
    struct tm      *ptime;

    ptime = localtime4(&thechicken->birthday);
    prints(" Name :" ANSI_COLOR(33) "%s" ANSI_RESET " (" ANSI_COLOR(32) "%s" ANSI_RESET ")%*s�ͤ�  "
	   ":" ANSI_COLOR(31) "%02d" ANSI_RESET "�~" ANSI_COLOR(31) "%2d" ANSI_RESET "��" ANSI_COLOR(31) "%2d" ANSI_RESET "�� "
	   "(" ANSI_COLOR(32) "%s %d��" ANSI_RESET ")\n"
	   " ��:" ANSI_COLOR(33) "%5d/%-5d" ANSI_RESET " �k:" ANSI_COLOR(33) "%5d/%-5d" ANSI_RESET " �����O:"
	   ANSI_COLOR(33) "%-7d" ANSI_RESET " �ӱ�  :" ANSI_COLOR(33) "%-7d" ANSI_RESET " ���� :" ANSI_COLOR(33) "%-7d"
	   ANSI_RESET " \n"
	   " �ּ� :" ANSI_COLOR(33) "%-7d" ANSI_RESET " ���N :" ANSI_COLOR(33) "%-7d" ANSI_RESET " �h��  :"
	   ANSI_COLOR(33) "%-7d" ANSI_RESET " ���  :" ANSI_COLOR(33) "%-7d " ANSI_RESET "�魫 :"
	   ANSI_COLOR(33) "%-5.2f" ANSI_RESET " \n"
	   " �f�� :" ANSI_COLOR(33) "%-7d" ANSI_RESET " ���b :" ANSI_COLOR(33) "%-7d" ANSI_RESET " ����  :"
	   ANSI_COLOR(33) "%-7d" ANSI_RESET " �j�ɤY:" ANSI_COLOR(33) "%-7d" ANSI_RESET " �ī~ :" ANSI_COLOR(33) "%-7d"
	   ANSI_RESET " \n",
	   thechicken->name, chicken_type[(int)thechicken->type],
	   (int)(15 - strlen(thechicken->name)), "",
	   ptime->tm_year % 100, ptime->tm_mon + 1, ptime->tm_mday,
	 cage[age > 16 ? 16 : age], age, thechicken->hp, thechicken->hp_max,
	   thechicken->mm, thechicken->mm_max,
	   thechicken->attack, thechicken->run, thechicken->book,
	   thechicken->happy, thechicken->satis, thechicken->tiredstrong,
	   thechicken->temperament,
	   ((float)(thechicken->hp_max + (thechicken->weight / 50))) / 100,
	   thechicken->sick, thechicken->clean, thechicken->food,
	   thechicken->oo, thechicken->medicine);
}

#define CHICKEN_PIC "etc/chickens"

void
show_chicken_data(chicken_t * thechicken, chicken_t * pkchicken)
{
    char            buf[1024];
    int age = ((now - thechicken->cbirth) / (60 * 60 * 24));
    if (age < 0) {
	thechicken->birthday = thechicken->cbirth = now - 10 * (60 * 60 * 24);
	age = 10;
    }
    /* Ptt:debug */
    thechicken->type %= NUM_KINDS;
    clear();
    showtitle(pkchicken ? "��tt������" : "��tt�i����", BBSName);
    move(1, 0);

    show_chicken_stat(thechicken, age);

    snprintf(buf, sizeof(buf), CHICKEN_PIC "/%c%d", thechicken->type + 'a',
	     age > 16 ? 16 : age);
    show_file(buf, 5, 14, NO_RELOAD);

    move(18, 0);

    if (thechicken->sick)
	outs("�ͯf�F...");
    if (thechicken->sick > thechicken->hp / 5)
	outs(ANSI_COLOR(5;31) "���...�f��!!" ANSI_RESET);

    if (thechicken->clean > 150)
	outs(ANSI_COLOR(31) "�S��Sż��.." ANSI_RESET);
    else if (thechicken->clean > 80)
	outs("���Iż..");
    else if (thechicken->clean < 20)
	outs(ANSI_COLOR(32) "�ܰ��b.." ANSI_RESET);

    if (thechicken->weight > thechicken->hp_max * 4)
	outs(ANSI_COLOR(31) "�ֹ����F!." ANSI_RESET);
    else if (thechicken->weight > thechicken->hp_max * 3)
	outs(ANSI_COLOR(32) "���ʹ�.." ANSI_RESET);
    else if (thechicken->weight < (thechicken->hp_max / 4))
	outs(ANSI_COLOR(31) "�־j���F!.." ANSI_RESET);
    else if (thechicken->weight < (thechicken->hp_max / 2))
	outs("�j�F..");

    if (thechicken->tiredstrong > thechicken->hp * 1.7)
	outs(ANSI_COLOR(31) "�ֱo���g�F..." ANSI_RESET);
    else if (thechicken->tiredstrong > thechicken->hp)
	outs("�֤F..");
    else if (thechicken->tiredstrong < thechicken->hp / 4)
	outs(ANSI_COLOR(32) "��O����..." ANSI_RESET);

    if (thechicken->hp < thechicken->hp_max / 4)
	outs(ANSI_COLOR(31) "��O�κ�..�a�a�@��.." ANSI_RESET);
    if (thechicken->happy > 500)
	outs(ANSI_COLOR(32) "�ܧּ�.." ANSI_RESET);
    else if (thechicken->happy < 100)
	outs("���ּ�..");
    if (thechicken->satis > 500)
	outs(ANSI_COLOR(32) "�ܺ���.." ANSI_RESET);
    else if (thechicken->satis < 50)
	outs("������..");

    if (pkchicken) {
	outc('\n');
	show_chicken_stat(pkchicken, age);
	outs("[���N��] ������� [q] ���] [o] �Y�j�ɤY");
    }
}

static void
ch_eat(void)
{
    chicken_t *mychicken = &cuser.mychicken;
    if (mychicken->food) {
	mychicken->weight += time_change[(int)mychicken->type][WEIGHT] +
	mychicken->hp_max / 5;
	mychicken->tiredstrong +=
	    time_change[(int)mychicken->type][TIREDSTRONG] / 2;
	mychicken->hp_max++;
	mychicken->happy += 5;
	mychicken->satis += 7;
	mychicken->food--;
	move(10, 10);

	show_file(CHICKEN_PIC "/eat", 5, 14, NO_RELOAD);
	pressanykey();
    }
}

static void
ch_clean(void)
{
    chicken_t *mychicken = &cuser.mychicken;
    mychicken->clean = 0;
    mychicken->tiredstrong +=
	time_change[(int)mychicken->type][TIREDSTRONG] / 3;
    show_file(CHICKEN_PIC "/clean", 5, 14, NO_RELOAD);
    pressanykey();
}

static void
ch_guess(void)
{
    char           *guess[3] = {"�ŤM", "���Y", "��"}, me, ch, win;

    chicken_t *mychicken = &cuser.mychicken;
    mychicken->happy += time_change[(int)mychicken->type][HAPPY] * 1.5;
    mychicken->satis += time_change[(int)mychicken->type][SATIS];
    mychicken->tiredstrong += time_change[(int)mychicken->type][TIREDSTRONG];
    mychicken->attack += time_change[(int)mychicken->type][ATTACK] / 4;
    move(20, 0);
    clrtobot();
    outs("�A�n�X[" ANSI_COLOR(32) "1" ANSI_RESET "]" ANSI_COLOR(33) "�ŤM" ANSI_RESET "(" ANSI_COLOR(32) "2" ANSI_RESET ")"
	 ANSI_COLOR(33) "���Y" ANSI_RESET "(" ANSI_COLOR(32) "3" ANSI_RESET ")" ANSI_COLOR(33) "��" ANSI_RESET ":\n");
    me = igetch();
    me -= '1';
    if (me > 2 || me < 0)
	me = 0;
    win = (int)(3.0 * random() / (RAND_MAX + 1.0)) - 1;
    ch = (me + win + 3) % 3;
    prints("%s:%s !      %s:%s !.....%s",
	   cuser.userid, guess[(int)me], mychicken->name, guess[(int)ch],
	   win == 0 ? "����" : win < 0 ? "�C..Ĺ�F :D!!" : "��..�ڿ�F :~");
    pressanykey();
}

static void
ch_book(void)
{
    chicken_t *mychicken = &cuser.mychicken;
    mychicken->book += time_change[(int)mychicken->type][BOOK];
    mychicken->tiredstrong += time_change[(int)mychicken->type][TIREDSTRONG];
    show_file(CHICKEN_PIC "/read", 5, 14, NO_RELOAD);
    pressanykey();
}

static void
ch_kiss(void)
{
    chicken_t *mychicken = &cuser.mychicken;
    mychicken->happy += time_change[(int)mychicken->type][HAPPY];
    mychicken->satis += time_change[(int)mychicken->type][SATIS];
    mychicken->tiredstrong +=
	time_change[(int)mychicken->type][TIREDSTRONG] / 2;
    show_file(CHICKEN_PIC "/kiss", 5, 14, NO_RELOAD);
    pressanykey();
}

static void
ch_hit(void)
{
    chicken_t *mychicken = &cuser.mychicken;
    mychicken->attack += time_change[(int)mychicken->type][ATTACK];
    mychicken->run += time_change[(int)mychicken->type][RUN];
    mychicken->mm_max += time_change[(int)mychicken->type][MM_MAX] / 15;
    mychicken->weight -= mychicken->hp_max / 15;
    mychicken->hp -= (int)((float)time_change[(int)mychicken->type][HP_MAX] *
			   random() / (RAND_MAX + 1.0)) / 2 + 1;

    if (mychicken->book > 2)
	mychicken->book -= 2;
    if (mychicken->happy > 2)
	mychicken->happy -= 2;
    if (mychicken->satis > 2)
	mychicken->satis -= 2;
    mychicken->tiredstrong += time_change[(int)mychicken->type][TIREDSTRONG];
    show_file(CHICKEN_PIC "/hit", 5, 14, NO_RELOAD);
    pressanykey();
}

void
ch_buyitem(int money, const char *picture, int *item, int haveticket)
{
    int             num = 0;
    char            buf[5];

    getdata_str(b_lines - 1, 0, "�n�R�h�֥��O:",
		buf, sizeof(buf), DOECHO, "1");
    num = atoi(buf);
    if (num < 1)
	return;
    reload_money();
    if (cuser.money/money >= num) {
	*item += num;
	if( haveticket )
	    vice(money * num, "�ʶR�d��,��L����");
	else
	    demoney(-money * num);
	show_file(picture, 5, 14, NO_RELOAD);
        pressanykey();
    } else {
	vmsg("�{������ !!!");
    }
    usleep(100000); // sleep 0.1s
}

static void
ch_eatoo(void)
{
    chicken_t *mychicken = &cuser.mychicken;
    if (mychicken->oo > 0) {
	mychicken->oo--;
	mychicken->tiredstrong = 0;
	if (mychicken->happy > 5)
	    mychicken->happy -= 5;
	show_file(CHICKEN_PIC "/oo", 5, 14, NO_RELOAD);
	pressanykey();
    }
}

static void
ch_eatmedicine(void)
{
    chicken_t *mychicken = &cuser.mychicken;
    if (mychicken->medicine > 0) {
	mychicken->medicine--;
	mychicken->sick = 0;
	if (mychicken->hp_max > 10)
	    mychicken->hp_max -= 3;
	mychicken->hp = mychicken->hp_max;
	if (mychicken->happy > 10)
	    mychicken->happy -= 10;
	show_file(CHICKEN_PIC "/medicine", 5, 14, NO_RELOAD);
	pressanykey();
    }
}

static void
ch_kill(void)
{
    chicken_t *mychicken = &cuser.mychicken;
    int        ans;

    ans = getans("��i�n�Q�@ 100 ��, �O�_�n��i?(y/N)");
    if (ans == 'y') {

	vice(100, "��i�d���O");
	more(CHICKEN_PIC "/deadth", YEA);
	log_file(CHICKENLOG, LOG_CREAT | LOG_VF,
		 ANSI_COLOR(31) "%s " ANSI_RESET "�� " ANSI_COLOR(33) "%s" ANSI_RESET ANSI_COLOR(32) " %s "
		 ANSI_RESET "�_�F �� %s\n", cuser.userid, mychicken->name,
		 chicken_type[(int)mychicken->type], ctime4(&now));
	mychicken->name[0] = 0;
    }
}

static int
ch_sell(int age)
{
    chicken_t *mychicken = &cuser.mychicken;
    /*
     * int money = (mychicken->weight -
     * time_change[(int)mychicken->type][WEIGHT])
     * (food_price[(int)mychicken->type])/4 + ( + ((mychicken->clean /
     * time_change[(int)mychicken->type][CLEAN]) + (mychicken->run /
     * time_change[(int)mychicken->type][RUN]) + (mychicken->attack /
     * time_change[(int)mychicken->type][ATTACK]) + (mychicken->book /
     * time_change[(int)mychicken->type][BOOK]) + (mychicken->happy /
     * time_change[(int)mychicken->type][HAPPY]) + (mychicken->satis /
     * time_change[(int)mychicken->type][SATIS]) + (mychicken->temperament /
     * time_change[(int)mychicken->type][TEMPERAMENT]) -
     * (mychicken->tiredstrong /
     * time_change[(int)mychicken->type][TIREDSTRONG]) - (mychicken->sick /
     * time_change[(int)mychicken->type][SICK]) + (mychicken->hp /
     * time_change[(int)mychicken->type][HP_MAX]) + (mychicken->mm /
     * time_change[(int)mychicken->type][MM_MAX]) + 7 - abs(age - 7)) * 3 ;
     */
    int             money = (age * food_price[(int)mychicken->type] * 3
			     + (mychicken->hp_max * 10 + mychicken->weight) /
			time_change[(int)mychicken->type][HP_MAX]) * 3 / 2 -
    mychicken->sick, ans;

    if (money < 0)
	money = 0;
    else if (money > MAX_CHICKEN_MONEY)
	money = MAX_CHICKEN_MONEY;
    //�������
    if (mychicken->type == 1 || mychicken->type == 7) {
	outs("\n" ANSI_COLOR(31) " ��..�˷R��..�c��H�f�O�|�Ǫk����.." ANSI_RESET);
	pressanykey();
	return 0;
    }
    if (age < 5) {
	outs("\n �٥����~�����");
	pressanykey();
	return 0;
    }
    if (age > 30) {
	outs("\n" ANSI_COLOR(31) " �o..�ӦѨS�H�n�F" ANSI_RESET);
	pressanykey();
	return 0;
    }
    ans = getans("�o��%d��%s�i�H�� %d ��, �O�_�n��?(y/N)", age, 
                 chicken_type[(int)mychicken->type], money);
    if (ans == 'y') {
	log_file(CHICKENLOG, LOG_CREAT | LOG_VF,
		 ANSI_COLOR(31) "%s" ANSI_RESET " �� " ANSI_COLOR(33) "%s" ANSI_RESET " "
                 ANSI_COLOR(32) "%s" ANSI_RESET " �� " ANSI_COLOR(36) "%d" ANSI_RESET " ��F �� %s\n",
                 cuser.userid, mychicken->name, 
                 chicken_type[(int)mychicken->type], money, ctime4(&now));
	mychicken->lastvisit = mychicken->name[0] = 0;
	passwd_update(usernum, &cuser);
	more(CHICKEN_PIC "/sell", YEA);
	demoney(money);
	return 1;
    }
    return 0;
}

static void
geting_old(int *hp, int *weight, int diff, int age)
{
    float           ex = 0.9;

    if (age > 70)
	ex = 0.1;
    else if (age > 30)
	ex = 0.5;
    else if (age > 20)
	ex = 0.7;

    diff /= 60 * 6;
    while (diff--) {
	*hp *= ex;
	*weight *= ex;
    }
}

/* �̮ɶ��ܰʪ���� */
void
time_diff(chicken_t * thechicken)
{
    int             diff;
    int             theage = ((now - thechicken->cbirth) / (60 * 60 * 24));

    thechicken->type %= NUM_KINDS;
    diff = (now - thechicken->lastvisit) / 60;

    if ((diff) < 1)
	return;

    if (theage > 13)		/* �Ѧ� */
	geting_old(&thechicken->hp_max, &thechicken->weight, diff, theage);

    thechicken->lastvisit = now;
    thechicken->weight -= thechicken->hp_max * diff / 540;	/* �魫 */
    if (thechicken->weight < 1) {
	thechicken->sick -= thechicken->weight / 10;	/* �j�o�f��W�� */
	thechicken->weight = 1;
    }
    /* �M��� */
    thechicken->clean += diff * time_change[(int)thechicken->type][CLEAN] / 30;

    /* �ּ֫� */
    thechicken->happy -= diff / 60;
    if (thechicken->happy < 0)
	thechicken->happy = 0;
    thechicken->attack -=
	time_change[(int)thechicken->type][ATTACK] * diff / (60 * 32);
    if (thechicken->attack < 0)
	thechicken->attack = 0;
    /* �����O */
    thechicken->run -= time_change[(int)thechicken->type][RUN] * diff / (60 * 32);
    /* �ӱ� */
    if (thechicken->run < 0)
	thechicken->run = 0;
    thechicken->book -= time_change[(int)thechicken->type][BOOK] * diff / (60 * 32);
    /* ���� */
    if (thechicken->book < 0)
	thechicken->book = 0;
    /* ��� */
    thechicken->temperament++;

    thechicken->satis -= diff / 60 / 3 * time_change[(int)thechicken->type][SATIS];
    /* ���N�� */
    if (thechicken->satis < 0)
	thechicken->satis = 0;

    /* ż�f�� */
    if (thechicken->clean > 1000)
	thechicken->sick += (thechicken->clean - 400) / 10;

    if (thechicken->weight > 1)
	thechicken->sick -= diff / 60;
    /* �f����@ */
    if (thechicken->sick < 0)
	thechicken->sick = 0;
    thechicken->tiredstrong -= diff *
	time_change[(int)thechicken->type][TIREDSTRONG] / 4;
    /* �h�� */
    if (thechicken->tiredstrong < 0)
	thechicken->tiredstrong = 0;
    /* hp_max */
    if (thechicken->hp >= thechicken->hp_max / 2)
	thechicken->hp_max +=
	    time_change[(int)thechicken->type][HP_MAX] * diff / (60 * 12);
    /* hp���@ */
    if (!thechicken->sick)
	thechicken->hp +=
	    time_change[(int)thechicken->type][HP_MAX] * diff / (60 * 6);
    if (thechicken->hp > thechicken->hp_max)
	thechicken->hp = thechicken->hp_max;
    /* mm_max */
    if (thechicken->mm >= thechicken->mm_max / 2)
	thechicken->mm_max +=
	    time_change[(int)thechicken->type][MM_MAX] * diff / (60 * 8);
    /* mm���@ */
    if (!thechicken->sick)
	thechicken->mm += diff;
    if (thechicken->mm > thechicken->mm_max)
	thechicken->mm = thechicken->mm_max;
}

static void
check_sick(void)
{
    chicken_t *mychicken = &cuser.mychicken;
    /* ż�f�� */
    if (mychicken->tiredstrong > mychicken->hp * 0.3 && mychicken->clean > 150)
	mychicken->sick += (mychicken->clean - 150) / 10;
    /* �֯f�� */
    if (mychicken->tiredstrong > mychicken->hp * 1.3)
	mychicken->sick += time_change[(int)mychicken->type][SICK];
    /* �f��ӭ��ٰ��ƴ�hp */
    if (mychicken->sick > mychicken->hp / 5) {
	mychicken->hp -= (mychicken->sick - mychicken->hp / 5) / 4;
	if (mychicken->hp < 0)
	    mychicken->hp = 0;
    }
}

static int
deadtype(const chicken_t * thechicken)
{
    chicken_t *mychicken = &cuser.mychicken;
    int             i;

    if (thechicken->hp <= 0)	/* hp�κ� */
	i = 1;
    else if (thechicken->tiredstrong > thechicken->hp * 3)	/* �޳ҹL�� */
	i = 2;
    else if (thechicken->weight > thechicken->hp_max * 5)	/* �έD�L�� */
	i = 3;
    else if (thechicken->weight == 1 &&
	     thechicken->sick > thechicken->hp_max / 4)
	i = 4;			/* �j���F */
    else if (thechicken->satis <= 0)	/* �ܤ����N */
	i = 5;
    else
	return 0;

    if (thechicken == mychicken) {
	log_file(CHICKENLOG, LOG_CREAT | LOG_VF,
                 ANSI_COLOR(31) "%s" ANSI_RESET " �үk�R��" ANSI_COLOR(33) " %s" ANSI_COLOR(32) " %s "
                 ANSI_RESET "���F �� %s\n", cuser.userid, thechicken->name,
                 chicken_type[(int)thechicken->type], ctime4(&now));
	mychicken->name[0] = 0;
	passwd_update(usernum, &cuser);
    }
    return i;
}

int
showdeadth(int type)
{
    switch (type) {
	case 1:
	more(CHICKEN_PIC "/nohp", YEA);
	break;
    case 2:
	more(CHICKEN_PIC "/tootired", YEA);
	break;
    case 3:
	more(CHICKEN_PIC "/toofat", YEA);
	break;
    case 4:
	more(CHICKEN_PIC "/nofood", YEA);
	break;
    case 5:
	more(CHICKEN_PIC "/nosatis", YEA);
	break;
    default:
	return 0;
    }
    more(CHICKEN_PIC "/deadth", YEA);
    return type;
}

int
isdeadth(const chicken_t * thechicken)
{
    int             i;

    if (!(i = deadtype(thechicken)))
	return 0;
    return showdeadth(i);
}

static void
ch_changename(void)
{
    chicken_t *mychicken = &cuser.mychicken;
    char      newname[20] = "";

    getdata_str(b_lines - 1, 0, "��..��Ӧn�W�r�a:", newname, 18, DOECHO,
		mychicken->name);

    if (strlen(newname) >= 3 && strcmp(newname, mychicken->name)) {
	strlcpy(mychicken->name, newname, sizeof(mychicken->name));
	log_file(CHICKENLOG, LOG_CREAT | LOG_VF, 
                ANSI_COLOR(31) "%s" ANSI_RESET " ��k�R��" ANSI_COLOR(33) " %s" ANSI_COLOR(32) " %s "
                ANSI_RESET "��W��" ANSI_COLOR(33) " %s" ANSI_RESET " �� %s\n",
                 cuser.userid, mychicken->name,
                 chicken_type[(int)mychicken->type], newname, ctime4(&now));
    }
}

static int
select_menu(int age)
{
    chicken_t *mychicken = &cuser.mychicken;
    char            ch;

    reload_money();
    move(19, 0);
    prints(ANSI_COLOR(44;37) " �� :" ANSI_COLOR(33) " %-10d                                  "
	   "                       " ANSI_RESET "\n"
	   ANSI_COLOR(33) "(" ANSI_COLOR(37) "1" ANSI_COLOR(33) ")�M�z (" ANSI_COLOR(37) "2" ANSI_COLOR(33) ")�Y�� "
	   "(" ANSI_COLOR(37) "3" ANSI_COLOR(33) ")�q�� (" ANSI_COLOR(37) "4" ANSI_COLOR(33) ")��� "
	   "(" ANSI_COLOR(37) "5" ANSI_COLOR(33) ")�˥L (" ANSI_COLOR(37) "6" ANSI_COLOR(33) ")���L "
	   "(" ANSI_COLOR(37) "7" ANSI_COLOR(33) ")�R%s$%d (" ANSI_COLOR(37) "8" ANSI_COLOR(33) ")�Y�ɤY\n"
	   "(" ANSI_COLOR(37) "9" ANSI_COLOR(33) ")�Y�f�� (" ANSI_COLOR(37) "o" ANSI_COLOR(33) ")�R�j�ɤY$100 "
	   "(" ANSI_COLOR(37) "m" ANSI_COLOR(33) ")�R��$10 (" ANSI_COLOR(37) "k" ANSI_COLOR(33) ")��i "
	   "(" ANSI_COLOR(37) "s" ANSI_COLOR(33) ")�汼 (" ANSI_COLOR(37) "n" ANSI_COLOR(33) ")��W "
	   "(" ANSI_COLOR(37) "q" ANSI_COLOR(33) ")���}:" ANSI_RESET,
	   cuser.money,
    /*
     * chicken_food[(int)mychicken->type],
     * chicken_type[(int)mychicken->type],
     * chicken_type[(int)mychicken->type],
     */
	   chicken_food[(int)mychicken->type],
	   food_price[(int)mychicken->type]);
    do {
	switch (ch = igetch()) {
	case '1':
	    ch_clean();
	    check_sick();
	    break;
	case '2':
	    ch_eat();
	    check_sick();
	    break;
	case '3':
	    ch_guess();
	    check_sick();
	    break;
	case '4':
	    ch_book();
	    check_sick();
	    break;
	case '5':
	    ch_kiss();
	    break;
	case '6':
	    ch_hit();
	    check_sick();
	    break;
	case '7':
	    ch_buyitem(food_price[(int)mychicken->type], CHICKEN_PIC "/food",
		       &mychicken->food, 1);
	    break;
	case '8':
	    ch_eatoo();
	    break;
	case '9':
	    ch_eatmedicine();
	    break;
	case 'O':
	case 'o':
	    ch_buyitem(100, CHICKEN_PIC "/buyoo", &mychicken->oo, 1);
	    break;
	case 'M':
	case 'm':
	    ch_buyitem(10, CHICKEN_PIC "/buymedicine", &mychicken->medicine, 1);
	    break;
	case 'N':
	case 'n':
	    ch_changename();
	    break;
	case 'K':
	case 'k':
	    ch_kill();
	    return 0;
	case 'S':
	case 's':
	    if (!ch_sell(age))
		break;
	case 'Q':
	case 'q':
	    return 0;
	}
    } while (ch < ' ' || ch > 'z');
    return 1;
}

static int
recover_chicken(chicken_t * thechicken)
{
    char            buf[200];
    int             price = egg_price[(int)thechicken->type], money = price + (random() % price);

    if (now - thechicken->lastvisit > (60 * 60 * 24 * 7))
	return 0;
    outmsg(ANSI_COLOR(33;44) "���F�ɦu��" ANSI_COLOR(37;45) " �O�`�� �ڬO�����A�� " ANSI_RESET);
    bell();
    igetch();
    outmsg(ANSI_COLOR(33;44) "���F�ɦu��" ANSI_COLOR(37;45) " �A�L�k���ڤ��y �]���ڬO�t�F, "
	   "�̪�ʿ��Q�ȥ~�� " ANSI_RESET);
    bell();
    igetch();
    snprintf(buf, sizeof(buf), ANSI_COLOR(33;44) "���F�ɦu��" ANSI_COLOR(37;45) " "
	     "�A���@�ӭ訫���[��%s�n�۴��^�Ӷ�? �u�n%d���� " ANSI_RESET,
	     chicken_type[(int)thechicken->type], price * 2);
    outmsg(buf);
    bell();
    getdata_str(21, 0, "    ��ܡG(N:�|�H��/y:��������)", buf, 3, LCECHO, "N");
    if (buf[0] == 'y' || buf[0] == 'Y') {
	reload_money();
	if (cuser.money < price * 2) {
	    outmsg(ANSI_COLOR(33;44) "���F�ɦu��" ANSI_COLOR(37;45) " ���� ���S�a�� "
		   "�S�����p�� �֥h�w���a " ANSI_RESET);
	    bell();
	    igetch();
	    return 0;
	}
	strlcpy(thechicken->name, "[�ߦ^�Ӫ�]", sizeof(thechicken->name));
	thechicken->hp = thechicken->hp_max;
	thechicken->sick = 0;
	thechicken->satis = 2;
	vice(money, "�F�ɦu��");
	snprintf(buf, sizeof(buf),
		 ANSI_COLOR(33;44) "���F�ɦu��" ANSI_COLOR(37;45) " OK�F �O�o���L�I�F�� "
		 "���M�i�ॢ�� ���b�ڤ]����Ptt ���A%d�N�n " ANSI_RESET, money);
	outmsg(buf);
	bell();
	igetch();
	return 1;
    }
    outmsg(ANSI_COLOR(33;44) "���F�ɦu��" ANSI_COLOR(37;45) " ���M���ڧ|�H! �o�~�Y�R�u���ȿ� "
	   "���D�ڦA�ӧ�A �A�A�]�S���|�F " ANSI_RESET);
    bell();
    igetch();
    thechicken->lastvisit = 0;
    passwd_update(usernum, &cuser);
    return 0;
}

#define lockreturn0(unmode, state) if(lockutmpmode(unmode, state)) return 0

int
chicken_main(void)
{
    chicken_t *mychicken = &cuser.mychicken;
    int age;
    lockreturn0(CHICKEN, LOCK_MULTI);
    reload_chicken();
    if (!mychicken->name[0] && !recover_chicken(mychicken) && !new_chicken()) {
	unlockutmpmode();
	return 0;
    }
    age = ((now - mychicken->cbirth) / (60 * 60 * 24));
    do {
	time_diff(mychicken);
	if (isdeadth(mychicken))
	    break;
	show_chicken_data(mychicken, NULL);
    } while (select_menu(age));
    reload_money();
    passwd_update(usernum, &cuser);
    unlockutmpmode();
    return 0;
}

int
chickenpk(int fd)
{
    chicken_t *mychicken = &cuser.mychicken;
    char            mateid[IDLEN + 1], data[200], buf[200];
    int             ch = 0;

    userinfo_t     *uin = &SHM->uinfo[currutmp->destuip];
    userec_t        ouser;
    chicken_t      *ochicken = &ouser.mychicken;
    int             r, attmax, i, datac, duid = currutmp->destuid, catched = 0,
                    count = 0;

    lockreturn0(CHICKEN, LOCK_MULTI);

    strlcpy(mateid, currutmp->mateid, sizeof(mateid));
    /* ���⪺id��local buffer�O�� */

    getuser(mateid, &ouser);
    reload_chicken();
    if (!ochicken->name[0] || !mychicken->name[0]) {
	bell();
	vmsg("���@��S���d��");	/* Ptt:����page�ɧ��d���汼 */
	add_io(0, 0);
	close(fd);
	unlockutmpmode();
	return 0;
    }
    show_chicken_data(ochicken, mychicken);
    add_io(fd, 3);		/* ��fd�[��igetch�ʵ� */
    while (1) {
	r = random();
	ch = igetch();
	getuser(mateid, &ouser);
	reload_chicken();
	show_chicken_data(ochicken, mychicken);
	time_diff(mychicken);

	i = mychicken->attack * mychicken->hp / mychicken->hp_max;
	for (attmax = 2; (i = i * 9 / 10); attmax++);

	if (ch == I_OTHERDATA) {
	    count = 0;
	    datac = recv(fd, data, sizeof(data), 0);
	    if (datac <= 1)
		break;
	    move(17, 0);
	    outs(data + 1);
	    switch (data[0]) {
	    case 'c':
		catched = 1;
		move(16, 0);
		outs("�n��L����?(y/N)");
		break;
	    case 'd':
		move(16, 0);
		outs("��~�ˤU�F!!");
		break;
	    }
	    if (data[0] == 'd' || data[0] == 'q' || data[0] == 'l')
		break;
	    continue;
	} else if (currutmp->turn) {
	    count = 0;
	    currutmp->turn = 0;
	    uin->turn = 1;
	    mychicken->tiredstrong++;
	    switch (ch) {
	    case 'y':
		if (catched == 1) {
		    snprintf(data, sizeof(data),
			     "l�� %s ���]�F\n", ochicken->name);
		}
		break;
	    case 'n':
		catched = 0;
	    default:
	    case 'k':
		r = r % (attmax + 2);
		if (r) {
		    snprintf(data, sizeof(data),
			     "M%s %s%s %s �ˤF %d �I\n", mychicken->name,
			     damage_degree[r / 3 > 15 ? 15 : r / 3],
			     attack_type[(int)mychicken->type],
			     ochicken->name, r);
		    ochicken->hp -= r;
		} else
		    snprintf(data, sizeof(data),
			     "M%s ı�o��n�X���L��\n", mychicken->name);
		break;
	    case 'o':
		if (mychicken->oo > 0) {
		    mychicken->oo--;
		    mychicken->hp += 300;
		    if (mychicken->hp > mychicken->hp_max)
			mychicken->hp = mychicken->hp_max;
		    mychicken->tiredstrong = 0;
		    snprintf(data, sizeof(data), "M%s �Y�F���j�ɤY�ɥR��O\n",
			     mychicken->name);
		} else
		    snprintf(data, sizeof(data),
			    "M%s �Q�Y�j�ɤY, �i�O�S���j�ɤY�i�Y\n",
			    mychicken->name);
		break;
	    case 'q':
		if (r % (mychicken->run + 1) > r % (ochicken->run + 1))
		    snprintf(data, sizeof(data), "q%s ���]�F\n",
			     mychicken->name);
		else
		    snprintf(data, sizeof(data),
			     "c%s �Q���], ���Q %s ���F\n",
			     mychicken->name, ochicken->name);
		break;
	    }
	    if (deadtype(ochicken)) {
		strtok(data, "\n");
		strlcpy(buf, data, sizeof(buf));
		snprintf(data, sizeof(data), "d%s , %s �Q %s �����F\n",
			 buf + 1, ochicken->name, mychicken->name);
	    }
	    move(17, 0);
	    outs(data + 1);
	    i = strlen(data) + 1;
	    passwd_update(duid, &ouser);
	    passwd_update(usernum, &cuser);
	    send(fd, data, i, 0);
	    if (data[0] == 'q' || data[0] == 'd')
		break;
	} else {
	    move(17, 0);
	    if (count++ > 30)
		break;
	}
    }
    add_io(0, 0);		/* ��igetch��_�^ */
    pressanykey();
    close(fd);
    showdeadth(deadtype(mychicken));
    unlockutmpmode();
    return 0;
}
