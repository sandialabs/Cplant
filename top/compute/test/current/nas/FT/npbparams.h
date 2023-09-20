c NPROCS = 64 CLASS = A
c  
c  
c  This file is generated automatically by the setparams utility.
c  It sets the number of processors and the class of the NPB
c  in this directory. Do not modify it by hand.
c  
        integer nx, ny, nz, maxdim, niter_default, ntotal, np_min
        parameter (nx=256, ny=256, nz=128, maxdim=256)
        parameter (niter_default=6)
        parameter (ntotal=nx*ny*nz)
        parameter (np_min = 64)
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
