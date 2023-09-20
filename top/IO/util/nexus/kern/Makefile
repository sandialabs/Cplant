#
# $Id: Makefile,v 0.8 2001/08/14 20:59:11 rklundt Exp $
#
RPCGEN=	../tools/rpcgen/rpcgen -b -C -M -K -1

SRC=	main.c debug.c namei.c mount.c vfs_subr.c vfs_conf.c \
	dead_vfsops.c dead_vops.c \
	cnx_serv.c cnx_subr.c cnx_conf.c cnx_debug.c \
	rpc_svc.c rpc_creds.c
OBJ=	main.o debug.o namei.o mount.o vfs_subr.o vfs_conf.o \
	dead_vfsops.o dead_vops.o \
	cnx_serv.o \
	cnx_svc.o cnx_subr.o cnx_conf.o cnx_debug.o cnx_xdr.o \
	rpc_svc.o rpc_creds.o

ifdef CPLANT_TOP
	LINUX_INCLUDE=-I$(CPLANT_TOP)/compute/OS/linux/include
endif

CFLAGS=	$(_CFLAGS) -I../include -I.  $(LINUX_INCLUDE)

all:	libkern.a

libkern.a: $(OBJ)
	${AR} cr libkern.a $(OBJ)
	${RANLIB} libkern.a

clean:
	${RM} cnx_svc.c cnx_xdr.c libkern.a $(OBJ)

real-clean: clean
	${RM} $(SRC) Makefile

cnx_xdr.c: ../include/rpcsvc/cnx_prot.x
	${RPCGEN} -c -o cnx_xdr.c ../include/rpcsvc/cnx_prot.x

cnx_svc.c: ../include/rpcsvc/cnx_prot.x
	${RPCGEN} -m -o cnx_svc.c ../include/rpcsvc/cnx_prot.x
