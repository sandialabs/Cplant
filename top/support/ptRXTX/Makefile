#
# $Id: Makefile,v 1.2 2001/05/09 21:03:42 pumatst Exp $
#

include ../../Makefile-common

CFLAGS   += -Wall -Wstrict-prototypes -g -O4 $(INCLUDE)
CSOURCES  = $(shell ls *.c)
COBJECTS  = $(CSOURCES:.c=.o)
EXECS     = $(CSOURCES:.c=)

INSTALLDIR     = $(sbindir)
INSTALLTARGETS = $(EXECS) setMACS.pl getNID.pl

all: $(EXECS) 

$(EXECS): % : %.o
	$(CC) $(CFLAGS) -o $@ $^

$(COBJECTS): %.o : %.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

install uninstall: $(INSTALLTARGETS)
	@$($@)

clean:
	rm -f $(EXECS) $(COBJECTS)
