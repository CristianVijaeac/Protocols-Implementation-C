#!/bin/bash

SPEED=1000
DELAY=0
LOSS=0
CORRUPT=$1

killall link &> /dev/null
killall server &> /dev/null
killall client &> /dev/null


echo "FAIL" > client_output
(./link_emulator/link speed=$SPEED delay=$DELAY loss=$LOSS corrupt=$CORRUPT &> /dev/null &)
sleep 1
(./server $2 &)
sleep 1
(./client $2 &)
sleep 2

killall link &> /dev/null
killall server &> /dev/null
killall client &> /dev/null
