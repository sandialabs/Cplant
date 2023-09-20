include ../../../../../Makefile-common
#CC	= gcc
#CC	= cc
#CPP	= gcc -E
#CPP	= cc -E
CPP	= $(CC) -E
LNS	= ln -fs
#F77	= g77
#F77	= f77

OBJS	= dcopy.o scopy.o zcopy.o ccopy.o

LIBCOPY	= libcopy.a

all : $(LIBCOPY)

$(LIBCOPY):$(OBJS)
	rm -f  $(LIBCOPY)
	ar cq  $(LIBCOPY) $(OBJS)
	ranlib $(LIBCOPY)

dcopy.o : copy.S
	rm -f temp.s
	$(CPP) -DDCOPY copy.S > temp.s
	$(AS) -o dcopy.o temp.s
	rm -f temp.s

scopy.o : copy.S
	rm -f temp.s
	$(CPP) -DSCOPY copy.S > temp.s
	$(AS) -o scopy.o temp.s
	rm -f temp.s

zcopy.o : copy.S
	rm -f temp.s
	$(CPP) -DZCOPY copy.S > temp.s
	$(AS) -o zcopy.o temp.s
	rm -f temp.s

ccopy.o : copy.S
	rm -f temp.s
	$(CPP) -DCCOPY copy.S > temp.s
	$(AS) -o ccopy.o temp.s
	rm -f temp.s

clean:
	rm -f *.o $(LIBCOPY)
