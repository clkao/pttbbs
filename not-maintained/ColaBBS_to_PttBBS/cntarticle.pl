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
    $content =~ s/^.*?m 作者 .*?m (.*\))\s+\S+? 信區 .*?m (\S+).*/作者: $1 看板: $2/m;
    $content =~ s/^.*?m 標題 .*?m (.*?)\s+\S+m/標題: $1/m;
    $content =~ s/^.*?m 時間 .*?m (.*?)\s+\S+m/時間: $1/m;
    $content =~ s/^\e\[36m────────────────────────────────────────\e\[m\n//m;
    "$content\n" > io($fn);
}
