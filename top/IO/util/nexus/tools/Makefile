#
# $Id: Makefile,v 0.2 1999/08/17 21:21:33 lee Stab $
#
SUBDIR=	rpcgen

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

real-clean: $(SUBDIRS)
	${RM} Makefile
	@for i in $(SUBDIR); \
		do \
		( cd $$i; $(MAKE) $(MFLAGS) $(PASS) real-clean ) || exit; \
		done
