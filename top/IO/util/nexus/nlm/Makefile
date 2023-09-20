RPCGEN=	../tools/rpcgen/rpcgen -b -C -M -K -1

OBJ=	nlm_serv.o nlm_svc.o nlm_xdr.o nlm_clnt.o

CFLAGS=	$(_CFLAGS) -I../include -I.

all:	libnlm.a

libnlm.a: $(OBJ)
	${AR} cr libnlm.a $(OBJ)
	${RANLIB} libnlm.a

nlm_xdr.c: ../include/rpcsvc/nlm_prot.x
	${RPCGEN} -c -o nlm_xdr.c ../include/rpcsvc/nlm_prot.x

nlm_svc.c: ../include/rpcsvc/nlm_prot.x
	${RPCGEN} -m -o nlm_svc.c ../include/rpcsvc/nlm_prot.x

nlm_clnt.c: ../include/rpcsvc/nlm_prot.x
	${RPCGEN} -l -o nlm_clnt.c ../include/rpcsvc/nlm_prot.x

clean:
	${RM} libnlm.a \
	 nlm_xdr.c nlm_svc.c nlm_clnt.c \
	 $(OBJ)
