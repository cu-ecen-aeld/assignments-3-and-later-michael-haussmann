#!/bin/bash
if [ "$#" -ne 2 ]; then
  echo 2 parameters expected:
  echo 1.\). file to write string to
  echo 2.\). string to write in the file
  exit 1
fi 

d=$(dirname "$1")
if [ ! -d $d ]; then
  echo directory $d does not exist, creating it
  mkdir -p $d
fi

echo writing string $2 to file $1 \(overwritting\)
echo $2 > $1
