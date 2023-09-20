#
# $Id: Makefile,v 1.2 1999/12/18 23:29:38 jotto Exp $
#

include ../../../Makefile-common

HTML = $(shell ls *.html)

INSTALLDIR        = $(htmldir)
INSTALLTARGETS    = $(HTML)

all:

install uninstall cp: $(INSTALLTARGETS)
	@$($@)

clean:
	"rm" -f $(HTML)
