#
# $Id: Makefile,v 1.2 1998/04/08 20:30:06 bright Exp $
#
include ../../../../Makefile-common

OBJ_SRCS= CMDhandler_tempnam.c CMDhandler_tmpnam.c
OBJS= $(patsubst %.c, %.o, $(OBJ_SRCS))

all: libcmd.a

libcmd.a:  $(OBJS)
	ar r ../cmd_handlers_unistd/libcmd.a $(OBJS)

depend: 
	$(MAKEDEP_OBJS)

clean:
	rm -f *.o makedep.objs
