      integer function mpir_iargc()

      mpir_iargc = IARGC()
      return
      end
c     
      subroutine mpir_getarg( i, s )

      integer       i
      character*(*) s
      external GETARG
      call GETARG(i,s)
      return
      end
