#!/usr/bin/perl
# $Id$
use lib '/home/bbs/bin/';
use LocalVars;
use strict;
use Mail::Sender;
use POSIX;

no strict 'subs';
setpriority(PRIO_PROCESS, $$, 20);
use strict subs;
chdir $BBSHOME;
open LOG, ">> log/tarqueue.log";

foreach my $board ( <$JOBSPOOL/tarqueue.*> ){
    $board =~ s/.*tarqueue\.//;
    ProcessBoard($board);
    unlink "$JOBSPOOL/tarqueue.$board";
}
close DIR;
close LOG;

sub ProcessBoard
{
    my($board)= @_;
    my($cmd, $owner, $email, $bakboard, $bakman, $now);

    $now = substr(POSIX::ctime(time()), 0, -1);
    open FH, "< $JOBSPOOL/tarqueue.$board";
    chomp($owner = <FH>);
    chomp($email = <FH>);
    chomp(($bakboard, $bakman) = split(/,/, <FH>));
    close FH;

    print LOG sprintf("%-28s %-12s %-12s %d %d %s\n",
		      $now, $owner, $board, $bakboard, $bakman, $email);

    MakeMail({tartarget  => "$TMP/$board.tgz",
	      tarsource  => "boards/". substr($board, 0, 1). "/$board",
	      mailto     => "$board���O�D$owner <$email>",
	      subject    => "$board���ݪO�ƥ�",
	      from       => "$board���O�D$owner <$owner.bbs\@$MYHOSTNAME>",
	      body       =>
	    "\n\n\t $owner �z�n�A����o�ʫH�A��ܱz�w�g����ݪO�ƥ��C\n\n".
	    "\t���±z���@�ߵ��ݡA�H�Ψϥ� $hostname���ݪO�ƥ��t�ΡA\n\n".
	    "\t�p������ðݡA�w��H�H�������A�ڭ̷|�ܼ֩󵹤���U�C\n\n\n".
	    "\t�̫�A�� $owner ���w�ּ֡I ^_^\n\n\n".
	    "\t $hostname�����s. \n\t$now"
	    }) if( $bakboard );

    MakeMail({tartarget  => "$TMP/man.$board.tgz",
	      tarsource  => "man/boards/". substr($board, 0, 1). "/$board",
	      mailto     => "$board���O�D$owner <$email>",
	      subject    => "$board����ذϳƥ�",
	      from       => "$board���O�D$owner <$owner.bbs\@$MYHOSTNAME>",
	      body       =>
	    "\n\n\t $owner �z�n�A����o�ʫH�A��ܱz�w�g�����ذϳƥ��C\n\n".
	    "\t���±z���@�ߵ��ݡA�H�Ψϥ� $hostname���ݪO�ƥ��t�ΡA\n\n".
	    "\t�p������ðݡA�w��H�H�������A�ڭ̷|�ܼ֩󵹤���U�C\n\n\n".
	    "\t�̫�A�� $owner ���w�ּ֡I ^_^\n\n\n".
	    "\t $hostname�����s. \n\t$now"
	    }) if( $bakman );

}

sub MakeMail
{
    my($arg) = @_;
    my $sender;
    `$TAR zcf $arg->{tartarget} $arg->{tarsource}`;
    $sender = new Mail::Sender{smtp => $SMTPSERVER,
			       from => $arg->{from}};
    $sender->MailFile({to         => $arg->{mailto},       
		       subject    => $arg->{subject},
		       msg        => $arg->{body},
		       file       => $arg->{tartarget},
		       b_charset  => 'big5',
		       b_encoding => '8bit',
		       ctype      => 'application/x-tar-gz'});
    unlink $arg->{tartarget};
}
