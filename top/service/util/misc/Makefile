#
# $Id: Makefile,v 1.2.2.5 2002/10/02 18:25:13 jrstear Exp $
#

include ../../../Makefile-common

INSTALLDIR     = $(bindir_sys)
INSTALLTARGETS = nrsh infoerr_parse.pl infoprotocol_parse.pl jobcheck myrinet_routes2cplant-map myrinet_hosts2misc logsum

all:

install uninstall: $(INSTALLTARGETS)
	@$($@)

clean:
	@ rm -f $(EXECS)
	

