#!/usr/bin/perl -w
# for saving memory, filter out unnecessary environment variables
use strict;

########################################
# mbbsd �|�Ψ쪺 external program:
# tar rm cat mv cp stty
# bin/railway_wrapper.pl -> lynx -> LANG
# bin/buildir bin/builddb.pl bin/xchatd
# mutt -> TMPDIR
# /usr/bin/uuencode /usr/sbin/sendmail
##########################################
# �Y�L getpwuid(2), mutt �|�ݭn HOME,USER

$ENV{PATH}="/bin:/usr/bin:/usr/local/bin";
#$ENV{SHELL}="/bin/sh";
my @acceptenv=qw(
        ^PATH$
        ^USER$ ^HOME$
        ^TZ$ ^TZDIR$ ^TMPDIR$
        ^MALLOC_
        );
# TERM SHELL PWD LANG LOGNAME
for my $env(keys %ENV) {
    delete $ENV{$env} if !grep { $env =~ $_ } @acceptenv;
}

exec { $ARGV[0] } @ARGV;
