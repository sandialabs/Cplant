#
#       @(#)Makefile	1.4 99/02/09 Connectathon Testsuite
#
# 'make all'			makes test programs
# 'make clean'			cleans directories
# 'make copy DESTDIR=path'	copies test programs to path
# 'make dist DESTDIR=path'	copies sources to path

DESTDIR=/no/such/path
COPYFILES=runtests tests.init server domount.c README READWIN.txt Testitems \
	getopt.c tests.h unixdos.h

include tests.init

all: domount getopt
	cd basic; $(MAKE)
	cd general; $(MAKE)
	cd special; $(MAKE)
	cd tools; $(MAKE)
	cd lock; $(MAKE)

lint:
	cd basic; $(MAKE) lint
	cd special; $(MAKE) lint
	cd tools; $(MAKE) lint
	cd lock; $(MAKE) lint

domount: domount.c
	$(CC) $(CFLAGS) -o $@ $@.c
	-chown root domount && chmod u+s domount

getopt: getopt.c
	$(CC) $(CFLAGS) -o $@ $@.c

clean:
	rm -f domount getopt
	cd basic; $(MAKE) clean
	cd general; $(MAKE) clean
	cd special; $(MAKE) clean
	cd tools; $(MAKE) clean
	cd lock; $(MAKE) clean

copy: mknewdirs
	cp -f domount $(COPYFILES) $(DESTDIR)
	cd basic; $(MAKE) copy DESTDIR=$(DESTDIR)/basic
	cd general; $(MAKE) copy DESTDIR=$(DESTDIR)/general
	cd special; $(MAKE) copy DESTDIR=$(DESTDIR)/special
	cd tools; $(MAKE) copy DESTDIR=$(DESTDIR)/tools
	cd lock; $(MAKE) copy DESTDIR=$(DESTDIR)/lock

dist: mknewdirs
	cp -f Makefile $(COPYFILES) $(DESTDIR)
	cd basic; $(MAKE) dist DESTDIR=$(DESTDIR)/basic
	cd general; $(MAKE) dist DESTDIR=$(DESTDIR)/general
	cd special; $(MAKE) dist DESTDIR=$(DESTDIR)/special
	cd tools; $(MAKE) dist DESTDIR=$(DESTDIR)/tools
	cd lock; $(MAKE) dist DESTDIR=$(DESTDIR)/lock

mknewdirs:
	-mkdir $(DESTDIR)/basic $(DESTDIR)/general $(DESTDIR)/special \
	       $(DESTDIR)/tools $(DESTDIR)/lock
