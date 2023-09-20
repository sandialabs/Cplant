#
# Makefile for the bin directory
#
# $Id: Makefile,v 1.7 2001/08/14 19:48:55 rklundt Exp $
#

#
# We use the system supplied RPC library. Ours comes with too much overhead
# for a program that will always be standalone.
#
#RPCGEN=	../tools/rpcgen/rpcgen -b -C -M
RPCGEN=	rpcgen -b -C

CMN_SRC= \
	cnx_misc.c

CNX_MOUNT_NFS_SRC= \
	cnx_mount_nfs.c

CNX_MOUNT_SKELFS_SRC= \
	cnx_mount_skelfs.c

CNX_MOUNT_NFS_OBJ= \
	cnx_mount_nfs.o cnx_misc.o cnx_clnt.o cnx_xdr.o nfsmnt_xdr.o

CNX_MOUNT_SKELFS_OBJ= \
	cnx_mount_skelfs.o cnx_misc.o cnx_clnt.o cnx_xdr.o skelfsmnt_xdr.o

CNX_UMOUNT_SRC= \
	cnx_umount.c

CNX_UMOUNT_OBJ= \
	cnx_umount.o cnx_misc.o cnx_clnt.o cnx_xdr.o

CNX_START_SRC= \
	cnx_start.c

CNX_START_OBJ= \
	cnx_start.o cnx_misc.o cnx_clnt.o cnx_xdr.o

CFLAGS= $(_CFLAGS) -I.

all:	cnx_mount_nfs cnx_umount cnx_start cnx_mount_skelfs
	cp cnx_mount_nfs cnx_umount cnx_start cnx_mount_skelfs ..

cnx_mount_nfs: $(CNX_MOUNT_NFS_OBJ)
	${CC} -o cnx_mount_nfs $(CNX_MOUNT_NFS_OBJ)

cnx_mount_skelfs: $(CNX_MOUNT_SKELFS_OBJ)
	${CC} -o cnx_mount_skelfs $(CNX_MOUNT_SKELFS_OBJ)

cnx_start: $(CNX_START_OBJ)
	${CC} -o cnx_start $(CNX_START_OBJ)

cnx_umount: $(CNX_UMOUNT_OBJ)
	${CC} -o cnx_umount $(CNX_UMOUNT_OBJ)

clean:
	${RM} \
	 cnx_mount_nfs $(CNX_MOUNT_NFS_OBJ) \
	 nfsmnt_xdr.c nfsmnt.h nfsmnt.x \
	 cnx_umount $(CNX_UMOUNT_OBJ) \
	 cnx_start $(CNX_START_OBJ) \
	 cnx_clnt.c cnx_xdr.c cnx_prot.h \
	 cnx_prot.x \
	 cnx_mount_skelfs $(CNX_MOUNT_SKELFS_OBJ) \
	 skelfsmnt_xdr.c skelfsmnt.h skelfsmnt.x \

cnx_prot.x: ../include/rpcsvc/cnx_prot.x
	ln ../include/rpcsvc/cnx_prot.x cnx_prot.x

cnx_prot.h: cnx_prot.x
	${RM} cnx_prot.h
	${RPCGEN} -h -o cnx_prot.h cnx_prot.x

cnx_clnt.c: cnx_prot.h cnx_prot.x
	${RM} cnx_clnt.c
	${RPCGEN} -l -o cnx_clnt.c cnx_prot.x

cnx_xdr.c: cnx_prot.h cnx_prot.x
	${RM} cnx_xdr.c
	${RPCGEN} -c -o cnx_xdr.c cnx_prot.x

nfsmnt.x: ../fs/nfs/nfsmnt.x
	ln ../fs/nfs/nfsmnt.x nfsmnt.x

nfsmnt.h: nfsmnt.x
	${RM} nfsmnt.h
	${RPCGEN} -h -o nfsmnt.h nfsmnt.x

nfsmnt_xdr.c: nfsmnt.h nfsmnt.x
	${RM} nfsmnt_xdr.c
	${RPCGEN} -c -o nfsmnt_xdr.c nfsmnt.x

cnx_mount_nfs.o cnx_umount.o cnx_start.o: cnx_prot.h nfsmnt.h

skelfsmnt.x: ../fs/skel/skelfsmnt.x
	ln ../fs/skel/skelfsmnt.x skelfsmnt.x

skelfsmnt.h: skelfsmnt.x
	${RM} skelfsmnt.h
	${RPCGEN} -h -o skelfsmnt.h skelfsmnt.x

skelfsmnt_xdr.c: skelfsmnt.h skelfsmnt.x
	${RM} skelfsmnt_xdr.c
	${RPCGEN} -c -o skelfsmnt_xdr.c skelfsmnt.x

cnx_mount_skelfs.o cnx_umount.o cnx_start.o: cnx_prot.h nfsmnt.h skelfsmnt.h
