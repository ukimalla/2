module add raspberry 
KERNEL=kernel7
LINUX_SOURCE=/project/scratch01/compile/u.malla/linux_source/linux
make -C $LINUX_SOURCE ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- M=$PWD modules
