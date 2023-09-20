#
# $Id: Makefile,v 1.12 2001/08/03 18:28:53 jsotto Exp $

include ../../../Makefile-common

#
# Makefile to build Myrinet stuff:
#     lib			Myrinet library; e.g. libLanaiDevice.a
#     rtscts			packet and driver module
#     util			Utilities such as mcpload and mcpstat
#     MCP			The MCP that runs on the Myrinet interface card
#     eeprom			A utility similar to l3prom that lists the
#				LANai eeprom
#     module			An IP over Myrinet module
#     routes			Route generation and node to name translation
#				Utilities
#

ifeq ($(RTSCTS_OVER_ETHERNET),yes)
all:
	@(cd lib; $(MAKE))
	@(cd rtscts; $(MAKE))
	@(cd util; $(MAKE))
	@(cd vroute; $(MAKE))
	@(cd eeprom; $(MAKE))
	@(cd module; $(MAKE))
	@(cd info; $(MAKE))
else
all:
	@(cd lib; $(MAKE))
	@(cd rtscts; $(MAKE))
	@(cd util; $(MAKE))
	@(cd MCP; $(MAKE))
	@(cd vroute; $(MAKE))
	@(cd eeprom; $(MAKE))
	@(cd module; $(MAKE))
	@(cd info; $(MAKE))
	@(cd routes; $(MAKE) gensiberia myr2sandia pnid2hname showroutesiberia)
endif

#-------------------------------------------------------------------------------

clean:
	@(cd lib; $(MAKE) clean)
	@(cd rtscts; $(MAKE) clean)
	@(cd util; $(MAKE) clean)
	@(cd MCP; $(MAKE) clean)
	@(cd vroute; $(MAKE) clean)
	@(cd eeprom; $(MAKE) clean)
	@(cd module; $(MAKE) clean)
	@(cd routes; $(MAKE) clean)
	@(cd info; $(MAKE) clean)

realclean:	clean
	@(cd lib; $(MAKE) realclean)
	@(cd rtscts; $(MAKE) realclean)
	@(cd util; $(MAKE) realclean)
	@(cd MCP; $(MAKE) realclean)
	@(cd vroute; $(MAKE) realclean)
	@(cd eeprom; $(MAKE) realclean)
	@(cd module; $(MAKE) realclean)
	@(cd routes; $(MAKE) realclean)

install:
	@(cd lib; $(MAKE) install)
	@(cd rtscts; $(MAKE) install)
	@(cd util; $(MAKE) install)
	@(cd MCP; $(MAKE) install)
	@(cd vroute; $(MAKE) install)
	@(cd eeprom; $(MAKE) install)
	@(cd module; $(MAKE) install)
	@(cd info; $(MAKE) install)
	@(cd routes; $(MAKE) install)

depend:
	@(cd lib; $(MAKE) depend)
	@(cd rtscts; $(MAKE) depend)
	@(cd util; $(MAKE) depend)
	@(cd MCP; $(MAKE) depend)
	@(cd vroute; $(MAKE) depend)
	@(cd module; $(MAKE) depend)
