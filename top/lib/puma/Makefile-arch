#
# $Id: Makefile-arch,v 1.1 2001/10/29 16:39:27 pumatst Exp $
#

include ../../../Makefile-common

LIB         = libpuma.a

TOPSOURCES  = $(shell ls ../*.c)
CLIBSOURCES = $(shell ls ../clib/*.c)
COLLSOURCES = $(shell ls ../coll/*.c)
PUMASOURCES = $(shell ls ../puma/*.c)

ALLSOURCES  = $(TOPSOURCES) $(CLIBSOURCES) $(COLLSOURCES) $(PUMASOURCES)

TOPOBJECTS  = $(notdir $(TOPSOURCES:.c=.o))
CLIBOBJECTS = $(notdir $(CLIBSOURCES:.c=.o))
COLLOBJECTS = $(notdir $(COLLSOURCES:.c=.o))
PUMAOBJECTS = $(notdir $(PUMASOURCES:.c=.o))

ALLOBJECTS  = $(TOPOBJECTS) $(CLIBOBJECTS) $(COLLOBJECTS) $(PUMAOBJECTS)

DEPS        = $(ALLOBJECTS:.o=.d)

CFLAGS     += -I$(CPLANT_TOP)/service/util/yod/include 

INSTALLDIR     = $(libdir)
INSTALLTARGETS = $(LIB)

# Location of default configured kernel headers to build against;
# these are only used if the linux link specified below has
# not been established...

KERNELDIR      = /Net/pecos/proj/puma01/linux/linux-$(KERNEL_VER)

#ifeq ($(TARGET),ALASKA)
#KERNELDIR      = /Net/pecos/proj/puma01/linux/linux-$(KERNEL_VER)
#endif

#ifeq ($(TARGET),SIBERIA)
#KERNELDIR      = /Net/pecos/proj/puma01/linux/siberia/linux-$(KERNEL_VER)
#endif

vpath %.c ../:../clib:../coll:../puma
vpath %.h $(CPLANT_TOP)/compute/OS/linux/include/linux

LINUX_STUFF = $(CPLANT_TOP)/compute/OS/linux

EXIST_LINUX = $(shell $(CPLANT_TOP)/config/exist.sh $(LINUX_STUFF))
EXIST_ALT   = $(shell $(CPLANT_TOP)/config/exist.sh $(KERNELDIR))

ifneq ($(EXIST_LINUX),yes)
ifneq ($(EXIST_ALT),yes)
all: bang_exist
else
all: $(CPLANT_TOP)/compute/OS/linux $(LIB)
endif
else 
all: $(CPLANT_TOP)/compute/OS/linux $(LIB)
endif


ifeq ($(MAKE_DEPS),yes)
include $(DEPS)
endif


bang_exist:
	@echo "----------------------------------------------------------"
	@echo " WARNING:"
	@echo " -- In top/lib/puma: could not find standard linux dir:"
	@echo " $(LINUX_STUFF)"
	@echo " "
	@echo " -- or the default (from top/lib/puma/Makefile-alpha) to"
	@echo " -- use as the linux dir:"
	@echo " $(KERNELDIR)"
	@echo " "
	@echo " -- You need to install configured linux kernel headers" 
	@echo " -- and have $(LINUX_STUFF) point to the directory above"
	@echo " -- the associated include dir (i.e., include/..)."
	@echo "----------------------------------------------------------"
	@exit 1


# If compute/OS/linux !exist then make it point to the backup headers
$(CPLANT_TOP)/compute/OS/linux:
	@echo "----------------------------------------------------------"
	@echo "Creating symbolic link of:"
	@echo "$@"
	@echo "to backup headers in:"
	@echo "$(KERNELDIR)"
	@echo "----------------------------------------------------------"
	ln -s $(KERNELDIR) $@

# Assume the kernel has been configured...
$(CPLANT_TOP)/compute/OS/linux/include/linux/autoconf.h:
	@echo "----------------------------------------------------------"
	@echo " -- in top/lib/puma: was checking for the linux kernel"
	@echo " -- autoconf.h and did not find one -- you probably need"
	@echo " -- to config your linux kernel. else, if you are using" 
	@echo " -- the default linux headers from the KERNELDIR location"
	@echo " -- then someone forget to configure them before"
	@echo " -- installing..."
	@echo "----------------------------------------------------------"
	@exit 1

$(TOPOBJECTS): %.o : ../%.c $(CPLANT_TOP)/compute/OS/linux/include/linux/autoconf.h
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

$(CLIBOBJECTS): %.o : ../clib/%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

$(COLLOBJECTS): %.o : ../coll/%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

$(PUMAOBJECTS): %.o : ../puma/%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

$(LIB): $(LIB)($(ALLOBJECTS))

install uninstall: $(INSTALLTARGETS)
	@$($@)

dep: $(DEPS)

clean:
	-rm -f $(ALLOBJECTS) $(LIB) $(DEPS)

