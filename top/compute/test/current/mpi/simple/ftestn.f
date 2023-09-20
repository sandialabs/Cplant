      program main

      include 'mpif.h'


	character*3 fex
	character*4 mess1, mess2
	character*30 fname
	character*30 fstat
	character*30 userv

      integer n, myid, numprocs, i, rc, ierr, stat(MPI_STATUS_SIZE)
	integer getpid
	integer length
	logical lexist
	integer function lnblnk
	integer function mylen

	real x0(100), x1(100)

      call MPI_INIT( ierr )
      call MPI_COMM_RANK( MPI_COMM_WORLD, myid, ierr )
      call MPI_COMM_SIZE( MPI_COMM_WORLD, numprocs, ierr )

	do ij01 = 1, 100
		x0(ij01) = 0.
		x1(ij01) = 1.
	enddo

	call getenv('USER', userv)
	print *, '  userv is ', userv
        length = mylen(userv)
	print *, '  length of userv is ', length

      print *, "Process ", myid, "(",getpid(),") of ", 
     1	numprocs, " is alive"

	do ii = 1, numprocs
	if(ii - 1 .eq. myid) then
		if(myid .lt. 10) then
		write(fname, '(a14,a5,a4,i1)') 
     1		'/raid_010/tmp/',userv,'/ran',myid
		else
		write(fname, '(a14,a5,a4,i2)')
     1		'/raid_010/tmp/',userv,'/ran',myid
		endif
		inquire( file = fname(1:lnblnk(fname)),
     1  exist = lexist )
	                if( lexist ) then
	                        fex = 'old'
	                else
	                        fex = 'new'
	                endif


        print *,'Process ', myid,' attempts to open ', fname
	print *,' fex for ', fname, ' is ', fex
         open( unit=ii+10, file = fname(1:lnblnk(fname)),
     1  status = 'unknown', form='formatted', err = 11 )
        print *, ' Ready to write: process ', myid
        write (ii+10,fmt=112) x0
         close(ii+10 )
        go to 111
 11     print *, 'Cannot open ',fname
         go to 30
 111    continue
 112    format(f8.4)

	endif

	enddo
	
	if(myid .eq. 0) then
		do mid = 1, numprocs - 1
		call MPI_SEND( mess1, 4, MPI_CHARACTER, mid,
     1		1, MPI_COMM_WORLD, ierr)
		enddo
	endif

	if( myid .gt. 0 ) then

	call MPI_RECV( mess2, 4, MPI_CHARACTER, MPI_ANY_SOURCE,
     1	MPI_ANY_TAG, MPI_COMM_WORLD, stat, ierr)
     	print *, "process ", myid, " received mess2: ", mess2 
	endif
	
         n = 0
      call MPI_BCAST(n,1,MPI_INTEGER,0,MPI_COMM_WORLD,ierr)

c                                 check for quit signal
      if ( n .le. 0 ) goto 30

 30   call MPI_FINALIZE(rc)
      stop
      end

	integer function lnblnk( record )
	character*(*) record
	ill = 0
	ils = len( record )
	do il = ils, 1, -1
	if ( record(il:il) .ne. ' ' ) then
		ill = il
		go to 1
	end if
	end do
 1	lnblnk = ill
	return 
		end

        integer function mylen( name )
        character*(*) name
        do j = 1, 30, 1
          if ( name(j:j) .eq. ' ') then
            mylen = j-1
            return
          endif
        enddo
        mylen = 30
        return 
        end
        

