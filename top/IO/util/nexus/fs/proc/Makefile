#
# Makefile for the CNX subordinate process file system
#
# $Id: Makefile,v 1.1 2000/06/22 17:14:00 smorgan Exp $
#
RPCGEN= ../../tools/rpcgen/rpcgen -b -C -M -K -1

SRC=	procfs_vfsops.c procfs_vops.c

OBJ=	${SRC:.c=.o}

RANLIB=	ranlib
AR=	ar
RM=	rm

CFLAGS=	$(_CFLAGS) -I../../include -I.

all:	libprocfsclnt.a

libprocfsclnt.a: ${OBJ}
	${AR} cr $@ ${OBJ}
	${RANLIB} $@

clean:
	${RM} -f libprocfsclnt.a ${OBJ}

