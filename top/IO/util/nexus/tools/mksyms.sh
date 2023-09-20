#!/bin/sh

#
# In the past, we've had problems with libc bringing in symbols from the RPC
# library within libc itself. This was used to force all our RPC library's
# symbols in prior to linking against libc. It's no longer used...
#

echo "/*"
echo " * Force inclusion of symbols to get around problems with libc."
echo " *"
echo " * Please do not edit."
echo " *  This file generated on " `date` " by " `whoami`
echo " */"
echo
for i in $@
do
	newsyms=`nm $i | grep T | awk '{ print $3; }'` || exit
	echo "/* Symbols in $i */"
	for j in $newsyms
	do
		echo "extern int $j();"
	done
	echo
	syms="$syms $newsyms"
done

echo "void *_symstbl[] = {"
for i in $syms
do
	echo "	$i,"
done
echo "	0"
echo "};"
