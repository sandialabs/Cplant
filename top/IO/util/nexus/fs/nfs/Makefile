#
# $Id: Makefile,v 1.3 2000/02/15 23:25:56 lward Exp $
#
RPCGEN=	../../tools/rpcgen/rpcgen -b -C -M -K -1

CMNOBJ=	mount_xdr.o nfs_xdr.o \
	enfs_xdr.o

FSPRE=	nfsmnt.h

FSSRC=	nfs_vfsops.c nfs_vops.c nfs_subr.c nfs_ino.c client.c
FSOBJ=	mount_clnt.o \
	nfs_clnt.o \
	nfsmnt.o \
	nfs_vfsops.o nfs_vops.o nfs_subr.o nfs_ino.o client.o

SVSRC=	mount_serv.c mountsvc.c \
	nfs_serv.c nfs_debug.c nfssvc.c
SVOBJ=	mount_svc.o \
	mount_serv.o mountsvc.o \
	nfs_svc.o \
	nfs_serv.o nfs_debug.o nfssvc.o

ESVSRC=	enfs_serv.c
ESVOBJ=	enfs_svc.o \
	enfs_serv.o

SRC=	$(FSSRC) $(SVSRC) $(CMNSRC) $(ESVSRC)
OBJ=	$(FSOBJ) $(SVOBJ) $(CMNOBJ) $(ESVOBJ)

CFLAGS=	$(_CFLAGS) -I../../include -I.

all:	libnfsclnt.a

libnfsclnt.a: $(FSPRE) $(OBJ)
	${AR} cr libnfsclnt.a $(OBJ)
	${RANLIB} libnfsclnt.a

nfsmnt.h: nfsmnt.x
	${RPCGEN} -h -o nfsmnt.h nfsmnt.x

nfsmnt.c: nfsmnt.x
	${RPCGEN} -c -o nfsmnt.c nfsmnt.x

mount_xdr.c: ../../include/rpcsvc/mount.x
	${RPCGEN} -c -o mount_xdr.c ../../include/rpcsvc/mount.x

mount_svc.c: ../../include/rpcsvc/mount.x
	${RPCGEN} -m -o mount_svc.c ../../include/rpcsvc/mount.x

mount_clnt.c: ../../include/rpcsvc/mount.x
	${RPCGEN} -l -o mount_clnt.c ../../include/rpcsvc/mount.x

nfs_xdr.c: ../../include/rpcsvc/nfs_prot.x
	${RPCGEN} -c -o nfs_xdr.c ../../include/rpcsvc/nfs_prot.x

nfs_svc.c: ../../include/rpcsvc/nfs_prot.x
	${RPCGEN} -m -o nfs_svc.c ../../include/rpcsvc/nfs_prot.x

nfs_clnt.c: ../../include/rpcsvc/nfs_prot.x
	${RPCGEN} -l -o nfs_clnt.c ../../include/rpcsvc/nfs_prot.x

enfs_xdr.c: ../../include/rpcsvc/enfs_prot.x
	${RPCGEN} -c -o enfs_xdr.c ../../include/rpcsvc/enfs_prot.x

enfs_svc.c: ../../include/rpcsvc/enfs_prot.x
	${RPCGEN} -m -o enfs_svc.c ../../include/rpcsvc/enfs_prot.x

clean:
	${RM} libnfsclnt.a \
	 mount_xdr.c mount_svc.c mount_clnt.c \
	 nfs_xdr.c nfs_svc.c nfs_clnt.c \
	 enfs_xdr.c enfs_svc.c \
	 nfsmnt.c nfsmnt.h \
	 $(OBJ)

real-clean: clean
	${RM} $(SRC) Makefile

nfs_vfsops.c: nfsfs.h nfsmnt.h
