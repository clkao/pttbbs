/* $Id$ */
#include "bbs.h"

/* ���� Multi play */
static int
is_playing(int unmode)
{
    register int    i;
    register userinfo_t *uentp;
    unsigned int p = StringHash(cuser.userid) % USHM_SIZE;

    for (i = 0; i < USHM_SIZE; i++, p++) { // XXX linear search
	if (p == USHM_SIZE)
	    p = 0;
	uentp = &(SHM->uinfo[p]);
	if (uentp->mode == DEBUGSLEEPING)
	    continue;
	if (uentp->uid == usernum &&
	    uentp->lockmode == unmode)
	    return 1;
    }
    return 0;
}

int
lockutmpmode(int unmode, int state)
{
    int             errorno = 0;

    if (currutmp->lockmode)
	errorno = LOCK_THIS;
    else if (state == LOCK_MULTI && is_playing(unmode))
	errorno = LOCK_MULTI;

    if (errorno) {
	clear();
	move(10, 20);
	if (errorno == LOCK_THIS)
	    prints("�Х����} %s �~��A %s ",
		   ModeTypeTable[currutmp->lockmode],
		   ModeTypeTable[unmode]);
	else
	    prints("��p! �z�w����L�u�ۦP��ID���b%s",
		   ModeTypeTable[unmode]);
	pressanykey();
	return errorno;
    }
    setutmpmode(unmode);
    currutmp->lockmode = unmode;
    return 0;
}

int
unlockutmpmode(void)
{
    currutmp->lockmode = 0;
    return 0;
}

/* �ϥο������ */
#define VICE_NEW   "vice.new"

/* Heat:�o�� */
int
vice(int money, const char *item)
{
    char            buf[128];
    unsigned int viceserial = (currutmp->lastact % 10000) * 10000 + 
	random() % 10000;

    // new logic: do not send useless vice tickets
    demoney(-money);
    if (money < VICE_MIN)
	return 0;

    setuserfile(buf, VICE_NEW);
    log_filef(buf, LOG_CREAT, "%8.8d\n", viceserial);
    snprintf(buf, sizeof(buf),
	    "%s ��F$%d �s��[%08d]", item, money, viceserial);
    mail_id(cuser.userid, buf, "etc/vice.txt", BBSMNAME "�g�ٳ�");
    return 0;
}

static int
inmailbox(int m)
{
    pwcuAddExMailBox(m);
    return cuser.exmailbox;
}


#if !HAVE_FREECLOAK
/* ������ */
int
p_cloak(void)
{
    if (vans(currutmp->invisible ? "�T�w�n�{��?[y/N]" : "�T�w�n����?[y/N]") != 'y')
	return 0;
    reload_money();
    if (cuser.money >= 19) {
	vice(19, "�I�O����");
	currutmp->invisible %= 2;
	vmsg((currutmp->invisible ^= 1) ? MSG_CLOAKED : MSG_UNCLOAK);
    }
    return 0;
}
#endif

int
p_from(void)
{
    char tmp_from[sizeof(currutmp->from)];
    if (vans("�T�w�n��G�m?[y/N]") != 'y')
	return 0;
    reload_money();
    if (cuser.money < 49)
	return 0;
    if (getdata(b_lines - 1, 0, "�п�J�s�G�m:",
		tmp_from, sizeof(tmp_from), DOECHO) &&
	strcmp(tmp_from, currutmp->from) != 0) 
    {
	vice(49, "���G�m");
	strlcpy(currutmp->from, tmp_from, sizeof(currutmp->from));
    }
    return 0;
}

int
p_exmail(void)
{
    char            ans[4], buf[200];
    int             n;

    if (cuser.exmailbox >= MAX_EXKEEPMAIL) {
	vmsgf("�e�q�̦h�W�[ %d �ʡA����A�R�F�C", MAX_EXKEEPMAIL);
	return 0;
    }
    snprintf(buf, sizeof(buf),
	     "�z���W�� %d �ʮe�q�A�٭n�A�R�h��? ", cuser.exmailbox);

    // no need to create default prompt.
    // and people usually come this this by accident...
    getdata(b_lines - 2, 0, buf, ans, sizeof(ans), NUMECHO);

    n = atoi(ans);
    if (!ans[0] || n<=0)
	return 0;

    if (n + cuser.exmailbox > MAX_EXKEEPMAIL)
	n = MAX_EXKEEPMAIL - cuser.exmailbox;
    reload_money();
    if (cuser.money < n * 1000)
    {
	vmsg("�A���������C");
	return 0;
    }

    if (vmsgf("�A�Q�ʶR %d �ʫH�c (�n�� %d ��), �T�w�ܡH[y/N] ", 
		n, n*1000) != 'y')
	return 0;

    vice(n * 1000, "�ʶR�H�c");
    inmailbox(n);
    vmsgf("�w�ʶR�H�c�C�s�e�q�W��: %d", cuser.exmailbox);
    return 0;
}

int
mail_redenvelop(const char *from, const char *to, int money, char *fpath)
{
    char            _fpath[PATHLEN], dirent[PATHLEN];
    fileheader_t    fhdr;
    FILE           *fp;

    if (!fpath) fpath = _fpath;

    sethomepath(fpath, to);
    stampfile(fpath, &fhdr);

    if (!(fp = fopen(fpath, "w")))
	return -1;

    fprintf(fp, "�@��: %s\n"
	    "���D: �۰]�i�_\n"
	    "�ɶ�: %s\n"
	    ANSI_COLOR(1;33) "�˷R�� %s �G\n\n" ANSI_RESET
	    ANSI_COLOR(1;31) "    �ڥ]���A�@�� %d �����j���]�� ^_^\n\n"
	    "    §�����N���A�Я���...... ^_^" ANSI_RESET "\n",
	    from, ctime4(&now), to, money);
    fclose(fp);

    // colorize topic to make sure this is issued by system.
    snprintf(fhdr.title, sizeof(fhdr.title), 
	    ANSI_COLOR(1;37;41) "[���]]" ANSI_RESET " $%d", money);
    strlcpy(fhdr.owner, from, sizeof(fhdr.owner));
    sethomedir(dirent, to);
    append_record(dirent, &fhdr, sizeof(fhdr));
    return 0;
}


int do_give_money(char *id, int uid, int money)
{
    int tax;

    reload_money();
    if (money < 1 || cuser.money < money)
	return -1;

    tax = give_tax(money);
    if (money - tax <= 0)
	return -1;		/* ú���|�N�S�����F */

    // ��ڵ��������C
    deumoney(uid, money - tax);
    demoney(-money);
    log_filef(FN_MONEY, LOG_CREAT, "%-12s �� %-12s %d\t(�|�� %d)\t%s\n",
	    cuser.userid, id, money, money - tax, Cdate(&now));

    // penalty
    if (money < 50) {
	usleep(2000000);    // 2 sec
    } else if (money < 200) {
	usleep(500000);	    // 0.5 sec
    } else {
	usleep(100000);	    // 0.1 sec
    }
    return 0;
}

int
p_give(void)
{
    give_money_ui(NULL);
    return -1;
}

int 
give_money_ui(const char *userid)
{
    int             uid;
    char            id[IDLEN + 1], money_buf[20];
    char	    passbuf[PASSLEN];
    int		    m = 0, mtax = 0, tries = 3, skipauth = 0;
    static time4_t  lastauth = 0;
    const char	    *myid = cuser.userid;

    // TODO prevent macros, we should check something here,
    // like user pw/id/...
    vs_hdr("��������");

    if (!userid || !*userid)
	usercomplete("�o�쩯�B�઺id: ", id);
    else {
	strlcpy(id, userid, sizeof(id));
	prints("�o�쩯�B�઺id: %s\n", id);
    }

    move(2, 0); clrtobot();

    if (!id[0] || strcasecmp(cuser.userid, id) == 0)
    {
	vmsg("�������!");
	return -1;
    }

    if (!getdata(2, 0, "�n���L�h�ֿ��O: ", money_buf, 7, NUMECHO) ||
	((m = atoi(money_buf)) < 2))
    {
	vmsg("���B�L�֡A�������!");
	return -1;
    }

    if ((uid = searchuser(id, id)) == 0) {
	vmsg("�d�L���H!");
	return -1;
    }

    reload_money();
    if (cuser.money < m) {
	vmsg("�A�S������h����!");
	return -1;
    }

    mtax = give_tax(m);
    move(4, 0);
    prints( "������e: %s �N���� %s : [���|] $%d (�|�� $%d )\n"
	    "����o: $%d\n",
	    cuser.userid, id, m, mtax, m-mtax);

    // safe context starts at (6, 0).
#ifdef PLAY_ANGEL
    if (HasUserPerm(PERM_ANGEL))
    {
	userec_t xuser = {0};
	getuser(id, &xuser);

	if (strcmp(xuser.myangel, cuser.userid) == 0)
	{
	    char yn[3];
	    outs("�L�O�A���p�D�H�A�O�_�ΦW�H[Y/n]: ");
	    vgets(yn, sizeof(yn), VGET_LOWERCASE);
	    if (yn[0] != 'n')
		myid = "�p�Ѩ�";
	}
    }
#endif // PLAY_ANGEL

    // safe context starts at (7, 0)
    move(7, 0);
    if (now - lastauth >= 15*60) // valid through 15 minutes
    {
	outs(ANSI_COLOR(1;31) "���F�קK�~���άO�c�N�B�F�A"
		"�b��������e�n���s�T�{�z�������C" ANSI_RESET);
    } else {
	outs("�A���{�ҩ|���L���A�i�Ȯɸ��L�K�X�{�ҵ{�ǡC\n");
	// auth is valid.
	if (vans("�T�w�i�����ܡH (y/N): ") == 'y')
	    skipauth = 1;
	else
	    tries = -1;
    }

    // safe context starts at (7, 0)
    while (!skipauth && tries-- > 0)
    {
	getdata(8, 0, MSG_PASSWD,
		passbuf, sizeof(passbuf), NOECHO);
	passbuf[8] = '\0';
	if (checkpasswd(cuser.passwd, passbuf))
	{
	    lastauth = now;
	    break;
	}
	// if we show '%d chances left', some user may think
	// they will be locked out...
	if (tries > 0 &&
	    vmsg("�K�X���~�A�Э��թΫ� n ��������C") == 'n')
	    return -1;
    }

    if (tries < 0)
    {
	vmsg("�������!");
	return -1;
    }

    outs("\n������b�i�椤�A�еy��...\n"); 
    refresh();

    if(do_give_money(id, uid, m) < 0)
    {
	outs(ANSI_COLOR(1;31) "������ѡI" ANSI_RESET "\n");
	vmsg("������ѡC");
	return -1;
    }

    outs(ANSI_COLOR(1;33) "��������I" ANSI_RESET "\n");

    // transaction complete.
    {
	char fpath[PATHLEN];
	if (mail_redenvelop( myid, id, m - mtax, fpath) < 0)
	{
	    vmsg("��������C");
	    return 0;
	}

	// TODO �Y�O�a�H�A�T��s�褺��H
	if (vans("����w�����A�n�ק���]�U�ܡH[y/N] ") == 'y')
	{
	    veditfile(fpath);
	}
	sendalert(id, ALERT_NEW_MAIL);
    }
    return 0;
}

int 
resolve_over18_user(const userec_t *u)
{
    /* get local time */
    struct tm ptime;

    localtime4_r(&now, &ptime);
    // �ӹ귳�p��A�S�ͤ骺��@���� 18
    if (u->year < 1 || u->month < 1)
	return 0;
    else if( (ptime.tm_year - u->year) > 18)
	return 1;
    else if (ptime.tm_year - u->year < 18)
	return 0;
    else if ((ptime.tm_mon+1) > u->month)
	return 1;
    else if ((ptime.tm_mon+1) < u->month)
	return 0;
    else if (ptime.tm_mday >= u->day )
	return 1;
    return 0;
}

void
resolve_over18(void)
{
    over18 = resolve_over18_user(cuser_ref);
}

int
p_sysinfo(void)
{
    char            *cpuloadstr;
    int             load;
    extern char    *compile_time;
#ifdef DETECT_CLIENT
    extern Fnv32_t  client_code;
#endif

    load = cpuload(NULL);
    cpuloadstr = (load < 5 ? "�}�n" : (load < 20 ? "�|�i" : "�L��"));

    clear();
    showtitle("�t�θ�T", BBSNAME);
    move(2, 0);
    prints("�z�{�b��� " TITLE_COLOR BBSNAME ANSI_RESET " (" MYIP ")\n"
	   "�t�έt�����p: %s\n"
	   "�u�W�A�ȤH��: %d/%d\n"
#ifdef DETECT_CLIENT
	   "client code:  %8.8X\n"
#endif
	   "�sĶ�ɶ�:     %s\n"
	   "�_�l�ɶ�:     %s\n",
	   cpuloadstr, SHM->UTMPnumber,
#ifdef DYMAX_ACTIVE
	   SHM->GV2.e.dymaxactive > 2000 ? SHM->GV2.e.dymaxactive : MAX_ACTIVE,
#else
	   MAX_ACTIVE,
#endif
#ifdef DETECT_CLIENT
	   client_code,
#endif
	   compile_time, Cdatelite(&start_time));

#ifdef REPORT_PIAIP_MODULES
    outs("\n" ANSI_COLOR(1;30)
	    "Modules powered by piaip:\n"
	    "\ttelnet protocol, ALOHA fixer, BRC v3\n"
#if defined(USE_PIAIP_MORE) || defined(USE_PMORE)
	    "\tpmore (piaip's more) 2007 w/Movie\n"
#endif
#ifdef HAVE_GRAYOUT
	    "\tGrayout Advanced Control �H�J�H�X�S�Ĩt��\n"
#endif 
#ifdef EDITPOST_SMARTMERGE
	    "\tSmart Merge �פ�۰ʦX��\n"
#endif
#ifdef EXP_EDIT_UPLOAD
	    "\t(EXP) Editor Uploader ����W��\n"
#endif
#if defined(USE_PFTERM)
	    "\t(EXP) pfterm (piaip's flat terminal, Perfect Term)\n"
#endif
#if defined(USE_BBSLUA)
	    "\t(EXP) BBS-Lua\n"
#endif
	    ANSI_RESET
	    );
#endif // REPORT_PIAIP_MODULES

    if (HasUserPerm(PERM_SYSOP)) {
	struct rusage ru;
#ifdef __linux__
	int vmdata=0, vmstk=0;
	FILE * fp;
	char buf[128];
	if ((fp = fopen("/proc/self/status", "r"))) {
	    while (fgets(buf, 128, fp)) {
		sscanf(buf, "VmData: %d", &vmdata);
		sscanf(buf, "VmStk: %d", &vmstk);
	    }
	    fclose(fp);
	}
#endif
	getrusage(RUSAGE_SELF, &ru);
	prints("�O����ζq: "
#ifdef IA32
	       "sbrk: %u KB, "
#endif
#ifdef __linux__
	       "VmData: %d KB, VmStk: %d KB, "
#endif
	       "idrss: %d KB, isrss: %d KB\n",
#ifdef IA32
	       ((unsigned int)sbrk(0) - 0x8048000) / 1024,
#endif
#ifdef __linux__
	       vmdata, vmstk,
#endif
	       (int)ru.ru_idrss, (int)ru.ru_isrss);
	prints("CPU �ζq:   %ld.%06ldu %ld.%06lds",
	       (long int)ru.ru_utime.tv_sec, 
	       (long int)ru.ru_utime.tv_usec,
	       (long int)ru.ru_stime.tv_sec, 
	       (long int)ru.ru_stime.tv_usec);
#ifdef CPULIMIT_PER_DAY
	prints(" (limit %d secs per day)", CPULIMIT_PER_DAY);
#endif
	outs("\n�S�O�Ѽ�:"
#ifdef CRITICAL_MEMORY
		" CRITICAL_MEMORY"
#endif
#ifdef UTMPD
		" UTMPD"
#endif
#ifdef FROMD
		" FROMD"
#endif
		);
    }
    pressanykey();
    return 0;
}

