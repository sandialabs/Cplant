#
# Top level makefile.
#
# $Id: Makefile,v 0.14.4.2.2.2 2002/09/26 23:45:20 rklundt Exp $
#
-include ../../../Makefile-common

FSSUPP=	-DENFS  -DSKELFS

CMNCFLAGS= -O2 -W -Wall -DDEBUG -DINSTRUMENT -DNSTORES=10
BLDCFLAGS= -g $(FSSUPP) -DUSE_MALLOC -DSINGLE_THREAD -DVNODES_MAX=4096
#BLDCFLAGS= -g $(FSSUPP) -DUSE_MALLOC
DBGCFLAGS= $(CMNCFLAGS) $(BLDCFLAGS)
#LIBS=	-lpthread
#LIBS=-lefence

_CFLAGS= $(DBGCFLAGS)
RANLIB=	ranlib

SUBDIR=	tools rpc cmn kern fs bin

PASS=	_CFLAGS="$(_CFLAGS)" LIBS="$(LIBS)" RANLIB="$(RANLIB)"

CFLAGS=	$(_CFLAGS)

INSTALLTARGETS= enfsd cnx_start cnx_umount cnx_mount_skelfs cnx_mount_nfs
INSTALLDIR= $(sbindir)

all:	enfsd

enfsd:	subdirs rev.o vers.o
	${CC} ${CFLAGS} -o enfsd \
	 rev.o vers.o \
	 kern/libkern.a \
	 fs/nfs/libnfsclnt.a \
	 fs/skel/libskelfsclnt.a \
	 rpc/librpc.a cmn/libcmn.a \
	 $(LIBS)

vers.c: FORCE
	date=`date -u`; \
	by=`whoami`@`hostname`; \
	 echo "const char *build_info = \"$${date} by $${by}\";" > vers.c

FORCE:

subdirs: $(SUBDIR)
	@for i in $(SUBDIR); \
		do \
		( cd $$i; $(MAKE) $(MFLAGS) $(PASS) all ) || exit; \
		done

clean:	$(SUBDIRS)
	${RM} vers.c vers.o rev.o
	${RM} enfsd *core* errs out
	@for i in $(SUBDIR); \
		do \
		( cd $$i; $(MAKE) $(MFLAGS) $(PASS) clean ) || exit; \
		done

real-clean: clean $(SUBDIRS)
	${RM} enfsd *core* errs out
	@for i in $(SUBDIR); \
		do \
		( cd $$i; $(MAKE) $(MFLAGS) $(PASS) real-clean ) || exit; \
		done
	${RM} Makefile

install:  all
	@$($@)
