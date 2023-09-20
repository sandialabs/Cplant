#
# $Id: Makefile,v 1.2 2001/03/28 00:04:53 pumatst Exp $
#

include ../../Makefile-common

CFLAGS   += -Wall -Wstrict-prototypes -g -O4 $(INCLUDE)
LIBDIRS   = -L$(CPLANT_TOP)/lib/puma/$(CPLANT_ARCH)-$(OS)
LIBS      = -lpuma
CSOURCES  = $(shell ls *.c)
COBJECTS  = $(CSOURCES:.c=.o)
EXECS     = $(CSOURCES:.c=)
DEPS      = $(CSOURCES:.c=.d)

INSTALLDIR     = $(sbindir)
INSTALLTARGETS = $(EXECS) cache.pl

ifeq ($(OS),linux) 
all: $(EXECS) cache.pl

ifeq ($(MAKE_DEPS),yes)
include $(DEPS)
endif

else
TARGETS = $(EXECS)

all:
	$(target-error)
endif

cache.pl: cache.pl.in
	@ $(CPLANT_TOP)/config/path_prefix.pl $(CPLANT_H) cache.pl.in cache.pl
	@ chmod ugo+x cache.pl

$(EXECS): % : %.o
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -o $@ $^ $(LIBDIRS) $(LIBS) 

$(COBJECTS): %.o : %.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

install uninstall: $(INSTALLTARGETS)
	@$($@)

dep: $(DEPS)

clean:
	rm -f $(EXECS) cache.pl $(COBJECTS) $(DEPS)

realclean: clean
	rm -f $(DEPS)

