/* $Id$ */
#include "bbs.h"

// Access Control List
// Holder of all access / permission related stuff
//
// Author: Hung-Te Lin (piaip)

///////////////////////////////////////////////////////////////////////////
// New ban system (store in user home)

#define BANNED_OBJECT_TYPE_USER     'u'
#define BANNED_OBJECT_TYPE_BORAD    'b'

static void
banned_make_tag_filename(const char *who, const char *object, char object_type,
                         size_t szbuf, char *buf, int create_folder) {
    char prefix[3] = { object_type, '_', 0 };

    if (!who)
        setuserfile(buf, FN_BANNED "/");
    else
        sethomefile(buf, who, FN_BANNED "/");
    if (create_folder && !dashd(buf)) {
        mkdir(buf, 0755);
    }
    strlcat(buf, prefix, szbuf);
    strlcat(buf, object, szbuf);
}

static int
banned_get_info(const char *filename,
                time4_t *pexpire, size_t szreason, char *reason) {
    FILE *fp = fopen(filename, "rt");
    int ts = 0;
    char buf[STRLEN];

    if(!fp)
        return 0;

    // banned file format:
    //  EXPIRE_TIME
    //  REASON
    fgets(buf, sizeof(buf), fp);
    if (pexpire) {
        sscanf(buf, "%u", &ts);
        *pexpire = (time4_t) ts;
    }
    fgets(buf, sizeof(buf), fp);
    if (szreason && reason) {
        chomp(buf);
        strlcpy(reason, buf, szreason);
    }
    fclose(fp);
    return 1;
}

static int
banned_set_info(const char *filename, time4_t expire, const char *reason) {
    FILE *fp = fopen(filename, "wt");
    if (!fp)
        return 0;
    if (!reason)
        reason = "";

    fprintf(fp, "%u\n%s\n", (unsigned)expire, reason);
    fclose(fp);
    return 1;
}

// return if 'who (NULL=self)' is banned by 'object'
static time4_t
is_banned_by(const char *who, const char *object, char object_type) {
    char tag_fn[PATHLEN];
    time4_t expire = 0;

    banned_make_tag_filename(who, object, object_type, sizeof(tag_fn),tag_fn,0);
    if (!dashf(tag_fn))
        return 0;

    // check expire
    if (banned_get_info(tag_fn, &expire, 0, NULL) && now > expire) {
        unlink(tag_fn);
        return 0;
    }
    return expire;
}

// add 'who (NULL=self)' to the ban list of 'object'
static int
ban_user_as(const char *who, const char *object, char object_type,
            time_t expire, const char *reason) {
    char tag_fn[PATHLEN];

    banned_make_tag_filename(who, object, object_type, sizeof(tag_fn),tag_fn,1);
    return banned_set_info(tag_fn, expire, reason);
}

time4_t
is_user_banned_by_board(const char *user, const char *board) {
    return is_banned_by(user, board, BANNED_OBJECT_TYPE_BORAD);
}

time4_t
is_banned_by_board(const char *board) {
    return is_user_banned_by_board(cuser.userid, board);
}

int
ban_user_for_board(const char *user, const char *board,
                   time4_t expire, const char *reason) {
    return ban_user_as(user, board, BANNED_OBJECT_TYPE_BORAD, expire, reason);
}

int
unban_user_for_board(const char *user, const char *board) {
    char tag_fn[PATHLEN];

    banned_make_tag_filename(user, board, BANNED_OBJECT_TYPE_BORAD,
                             sizeof(tag_fn), tag_fn, 0);
    return unlink(tag_fn) == 0;
}

int
edit_banned_list_for_board(const char *board) {
    // TODO generalize this.
    int result;
    char uid[IDLEN+1], ans[3];
    char history_log[PATHLEN];
    char reason[STRLEN];
    char datebuf[STRLEN];
    time4_t expire = now;

    if (!board || !*board || getbnum(board) < 1)
        return 0;

    setbfile(history_log, board, FN_BANNED_HISTORY);

    while (1) {
        clear();
        vs_hdr2f(" �]�w�ݪO���� \t �ݪO: %s", board);
        getdata(1, 0, "�n (A)�W�[ (D)���e�M�� (L)�C�X������v (Q)����? [Q] ",
                ans, sizeof(ans), LCECHO);
        if (*ans == 'q' || !*ans)
            break;

        switch (*ans) {
            case 'a':
                move(1, 0);
                usercomplete(msg_uid, uid);
                if (!*uid || !searchuser(uid, uid))
                    continue;
                if (is_user_banned_by_board(uid, board)) {
                    vmsg("�ϥΪ̤w�b�����C");
                    continue;
                }
                move(1, 0); clrtobot();
                prints("�N�ϥΪ� %s �[�J�ݪO %s ������C", uid, board);
                syncnow();
                move(4, 0);
                outs("�ثe�������榡�O [�Ʀr][���]�C ��즳: �~(y), ��(m), ��(d)\n");
                outs("�d��: 3m (�T�Ӥ�), 180d (180��), 10y (10�~)\n");
                outs("�`�N���i�V�X��J(ex: �S���T�ӥb��o�تF��,�n���⦨�Ѽ�\n");
                getdata(2, 0, "�ХH�Ʀr����(�w�]����)��J����: ", datebuf, 8, DOECHO);
                trim(datebuf);
                if (!*datebuf) {
                    vmsg("����J�����A���C");
                    continue;
                } else {
                    int val = atoi(datebuf);
                    switch(tolower(datebuf[strlen(datebuf)-1])) {
                        case 'y':
                            val *= 365;
                            break;
                        case 'm':
                            val *= 30;
                            break;
                        case 'd':
                        default:
                            break;
                    }
                    if (val < 1) {
                        vmsg("����榡��J���~�άO�p��@�ѵL�k�B�z�C");
                        continue;
                    }
                    expire = now + val * DAY_SECONDS;
                    move(4, 0); clrtobot();
                    prints("��������N�]�w�� %d �ѫ�: %s",
                            val, Cdatelite(&expire));
                }

                assert(sizeof(reason) >= TTLEN);
                // maybe race condition here, but fine.
                getdata(5, 0, "�п�J�z��(�ťեi����): ", reason, TTLEN,DOECHO);
                if (!*reason) {
                    vmsg("����J�z�ѡA�L�k���z�C");
                    continue;
                }

                sprintf(datebuf, "%s", Cdatelite(&expire));
                move(1, 0); clrtobot();
                prints("\n�ϥΪ� %s �Y�N�[�J���� (�Ѱ��ɶ�: %s)\n"
                       "�z��: %s\n", uid, datebuf, reason);

                // last chance
                getdata(5, 0, "�T�{�H�W��ƥ������T�ܡH [y/N]: ",
                        ans, sizeof(ans), LCECHO);
                if (ans[0] != 'y') {
                    vmsg("�Э��s��J");
                    continue;
                }

                result = ban_user_for_board(uid, board, expire, reason);
                log_filef(history_log, LOG_CREAT,
                          "%s %s%s �N %s �[�J���� (�Ѱ��ɶ�: %s)�A�z��: %s\n",
                          Cdatelite(&now),
                          result ? "" : "(�����\\)",
                          cuser.userid, uid, datebuf, reason);
                vmsg(result ? "�w�N�ϥΪ̥[�J����" : "���ѡA�ЦV�������i");
                invalid_board_permission_cache(board);
                break;

            case 'd':
                move(1, 0);
                usercomplete(msg_uid, uid);
                if (!*uid || !searchuser(uid, uid))
                    continue;
                if (!is_user_banned_by_board(uid, board)) {
                    vmsg("�ϥΪ̥��Q����C");
                    continue;
                }
                move(1, 0); clrtobot();
                prints("���e�Ѱ��ϥΪ� %s ��ݪO %s ������C", uid, board);
                assert(sizeof(reason) >= TTLEN);
                getdata(2, 0, "�п�J�z��(�ťեi�����Ѱ�): ",reason,TTLEN,DOECHO);
                if (!*reason) {
                    vmsg("����J�z�ѡA�L�k���z�C");
                    continue;
                }
                unban_user_for_board(uid, board);
                log_filef(history_log, LOG_CREAT,
                          "%s %s �Ѱ� %s ������A�z��: %s\n",
                          Cdatelite(&now), cuser.userid, uid, reason);
                vmsg("�ϥΪ̤���w�Ѱ��C");
                invalid_board_permission_cache(board);
                break;

            case 'l':
                if (more(history_log, YEA) == -1)
                    vmsg("�ثe�|�L����O���C");
                break;

            default:
                break;
        }
    }
    return 0;
}
