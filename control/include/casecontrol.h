#ifndef CASECONTROL_H
#define CASECONTROL_H

#define _BSD_SOURCE

#define CASECONTROL_VID		0x1781
#define CASECONTROL_PID		0x1111
#define CASECONTROL_REQUESTTYPE	(1 << 7) | (1 << 6)
#define CASECONTROL_RQ_GET	0
#define CASECONTROL_RQ_TEST	1
#define CASECONTROL_RQ_LED0	2
#define CASECONTROL_RQ_LED1	3

#define CASECONTROL_INT_TIMEOUT	1000

#ifndef CASECONTROL_PREFIX
#define CASECONTROL_PREFIX	"/etc/casecontrol"
#endif

#ifndef CASECONTROL_RUNDIR
#define CASECONTROL_RUNDIR	"/var/run/casecontrol"
#endif

#ifndef CASECONTROL_PID_FILE
#define CASECONTROL_PID_FILE	CASECONTROL_RUNDIR "/pid"
#endif

#ifndef CASECONTROL_SWITCH0_SCRIPT_DIR
#define CASECONTROL_SWITCH0_SCRIPT_DIR CASECONTROL_PREFIX "/switch0.d"
#endif

#ifndef CASECONTROL_LED0_SCRIPT_DIR
#define CASECONTROL_LED0_SCRIPT_DIR CASECONTROL_PREFIX "/led0.d"
#endif

#ifndef CASECONTROL_LED1_SCRIPT_DIR
#define CASECONTROL_LED1_SCRIPT_DIR CASECONTROL_PREFIX "/led1.d"
#endif

#ifndef CASECONTROL_CONNECTED_SCRIPT_DIR
#define CASECONTROL_CONNECTED_SCRIPT_DIR CASECONTROL_PREFIX "/connected.d"
#endif

#define CASECONTROL_LOG_IDENT	"CASECONTROL"

#define SUCCESS	"\033[0;32mSUCCESS\033[0m"
#define FAILURE	"\033[0;31mFAILURE\033[0m"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>

#include <signal.h>
#include <dirent.h>
#include <sys/wait.h>
#include <syslog.h>

#include <libusb-1.0/libusb.h>

#endif
