#
# $Id: Makefile,v 1.3 2001/07/18 18:57:39 rklundt Exp $
#
RPCGEN= ../../tools/rpcgen/rpcgen -b -C -M -K -1

FSPRE=	skelfsmnt.h skelfsmnt.c

SRC=	skelfs_vfsops.c skelfs_vops.c dtree.c \
	hash.c skelfsmnt.c

OBJ=	${SRC:.c=.o}

RANLIB=	ranlib
AR=	ar
RM=	rm

CFLAGS=	$(_CFLAGS) -I../../include -I.

all:	libskelfsclnt.a

libskelfsclnt.a: ${FSPRE} ${OBJ}
	${AR} cr libskelfsclnt.a ${OBJ}
	${RANLIB} libskelfsclnt.a

skelfsmnt.h: skelfsmnt.x
	${RPCGEN} -h -o skelfsmnt.h skelfsmnt.x

skelfsmnt.c: skelfsmnt.x
	${RPCGEN} -c -o skelfsmnt.c skelfsmnt.x

clean:
	${RM} -f libskelfsclnt.a ${OBJ} ${FSPRE}

