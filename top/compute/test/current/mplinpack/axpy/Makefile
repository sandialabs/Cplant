include ../../../../../Makefile-common

#CC	= gcc
#CC	= cc
#CPP	= gcc -E
#CPP	= cc -E
CPP	= $(CC) -E
LNS	= ln -fs
#F77	= g77
#F77	= f77

AXPY	= daxpy.o saxpy.o zaxpy.o caxpy.o

LIBAXPY = libaxpy.a

all: $(LIBAXPY)

$(LIBAXPY) : $(AXPY)
	rm -f $(LIBAXPY)
	ar cq $(LIBAXPY) $(AXPY)
	ranlib $(LIBAXPY)

daxpy.o : axpy.S
	rm -f temp.s
	$(CPP) -DDOUBLE axpy.S > temp.s
	$(AS) -o daxpy.o temp.s
	rm -f temp.s

saxpy.o : axpy.S
	rm -f temp.s
	$(CPP) -UDOUBLE axpy.S > temp.s
	$(AS) -o saxpy.o temp.s
	rm -f temp.s

zaxpy.o : zaxpy.S
	rm -f temp.s
	$(CPP) -DDOUBLE zaxpy.S > temp.s
	$(AS) -o zaxpy.o temp.s
	rm -f temp.s

caxpy.o : zaxpy.S
	rm -f temp.s
	$(CPP) -UDOUBLE zaxpy.S > temp.s
	$(AS) -o caxpy.o temp.s
	rm -f temp.s

clean:
	rm -f *.o $(LIBAXPY)
