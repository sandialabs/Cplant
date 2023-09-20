#
# $Id: Makefile,v 1.32 2002/03/05 23:53:14 jbogden Exp $
#

include ../../../../Makefile-common

CC	   = gcc

ifeq ($(OS),linux)
CFLAGS    += -O2 -g -Wall
endif

ifeq ($(OS),osf)
CFLAGS      += -DLINUX_PORTALS -D__alpha__ -O2
endif

INCLUDES  += -I../../../../include -I../../linux/include -I../include -I..
INCLUDES  += -I../MCP -I../../../../include/sys -I../../../../include/portals
INCLUDES  += -I../util
INCLUDES  += -I../rtscts
CFLAGS    += $(INCLUDES) 

EXECS = aroute crawl eping do-ping get-ping getroute mping req rroute troute vping vroute vrouted all2.pl lmem.pl mach2.pl one2all.pl su.pl cplant_lan_check
HTML = vroute.html vrouted.html

INSTALLDIR        = $(sbindir)
INSTALLTARGETS    = $(EXECS)

all: $(EXECS)

html: $(HTML)

all2.pl: all2.pl.in
	@ $(CPLANT_TOP)/config/path_prefix.pl $(CPLANT_H) all2.pl.in all2.pl
	@ chmod ugo+x mping

mach2.pl: mach2.pl.in
	@ $(CPLANT_TOP)/config/path_prefix.pl $(CPLANT_H) mach2.pl.in mach2.pl
	@ chmod ugo+x mach2.pl

one2all.pl: one2all.pl.in
	@ $(CPLANT_TOP)/config/path_prefix.pl $(CPLANT_H) one2all.pl.in one2all.pl
	@ chmod ugo+x one2all.pl

lmem.pl: lmem.pl.in
	@ $(CPLANT_TOP)/config/path_prefix.pl $(CPLANT_H) lmem.pl.in lmem.pl
	@ chmod ugo+x lmem.pl

su.pl: su.pl.in
	@ $(CPLANT_TOP)/config/path_prefix.pl $(CPLANT_H) su.pl.in su.pl
	@ chmod ugo+x su.pl

mping: mping.pl
	@ $(CPLANT_TOP)/config/path_prefix.pl $(CPLANT_H) mping.pl mping
	@ chmod ugo+x mping

crawl: crawl.pl
	@ $(CPLANT_TOP)/config/path_prefix.pl $(CPLANT_H) crawl.pl crawl
	@ chmod ugo+x crawl

vrouted.o: vrouted.c vroute.h
	$(CC) $(CFLAGS) -c $< -o $@

vrouted: vrouted.o
	$(CC) $(CFLAGS) -o $@ $^

sock_init.o: sock_init.c
	$(CC) $(CFLAGS) -c $< -o $@

vroute.o: vroute.c sock_init.c vroute.h
	$(CC) $(CFLAGS) -c $< -o $@

vroute: vroute.o sock_init.o
	$(CC) $(CFLAGS) -o $@ $^

vroute.html: vroute.1
	rman -f html vroute.1 > vroute.html

vrouted.html: vrouted.8
	rman -f html vrouted.8 > vrouted.html

req: req.o ../util/common.o ../lib/libLanaiDevice.a
	$(CC) $(CFLAGS) -o $@ $^  -L../lib -lLanaiDevice -lm

req.o: req.c vping.h
	$(CC) $(CFLAGS) -c $< -o $@

rroute: rroute.o ../util/common.o ../lib/libLanaiDevice.a
	$(CC) $(CFLAGS) -o $@ $^  -L../lib -lLanaiDevice -lm

rroute.o: rroute.c vping.h
	$(CC) $(CFLAGS) -c $< -o $@

troute: troute.o ../util/common.o ../lib/libLanaiDevice.a
	$(CC) $(CFLAGS) -o $@ $^  -L../lib -lLanaiDevice -lm

troute.o: troute.c vping.h
	$(CC) $(CFLAGS) -c $< -o $@

cplant_lan_check: cplant_lan_check.o ../util/common.o ../lib/libLanaiDevice.a
	$(CC) $(CFLAGS) -o $@ $^  -L../lib -lLanaiDevice -lm

cplant_lan_check.o: cplant_lan_check.c vping.h
	$(CC) $(CFLAGS) -c $< -o $@

aroute: aroute.o ../util/common.o ../lib/libLanaiDevice.a
	$(CC) $(CFLAGS) -o $@ $^  -L../lib -lLanaiDevice -lm

aroute.o: aroute.c vping.h
	$(CC) $(CFLAGS) -c $< -o $@

vping: vping.o ../util/common.o ../lib/libLanaiDevice.a
	$(CC) $(CFLAGS) -o $@ $^  -L../lib -lLanaiDevice -lm

vping.o: vping.c vping.h
	$(CC) $(CFLAGS) -c $< -o $@

eping: eping.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

eping.o: eping.c eping.h
	$(CC) $(CFLAGS) -c $< -o $@

../util/common.o: ../util/common.c
	$(CC) $(CFLAGS) -c $< -o $@

install uninstall cp: $(INSTALLTARGETS)
	@$($@)

clean:
	"rm" -f $(EXECS) $(HTML) *.o
