c NPROCS = 49 CLASS = B
c  
c  
c  This file is generated automatically by the setparams utility.
c  It sets the number of processors and the class of the NPB
c  in this directory. Do not modify it by hand.
c  
        integer maxcells, problem_size, niter_default
        parameter (maxcells=7, problem_size=102, niter_default=400)
        double precision dt_default
        parameter (dt_default = 0.001d0)
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
