      program main

      real x, y, z

      open(unit=1, file='/raid_010/tmp/jotto/myfile', 
     z     form = 'formatted',
     z     access= 'sequential', status='old') 

      read(unit=1,fmt=11) x, y, z
11    format(f8.4, f8.4, f8.4)

      close(unit=1)

      print *, x, y, z

      stop
      end

