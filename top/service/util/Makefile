#
# $Id: Makefile,v 1.13 2001/03/12 19:48:39 pumatst Exp $
#

include ../../Makefile-common

SUBDIRS  = yod bebopd bt cgdb showmesh start-tvdsvr

ifeq ($(USE_DB),yes)
SUBDIRS  += db
endif

all:
%:
	$(do-subdirs)
