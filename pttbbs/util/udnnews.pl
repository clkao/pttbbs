#!/usr/bin/perl
#
# �е��N�I
# ���{�����v�ݩ� PttBBS ,
# ���u��ܱz�i�H���}�K�O���o���{��,
# �ä���ܱz�i�H�ϥΥ��{��.
#
# ���{���N�����s�� udnnews�������o��e�s�D,
# �ӷs�D���e�����v�O�ݩ� �p�X�s�D�� �Ҧ�.
# ��Y, �z�Y�S���p�X�s�D�����ѭ����v,
# �z�áu����v�ϥΥ��{���U���Ӻ��s�D.
#

use lib '/home/bbs/bin/';
use LocalVars;
use strict;
use vars qw/@titles/;

chdir '/home/bbs';
getudnnewstitle(\@titles);
foreach( @titles ){
    postout({brdname => 'udnnews',
             title   => strreplace(FormatChinese($_->[1])),
             owner   => 'udnnews.',
             content => getudnnewscontent("http://www.udn.com/NEWS/FOCUSNEWS/$_->[0]", $_)});
}

sub strreplace
{
    my($str) = @_;
    $str =~ s/��/�Q/g;
    $str =~ s/��/��/g;
    $str =~ s/���\&\#22531\;/��}/g;
    return $str;
}

sub getudnnewscontent($$)
{
    my($url, $title) = @_;
    my($buf, $content, $ret);
    $buf = `$LYNX -source '$url'`;
    ($content) = $buf =~ m|<!-- start of content -->(.*?)<!-- end of content -->|s;
#    ($content) = $buf =~ m|<p><font color="#CC0033" class="text12">(.*?)<tr valign="top">|s if( !$content );
    $content =~ s/<p>/\n/gi;
    $content =~ s/<.*?>//g;
    $content =~ s/&nbsp;//g;
    $content =~ s/\r//g;
    $content =~ s/\n+/\n/gs;
    $content = strreplace($content);
    undef $ret;
    foreach( split(/\n/, $content) ){
        s/ //g;
        $ret .= FormatChinese($_, 60). "\n" if( $_ );
    }
    return
           "�@��: udnnews.(�p�X�s�D��) �ݪO: udnnews\n".
           "���D: $title\n".
           "�ɶ�: �Y��\n".
           "�� [����� $url ]\n\n$ret\n\n".
           "--\n\n �p�X�s�D�� http://www.udn.com/ �W�a���v�����~�{ ".
           "\n ���g���\\�Фžզۨϥ� ";
}

sub getudnnewstitle($)
{
    my($ra_titles) = @_;
    my($url, $title);
    open FH, "$LYNX -source http://www.udn.com/NEWS/FOCUSNEWS/ | $GREP '<font color=\"#FF9933\">' |";
    while( <FH> ){
        ($url, $title) = $_ =~ m|<font color="#FF9933">�D</font><a href="(.*?)"><font color="#003333">(.*?)</font></a><font color="#003333">|;
        $title =~ s/<.*?>//g;
        push @{$ra_titles}, [$url, $title];
    }
    close FH;
    return @{$ra_titles};
}

sub FormatChinese
{
    my($str, $length) = @_;
    my($i, $ret, $count, $s);
    return if( !$str );
    for( $i = 0 ; $i < length($str) ; ++$i ){
        if( ord(substr($str, $i, 1)) > 127 ){
            $ret .= substr($str, $i, 2);
            ++$i;
        }
        else{
            for( $count = 0, $s = $i ; $i < length($str) ; ++$i, ++$count ){
                last if( ord(substr($str, $i, 1)) > 127 );
            }
            --$i;
            $ret .= ' ' if( $count % 2 == 1);
            $ret .= substr($str, $s, $count);
        }
    }
    if( $length ){
        $str = $ret;
        undef $ret;
        while( $str ){
            $ret .= substr($str, 0, $length)."\n";
            $str = substr($str, $length);
        }
    }
    return $ret;
}

sub postout
{
    my($param) = @_;
    return if( !$param->{title} );
    open FH, ">/tmp/postout.$$";
    print FH $param->{content};
#print "$param->{content}";
    close FH;

    system("bin/post '$param->{brdname}' '$param->{title}' '$param->{owner}' /tmp/postout.$$");
    unlink "/tmp/postout.$$";
}
