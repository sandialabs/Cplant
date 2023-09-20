      program main

      real x, y, z
      integer myid

      x = 1.0000
      y = 1.2222
      z = -4.3

      myid = 1

      print *, "myid=", myid, "."

      open(unit=1, file='./myfile', 
     z     form = 'formatted',
     z     access= 'sequential', status='new') 

      write(unit=1,fmt=11) x, y, z
11    format(f8.4, f8.4, f8.4)

      endfile(unit=1)
      close(unit=1)

      stop
      end
