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

static int 
do_pay(int uid, int money, const char *item, const char *reason)  
{ 
    int oldm, newm; 
    const char *userid; 
 
    assert(money != 0); 
    userid = getuserid(uid); 
    assert(userid); 
    assert(reason); 
 
    // if we cannot find user, abort 
    if (!userid) 
        return -1; 
 
    oldm = moneyof(uid); 
    newm = deumoney(uid, -money); 
    if (uid == usernum) 
        reload_money(); 
 
    {
        char buf[PATHLEN]; 
        sethomefile(buf, userid, FN_RECENTPAY); 
        rotate_text_logfile(buf, SZ_RECENTPAY, 0.2); 
        syncnow(); 
        log_payment(buf, money, oldm, newm, reason);
    }

    return newm; 
} 
 
int 
pay_as_uid(int uid, int money, const char *item, ...) 
{ 
    va_list ap; 
    char reason[STRLEN*3] =""; 
 
    if (!money) 
        return 0; 
 
    if (item) { 
        va_start(ap, item); 
        vsnprintf(reason, sizeof(reason)-1, item, ap); 
        va_end(ap); 
    } 
 
    return do_pay(uid, money, item, reason); 
} 
 
int 
pay(int money, const char *item, ...) 
{ 
    va_list ap; 
    char reason[STRLEN*3] =""; 
 
    if (!money) 
        return 0; 
 
    if (item) { 
        va_start(ap, item); 
        vsnprintf(reason, sizeof(reason)-1, item, ap); 
        va_end(ap); 
    } 
 
    return do_pay(usernum, money, item, reason); 
}

static int
inmailbox(int m)
{
    pwcuAddExMailBox(m);
    return cuser.exmailbox;
}


int
p_from(void)
{
    char tmp_from[sizeof(currutmp->from)];

    if (vans("�T�w�n��G�m?[y/N]") != 'y')
	return 0;

    strlcpy(tmp_from, currutmp->from, sizeof(tmp_from)); 
    if (getdata(b_lines - 1, 0, "�п�J�s�G�m:",
		tmp_from, sizeof(tmp_from), DOECHO) &&
	strcmp(tmp_from, currutmp->from) != 0) 
    {
	strlcpy(currutmp->from, tmp_from, sizeof(currutmp->from));
    }
    return 0;
}

int
p_exmail(void)
{
    char            ans[4], buf[200];
    int             n, oldm;

    if (cuser.exmailbox >= MAX_EXKEEPMAIL) {
	vmsgf("�e�q�̦h�W�[ %d �ʡA����A�R�F�C", MAX_EXKEEPMAIL);
	return 0;
    }
    snprintf(buf, sizeof(buf),
	     "�z���W�� %d �ʮe�q�A�٭n�A�R�h��? ", cuser.exmailbox);

    // no need to create default prompt.
    // and people usually come this this by accident...
    getdata(b_lines - 2, 0, buf, ans, sizeof(ans), NUMECHO);

    oldm = cuser.exmailbox;
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

    reload_money();
    pay(n * 1000, "�ʶR�H�c");
    inmailbox(n);
    log_filef("log/exmailbox.log", LOG_CREAT,
	    "%-13s %d+%d->%d %s\n", cuser.userid, 
	    oldm, n, cuser.exmailbox, Cdatelite(&now));

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

/* �����P�ػP�| */

int
give_tax(int money)
{
    int tax = money * 0.1;
    assert (money >= 0);
    if (money % 10)
	tax ++;
    return (tax < 1) ? 1 : tax;
}
int
cal_before_givetax(int taxed_money)
{
    int m = taxed_money / 9.0f * 10 + 1;
    if (m > 1 && taxed_money % 9 == 0)
	m--;
    return m;
}

int
cal_after_givetax(int money)
{
    return money - give_tax(money);
}

static int 
give_money_vget_changecb(int key, VGET_RUNTIME *prt, void *instance)
{
    int  m1 = atoi(prt->buf), m2 = m1;
    char c1 = ' ', c2 = ' ';
    int is_before_tax = *(int*)instance;

    if (is_before_tax)
	m2 = cal_after_givetax(m1),  c1 = '>';
    else
	m1 = cal_before_givetax(m2), c2 = '>';

    // adjust output
    if (m1 <= 0 || m2 <= 0)
	m1 = m2 = 0;

    move(4, 0);
    prints(" %c �A�n�I�X (�|�e): %d\n", c1, m1);
    prints(" %c ��覬�� (�|��): %d\n", c2, m2);
    return VGETCB_NONE;
}

static int 
give_money_vget_peekcb(int key, VGET_RUNTIME *prt, void *instance)
{
    int *p_is_before_tax = (int*) instance;
    
    // digits will be refreshed later.
    if (key >= '0' && key <= '9')
	return VGETCB_NONE;
    if (key != KEY_TAB)
	return VGETCB_NONE;

    // TAB - toggle mode and update display
    assert(p_is_before_tax);
    *p_is_before_tax = !*p_is_before_tax;
    give_money_vget_changecb(key, prt, instance);
    return VGETCB_NEXT;
}

static int
do_give_money(char *id, int uid, int money, const char *myid)
{
    int tax;
    char prompt[STRLEN*2] = "";

    reload_money();
    if (money < 1 || cuser.money < money)
	return -1;

    tax = give_tax(money);
    if (money - tax <= 0)
	return -1;		/* ú���|�N�S�����F */

    if (strcasecmp(myid, cuser.userid) != 0)  {
        snprintf(prompt, sizeof(prompt)-1,
                "�H %s ���W�q��b�� %s (�|�� $%d)",
                myid, id, money - tax); 
    } else {
        snprintf(prompt, sizeof(prompt)-1, 
                "��b�� %s (�|�� $%d)", id, money - tax); 
    }

    // ��ڵ��������C ���קK�{���G��/�c�N�_�u�A�@�ߥ����A�o�C
    pay(money, "%s", prompt);
    pay_as_uid(uid, -(money - tax), "�Ӧ� %s ����b (�|�e $%d)", 
               myid, money); 
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
    const char	    *uid_prompt = "�o�쩯�B�઺id: ";

    // TODO prevent macros, we should check something here,
    // like user pw/id/...
    vs_hdr("��������");

    if (!userid || !*userid || strcmp(userid, cuser.userid) == 0)
	userid = NULL;

    // XXX should we prevent editing if userid is already assigned?
    usercomplete2(uid_prompt, id, userid);
    // redraw highlighted prompt
    move(1, 0); clrtobot();
    outs(uid_prompt); outs(ANSI_COLOR(1)); outs(id);
    outs(ANSI_RESET "\n");

    if (!id[0] || strcasecmp(cuser.userid, id) == 0)
    {
	vmsg("�������!");
	return -1;
    }

    if ((uid = searchuser(id, id)) == 0) {
	vmsg("�d�L���H!");
	return -1;
    }

    m = 0;
    money_buf[0] = 0;
    outs("�n���L�h�ֿ��O? (�i�� TAB ������J�|�e/�|����B, �|�v�T�w 10%)\n");
    outs(" �п�J���B: ");  // (3, 0)
    {
	int is_before_tax = 1;
	const VGET_CALLBACKS cb = { 
	    give_money_vget_peekcb, 
	    NULL, 
	    give_money_vget_changecb,
	};
	if (vgetstring(money_buf, 7, VGET_DIGITS, "", &cb, &is_before_tax))
	    m = atoi(money_buf);
	if (m > 0 && !is_before_tax)
	    m = cal_before_givetax(m);
    }
    if (m < 2) {
	vmsg("���B�L�֡A�������!");
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

    if(do_give_money(id, uid, m, myid) < 0)
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
#ifdef USE_RECENTPAY
            outs("�z�i��U�C��m���̪񪺥���O��:\n" 
                    "�D��� => (U)ser�ӤH�]�w => (L)MyLogs �ӤH�O�� => " 
                    "(P)Recent Pay �̪����O��\n"); 
#endif
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
	   // XXX check the related logic in mbbsd.c
	   (SHM->GV2.e.dymaxactive > 2000 && SHM->GV2.e.dymaxactive < MAX_ACTIVE) ? 
	    SHM->GV2.e.dymaxactive : MAX_ACTIVE,
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
	    "\ttelnet/vkbd protocol, vtuikit, ALOHA fixer, BRC v3\n"
	    "\tpiaip's Common Chat Window (CCW)\n"
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

