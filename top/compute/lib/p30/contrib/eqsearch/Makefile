#
#  $Id: Makefile,v 1.1 2001/02/12 22:05:01 rbbrigh Exp $
#

include ../../../../../Makefile-common
 
SHELL     = /bin/sh
SUBDIR    = $(CPLANT_ARCH)-$(OS)
TARGETS   = all install uninstall dep clean
 
$(TARGETS): $(SUBDIR)
	@$(MAKE) -C $(SUBDIR) $@
 
$(SUBDIR):
	mkdir $(SUBDIR)
	ln -s ../Makefile-$(CPLANT_ARCH) $(SUBDIR)/Makefile

realclean:
	rm -rf $(SUBDIR) ../lib

