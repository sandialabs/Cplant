#
# $Id: Makefile,v 0.5 1999/08/17 21:04:28 lee Stab $
#
CFLAGS=	$(_CFLAGS)

SRC=	rpc_clntout.c rpc_cout.c rpc_hout.c rpc_main.c rpc_parse.c \
	rpc_sample.c rpc_scan.c rpc_svcout.c rpc_tblout.c rpc_util.c \
	rpc_parse.h rpc_scan.h rpc_util.h
OBJ=	rpc_clntout.o rpc_cout.o rpc_hout.o rpc_main.o rpc_parse.o \
	rpc_sample.o rpc_scan.o rpc_svcout.o rpc_tblout.o rpc_util.o

all:	rpcgen

rpcgen:	$(OBJ)
	${CC} -o rpcgen $(CFLAGS) $(OBJ)

clean:
	${RM} rpcgen $(OBJ) *core*

real-clean: clean
	${RM} ${SRC} Makefile

${OBJ}:: rpc_parse.h rpc_scan.h rpc_util.h
