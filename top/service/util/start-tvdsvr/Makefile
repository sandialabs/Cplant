#
# $Id: Makefile,v 1.2 2001/10/29 22:39:36 pumatst Exp $
#

include ../../../Makefile-common

SHELL     = /bin/sh
SUBDIR    = $(CPLANT_ARCH)-$(OS)
TARGETS   = all install uninstall dep

$(TARGETS): $(SUBDIR)
	@$(MAKE) -C $(SUBDIR) $@ 

$(SUBDIR):
	mkdir $(SUBDIR)
	ln -s ../Makefile-arch $(SUBDIR)/Makefile

clean:
	@$(MAKE) MAKE_DEPS=no -C $(SUBDIR) $@

realclean:
	rm -rf $(SUBDIR)
