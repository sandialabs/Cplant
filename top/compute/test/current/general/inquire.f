	program inqtst
	logical lexist

	character*256 filn

 1	continue

	write(6,*) 'Enter a file name...'
	read(5,'(a)') filn

        inquire(file=filn,exist=lexist)
        if(lexist) go to 55
        go to 58


   55   continue
	write(6,12)  ' The file ', filn, ' exists'
 12	format(a10, a36, a7)
	go to 59

  58	continue

	write(6,13)  ' The file ', filn, ' does not exist'
 13	format(a10, a36, a15)

  59 	continue

	open(unit = 8, file = filn, status = 'old', err = 14)

	print *, ' open succeeded for ', filn
	go to 15
 14	print *, ' open failed for ', filn
 15	continue

        end
