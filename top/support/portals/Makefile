#
# $Id: Makefile,v 1.12 2000/10/13 19:11:00 pumatst Exp $
#

include ../../Makefile-common

CC = gcc

CFLAGS   += -Wall -Wstrict-prototypes -g -O4 $(INCLUDE)
LIBDIRS   = -L$(CPLANT_TOP)/lib/puma/$(CPLANT_ARCH)-$(OS)
LIBS      = -lpuma
CSOURCES  = $(shell ls *.c)
COBJECTS  = $(CSOURCES:.c=.o)
EXECS     = $(CSOURCES:.c=)
DEPS      = $(CSOURCES:.c=.d)

INSTALLDIR     = $(sbindir)
INSTALLTARGETS = $(EXECS)

ifeq ($(OS),linux) 
all: $(EXECS) 

ifeq ($(MAKE_DEPS),yes)
include $(DEPS)
endif

else
TARGETS = $(EXECS)

all:
	$(target-error)
endif

$(EXECS): % : %.o
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -o $@ $^ $(LIBDIRS) $(LIBS) 

$(COBJECTS): %.o : %.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

install uninstall: $(INSTALLTARGETS)
	@$($@)

dep: $(DEPS)

clean:
	rm -f $(EXECS) $(COBJECTS) $(DEPS)

realclean: clean
	rm -f $(DEPS)

