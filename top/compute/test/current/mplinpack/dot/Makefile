#CC	= gcc
#CC	= cc
#CPP	= gcc -E
#CPP	= cc -E
CPP	= $(CC) -E
LNS	= ln -fs

DOT	= ddot.o sdot.o dsdot.o zdotu.o zdotc.o cdotu.o cdotc.o
LIBDOT	= libdot.a

all: $(LIBDOT)

$(LIBDOT) : $(DOT)
	rm -f $(LIBDOT)
	ar cq $(LIBDOT) $(DOT)

ddot.o:dot.S
	rm -f temp.s
	$(CPP) -DDDOT dot.S > temp.s
	$(CC)  -c temp.s -o ddot.o
	rm -f temp.s

sdot.o:dot.S
	rm -f temp.s
	$(CPP) -DSDOT dot.S > temp.s
	$(CC)  -c temp.s -o sdot.o
	rm -f temp.s

dsdot.o:dot.S
	rm -f temp.s
	$(CPP) -DDSDOT dot.S > temp.s
	$(CC)  -c temp.s -o dsdot.o
	rm -f temp.s

zdotu.o:zdot.S
	rm -f temp.s
	$(CPP) -DZDOT -UCONJ zdot.S > temp.s
	$(CC)  -c temp.s -o zdotu.o
	rm -f temp.s

zdotc.o:zdot.S
	rm -f temp.s
	$(CPP) -DZDOT -DCONJ zdot.S > temp.s
	$(CC)  -c temp.s -o zdotc.o
	rm -f temp.s

cdotu.o:zdot.S
	rm -f temp.s
	$(CPP) -UZDOT -UCONJ zdot.S > temp.s
	$(CC)  -c temp.s -o cdotu.o
	rm -f temp.s

cdotc.o:zdot.S
	rm -f temp.s
	$(CPP) -UZDOT -DCONJ zdot.S > temp.s
	$(CC)  -c temp.s -o cdotc.o
	rm -f temp.s

clean:
	rm -f *.o $(LIBDOT)

