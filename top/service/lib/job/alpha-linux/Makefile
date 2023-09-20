#
# $Id: Makefile-arch,v 1.1 2001/11/04 20:22:56 lafisk Exp $
#

include ../../../../Makefile-common

#
# we share the IO handling code and a few other things with original yod
#
YODDIR = $(CPLANT_TOP)/service/util/yod
YODOBJDIR = $(CPLANT_TOP)/service/util/yod/$(CPLANT_ARCH)-$(OS)

CLIBDIR   = $(YODDIR)/cmd_handlers_clib
UNISTDDIR = $(YODDIR)/cmd_handlers_unistd
UTLIBDIR  = $(YODDIR)/utlib

#
# these we need to build and keep locally, since we build them
# with the _FOR_LIBJOB switch on
#
TOPSOURCES    = $(YODDIR)/load_msgs.c $(YODDIR)/yod_tv.c $(YODDIR)/CMDhandlerTable.c

#
# these we can use right out of yod's directory since the build for
# yod will work for us too
#
CLIBSOURCES   = $(filter-out $(CLIBDIR)/CMDhandler_exit.c, $(shell ls $(CLIBDIR)/*.c))
UNISTDOBSOLETE = $(shell ls $(UNISTDDIR)/*id.c)
UNISTDSOURCES = $(filter-out $(UNISTDOBSOLETE),$(shell ls $(UNISTDDIR)/*.c))
UTLIBSOURCES  = $(shell ls $(UTLIBDIR)/*.c)

TOPOBJS =      $(patsubst $(YODDIR)/%,    %              , $(TOPSOURCES:.c=.o))
CLIBOBJS =     $(patsubst $(CLIBDIR)/%,   $(YODOBJDIR)/% , $(CLIBSOURCES:.c=.o))
UNISTDOBJS =   $(patsubst $(UNISTDDIR)/%, $(YODOBJDIR)/% , $(UNISTDSOURCES:.c=.o))
UTLIBOBJS =    $(patsubst $(UTLIBDIR)/%,  $(YODOBJDIR)/% , $(UTLIBSOURCES:.c=.o))

YODOBJS = $(TOPOBJS) $(CLIBOBJS) $(UNISTDOBJS) $(UTLIBOBJS)

LOCALTOP    = $(notdir $(TOPOBJS))
LOCALCLIB   = $(notdir $(CLIBOBJS))
LOCALUNISTD = $(notdir $(UNISTDOBJS))
LOCALUTLIB  = $(notdir $(UTLIBOBJS))

INCLUDEDIRS += -I $(YODDIR)/include -I $(CPLANT_TOP)/service/lib/job

#
############################################################################
#

LIB         = libjob.a

CSOURCES    = $(shell ls ../*.c)
COBJECTS    = $(notdir $(CSOURCES:.c=.o))
DEPS        = $(COBJECTS:.o=.d)

INSTALLTARGETS = $(LIB)
INSTALLDIR     = $(libdir)

CFLAGS += -g -Wall -D_FOR_LIBJOB

#vpath %.c ..

all: $(LIB) 

ifeq ($(MAKE_DEPS),yes)
include $(DEPS)
endif

$(YODOBJDIR):
	mkdir $(YODOBJDIR)

$(YODOBJDIR)/Makefile:
	ln -s $(YODDIR)/Makefile-arch $(YODOBJDIR)/Makefile

$(COBJECTS): %.o : ../%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

$(TOPOBJS): %.o : $(YODDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

$(CLIBOBJS): $(YODOBJDIR)/%.o : $(CLIBDIR)/%.c
	cd $(YODOBJDIR) ; $(MAKE) $(LOCALCLIB)

$(UNISTDOBJS): $(YODOBJDIR)/%.o : $(UNISTDDIR)/%.c
	cd $(YODOBJDIR) ; $(MAKE) $(LOCALUNISTD)

$(UTLIBOBJS): $(YODOBJDIR)/%.o : $(UTLIBDIR)/%.c
	cd $(YODOBJDIR) ; $(MAKE) $(LOCALUTLIB)


$(LIB):  $(COBJECTS) $(YODOBJDIR) $(YODOBJDIR)/Makefile $(YODOBJS)
	ar -r $(LIB) $(COBJECTS) $(YODOBJS)

install uninstall: $(INSTALLTARGETS)
	@$($@)

dep: $(DEPS)

clean:
	-rm -f $(COBJECTS) $(TOPOBJS) $(LIB) $(DEPS)
