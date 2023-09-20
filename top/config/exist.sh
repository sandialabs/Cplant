#!/bin/sh

#TITLE(exist_sh, "@(#) $Id: exist.sh,v 1.3 1999/01/07 17:09:25 wmdavid Exp $");

# Is path empty?
if [ -z "$1" ] ; then
# Yes, test file does not exist
	echo "no"
	exit 0
fi

# Does path exist?
if [ -f $1 -o -d $1 ] ; then
	echo "yes"
else
	echo "no"
fi

exit 0
