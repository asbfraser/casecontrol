DEVICE  = attiny85
F_CPU   = 16000000	# in Hz
AVRDUDE = avrdude -c usbtiny -p $(DEVICE) # edit this line for your programmer

CFLAGS  = -Iusbdrv -I. -DDEBUG_LEVEL=0
OBJECTS = usbdrv/usbdrv.o usbdrv/usbdrvasm.o usbdrv/oddebug.o casecontrol.o

COMPILE = avr-gcc -Wall -Os -DF_CPU=$(F_CPU) $(CFLAGS) -mmcu=$(DEVICE)

# symbolic targets:
help:
	@echo "This Makefile has no default rule. Use one of the following:"
	@echo "make hex ....... to build casecontrol.hex"
	@echo "make flash ..... to flash the firmware (use this on metaboard)"
	@echo "make clean ..... to delete objects and hex file"

hex: casecontrol.hex

# rule for uploading firmware:
flash: casecontrol.hex
	$(AVRDUDE) -U flash:w:casecontrol.hex:i

# rule for deleting dependent files (those which can be built by Make):
clean:
	rm -f casecontrol.hex casecontrol.lst casecontrol.obj casecontrol.cof casecontrol.list casecontrol.map casecontrol.eep.hex casecontrol.elf *.o usbdrv/*.o casecontrol.s usbdrv/oddebug.s usbdrv/usbdrv.s

# Generic rule for compiling C files:
.c.o:
	$(COMPILE) -c $< -o $@

# Generic rule for assembling Assembler source files:
.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

# Generic rule for compiling C to assembler, used for debugging only.
.c.s:
	$(COMPILE) -S $< -o $@

# file targets:

casecontrol.elf: usbdrv $(OBJECTS)	# usbdrv dependency only needed because we copy it
	$(COMPILE) -o casecontrol.elf $(OBJECTS)

casecontrol.hex: casecontrol.elf
	rm -f casecontrol.hex casecontrol.eep.hex
	avr-objcopy -j .text -j .data -O ihex casecontrol.elf casecontrol.hex
	avr-size casecontrol.hex

