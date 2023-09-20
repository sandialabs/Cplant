#
# $Id: Makefile,v 1.12.6.2 2002/10/04 16:26:14 jrstear Exp $
# Makefile for the routes directory. These files can be compiled
# anywhere; not just under Alpha-linux. The only requirement to
# build routes is that simple_routes is somewhere in the PATH.
# Read HOWTO-routes on how to obetain and build rimple_routes
# from Myricom.
#

include ../../../../Makefile-common

CC = gcc

TARGETS	= gensiberia geniceberg geniceberg2 myrinet_routes2cplant_routes pnid2hname \
	  sib2cplant showroutesiberia gen_nodeNames hname2pnid genroutesiberia


all:   $(TARGETS) show.pl
routes: routes.siberia routes.tinyplant routes.iceberg routes.iceberg2

sib2cplant: sib2cplant.c gen_host.c
	$(CC) -O -o $@ $^

genroutesiberia:	genroutesiberia.c gen_host.c
	$(CC) -O -o $@ $^

gensiberia:	gensiberia.c gen_host.c
	$(CC) -O -o $@ $^

geniceberg:	geniceberg.c gen_host.c
	$(CC) -O -o $@ $^

geniceberg2:	geniceberg2.c gen_host.c
	$(CC) -O -o $@ $^

myrinet_routes2cplant_routes:	myrinet_routes2cplant_routes.c
	$(CC) -O -o $@ $^

hname2pnid:	hname2pnid.c gen_host.c
	$(CC) -O -o $@ $^

gen_nodeNames:	gen_nodeNames.c gen_host.c
	$(CC) -O -o $@ $^

pnid2hname:	pnid2hname.c gen_host.c
	$(CC) -O -o $@ $^

showroutesiberia:	showroutesiberia.c gen_host.c
	$(CC) -O -o $@ $^

show.pl:
	@echo "    Should be here..."

simple_routes.$(CPLANT_ARCH):
	@echo " "
	@echo "    Should be checked in...or build from Myricom's"
	@echo "    GM distribution and put it, or a link, in this directory"
	@echo " "

#
# Siberia
#
siberia.map:	gensiberia
	gensiberia > $@

siberia.routes:	siberia.map simple_routes.$(CPLANT_ARCH)
	simple_routes.$(CPLANT_ARCH) siberia.map stdout -route-args > $@

routes.siberia:		siberia.routes myrinet_routes2cplant_routes
	@mkdir $@
	(cd $@ ; ../myrinet_routes2cplant_routes ../$< siberia ; cd ..)
	@echo "Route generation done"

#
# Tinyplant
#
tinyplant.routes:	tinyplant.map simple_routes.$(CPLANT_ARCH)
	simple_routes.$(CPLANT_ARCH) tinyplant.map stdout -route-args > $@

routes.tinyplant:	tinyplant.routes myrinet_routes2cplant_routes
	@mkdir $@
	(cd $@ ; ../myrinet_routes2cplant_routes ../$< tinyplant ; cd ..)
	@echo "Route generation done"

#
# iceberg
#
iceberg.map:	geniceberg
	geniceberg > $@

iceberg.routes:	iceberg.map simple_routes.$(CPLANT_ARCH)
	simple_routes.$(CPLANT_ARCH) iceberg.map stdout -route-args > $@

routes.iceberg:	iceberg.routes myrinet_routes2cplant_routes
	@mkdir $@
	(cd $@ ; ../myrinet_routes2cplant_routes ../$< iceberg ; cd ..)
	@echo "Route generation done"



#
# iceberg2
#
iceberg2.map:	geniceberg2
	geniceberg2 > $@

iceberg2.routes:	iceberg2.map simple_routes.$(CPLANT_ARCH)
	simple_routes.$(CPLANT_ARCH) iceberg2.map stdout -route-args > $@

routes.iceberg2:	iceberg2.routes myrinet_routes2cplant_routes
	@mkdir $@
	(cd $@ ; ../myrinet_routes2cplant_routes ../$< iceberg2 ; cd ..)
	@echo "Route generation done"



# ### -------------------------------------------------------------------- ### #

.PHONY:	clean realclean 

install cp:
	install $(TARGETS) show.pl $(sbindir)

clean:
	"rm" -rf *.o *.lst $(TARGETS) routes.siberia routes.tinyplant \
	    routes.iceberg routes.iceberg2

realclean:	clean
	"rm" -rf siberia.map iceberg.map iceberg2.map \
		siberia.routes tinyplant.routes iceberg.routes iceberg2.routes
