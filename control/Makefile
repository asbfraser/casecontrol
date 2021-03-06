CONFDIR=${PREFIX}/etc/casecontrol
RUNDIR=${PREFIX}/var/run/casecontrol
BINDIR=${PREFIX}/usr/local/bin
INITDIR=${PREFIX}/etc/init.d
LOGWATCHDIR=${PREFIX}/etc/logwatch
CC=gcc
CFLAGS=-Wall -lusb-1.0 -I include -DCASECONTROL_PREFIX=\"${CONFDIR}\" -DCASECONTROL_RUNDIR=\"${RUNDIR}\"
SOURCES=casecontrol.c

all: casecontrol

debug: CFLAGS += -DDEBUG -ggdb
debug: casecontrol

casecontrol: $(SOURCES)
	$(CC) -o casecontrol $(SOURCES) $(CFLAGS)

.PHONY: clean
clean:
	rm -f casecontrol

.PHONY: install
install: casecontrol
	@mkdir -p ${CONFDIR}
	@mkdir -p ${RUNDIR}
	@mkdir -p ${BINDIR}
	@mkdir -p ${INITDIR}
	@mkdir -p ${CONFDIR}/connected.d
	@cp connected.d/* ${CONFDIR}/connected.d 2>/dev/null || true
	@mkdir -p ${CONFDIR}/switch0.d
	@cp switch0.d/* ${CONFDIR}/switch0.d 2>/dev/null || true
	@mkdir -p ${CONFDIR}/led0.d
	@cp led0.d/* ${CONFDIR}/led0.d 2>/dev/null || true
	@mkdir -p ${CONFDIR}/led1.d
	@cp led1.d/* ${CONFDIR}/led1.d 2>/dev/null || true
	@cp -f casecontrol ${BINDIR}/
	@cp -f init.sh ${INITDIR}/casecontrol
	@cp logwatch/conf/services/casecontrol.conf ${LOGWATCHDIR}/conf/services/ 2>/dev/null || true
	@cp logwatch/scripts/services/casecontrol ${LOGWATCHDIR}/scripts/services/ 2>/dev/null || true

.PHONY: remove
remove:
	@rm -rf ${CONFDIR}
	@rm -rf ${RUNDIR}
	@rm -f ${BINDIR}/casecontrol
	@rm -f ${INITDIR}/casecontrol
	@rm -f ${LOGWATCHDIR}/conf/services/casecontrol.conf 2>/dev/null || true
	@rm -f ${LOGWATCHDIR}/scripts/services/casecontrol 2>/dev/null || true
