#
# $Id: Makefile,v 1.10.4.1 2002/08/30 20:25:44 jbogden Exp $
#
include ../../../Makefile-common

CONF_FILE = ../linux/include/linux/autoconf.h

CONFIG_SMP = $(shell $(CPLANT_TOP)/config/smp_check.pl $(CONF_FILE))

ifeq ($(CONFIG_SMP),yes)
CFLAGS += -D__SMP__
endif

INCLUDES  = -I/usr/src/include -I../../OS/linux/include 
#INCLUDES += -I../../OS/linux/include/linux/modversions.h 

CFLAGS += -D__KERNEL__ -DLINUX -Wstrict-prototypes -O2  
CFLAGS += -fomit-frame-pointer -DMODULE -DMODVERSIONS

ifeq ($(CPLANT_ARCH),ia64)
CFLAGS  += -Wno-trigraphs -fno-strict-aliasing \
           -fno-common -falign-functions=32 \
           -ffixed-r13 \
           -mfixed-range=f10-f15,f32-f127
endif

ifeq ($(CPLANT_ARCH),alpha)
CFLAGS += -ffixed-8 -mno-fp-regs 
endif

INSTALLDIR     = $(moduledir)
INSTALLTARGETS = ptRXTX.mod

all: ptRXTX.o

ptRXTX.o ptRXTX.mod: ptRXTX.c ptRXTX.h
	gcc $(CFLAGS) $(INCLUDES) -c -o ptRXTX.o ptRXTX.c
	@cp ptRXTX.o ptRXTX.mod

install uninstall cp: $(INSTALLTARGETS)
	@$($@)

clean: 
	rm -f ptRXTX.o ptRXTX.mod
