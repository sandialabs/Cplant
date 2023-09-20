#
# $Id: Makefile,v 0.8 2001/07/18 18:57:26 rklundt Exp $
#
CFLAGS=	$(_CFLAGS) -I../include

SRC=	assert.c compress.c creds.c logmsg.c panic.c my_perror.c savstr.c \
	snglthrd.c heap.c rwlk.c
OBJ=	assert.o compress.o creds.o logmsg.o panic.o my_perror.o savstr.o \
	snglthrd.o heap.o rwlk.o

all:	libcmn.a

libcmn.a: $(OBJ)
	$(AR) cr libcmn.a $(OBJ)
	${RANLIB} libcmn.a

clean:
	${RM} libcmn.a $(OBJ)

real-clean: clean
	${RM} ${SRC} Makefile
