#! /bin/sh

RH_RELEASE=/etc/redhat-release

if [ -r $RH_RELEASE ] ; then
    redhat_major=`cat $RH_RELEASE | cut -d' ' -f 5 | cut -d\. -f 1`
    redhat_minor=`cat $RH_RELEASE | cut -d' ' -f 5 | cut -d\. -f 2`
    redhat_minor=x
else
    redhat_major=5
    redhat_minor=x
fi

echo "$redhat_major.$redhat_minor"
exit 0
