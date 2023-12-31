c NPROCS = 64 CLASS = A
c  
c  
c  This file is generated automatically by the setparams utility.
c  It sets the number of processors and the class of the NPB
c  in this directory. Do not modify it by hand.
c  

c number of nodes for which this version is compiled
        integer nnodes_compiled
        parameter (nnodes_compiled = 64)

c full problem size
        integer isiz01, isiz02, isiz03
        parameter (isiz01=64, isiz02=64, isiz03=64)

c sub-domain array size
        integer isiz1, isiz2, isiz3
        parameter (isiz1=8, isiz2=8, isiz3=isiz03)

c number of iterations and how often to print the norm
        integer itmax_default, inorm_default
        parameter (itmax_default=250, inorm_default=250)
        double precision dt_default
        parameter (dt_default = 2.0d0)
        logical  convertdouble
        parameter (convertdouble = .false.)
        character*11 compiletime
        parameter (compiletime='09 Oct 1998')
        character*3 npbversion
        parameter (npbversion='2.3')
        character*23 cs1
        parameter (cs1='/Net/mp/cplant/bin/f77 ')
        character*9 cs2
        parameter (cs2='$(MPIF77)')
        character*19 cs3
        parameter (cs3='-L$(MPIPATH) -lmpi ')
        character*20 cs4
        parameter (cs4='-I$(MPIPATH)/include')
        character*37 cs5
        parameter (cs5='-O3 -align records -align dcommons -g')
        character*2 cs6
        parameter (cs6='-v')
        character*6 cs7
        parameter (cs7='randi8')
