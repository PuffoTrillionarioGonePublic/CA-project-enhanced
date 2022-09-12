#!/bin/bash

# compile the best cuda executables (both double and float)
# varing the CUDA Block Size (i.e. the bumber of thread for
# a CUDA block)

SRC=src
OUT_DIR=cuda-exe-variable-block-num

mkdir -p $OUT_DIR

for CBS in {4..1020..4};
do
    echo $CBS
    # float
    echo FLOAT:
    cmd="time nvcc -g -O3 -DGPU -DSTORE_BY_ROWS -DFLOAT -DCUDA_BLOCKS=$CBS -x cu $SRC/main.cc $SRC/argparser.cc -o ${OUT_DIR}/cuda-row-float-buf-noyield-fast-BLOCKS-${CBS} & "
    echo "Executing \"$cmd\" ..."
    eval $cmd
    echo

    # double
    echo DOUBLE:
    cmd="time nvcc -g -O3 -DGPU -DSTORE_BY_ROWS -DCUDA_BLOCKS=$CBS -x cu $SRC/main.cc $SRC/argparser.cc -o ${OUT_DIR}/cuda-row-double-buf-noyield-fast-BLOCKS-${CBS}"
    echo "Executing \"$cmd\" ..."
    eval $cmd
    echo
    echo
done


