#!/usr/bin/perl
# $Id$
use lib '/home/bbs/bin/';
use LocalVars;
use Time::Local;
use POSIX;
use FileHandle;
use strict;
use Mail::Sender;

my($fndes, $fnsrc, $userid, $mailto, $outmode);
foreach $fndes ( <$JOBSPOOL/water.des.*> ){ #des: userid, mailto, outmode
    (open FH, "< $fndes") or next;
    chomp($userid = <FH>);
    chomp($mailto = <FH>);
    chomp($outmode= <FH>);
    close FH;
    next if( !$userid );
    print "$userid, $mailto, $outmode\n";
    `rm -Rf $TMP/water`;
    `mkdir -p $TMP/water`;

    $fnsrc = $fndes;
    $fnsrc =~ s/\.des\./\.src\./;
    eval{
	process($fnsrc, "$TMP/water/", $outmode, $userid);
    };
    if( $@ ){
	print "$@\n";
    }
    else{
	chdir "$TMP/water";
	if( $mailto eq '.' || $mailto =~ /\.bbs/ ){
	    $mailto = "$userid.bbs\@$hostname" if( $mailto eq '.' );
	    foreach my $fn ( <$TMP/water/*> ){
		my $who = substr($fn, rindex($fn, '/') + 1);
		my $content = '';
		open FH, "< $fn";while( <FH> ){chomp;$content .= "$_\n";}
		if( !MakeMail({mailto  => $mailto,
			       subject => "�M $who �����y�O��",
			       body    => $content,
			   }) ){ print "fault\n"; }
	    }
	    unlink $fnsrc;
	    unlink $fndes;
	}
	else{
	    my $body = 
		"�˷R���ϥΪ̱z�n:\n\n".
		"�w��z�ϥ� Ptt�t�C�����y��z�\�� ^_^\n".
		"���y��z�����G�Q���Y�n���[�b���H��\n".
		"�z���n���N������Y (�p�� tar+gunzip, winzip ���{��)\n".
		"�ѥX�Ӫ��ɮ׬��¤�r�榡, \n".
		"�z�i�H�z�L����¤�r�s��{�� (�p emacs, notepad, word)\n".
		"���}���i��s���z\n\n".
		"�A���P�±z�ϥΥ��t�ΥH�ι� $hostname ����� ^^\n".
		"\n $hostname �����s ". POSIX::ctime(time());
	    if( MakeMail({tartarget => "$TMP/$userid.waterball.tgz",
			  tarsource => "*",
			  mailto    => "$userid <$mailto>",
			  subject   => "���y����",
			  body      => $body}) ){
		unlink $fnsrc;
		unlink $fndes;
	    }
	}
    }
}

sub process
{
    my($fn, $outdir, $outmode, $me) = @_;
    my($cmode, $who, $time, $say, $orig, %FH, %LAST, $len);
    open DIN, "< $fn";
    while( <DIN> ){
	chomp;
	next if( !(($cmode, $who, $time, $say, $orig) = parse($_)) );
	next if( !$who );

	if( ! $FH{$who} ){
	    $FH{$who} = new FileHandle "> $outdir/$who";
	}
	if( $outmode == 0 ){
	    next if( $say =~ /<<�U���q��>> -- �ڨ��o�I/ ||
		     $say =~ /<<�W���q��>> -- �ڨӰաI/    );
	    if( $time - $LAST{$who} > 1800 ){
		if( $LAST{$who} != 0 ){
		    ($FH{$who})->print( POSIX::ctime($LAST{$who}) , "\n");
		}
		($FH{$who})->print( POSIX::ctime($time) );
		$LAST{$who} = $time;
	    }
	    $len = (length($who) > length($me) ? length($who) : length($me))+1;
	    ($FH{$who})->printf("%-${len}s %s\n", ($cmode?$who:$me).':', $say);
	}
	elsif( $outmode == 1 ){
	    ($FH{$who})->print("$orig\n");
	}
    }
    if( $outmode == 0 ){
	foreach( keys %FH ){
	    ($FH{$_})->print( POSIX::ctime($LAST{$_}) );
	}
    }
    foreach( keys %FH ){
	($FH{$_})->close();
    }
    close DIN;
}

sub parse
{
    my $dat = $_[0];
    my($cmode, $who, $year, $month, $day, $hour, $min, $sec, $say);
    if( $dat =~ /^To/ ){
	$cmode = 0;
	($who, $say, $month, $day, $year, $hour, $min, $sec) =
	    $dat =~ m|^To (\w+):\s*(.*)\[(\d+)/(\d+)/(\d+) (\d+):(\d+):(\d+)\]|;
    }
    else{
	$cmode = 1;
	($who, $say, $month, $day, $year, $hour, $min, $sec) =
	    $dat =~ m|��(\w+?)\[37;45m\s*(.*).*?\[(\w+)/(\w+)/(\w+) (\w+):(\w+):(\w+)\]|;

    }
#    $time = timelocal($sec,$min,$hours,$mday,$mon,$year);

    return undef if( $month == 0 );
    return ($cmode, $who, timelocal($sec, $min, $hour, $day, $month - 1, $year), $say, $_[0]);
}

sub MakeMail
{
    my($arg) = @_;
    my $sender;
    `$TAR zcf $arg->{tartarget} $arg->{tarsource}`
	if( $arg->{tarsource} );
    $sender = new Mail::Sender{smtp => $SMTPSERVER,
			       from => "$hostname���y��z�{�� <$userid.bbs\@$MYHOSTNAME>"};
    foreach( 0..3 ){
	if( (!$arg->{tartarget} &&
	     $sender->MailMsg({to      => $arg->{mailto},
			       subject => $arg->{subject},
			       msg     => $arg->{body}
			   }) ) ||
	    ($arg->{tartarget} && 
	     $sender->MailFile({to      => $arg->{mailto},
				subject => $arg->{subject},
				msg     => $arg->{body},
				file    => $arg->{tartarget}})) ){
		unlink $arg->{tartarget} if( $arg->{tartarget} );
		return 1;
	    }
    }
    $sender->MailMsg({to      => "$userid.bbs\@$MYHOSTNAME",
		      subject => "�L�k�H�X���y��z",
		      msg     =>
			  "�˷R���ϥΪ̱z�n\n\n".
			  "�A�����y��z�O���L�k�H�F���w��m $mailto \n\n".
			  "$hostname�����s �q�W ".POSIX::ctime(time())});
    unlink $arg->{tartarget} if( $arg->{tartarget} );
    return 1;
}
