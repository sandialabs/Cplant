#
# $Id: Makefile-arch,v 1.1 2001/10/29 16:40:53 pumatst Exp $
#

include ../../../Makefile-common

LIB         = libsrvr.a
CSOURCES    = $(shell ls ../*.c)
COBJECTS    = $(notdir $(CSOURCES:.c=.o))
DEPS        = $(COBJECTS:.o=.d)

INSTALLTARGETS = $(LIB)
INSTALLDIR     = $(libdir)

CFLAGS += -g

vpath %.c ..

all: $(LIB)

ifeq ($(MAKE_DEPS),yes)
include $(DEPS)
endif

$(COBJECTS): %.o : ../%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

$(LIB): $(LIB)($(COBJECTS))

install uninstall: $(INSTALLTARGETS)
	@$($@)

dep: $(DEPS)

clean:
	-rm -f $(COBJECTS) $(LIB) $(DEPS)

