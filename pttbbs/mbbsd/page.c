/* $Id: page.c,v 1.10 2003/01/19 16:06:06 kcwu Exp $ */
#include "bbs.h"

#define hpressanykey(a) {move(22, 0); prints(a); pressanykey();}
static void
filt_railway(char *fpath)
{
    char            buf[256], tmppath[32];
    FILE           *fp = fopen(fpath, "w"), *tp;

    snprintf(tmppath, sizeof(tmppath), "%s.railway", fpath);
    if (!fp || !(tp = fopen(tmppath, "r"))) // XXX fclose(fp) if tp fail
	return;

    while (fgets(buf, 255, tp)) {
	if (strstr(buf, "INLINE"))
	    continue;
	if (strstr(buf, "LINK"))
	    break;
	fprintf(fp, "%s", buf);
    }
    fclose(fp);
    fclose(tp);
    unlink(tmppath);
}

int
main_railway()
{
    fileheader_t    mhdr;
    char            genbuf[200];
    int             from, to, time_go, time_reach;
    char            tt[2], type[2];
    char            command[256], buf[8];
    char           *addr[] = {
	"��", "�K��", "�C��", "����", "����", "�n��", "�Q�s", "�x�_", "�U��",
	"�O��", "��L", "�s��", "�a�q", "���", "���c", "���c", "�H��", "����",
	"��f", "�s��", "�˥_", "�s��", "���s", "�T��", "�˫n", "�y��", "�״I",
	"�ͤ�", "�j�s", "���s", "�s��", "�ըF��", "�s�H", "�q�]", "�b��",
	"��n", "�j��", "�O����", "�M��", "�F��", "�s��", "�j�{", "�l��",
	"�]��", "�n��", "���r", "�T�q", "�ӿ�", "���w", "�Z��", "�׭�", "��l",
	"�x��", "�Q��", "���\\", "����", "���", "���L", "�ùt", "���Y",
	"�Ф�", "�G��", "�L��", "�ۺh", "�椻", "��n", "���t", "�j�L",
	"����", "�Ÿq", "���W", "�n�t", "���", "�s��", "�h��", "�L����",
	"����", "�ުL", "����", "�s��", "�ñd", "�x�n", "�O�w", "���w",
	"�j��", "����", "���s", "���Y", "����", "����", "����", "��s",
	"�E����", "�̪F", NULL, NULL
    };

    setutmpmode(RAIL_WAY);
    clear();
    move(0, 25);
    prints("\033[1;37;45m �����d�ߨt�� \033[1;44;33m�@��:Heat\033[m");
    move(1, 0);
    outs("\033[1;33m\n"
	 " 1.��    16.���c     31.�s��     46.���r     61.�Ф�    76.�L����   91.����\n"
	 " 2.�K��    17.�H��     32.�ըF��   47.�T�q     62.�G��    77.����     92.��s\n"
	 " 3.�C��    18.����     33.�s�H     48.�ӿ�     63.�L��    78.�ުL     93.�E����\n"
	 " 4.����    19.��f     34.�q�]     49.���w     64.�ۺh    79.����     94.�̪F\n"
       " 5.����    20.�s��     35.�b��     50.�Z��     65.�椻    80.�s��\n"
       " 6.�n��    21.�˥_     36.��n     51.�׭�     66.��n    81.�ñd\n"
       " 7.�Q�s    22.�s��     37.�j��     52.��l     67.���t    82.�x�n\n"
       " 8.�x�_    23.���s     38.�O����   53.�x��     68.�j�L    83.�O�w\n"
       " 9.�U��    24.�T��     39.�M��     54.�Q��     69.����    84.���w\n"
      "10.�O��    25.�˫n     40.�F��     55.���\\     70.�Ÿq    85.�j��\n"
       "11.��L    26.�y��     41.�s��     56.����     71.���W    86.����\n"
       "12.�s��    27.�״I     42.�j�{     57.���     72.�n�t    87.���s\n"
       "13.�a�q    28.�ͤ�     43.�l��     58.���L     73.���    88.���Y\n"
       "14.���    29.�j�s     44.�]��     59.�ùt     74.�s��    89.����\n"
	 "15.���c    30.���s     45.�n��     60.���Y     75.�h��    90.����\033[m");

    getdata(17, 0, "\033[1;35m�A�T�w�n�j�M��?[y/n]:\033[m", buf, 2, LCECHO);
    if (buf[0] != 'y' && buf[0] != 'Y')
	return 0;
    while (1)
	if (getdata(18, 0, "\033[1;35m�п�J�_��(1-94):\033[m", buf, 3, LCECHO) &&
	    (from = atoi(buf)) >= 1 && from <= 94)
	    break;
    while (1)
	if (getdata(18, 40, "\033[1;35m�п�J�ت��a(1-94):\033[m",
		    buf, 3, LCECHO) &&
	    (to = atoi(buf)) >= 1 && to <= 94)
	    break;
    while (1)
	if (getdata(19, 0, "\033[1;35m�п�J�ɶ��Ϭq(0-23) ��:\033[m",
		    buf, 3, LCECHO) &&
	    (time_go = atoi(buf)) >= 0 && time_go <= 23)
	    break;
    while (1)
	if (getdata(19, 40, "\033[1;35m��:\033[m", buf, 3, LCECHO) &&
	    (time_reach = atoi(buf)) >= 0 && time_reach <= 23)
	    break;
    while (1)
	if (getdata(20, 0, "\033[1;35m�Q�d�� 1:�︹�֨�  2:���q����\033[m",
		    type, 2, LCECHO) && (type[0] == '1' || type[0] == '2'))
	    break;
    while (1)
	if (getdata(21, 0, "\033[1;35m���d�� 1:�X�o�ɶ�  2:��F�ɶ�\033[m",
		    tt, sizeof(tt), LCECHO) &&
	    (tt[0] == '1' || tt[0] == '2'))
	    break;
    sethomepath(genbuf, cuser.userid);
    stampfile(genbuf, &mhdr);
    strlcpy(mhdr.owner, "Ptt�j�M��", sizeof(mhdr.owner));
    strncpy(mhdr.title, "�����ɨ�j�M���G", TTLEN);

    snprintf(command, sizeof(command), "echo \"from-station=%s&to-station=%s"
	    "&from-time=%02d00&to-time=%02d00&tt=%s&type=%s\" | "
	    "lynx -dump -post_data "
	    "\"http://www.railway.gov.tw/cgi-bin/timetk.cgi\" > %s.railway",
	    addr[from - 1], addr[to - 1], time_go, time_reach,
	    (tt[0] == '1') ? "start" : "arriv",
	    (type[0] == '1') ? "fast" : "slow", genbuf);

    system(command);
    filt_railway(genbuf);
    sethomedir(genbuf, cuser.userid);
    if (append_record(genbuf, &mhdr, sizeof(mhdr)) == -1)
	return -1;
    hpressanykey("\033[1;31m�ڭ̷|��j�M���G�ܧִN�H���A��  ^_^\033[m");
    return 0;
}
