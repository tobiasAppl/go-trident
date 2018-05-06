#!/bin/bash

wordsize=1
max_wordsize=500

tx1=17
rx1=22
data1=27
tx0=9
rx0=10
data0=11

step=1

while true; do
  echo "bit width: $wordsize" 
  ./pingpong_benchmark_peer $tx0 $rx0 $data0 $tx1 $rx1 $data1 $wordsize 
  
  if [ $wordsize -ge $max_wordsize ] ; then 
    step=-1
  elif [ $wordsize -le 1 ] ; then
    step=1
  fi
  wordsize=$[ $wordsize + $step ]
done
