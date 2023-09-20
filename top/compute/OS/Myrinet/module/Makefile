#
# $Id: Makefile,v 1.33 2001/12/07 23:36:49 pumatst Exp $
# Makefile for the Myrinet/Portals IP module
#

include ../../../../Makefile-common

CONF_FILE = ../../linux/include/linux/autoconf.h

CONFIG_SMP = $(shell $(CPLANT_TOP)/config/smp_check.pl $(CONF_FILE))

ifeq ($(CONFIG_SMP),yes)
CFLAGS += -D__SMP__
endif

CC      = gcc

ifeq ($(CPLANT_ARCH),ia64)
CFLAGS  += -Wno-trigraphs -fomit-frame-pointer -fno-strict-aliasing \
           -fno-common -falign-functions=32 \
           -ffixed-r13 \
           -mfixed-range=f10-f15,f32-f127
endif

ifeq ($(CPLANT_ARCH),alpha)
CFLAGS += -ffixed-8 -mno-fp-regs
DEFINES = -Ddec_linux
endif

ifeq ($(CPLANT_ARCH),i386)
DEFINES = -Dintel_linux
endif

CFLAGS += -O2 -pipe

INCLUDES	= -I../../../../include -I../../../../include/portals -I../MCP
INCLUDES       += -I../../linux/include -I../../../../include/sys
DEFINES        += -D_KERNEL_ -D__KERNEL__ -DMODULE -DUSE_PROC_FS -DLINUX_PORTALS

WFLAGS          = -Wstrict-prototypes
CFLAGS         += $(WFLAGS) $(DEFINES) $(INCLUDES) $(INCLUDEDIRS)
CSOURCES1  = $(shell ls *.c)
CSOURCES   = $(filter-out versions.c, $(CSOURCES1))

all:	myrIP.mod pnid2ip

myrIP.mod: IP_init.o IP_module.o IP_send.o IP_recv.o \
           IP_proc.o addr_convert.o versions.o
	ld -r -s -dc -o $@ $^

pnid2ip: pnid2ip.c
	$(CC) -g $(INCLUDES) $^ -o $@

%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@


IP_proc.c:	versions.h

versions.o:	versions.c
	$(CC) -Wall -c $< -o $@

versions.h:
	ln -s ../rtscts/$@ $@

versions.c:	$(CSOURCES) *.h ../../../../config/id_string.$(CPLANT_ARCH) \
		versions.h
	grep -h "\$\I\d\: " $(CSOURCES) *.h | ../../../../config/id_string.$(CPLANT_ARCH)

../../../../config/id_string.$(CPLANT_ARCH):	../../../../config/id_string.c
	$(CC) -Wall $< -o $@

# Print the version information to be encoded in the module
printv:
	grep -h "\$\I\d\: " $(CSOURCES) *.h | ../../../../config/print_id_string.$(CPLANT_ARCH)

#-------------------------------------------------------------------------------
# Dependencies
#

#-------------------------------------------------------------------------------

.PHONY: clean realclean install cp depend

clean:
	"rm" -f *.o *.lst

realclean:	clean
	"rm" -f myrIP.mod pnid2ip versions.[cho]

install cp:
	install myrIP.mod $(moduledir)
	install pnid2ip $(sbindir)

%.lst:	%.c
	$(CC) $(CFLAGS) -c $*.c -g -Wa,-adhnls=$*.lst

%.s:	%.c
	$(CC) $(CFLAGS) -S $*.c -o $*.s

depend:
	@echo "No need for depend. Explicit dependencies in Makefile"
