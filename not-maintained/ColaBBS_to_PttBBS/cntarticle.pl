#!/usr/bin/perl
# $Id$
use IO::All;
die "usage: cntarticle.pl [base dir]"
    if( !@ARGV );

@proc = @ARGV;
while( $dir = pop @proc ){
    print "converting: $dir\n";
    while( <$dir/*> ){
	next if( /^\./ );
	if( -d $_ ){
	    push @proc, $_;
	} elsif( /M\..*\.A/ ){
	    convert($_);
	}
    }
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
