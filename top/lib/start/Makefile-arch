#
# $Id: Makefile-arch,v 1.1 2001/10/29 16:41:29 pumatst Exp $
#

include ../../../Makefile-common

CSOURCES       = $(shell ls ../*.c)

COBJECTS       = $(notdir $(CSOURCES:.c=.o))

DEPS           = $(COBJECTS:.o=.d)

INSTALLTARGETS = $(COBJECTS) 
INSTALLDIR     = $(libdir)

INCLUDEDIRS += -I$(CPLANT_TOP)/compute/lib/apps

vpath %.c ..

all: $(COBJECTS) startup_srvr.o

ifeq ($(MAKE_DEPS),yes)
include $(DEPS)
endif

$(COBJECTS): %.o : ../%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

startup_srvr.o: ../startup.c
	$(CC) $(CFLAGS) -DSRVR_STARTUP $(INCLUDEDIRS) -o startup_srvr.o -c ../startup.c

install uninstall: $(INSTALLTARGETS)
	@$($@)

dep: $(DEPS)

clean:
	-rm -f $(COBJECTS) startup_srvr.o $(DEPS)

realclean: clean
	-rm -f $(DEPS)

