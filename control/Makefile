CC=gcc
CFLAGS=-Wall -lusb-1.0 -I include -DCASECONTROL_SCRIPT_DIR=\"scripts\"
SOURCES=casecontrol.c

casecontrol: $(SOURCES)
	$(CC) -o casecontrol $(SOURCES) $(CFLAGS)

.PHONY: clean
clean:
	rm -f casecontrol
