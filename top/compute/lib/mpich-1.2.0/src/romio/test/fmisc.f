      program main
      implicit none

      include 'mpif.h'

!     Fortran equivalent of misc.c
!     tests various miscellaneous functions.

      integer buf(1024), amode, flag, fh, status(MPI_STATUS_SIZE)
      integer ierr, newtype, i, group
      integer etype, filetype, mynod, argc, iargc
      character*7 datarep
      character*1024 str    ! used to store the filename
      integer*8 disp, offset, filesize
      

      call MPI_INIT(ierr)
      call MPI_COMM_RANK(MPI_COMM_WORLD, mynod, ierr)

!     process 0 takes the file name as a command-line argument and 
!     broadcasts it to other processes

      if (mynod .eq. 0) then
         argc = iargc()
         i = 0
         call getarg(i,str)
         do while ((i .lt. argc) .and. (str .ne. '-fname'))
            i = i + 1
            call getarg(i,str)
         end do
         if (i .ge. argc) then
	    print *
            print *, '*#  Usage: fmisc -fname filename'
            print *
            call MPI_ABORT(MPI_COMM_WORLD, 1, ierr)
         end if

         i = i + 1
         call getarg(i,str)
         call MPI_BCAST(str, 1024, MPI_CHARACTER, 0,                    &
     &        MPI_COMM_WORLD, ierr)
      else 
         call MPI_BCAST(str, 1024, MPI_CHARACTER, 0,                    &
     &        MPI_COMM_WORLD, ierr)
      end if


      call MPI_FILE_OPEN(MPI_COMM_WORLD, str,                           &
     &     MPI_MODE_CREATE + MPI_MODE_RDWR, MPI_INFO_NULL, fh, ierr)

      call MPI_FILE_WRITE(fh, buf, 1024, MPI_INTEGER, status, ierr)

      call MPI_FILE_SYNC(fh, ierr)

      call MPI_FILE_GET_AMODE(fh, amode, ierr)
      if (mynod .eq. 0) then
         print *, ' testing MPI_FILE_GET_AMODE'
      end if
      if (amode .ne. (MPI_MODE_CREATE + MPI_MODE_RDWR)) then
         print *, 'amode is ', amode, ', should be ', MPI_MODE_CREATE   &
     &           + MPI_MODE_RDWR
      end if

      call MPI_FILE_GET_ATOMICITY(fh, flag, ierr)
      if (flag .ne. 0) then
         print *, 'atomicity is ', flag, ', should be 0'
      end if
      if (mynod .eq. 0) then
         print *, ' setting atomic mode'
      end if
      call MPI_FILE_SET_ATOMICITY(fh, 1, ierr)
      call MPI_FILE_GET_ATOMICITY(fh, flag, ierr)
      if (flag .ne. 1) then
         print *, 'atomicity is ', flag, ', should be 1'
      end if
      call MPI_FILE_SET_ATOMICITY(fh, 0, ierr)
      if (mynod .eq. 0) then
         print *, ' reverting back to nonatomic mode'
      end if

      call MPI_TYPE_VECTOR(10, 10, 20, MPI_INTEGER, newtype, ierr)
      call MPI_TYPE_COMMIT(newtype, ierr)

      disp = 1000
      call MPI_FILE_SET_VIEW(fh, disp, MPI_INTEGER, newtype, 'native',  & 
     &     MPI_INFO_NULL, ierr)
      if (mynod .eq. 0) then
         print *, ' testing MPI_FILE_GET_VIEW'
      end if

      disp = 0
      call MPI_FILE_GET_VIEW(fh, disp, etype, filetype, datarep, ierr)
      if ((disp .ne. 1000) .or. (datarep .ne. 'native')) then
         print *, 'disp = ', disp, ', datarep = ', datarep,             &
     &     ', should be 1000, native'
      end if

      if (mynod .eq. 0) then
         print *, ' testing MPI_FILE_GET_BYTE_OFFSET'
      end if
      offset = 10
      call MPI_FILE_GET_BYTE_OFFSET(fh, offset, disp, ierr)
      if (disp .ne. 1080) then 
         print *, 'byte offset = ', disp, ', should be 1080'
      end if

      call MPI_FILE_GET_GROUP(fh, group, ierr)

      if (mynod .eq. 0) then
         print *, ' setting file size to 1060 bytes'
      end if
      filesize = 1060
      call MPI_FILE_SET_SIZE(fh, filesize, ierr)
      call MPI_BARRIER(MPI_COMM_WORLD, ierr)
      call MPI_FILE_SYNC(fh, ierr)
      filesize = 0
      call MPI_FILE_GET_SIZE(fh, filesize, ierr)
      if (filesize .ne. 1060) then
         print *, 'file size = ', filesize, ', should be 1060'
      end if
 
      if (mynod .eq. 0) then
         print *, ' seeking to eof and testing MPI_FILE_GET_POSITION'
      end if
      offset = 0
      call MPI_FILE_SEEK(fh, offset, MPI_SEEK_END, ierr)
      call MPI_FILE_GET_POSITION(fh, offset, ierr)
      if (offset .ne. 10) then 
         print *, 'file pointer posn = ', offset, ', should be 10'
      end if

      if (mynod .eq. 0) then
         print *, ' testing MPI_FILE_GET_BYTE_OFFSET'
      end if
      call MPI_FILE_GET_BYTE_OFFSET(fh, offset, disp, ierr)
      if (disp .ne. 1080) then
         print *, 'byte offset = ', disp, ', should be 1080'
      end if
      call MPI_BARRIER(MPI_COMM_WORLD, ierr)

      if (mynod .eq. 0) then
         print *, ' testing MPI_FILE_SEEK with MPI_SEEK_CUR'
      end if
      offset = -10
      call MPI_FILE_SEEK(fh, offset, MPI_SEEK_CUR, ierr)
      call MPI_FILE_GET_POSITION(fh, offset, ierr)
      call MPI_FILE_GET_BYTE_OFFSET(fh, offset, disp, ierr)
      if (disp .ne. 1000) then 
         print *, 'file pointer posn in bytes = ', disp,                &
     &     ', should be 1000'
      end if

      if (mynod .eq. 0) then
         print *, ' preallocating disk space up to 8192 bytes'
      end if
      filesize = 8192
      call MPI_FILE_PREALLOCATE(fh, filesize, ierr)

      if (mynod .eq. 0) then
         print *, ' closing the file and deleting it'
      end if
      call MPI_FILE_CLOSE(fh, ierr)

      call MPI_BARRIER(MPI_COMM_WORLD, ierr)
      if (mynod .eq. 0) then
         call MPI_FILE_DELETE(str, MPI_INFO_NULL, ierr)
      end if

      call MPI_TYPE_FREE(newtype, ierr)    
      call MPI_TYPE_FREE(filetype, ierr)    
      call MPI_GROUP_FREE(group, ierr)
      call MPI_FINALIZE(ierr)

      stop
      end
