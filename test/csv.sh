#!/usr/bin/bash

# generate consecutive numbers
CNT=0

# matrix size
ROWS=10; COLS=3

# add header
echo -n '"col1"';
for ((C = 2 ; C <= COLS ; C++)); do
    echo -n ",\"col$C\""
done
# add newline
echo

for ((R = 0 ; R < ROWS ; R++)); do
    echo -n '"'$CNT'"'
    CNT=$(($CNT + 1));
    for ((C = 1 ; C < COLS ; C++)); do
        echo -n ",\"$CNT\""
        CNT=$(($CNT + 1));
    done
    # newline
    echo
done



CNT=$(($CNT + 1));
