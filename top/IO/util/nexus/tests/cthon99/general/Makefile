#
#       @(#)Makefile	1.3 98/11/30 Connectathon Testsuite
#	1.3 Lachman ONC Test Suite source
#

DESTDIR=/no/such/path
FILES=Makefile runtests runtests.wrk *.sh *.c mkdummy rmdummy \
	nroff.in makefile.tst

all:

clean:
	rm -f *.time *.o stat

copy dist:
	cd $(DESTDIR); rm -f $(FILES)
	cp $(FILES) $(DESTDIR)
