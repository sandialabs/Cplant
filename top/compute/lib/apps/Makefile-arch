#
# $Id: Makefile-arch,v 1.1 2001/10/29 16:37:39 pumatst Exp $
#

include ../../../../Makefile-common

include Makefile-apps

LIBIO     = libgrp_io.a
LIBC      = libgrp_c.a
LIBUFOR   = libUfor.a
LIBENFS   = libenfs.a

ifeq ($(OS),osf)
ifeq ($(SYSTEM),REDHAT_6)
CFLAGS += -DREDHAT_6
endif
#LIBS      = $(LIBIO) $(LIBUFOR)
#LIBS      = $(LIBIO) $(LIBC) $(LIBUFOR) $(LIBENFS)
LIBS      = $(LIBIO) $(LIBUFOR) $(LIBENFS)
else
ifeq ($(LINUX_COMPILE),compaq)
LIBS      = $(LIBIO) $(LIBUFOR) $(LIBENFS)
else
LIBS      = $(LIBIO) $(LIBENFS)
endif
endif

LIBCDIR   = /usr/lib
LIBCORIG  = libc.a

ARDELETE  = d

DEPS      = $(LIBIOOBJECTS:.o=.d)

INSTALLDIR     = $(libdir)
INSTALLTARGETS = $(LIBS)

vpath %.c ..

all: $(LIBS)

ifeq ($(MAKE_DEPS),yes)
include $(DEPS)
endif

ifeq ($(OS),osf)
SYS_INTERFACE=ntv_sys.o
else
SYS_INTERFACE=
endif

$(LIBENFS): pf_io_fns.o $(SYS_INTERFACE)
	ar crl $(LIBENFS) pf_io_fns.o $(SYS_INTERFACE)

pf_io_fns.o: %.o: ../%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

ntv_sys.o: %.o: ../%.s
	$(CC) $(CFLAGS) $(INCLUDIRS) -c $<

$(LIBIOOBJECTS): %.o: ../%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

libgrp_io.a: libgrp_io.a($(LIBIOOBJECTS))

#$(LIBIO): $(LIBIO)($(LIBIOOBJECTS))

$(LIBC): $(LIBCDIR)/$(LIBCORIG)
	cp $(LIBCDIR)/$(LIBCORIG) $(LIBC) 
	$(AR) $(ARDELETE) $@ $(LIBCOBJECTS)

$(LIBUFOR): $(LIBCDIR)/$(LIBUFOR)
	cp $(LIBCDIR)/$(LIBUFOR) $(LIBUFOR)
	chmod +w $(LIBUFOR)
	$(AR) $(ARDELETE) $@ $(LIBUFOROBJECTS)

install uninstall: $(INSTALLTARGETS)
	@$($@)

dep: $(DEPS)

clean:
	-rm -f $(LIBS) $(LIBIOOBJECTS) $(DEPS) SYS_INTERFACE pf_io_fns.o

