#!/bin/bash
make clean
make
cd Benchmark
make clean
make
./parallelCal $1
cd ..
