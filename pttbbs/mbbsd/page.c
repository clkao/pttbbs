/* $Id$ */
#include "bbs.h"

#define hpressanykey(a) {move(22, 0); outs(a); pressanykey();}
#define TITLE "\033[1;37;45m �����d�ߨt�� \033[1;44;33m��@��:Heat\033[m"

static void
print_station(const char * const addr[6][100], int path, int *line, int *num)
{
	int i;

	*num = 0;
	move(*line,0);
	do{
		for(i=0; i<7 && addr[path - 1][*num]!=NULL; i++){
			prints(" %2d.%-6s", (*num)+1, addr[path - 1][*num]);
			(*num)++;
		}
		outc('\n');
		(*line)++;
	}while(i==7);
}

int
main_railway()
{
    fileheader_t    mhdr;
    char            genbuf[200];
    int             from, to, time_go, time_reach, date, path;
    int             line, station_num;
    char            tt[2], type[2];
    char            command[256], buf[8];
	static const char * const addr[6][100] = {
		{
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
			"�E����", "�̪F", NULL
		},
		{
			"��L", "�O��", "�U��", "�x�_", "�Q�s", "�n��", "����", "��", "�K��",
			"�x�x", "�|�}�F", "���", "�Jֻ", "�T�I��", "�d��", "����", "�^�d",
			"�ֶ�", "�۫�", "�j��", "�j��", "�t�s", "�~�D", "�Y��", "���H", "�G��",
			"�|��", "�y��", "�G��", "����", "ù�F", "�V�s", "�s��", "Ĭ�D�s��",
			"Ĭ�D", "�ü�", "�F�D", "�n�D", "�Z��", "�~��", "�M��", "�M��", "�R�w",
			"�s��", "����", "�_�H", "�Ὤ", "�N�w", "�Ӿ�", "���M", "����", "�ץ�",
			"�ˤf", "�n��", "��L", "�U�a", "���_", "�j�I", "�I��", "��_", "���J",
			"�T��", "�ɨ�", "�w�q", "�F��", "�F��", "�I��", "���W", "����", "���s",
			"���", "��M", "�緽", "����", "�s��", "�x�F", NULL
		},
		{
			"����", "��s", "����", "�E����", "������", "�̪F", "�k��", "�ﬥ",
			"���", "�˥�", "��{", "�r��", "�n�{", "��w", "�L��", "�ΥV", "�F��",
			"�D�d", "�[�S", "����", "�D�s", "�j��", "�j�Z", "�]��", "�h�}", "���[",
			"�ӳ¨�", "����", "�d��", "�x�F", NULL
		},
		{
			"�K��", "�x�x", "�|�}�F", "���", "�Jֻ", "�T�I��", "�j��", "�Q��",
			"��j", "���}", "����", "�׮�", NULL
		},
		{
			"�s��", "�ˤ�", "�W��", "�a��", "�˪F", "��s", "�E�g�Y", "�X��", "�n�e",
			"���W", NULL
		},
		{
			"�x��", "�Q��", "���\\", "����", "���", "���L", "�ùt", "���Y", "�Ф�",
			"�G��", "���u", "�B��", "�s�u", "����", "����", "���L", NULL
		}
	};

    setutmpmode(RAIL_WAY);
    clear();
    move(0, 25);
    outs(TITLE);
    move(1, 0);

    getdata(3, 0, "\033[1;35m�A�T�w�n�j�M��?[y/n]:\033[m", buf, 2, LCECHO);
    if (buf[0] != 'y' && buf[0] != 'Y')
	return 0;
    outs("\033[1;33m1.�賡�F�u(�t�x���u)  2.�F���F�u(�t�_�j�u)\n");
    outs("\033[1;33m3.�n�j�u  4.���˽u  5.���W�u  6.�����u\n");
    while (1)
    if (getdata(7, 0, "\033[1;35m�п�ܸ��u(1-6):\033[m", buf, 2, LCECHO) &&
   	 (path = atoi(buf)) >= 1 && path <= 6)
	    break;

    clear();
    move(0, 25);
    outs(TITLE);
	line = 3;
	print_station(addr, path, &line, &station_num);
    sprintf(genbuf, "\033[1;35m�п�J�_��(1-%d):\033[m", station_num);
    while (1)
	if (getdata(line, 0, genbuf, buf, 3, LCECHO) && (from = atoi(buf)) >= 1 && from <= station_num)
	   	break;
    sprintf(genbuf, "\033[1;35m�п�J�ׯ�(1-%d):\033[m", station_num);
    while (1)
	if (getdata(line, 40, genbuf, buf, 3, LCECHO) && (to = atoi(buf)) >= 1 && to <= station_num)
	   	break;
	line++;
	
    while (1)
	if (getdata(line, 0, "\033[1;35m�п�J�ɶ��Ϭq(0-23) ��:\033[m",
		    buf, 3, LCECHO) &&
	    (time_go = atoi(buf)) >= 0 && time_go <= 23)
	    break;
    while (1)
	if (getdata(line, 40, "\033[1;35m��:\033[m", buf, 3, LCECHO) &&
	    (time_reach = atoi(buf)) >= 0 && time_reach <= 23)
	    break;
	line++;
	if (path<=3){
    while (1)
		if (getdata(line, 0, "\033[1;35m�Q�d�� 1:�︹�֨�  2:���q����\033[m",
		    	type, 2, LCECHO) && (type[0] == '1' || type[0] == '2'))
	    	break;
		line++;
	}
    while (1)
	if (getdata(line, 0, "\033[1;35m���d�� 1:�X�o�ɶ�  2:��F�ɶ�\033[m",
		    tt, sizeof(tt), LCECHO) &&
	    (tt[0] == '1' || tt[0] == '2'))
	    break;
	line++;
    while (1)
	if (getdata(line, 0, "\033[1;35m�п�J���d�ߤ��(0-29)�ѫ�\033[m",
		    buf, 3, LCECHO) && (date = atoi(buf))>=0 && date<=29)
	    break;
	line++;

    sethomepath(genbuf, cuser.userid);
    stampfile(genbuf, &mhdr);
    strlcpy(mhdr.owner, "Ptt�j�M��", sizeof(mhdr.owner));
    strncpy(mhdr.title, "�����ɨ�j�M���G", TTLEN);

    snprintf(command, sizeof(command), "echo \"path=%d from-station=%s to-station=%s"
	    " from-time=%02d to-time=%02d tt=%s type=%s date=%d\" | "BBSHOME"/bin/railway_wrapper.pl > %s",
	    path, addr[path - 1][from - 1], addr[path - 1][to - 1], time_go, time_reach,
	    (tt[0] == '1') ? "start" : "arriv",
	    (type[0] == '1') ? "fast" : "slow", date, genbuf);

    system(command);
    sethomedir(genbuf, cuser.userid);
    if (append_record(genbuf, &mhdr, sizeof(mhdr)) == -1)
	return -1;
    hpressanykey("\033[1;31m�ڭ̷|��j�M���G�ܧ֦a�H���A��  ^_^\033[m");
    return 0;
}
