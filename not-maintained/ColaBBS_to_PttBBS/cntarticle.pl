#!/usr/bin/perl
use IO::All;
die "usage: cntarticle.pl [base dir]"
    if( !@ARGV );

foreach( @ARGV ){
    print "converting: $_\n";
    convert($_)
	foreach( <$_/M.*.A> );
}

sub convert
{
    my($fn) = @_;
    $content < io($fn);
    $content =~ s/\r//gs;
    $content =~ s/^.*?m �@�� .*?m (.*\))\s+\S+? �H�� .*?m (\S+).*/�@��: $1 �ݪO: $2/m;
    $content =~ s/^.*?m ���D .*?m (.*?)\s+\S+m/���D: $1/m;
    $content =~ s/^.*?m �ɶ� .*?m (.*?)\s+\S+m/�ɶ�: $1/m;
    $content =~ s/^\e\[36m�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w\e\[m\n//m;
    "$content\n" > io($fn);
}
