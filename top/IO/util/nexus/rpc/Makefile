#
# $Id: Makefile,v 0.4 1999/08/17 21:49:28 lee Stab $
#
CFLAGS= $(_CFLAGS) -I. -I../include -DPORTING_CHECKS

SRC=	auth_none.c auth_unix.c authunix_prot.c bindresvport.c \
	clnt_generic.c clnt_perror.c clnt_raw.c clnt_simple.c \
	clnt_tcp.c clnt_udp.c get_myaddress.c getrpcent.c getrpcport.c \
	pmap_clnt.c pmap_getmaps.c pmap_getport.c pmap_prot.c pmap_prot2.c \
	pmap_rmt.c rpc_callmsg.c rpc_commondata.c rpc_dtablesize.c \
	rpc_prot.c svc.c svc_auth.c svc_auth_unix.c svc_raw.c svc_run.c \
	svc_simple.c svc_tcp.c svc_udp.c xdr.c xdr_array.c xdr_float.c \
	xdr_mem.c xdr_rec.c xdr_reference.c xdr_stdio.c
OBJ=	auth_none.o auth_unix.o authunix_prot.o bindresvport.o \
	clnt_generic.o clnt_perror.o clnt_raw.o clnt_simple.o \
	clnt_tcp.o clnt_udp.o get_myaddress.o getrpcent.o getrpcport.o \
	pmap_clnt.o pmap_getmaps.o pmap_getport.o pmap_prot.o pmap_prot2.o \
	pmap_rmt.o rpc_callmsg.o rpc_commondata.o rpc_dtablesize.o \
	rpc_prot.o svc.o svc_auth.o svc_auth_unix.o svc_raw.o svc_run.o \
	svc_simple.o svc_tcp.o svc_udp.o xdr.o xdr_array.o xdr_float.o \
	xdr_mem.o xdr_rec.o xdr_reference.o xdr_stdio.o

all:	librpc.a

librpc.a: $(OBJ)
	$(AR) cr librpc.a $(OBJ)
	$(RANLIB) librpc.a

clean:
	$(RM) librpc.a $(OBJ)

real-clean: clean
	$(RM) $(SRC) private.h Makefile

$(OBJ):: private.h
