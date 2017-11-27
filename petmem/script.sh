rmmod petmem.ko
make
insmod petmem.ko
cd user
./petmem 128
./test