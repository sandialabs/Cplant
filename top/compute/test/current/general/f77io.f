      program main

      real x, y, z
      real a, b, c

      integer istat

      x = 1.0000
      y = 1.2222
      z = -4.3

      open(unit=1, file='./myfile', 
     z     form = 'formatted',
     z     access= 'sequential', status='old', iostat=istat) 

      if ( istat .eq. 0 ) then
        close(unit=1, status='delete')
      end if

      open(unit=1, file='./myfile', 
     z     form = 'formatted',
     z     access= 'sequential', status='new') 

      write(unit=1,fmt=11) x, y, z
11    format(f8.4, f8.4, f8.4)

      endfile(unit=1)
      close(unit=1)


      open(unit=1, file='./myfile', 
     z     form = 'formatted',
     z     access= 'sequential', status='old') 

      read(unit=1,fmt=11) a, b, c

      close(unit=1)

      print *, a, b, c

      stop
      end

