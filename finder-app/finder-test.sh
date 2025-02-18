#!/bin/sh
# Tester script for assignment 1 and assignment 2
# Author: Siddhant Jajoo

# in order to be called from any directory, we chance dir to
# the one where this script is stored -> assignment 4.2
cd `dirname $0`

set -e
set -u

NUMFILES=10
WRITESTR=AELD_IS_FUN
WRITEDIR=/tmp/aeld-data
username=$(cat /etc/finder-app/conf/username.txt)

# for assignment 4.2, we write all outputs to a file
RESULTSFILE=/tmp/assignment4-results.txt
echo "Results from assigmment 4.2" > ${RESULTSFILE}

if [ $# -lt 3 ]
then
	echo "Using default value ${WRITESTR} for string to write" >> ${RESULTSFILE}
	if [ $# -lt 1 ]
	then
		echo "Using default value ${NUMFILES} for number of files to write" >> ${RESULTSFILE}
	else
		NUMFILES=$1
	fi	
else
	NUMFILES=$1
	WRITESTR=$2
	WRITEDIR=/tmp/aeld-data/$3
fi

MATCHSTR="The number of files are ${NUMFILES} and the number of matching lines are ${NUMFILES}"

echo "Writing ${NUMFILES} files containing string ${WRITESTR} to ${WRITEDIR}" >> ${RESULTSFILE}

rm -rf "${WRITEDIR}"

# create $WRITEDIR if not assignment1
assignment=`cat /etc/finder-app/conf/assignment.txt`

if [ $assignment != 'assignment1' ]
then
	mkdir -p "$WRITEDIR"

	#The WRITEDIR is in quotes because if the directory path consists of spaces, then variable substitution will consider it as multiple argument.
	#The quotes signify that the entire string in WRITEDIR is a single string.
	#This issue can also be resolved by using double square brackets i.e [[ ]] instead of using quotes.
	if [ -d "$WRITEDIR" ]
	then
		echo "$WRITEDIR created" >> ${RESULTSFILE}
	else
		exit 1
	fi
fi
echo "Removing the old writer utility and compiling as a native application" >> ${RESULTSFILE}
# make clean  # commented out as part of assignment 3 - part 1
# make        # commented out as part of assignment 3 - part 1

for i in $( seq 1 $NUMFILES)
do
	./writer "$WRITEDIR/${username}$i.txt" "$WRITESTR"
done

OUTPUTSTRING=$(./finder.sh "$WRITEDIR" "$WRITESTR")

# remove temporary directories
#rm -rf /tmp/aeld-data

set +e
echo ${OUTPUTSTRING} | grep "${MATCHSTR}" >> ${RESULTSFILE}
if [ $? -eq 0 ]; then
	echo "success" >> ${RESULTSFILE}
	exit 0
else
	echo "failed: expected  ${MATCHSTR} in ${OUTPUTSTRING} but instead found" >> ${RESULTSFILE}
	exit 1
fi
