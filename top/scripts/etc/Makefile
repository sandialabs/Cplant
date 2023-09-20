#
# $Id: Makefile,v 1.18.4.2 2002/08/09 21:58:33 jrstear Exp $
#

include ../../Makefile-common

all:

install:
	# copy etc.cluster config directories into place...
	tar --exclude='*CVS*' -cf - etc.* | tar -C $(prefix_sys) -xvf -
