#
# $Id: Makefile,v 1.1 1998/04/08 20:24:05 bright Exp $
#

LIB        = libgetopt.a
CSOURCES   = getopt.c getopt1.c 
COBJECTS   = $(CSOURCES:.c=.o)

CC         = cc
INCLUDEDIR = -I../../include
ARFLAGS    = crl

.PRECIOUS  = .o

all: $(LIB)

%.o: %.c
	$(CC) $(INCLUDEDIR) -c $<

$(LIB): $(COBJECTS) $(LIB)($(COBJECTS))

clean:
	rm -f $(LIB) $(COBJECTS)

realclean: clean
