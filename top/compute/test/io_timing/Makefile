#
# $Id: Makefile,v 1.2 1999/01/29 23:05:01 jpvandy Exp $
#

include ../../../Makefile-common

SHELL     = /bin/sh
SUBDIR    = $(CPLANT_ARCH)-$(OS)
TARGETS   = all install uninstall dep clean print

$(TARGETS): $(SUBDIR)
	@$(MAKE) -C $(SUBDIR) $@ 

$(SUBDIR):
	mkdir $(SUBDIR)
	ln -s ../Makefile-$(CPLANT_ARCH) $(SUBDIR)/Makefile

realclean:
	rm -rf $(SUBDIR)
