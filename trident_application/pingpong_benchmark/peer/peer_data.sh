#!/bin/bash

wordsize=1

tx1=17
rx1=22
data1=27
tx0=9
rx0=10
data0=11

declare -A redocount

redocount[1]=20
redocount[200]=5
redocount[1000]=3

while [ $wordsize -le 1000 ] ; do 
  reruns=${redocount[1]}
  for i in ${!redocount[@]}; do
    if [ $wordsize -ge $i ] ; then
      reruns=${redocount[$i]}
    else
      break
    fi
  done
  echo "bit width: $wordsize, reruns: ${reruns}"
  for ((k=0; k < ${reruns}; k++)) ; do
    ./pingpong_benchmark_peer $tx0 $rx0 $data0 $tx1 $rx1 $data1 $wordsize 
  done
  wordsize=$[$wordsize + 1]
done
