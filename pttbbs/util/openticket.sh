#!/bin/sh
# $Id: openticket.sh,v 1.1 2002/03/07 15:13:46 in2 Exp $

bin/openticket > etc/jackpot
bin/post Record "�m�餤���W��" "[������i]" etc/jackpot
bin/post Record "�q�Ʀr�����W��" "[�q�Ʀr���i]" etc/winguess.log
bin/post Record "�¥մѹ�԰O��" "[�q�Ʀr���i]" etc/othello.log
rm -f etc/winguess.log
rm -f etc/loseguess.log
rm -f etc/othello.log
