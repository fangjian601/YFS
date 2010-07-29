#!/usr/bin/env bash

cd ..
TOPDIR=`pwd -P`
cd $TOPDIR/scripts

ulimit -c unlimited

LOSSY=$1
NUM_LS=$2

if [ -z $NUM_LS ]; then
    NUM_LS=0
fi

echo $LOSSY, $NUM_LS

BASE_PORT=$RANDOM
BASE_PORT=$[BASE_PORT+2000]
EXTENT_PORT=$BASE_PORT
YFS1_PORT=$[BASE_PORT+2]
YFS2_PORT=$[BASE_PORT+4]
LOCK_PORT=$[BASE_PORT+6]

YFSDIR1=$TOPDIR/yfs1
YFSDIR2=$TOPDIR/yfs2

LOCK_SERVER=$TOPDIR/yfs_server/bin/lock_server
YFS_MASTER=$TOPDIR/yfs_server/bin/yfs_master
YFS_CLIENT=$TOPDIR/yfs_client/bin/yfs_client

if [ "$LOSSY" ]; then
    export RPC_LOSSY=$LOSSY
fi

if [ $NUM_LS -gt 1 ]; then
    x=0
    rm config
    while [ $x -lt $NUM_LS ]; do
      port=$[LOCK_PORT+2*x]
      x=$[x+1]
      echo $port >> config
    done
    x=0
    while [ $x -lt $NUM_LS ]; do
      port=$[LOCK_PORT+2*x]
      x=$[x+1]
      echo "starting $LOCK_SERVER $LOCK_PORT $port > $TOPDIR/log/lock_server$x.log 2>&1 &"
      $LOCK_SERVER $LOCK_PORT $port > $TOPDIR/log/lock_server$x.log 2>&1 &
      sleep 1
    done
else
    echo "starting $LOCK_SERVER $LOCK_PORT > $TOPDIR/log/lock_server.log 2>&1 &"
    $LOCK_SERVER $LOCK_PORT > $TOPDIR/log/lock_server.log 2>&1 &
    sleep 1
fi

unset RPC_LOSSY

echo "starting $YFS_MASTER $EXTENT_PORT > $TOPDIR/log/extent_server.log 2>&1 &"
$YFS_MASTER $EXTENT_PORT > $TOPDIR/log/extent_server.log 2>&1 &
sleep 1

mkdir -p $YFSDIR1
sleep 1
echo "starting $YFS_CLIENT $YFSDIR1 $EXTENT_PORT $LOCK_PORT > $TOPDIR/log/yfs_client1.log 2>&1 &"
$YFS_CLIENT $YFSDIR1 $EXTENT_PORT $LOCK_PORT > $TOPDIR/log/yfs_client1.log 2>&1 &
sleep 1

mkdir -p $YFSDIR2
sleep 1
echo "$YFS_CLIENT $YFSDIR2 $EXTENT_PORT $LOCK_PORT > $TOPDIR/log/yfs_client2.log 2>&1 &"
$YFS_CLIENT $YFSDIR2 $EXTENT_PORT $LOCK_PORT > $TOPDIR/log/yfs_client2.log 2>&1 &

sleep 2

# make sure FUSE is mounted where we expect
pwd=`pwd -P`
if [ `mount | grep "$TOPDIR/yfs1" | grep -v grep | wc -l` -ne 1 ]; then
    sh stop.sh
    echo "Failed to mount YFS properly at $TOPDIR/yfs1"
    exit -1
fi

# make sure FUSE is mounted where we expect
if [ `mount | grep "$TOPDIR/yfs2" | grep -v grep | wc -l` -ne 1 ]; then
    sh stop.sh
    echo "Failed to mount YFS properly at $TOPDIR/yfs2"
    exit -1
fi
