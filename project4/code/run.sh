#!/bin/sh
make clean
make
cd benchmark
make clean
make
./simple_test
cd ..
rm -r /tmp/jss393/mountdir/files

