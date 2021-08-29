#!/bin/sh
for i in 1 2 3 4 6 8 12
do
    echo $i
    for j in 1024 2048 4096 8192 16384
    do
        #echo $j
        mpiexec -use-hwthread-cpus -n $i ./bin/q22 0 10000 $j
    done
    echo " "
done
