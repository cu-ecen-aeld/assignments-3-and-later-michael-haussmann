#!/bin/bash
if [ "$#" -ne 2 ]; then
  echo 2 parameters expected:
  echo 1.\). directory to be searched
  echo 2.\). string to be searched in files
  exit 1
fi 

if [ ! -d "$1" ]; then
  echo directory $1 does not exist
  exit 1
fi

echo The number of files are `ls -r $1 | wc -l` and the number of matching lines are `grep -ir $2 $1 | wc -l`