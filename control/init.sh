#!/bin/sh
# Linux Standard Base comments
### BEGIN INIT INFO
# Provides:          casecontrol
# Required-Start:    $local_fs $network $remote_fs
# Required-Stop:     $local_fs $network $remote_fs
# Should-Start:
# Should-Stop:
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Casecontrol
# Description:       Casecontrol
### END INIT INFO

#############################################################
# Init script for Casecontrol
#############################################################

# Defaults
PREFIX=/home/afraser/devel/casecontrol/control/test
RUNDIR=${PREFIX}/var/run/casecontrol
BINDIR=${PREFIX}/usr/local/bin
BINNAME=casecontrol

if [ -f "${RUNDIR}/pid" ];
then
	PID=`cat ${RUNDIR}/pid`
	PROCS=`ps -p ${PID} -h | grep ${BINNAME}`
fi

case "$1" in
start)
	if [ ! -z "${PROCS}" ];
	then
		echo "casecontrol is already running!"
		exit 1
	else
		${BINDIR}/${BINNAME} || echo "casecontrol failed to start"
		echo "casecontrol started!"
	fi
	;;
stop)
	if [ -z "${PROCS}" ];
	then
		echo "casecontrol is not running"
		exit 1
	else
		echo "Stopping casecontrol..."
		/bin/kill -s SIGINT ${PID} || echo "Error stopping casecontrol"
	fi
	;;
restart)
	./$0 stop
	./$0 start
	;;
force-reload)
	kill -9 ${PID}
	killall -9 ${BINNAME}
	rm -f ${RUNDIR}/pid
	./$0 start
	;;
status)
	if [ -z "${PROCS}" ];
	then
		echo "casecontrol is not running"
		exit 0
	fi

	echo "casecontrol is running (PID: ${PID})"

	(
		flock -n 9 || exit 1

		rm -f ${RUNDIR}/status
		/bin/kill -s SIGHUP ${PID}
		if [ "$?" -ne 0 ];
		then
			echo "Error getting status (1)"
			exit 1
		fi

		sleep 2

		if [ ! -e ${RUNDIR}/status ];
		then
			echo "Error getting status (2)"
			exit 1
		fi

		cat ${RUNDIR}/status
		rm -f ${RUNDIR}/status

       ) 9>${RUNDIR}/status.lock
	rm ${RUNDIR}/status.lock

	;;
led0)
	if [ -z "${PROCS}" ];
	then
		echo "casecontrol is not running"
		exit 1
	else
		/bin/kill -s SIGUSR1 ${PID} || echo "Error toggling LED0"
	fi
	;;
led1)
	if [ -z "${PROCS}" ];
	then
		echo "casecontrol is not running"
		exit 1
	else
		/bin/kill -s SIGUSR2 ${PID} || echo "Error toggling LED1"
	fi
	;;
*) 	
	echo "Usage: $0 <start|stop|restart|force-reload|status>" >&2
	exit 3
	;;
esac
exit 0
