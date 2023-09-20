#
# $Id: Makefile,v 1.7.4.2 2002/09/30 21:30:57 jrstear Exp $
#

include ../../../Makefile-common

EXECS = showmesh

INSTALLDIR     = $(bindir_sys)
INSTALLTARGETS = $(EXECS) showmesh.WEST showmesh.ROSS showmesh.ROSS2 showmesh.ZERMATT showmesh.full

all: $(EXECS)

showmesh: showmesh

install uninstall: $(INSTALLTARGETS)
	@$($@)

clean:
	@ rm -f $(EXECS)
	

