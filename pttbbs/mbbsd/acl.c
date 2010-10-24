/* $Id$ */
#include "bbs.h"

// Access Control List
// Holder of all access / permission related stuff
//
// Author: Hung-Te Lin (piaip)

///////////////////////////////////////////////////////////////////////////
// Bakuman: New banning system (store in user home)

#define BAKUMAN_OBJECT_TYPE_USER     'u'
#define BAKUMAN_OBJECT_TYPE_BOARD    'b'
#define BAKUMAN_REASON_LEN          (BTLEN)

static void
bakuman_make_tag_filename(const char *who, const char *object, char object_type,
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
bakuman_get_info(const char *filename,
                 time4_t *pexpire, size_t szreason, char *reason) {
    FILE *fp = fopen(filename, "rt");
    int ts = 0;
    char buf[STRLEN];

    if(!fp)
        return 0;

    // banned file format:
    //  EXPIRE_TIME
    //  REASON
    buf[0] = 0;
    fgets(buf, sizeof(buf), fp);
    if (pexpire) {
        sscanf(buf, "%u", &ts);
        *pexpire = (time4_t) ts;
    }
    buf[0] = 0;
    fgets(buf, sizeof(buf), fp);
    if (szreason && reason) {
        chomp(buf);
        strlcpy(reason, buf, szreason);
    }
    fclose(fp);
    return 1;
}

static int
bakuman_set_info(const char *filename, time4_t expire, const char *reason) {
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

    bakuman_make_tag_filename(who, object, object_type,
                              sizeof(tag_fn), tag_fn, 0);
    if (!dashf(tag_fn))
        return 0;

    // check expire
    if (bakuman_get_info(tag_fn, &expire, 0, NULL) && now > expire) {
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

    bakuman_make_tag_filename(who, object, object_type,
                              sizeof(tag_fn), tag_fn, 1);
    return bakuman_set_info(tag_fn, expire, reason);
}

time4_t
is_user_banned_by_board(const char *user, const char *board) {
    return is_banned_by(user, board, BAKUMAN_OBJECT_TYPE_BOARD);
}

time4_t
is_banned_by_board(const char *board) {
    return is_user_banned_by_board(cuser.userid, board);
}

int
ban_user_for_board(const char *user, const char *board,
                   time4_t expire, const char *reason) {
    return ban_user_as(user, board, BAKUMAN_OBJECT_TYPE_BOARD, expire, reason);
}

int
unban_user_for_board(const char *user, const char *board) {
    char tag_fn[PATHLEN];

    bakuman_make_tag_filename(user, board, BAKUMAN_OBJECT_TYPE_BOARD,
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
        vs_hdr2f(" Bakuman �v���]�w�t�� \t"
                 " �ݪO: %s �A����: �T��o��(����)�A�W��W��: ��", board);
        move(4, 0);
        outs(ANSI_COLOR(1)
        "                   �w��ϥ� Bakuman �v���]�w�t��!\n\n" ANSI_RESET
        "                        ���t�δ��ѤU�C�\\��:\n"
        ANSI_COLOR(1;33)
        "                          - �L�W�����W��]�w\n"
        "                          - �۰ʥͮĪ��ɮĭ���\n"
        "                          - �W�椺�±b���L�����s���U�ɦ۰ʨ���\n\n"
        ANSI_RESET
        "      �p����: ���¨t�εy�����P�A�s�t�Τ��|(�]�L�k)�C�X [�{�b�W�椺�����ǤH]\n"
        "              ���Ĩ����O�]�ᤣ�z+�O�����������G\n"
        "            - ���ɥ�(A)�s�W�ó]�n������N���ΦA�h�޳]�F���ǤH�C\n"
        "            - ���D�Q���e�Ѱ��εo�{�]���A���ɥi��(D)���R���M��A��(A)���s�]�w\n"
        "            - �Q�T�{�O�_�]���άd�Y�ӨϥΪ̬O���O���b�����A�i��(S)���ˬd�C\n"
        "              �t�~�]�i��(L)�ݳ]�w�O���C\n\n"
#ifdef WATERBAN_UPGRADE_TIME_STR
        // enable and change this if you've just made an upgrade
        ANSI_COLOR(1;32)
        "      ��: �t�Χ�s�ɤw��Ҧ��z��b�¤���W�檺�b�������]�W�F" 
                   WATERBAN_UPGRADE_TIME_STR "������A\n"
        "          ���S���O���b(L)���C��̭��C �z�i�H�Q��(O)���¦W��ѦҦ��S���Q\n"
        "          �ק諸�����A�M��Q��(D)��(A)�ӽվ�C\n" ANSI_RESET
#endif
        "");

        getdata(1, 0, "(A)�W�[ (D)���e�M�� (S)���o�ثe���A "
                      "(L)�C�X�]�w���v (O)�˵��¤��� [Q]����? ",
                ans, 2, LCECHO);
        move(2, 0); clrtobot();
        if (*ans == 'q' || !*ans)
            break;

        switch (*ans) {
            case 's':
                move(1, 0);
                usercomplete(msg_uid, uid);
                if (!*uid || !searchuser(uid, uid))
                    continue;
                move(1, 0); clrtobot();
                expire = is_user_banned_by_board(uid, board);
                if (expire > now) {
                    prints("�ϥΪ� %s �T�����A�Ѱ��ɶ�: %s\n",
                           uid, Cdatelite(&expire));
                } else {
                    prints("�ϥΪ� %s �ثe���b�T���W�椤�C\n",
                           uid);
                }
                pressanykey();
                break;

            case 'a':
                move(1, 0);
                usercomplete(msg_uid, uid);
                if (!*uid || !searchuser(uid, uid))
                    continue;
                if ((expire = is_user_banned_by_board(uid, board))) {
                    vmsgf("�ϥΪ̤��e�w�Q�T���A�|�� %d �ѡC",
                          (expire - now) / DAY_SECONDS+1);
                    continue;
                }
                move(1, 0); clrtobot();
                prints("�N�ϥΪ� %s �[�J�ݪO %s ���T���W��C", uid, board);
                syncnow();
                move(4, 0);
                outs("�ثe�������榡�O [�Ʀr][���]�C "
                     "��즳: �~(y), ��(m), ��(d)\n"
                     "�d��: 3m (�T�Ӥ�), 180d (180��), 10y (10�~)\n"
                     "�`�N���i�V�X��J(��:�S���T�ӥb��o�تF��,�д��⦨�Ѽ�)\n"
                     );
                getdata(2, 0, "�ХH�Ʀr����(�w�]����)��J����: ",
                        datebuf, 8, DOECHO);
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
                    if (now + val * DAY_SECONDS < now) {
                        vmsg("����L�j�εL�k�B�z�A�Э��s��J�C");
                        continue;
                    }
                    expire = now + val * DAY_SECONDS;
                    move(4, 0); clrtobot();
                    // sprintf(datebuf, "%s", Cdatelite(&expire));
                    sprintf(datebuf, "%d ��", val);
                    prints("�����N�]�w�� %s����: %s",
                            datebuf, Cdatelite(&expire));
                }

                assert(sizeof(reason) >= BAKUMAN_REASON_LEN);
                // maybe race condition here, but fine.
                getdata(5, 0, "�п�J�z��(�ťեi�����s�W): ",
                        reason, BAKUMAN_REASON_LEN, DOECHO);
                if (!*reason) {
                    vmsg("����J�z�ѡA���������]�w");
                    continue;
                }

                move(1, 0); clrtobot();
                prints("\n�ϥΪ� %s �Y�N�[�J�T���W�� (����: %s)\n"
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
                          ANSI_COLOR(1) "%s %s" ANSI_COLOR(33) "%s" ANSI_RESET 
                          " ���� " ANSI_COLOR(1;31) "%s" ANSI_RESET 
                          " �o���A������ %s\n  �z��: %s\n",
                          Cdatelite(&now),
                          result ? "" : 
                            ANSI_COLOR(0;31)"[�t�ο��~] "ANSI_COLOR(1),
                          cuser.userid, uid,  datebuf, reason);
                vmsg(result ? "�w�N�ϥΪ̥[�J�T���W��" : "���ѡA�ЦV�������i");
                invalid_board_permission_cache(board);
                break;

            case 'd':
                move(1, 0);
                usercomplete(msg_uid, uid);
                if (!*uid || !searchuser(uid, uid))
                    continue;
                if (!(expire = is_user_banned_by_board(uid, board))) {
                    vmsg("�ϥΪ̥��b�T���W��C");
                    continue;
                }
                move(1, 0); clrtobot();
                prints("���e�Ѱ��ϥΪ� %s ��ݪO %s ���T������ (�|�� %d ��)�C",
                       uid, board, (expire-now)/DAY_SECONDS+1);
                assert(sizeof(reason) >= BAKUMAN_REASON_LEN);
                getdata(2, 0, "�п�J�z��(�ťեi�����Ѱ�): ",
                        reason, BAKUMAN_REASON_LEN, DOECHO);
                if (!*reason) {
                    vmsg("����J�z�ѡA���������]�w");
                    continue;
                }

                // last chance
                getdata(5, 0, "�T�{�H�W��ƥ������T�ܡH [y/N]: ",
                        ans, sizeof(ans), LCECHO);
                if (ans[0] != 'y') {
                    vmsg("�Э��s��J");
                    continue;
                }

                unban_user_for_board(uid, board);
                log_filef(history_log, LOG_CREAT,
                          ANSI_COLOR(1) "%s " ANSI_COLOR(33) "%s" ANSI_RESET
                          " �Ѱ� " ANSI_COLOR(1;32) "%s" ANSI_RESET 
                          " ���T������ (�Z������|�� %d ��)\n  �z��: %s\n",
                          Cdatelite(&now), cuser.userid, uid, 
                          (expire - now) / DAY_SECONDS+1, reason);
                vmsg("�ϥΪ̪��T������w�Ѱ��C");
                invalid_board_permission_cache(board);
                break;

            case 'l':
                if (more(history_log, YEA) == -1)
                    vmsg("�ثe�|�L�]�w�O���C");
                break;

            case 'o':
                {
                    char old_log[PATHLEN];
                    setbfile(old_log, board, fn_water);
                    if (more(old_log, YEA) == -1)
                        vmsg("�L�¤����ơC");
                }
                break;

            default:
                break;
        }
    }
    return 0;
}
