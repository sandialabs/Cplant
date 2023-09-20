include ../../../../../Makefile-common
#If you want C version DGEMM
#CVERSION = -DC_VERSION

CFLAGS	= -O2 -I. $(CVERSION)

#CC	= gcc
#CC	= cc
#CPP	= gcc -E
#CPP	= cc -E
CPP	= $(CC) -E
LNS	= ln -fs

# If you're going to run on NT/Alpha, Please enable this flag
ASFLAGS	   = $(CVERSION) # -DWINNT

DGEMMOBJS  =  dgemm.o dgemm_nt.o dgemm_tn.o dgemm_nn.o dgemm_tt.o

SGEMMOBJS  =  sgemm.o sgemm_nt.o sgemm_tn.o sgemm_nn.o sgemm_tt.o

LIBGEMM	   = libgemm.a

all: $(LIBGEMM)

dgemm.o : gemm.c  Makefile
	$(CC) -c $(CFLAGS) -DDGEMM -o dgemm.o gemm.c

sgemm.o : gemm.c  Makefile
	$(CC) -c $(CFLAGS) -UDGEMM -o sgemm.o gemm.c

dgemm_nn.o : gemm_k.S Makefile
	rm -f temp.s
	$(CPP) -DDGEMM -DNN $(ASFLAGS) gemm_k.S > temp.s
	$(CC)  -c temp.s -o dgemm_nn.o
	rm -f temp.s

dgemm_tn.o : gemm_k.S Makefile
	rm -f temp.s
	$(CPP) -DDGEMM -DTN $(ASFLAGS) gemm_k.S > temp.s
	$(CC)  -c temp.s -o dgemm_tn.o
	rm -f temp.s

dgemm_nt.o : gemm_k.S Makefile
	rm -f temp.s
	$(CPP) -DDGEMM -DNT $(ASFLAGS) gemm_k.S > temp.s
	$(CC)  -c temp.s -o dgemm_nt.o
	rm -f temp.s

dgemm_tt.o : gemm_k.S Makefile
	rm -f temp.s
	$(CPP) -DDGEMM -DTT $(ASFLAGS) gemm_k.S > temp.s
	$(CC)  -c temp.s -o dgemm_tt.o
	rm -f temp.s

sgemm_nn.o : gemm_k.S Makefile
	rm -f temp.s
	$(CPP) -DSGEMM -DNN $(ASFLAGS) gemm_k.S > temp.s
	$(CC)  -c temp.s -o sgemm_nn.o
	rm -f temp.s

sgemm_tn.o : gemm_k.S Makefile
	rm -f temp.s
	$(CPP) -DSGEMM -DTN $(ASFLAGS) gemm_k.S > temp.s
	$(CC)  -c temp.s -o sgemm_tn.o
	rm -f temp.s

sgemm_nt.o : gemm_k.S Makefile
	rm -f temp.s
	$(CPP) -DSGEMM -DNT $(ASFLAGS) gemm_k.S > temp.s
	$(CC)  -c temp.s -o sgemm_nt.o
	rm -f temp.s

sgemm_tt.o : gemm_k.S Makefile
	rm -f temp.s
	$(CPP) -DSGEMM -DTT $(ASFLAGS) gemm_k.S > temp.s
	$(CC)  -c temp.s -o sgemm_tt.o
	rm -f temp.s

$(LIBGEMM): $(DGEMMOBJS) $(SGEMMOBJS)
	rm -f $(LIBGEMM)
	ar cq $(LIBGEMM) $(DGEMMOBJS) $(SGEMMOBJS)
	ranlib $(LIBGEMM)

clean:
	rm -rf $(LIBGEMM) *.o temp.s
