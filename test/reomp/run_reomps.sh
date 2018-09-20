#!/bin/bash

reomp_dir="/tmp/sato5"
#for test_case in omp_reduction omp_critical omp_atomic data_race
for test_case in omp_reduction 
do
    for threads in 1 2 4 8 12 16 20 24
    do
	com="./run_reomp.sh $threads $test_case"
	echo $com
	$com
	for mt in 2 1 0;
	do
	    for mode in 0 1;
	    do
		com="./run_reomp.sh $threads $test_case $mode $reomp_dir $mt 0"
		echo $com
		$com
		du ${reomp_dir}/*
		echo "total_size `du ${reomp_dir}`"
	    done
	done
    done
done
