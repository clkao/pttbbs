#!/usr/bin/perl
# $Id: weather.perl,v 1.8 2003/05/06 16:54:42 victor Exp $
#
# ����]���ܡA�ݬ� bbspost �����|�O�_���T�C
# �p�G�o�X�� post �S����H���i�ӬO�� URL �䤣��A�h�T�w�@�U�ण��ݨ�
# ������H���� WWW �� URL �O�_���T�C
# �z�פW�A�ΩҦ� Eagle BBS �t�C�C
#                                       -- Beagle Apr 13 1997

use lib '/home/bbs/bin';
use LocalVars;

open(BBSPOST, "| bin/webgrep>etc/weather.tmp");
# ���
open(DATE, "date +'%a %b %d %T %Y' |");
$date = <DATE>;
chop $date;
close DATE;

# Header
# ���e
#open(WEATHER, "$LYNX -assume_charset=big5 -assume_local_charset=big5 -dump http://www.cwb.gov.tw/V3.0/weather/text/W03.htm |");
open(WEATHER, "$LYNX -assume_charset=big5 -assume_local_charset=big5 -dump -nolist ftp://ftpsv.cwb.gov.tw/pub/forecast/W002.txt|");

while (<WEATHER>) {
  print BBSPOST if ($_ ne "\n");
}
close WEATHER;

# ñ�W��
print BBSPOST "\n--\n";
print BBSPOST "�ڬObeagle�Ҧ��i�R���p�氮...�����Ptt�A��\n";
print BBSPOST "--\n";
print BBSPOST "�� [Origin: ���G��p����] [From: [�Ų��P���]       ] ";

close BBSPOST;

