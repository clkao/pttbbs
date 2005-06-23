#!/usr/bin/perl
# $Id$
# usage: cd ColaBBS/home; perl cnthomedir.pl
use IO::All;
`mkdir -p ~/home/$_` foreach( 'a'..'z', 'A'..'Z' );

foreach( <*> ){
    next if( !-d $_ || /\./ || /^[A-Za-z]$/ );
    $USERDATA < io("$_/USERDATA.DAT");
    ($userid) = $USERDATA =~ /^(\w+)/;
    $c = substr($userid, 0, 1);

    `mv ~/home/$c/$userid ~/tmp/; mv $_ ~/home/$c/$userid`;
    print "$_ => $userid\n";
}
