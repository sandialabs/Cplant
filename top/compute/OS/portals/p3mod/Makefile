#
# $Id: Makefile,v 1.14 2001/12/07 23:34:49 pumatst Exp $
#

include ../../../../Makefile-common

CONF_FILE = ../../linux/include/linux/autoconf.h

CONFIG_SMP = $(shell $(CPLANT_TOP)/config/smp_check.pl $(CONF_FILE))

ifeq ($(CONFIG_SMP),yes)
CFLAGS += -D__SMP__
endif

CC = gcc

INCLUDEDIRS += -I$(CPLANT_TOP)/compute/OS

ifeq ($(CPLANT_ARCH),ia64)
CFLAGS  += -Wno-trigraphs -fomit-frame-pointer -fno-strict-aliasing \
           -fno-common -falign-functions=32 \
           -ffixed-r13 \
           -mfixed-range=f10-f15,f32-f127
endif

ifeq ($(CPLANT_ARCH),alpha)
CFLAGS     += -ffixed-8 -mno-fp-regs 
endif

ifeq ($(KERNEL_ADDR_CACHE),yes)
CFLAGS += -DKERNEL_ADDR_CACHE
endif

CFLAGS    += -Wstrict-prototypes -O2 -D__KERNEL__ -pipe \
-DMODULE $(DEBUG)

LD         = ld
LDFLAGS    = -r -s -dc
CSOURCES1  = $(shell ls *.c)
CSOURCES   = $(filter-out versions.c, $(CSOURCES1))
COBJECTS   = $(CSOURCES:.c=.o)
P30HSOURCES = $(shell ls ../../../lib/p30/include/*.h)
P30HSOURCES += $(shell ls ../../../lib/p30/include/p30/*.h)
P30SOURCES = $(shell ls ../../../lib/p30/lib-p30/*.c)
P30CSOURCES= $(notdir $(P30SOURCES))
P30OBJECTS = $(P30CSOURCES:.c=.o)
MODULE     = p3.mod
DEPS       = $(COBJECTS:.o=.d)

INSTALLDIR     = $(moduledir)
INSTALLTARGETS = $(MODULE)

.PRECIOUS: %.o

all:	$(MODULE)

ifeq ($(MAKE_DEPS),yes)
include $(DEPS)
endif

versions.o:	versions.c
	$(CC) -Wall -c $< -o $@

proc.c:		versions.h

versions.h:
	ln -s ../../Myrinet/rtscts/$@ $@

versions.c:	$(CSOURCES) *.h $(P30SOURCES) $(P30HSOURCES) \
		../../../../config/id_string.$(CPLANT_ARCH) versions.h
	grep -h "\$\I\d\: " $(CSOURCES) *.h $(P30SOURCES) $(P30HSOURCES) | \
	../../../../config/id_string.$(CPLANT_ARCH)

../../../../config/id_string.$(CPLANT_ARCH):	../../../../config/id_string.c
	$(CC) -Wall $< -o $@

# Print the version information to be encoded in the module
printv:
	grep -h "\$\I\d\: " $(CSOURCES) *.h $(P30SOURCES) $(P30HSOURCES) | \
	../../../../config/print_id_string.$(CPLANT_ARCH)

%.o:../../../lib/p30/lib-p30/%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

%.o:%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

$(MODULE): $(COBJECTS) $(P30OBJECTS) versions.o
	$(LD) $(LDFLAGS) -o $@ $^

install uninstall cp: $(INSTALLTARGETS)
	@$($@)

dep: $(DEPS)

clean:
	rm -f $(COBJECTS) $(P30OBJECTS) $(MODULE) $(DEPS) versions.[cho] \
	../../../../config/id_string.$(CPLANT_ARCH)

realclean: clean

