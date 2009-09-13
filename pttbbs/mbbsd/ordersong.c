/* $Id$ */
#include "bbs.h"

#define lockreturn(unmode, state)  if(lockutmpmode(unmode, state)) return
#define lockreturn0(unmode, state) if(lockutmpmode(unmode, state)) return 0
#define lockbreak(unmode, state)   if(lockutmpmode(unmode, state)) break
#define SONGBOOK  "etc/SONGBOOK"
#define OSONGPATH "etc/SONGO"
#define ORDER_SONG_COST   (200)	// how much to order a song

#define MAX_SONGS (MAX_MOVIE-100) // (400) XXX MAX_SONGS should be fewer than MAX_MOVIE.

static void sortsong(void);

static int
do_order_song(void)
{
    char            sender[IDLEN + 1], receiver[IDLEN + 1], buf[200],
		    genbuf[200], filename[256], say[51];
    char            trans_buffer[PATHLEN];
    char            address[45];
    FILE           *fp, *fp1;
    fileheader_t    mail;
    int             nsongs;
    char save_title[STRLEN];

    strlcpy(buf, Cdatedate(&now), sizeof(buf));

    lockreturn0(OSONG, LOCK_MULTI);
    pwcuReload();

    /* Jaky �@�H�@���I�@�� */
    if (!strcmp(buf, Cdatedate(&cuser.lastsong)) && !HasUserPerm(PERM_SYSOP)) {
	move(22, 0);
	vmsg("�A���Ѥw�g�I�L�o�A���ѦA�I�a....");
	unlockutmpmode();
	return 0;
    }

    while (1) {
	char ans[4];
	move(12, 0);
	clrtobot();
	prints("�˷R�� %s �w��Ө�۰��I�q�t��\n\n", cuser.userid);
	outs(ANSI_COLOR(1) "�`�N�I�q���e�ФůA����| �H������ �T��"
	     "���M�V�d ����\n"
	     "�Y���W�z�H�W���ΡA����N�O�d�M�w�O�_���}�����v�Q\n"
	     "�p���P�N�Ы� (3) ���}�C" ANSI_RESET "\n");
	getdata(18, 0, 
#ifdef USE_PFTERM
		"�п�� " ANSI_COLOR(1) "1)" ANSI_RESET " �}�l�I�q�B"
		ANSI_COLOR(1) "2)" ANSI_RESET " �ݺq���B"
		"�άO " ANSI_COLOR(1) "3)" ANSI_RESET " ���}: ",
#else
		"�п�� 1)�}�l�I�q 2)�ݺq�� 3)���}: ",
#endif
		ans, sizeof(ans), DOECHO);

	if (ans[0] == '1')
	    break;
	else if (ans[0] == '2') {
	    a_menu("�I�q�q��", SONGBOOK, 0, 0, NULL);
	    clear();
	}
	else if (ans[0] == '3') {
	    vmsg("���¥��{ :)");
	    unlockutmpmode();
	    return 0;
	}
    }

    reload_money();
    if (cuser.money < ORDER_SONG_COST) {
	move(22, 0);
	vmsgf("�I�q�n %d ����!....", ORDER_SONG_COST);
	unlockutmpmode();
	return 0;
    }

    getdata_str(19, 0, "�I�q��(�i�ΦW): ", sender, sizeof(sender), DOECHO, cuser.userid);
    getdata(20, 0, "�I��(�i�ΦW): ", receiver, sizeof(receiver), DOECHO);

    getdata_str(21, 0, "�Q�n�n��L(�o)��..:", say,
		sizeof(say), DOECHO, "�ڷR�p..");
    snprintf(save_title, sizeof(save_title),
	     "%s:%s", sender, say);
    getdata_str(22, 0, "�H��֪��H�c(�u�� ID �� E-mail)?",
		address, sizeof(address), LCECHO, receiver);
    vmsg("���ۭn��q�o..�i�J�q���n�n����@���q�a..^o^");
    a_menu("�I�q�q��", SONGBOOK, 0, 0, trans_buffer);
    if (!trans_buffer[0] || strstr(trans_buffer, "home") ||
	strstr(trans_buffer, "boards") || !(fp = fopen(trans_buffer, "r"))) {
	unlockutmpmode();
	return 0;
    }
#ifdef DEBUG
    vmsg(trans_buffer);
#endif
    strlcpy(filename, OSONGPATH, sizeof(filename));

    stampfile(filename, &mail);

    unlink(filename);

    if (!(fp1 = fopen(filename, "w"))) {
	fclose(fp);
	unlockutmpmode();
	return 0;
    }
    strlcpy(mail.owner, "�I�q��", sizeof(mail.owner));
    snprintf(mail.title, sizeof(mail.title), "�� %s �I�� %s ", sender, receiver);

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
	    const char *dot = "";
	    if (is_validuserid(sender) && strcmp(sender, cuser.userid) != 0)
		dot = ".";
	    po[0] = 0;
	    snprintf(genbuf, sizeof(genbuf), "%s%s%s%s", buf, sender, dot, po + 7);
	    strlcpy(buf, genbuf, sizeof(buf));
	}
	while ((po = strstr(buf, "<~Des~>"))) {
	    po[0] = 0;
	    snprintf(genbuf, sizeof(genbuf), "%s%s%s", buf, receiver, po + 7);
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

    log_filef("etc/osong.log",  LOG_CREAT, "id: %-12s �� %s �I�� %s : \"%s\", ��H�� %s\n", cuser.userid, sender, receiver, say, address);

    if (append_record(OSONGPATH "/" FN_DIR, &mail, sizeof(mail)) != -1) {
	pwcuSetLastSongTime(now);
	/* Jaky �W�L MAX_MOVIE ���q�N�}�l�� */
	// XXX ���J�����Ƿ|���o���O:
	// 3. �� <�t��> �ʺA�ݪO   SYSOP [01/23/08]
	// 4. �� <�I�q> �ʺA�ݪO   Ptt   [08/26/09]
	// 5. �� <�s�i> �ʺA�ݪO   SYSOP [08/22/09]
	// 6. �� <�ݪO> �ʺA�ݪO   SYSOP [04/16/09]
	// �ѩ��I�q������O�����J���A���ઽ���� MAX_MOVIE ���M�᭱���S�o���C
	nsongs = get_num_records(OSONGPATH "/" FN_DIR, sizeof(mail));
	if (nsongs > MAX_SONGS) {
	    // XXX race condition
	    delete_range(OSONGPATH "/" FN_DIR, 1, nsongs - MAX_SONGS);
	}
	snprintf(genbuf, sizeof(genbuf), "%s says \"%s\" to %s.", 
		sender, say, receiver);
	log_usies("OSONG", genbuf);
	vice(ORDER_SONG_COST, "�I�q");
    }
    snprintf(save_title, sizeof(save_title), "%s:%s", sender, say);
    hold_mail(filename, receiver, save_title);

    if (address[0]) {
	bsmtp(filename, save_title, address, NULL);
    }
    clear();
    outs(
	 "\n\n  ���߱z�I�q�����o...\n"
	 "  �@�p�ɤ��ʺA�ݪO�|�۰ʭ��s��s�A\n"
	 "  �j�a�N�i�H�ݨ�z�I���q�o�I\n\n"
	 "  �I�q��������D�i�H�� " BN_NOTE " �O����ذϧ䵪�סA\n"
	 "  �]�i�b " BN_NOTE " �O��ذϬݨ�ۤv���I�q�O���C\n"
	 "  �������_�Q���N���]�w��� " BN_NOTE " �ݪO�d�ܡA\n"
	 "  ���ˤ����O�D���z�A�ȡC\n");
    pressanykey();
    sortsong();
    topsong();

    unlockutmpmode();
    return 1;
}

int
ordersong(void)
{
    do_order_song();
    return 0;
}

// topsong

#define QCAST     int (*)(const void *, const void *)

typedef struct songcmp_t {
    char            name[100];
    char            cname[100];
    int             count;
}               songcmp_t;


static int
count_cmp(songcmp_t * b, songcmp_t * a)
{
    return (a->count - b->count);
}

int
topsong(void)
{
    more(FN_TOPSONG, YEA);
    return 0;
}


static void
sortsong(void)
{
    FILE           *fo, *fp = fopen(BBSHOME "/" FN_USSONG, "r");
    songcmp_t       songs[MAX_SONGS + 1];
    int             n;
    char            buf[256], cbuf[256];
    int totalcount = 0;

    memset(songs, 0, sizeof(songs));
    if (!fp)
	return;
    if (!(fo = fopen(FN_TOPSONG, "w"))) {
	fclose(fp);
	return;
    }
    totalcount = 0;
    /* XXX: ���F�e MAX_SONGS ��, �ѤU���|�Ƨ� */
    while (fgets(buf, 200, fp)) {
	chomp(buf);
	strip_blank(cbuf, buf);
	if (!cbuf[0] || !isprint2((int)cbuf[0]))
	    continue;

	for (n = 0; n < MAX_SONGS && songs[n].name[0]; n++)
	    if (!strcmp(songs[n].cname, cbuf))
		break;
	strlcpy(songs[n].name, buf, sizeof(songs[n].name));
	strlcpy(songs[n].cname, cbuf, sizeof(songs[n].cname));
	songs[n].count++;
	totalcount++;
    }
    qsort(songs, MAX_SONGS, sizeof(songcmp_t), (QCAST) count_cmp);
    fprintf(fo,
	    "    " ANSI_COLOR(36) "�w�w" ANSI_COLOR(37) "�W��" ANSI_COLOR(36) "�w�w�w�w�w�w" ANSI_COLOR(37) 
	    "�q�W" ANSI_COLOR(36) "�w�w�w�w�w�w�w�w�w�w�w" ANSI_COLOR(37) "����" ANSI_COLOR(36)
	    "�w�w" ANSI_COLOR(32) "�@%d��" ANSI_COLOR(36) "�w�w" ANSI_RESET "\n", totalcount);
    for (n = 0; n < 100 && songs[n].name[0]; n++) {
	fprintf(fo, "      %5d. %-38.38s %4d " ANSI_COLOR(32) "[%.2f]" ANSI_RESET "\n", n + 1,
		songs[n].name, songs[n].count,
		(float)songs[n].count / totalcount);
    }
    fclose(fp);
    fclose(fo);
}
