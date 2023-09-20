#
# $Id: Makefile,v 1.4 2000/07/15 23:20:37 pumatst Exp $
#

include ../../../../Makefile-common

MAN8 = $(shell ls *.8)
HTML = $(notdir $(MAN8:.8=.html))

.SUFFIXES : .html .8

.8.html :
	rman -f html $< > $*.html
	cp $*.html ../../html

INSTALLDIR        = $(man8dir)
INSTALLTARGETS    = $(MAN8)

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
