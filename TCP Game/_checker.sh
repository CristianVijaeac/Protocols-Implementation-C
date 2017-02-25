#!/bin/bash

COUNT=0
PRINT_LEN=55
function get_test_no {
	printf "%03d. " $COUNT
	COUNT=$(($COUNT + 1))
}

function print_points {
	NUM=$1
	for i in $(seq $NUM)
		do echo -n .
	done
}

function test {
	./run_experiment.sh $1 $2

	DIFF=$(diff client_output out_ref/test 2>&1) 
	if [ "$DIFF" != "" ] 
	then
		echo "FAILED"
	else
		echo "PASSED"
	fi
}

function check_test {
	TEST=$1
	VAL=$2

	get_test_no
	len=$(($PRINT_LEN - ${#TEST}))
	if [ "FAILED" = "$VAL" ];
	then
		#echo $2 >> $RESULT_FILE
		echo "$TEST$(print_points $len)FAILED"
		FAILED_TEST=1
	else
		PASS_COUNT=$(($PASS_COUNT + 1))
		echo "$TEST$(print_points $len)PASSED"
	fi
}

check_test "Test simplu " $(test 0)
check_test "Test cu ACK " $(test 0 ack)
check_test "Test cu parity " $(test 50 parity)
check_test "Test cu hamming " $(test 100 hamming)

echo 
echo 
echo 
echo "                                      Total  =  [ $(echo "scale=3; $PASS_COUNT/4*100" | bc)/100.000 ]"
echo 
echo 