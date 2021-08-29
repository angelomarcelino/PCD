#!/bin/sh
for i in 1 2 4 6 12
do
    echo $i
    for j in 120 1200 12000 120000 1200000
    do
        #echo $j
        mpiexec -use-hwthread-cpus -n $i ./bin/q27 g $j
    done
    echo " "
done
