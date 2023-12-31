!-------------------------------------------------------------------------!
!                                                                         !
!        N  A  S     P A R A L L E L     B E N C H M A R K S  2.3         !
!                                                                         !
!                                   F T                                   !
!                                                                         !
!-------------------------------------------------------------------------!
!                                                                         !
!    This benchmark is part of the NAS Parallel Benchmark 2.3 suite.      !
!    It is described in NAS Technical Report 95-020.                      !
!                                                                         !
!    Permission to use, copy, distribute and modify this software         !
!    for any purpose with or without fee is hereby granted.  We           !
!    request, however, that all derived work reference the NAS            !
!    Parallel Benchmarks 2.3. This software is provided "as is"           !
!    without express or implied warranty.                                 !
!                                                                         !
!    Information on NPB 2.3, including the technical report, the          !
!    original specifications, source code, results and information        !
!    on how to submit new results, is available at:                       !
!                                                                         !
!           http://www.nas.nasa.gov/NAS/NPB/                              !
!                                                                         !
!    Send comments or suggestions to  npb@nas.nasa.gov                    !
!    Send bug reports to              npb-bugs@nas.nasa.gov               !
!                                                                         !
!          NAS Parallel Benchmarks Group                                  !
!          NASA Ames Research Center                                      !
!          Mail Stop: T27A-1                                              !
!          Moffett Field, CA   94035-1000                                 !
!                                                                         !
!          E-mail:  npb@nas.nasa.gov                                      !
!          Fax:     (415) 604-3957                                        !
!                                                                         !
!-------------------------------------------------------------------------!

c---------------------------------------------------------------------
c
c Authors: D. Bailey
c          W. Saphir
c
c---------------------------------------------------------------------

c---------------------------------------------------------------------

c---------------------------------------------------------------------
c FT benchmark
c---------------------------------------------------------------------

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      program ft

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none

      include 'mpif.h'
      include 'global.h'
      integer i, ierr
      
c---------------------------------------------------------------------
c u0, u1, u2 are the main arrays in the problem. 
c Depending on the decomposition, these arrays will have different 
c dimensions. To accomodate all possibilities, we allocate them as 
c one-dimensional arrays and pass them to subroutines for different 
c views
c  - u0 contains the initial (transformed) initial condition
c  - u1 and u2 are working arrays
c  - indexmap maps i,j,k of u0 to the correct i^2+j^2+k^2 for the
c    time evolution operator. 
c---------------------------------------------------------------------

      double complex u0(ntotal/np_min), 
     >               u1(ntotal/np_min), 
     >               u2(ntotal/np_min)
      integer indexmap(ntotal/np_min)
c---------------------------------------------------------------------
c Large arrays are in common so that they are allocated on the
c heap rather than the stack. This common block is not
c referenced directly anywhere else. Padding is to avoid accidental 
c cache problems, since all array sizes are powers of two.
c---------------------------------------------------------------------

      double complex pad1(3), pad2(3), pad3(3)
      common /bigarrays/ u0, pad1, u1, pad2, u2, pad3, indexmap

      integer iter
      double precision total_time, mflops
      logical verified
      character class

      call MPI_Init(ierr)

c---------------------------------------------------------------------
c Run the entire problem once to make sure all data is touched. 
c This reduces variable startup costs, which is important for such a 
c short benchmark. The other NPB 2 implementations are similar. 
c---------------------------------------------------------------------
      do i = 1, t_max
         call timer_clear(i)
      end do
      call setup()
      call compute_indexmap(indexmap, dims(1,3))
      call compute_initial_conditions(u1, dims(1,1))
      call fft_init (dims(1,1))
      call fft(1, u1, u0)

c---------------------------------------------------------------------
c Start over from the beginning. Note that all operations must
c be timed, in contrast to other benchmarks. 
c---------------------------------------------------------------------
      do i = 1, t_max
         call timer_clear(i)
      end do
      call MPI_Barrier(MPI_COMM_WORLD, ierr)

      call timer_start(T_total)
      if (timers_enabled) call timer_start(T_setup)

      call compute_indexmap(indexmap, dims(1,3))

      call compute_initial_conditions(u1, dims(1,1))

      call fft_init (dims(1,1))

      if (timers_enabled) call synchup()
      if (timers_enabled) call timer_stop(T_setup)

      if (timers_enabled) call timer_start(T_fft)
      call fft(1, u1, u0)
      if (timers_enabled) call timer_stop(T_fft)
      do iter = 1, niter
         if (timers_enabled) call timer_start(T_evolve)
         call evolve(u0, u1, iter, indexmap, dims(1, 1))
         if (timers_enabled) call timer_stop(T_evolve)
         if (timers_enabled) call timer_start(T_fft)
         call fft(-1, u1, u2)
         if (timers_enabled) call timer_stop(T_fft)
         if (timers_enabled) call synchup()
         if (timers_enabled) call timer_start(T_checksum)
         call checksum(iter, u2, dims(1,1))
         if (timers_enabled) call timer_stop(T_checksum)
      end do

      call verify(nx, ny, nz, niter, verified, class)
      call timer_stop(t_total)
      if (np .ne. np_min) verified = .false.
      total_time = timer_read(t_total)

      if( total_time .ne. 0. ) then
         mflops = 1.0d-6*float(ntotal) *
     >             (14.8157+7.19641*log(float(ntotal))
     >          +  (5.23518+7.21113*log(float(ntotal)))*niter)
     >                 /total_time
      else
         mflops = 0.0
      endif
      if (me .eq. 0) then
         call print_results('FT', class, nx, ny, nz, niter, np_min, np,
     >     total_time, mflops, '          floating point', verified, 
     >     npbversion, compiletime, cs1, cs2, cs3, cs4, cs5, cs6, cs7)
      endif
      if (timers_enabled) call print_timers()
      call MPI_Finalize(ierr)
      end

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine evolve(u0, u1, t, indexmap, d)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

c---------------------------------------------------------------------
c evolve u0 -> u1 (t time steps) in fourier space
c---------------------------------------------------------------------

      implicit none
      include 'global.h'
      integer d(3)
      double complex u0(d(1),d(2),d(3))
      double complex u1(d(1),d(2),d(3))
      integer  indexmap(d(1),d(2),d(3))
      integer t
      integer i, j, k

      do k = 1, d(3)
         do j = 1, d(2)
            do i = 1, d(1)
               u1(i,j,k) = u0(i,j,k)*ex(t*indexmap(i,j,k))
            end do
         end do
      end do

      return
      end


c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine compute_initial_conditions(u0, d)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

c---------------------------------------------------------------------
c Fill in array u0 with initial conditions from 
c random number generator 
c---------------------------------------------------------------------
      implicit none
      include 'global.h'
      integer d(3)
      double complex u0(d(1), d(2), d(3))
      integer k
      double precision x0, start, an, dummy
      
c---------------------------------------------------------------------
c 0-D and 1-D layouts are easy because each processor gets a contiguous
c chunk of the array, in the Fortran ordering sense. 
c For a 2-D layout, it's a bit more complicated. We always
c have entire x-lines (contiguous) in processor. 
c We can do ny/np1 of them at a time since we have
c ny/np1 contiguous in y-direction. But then we jump
c by z-planes (nz/np2 of them, total). 
c For the 0-D and 1-D layouts we could do larger chunks, but
c this turns out to have no measurable impact on performance. 
c---------------------------------------------------------------------


      start = seed                                    
c---------------------------------------------------------------------
c Jump to the starting element for our first plane.
c---------------------------------------------------------------------
      call ipow46(a, (zstart(1)-1)*2*nx*ny + (ystart(1)-1)*2*nx, an)
      dummy = randlc(start, an)
      call ipow46(a, 2*nx*ny, an)
      
c---------------------------------------------------------------------
c Go through by z planes filling in one square at a time.
c---------------------------------------------------------------------
      do k = 1, dims(3, 1) ! nz/np2
         x0 = start
         call vranlc(2*nx*dims(2, 1), x0, a, u0(1, 1, k))
         if (k .ne. dims(3, 1)) dummy = randlc(start, an)
      end do
      return
      end

	            
c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine ipow46(a, exponent, result)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

c---------------------------------------------------------------------
c compute a^exponent mod 2^46
c---------------------------------------------------------------------

      implicit none
      double precision a, result, dummy, q, r
      integer exponent, n, n2
      external randlc
      double precision randlc
c---------------------------------------------------------------------
c Use
c   a^n = a^(n/2)*a^(n/2) if n even else
c   a^n = a*a^(n-1)       if n odd
c---------------------------------------------------------------------
      result = 1
      if (exponent .eq. 0) return
      q = a
      r = 1
      n = exponent


      do while (n .gt. 1)
         n2 = n/2
         if (n2 * 2 .eq. n) then
            dummy = randlc(q, q) 
            n = n2
         else
            dummy = randlc(r, q)
            n = n-1
         endif
      end do
      dummy = randlc(r, q)
      result = r
      return
      end


c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine setup

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none
      include 'mpinpb.h'
      include 'global.h'

      integer ierr, i, j, fstatus
      debug = .FALSE.
      
      call MPI_Comm_size(MPI_COMM_WORLD, np, ierr)
      call MPI_Comm_rank(MPI_COMM_WORLD, me, ierr)

      if (.not. convertdouble) then
         dc_type = MPI_DOUBLE_COMPLEX
      else
         dc_type = MPI_COMPLEX
      endif


      if (me .eq. 0) then
         write(*, 1000)
         open (unit=2,file='inputft.data',status='old', iostat=fstatus)

         if (fstatus .eq. 0) then
            write(*,233) 
 233        format(' Reading from input file inputft.data')
            read (2,*) niter
            read (2,*) layout_type
            read (2,*) np1, np2
            close(2)

c---------------------------------------------------------------------
c check to make sure input data is consistent
c---------------------------------------------------------------------

    
c---------------------------------------------------------------------
c 1. product of processor grid dims must equal number of processors
c---------------------------------------------------------------------

            if (np1 * np2 .ne. np) then
               write(*, 238)
 238           format(' np1 and np2 given in input file are not valid.')
               write(*, 239) np1*np2, np
 239           format(' Product is ', i4, ' and should be ', i4)
               call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
            endif

c---------------------------------------------------------------------
c 2. layout type must be valid
c---------------------------------------------------------------------

            if (layout_type .ne. layout_0D .and.
     >          layout_type .ne. layout_1D .and.
     >          layout_type .ne. layout_2D) then
               write(*, 240)
 240           format(' Layout type specified in inputft.data is 
     >                  invalid ')
               call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
            endif

c---------------------------------------------------------------------
c 3. 0D layout must be 1x1 grid
c---------------------------------------------------------------------

            if (layout_type .eq. layout_0D .and.
     >            (np1 .ne.1 .or. np2 .ne. 1)) then
               write(*, 241)
 241           format(' For 0D layout, both np1 and np2 must be 1 ')
               call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
            endif
c---------------------------------------------------------------------
c 4. 1D layout must be 1xN grid
c---------------------------------------------------------------------

            if (layout_type .eq. layout_1D .and. np1 .ne. 1) then
               write(*, 242)
 242           format(' For 1D layout, np1 must be 1 ')
               call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
            endif

         else
            write(*,234) 
            niter = niter_default
            if (np .eq. 1) then
               np1 = 1
               np2 = 1
               layout_type = layout_0D
            else if (np .le. nz) then
               np1 = 1
               np2 = np
               layout_type = layout_1D
            else
               np1 = nz
               np2 = np/nz
               layout_type = layout_2D
            endif
         endif

         if (np .lt. np_min) then
            write(*, 10) np_min
 10         format(' Error: Compiled for ', I4, ' processors. ')
            write(*, 11) np
 11         format(' Only ',  i4, ' processors found ')
            call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         endif

 234     format(' No input file inputft.data. Using compiled defaults')
         write(*, 1001) nx, ny, nz
         write(*, 1002) niter
         write(*, 1004) np
         write(*, 1005) np1, np2
         if (np .ne. np_min) write(*, 1006) np_min

         if (layout_type .eq. layout_0D) then
            write(*, 1010) '0D'
         else if (layout_type .eq. layout_1D) then
            write(*, 1010) '1D'
         else
            write(*, 1010) '2D'
         endif

 1000 format(//,' NAS Parallel Benchmarks 2.3 -- FT Benchmark',/)
 1001    format(' Size                : ', i3, 'x', i3, 'x', i3)
 1002    format(' Iterations          :     ', i7)
 1004    format(' Number of processes :     ', i7)
 1005    format(' Processor array     :     ', i3, 'x', i3)
 1006    format(' WARNING: compiled for ', i5, ' processes. ',
     >          ' Will not verify. ')
 1010    format(' Layout type         :       ', A5)
      endif


c---------------------------------------------------------------------
c Since np1, np2 and layout_type are in a common block, 
c this sends all three. 
c---------------------------------------------------------------------
      call MPI_BCAST(np1, 3, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
      call MPI_BCAST(niter, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)

      if (np1 .eq. 1 .and. np2 .eq. 1) then
        layout_type = layout_0D
      else if (np1 .eq. 1) then
         layout_type = layout_1D
      else
         layout_type = layout_2D
      endif

      if (layout_type .eq. layout_0D) then
         do i = 1, 3
            dims(1, i) = nx
            dims(2, i) = ny
            dims(3, i) = nz
         end do
      else if (layout_type .eq. layout_1D) then
         dims(1, 1) = nx
         dims(2, 1) = ny
         dims(3, 1) = nz

         dims(1, 2) = nx
         dims(2, 2) = ny
         dims(3, 2) = nz

         dims(1, 3) = nz
         dims(2, 3) = nx
         dims(3, 3) = ny
      else if (layout_type .eq. layout_2D) then
         dims(1, 1) = nx
         dims(2, 1) = ny
         dims(3, 1) = nz

         dims(1, 2) = ny
         dims(2, 2) = nx
         dims(3, 2) = nz

         dims(1, 3) = nz
         dims(2, 3) = nx
         dims(3, 3) = ny

      endif
      do i = 1, 3
         dims(2, i) = dims(2, i) / np1
         dims(3, i) = dims(3, i) / np2
      end do


c---------------------------------------------------------------------
c Determine processor coordinates of this processor
c Processor grid is np1xnp2. 
c Arrays are always (n1, n2/np1, n3/np2)
c Processor coords are zero-based. 
c---------------------------------------------------------------------
      me2 = mod(me, np2)  ! goes from 0...np2-1
      me1 = me/np2        ! goes from 0...np1-1
c---------------------------------------------------------------------
c Communicators for rows/columns of processor grid. 
c commslice1 is communicator of all procs with same me1, ranked as me2
c commslice2 is communicator of all procs with same me2, ranked as me1
c mpi_comm_split(comm, color, key, ...)
c---------------------------------------------------------------------
      call MPI_Comm_split(MPI_COMM_WORLD, me1, me2, commslice1, ierr)
      call MPI_Comm_split(MPI_COMM_WORLD, me2, me1, commslice2, ierr)
      if (timers_enabled) call synchup()

      if (debug) print *, 'proc coords: ', me, me1, me2

c---------------------------------------------------------------------
c Determine which section of the grid is owned by this
c processor. 
c---------------------------------------------------------------------
      if (layout_type .eq. layout_0d) then

         do i = 1, 3
            xstart(i) = 1
            xend(i)   = nx
            ystart(i) = 1
            yend(i)   = ny
            zstart(i) = 1
            zend(i)   = nz
         end do

      else if (layout_type .eq. layout_1d) then

         xstart(1) = 1
         xend(1)   = nx
         ystart(1) = 1
         yend(1)   = ny
         zstart(1) = 1 + me2 * nz/np2
         zend(1)   = (me2+1) * nz/np2

         xstart(2) = 1
         xend(2)   = nx
         ystart(2) = 1
         yend(2)   = ny
         zstart(2) = 1 + me2 * nz/np2
         zend(2)   = (me2+1) * nz/np2

         xstart(3) = 1
         xend(3)   = nx
         ystart(3) = 1 + me2 * ny/np2
         yend(3)   = (me2+1) * ny/np2
         zstart(3) = 1
         zend(3)   = nz

      else if (layout_type .eq. layout_2d) then

         xstart(1) = 1
         xend(1)   = nx
         ystart(1) = 1 + me1 * ny/np1
         yend(1)   = (me1+1) * ny/np1
         zstart(1) = 1 + me2 * nz/np2
         zend(1)   = (me2+1) * nz/np2

         xstart(2) = 1 + me1 * nx/np1
         xend(2)   = (me1+1)*nx/np1
         ystart(2) = 1
         yend(2)   = ny
         zstart(2) = zstart(1)
         zend(2)   = zend(1)

         xstart(3) = xstart(2)
         xend(3)   = xend(2)
         ystart(3) = 1 + me2 *ny/np2
         yend(3)   = (me2+1)*ny/np2
         zstart(3) = 1
         zend(3)   = nz
      endif

c---------------------------------------------------------------------
c Set up info for blocking of ffts and transposes.  This improves
c performance on cache-based systems. Blocking involves
c working on a chunk of the problem at a time, taking chunks
c along the first, second, or third dimension. 
c
c - In cffts1 blocking is on 2nd dimension (with fft on 1st dim)
c - In cffts2/3 blocking is on 1st dimension (with fft on 2nd and 3rd dims)

c Since 1st dim is always in processor, we'll assume it's long enough 
c (default blocking factor is 16 so min size for 1st dim is 16)
c The only case we have to worry about is cffts1 in a 2d decomposition. 
c so the blocking factor should not be larger than the 2nd dimension. 
c---------------------------------------------------------------------

      fftblock = fftblock_default
      fftblockpad = fftblockpad_default

      if (layout_type .eq. layout_2d) then
         if (dims(2, 1) .lt. fftblock) fftblock = dims(2, 1)
         if (dims(2, 2) .lt. fftblock) fftblock = dims(2, 2)
         if (dims(2, 3) .lt. fftblock) fftblock = dims(2, 3)
      endif
      
      if (fftblock .ne. fftblock_default) fftblockpad = fftblock+3

      return
      end

      
c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine compute_indexmap(indexmap, d)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

c---------------------------------------------------------------------
c compute function from local (i,j,k) to ibar^2+jbar^2+kbar^2 
c for time evolution exponent. 
c---------------------------------------------------------------------

      implicit none
      include 'mpinpb.h'
      include 'global.h'
      integer d(3)
      integer indexmap(d(1), d(2), d(3))
      integer i, j, k, ii, ii2, jj, ij2, kk
      double precision ap

c---------------------------------------------------------------------
c this function is very different depending on whether 
c we are in the 0d, 1d or 2d layout. Compute separately. 
c basically we want to convert the fortran indices 
c   1 2 3 4 5 6 7 8 
c to 
c   0 1 2 3 -4 -3 -2 -1
c The following magic formula does the trick:
c mod(i-1+n/2, n) - n/2
c---------------------------------------------------------------------

      if (layout_type .eq. layout_0d) then ! xyz layout
         do i = 1, dims(1,3)
            ii =  mod(i+xstart(3)-2+nx/2, nx) - nx/2
            ii2 = ii*ii
            do j = 1, dims(2,3)
               jj = mod(j+ystart(3)-2+ny/2, ny) - ny/2
               ij2 = jj*jj+ii2
               do k = 1, dims(3,3)
                  kk = mod(k+zstart(3)-2+nz/2, nz) - nz/2
                  indexmap(i, j, k) = kk*kk+ij2
               end do
            end do
         end do
      else if (layout_type .eq. layout_1d) then ! zxy layout 
         do i = 1,dims(2,3)
            ii =  mod(i+xstart(3)-2+nx/2, nx) - nx/2
            ii2 = ii*ii
            do j = 1,dims(3,3)
               jj = mod(j+ystart(3)-2+ny/2, ny) - ny/2
               ij2 = jj*jj+ii2
               do k = 1,dims(1,3)
                  kk = mod(k+zstart(3)-2+nz/2, nz) - nz/2
                  indexmap(k, i, j) = kk*kk+ij2
               end do
            end do
         end do
      else if (layout_type .eq. layout_2d) then ! zxy layout
         do i = 1,dims(2,3)
            ii =  mod(i+xstart(3)-2+nx/2, nx) - nx/2
            ii2 = ii*ii
            do j = 1, dims(3,3)
               jj = mod(j+ystart(3)-2+ny/2, ny) - ny/2
               ij2 = jj*jj+ii2
               do k =1,dims(1,3)
                  kk = mod(k+zstart(3)-2+nz/2, nz) - nz/2
                  indexmap(k, i, j) = kk*kk+ij2
               end do
            end do
         end do
      else
         print *, ' Unknown layout type ', layout_type
         stop
      endif

c---------------------------------------------------------------------
c compute array of exponentials for time evolution. 
c---------------------------------------------------------------------
      ap = - 4.d0 * alpha * pi *pi

      ex(0) = 1.0d0
      ex(1) = exp(ap)
      do i = 2, expmax
         ex(i) = ex(i-1)*ex(1)
      end do

      return
      end



c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine print_timers()

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none
      integer i
      include 'global.h'
      character*25 tstrings(T_max)
      data tstrings / '          total ', 
     >                '          setup ', 
     >                '            fft ', 
     >                '         evolve ', 
     >                '       checksum ', 
     >                '         fftlow ', 
     >                '        fftcopy ', 
     >                '      transpose ', 
     >                ' transpose1_loc ', 
     >                ' transpose1_glo ', 
     >                ' transpose1_fin ', 
     >                ' transpose2_loc ', 
     >                ' transpose2_glo ', 
     >                ' transpose2_fin ', 
     >                '           sync ' /

      if (me .ne. 0) return
      do i = 1, t_max
         if (timer_read(i) .ne. 0.0d0) then
            write(*, 100) i, tstrings(i), timer_read(i)
         endif
      end do
 100  format(' timer ', i2, '(', A16,  ') :', F10.6)
      return
      end



c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine fft(dir, x1, x2)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none
      include 'global.h'
      integer dir
      double complex x1(ntotal/np), x2(ntotal/np)

      double complex scratch(fftblockpad_default*maxdim*2)

c---------------------------------------------------------------------
c note: args x1, x2 must be different arrays
c note: args for cfftsx are (direction, layout, xin, xout, scratch)
c       xin/xout may be the same and it can be somewhat faster
c       if they are
c note: args for transpose are (layout1, layout2, xin, xout)
c       xin/xout must be different
c---------------------------------------------------------------------

      if (dir .eq. 1) then
         if (layout_type .eq. layout_0d) then
            call cffts1(1, dims(1,1), x1, x1, scratch)
            call cffts2(1, dims(1,2), x1, x1, scratch)
            call cffts3(1, dims(1,3), x1, x2, scratch)
         else if (layout_type .eq. layout_1d) then
            call cffts1(1, dims(1,1), x1, x1, scratch)
            call cffts2(1, dims(1,2), x1, x1, scratch)
            if (timers_enabled) call timer_start(T_transpose)
            call transpose_xy_z(2, 3, x1, x2)
            if (timers_enabled) call timer_stop(T_transpose)
            call cffts1(1, dims(1,3), x2, x2, scratch)
         else if (layout_type .eq. layout_2d) then
            call cffts1(1, dims(1,1), x1, x1, scratch)
            if (timers_enabled) call timer_start(T_transpose)
            call transpose_x_y(1, 2, x1, x2)
            if (timers_enabled) call timer_stop(T_transpose)
            call cffts1(1, dims(1,2), x2, x2, scratch)
            if (timers_enabled) call timer_start(T_transpose)
            call transpose_x_z(2, 3, x2, x1)
            if (timers_enabled) call timer_stop(T_transpose)
            call cffts1(1, dims(1,3), x1, x2, scratch)
         endif
      else
         if (layout_type .eq. layout_0d) then
            call cffts3(-1, dims(1,3), x1, x1, scratch)
            call cffts2(-1, dims(1,2), x1, x1, scratch)
            call cffts1(-1, dims(1,1), x1, x2, scratch)
         else if (layout_type .eq. layout_1d) then
            call cffts1(-1, dims(1,3), x1, x1, scratch)
            if (timers_enabled) call timer_start(T_transpose)
            call transpose_x_yz(3, 2, x1, x2)
            if (timers_enabled) call timer_stop(T_transpose)
            call cffts2(-1, dims(1,2), x2, x2, scratch)
            call cffts1(-1, dims(1,1), x2, x2, scratch)
         else if (layout_type .eq. layout_2d) then
            call cffts1(-1, dims(1,3), x1, x1, scratch)
            if (timers_enabled) call timer_start(T_transpose)
            call transpose_x_z(3, 2, x1, x2)
            if (timers_enabled) call timer_stop(T_transpose)
            call cffts1(-1, dims(1,2), x2, x2, scratch)
            if (timers_enabled) call timer_start(T_transpose)
            call transpose_x_y(2, 1, x2, x1)
            if (timers_enabled) call timer_stop(T_transpose)
            call cffts1(-1, dims(1,1), x1, x2, scratch)
         endif
      endif
      return
      end



c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine cffts1(is, d, x, xout, y)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none

      include 'global.h'
      integer is, d(3), logd(3)
      double complex x(d(1),d(2),d(3))
      double complex xout(d(1),d(2),d(3))
      double complex y(fftblockpad, d(1), 2) 
      integer i, j, k, jj

      do i = 1, 3
         logd(i) = ilog2(d(i))
      end do
      do k = 1, d(3)
         do jj = 0, d(2) - fftblock, fftblock
            if (timers_enabled) call timer_start(T_fftcopy)
            do j = 1, fftblock
               do i = 1, d(1)
                  y(j,i,1) = x(i,j+jj,k)
               enddo
            enddo
            if (timers_enabled) call timer_stop(T_fftcopy)
            
            if (timers_enabled) call timer_start(T_fftlow)
            call cfftz (is, logd(1), 
     >                  d(1), y, y(1,1,2))

            if (timers_enabled) call timer_stop(T_fftlow)

            if (timers_enabled) call timer_start(T_fftcopy)
            do j = 1, fftblock
               do i = 1, d(1)
                  xout(i,j+jj,k) = y(j,i,1)
               enddo
            enddo
            if (timers_enabled) call timer_stop(T_fftcopy)
         enddo
      enddo

      return
      end


c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine cffts2(is, d, x, xout, y)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none

      include 'global.h'
      integer is, d(3), logd(3)
      double complex x(d(1),d(2),d(3))
      double complex xout(d(1),d(2),d(3))
      double complex y(fftblockpad, d(2), 2) 
      integer i, j, k, ii

      do i = 1, 3
         logd(i) = ilog2(d(i))
      end do
      do k = 1, d(3)
        do ii = 0, d(1) - fftblock, fftblock
           if (timers_enabled) call timer_start(T_fftcopy)
           do j = 1, d(2)
              do i = 1, fftblock
                 y(i,j,1) = x(i+ii,j,k)
              enddo
           enddo
           if (timers_enabled) call timer_stop(T_fftcopy)

           if (timers_enabled) call timer_start(T_fftlow)
           call cfftz (is, logd(2), 
     >          d(2), y, y(1, 1, 2))
           
           if (timers_enabled) call timer_stop(T_fftlow)
           if (timers_enabled) call timer_start(T_fftcopy)
           do j = 1, d(2)
              do i = 1, fftblock
                 xout(i+ii,j,k) = y(i,j,1)
              enddo
           enddo
           if (timers_enabled) call timer_stop(T_fftcopy)
        enddo
      enddo

      return
      end


c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine cffts3(is, d, x, xout, y)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none

      include 'global.h'
      integer is, d(3), logd(3)
      double complex x(d(1),d(2),d(3))
      double complex xout(d(1),d(2),d(3))
      double complex y(fftblockpad, d(3), 2) 
      integer i, j, k, ii

      do i = 1, 3
         logd(i) = ilog2(d(i))
      end do
      do j = 1, d(2)
        do ii = 0, d(1) - fftblock, fftblock
           if (timers_enabled) call timer_start(T_fftcopy)
           do k = 1, d(3)
              do i = 1, fftblock
                 y(i,k,1) = x(i+ii,j,k)
              enddo
           enddo
           if (timers_enabled) call timer_stop(T_fftcopy)

           if (timers_enabled) call timer_start(T_fftlow)
           call cfftz (is, logd(3), 
     >          d(3), y, y(1, 1, 2))
           if (timers_enabled) call timer_stop(T_fftlow)
           if (timers_enabled) call timer_start(T_fftcopy)
           do k = 1, d(3)
              do i = 1, fftblock
                 xout(i+ii,j,k) = y(i,k,1)
              enddo
           enddo
           if (timers_enabled) call timer_stop(T_fftcopy)
        enddo
      enddo

      return
      end


c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine fft_init (n)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

c---------------------------------------------------------------------
c compute the roots-of-unity array that will be used for subsequent FFTs. 
c---------------------------------------------------------------------

      implicit none
      include 'global.h'

      integer m,n,nu,ku,i,j,ln
      double precision t, ti


c---------------------------------------------------------------------
c   Initialize the U array with sines and cosines in a manner that permits
c   stride one access at each FFT iteration.
c---------------------------------------------------------------------
      nu = n
      m = ilog2(n)
      u(1) = m
      ku = 2
      ln = 1

      do j = 1, m
         t = pi / ln
         
         do i = 0, ln - 1
            ti = i * t
            u(i+ku) = dcmplx (cos (ti), sin(ti))
         enddo
         
         ku = ku + ln
         ln = 2 * ln
      enddo
      
      return
      end

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine cfftz (is, m, n, x, y)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

c---------------------------------------------------------------------
c   Computes NY N-point complex-to-complex FFTs of X using an algorithm due
c   to Swarztrauber.  X is both the input and the output array, while Y is a 
c   scratch array.  It is assumed that N = 2^M.  Before calling CFFTZ to 
c   perform FFTs, the array U must be initialized by calling CFFTZ with IS 
c   set to 0 and M set to MX, where MX is the maximum value of M for any 
c   subsequent call.
c---------------------------------------------------------------------

      implicit none
      include 'global.h'

      integer is,m,n,i,j,l,mx
      double complex x, y

      dimension x(fftblockpad,n), y(fftblockpad,n)

c---------------------------------------------------------------------
c   Check if input parameters are invalid.
c---------------------------------------------------------------------
      mx = u(1)
      if ((is .ne. 1 .and. is .ne. -1) .or. m .lt. 1 .or. m .gt. mx)    
     >  then
        write (*, 1)  is, m, mx
 1      format ('CFFTZ: Either U has not been initialized, or else'/    
     >    'one of the input parameters is invalid', 3I5)
        stop
      endif

c---------------------------------------------------------------------
c   Perform one variant of the Stockham FFT.
c---------------------------------------------------------------------
      do l = 1, m, 2
        call fftz2 (is, l, m, n, fftblock, fftblockpad, u, x, y)
        if (l .eq. m) goto 160
        call fftz2 (is, l + 1, m, n, fftblock, fftblockpad, u, y, x)
      enddo

      goto 180

c---------------------------------------------------------------------
c   Copy Y to X.
c---------------------------------------------------------------------
 160  do j = 1, n
        do i = 1, fftblock
          x(i,j) = y(i,j)
        enddo
      enddo

 180  continue

      return
      end

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine fftz2 (is, l, m, n, ny, ny1, u, x, y)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

c---------------------------------------------------------------------
c   Performs the L-th iteration of the second variant of the Stockham FFT.
c---------------------------------------------------------------------

      implicit none
      
      integer me

      integer is,k,l,m,n,ny,ny1,n1,li,lj,lk,ku,i,j,i11,i12,i21,i22
      double complex u,x,y,u1,x11,x21
      dimension u(n), x(ny1,n), y(ny1,n)

c---------------------------------------------------------------------
c   Set initial parameters.
c---------------------------------------------------------------------

      n1 = n / 2
      lk = 2 ** (l - 1)
      li = 2 ** (m - l)
      lj = 2 * lk
      ku = li + 1

      do i = 0, li - 1
        i11 = i * lk + 1
        i12 = i11 + n1
        i21 = i * lj + 1
        i22 = i21 + lk
        if (is .ge. 1) then
          u1 = u(ku+i)
        else
          u1 = dconjg (u(ku+i))
        endif

c---------------------------------------------------------------------
c   This loop is vectorizable.
c---------------------------------------------------------------------
        do k = 0, lk - 1
          do j = 1, ny
            x11 = x(j,i11+k)
            x21 = x(j,i12+k)
            y(j,i21+k) = x11 + x21
            y(j,i22+k) = u1 * (x11 - x21)
          enddo
        enddo
      enddo

      return
      end

c---------------------------------------------------------------------


c---------------------------------------------------------------------
c---------------------------------------------------------------------

      integer function ilog2(n)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none
      integer n, nn, lg
      if (n .eq. 1) then
         ilog2=0
         return
      endif
      lg = 1
      nn = 2
      do while (nn .lt. n)
         nn = nn*2
         lg = lg+1
      end do
      ilog2 = lg
      return
      end


c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine transpose_x_yz(l1, l2, xin, xout)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none
      include 'global.h'
      integer l1, l2
      double complex xin(ntotal/np), xout(ntotal/np)

      call transpose2_local(dims(1,l1),dims(2, l1)*dims(3, l1),
     >                          xin, xout)
      call transpose2_global(xout, xin)
      call transpose2_finish(dims(1,l1),dims(2, l1)*dims(3, l1),
     >                          xin, xout)

      return
      end


c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine transpose_xy_z(l1, l2, xin, xout)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none
      include 'global.h'
      integer l1, l2
      double complex xin(ntotal/np), xout(ntotal/np)

      call transpose2_local(dims(1,l1)*dims(2, l1),dims(3, l1),
     >                          xin, xout)
      call transpose2_global(xout, xin)
      call transpose2_finish(dims(1,l1)*dims(2, l1),dims(3, l1),
     >                          xin, xout)

      return
      end



c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine transpose2_local(n1, n2, xin, xout)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none
      include 'mpinpb.h'
      include 'global.h'
      integer n1, n2
      double complex xin(n1, n2), xout(n2, n1)
      
      double complex z(transblockpad, transblock)

      integer i, j, ii, jj

      if (timers_enabled) call timer_start(T_transxzloc)

c---------------------------------------------------------------------
c If possible, block the transpose for cache memory systems. 
c How much does this help? Example: R8000 Power Challenge (90 MHz)
c Blocked version decreases time spend in this routine 
c from 14 seconds to 5.2 seconds on 8 nodes class A.
c---------------------------------------------------------------------

      if (n1 .lt. transblock .or. n2 .lt. transblock) then
         if (n1 .ge. n2) then 
            do j = 1, n2
               do i = 1, n1
                  xout(j, i) = xin(i, j)
               end do
            end do
         else
            do i = 1, n1
               do j = 1, n2
                  xout(j, i) = xin(i, j)
               end do
            end do
         endif
      else
         do j = 0, n2-1, transblock
            do i = 0, n1-1, transblock
               
c---------------------------------------------------------------------
c Note: compiler should be able to take j+jj out of inner loop
c---------------------------------------------------------------------
               do jj = 1, transblock
                  do ii = 1, transblock
                     z(jj,ii) = xin(i+ii, j+jj)
                  end do
               end do
               
               do ii = 1, transblock
                  do jj = 1, transblock
                     xout(j+jj, i+ii) = z(jj,ii)
                  end do
               end do
               
            end do
         end do
      endif
      if (timers_enabled) call timer_stop(T_transxzloc)

      return
      end


c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine transpose2_global(xin, xout)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none
      include 'global.h'
      include 'mpinpb.h'
      double complex xin(ntotal/np)
      double complex xout(ntotal/np) 
      integer ierr
c     call svRec(me,7,0,0,0)

      if (timers_enabled) call synchup()

      if (timers_enabled) call timer_start(T_transxzglo)
      call mpi_alltoall(xin, ntotal/(np*np), dc_type,
     >                  xout, ntotal/(np*np), dc_type,
     >                  commslice1, ierr)
      if (timers_enabled) call timer_stop(T_transxzglo)
      return
      end



c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine transpose2_finish(n1, n2, xin, xout)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none
      include 'global.h'
      integer n1, n2, ioff
      double complex xin(n2, n1/np2, 0:np2-1), xout(n2*np2, n1/np2)
      
      integer i, j, p

      if (timers_enabled) call timer_start(T_transxzfin)
      do p = 0, np2-1
         ioff = p*n2
         do j = 1, n1/np2
            do i = 1, n2
               xout(i+ioff, j) = xin(i, j, p)
            end do
         end do
      end do
      if (timers_enabled) call timer_stop(T_transxzfin)

      return
      end


c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine transpose_x_z(l1, l2, xin, xout)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none
      include 'global.h'
      integer l1, l2
      double complex xin(ntotal/np), xout(ntotal/np)

      call transpose_x_z_local(dims(1,l1),xin, xout)
      call transpose_x_z_global(dims(1,l1), xout, xin)
      call transpose_x_z_finish(dims(1,l2), xin, xout)
      return
      end


c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine transpose_x_z_local(d, xin, xout)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none
      include 'global.h'
      integer d(3)
      double complex xin(d(1),d(2),d(3))
      double complex xout(d(3),d(2),d(1))
      integer block1, block3
      integer i, j, k, kk, ii, i1, k1
      double complex buf(transblockpad, maxdim)

      if (timers_enabled) call timer_start(T_transxzloc)
      if (d(1) .lt. 32) goto 100
      block3 = d(3)
      if (block3 .eq. 1)  goto 100
      if (block3 .gt. transblock) block3 = transblock
      block1 = d(1)
      if (block1*block3 .gt. transblock*transblock) 
     >          block1 = transblock*transblock/block3
c---------------------------------------------------------------------
c blocked transpose
c---------------------------------------------------------------------
      do j = 1, d(2)
         do kk = 0, d(3)-block3, block3
            do ii = 0, d(1)-block1, block1
               
               do k = 1, block3
                  k1 = k + kk
                  do i = 1, block1
                     buf(k, i) = xin(i+ii, j, k1)
                  end do
               end do

               do i = 1, block1
                  i1 = i + ii
                  do k = 1, block3
                     xout(k+kk, j, i1) = buf(k, i)
                  end do
               end do

            end do
         end do
      end do
      goto 200
      

c---------------------------------------------------------------------
c basic transpose
c---------------------------------------------------------------------
 100  continue
      
      do j = 1, d(2)
         do k = 1, d(3)
            do i = 1, d(1)
               xout(k, j, i) = xin(i, j, k)
            end do
         end do
      end do

c---------------------------------------------------------------------
c all done
c---------------------------------------------------------------------
 200  continue

      if (timers_enabled) call timer_stop(T_transxzloc)
      return 
      end


c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine transpose_x_z_global(d, xin, xout)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none
      include 'global.h'
      include 'mpinpb.h'
      integer d(3)
      double complex xin(d(3),d(2),d(1))
      double complex xout(d(3),d(2),d(1)) ! not real layout, but right size
      integer ierr

      if (timers_enabled) call synchup()

c---------------------------------------------------------------------
c do transpose among all  processes with same 1-coord (me1)
c---------------------------------------------------------------------
      if (timers_enabled)call timer_start(T_transxzglo)
      call mpi_alltoall(xin, d(1)*d(2)*d(3)/np2, dc_type,
     >                  xout, d(1)*d(2)*d(3)/np2, dc_type,
     >                  commslice1, ierr)
      if (timers_enabled) call timer_stop(T_transxzglo)
      return
      end
      

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine transpose_x_z_finish(d, xin, xout)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none
      include 'global.h'
      integer d(3)
      double complex xin(d(1)/np2, d(2), d(3), 0:np2-1)
      double complex xout(d(1),d(2),d(3))
      integer i, j, k, p, ioff
      if (timers_enabled) call timer_start(T_transxzfin)
c---------------------------------------------------------------------
c this is the most straightforward way of doing it. the
c calculation in the inner loop doesn't help. 
c      do i = 1, d(1)/np2
c         do j = 1, d(2)
c            do k = 1, d(3)
c               do p = 0, np2-1
c                  ii = i + p*d(1)/np2
c                  xout(ii, j, k) = xin(i, j, k, p)
c               end do
c            end do
c         end do
c      end do
c---------------------------------------------------------------------

      do p = 0, np2-1
         ioff = p*d(1)/np2
         do k = 1, d(3)
            do j = 1, d(2)
               do i = 1, d(1)/np2
                  xout(i+ioff, j, k) = xin(i, j, k, p)
               end do
            end do
         end do
      end do
      if (timers_enabled) call timer_stop(T_transxzfin)
      return
      end


c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine transpose_x_y(l1, l2, xin, xout)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none
      include 'global.h'
      integer l1, l2
      double complex xin(ntotal/np), xout(ntotal/np)

c---------------------------------------------------------------------
c xy transpose is a little tricky, since we don't want
c to touch 3rd axis. But alltoall must involve 3rd axis (most 
c slowly varying) to be efficient. So we do
c (nx, ny/np1, nz/np2) -> (ny/np1, nz/np2, nx) (local)
c (ny/np1, nz/np2, nx) -> ((ny/np1*nz/np2)*np1, nx/np1) (global)
c then local finish. 
c---------------------------------------------------------------------


      call transpose_x_y_local(dims(1,l1),xin, xout)
      call transpose_x_y_global(dims(1,l1), xout, xin)
      call transpose_x_y_finish(dims(1,l1), xin, xout)

      return
      end


c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine transpose_x_y_local(d, xin, xout)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none
      include 'global.h'
      integer d(3)
      double complex xin(d(1),d(2),d(3))
      double complex xout(d(2),d(3),d(1))
      integer i, j, k
      if (timers_enabled) call timer_start(T_transxyloc)

      do k = 1, d(3)
         do i = 1, d(1)
            do j = 1, d(2)
               xout(j,k,i)=xin(i,j,k)
            end do
         end do
      end do
      if (timers_enabled) call timer_stop(T_transxyloc)
      return 
      end


c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine transpose_x_y_global(d, xin, xout)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none
      include 'global.h'
      include 'mpinpb.h'
      integer d(3)
c---------------------------------------------------------------------
c array is in form (ny/np1, nz/np2, nx)
c---------------------------------------------------------------------
      double complex xin(d(2),d(3),d(1))
      double complex xout(d(2),d(3),d(1)) ! not real layout but right size
      integer ierr

      if (timers_enabled) call synchup()

c---------------------------------------------------------------------
c do transpose among all processes with same 1-coord (me1)
c---------------------------------------------------------------------
      if (timers_enabled) call timer_start(T_transxyglo)
      call mpi_alltoall(xin, d(1)*d(2)*d(3)/np1, dc_type,
     >                  xout, d(1)*d(2)*d(3)/np1, dc_type,
     >                  commslice2, ierr)
      if (timers_enabled) call timer_stop(T_transxyglo)

      return
      end
      

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine transpose_x_y_finish(d, xin, xout)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none
      include 'global.h'
      integer d(3)
      double complex xin(d(1)/np1, d(3), d(2), 0:np1-1)
      double complex xout(d(1),d(2),d(3))
      integer i, j, k, p, ioff
      if (timers_enabled) call timer_start(T_transxyfin)
c---------------------------------------------------------------------
c this is the most straightforward way of doing it. the
c calculation in the inner loop doesn't help. 
c      do i = 1, d(1)/np1
c         do j = 1, d(2)
c            do k = 1, d(3)
c               do p = 0, np1-1
c                  ii = i + p*d(1)/np1
c note order is screwy bcz we have (ny/np1, nz/np2, nx) -> (ny, nx/np1, nz/np2)
c                  xout(ii, j, k) = xin(i, k, j, p)
c               end do
c            end do
c         end do
c      end do
c---------------------------------------------------------------------

      do p = 0, np1-1
         ioff = p*d(1)/np1
         do k = 1, d(3)
            do j = 1, d(2)
               do i = 1, d(1)/np1
                  xout(i+ioff, j, k) = xin(i, k, j, p)
               end do
            end do
         end do
      end do
      if (timers_enabled) call timer_stop(T_transxyfin)
      return
      end


c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine checksum(i, u1, d)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none
      include 'global.h'
      include 'mpinpb.h'
      integer i, d(3)
      double complex u1(d(1),d(2),d(3))
      integer j, q,r,s, ierr
      double complex chk,allchk
      chk = (0.0,0.0)

      do j=1,1024
         q = mod(j, nx)+1
         if (q .ge. xstart(1) .and. q .le. xend(1)) then
            r = mod(3*j,ny)+1
            if (r .ge. ystart(1) .and. r .le. yend(1)) then
               s = mod(5*j,nz)+1
               if (s .ge. zstart(1) .and. s .le. zend(1)) then
                  chk=chk+u1(q-xstart(1)+1,r-ystart(1)+1,s-zstart(1)+1)
               end if
            end if
         end if
      end do
      chk = chk/dble(ntotal)
      
      call MPI_Reduce(chk, allchk, 1, dc_type, MPI_SUM, 
     >                0, MPI_COMM_WORLD, ierr)      
      if (me .eq. 0) then
            write (*, 30) i, allchk
 30         format (' T =',I5,5X,'Checksum =',1P2D22.12)
      endif
      sums(i) = allchk
      return
      end

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine synchup

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none
      include 'global.h'
      include 'mpinpb.h'
      integer ierr
      call timer_start(T_synch)
      call mpi_barrier(MPI_COMM_WORLD, ierr)
      call timer_stop(T_synch)
      return
      end


c---------------------------------------------------------------------
c---------------------------------------------------------------------

      subroutine verify (d1, d2, d3, nt, verified, class)

c---------------------------------------------------------------------
c---------------------------------------------------------------------

      implicit none
      include 'global.h'
      include 'mpinpb.h'
      integer d1, d2, d3, nt
      character class
      logical verified
      integer ierr, size, i
      double precision err, epsilon

c---------------------------------------------------------------------
c   Sample size reference checksums
c---------------------------------------------------------------------
      double precision vdata_real_s(6)
      double precision vdata_imag_s(6)
      double precision vdata_real_w(6)
      double precision vdata_imag_w(6)
      double precision vdata_real_a(6)
      double precision vdata_imag_a(6)
      double precision vdata_real_b(20)
      double precision vdata_imag_b(20)
      double precision vdata_real_c(20)
      double precision vdata_imag_c(20)
      data vdata_real_s / 5.546087004964D+02,
     >                5.546385409189D+02,
     >                5.546148406171D+02,
     >                5.545423607415D+02,
     >                5.544255039624D+02,
     >                5.542683411902D+02 /

      data vdata_imag_s / 4.845363331978D+02,
     >                4.865304269511D+02,
     >                4.883910722336D+02,
     >                4.901273169046D+02,
     >                4.917475857993D+02,
     >                4.932597244941D+02 /
c---------------------------------------------------------------------
c   Class W size reference checksums
c---------------------------------------------------------------------
       data vdata_real_w /
     >                5.673612178944D+02,
     >                5.631436885271D+02,
     >                5.594024089970D+02,
     >                5.560698047020D+02,
     >                5.530898991250D+02,
     >                5.504159734538D+02/
       data vdata_imag_w /
     >               5.293246849175D+02,
     >               5.282149986629D+02,
     >               5.270996558037D+02, 
     >               5.260027904925D+02, 
     >               5.249400845633D+02,
     >               5.239212247086D+02/

c---------------------------------------------------------------------
c   Class A size reference checksums
c---------------------------------------------------------------------
      data vdata_real_a / 5.046735008193D+02,
     >                5.059412319734D+02,
     >                5.069376896287D+02,
     >                5.077892868474D+02,
     >                5.085233095391D+02,
     >                5.091487099959D+02 /
      
      data vdata_imag_a / 5.114047905510D+02,
     >                5.098809666433D+02,
     >                5.098144042213D+02,
     >                5.101336130759D+02,
     >                5.104914655194D+02,
     >                5.107917842803D+02 /
      
c---------------------------------------------------------------------
c   Class B size reference checksums
c---------------------------------------------------------------------
      data vdata_real_b / 5.177643571579D+02,
     >                5.154521291263D+02,
     >                5.146409228649D+02,
     >                5.142378756213D+02,
     >                5.139626667737D+02,
     >                5.137423460082D+02,
     >                5.135547056878D+02,
     >                5.133910925466D+02,
     >                5.132470705390D+02,
     >                5.131197729984D+02,
     >                5.130070319283D+02,
     >                5.129070537032D+02,
     >                5.128182883502D+02,
     >                5.127393733383D+02,
     >                5.126691062020D+02,
     >                5.126064276004D+02,
     >                5.125504076570D+02,
     >                5.125002331720D+02,
     >                5.124551951846D+02,
     >                5.124146770029D+02 /
   
      data vdata_imag_b / 5.077803458597D+02,
     >                5.088249431599D+02,                  
     >                5.096208912659D+02,                     
     >                5.101023387619D+02,                  
     >                5.103976610617D+02,                  
     >                5.105948019802D+02,                  
     >                5.107404165783D+02,                  
     >                5.108576573661D+02,                  
     >                5.109577278523D+02,                  
     >                5.110460304483D+02,                  
     >                5.111252433800D+02,                  
     >                5.111968077718D+02,                  
     >                5.112616233064D+02,                  
     >                5.113203605551D+02,                  
     >                5.113735928093D+02,                  
     >                5.114218460548D+02,
     >                5.114656139760D+02,
     >                5.115053595966D+02,
     >                5.115415130407D+02,
     >                5.115744692211D+02 /

c---------------------------------------------------------------------
c   Class C size reference checksums
c---------------------------------------------------------------------
      data vdata_real_c / 5.195078707457D+02,
     >                5.155422171134D+02,
     >                5.144678022222D+02,
     >                5.140150594328D+02,
     >                5.137550426810D+02,
     >                5.135811056728D+02,
     >                5.134569343165D+02,
     >                5.133651975661D+02,
     >                5.132955192805D+02,
     >                5.132410471738D+02,
     >                5.131971141679D+02,
     >                5.131605205716D+02,
     >                5.131290734194D+02,
     >                5.131012720314D+02,
     >                5.130760908195D+02,
     >                5.130528295923D+02,
     >                5.130310107773D+02,
     >                5.130103090133D+02,
     >                5.129905029333D+02,
     >                5.129714421109D+02 /

      data vdata_imag_c / 5.149019699238D+02,
     >                5.127578201997D+02,
     >                5.122251847514D+02,
     >                5.121090289018D+02,
     >                5.121143685824D+02,
     >                5.121496764568D+02,
     >                5.121870921893D+02,
     >                5.122193250322D+02,
     >                5.122454735794D+02,
     >                5.122663649603D+02,
     >                5.122830879827D+02,
     >                5.122965869718D+02,
     >                5.123075927445D+02,
     >                5.123166486553D+02,
     >                5.123241541685D+02,
     >                5.123304037599D+02,
     >                5.123356167976D+02,
     >                5.123399592211D+02,
     >                5.123435588985D+02,
     >                5.123465164008D+02 /





      if (me .ne. 0) return


      epsilon = 1.0d-12

      verified = .FALSE.
      class = 'U'

      if (d1 .eq. 64 .and.
     >    d2 .eq. 64 .and.
     >    d3 .eq. 64 .and.
     >    nt .eq. 6) then
         class = 'S'
         do i = 1, nt
            err = (dble(sums(i)) - vdata_real_s(i)) / vdata_real_s(i)
            if (abs(err) .gt. epsilon) goto 100
            err = (dimag(sums(i)) - vdata_imag_s(i)) / vdata_imag_s(i)
c If you have a machine where the above does not compile, let 
c us know and use the following
c            err = (aimag(sums(i)) - vdata_imag_s(i)) / vdata_imag_s(i)
            if (abs(err) .gt. epsilon) goto 100
         end do
         verified = .TRUE.
 100     continue
      else if (d1 .eq. 128 .and.
     >    d2 .eq. 128 .and.
     >    d3 .eq. 32 .and.
     >    nt .eq. 6) then
         class = 'W'
         do i = 1, nt
            err = (dble(sums(i)) - vdata_real_w(i)) / vdata_real_w(i)
            if (abs(err) .gt. epsilon) goto 105
            err = (dimag(sums(i)) - vdata_imag_w(i)) / vdata_imag_w(i)
c If you have a machine where the above does not compile, let 
c us know and use the following
c            err = (aimag(sums(i)) - vdata_imag_w(i)) / vdata_imag_w(i)
            if (abs(err) .gt. epsilon) goto 105
         end do
         verified = .TRUE.
 105     continue
      else if (d1 .eq. 256 .and.
     >    d2 .eq. 256 .and.
     >    d3 .eq. 128 .and.
     >    nt .eq. 6) then
         class = 'A'
         do i = 1, nt
            err = (dble(sums(i)) - vdata_real_a(i)) / vdata_real_a(i)
            if (abs(err) .gt. epsilon) goto 110
            err = (dimag(sums(i)) - vdata_imag_a(i)) / vdata_imag_a(i)
c If you have a machine where the above does not compile, let 
c us know and use the following
c            err = (aimag(sums(i)) - vdata_imag_a(i)) / vdata_imag_a(i)
            if (abs(err) .gt. epsilon) goto 110
         end do
         verified = .TRUE.
 110     continue
      else if (d1 .eq. 512 .and.
     >    d2 .eq. 256 .and.
     >    d3 .eq. 256 .and.
     >    nt .eq. 20) then
         class = 'B'
         do i = 1, nt
            err = (dble(sums(i)) - vdata_real_b(i)) / vdata_real_b(i)
            if (abs(err) .gt. epsilon) goto 120
            err = (dimag(sums(i)) - vdata_imag_b(i)) / vdata_imag_b(i)
c If you have a machine where the above does not compile, let 
c us know and use the following
c            err = (aimag(sums(i)) - vdata_imag_b(i)) / vdata_imag_b(i)
            if (abs(err) .gt. epsilon) goto 120
         end do
         verified = .TRUE.
 120     continue
      else if (d1 .eq. 512 .and.
     >    d2 .eq. 512 .and.
     >    d3 .eq. 512 .and.
     >    nt .eq. 20) then
         class = 'C'
         do i = 1, nt 
            err = (dble(sums(i)) - vdata_real_c(i)) / vdata_real_c(i)
            if (abs(err) .gt. epsilon) goto 130
            err = (dimag(sums(i)) - vdata_imag_c(i)) / vdata_imag_c(i)
c            err = (aimag(sums(i)) - vdata_imag_c(i)) / vdata_imag_c(i)
            if (abs(err) .gt. epsilon) goto 130
         end do
         verified = .TRUE.
 130     continue
      endif
      call MPI_COMM_SIZE(MPI_COMM_WORLD, size, ierr)
      if (size .ne. np) then
         write(*, 4010) np
         write(*, 4011)
         write(*, 4012)
c---------------------------------------------------------------------
c multiple statements because some Fortran compilers have
c problems with long strings. 
c---------------------------------------------------------------------
 4010    format( ' Warning: benchmark was compiled for ', i3, 
     >           'processors')
 4011    format( ' Must be run on this many processors for official',
     >           ' verification')
 4012    format( ' so memory access is repeatable')
         verified = .false.
      endif
         
      if (class .ne. 'U') then
         if (verified) then
            write(*,2000)
 2000       format(' Result verification successful')
         else
            write(*,2001)
 2001       format(' Result verification failed')
         endif
      endif
      print *, 'class = ', class

      return
      end


