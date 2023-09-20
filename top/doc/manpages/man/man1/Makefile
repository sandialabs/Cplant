#
# $Id: Makefile,v 1.5 2000/07/15 23:19:48 pumatst Exp $
#
include ../../../../Makefile-common

MAN1 = $(shell ls *.1)
HTML = $(notdir $(MAN1:.1=.html))

.SUFFIXES : .html .1

.1.html :
	rman -f html $< > $*.html
	cp $*.html ../../html

INSTALLDIR        = $(man1dir)
INSTALLTARGETS    = $(MAN1)

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
