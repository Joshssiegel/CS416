#!/bin/sh
rm DISKFILE
make clean
make
cd benchmark
make clean
make
cd ..
 ./tfs -s -f /tmp/av558/mountdir
 echo Mounted /tmp/av558/mountdir
