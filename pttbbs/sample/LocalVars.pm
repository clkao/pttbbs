#!/usr/bin/perl
package LocalVars;
require Exporter;
@ISA = qw/Exporter/;
@EXPORT = qw/
    $hostname $MYHOSTNAME $FQDN $SMTPSERVER
    $BBSHOME $JOBSPOOL $TMP
    $TAR $LYNX $GREP
    $BLOGDATA $BLOGCACHE
    $BLOGdbname $BLOGdbhost $BLOGdbuser $BLOGdbpasswd $BLOGdefault
    $MANDATA $MANIDX $MANCACHE
/;

# host
$hostname = 'ptt';
$MYHOSTNAME = 'ptt.cc';
$FQDN = 'ptt.cc';
$SMTPSERVER = 'ptt2.cc';

# dir
$BBSHOME = '/home/bbs';
$JOBSPOOL = "$BBSHOME/jobspool";
$TMP = '/tmp';

# program
$TAR = '/usr/bin/tar';
$LYNX = '/usr/local/bin/lynx';   # /usr/ports/www/lynx
$GREP = '/usr/bin/grep';

# BLOG:
#    $BLOGDATA �O�Ψө�m Blog ��ƪ����|
#    $BLOGCACHE�O�Ψө�m Template compiled ��ƪ����|,
#              ���� apache owner �i�g�J
$BLOGDATA = '/home/bbs/blog/data';
$BLOGCACHE = '/home/bbs/blog/cache';
$BLOGdbname = 'myblog';
$BLOGdbhost = 'localhost';
$BLOGdbuser = 'root';
$BLOGdbpasswd = '';
$BLOGdefault = 'Blog';

$MANDATA = '/home/web/ptt.man.data';
$MANIDX = '/home/web/ptt.man.data';
$MANCACHE = '/home/web/ptt.man.cache';

1;
