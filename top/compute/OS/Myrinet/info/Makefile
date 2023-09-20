#
# $Id: Makefile,v 1.12.2.2 2002/04/04 19:26:46 jbogden Exp $
#

include ../../../../Makefile-common

CC	   = gcc

ifeq ($(OS),linux)
CFLAGS    += -O2 -Wall
endif

ifeq ($(OS),osf)
CFLAGS      += -D__alpha__ -O2
endif

ifeq ($(RTSCTS_ERROR_CORRECTION),yes)
DEFINES += -DDO_TIMEOUT_PROTOCOL
endif

ifeq ($(EXTENDED_P3_RTSCTS),yes)
DEFINES += -DEXTENDED_P3_RTSCTS
endif

# --- The following 5 includes are needed for config.h list parse routines
INCLUDES  += -I../../../../include
INCLUDES  += -I../../../../include/portals
INCLUDES  += -I../../../../include/sys
INCLUDES  += -I../../../../include/load

INCLUDES  += -I../../../lib/p30/include
INCLUDES  += -I../../linux/include
INCLUDES  += -I../rtscts
INCLUDES  += -I../MCP
INCLUDES  += -I../util
CFLAGS    += $(INCLUDES) $(DEFINES)

EXECS = infoping infop3 infomcp infoerr infoprotocol infoevent example \
	    getevent getpackets getp3 p3_ping msgid
INSTEXECS = infoping infop3 infomcp infoerr infoprotocol infoevent p3_ping msgid

INSTALLDIR        = $(sbindir)
INSTALLTARGETS    = $(INSTEXECS)

all: $(EXECS)

example:	example.c library.o timing.o
	$(CC) $(CFLAGS) -o $@ $^

infoping:	infoping.o library.o nodeList.o timing.o
	$(CC) -o $@ $^

nodeList.o:	../../../../lib/puma/puma/nodeList.c
	$(CC) $(CFLAGS) -c -o $@ $^

timing.o:	../util/timing.c
	$(CC) $(CFLAGS) -c -o $@ $^

infoprotocol:	infoprotocol.o library.o nodeList.o timing.o
	$(CC) -o $@ $^

infop3:		infop3.o library.o nodeList.o timing.o
	$(CC) -o $@ $^

infomcp:	infomcp.o library.o nodeList.o timing.o
	$(CC) -o $@ $^

infoerr:	infoerr.o library.o nodeList.o timing.o
	$(CC) -o $@ $^

infoevent:	infoevent.o library.o nodeList.o timing.o
	$(CC) -o $@ $^

getevent:	getevent.o library.o nodeList.o timing.o
	$(CC) -o $@ $^

getpackets:	getpackets.o library.o nodeList.o timing.o
	$(CC) -o $@ $^

getp3:	getp3.o library.o nodeList.o timing.o
	$(CC) -o $@ $^

p3_ping: p3_ping.o library.o nodeList.o timing.o
	$(CC) -o $@ $^

msgid: msgid.o
	$(CC) -o $@ $^

install uninstall cp: $(INSTALLTARGETS)
	@$($@)

clean:
	"rm" -f $(EXECS) *.o
