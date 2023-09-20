#
#  $Id: Makefile,v 1.1 2001/12/18 00:34:42 rbbrigh Exp $
#

include ../../../../../Makefile-common
 
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

