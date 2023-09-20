#! /bin/sh

machine=`uname -m`
case $machine in
	sun4*)
		machine=sparc
		;;
	sparc*)
		machine=sparc
		;;
	9000/*)
		machine=hp
		;;
	i[3456]86 | i86pc)
		machine=i386
		;;
	alpha)
		machine=alpha
		;;
	ia64)
		machine=ia64
		;;
	IP*)
		machine=sgi
		;;
	*)
		machine=blotto
		;;		
esac	
 
echo "$machine"
exit 0
