#
# $Id: Makefile,v 1.3.2.2 2002/04/19 17:03:18 jrstear Exp $
#

include ../../../Makefile-common

SHELL     = /bin/sh
SUBDIR    = $(CPLANT_ARCH)-$(OS)
TARGETS   = install uninstall dep

all: default

ifeq ($(OS),linux)
default: $(SUBDIR)
	@$(MAKE) realclean
	@$(MAKE) no2underscores
	@$(MAKE) clean
	@$(MAKE) pmpi
	@$(MAKE) clean
	@$(MAKE) normal
endif

ifeq ($(OS),osf)
default: $(SUBDIR)
	@$(MAKE) normal
endif
                
$(TARGETS): $(SUBDIR)
	@$(MAKE) NO2UNDERSCORE=no PMPI=no -C $(SUBDIR) $@ 

normal: $(SUBDIR)
	@$(MAKE) NO2UNDERSCORE=no PMPI=no -C $(SUBDIR) all

no2underscores: $(SUBDIR)
	@$(MAKE) NO2UNDERSCORE=yes PMPI=no -C $(SUBDIR) all

pmpi: $(SUBDIR)
	@$(MAKE) NO2UNDERSCORE=no PMPI=yes -C $(SUBDIR) all

        
$(SUBDIR):
	mkdir $(SUBDIR)
	ln -s ../Makefile-arch $(SUBDIR)/Makefile

clean:
	@$(MAKE) MAKE_DEPS=no -C $(SUBDIR) $@

realclean:
	if [ -d $(SUBDIR) ]; then \
	  $(MAKE) -C $(SUBDIR) $@; \
	  rm -rf $(SUBDIR); \
	fi

