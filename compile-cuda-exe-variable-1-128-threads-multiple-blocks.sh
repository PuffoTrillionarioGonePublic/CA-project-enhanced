#!/bin/bash

# compile the best cuda executables (both double and float)
# varing the CUDA Block Size (i.e. the bumber of thread for
# a CUDA block)

SRC=src
OUT_DIR=cuda-exe-variable-1-128-multiple-blocks

mkdir -p $OUT_DIR

for CBS in {1..128};
do
    echo $CBS
    # float
    echo FLOAT:
    cmd="time nvcc -g -O3 -DGPU -DSTORE_BY_ROWS -DFLOAT -DCUDA_BLOCK_SIZE=$CBS -x cu $SRC/main.cc $SRC/argparser.cc -o ${OUT_DIR}/cuda-row-float-buf-noyield-fast-${CBS}threads"
    echo "Executing \"$cmd\" ..."
    eval $cmd
    echo

    # double
    echo DOUBLE:
    cmd="time nvcc -g -O3 -DGPU -DSTORE_BY_ROWS -DCUDA_BLOCK_SIZE=$CBS -x cu $SRC/main.cc $SRC/argparser.cc -o ${OUT_DIR}/cuda-row-double-buf-noyield-fast-CBS-${CBS}threads"
    echo "Executing \"$cmd\" ..."
    eval $cmd
    echo
    echo
done


