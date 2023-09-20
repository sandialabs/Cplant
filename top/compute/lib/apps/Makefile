#
# $Id: Makefile,v 1.11 2001/10/29 16:37:39 pumatst Exp $
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
	ln -s ../Makefile-apps $(SUBDIR)/Makefile-apps

clean:
	@$(MAKE) MAKE_DEPS=no -C $(SUBDIR) $@

realclean:
	rm -rf $(SUBDIR)
