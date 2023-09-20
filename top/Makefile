#
# $Id: Makefile,v 1.11 1999/08/03 19:32:56 wmdavid Exp $
#
include Makefile-common
 

SUBDIRS   = lib

#ifeq ($(OS),linux)
#IO MUST be before compute
SUBDIRS += IO
#endif

SUBDIRS   += compute/lib compute/tool compute/test scripts

ifeq ($(OS),linux)
SUBDIRS += service/util compute/util/pct compute/OS/portals \
            compute/OS/Myrinet support/portals
endif

all:
 
%:
	$(do-subdirs)

