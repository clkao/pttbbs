#!/usr/bin/perl
# $Id: stock.perl,v 1.1 2002/03/07 15:13:46 in2 Exp $
#
# ����]���ܡA�ݬ� bbspost �����|�O�_���T�C
# �p�G�o�X�� post �S����H���i�ӬO�� URL �䤣��A�h�T�w�@�U�ण��ݨ�
# ������H���� WWW �� URL �O�_���T�C
# �z�פW�A�ΩҦ� Eagle BBS �t�C�C
#                                       -- Beagle Apr 13 1997
open(BBSPOST, "| bin/webgrep >etc/stock.tmp");
# ���
open(DATE, "date +'%a %b %d %T %Y' |");
$date = <DATE>;
chop $date;
close DATE;

# Header
# ���e
#open(WEATHER, "/usr/local/bin/lynx -dump http://www.dashin.com.tw/bulletin_board/today_stock_price.htm |"); while (<WEATHER>) {
open(WEATHER, "/usr/bin/lynx -dump http://quotecenter.jpc.com.tw/today_stock_price.htm |"); while(<WEATHER>) {
  print BBSPOST if ($_ ne "\n");
}
close WEATHER;

# ñ�W��
print BBSPOST "\n--\n";
print BBSPOST "�ڬObeagle�Ҧ��i�R���p�氮...�����Ptt�A��\n";
print BBSPOST "--\n";
print BBSPOST "�� [Origin: ���G��p����] [From: [�Ų��P���]       ] ";

close BBSPOST;

