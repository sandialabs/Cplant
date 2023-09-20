      integer function mpir_iargc()

      mpir_iargc = iargc()
      return
      end
c     
      subroutine mpir_getarg( i, s )

      integer       i
      character*(*) s
      external getarg
      call getarg(i,s)
      return
      end
