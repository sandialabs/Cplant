#
# $Id: Makefile,v 1.13 2001/12/07 23:29:32 pumatst Exp $
#

include ../../../Makefile-common

CC = gcc

CONF_FILE = ../linux/include/linux/autoconf.h

CONFIG_SMP = $(shell $(CPLANT_TOP)/config/smp_check.pl $(CONF_FILE))

ifeq ($(CONFIG_SMP),yes)
CFLAGS += -D__SMP__
endif

ifeq ($(CPLANT_ARCH),ia64)
CFLAGS  += -Wno-trigraphs -fomit-frame-pointer -fno-strict-aliasing \
           -fno-common -falign-functions=32 \
           -ffixed-r13 \
           -mfixed-range=f10-f15,f32-f127
endif

ifeq ($(CPLANT_ARCH),alpha)
CFLAGS     += -ffixed-8 -mno-fp-regs 
endif


CFLAGS    += -Wstrict-prototypes -O2 -D__KERNEL__ -pipe \
-DMODULE

LD         = ld
LDFLAGS    = -r -s -dc
CSOURCES   = $(shell ls *.c)
COBJECTS   = $(CSOURCES:.c=.o)
MODULE     = cTask.mod
DEPS       = $(COBJECTS:.o=.d)

INSTALLDIR     = $(moduledir)
INSTALLTARGETS = $(MODULE)

.PRECIOUS: %.o

all: $(MODULE)

ifeq ($(MAKE_DEPS),yes)
include $(DEPS)
endif

%.o:%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

$(MODULE): $(COBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

install uninstall cp: $(INSTALLTARGETS)
	@$($@)

dep: $(DEPS)

clean:
	rm -f $(COBJECTS) $(MODULE) $(DEPS)

realclean: clean

