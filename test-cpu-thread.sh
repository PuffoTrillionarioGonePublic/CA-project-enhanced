#!/bin/bash

OUTDIR="out"
REPS="1"

function usage()
{
  echo "Do the thing many times, usage:"
  echo ""
  echo "./s.sh --exedir=dir_with_executables --inputdir=dir_with_input [--outputdir=dir_for_output --numreps=number_of_reps]"
  echo ""
}

# parse command line args
while [ "$1" != "" ]; do
  PARAM=`echo $1 | awk -F= '{print $1}'`
  VALUE=`echo $1 | awk -F= '{print $2}'`
  case $PARAM in
    -h | --help)
      usage
      exit
      ;;
    --exedir)
      EXEDIR=$VALUE
      ;;
    --inputdir)
      INPUTDIR=$VALUE
      ;;
    --outputdir)
      OUTDIR=$VALUE
      ;;
    --numreps)
      REPS=$VALUE
      ;;
    *)
      echo "ERROR: unknown parameter \"$PARAM\""
      usage
      exit 1
      ;;
  esac
  shift
done

# check that all required args are set
if [ -z ${EXEDIR+x} ] || [ -z ${INPUTDIR+x} ]; then
  echo "ERROR: some parameter are missing"
  usage
  exit 1
fi

# check that output directory does not exists
if [ -d "$OUTDIR" ]; then
  echo "ERROR: directory already exists"
  exit 1
fi
mkdir $OUTDIR

echo $EXEDIR $INPUTDIR $OUTDIR $REPS

# reduce number of workers to compensate hyperthreading penalty 

# begin of benchmark
echo BEGIN: $(date)

RUN=$(date +%F-%T)

# do the thing
for i in $(seq 1 $REPS); do 
  echo ITERATION $i START AT: $(date)
  for WORKERS in {0..15}; do
    for input in $(ls $INPUTDIR); do
      for exe in $(ls $EXEDIR); do
        cmd="time perf stat record -ddd -o ${OUTDIR}/perf-stat-run-${RUN}-${exe}-workers-${WORKERS}-${input}-${i}.out ${EXEDIR}/${exe} --workers $WORKERS ${INPUTDIR}/${input} &> /dev/null"
        echo "Executing \"$cmd\" ..."
        eval $cmd
      done
    done
  done
  echo ITERATION $i STOP AT: $(date)
done

