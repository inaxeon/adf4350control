##############################################################################
# Title        : AVR Makefile for Windows
#
# Created      : Matthew Millman 2018-05-29
#                http://tech.mattmillman.com/
#
##############################################################################

# Fixes clash between windows and coreutils mkdir. Comment out the below line to compile on Linux
COREUTILS  = C:/Dev/compilers/coreutils/bin/

DEVICE              = attiny1624
AVRDUDEDEV          = t1624
CLOCK               = 20000000

PROGRAMMING_MODE    = local
LOCALPORT           = COM4
PGMPREP             = C:\Dev\rfavrprogramprep\RfAvrProgramPrep\bin\Debug\RfAvrProgramPrep.exe
BL_LDFLAGS          = -Wl,--section-start=.text=0x200

ifeq ($(PROGRAMMING_MODE),local)
AVRDUDE_ARGS = -u -c arduino -D -P $(LOCALPORT) -b 115200 -p $(AVRDUDEDEV)
AVRDUDE = $(PGMPREP) -p $(LOCALPORT) -o Local && avrdude $(AVRDUDE_ARGS)
#AVRDUDE = avrdude $(AVRDUDE_ARGS)
AVRDUDE_POSTCMD =
endif

ifeq ($(PROGRAMMING_MODE),noboot)
AVRDUDE_ARGS = -u -c atmelice_updi -p $(AVRDUDEDEV)
AVRDUDE = avrdude $(AVRDUDE_ARGS)
AVRDUDE_POSTCMD =
FUSES      = -U fuse0:w:0x00:m -U fuse1:w:0x00:m -U fuse2:w:0x02:m -U fuse5:w:0xC4:m -U fuse6:w:0x06:m -U fuse7:w:0x00:m -U fuse8:w:0x00:m
endif

SRCS       = main.c cmd.c config.c util.c usart_buffered.c timer.c adf4350.c
OBJS       = $(SRCS:.c=.o)
DEPDIR     = deps
DEPFLAGS   = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td
RM         = rm
MV         = mv
MKDIR      = $(COREUTILS)mkdir

POSTCOMPILE = $(MV) $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

COMPILE = avr-gcc -Wall -Os $(DEPFLAGS) -DF_CPU=$(CLOCK) -mmcu=$(DEVICE)

all:	main.hex

.c.o:
	@$(MKDIR) -p $(DEPDIR)
	$(COMPILE) -c $< -o $@
	@$(POSTCOMPILE)

.S.o:
	@$(MKDIR) -p $(DEPDIR)
	$(COMPILE) -x assembler-with-cpp -c $< -o $@
	@$(POSTCOMPILE)

.c.s:
	@$(MKDIR) -p $(DEPDIR)
	$(COMPILE) -S $< -o $@
	@$(POSTCOMPILE)

flash: all
	$(AVRDUDE) -U flash:w:main.hex $(AVRDUDE_POSTCMD)

fuse:
	$(AVRDUDE) $(FUSES)

install: flash

clean:
	$(RM) -f main.hex main.elf $(OBJS) dev.h

main.elf: $(OBJS)
	$(COMPILE) -o main.elf $(OBJS) -Wl,-Map,"main.map" $(BL_LDFLAGS)

main.hex: main.elf
	avr-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature main.elf main.hex

disasm:	main.elf
	avr-objdump -d main.elf

cpp:
	$(COMPILE) -E $(SRCS)

$(DEPDIR)/%.d:
.PRECIOUS: $(DEPDIR)/%.d

include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS))))