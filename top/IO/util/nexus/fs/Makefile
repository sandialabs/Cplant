#
# $Id: Makefile,v 1.3 2000/07/29 14:57:41 lward Exp $
#
SUBDIR=	nfs skel proc

PASS=	_CFLAGS="$(_CFLAGS)" RANLIB="$(RANLIB)"

all:	$(SUBDIR)
	@for i in $(SUBDIR); \
		do \
		( cd $$i; $(MAKE) $(MFLAGS) $(PASS) all ) || exit; \
		done

clean:	$(SUBDIRS)
	@for i in $(SUBDIR); \
		do \
		( cd $$i; $(MAKE) $(MFLAGS) $(PASS) clean ) || exit; \
		done

