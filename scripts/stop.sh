#!/usr/bin/env bash

cd ..
TOPDIR=`pwd -P`
cd $TOPDIR/scripts

YFSDIR1=$TOPDIR/yfs1
YFSDIR2=$TOPDIR/yfs2

export PATH=$PATH:/usr/local/bin
UMOUNT="umount"
if [ -f "/usr/local/bin/fusermount" -o -f "/usr/bin/fusermount" -o -f "/bin/fusermount" ]; then
    UMOUNT="fusermount -u";
fi
$UMOUNT $YFSDIR1
$UMOUNT $YFSDIR2
killall yfs_master
killall yfs_client
killall lock_server
