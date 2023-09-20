#
# $Id: Makefile,v 1.6 2001/10/29 22:50:42 pumatst Exp $
#

include ../../../../../../Makefile-common

SHELL     = /bin/sh
SUBDIR    = $(CPLANT_ARCH)-$(OS)
TARGETS   = all install uninstall dep clean

$(TARGETS): $(SUBDIR)
	@$(MAKE) -C $(SUBDIR) $@ 

$(SUBDIR):
	mkdir $(SUBDIR)
	ln -s ../Makefile-arch $(SUBDIR)/Makefile

realclean:
	rm -rf $(SUBDIR)
