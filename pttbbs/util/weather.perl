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

weather_report('etc/weather.today', 'ftp://ftpsv.cwb.gov.tw/pub/forecast/W002.txt');
weather_report('etc/weather.tomorrow', 'ftp://ftpsv.cwb.gov.tw/pub/forecast/W003.txt');

sub weather_report
{
    my ($file, $link) = @_;
    open(BBSPOST, "> $file");

# Header
# ���e
    open(WEATHER, "$LYNX -assume_charset=big5 -assume_local_charset=big5 -dump -nolist $link|");

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
}
