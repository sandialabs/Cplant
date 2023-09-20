#
# $Id: Makefile,v 1.42.2.7 2002/09/16 17:29:48 jbogden Exp $
#
# This makefile just provides a wrapper so we can easily make
# both the MCP and Ethernet versions of RTSCTS at once.

include ../../../../Makefile-common

CORE_MAKEFILE = Makefile-core
TARGETS = rtscts.mod rtscts_eth.mod clean realclean install uninstall

default mcp: $(CORE_MAKEFILE)
	@$(MAKE) -f $(CORE_MAKEFILE) RTSCTS_OVER_ETHERNET=no rtscts.mod

all: $(CORE_MAKEFILE)
	@$(MAKE) realclean
	@$(MAKE) -f $(CORE_MAKEFILE) RTSCTS_OVER_ETHERNET=yes rtscts_eth.mod
	@$(MAKE) clean
	@$(MAKE) -f $(CORE_MAKEFILE) RTSCTS_OVER_ETHERNET=no rtscts.mod

eth: $(CORE_MAKEFILE)
	@$(MAKE) -f $(CORE_MAKEFILE) RTSCTS_OVER_ETHERNET=yes rtscts_eth.mod

$(TARGETS): $(SUBDIR)
	@$(MAKE) -f $(CORE_MAKEFILE) $@
