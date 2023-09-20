c   $Id: forFault.f,v 1.2 2001/04/12 06:58:33 lafisk Exp $
c   When fortran programs compiled on Tru 64 unix machines encounter
c   certain types of errors, the runtime system attempts to find an
c   error message in for_msg.cat.
c
c   Here is a simple fortran code that will encounter an error if
c   you run it from a directory that is not writable by you.  It
c   can be used to test the use of for_msg.cat.

c -------------------------------------------------------------------------
      program faulter

      open(11, file="faulterTest", status='new')

c
c     This should fail and the runtime library should seek an
c     error message in for_msg.cat
c
      write(11, *) 'trying to write to non-writable file'

      close(11)

      write(6,*) "done"

      end



