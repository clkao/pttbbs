#!/usr/bin/perl
use LocalVars;
# �C�A���@, �Q���N�|�]�@�� $ARGV[0]
# ��ƨӷ� http://tw.weathers.yahoo.com/
open FH, "$LYNX -source http://tw.weathers.yahoo.com/ | grep '����'|";
$din = <FH>;
close FH;

($month, $day) = $din =~ /�A�� (.*?)�� (.*?)��/;
system($ARGV[0]) if( $day eq '�@' || $day eq '�Q��' );
