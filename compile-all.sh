#!/bin/bash


SRC=src
EXES=cuda-executables

mkdir -p $EXES

# test macros:
## to be activated?
## STORE_BY_ROWS: or by column (default)?
## FLOAT: or double (default)?
## NO_PREALLOCATE_BUFFER: or preallocate buffer (default)
## YIELD: or no yield processor if iteration stalls (default)?
## SLOW: or perform computation in a much smarter manner (default)?


######################################## row | col
nvcc -g -O3 -DGPU -x cu \
    -DSTORE_BY_ROWS         \
    -DFLOAT                 \
    -DNO_PREALLOCATE_BUFFER \
    -DYIELD                 \
    -DSLOW                  \
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-row-float-nobuf-yield-slow

nvcc -g -O3 -DGPU -x cu \
\
    -DFLOAT                 \
    -DNO_PREALLOCATE_BUFFER \
    -DYIELD                 \
    -DSLOW                  \
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-col-float-nobuf-yield-slow


######################################## float | double
nvcc -g -O3 -DGPU -x cu \
    -DSTORE_BY_ROWS         \
\
    -DNO_PREALLOCATE_BUFFER \
    -DYIELD                 \
    -DSLOW                  \
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-row-double-nobuf-yield-slow

nvcc -g -O3 -DGPU -x cu \
\
\
    -DNO_PREALLOCATE_BUFFER \
    -DYIELD                 \
    -DSLOW                  \
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-col-double-nobuf-yield-slow



######################################## buf | no buf
nvcc -g -O3 -DGPU -x cu \
    -DSTORE_BY_ROWS         \
    -DFLOAT                 \
\
    -DYIELD                 \
    -DSLOW                  \
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-row-float-buf-yield-slow

nvcc -g -O3 -DGPU -x cu \
\
    -DFLOAT                 \
\
    -DYIELD                 \
    -DSLOW                  \
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-col-float-buf-yield-slow

nvcc -g -O3 -DGPU -x cu \
    -DSTORE_BY_ROWS         \
\
\
    -DYIELD                 \
    -DSLOW                  \
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-row-double-buf-yield-slow

nvcc -g -O3 -DGPU -x cu \
\
\
\
    -DYIELD                 \
    -DSLOW                  \
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-col-double-buf-yield-slow





######################################## yield | no yield
nvcc -g -O3 -DGPU -x cu \
    -DSTORE_BY_ROWS         \
    -DFLOAT                 \
    -DNO_PREALLOCATE_BUFFER \
\
    -DSLOW                  \
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-row-float-nobuf-noyield-slow

nvcc -g -O3 -DGPU -x cu \
\
    -DFLOAT                 \
    -DNO_PREALLOCATE_BUFFER \
\
    -DSLOW                  \
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-col-float-nobuf-noyield-slow

nvcc -g -O3 -DGPU -x cu \
    -DSTORE_BY_ROWS         \
\
    -DNO_PREALLOCATE_BUFFER \
\
    -DSLOW                  \
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-row-double-nobuf-noyield-slow

nvcc -g -O3 -DGPU -x cu \
\
\
    -DNO_PREALLOCATE_BUFFER \
\
    -DSLOW                  \
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-col-double-nobuf-noyield-slow

nvcc -g -O3 -DGPU -x cu \
    -DSTORE_BY_ROWS         \
    -DFLOAT                 \
\
\
    -DSLOW                  \
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-row-float-buf-noyield-slow

nvcc -g -O3 -DGPU -x cu \
\
    -DFLOAT                 \
\
\
    -DSLOW                  \
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-col-float-buf-noyield-slow

nvcc -g -O3 -DGPU -x cu \
    -DSTORE_BY_ROWS         \
\
\
\
    -DSLOW                  \
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-row-double-buf-noyield-slow

nvcc -g -O3 -DGPU -x cu \
\
\
\
\
    -DSLOW                  \
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-col-double-buf-noyield-slow







########################################## slow | fast
........................

nvcc -g -O3 -DGPU -x cu \
    -DSTORE_BY_ROWS         \
    -DFLOAT                 \
    -DNO_PREALLOCATE_BUFFER \
    -DYIELD                 \
\
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-row-float-nobuf-yield-fast

nvcc -g -O3 -DGPU -x cu \
\
    -DFLOAT                 \
    -DNO_PREALLOCATE_BUFFER \
    -DYIELD                 \
\
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-col-float-nobuf-yield-fast

nvcc -g -O3 -DGPU -x cu \
    -DSTORE_BY_ROWS         \
\
    -DNO_PREALLOCATE_BUFFER \
    -DYIELD                 \
\
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-row-double-nobuf-yield-fast

nvcc -g -O3 -DGPU -x cu \
\
\
    -DNO_PREALLOCATE_BUFFER \
    -DYIELD                 \
\
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-col-double-nobuf-yield-fast

nvcc -g -O3 -DGPU -x cu \
    -DSTORE_BY_ROWS         \
    -DFLOAT                 \
\
    -DYIELD                 \
\
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-row-float-buf-yield-fast

nvcc -g -O3 -DGPU -x cu \
\
    -DFLOAT                 \
\
    -DYIELD                 \
\
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-col-float-buf-yield-fast

nvcc -g -O3 -DGPU -x cu \
    -DSTORE_BY_ROWS         \
\
\
    -DYIELD                 \
\
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-row-double-buf-yield-fast

nvcc -g -O3 -DGPU -x cu \
\
\
\
    -DYIELD                 \
\
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-col-double-buf-yield-fast

nvcc -g -O3 -DGPU -x cu \
    -DSTORE_BY_ROWS         \
    -DFLOAT                 \
    -DNO_PREALLOCATE_BUFFER \
\
\
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-row-float-nobuf-noyield-fast

nvcc -g -O3 -DGPU -x cu \
\
    -DFLOAT                 \
    -DNO_PREALLOCATE_BUFFER \
\
\
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-col-float-nobuf-noyield-fast

nvcc -g -O3 -DGPU -x cu \
    -DSTORE_BY_ROWS         \
\
    -DNO_PREALLOCATE_BUFFER \
\
\
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-row-double-nobuf-noyield-fast

nvcc -g -O3 -DGPU -x cu \
\
\
    -DNO_PREALLOCATE_BUFFER \
\
\
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-col-double-nobuf-noyield-fast

nvcc -g -O3 -DGPU -x cu \
    -DSTORE_BY_ROWS         \
    -DFLOAT                 \
\
\
\
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-row-float-buf-noyield-fast

nvcc -g -O3 -DGPU -x cu \
\
    -DFLOAT                 \
\
\
\
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-col-float-buf-noyield-fast

nvcc -g -O3 -DGPU -x cu \
    -DSTORE_BY_ROWS         \
\
\
\
\
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-row-double-buf-noyield-fast

nvcc -g -O3 -DGPU -x cu \
\
\
\
\
\
    $SRC/main.cc $SRC/argparser.cc -o $EXES/cuda-col-double-buf-noyield-fast


### END OF EXECUTABLES #######