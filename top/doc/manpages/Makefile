#
# $Id: Makefile,v 1.4.2.2 2002/10/01 20:25:25 jrstear Exp $
#

include ../../Makefile-common

SUBDIRS  = $(wildcard sgml*)

all:
%:
	$(do-subdirs)

install:
	cp -R man/man?/ $(mandir)/
	cp -R html/* $(htmldir)/
	cp -R ps/* $(psdir)/
