#!/bin/sh
for i in 1 2 3 4 6 8 12
do
    echo $i
    for j in 120 1200 12000 120000 1200000
    do
        #echo $j
        mpiexec -use-hwthread-cpus -n $i ./bin/q22 0 10000 $j
    done
    echo " "
done
