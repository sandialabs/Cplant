#
# $Id: Makefile,v 1.13 2002/02/05 17:20:46 pumatst Exp $
# Makefile for the Myrinet library
#

include ../../../../Makefile-common

CC           = gcc

ifeq ($(CPLANT_ARCH),alpha) 
CFLAGS   = -Wall -O4 -mno-fp-regs 
DEFINES  = -Ddec_linux
endif

ifeq ($(CPLANT_ARCH),i386) 
CFLAGS   = -Wall -O4
DEFINES  = -Dintel_linux
endif

INCLUDES = -I../include -I../../linux/include
CFLAGS  += $(DEFINES) $(INCLUDES)

ifeq ($(CPLANT_ARCH),i386) 
all: libbfd.a libmyriFiles.a libLanaiDevice.a stat.o
else
all: libbfd.a libmyriFiles.a libLanaiDevice.a
endif

libbfd.a: libbfd.a.$(CPLANT_ARCH)
	ln -s libbfd.a.$(CPLANT_ARCH) libbfd.a

# this one has the bfd library references...
libmyriFiles.a: myriFiles.o
	ar cru $@ $^
	ranlib $@

libLanaiDevice.a: LinuxDevice.o myriPut.o
	ar cru $@ $^
	ranlib $@

%.o: %.c 
	$(CC) $(CFLAGS) -c $< -o $@


#-------------------------------------------------------------------------------
# Dependencies:
#
LinuxDevice.o:	../include/lanai_device.h
myriFiles.o:	../include/lanai_device.h
myriFiles.o:	../include/MyrinetPCI.h
myriFiles.o:	../include/bfd.h
myriPut.o:	../include/lanai_device.h
myriPut.o:	../include/MyrinetPCI.h
myriPut.o:	../util/arch_asm.h

#-------------------------------------------------------------------------------

clean:
	"rm" -f *.o *.lst *.s libLanaiDevice.a libmyriFiles.a libbfd.a 

install:
	@echo

%.lst:	%.c
	$(CC) $(CFLAGS) -c $*.c -g -Wa,-adhnls=$*.lst

%.s:	%.c
	$(CC) $(CFLAGS) -S $*.c -o $*.s

depend:
	@echo "No need for depend. Explicit dependencies in Makefile"
