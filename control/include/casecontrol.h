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

#ifndef CASECONTROL_SCRIPT_DIR
#define CASECONTROL_SCRIPT_DIR "/etc/casecontrol/casecontrol.d"
#endif

#define SUCCESS	"\033[0;32mSUCCESS\033[0m"
#define FAILURE	"\033[0;31mFAILURE\033[0m"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <signal.h>
#include <dirent.h>
#include <sys/wait.h>

#include <libusb-1.0/libusb.h>

#endif
