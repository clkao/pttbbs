#!/bin/sh
# �Ъ`�N! �o���ɮױN�H root ���v������.
# �w�]�ϥ� bbs�o�ӱb��, �w�˥ؿ��� /home/bbs

case "$1" in
start)
	# ��l�� shared-memory, ���J�ϥΪ̱b��
	/usr/bin/su -fm bbs -c /home/bbs/bin/uhash_loader

	# �H�H�ܯ��~
	/usr/bin/su -fm bbs -c /home/bbs/bin/outmail &

	# ��H
	/usr/bin/su -fm bbs -c /home/bbs/innd/innbbsd &

	# �Ұ� port 23 (port 23���ϥ� root �~��i�� bind )
	/home/bbs/bin/mbbsd 23 &

	# �Ұ� utmpsortd
	/bin/sleep 5; "/usr/bin/su -fm bbs -c /home/bbs/bin/shmctl utmpsortd" &

	# ����
	echo -n ' mbbsd'
	;;
stop)
	/usr/bin/killall outmail
	/usr/bin/killall innbbsd
	/usr/bin/killall mbbsd
	/usr/bin/killall shmctl
	/bin/sleep 2; /usr/bin/killall shmctl
	;;
*)
	echo "Usage: `basename $0` {start|stop}" >&2
	;;
esac

exit 0
