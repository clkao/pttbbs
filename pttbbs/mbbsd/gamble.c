/* $Id$ */
#include "bbs.h"

#define MAX_ITEM	8	//�̤j �䶵(item) �Ӽ�
#define MAX_ITEM_LEN	30	//�̤j �C�@�䶵�W�r����
#define MAX_SUBJECT_LEN 650	//8*81 = 648 �̤j �D�D����

static int
load_ticket_record(const char *direct, int ticket[])
{
    char            buf[256];
    int             i, total = 0;
    FILE           *fp;
    snprintf(buf, sizeof(buf), "%s/" FN_TICKET_RECORD, direct);
    if (!(fp = fopen(buf, "r")))
	return 0;
    for (i = 0; i < MAX_ITEM && fscanf(fp, "%9d ", &ticket[i])==1; i++)
	total = total + ticket[i];
    fclose(fp);
    return total;
}

static int
show_ticket_data(char betname[MAX_ITEM][MAX_ITEM_LEN],const char *direct, int *price, const boardheader_t * bh)
{
    int             i, count, total = 0, end = 0, ticket[MAX_ITEM] = {0, 0, 0, 0, 0, 0, 0, 0};
    FILE           *fp;
    char            genbuf[256], t[25];

    clear();
    if (bh) {
	snprintf(genbuf, sizeof(genbuf), "%s ��L", bh->brdname);
	if (bh->endgamble && now < bh->endgamble &&
	    bh->endgamble - now < 3600) {
	    snprintf(t, sizeof(t),
		     "�ʽL�˼� %d ��", (int)(bh->endgamble - now));
	    showtitle(genbuf, t);
	} else
	    showtitle(genbuf, BBSNAME);
    } else
	showtitle(BBSMNAME "��L", BBSNAME);
    move(2, 0);
    snprintf(genbuf, sizeof(genbuf), "%s/" FN_TICKET_ITEMS, direct);
    if (!(fp = fopen(genbuf, "r"))) {
	outs("\n�ثe�èS���|���L\n");
	snprintf(genbuf, sizeof(genbuf), "%s/" FN_TICKET_OUTCOME, direct);
	more(genbuf, NA);
	return 0;
    }
    fgets(genbuf, MAX_ITEM_LEN, fp);
    *price = atoi(genbuf);
    for (count = 0; fgets(betname[count], MAX_ITEM_LEN, fp) && count < MAX_ITEM; count++) {
	chomp(betname[count]);
    }
    fclose(fp);

    prints(ANSI_COLOR(32) "���W:" ANSI_RESET 
     " 1.�i�ʶR�H�U���P�������m���C�C�i�n�� " ANSI_COLOR(32) "%d" 
	 ANSI_RESET " ���C\n"
"      2.%s\n"
"      3.�}���ɥu���@�رm������, ���ʶR�ӱm����, �h�i���ʶR���i�Ƨ����`����C\n"
"      4.�C�������Ѩt�Ω�� 5%% ���|��%s�C\n"
"      5." ANSI_COLOR(1;31) "�p�J�t�άG�ٳy���b���^�����U�ذ��D�A"
	 "��h�W�����H���v�A���I�۾�C" ANSI_RESET "\n"
	    ANSI_COLOR(32) "%s:" ANSI_RESET, *price,
	   bh ? "����L�ѪO�D�t�d�|��èM�w�}���ɶ����G, ���褣��, �@��A��C" :
	        "�t�ΨC�� 2:00 11:00 16:00 21:00 �}���C",
	   bh ? ", �䤤 2% �����}���O�D" : "",
	   bh ? "�O�D�ۭq�W�h�λ���" : "�e�X���}�����G");

    snprintf(genbuf, sizeof(genbuf), "%s/" FN_TICKET, direct);
    if (!dashf(genbuf)) {
	snprintf(genbuf, sizeof(genbuf), "%s/" FN_TICKET_END, direct);
	end = 1;
    }
    show_file(genbuf, 8, -1, SHOWFILE_ALLOW_ALL);
    move(15, 0);
    outs(ANSI_COLOR(1;32) "�ثe�U�`���p:" ANSI_RESET "\n");

    total = load_ticket_record(direct, ticket);

    outs(ANSI_COLOR(33));
    for (i = 0; i < count; i++) {
	prints("%d.%-8s: %-7d", i + 1, betname[i], ticket[i]);
	if (i == 3)
	    outc('\n');
    }
    prints(ANSI_RESET "\n" ANSI_COLOR(42) " �U�`�`���B:" ANSI_COLOR(31) " %d �� " ANSI_RESET, total * (*price));
    if (end) {
	outs("\n��L�w�g����U�`\n");
	return -count;
    }
    return count;
}

static int 
append_ticket_record(const char *direct, int ch, int n, int count)
{
    FILE           *fp;
    int             ticket[8] = {0, 0, 0, 0, 0, 0, 0, 0}, i;
    char            genbuf[256];

    snprintf(genbuf, sizeof(genbuf), "%s/" FN_TICKET, direct);
    if (!dashf(genbuf))
	return -1;

    snprintf(genbuf, sizeof(genbuf), "%s/" FN_TICKET_USER, direct);
    if ((fp = fopen(genbuf, "a"))) {
	fprintf(fp, "%s %d %d\n", cuser.userid, ch, n);
	fclose(fp);
    }

    snprintf(genbuf, sizeof(genbuf), "%s/" FN_TICKET_RECORD, direct);

    if (!dashf(genbuf)) {
	creat(genbuf, S_IRUSR | S_IWUSR);
    }

    if ((fp = fopen(genbuf, "r+"))) {

	flock(fileno(fp), LOCK_EX);

	for (i = 0; i < MAX_ITEM; i++)
	    if (fscanf(fp, "%9d ", &ticket[i]) != 1)
		break;
	ticket[ch] += n;

	ftruncate(fileno(fp), 0);
	rewind(fp);
	for (i = 0; i < count; i++)
	    fprintf(fp, "%d ", ticket[i]);
	fflush(fp);

	flock(fileno(fp), LOCK_UN);
	fclose(fp);
    }
    return 0;
}

#define lockreturn0(unmode, state) if(lockutmpmode(unmode, state)) return 0
int
ticket(int bid)
{
    int             ch, end = 0;
    int	            n, price, count; /* �ʶR�i�ơB����B�ﶵ�� */
    char            path[128], fn_ticket[PATHLEN];
    char            betname[MAX_ITEM][MAX_ITEM_LEN];
    boardheader_t  *bh = NULL;

    STATINC(STAT_GAMBLE);
    if (bid) {
	bh = getbcache(bid);
	setbpath(path, bh->brdname);
	setbfile(fn_ticket, bh->brdname, FN_TICKET);
	currbid = bid;
    } else
	strcpy(path, "etc/");

    lockreturn0(TICKET, LOCK_MULTI);
    while (1) {
	count = show_ticket_data(betname, path, &price, bh);
	if (count <= 0) {
	    pressanykey();
	    break;
	}
	move(20, 0);
	reload_money();
	prints(ANSI_COLOR(44) "��: %-10d  " ANSI_RESET "\n" ANSI_COLOR(1) "�п�ܭn�ʶR������(1~%d)"
	       "[Q:���}]" ANSI_RESET ":", cuser.money, count);
	ch = igetch();
	/*--
	  Tim011127
	  ���F����CS���D ���O�o���٤��৹���ѨM�o���D,
	  �Yuser�q�L�ˬd�U�h, ��n�O�D�}��, �٬O�|�y��user���o������
	  �ܦ��i��]��U����L�������h, �]�ܦ��i��Q�O�D�s�}��L�ɬ~��
	  ���L�o��ܤ֥i�H���쪺�O, ���h�u�|���@����ƬO����
	--*/
	if (ch == 'q' || ch == 'Q')
	    break;
	ch -= '1';
	if (end || ch >= count || ch < 0)
	    continue;
	n = 0;
	ch_buyitem(price, "etc/buyticket", &n, 0);

	if (bid && !dashf(fn_ticket))
	    goto doesnt_catch_up;

	if (n > 0) {
	    if (append_ticket_record(path, ch, n, count) < 0)
		goto doesnt_catch_up;
	}
    }
    unlockutmpmode();
    return 0;

doesnt_catch_up:

    price = price * n;
    if (price > 0)
	deumoney(currutmp->uid, price);
    vmsg("�O�D�w�g����U�`�F ������P");
    unlockutmpmode();
    return -1;
}

int
openticket(int bid)
{
    char            path[PATHLEN], buf[PATHLEN], outcome[PATHLEN];
    int             i, money = 0, count, bet, price, total = 0, 
		    ticket[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    boardheader_t  *bh = getbcache(bid);
    FILE           *fp, *fp1;
    char            betname[MAX_ITEM][MAX_ITEM_LEN];

    setbpath(path, bh->brdname);
    count = -show_ticket_data(betname, path, &price, bh);
    if (count == 0) {
	setbfile(buf, bh->brdname, FN_TICKET_END);
	unlink(buf);
//Ptt:	��bug
	    return 0;
    }
    lockreturn0(TICKET, LOCK_MULTI);
    do {
	do {
	    getdata(20, 0,
		    ANSI_COLOR(1) "��ܤ��������X(0:���}�� 99:�����h��)" ANSI_RESET ":", buf, 3, LCECHO);
	    bet = atoi(buf);
	    move(0, 0);
	    clrtoeol();
	} while ((bet < 0 || bet > count) && bet != 99);
	if (bet == 0) {
	    unlockutmpmode();
	    return 0;
	}
	getdata(21, 0, ANSI_COLOR(1) "�A���T�{��J���X" ANSI_RESET ":", buf, 3, LCECHO);
    } while (bet != atoi(buf));

    // before we fork to process, 
    // confirm lock status is correct.
    setbfile(buf, bh->brdname, FN_TICKET_END);
    setbfile(outcome, bh->brdname, FN_TICKET_LOCK);

    if(access(outcome, 0) == 0)
    {
	unlockutmpmode();
	vmsg("�w�t���H�}���A�t�εy��N�۰ʤ��G�������G��ݪO");
	return 0;
    }
    if(rename(buf, outcome) != 0)
    {
	unlockutmpmode();
	vmsg("�L�k�ǳƶ}��... �Ц� " BN_BUGREPORT " ���i�ê��W�O�W�C");
	return 0;

    }

    if (fork()) {
	/* Ptt: �� fork() ������`�_�u�~�� */
	unlockutmpmode();
	vmsg("�t�εy��N�۰ʤ��G�󤤼����G�ݪO(�ѥ[�̦h�ɭn�Ƥ���)..");
	return 0;
    }
    close(0);
    close(1);
    setproctitle("open ticket");
#ifdef CPULIMIT
    {
	struct rlimit   rml;
	rml.rlim_cur = RLIM_INFINITY;
	rml.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CPU, &rml);
    }
#endif


    bet--;			/* �ন�x�}��index */

    total = load_ticket_record(path, ticket);
    setbfile(buf, bh->brdname, FN_TICKET_LOCK);
    if (!(fp1 = fopen(buf, "r")))
	exit(1);

    /* �٨S�}���������� �u�nmv�@���N�n */
    if (bet != 98) {
	money = total * price;
	demoney(money * 0.02);
	mail_redenvelop("[������Y]", cuser.userid, money * 0.02, 'n');
	money = ticket[bet] ? money * 0.95 / ticket[bet] : 9999999;
    } else {
	vice(price * 10, "��L�h������O");
	money = price;
    }
    setbfile(outcome, bh->brdname, FN_TICKET_OUTCOME);
    if ((fp = fopen(outcome, "w"))) {
	fprintf(fp, "��L����\n");
	while (fgets(buf, sizeof(buf), fp1)) {
	    buf[sizeof(buf)-1] = 0;
	    fputs(buf, fp);
	}
	fprintf(fp, "�U�`���p\n");

	fprintf(fp, ANSI_COLOR(33));
	for (i = 0; i < count; i++) {
	    fprintf(fp, "%d.%-8s: %-7d", i + 1, betname[i], ticket[i]);
	    if (i == 3)
		fprintf(fp, "\n");
	}
	fprintf(fp, ANSI_RESET "\n");

	if (bet != 98) {
	    fprintf(fp, "\n\n�}���ɶ��G %s \n\n"
		    "�}�����G�G %s \n\n"
		    "�Ҧ����B�G %d �� \n"
		    "������ҡG %d�i/%d�i  (%f)\n"
		    "�C�i�����m���i�o %d " MONEYNAME "�� \n\n",
	    Cdatelite(&now), betname[bet], total * price, ticket[bet], total,
		    (float)ticket[bet] / total, money);

	    fprintf(fp, "%s ��L�}�X:%s �Ҧ����B:%d �� ����/�i:%d �� ���v:%1.2f\n\n",
		    Cdatelite(&now), betname[bet], total * price, money,
		    total ? (float)ticket[bet] / total : 0);
	} else
	    fprintf(fp, "\n\n��L�����h���G %s \n\n", Cdatelite(&now));

    } // XXX somebody may use fp even fp==NULL
    fclose(fp1);
    /*
     * �H�U�O�����ʧ@
     */
    setbfile(buf, bh->brdname, FN_TICKET_USER);
    if ((bet == 98 || ticket[bet]) && (fp1 = fopen(buf, "r"))) {
	int             mybet, uid;
	char            userid[IDLEN + 1];

	while (fscanf(fp1, "%s %d %d\n", userid, &mybet, &i) != EOF) {
	    if (bet == 98 && mybet >= 0 && mybet < count) {
		if (fp)
		    fprintf(fp, "%s �R�F %d �i %s, �h�^ %d �T" MONEYNAME "��\n"
			    ,userid, i, betname[mybet], money * i);
		snprintf(buf, sizeof(buf),
			 "%s ����h��! $ %d", bh->brdname, money * i);
	    } else if (mybet == bet) {
		if (fp)
		    fprintf(fp, "���� %s �R�F%d �i %s, ��o %d �T" MONEYNAME "��\n"
			    ,userid, i, betname[mybet], money * i);
		snprintf(buf, sizeof(buf), "%s ������! $ %d", bh->brdname, money * i);
	    } else
		continue;
	    if ((uid = searchuser(userid, userid)) == 0)
		continue;
	    deumoney(uid, money * i);
	    mail_id(userid, buf, "etc/ticket.win", BBSMNAME "���");
	}
	fclose(fp1);
    }
    if (fp) {
	fprintf(fp, "\n--\n�� �}���� :" BBSNAME "(" MYHOSTNAME
		") \n�� From: %s\n", fromhost);
	fclose(fp);
    }

    if (bet != 98)
	snprintf(buf, sizeof(buf), "[���i] %s ��L�}��", bh->brdname);
    else
	snprintf(buf, sizeof(buf), "[���i] %s ��L����", bh->brdname);
    post_file(bh->brdname, buf, outcome, "[�䯫]");
    post_file("Record", buf + 7, outcome, "[�������l]");
    post_file(BN_SECURITY, buf + 7, outcome, "[�������l]");

    setbfile(buf, bh->brdname, FN_TICKET_RECORD);
    unlink(buf);

    setbfile(buf, bh->brdname, FN_TICKET_USER);
    post_file(BN_SECURITY, bh->brdname, buf, "[�U�`����]");
    unlink(buf);

    setbfile(buf, bh->brdname, FN_TICKET_LOCK);
    unlink(buf);
    exit(1);
    return 0;
}

int
ticket_main(void)
{
    ticket(0);
    return 0;
}
