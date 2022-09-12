#!/bin/bash

# Sample script used to generate *.csv
# with random numbers

# usage: ./cav.sh [column default(2)] [row default(30)]

# default values
COL_COUNT=2
ROW_COUNT=30

# regex to be used to check strings reprenting numbers
re='^[0-9]+$'

if [ $# -ge 1 ]; then
    # row arg provided
    if [[ ( ! $1 =~ $re ||  $1 -lt 2 ) ]]; then
        echo "First argument must be an integer >= 2" >/dev/stderr
        exit 1
    fi
    #echo '$1 = ' $1
    COL_COUNT=$1

    if [ $# -ge 2 ]; then
        # row arg provided
        if [[ ( ! $2 =~ $re ||  $2 -lt 1 ) ]]; then
            echo "Second argument must be an integer >= 1" >/dev/stderr
            exit 1
        fi
        #echo '$2 = ' $2
        ROW_COUNT=$2
    fi
fi

#echo "COL_COUNT = $COL_COUNT"
#echo "ROW_COUNT = $ROW_COUNT"

echo -n '"col1"';
for ((C = 2 ; C <= COL_COUNT ; C++)); do
    echo -n ",\"col$C\""
done
# add newline
echo

for ((R = 0 ; R < ROW_COUNT ; R++)); do
    echo -n '"'$RANDOM'"'
    for ((C = 1 ; C < COL_COUNT ; C++)); do
        echo -n ",\"$RANDOM\""
    done
    # newline
    echo
done

