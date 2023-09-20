#
# $Id: Makefile,v 1.57 2001/09/08 19:21:09 pumatst Exp $
# Makefile for the MCP
#

OPT = -O4

OS_TYPE		= Linux
include ../../../../Makefile-common

DEFINES		= -DSTATE_DEBUG -DASSERT -DBENCH -DENABLE_CRC32 -DDO_TIMEOUT_PROTOCOL
INCLUDES	= -I../include

REDHAT_DIST = $(shell $(CPLANT_TOP)/config/distribution.sh)

ifeq ($(REDHAT_DIST),7.x)
LANAI_VER = 6.x
else
LANAI_VER = $(REDHAT_DIST)
endif

ifeq ($(LINUX24),yes)
L24FLAG = -DLINUX24
else
L24FLAG =
endif

ifeq ($(CPLANT_ARCH),alpha)
LANAI_COMPILER = alpha-$(LANAI_VER)
else
LANAI_COMPILER = $(CPLANT_ARCH)
endif

LANAI_STUFF     = $(CPLANT_TOP)/support/lanai/$(LANAI_COMPILER)
LxSTRIP		= $(LANAI_STUFF)/bin/lanai-strip
LxCC		= $(LANAI_STUFF)/bin/lanai-gcc

ifeq ($(CPLANT_ARCH),i386) 
DEFINES += -DX86_BILD
endif

L3CFLAGS	= -DL4 -m4.1 $(OPT) -Wall -Wcast-align -nostdinc $(INCLUDES)
ifeq ($(CPLANT_ARCH),i386) 
L3CFLAGS       += $(DEFINES) -B$(LANAI_STUFF)/lib/gcc-lib/lanai/2.95.2..1.1/
else
L3CFLAGS       += $(DEFINES) -B$(LANAI_STUFF)/lib/gcc-lib/lanai/2.95.2..1.3/
endif
L3CFLAGS       += $(L24FLAG)
L3CFLAGS       += -B$(LANAI_STUFF)/lanai/bin/


L7CFLAGS	= -DL7 -m7.0 $(OPT) -Wall -Wcast-align -nostdinc $(INCLUDES)
ifeq ($(CPLANT_ARCH),i386) 
L7CFLAGS       += $(DEFINES) -B$(LANAI_STUFF)/lib/gcc-lib/lanai/2.95.2..1.1/
else
L7CFLAGS       += $(DEFINES) -B$(LANAI_STUFF)/lib/gcc-lib/lanai/2.95.2..1.3/
endif
L7CFLAGS       += $(L24FLAG)
L7CFLAGS       += -B$(LANAI_STUFF)/lanai/bin/

L9CFLAGS	= -DL9 -m9.0 $(OPT) -Wall -Wcast-align -nostdinc $(INCLUDES)
ifeq ($(CPLANT_ARCH),i386) 
L9CFLAGS       += $(DEFINES) -B$(LANAI_STUFF)/lib/gcc-lib/lanai/2.95.2..1.1/
else
L9CFLAGS       += $(DEFINES) -B$(LANAI_STUFF)/lib/gcc-lib/lanai/2.95.2..1.3/
endif
L9CFLAGS       += $(L24FLAG)
L9CFLAGS       += -B$(LANAI_STUFF)/lanai/bin/

PATH		= $(LANAI_STUFF)/bin:/bin:/usr/bin
INSTALLDIR	= $(mcpdir)
INSTALLTARGETS	= rtsmcp.4 rtsmcp.7 rtsmcp.9

EXIST_LANAI_STUFF	= $(shell $(CPLANT_TOP)/config/exist.sh $(LANAI_STUFF))

EXIST_RTS_MCP	= $(shell $(CPLANT_TOP)/config/exist.sh ./rtsmcp)

ifeq ($(EXIST_LANAI_STUFF),yes)
all:   rtsmcp.9 rtsmcp.7 rtsmcp.4
else
all:   bang_exist
endif

bang_exist:
	@echo "-------------------------------------------------------------"
	@echo "WARNING:"
	@echo " --in top/compute/OS/Myrinet/MCP"
	@echo " --make failing for target = all"
	@echo " --the directory $(LANAI_STUFF) doesn't exist"
ifeq ($(CPLANT_ARCH),alpha)
ifeq ($(EXIST_RTS_MCP),yes)
	@echo " --Found rtsmcp here, so you're probably OK!"
else
	@echo " --cannot build a Myrinet Control Program (MCP) on a DEC Alpha"
	@echo " --try building MCP on an x86 or a Sparc"
	@echo " --you may need to check out lanai tools w/ \"cvs co lanai\""
endif
else
	@echo " --probably need to check out lanai tools w/ \"cvs co lanai\""
endif
	@echo "-------------------------------------------------------------"

# ### -------------------------------------------------------------------- ### #
# ### build a LANai 4.x and a LANai 7.x MCP

rtsmcp.4:	crt0.4.o ebus.4.o init.4.o integrity.4.o RTS_main.4.o \
		Pkt_statem.4.o route.4.o eventlog.4.o
	$(LxCC) $(L3CFLAGS) -o $@ -nostdlib $^ -lgcc
	$(LxSTRIP) $@

rtsmcp.7:	crt0.7.o ebus.7.o init.7.o integrity.7.o RTS_main.7.o \
		Pkt_statem.7.o route.7.o eventlog.7.o
	$(LxCC) $(L7CFLAGS) -o $@ -nostdlib $^ -lgcc
	$(LxSTRIP) $@

rtsmcp.9:	crt0.9.o ebus.9.o init.9.o integrity.9.o RTS_main.9.o \
		Pkt_statem.9.o route.9.o eventlog.9.o
	$(LxCC) $(L9CFLAGS) -o $@ -nostdlib $^ -lgcc
	$(LxSTRIP) $@


# ### -------------------------------------------------------------------- ### #
# ### Rules to build a LANai 4.x MCP

%.4.lst:	%.4.c
	$(LxCC) $(L3CFLAGS) -c $^ -g -Wa,-adhnls > $@

%.4.lst:	%.c
	$(LxCC) $(L3CFLAGS) -c $^ -g -Wa,-adhnls > $@

%.4.o:	%.4.c
	$(LxCC) $(L3CFLAGS) -c -o $@ $^

%.4.o:	%.c
	$(LxCC) $(L3CFLAGS) -c -o $@ $^

%.4.o:	%.S
	$(LxCC) $(L3CFLAGS) -c $^ -o $@


# ### -------------------------------------------------------------------- ### #
# ### Rules to build a LANai 7.x MCP

%.7.lst:	%.7.c
	$(LxCC) $(L7CFLAGS) -c $^ -g -Wa,-adhnls > $@

%.7.lst:	%.c
	$(LxCC) $(L7CFLAGS) -c $^ -g -Wa,-adhnls > $@

%.7.o:	%.7.c
	$(LxCC) $(L7CFLAGS) -c -o $@ $^

%.7.o:	%.c
	$(LxCC) $(L7CFLAGS) -c -o $@ $^

%.7.o:	%.S
	$(LxCC) $(L7CFLAGS) -c $^ -o $@


# ### -------------------------------------------------------------------- ### #
# ### Rules to build a LANai 9.x MCP

%.9.lst:	%.7.c
	$(LxCC) $(L9CFLAGS) -c $^ -g -Wa,-adhnls > $@

%.9.lst:	%.c
	$(LxCC) $(L9CFLAGS) -c $^ -g -Wa,-adhnls > $@

%.9.o:	%.7.c
	$(LxCC) $(L9CFLAGS) -c -o $@ $^

%.9.o:	%.c
	$(LxCC) $(L9CFLAGS) -c -o $@ $^

%.9.o:	%.S
	$(LxCC) $(L9CFLAGS) -c $^ -o $@


# ### -------------------------------------------------------------------- ### #

.PHONY:	clean realclean install cp depend

clean:
	"rm" -f *.o *.lst

realclean:	clean
	"rm" -f rtsmcp.4 rtsmcp.7 rtsmcp.9

install:
	@$($@)

depend:
	@echo "No need for depend. Explicit dependencies in Makefile"

