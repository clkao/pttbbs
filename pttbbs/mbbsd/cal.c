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
	if (uentp->uid == usernum)
	    if (uentp->lockmode == unmode)
		return 1;
    }
    return 0;
}

int
lockutmpmode(int unmode, int state)
{
    int             errorno = 0;

    if (currutmp->lockmode)
	errorno = 1;
    else if (is_playing(unmode))
	errorno = 2;

    if (errorno && !(state == LOCK_THIS && errorno == LOCK_MULTI)) {
	clear();
	move(10, 20);
	if (errorno == 1)
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
vice(int money, char *item)
{
   char            buf[128];
   unsigned int viceserial = (currutmp->lastact % 10000) * 10000 + random() % 10000;

    demoney(-money);
    if(money>=100) 
	{
          setuserfile(buf, VICE_NEW);
          log_file(buf, LOG_CREAT | LOG_VF, "%8.8d\n", viceserial);
	}
    snprintf(buf, sizeof(buf),
	     "%s ��F$%d �s��[%08d]", item, money, viceserial);
    mail_id(cuser.userid, buf, "etc/vice.txt", "Ptt�g�ٳ�");
    return 0;
}

#define lockreturn(unmode, state) if(lockutmpmode(unmode, state)) return
#define lockreturn0(unmode, state) if(lockutmpmode(unmode, state)) return 0
#define lockbreak(unmode, state) if(lockutmpmode(unmode, state)) break
#define SONGBOOK  "etc/SONGBOOK"
#define OSONGPATH "etc/SONGO"

static int
osong(char *defaultid)
{
    char            destid[IDLEN + 1], buf[200], genbuf[200], filename[256],
                    say[51];
    char            receiver[45], ano[3];
    FILE           *fp, *fp1;
    //*fp2;
    fileheader_t    mail;
    int             nsongs;

    strlcpy(buf, Cdatedate(&now), sizeof(buf));

    lockreturn0(OSONG, LOCK_MULTI);

    /* Jaky �@�H�@���I�@�� */
    if (!strcmp(buf, Cdatedate(&cuser.lastsong)) && !HAS_PERM(PERM_SYSOP)) {
	move(22, 0);
	vmsg("�A���Ѥw�g�I�L�o�A���ѦA�I�a....");
	unlockutmpmode();
	return 0;
    }
    if (cuser.money < 200) {
	move(22, 0);
	vmsg("�I�q�n200�ȭ�!....");
	unlockutmpmode();
	return 0;
    }
    move(12, 0);
    clrtobot();
    prints("�˷R�� %s �w��Ө�ڮ�۰��I�q�t��\n", cuser.userid);
    trans_buffer[0] = 0;
    if (!defaultid) {
	getdata(13, 0, "�n�I���֩O:[�i������ Enter ����q]",
		destid, sizeof(destid), DOECHO);
	while (!destid[0]) {
	    a_menu("�I�q�q��", SONGBOOK, 0);
	    clear();
	    getdata(13, 0, "�n�I���֩O:[�i�� Enter ���s��q]",
		    destid, sizeof(destid), DOECHO);
	}
    } else
	strlcpy(destid, defaultid, sizeof(destid));

    /* Heat:�I�q�̰ΦW�\�� */
    getdata(14, 0, "�n�ΦW��?[y/n]:", ano, sizeof(ano), LCECHO);

    if (!destid[0]) {
	unlockutmpmode();
	return 0;
    }
    getdata_str(14, 0, "�Q�n�n��L(�o)��..:", say,
		sizeof(say), DOECHO, "�ڷR�p..");
    snprintf(save_title, sizeof(save_title),
	     "%s:%s", (ano[0] == 'y') ? "�ΦW��" : cuser.userid, say);
    getdata_str(16, 0, "�H��֪��H�c(�i��E-mail)?",
		receiver, sizeof(receiver), LCECHO, destid);

    if (!trans_buffer[0]) {
	outs("\n���ۭn��q�o..�i�J�q���n�n����@���q�a..^o^");
	pressanykey();
	a_menu("�I�q�q��", SONGBOOK, 0);
    }
    if (!trans_buffer[0] || strstr(trans_buffer, "home") ||
	strstr(trans_buffer, "boards") || !(fp = fopen(trans_buffer, "r"))) {
	unlockutmpmode();
	return 0;
    }
    strlcpy(filename, OSONGPATH, sizeof(filename));

    stampfile(filename, &mail);

    unlink(filename);

    if (!(fp1 = fopen(filename, "w"))) {
	fclose(fp);
	unlockutmpmode();
	return 0;
    }
    strlcpy(mail.owner, "�I�q��", sizeof(mail.owner));
    snprintf(mail.title, sizeof(mail.title),
	     "�� %s �I�� %s ",
	     (ano[0] == 'y') ? "�ΦW��" : cuser.userid, destid);

    while (fgets(buf, sizeof(buf), fp)) {
	char           *po;
	if (!strncmp(buf, "���D: ", 6)) {
	    clear();
	    move(10, 10);
	    outs(buf);
	    pressanykey();
	    fclose(fp);
	    fclose(fp1);
	    unlockutmpmode();
	    return 0;
	}
	while ((po = strstr(buf, "<~Src~>"))) {
	    po[0] = 0;
	    snprintf(genbuf, sizeof(genbuf),
		     "%s%s%s", buf,
		     (ano[0] == 'y') ? "�ΦW��" : cuser.userid, po + 7);
	    strlcpy(buf, genbuf, sizeof(buf));
	}
	while ((po = strstr(buf, "<~Des~>"))) {
	    po[0] = 0;
	    snprintf(genbuf, sizeof(genbuf), "%s%s%s", buf, destid, po + 7);
	    strlcpy(buf, genbuf, sizeof(buf));
	}
	while ((po = strstr(buf, "<~Say~>"))) {
	    po[0] = 0;
	    snprintf(genbuf, sizeof(genbuf), "%s%s%s", buf, say, po + 7);
	    strlcpy(buf, genbuf, sizeof(buf));
	}
	fputs(buf, fp1);
    }
    fclose(fp1);
    fclose(fp);


    if (append_record(OSONGPATH "/.DIR", &mail, sizeof(mail)) != -1) {
	cuser.lastsong = now;
	/* Jaky �W�L 500 ���q�N�}�l�� */
	nsongs = get_num_records(OSONGPATH "/.DIR", sizeof(mail));
	if (nsongs > 500) {
	    delete_range(OSONGPATH "/.DIR", 1, nsongs - 500);
	}
	/* ��Ĥ@������ */
	vice(200, "�I�q");
    }
    snprintf(save_title, sizeof(save_title),
	     "%s:%s", (ano[0] == 'y') ? "�ΦW��" : cuser.userid, say);
    hold_mail(filename, destid);

    if (receiver[0]) {
#ifndef USE_BSMTP
	bbs_sendmail(filename, save_title, receiver);
#else
	bsmtp(filename, save_title, receiver, 0);
#endif
    }
    clear();
    outs(
	 "\n\n  ���߱z�I�q�����o..\n"
	 "  �@�p�ɤ��ʺA�ݪO�|�۰ʭ��s��s\n"
	 "  �j�a�N�i�H�ݨ�z�I���q�o\n\n"
	 "  �I�q��������D�i�H��Note�O����ذϧ䵪��\n"
	 "  �]�i�bNote�O��ذϬݨ�ۤv���I�q�O��\n"
	 "  ������O�Q���N���]�w���Note�O�d��\n"
	 "  ���ˤ����O�D���z�A��\n");
    pressanykey();
    sortsong();
    topsong();

    unlockutmpmode();
    return 1;
}

int
ordersong(void)
{
    osong(NULL);
    return 0;
}

static int
inmailbox(int m)
{
    userec_t xuser;
    passwd_query(usernum, &xuser);
    cuser.exmailbox = xuser.exmailbox + m;
    passwd_update(usernum, &cuser);
    return cuser.exmailbox;
}


#if !HAVE_FREECLOAK
/* ������ */
int
p_cloak(void)
{
    if (getans(currutmp->invisible ? "�T�w�n�{��?[y/N]" : "�T�w�n����?[y/N]") != 'y')
	return 0;
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
    if (getans("�T�w�n��G�m?[y/N]") != 'y')
	return 0;
    reload_money();
    if (cuser.money < 49)
	return 0;
    if (getdata_buf(b_lines - 1, 0, "�п�J�s�G�m:",
		    currutmp->from, sizeof(currutmp->from), DOECHO)) {
	vice(49, "���G�m");
	currutmp->from_alias = 0;
    }
    return 0;
}

int
p_exmail(void)
{
    char            ans[4], buf[200];
    int             n;
//   Ptt: what is it for?
//    assert(MAX_EXKEEPMAIL < (1<< (sizeof(cuser.exmailbox)*8-1) ));
    if (cuser.exmailbox >= MAX_EXKEEPMAIL) {
	vmsg("�e�q�̦h�W�[ %d �ʡA����A�R�F�C", MAX_EXKEEPMAIL);
	return 0;
    }
    snprintf(buf, sizeof(buf),
	     "�z���W�� %d �ʮe�q�A�٭n�A�R�h��?", cuser.exmailbox);

    getdata_str(b_lines - 2, 0, buf, ans, sizeof(ans), LCECHO, "10");

    n = atoi(ans);
    if (!ans[0] || n<=0)
	return 0;
    if (n + cuser.exmailbox > MAX_EXKEEPMAIL)
	n = MAX_EXKEEPMAIL - cuser.exmailbox;
    reload_money();
    if (cuser.money < n * 1000)
	return 0;
    vice(n * 1000, "�ʶR�H�c");
    inmailbox(n);
    return 0;
}

void
mail_redenvelop(char *from, char *to, int money, char mode)
{
    char            genbuf[200];
    fileheader_t    fhdr;
    FILE           *fp;
    snprintf(genbuf, sizeof(genbuf), "home/%c/%s", to[0], to);
    stampfile(genbuf, &fhdr);
    if (!(fp = fopen(genbuf, "w")))
	return;
    fprintf(fp, "�@��: %s\n"
	    "���D: �۰]�i�_\n"
	    "�ɶ�: %s\n"
	    "\033[1;33m�˷R�� %s �G\n\n\033[m"
	    "\033[1;31m    �ڥ]���A�@�� %d �����j���]�� ^_^\n\n"
	    "    §�����N���A�Я���...... ^_^\033[m\n",
	    from, ctime4(&now), to, money);
    fclose(fp);
    snprintf(fhdr.title, sizeof(fhdr.title), "�۰]�i�_");
    strlcpy(fhdr.owner, from, sizeof(fhdr.owner));

    if (mode == 'y')
	vedit(genbuf, NA, NULL);
    snprintf(genbuf, sizeof(genbuf), "home/%c/%s/.DIR", to[0], to);
    append_record(genbuf, &fhdr, sizeof(fhdr));
}

/* �p���ػP�| */
int
give_tax(int money)
{
    int             i, tax = 0;
    int      tax_bound[] = {1000000, 100000, 10000, 1000, 0};
    double   tax_rate[] = {0.4, 0.3, 0.2, 0.1, 0.08};
    for (i = 0; i <= 4; i++)
	if (money > tax_bound[i]) {
	    tax += (money - tax_bound[i]) * tax_rate[i];
	    money -= (money - tax_bound[i]);
	}
    return (tax <= 0) ? 1 : tax;
}

int
p_give(void)
{
    int             money, tax;
    char            id[IDLEN + 1], money_buf[20];
#ifdef PLAY_ANGEL
    userec_t        xuser;
#endif

    move(1, 0);
    usercomplete("�o�쩯�B�઺id:", id);
    if (!id[0] || !strcmp(cuser.userid, id) ||
	!getdata(2, 0, "�n���h�ֿ�:", money_buf, 7, LCECHO))
	return 0;
    money = atoi(money_buf);
    reload_money();
    if (money > 0 && cuser.money >= money) {
	tax = give_tax(money);
	if (money - tax <= 0)
	    return 0;		/* ú���|�N�S�����F */
	deumoney(searchuser(id), money - tax); // TODO if searchuser(id) return 0
	demoney(-money);
	log_file(FN_MONEY, LOG_CREAT | LOG_VF, "%s\t��%s\t%d\t%s",
                 cuser.userid, id, money - tax, ctime4(&now));
#ifdef PLAY_ANGEL
	getuser(id, &xuser);
	if (!strcmp(xuser.myangel, cuser.userid)){
	    mail_redenvelop(
		    getkey("�L�O�A���p�D�H�A�O�_�ΦW�H[Y/n]") == 'n' ?
		    cuser.userid : "�p�Ѩ�", id, money - tax,
			getans("�n�ۦ�Ѽg���]�U�ܡH[y/N]"));
	} else
#endif
	mail_redenvelop(cuser.userid, id, money - tax,
		getans("�n�ۦ�Ѽg���]�U�ܡH[y/N]"));
    }
    return 0;
}

int
p_sysinfo(void)
{
    char            *cpuloadstr;
    int             load;
    extern char    *compile_time;

    load = cpuload(NULL);
    cpuloadstr = (load < 5 ? "�}�n" : (load < 20 ? "�|�i" : "�L��"));

    clear();
    showtitle("�t�θ�T", BBSNAME);
    move(2, 0);
    prints("�z�{�b��� " TITLE_COLOR BBSNAME "\033[m (" MYIP ")\n"
	   "�t�έt�����p: %s\n"
	   "�u�W�A�ȤH��: %d/%d\n"
	   "�sĶ�ɶ�:     %s\n"
	   "�_�l�ɶ�:     %s\n",
	   cpuloadstr, SHM->UTMPnumber,
#ifdef DYMAX_ACTIVE
	   SHM->GV2.e.dymaxactive > 2000 ? SHM->GV2.e.dymaxactive : MAX_ACTIVE,
#else
	   MAX_ACTIVE,
#endif
	   compile_time, ctime4(&start_time));
    if (HAS_PERM(PERM_SYSOP)) {
	struct rusage ru;
	getrusage(RUSAGE_SELF, &ru);
	prints("�O����ζq: "
#ifdef IA32
	       "sbrk: %d KB, "
#endif
	       "idrss: %d KB, isrss: %d KB\n",
#ifdef IA32
	       ((int)sbrk(0) - 0x8048000) / 1024,
#endif
	       (int)ru.ru_idrss, (int)ru.ru_isrss);
	prints("CPU �ζq:   %ld.%06ldu %ld.%06lds",
	       ru.ru_utime.tv_sec, ru.ru_utime.tv_usec,
	       ru.ru_stime.tv_sec, ru.ru_stime.tv_usec);
#ifdef CPULIMIT
	prints(" (limit %d secs)", (int)(CPULIMIT * 60));
#endif
	outs("\n�S�O�Ѽ�:"
#ifdef CRITICAL_MEMORY
		" CRITICAL_MEMORY"
#endif
#ifdef OUTTACACHE
		" OUTTACACHE"
#endif
		);
    }
    pressanykey();
    return 0;
}

