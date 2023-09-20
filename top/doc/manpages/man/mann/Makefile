#
# $Id: Makefile,v 1.5 2000/07/15 23:20:59 pumatst Exp $
#
include ../../../../Makefile-common

MANN = $(shell ls *.n)
HTML = $(notdir $(MANN:.n=.html))

.SUFFIXES : .html .n

.n.html :
	rman -f html $< > $*.html
	cp $*.html ../../html

INSTALLDIR        = $(manndir)
INSTALLTARGETS    = $(MANN)

ifeq ($(HAVE_RMAN),yes)
all: $(HTML)
else
all:
endif

html: $(HTML)

install uninstall cp: $(INSTALLTARGETS)
	@$($@)

clean:
	"rm" -f $(HTML)
