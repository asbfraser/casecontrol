CC=gcc
CFLAGS=-Wall -lusb-1.0 -I include -DCASECONTROL_SWITCH0_SCRIPT_DIR=\"switch0.d\" -DCASECONTROL_LED0_SCRIPT_DIR=\"led0.d\" -DCASECONTROL_LED1_SCRIPT_DIR=\"led1.d\"
SOURCES=casecontrol.c

casecontrol: $(SOURCES)
	$(CC) -o casecontrol $(SOURCES) $(CFLAGS)

.PHONY: clean
clean:
	rm -f casecontrol
