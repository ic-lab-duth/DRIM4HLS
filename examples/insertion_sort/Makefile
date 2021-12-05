
ifndef V
        QUIET_CC            = @echo '   ' CC $@;
        QUIET_LINK          = @echo '   ' LINK $@;
        QUIET_MKDIR         = @echo '   ' MKDIR $@;
        QUIET_RUN           = @echo '   ' RUN $@;
        QUIET_MAKE          = @echo '   ' MAKE $@;
        QUIET_OBJDUMP       = @echo '   ' OBJDUMP $@;
        QUIET_OBJCOPY       = @echo '   ' OBJCOPY $@;
        QUIET_CLEAN         = @echo '   ' CLEAN $@;
endif
RM = rm -rf

CROSS_COMPILE ?= riscv64-unknown-elf-
CC = $(CROSS_COMPILE)gcc
OBJDUMP = $(CROSS_COMPILE)objdump
OBJCOPY = $(CROSS_COMPILE)objcopy

CFLAGS ?=
CFLAGS += -O3 -march=rv32im -mabi=ilp32

SRCS = $(wildcard *.c)
TARGETS = $(SRCS:.c=.elf)
LSTS = $(SRCS:.c=.lst)
SREC = $(SRCS:.c=.srec)
TEXT = $(SRCS:.c=.txt)

all: $(TARGETS) $(SREC) $(LSTS) $(TEXT)


.SUFFIX: .c .lst .srec .txt .elf

$(LSTS): %.lst : %.elf
	$(QUIET_OBJDUMP) $(OBJDUMP) -D $< > $@

$(SREC): %.srec : %.elf
	$(QUIET_OBJCOPY) $(OBJCOPY) -O srec --gap-fill 0 $< $@

$(TEXT): %.txt : %.srec srec2text.py
	$(QUIET_OBJCOPY) ./srec2text.py $< > $@

$(TARGETS): %.elf : %.c
	$(QUIET_CC) $(CC) $(CFLAGS) -T lscript bootstrap.s $< -o $@ -nostdlib

clean:
	$(QUIET_CLEAN) $(RM) $(OBJS) *.lst *.srec *.txt

distclean: clean
	$(QUIET_CLEAN) $(RM) $(TARGETS)

.PHONY: all clean distclean
