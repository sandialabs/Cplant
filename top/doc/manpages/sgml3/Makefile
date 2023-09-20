#
# $Id: Makefile,v 1.3.4.2 2002/09/27 17:12:45 jrstear Exp $
#

include ../../../Makefile-common

# This makefile just doesn't work well with ksh. Use /bin/sh
SHELL = /bin/sh

TARGETS       = $(wildcard *.sgml)
MANDIR        = ../man/man3
MANTARGETS    = $(addprefix $(MANDIR)/,$(TARGETS:.sgml=.3))

#
# we should generate these links automatically with a perl
# script that reads the sgml files.
#
MANLINKSRC      = $(wildcard *.3)
MANLINKDST      = $(addprefix $(MANDIR)/, $(MANLINKSRC))

PSDIR         = ../ps
PSTARGETS     = $(addprefix $(PSDIR)/, $(TARGETS:.sgml=.ps))
HTMLDIR       = ../html
HTMLTARGETS   = $(addprefix $(HTMLDIR)/, $(TARGETS:.sgml=.html))


###############################################################################
# Find SGML format
#
floc1 = $(wildcard /usr/local/sgml2latex-1.3/bin/format)
ifeq ($(OS_TYPE),SunOS)
    OS_REV = $(shell uname -r)
    ifeq ($(OS_REV),5.5)
	floc2 = $(wildcard /Net/pumatmp1/tools.solaris/sgml2latex-1.3/bin/format)
    else
	floc2 = $(wildcard /Net/pumatmp1/tools/sgml2latex-1.3/bin/format)
    endif
else
    ifeq ($(OS_TYPE),Linux)
	floc2 = $(wildcard /usr/local/bin/format)
    endif
endif

ifneq ($(floc1),)
    SGML2MAN = $(floc1)
endif
ifneq ($(floc2),)
    SGML2MAN = $(floc2)
endif
SGML2MANFLAGS = -T pumaman


###############################################################################
# Find groff
#
try1 = $(wildcard /usr/bin/groff)
try2 = $(wildcard /usr/local/bin/groff)
try3 = $(wildcard /usr/local/gnu/bin/groff)

ifneq ($(try3),)
    MAN2PS        = $(try3)
endif
ifneq ($(try2),)
    MAN2PS        = $(try2)
endif
ifneq ($(try1),)
    MAN2PS        = $(try1)
endif
MAN2PSFLAGS   = -man -Wfont


###############################################################################
# Find rman
#
here1 = $(wildcard /usr/local/bin/rman)
ifeq ($(OS_TYPE),SunOS)
    OS_REV = $(shell uname -r)
    ifeq ($(OS_REV),5.5)
	here2 = $(wildcard /Net/pumatmp1/tools.solaris/bin/rman)
    else
	here2 = $(wildcard /Net/pumatmp1/tools/bin/rman)
    endif
endif
here3 = $(wildcard /usr/X11R6/bin/rman)
here4 = $(wildcard /usr/bin/rman)

ifneq ($(here4),)
    MAN2HTML      = $(here4)
endif
ifneq ($(here2),)
    MAN2HTML      = $(here2)
endif
ifneq ($(here1),)
    MAN2HTML      = $(here1)
endif
ifneq ($(here3),)
    MAN2HTML      = $(here3)
endif
MAN2HTMLFLAGS = -f HTML



###############################################################################
# Now get some work done
#

INSTALLDIR     = $(man3dir)
INSTALLTARGETS = $(MANTARGETS)

all:
	echo "doing nothing (everything should already be converted)"

format: $(MANTARGETS) COPYLINKS $(PSTARGETS) $(HTMLTARGETS)

.SECONDARY:

$(MANDIR) $(PSDIR) $(HTMLDIR):
	mkdir $@

$(MANDIR)/%.3: %.sgml $(MANDIR)
	$(SGML2MAN) $(SGML2MANFLAGS) $< > $@

COPYLINKS: $(MANLINKSRC)
	cp -f $(MANLINKSRC) $(MANDIR)

$(PSDIR)/%.ps: $(MANDIR)/%.3 $(PSDIR) 
	$(MAN2PS) $(MAN2PSFLAGS) $< > $@

$(HTMLDIR)/%.html: $(MANDIR)/%.3 $(HTMLDIR) 
	$(MAN2HTML) $(MAN2HTMLFLAGS) $< > $@

install uninstall: $(INSTALLTARGETS)
	@$($@)

clean:
	@echo "doing nothing.  use make cclean or realclean to really prep for reformatting"

cclean:
	-rm -f $(MANTARGETS) $(PSTARGETS) $(HTMLTARGETS) $(MANLINKDST)

realclean:
	-rm -rf $(MANDIR) $(PSDIR) $(HTMLDIR) 

