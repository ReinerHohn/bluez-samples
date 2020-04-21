#!/bin/bash

#IP_CLIENT=192.168.178.60
#IP_CLIENT=10.14.32.14

IP_CLIENT=$1

BDADDR_SERVER=$(hciconfig -a | grep "BD Address" | awk ' { print $3 } ' )
IP_SERVER=$(ip a | grep eth0  | grep inet | awk ' { print $2 } '  | cut -d/ -f1)

#PROTOCOL=rfcomm
PROTOCOL=l2cap

LATENCY_TEST=1

PROG_CLIENT=/opt/hgs/client
PROG_SERVER=/opt/hgs/server

PACK_SIZE_END=8096

ssh-copy-id $IP_CLIENT

i=1
while [ $i -le $PACK_SIZE_END ];
do
	PACK_SIZE_SEND=$i
	PACK_SIZE_RECV=$i

	#ssh pi@$CLIENT_IP "./server  $PACK_SIZE_SEND $PACK_SIZE_RECV $PROTOCOL" &
	$PROG_SERVER  $PACK_SIZE_SEND $PACK_SIZE_RECV $PROTOCOL &
	sleep 2
	ssh pi@$IP_CLIENT "$PROG_CLIENT $BDADDR_SERVER $PACK_SIZE_SEND $PACK_SIZE_RECV $PROTOCOL $LATENCY_TEST"

	i=$(( $i * 2 ))
done
