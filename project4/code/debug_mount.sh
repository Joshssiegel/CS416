#!/bin/sh
rm DISKFILE
make clean
make
cd benchmark
make clean
make
cd ..
 ./tfs -s -d -o nonempty /tmp/jss393/mountdir
 echo Mounted /tmp/jss393/mountdir
