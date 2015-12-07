#/bin/sh
cp /mnt/hgfs/Desktop/final.c .
cp /mnt/hgfs/Desktop/client.c ~/hi3516c/nfs/
make
gcc ~/hi3516c/nfs/client.c -o ~/hi3516c/nfs/client
cp final ~/hi3516c/nfs

