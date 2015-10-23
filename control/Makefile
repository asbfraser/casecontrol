CONFDIR=${PREFIX}/etc/casecontrol
RUNDIR=${PREFIX}/var/run/casecontrol
BINDIR=${PREFIX}/usr/local/bin
INITDIR=${PREFIX}/etc/init.d
CC=gcc
CFLAGS=-Wall -lusb-1.0 -I include -DCASECONTROL_PREFIX=\"${CONFDIR}\" -DCASECONTROL_RUNDIR=\"${RUNDIR}\"
SOURCES=casecontrol.c

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
	@mkdir -p ${CONFDIR}/switch0.d
	@cp switch0.d/* ${CONFDIR}/switch0.d
	@mkdir -p ${CONFDIR}/led0.d
	@cp led0.d/* ${CONFDIR}/led0.d
	@mkdir -p ${CONFDIR}/led1.d
	@cp led1.d/* ${CONFDIR}/led1.d
	@cp -f casecontrol ${BINDIR}/
	@cp -f init.sh ${INITDIR}/casecontrol

.PHONY: remove
remove:
	@rm -rf ${CONFDIR}
	@rm -rf ${RUNDIR}
	@rm -f ${BINDIR}/casecontrol
	@rm -f ${INITDIR}/casecontrol
