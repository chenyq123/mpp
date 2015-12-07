#/bin/sh
cp /mnt/hgfs/Desktop/videotemp/ive_test2.c .
make
echo 'cp ive_test2 ../../../nfs'
cp ive_test2 ../../../nfs
echo 'ok'
