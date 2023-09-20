#! /bin/sh

KERNEL_VER=$1
KERNEL_LOC=$2
TARGET=$3

echo "tar xfz $KERNEL_LOC/linux-$KERNEL_VER.tar.gz"
tar xfz $KERNEL_LOC/linux-$KERNEL_VER.tar.gz

echo "cd linux-$KERNEL_VER"
cd linux-$KERNEL_VER
echo "tar cf - -C ../axp-cplant . | tar xvf -"
tar cf - -C ../axp-cplant . | tar xf -

echo "cp config.$TARGET .config"
cp config.$TARGET .config

echo "make config ; make dep ; make clean"
make oldconfig ; make dep ; make clean 

echo "make"
make 

echo "make bootpfile"
make bootpfile

echo "cp arch/alpha/boot/bootpfile .."
cp arch/alpha/boot/bootpfile ..
