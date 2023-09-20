# Generated automatically from Makefile.in by configure.

include ../../../../../Makefile-common

ARCH        = LINUX_ALPHA
RANLIB      = ranlib
FILE_SYSTEM = nfs
MAKE        = make
AR          = ar crl
LIBNAME     = ${CPLANT_TOP}/compute/lib/mpich-1.2.0/alpha-linux/libmpi.a
NOF77       = 0
NOPROFILE   = 1
MPI         = mpich
PREFIX      = 
FROM_MPICH  = 1
BUILD_MPI_INFO = 
BUILD_MPI_ARRAY = 
DIRS        = mpi-io adio/common 
MPIO_DIRS   = mpi-io
ALL_DIRS    = mpi-io mpi-io/fortran mpi2-other/info mpi2-other/info/fortran mpi2-other/array mpi2-other/array/fortran adio/common adio/ad_pfs adio/ad_piofs adio/ad_nfs adio/ad_ufs adio/ad_xfs adio/ad_hfs adio/ad_sfs test 
SHELL       = /bin/sh

VPATH=.:$(srcdir)

all: mpio

mpio:
	$(MAKE) clean
	$(MAKE) mpiolib
	$(MAKE) clean
	@if test "$(NOPROFILE)" = 0 ; then\
            $(MAKE) profile; \
        fi;\
	$(MAKE) clean
	$(RANLIB) $(LIBNAME)
	@if test "$(FROM_MPICH)" = 0 ; then \
           echo " "; \
           echo "Completed build of ROMIO. Check the output for errors."; \
           echo " " ;\
           echo "To compile the example programs, cd to the test directory"; \
           echo "and type make."; \
           echo "Run the examples the way you would run any MPI program."; \
           echo "Each program takes the filename as a command-line argument"; \
           echo "'-fname filename'." ; \
           echo " " ;\
        fi

clean:
	@dirs=`echo "$(DIRS)"`; \
        for x in $(FILE_SYSTEM) ;\
         do \
             dirs=`echo "$$dirs" adio/ad_$$x` ;\
         done ; \
        if test "$(BUILD_MPI_INFO)" = 1 ; then \
            dirs=`echo "$$dirs" mpi2-other/info` ;\
            if test "$(NOF77)" = 0 ; then\
                dirs=`echo "$$dirs" mpi2-other/info/fortran` ;\
            fi;\
        fi; \
        if test "$(BUILD_MPI_ARRAY)" = 1 ; then \
            dirs=`echo "$$dirs" mpi2-other/array` ;\
            if test "$(NOF77)" = 0 ; then\
                dirs=`echo "$$dirs" mpi2-other/array/fortran` ;\
            fi;\
        fi; \
        if test "$(NOF77)" = 0 ; then\
            dirs=`echo "$$dirs" mpi-io/fortran` ;\
        fi;\
        for dir in $$dirs ;\
         do \
            echo " " ;\
            echo cleaning directory $$dir ;\
            (cd $$dir; rm -f *.o) ;\
         done; \
         echo " " ;\
         echo cleaning directory test ;\
         (cd test; $(MAKE) clean);\
         echo " ";

mpiolib:
	@if test "$(FROM_MPICH)" = 0 ; then \
            echo " " ;\
            echo "deleting previously built library if it exists" ;\
            rm -f $(LIBNAME) ;\
        fi; \
        dirs=`echo "$(DIRS)"`; \
        for x in $(FILE_SYSTEM) ;\
         do \
             dirs=`echo "$$dirs" adio/ad_$$x` ;\
         done ; \
        if test "$(BUILD_MPI_INFO)" = 1 ; then \
            dirs=`echo "$$dirs" mpi2-other/info` ;\
            if test "$(NOF77)" = 0 ; then \
                dirs=`echo "$$dirs" mpi2-other/info/fortran` ;\
            fi; \
        fi; \
        if test "$(BUILD_MPI_ARRAY)" = 1 ; then \
            dirs=`echo "$$dirs" mpi2-other/array` ;\
            if test "$(NOF77)" = 0 ; then \
                dirs=`echo "$$dirs" mpi2-other/array/fortran` ;\
            fi; \
        fi; \
        if test "$(NOF77)" = 0 ; then \
            dirs=`echo "$$dirs" mpi-io/fortran` ;\
        fi; \
        for dir in $$dirs ;\
         do \
            echo " " ;\
            echo compiling ROMIO in directory $$dir ;\
            (cd $$dir; $(MAKE) ;) ;\
            if [ $$? != 0 ] ; then \
                echo "Make failed in directory $$dir" ; exit 1 ;\
            fi ;\
         done

profile: 
	@if [ "1" = 0 ] ; then \
           dirs=`echo "$(MPIO_DIRS)"`; \
           if test "$(BUILD_MPI_INFO)" = 1 ; then \
               dirs=`echo "$$dirs" mpi2-other/info` ;\
               if test "$(NOF77)" = 0 ; then\
                   dirs=`echo "$$dirs" mpi2-other/info/fortran` ;\
               fi;\
           fi; \
           if test "$(BUILD_MPI_ARRAY)" = 1 ; then \
               dirs=`echo "$$dirs" mpi2-other/array` ;\
               if test "$(NOF77)" = 0 ; then\
                   dirs=`echo "$$dirs" mpi2-other/array/fortran` ;\
               fi;\
           fi; \
           if test "$(NOF77)" = 0 ; then\
               dirs=`echo "$$dirs" mpi-io/fortran` ;\
           fi;\
           for dir in $$dirs ;\
            do \
               echo " " ;\
               echo building profiling interface in directory $$dir ;\
               (cd $$dir; \
                $(MAKE) profile MPIOPROFILE="-D__MPIO_BUILD_PROFILING";) ;\
                if [ $$? != 0 ] ; then \
                    echo "Make failed in directory $$dir" ; exit 1 ;\
                fi ;\
            done \
	fi

cleanall:
	@dirs=`echo "$(ALL_DIRS)"`; \
        echo cleaning current directory ;\
        (rm -f config.status Makefile *~ *.log romio.tar*) ;\
        for dir in $$dirs ;\
         do \
            echo " " ;\
            echo cleaning directory $$dir ;\
            (cd $$dir; rm -f *.o Makefile *~) ;\
         done; \
         (cd test; rm -f .P* PI* misc.c large_file.c fcoll_test.f fmisc.f fperf.f mpif.h simple async coll_test coll_perf misc perf fcoll_test fperf fmisc file_info excl large_array large_file noncontig i_noncontig noncontig_coll atomicity *.out runtests); \
         echo " " ;\
         echo "cleaning directory include" ;\
         (cd include; rm -f *.h);\
         echo " " ;\
         echo "cleaning directory util" ;\
         (cd util; rm -f romioinstall *~)

install:
	$(MAKE) clean
	@./util/romioinstall -prefix=$(PREFIX)

romio_lflags:
	@echo ''

romio_liblist:
	@echo '-laio'

romio_tcflags:
	@echo ''

romio_tcppflags:
	@echo ''

romio_tfflags:
	@echo ''
