#!/bin/sh

bin/bbsctl permreport > etc/permrpt.log
if [ -s etc/permrpt.log ] ; then 
    bin/post  Administor "�S���v���ϥΪ̦W�� `date +'%Y%m%d'`"  "[�v�����i]"   etc/permrpt.log
fi
/bin/rm -f etc/permrpt.log
