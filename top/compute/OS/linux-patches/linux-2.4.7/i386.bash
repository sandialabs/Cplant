#! /bin/sh

KERNEL_VER=$1
KERNEL_LOC=$2
TARGET=$3

echo "cp $KERNEL_LOC/linux-$KERNEL_VER.tar.gz ."
cp $KERNEL_LOC/linux-$KERNEL_VER.tar.gz .

echo "tar xfz linux-$KERNEL_VER.tar.gz"
tar xfz linux-$KERNEL_VER.tar.gz

echo "cp -a axp-cplant/* linux"
cp -a axp-cplant/* linux

echo "cd linux"
cd linux

echo "cp config.$TARGET .config"
cp config.$TARGET .config

echo "make config ; make dep ; make clean"
make oldconfig ; make dep ; make clean 

echo "make"
make 

echo "make bzImage"
make bzImage

echo "cp arch/alpha/boot/bzImage .."
cp arch/alpha/boot/bzImage ..
