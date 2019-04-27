#!/bin/sh
./mount.sh
make clean
make
cd benchmark
make clean
make
./simple_test
cd ..
./unmount.sh
#rm -r /tmp/jss393/mountdir/files


