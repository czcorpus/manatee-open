#!/bin/bash
set -euo pipefail

if [ $# -ne 3 ]; then
	echo "Usage $0 CORPUS SRC_ATTR DST_ATTR < QUERY_FILE"
	echo "QUERY_FILE has the form of lines containing tab-delimited word string and CQL query"
	exit 1
fi

CORPUS=$1
SRCATTR=$2
DSTATTR=$3
CORPPATH=`corpinfo -p $CORPUS`

for FILE in $CORPPATH/$SRCATTR.lex*; do \
	cp $FILE `echo $FILE | sed "s/$SRCATTR/$DSTATTR/"`; \
done
filterquery2attr $CORPUS $SRCATTR $DSTATTR | mkdtext -g $CORPPATH/$DSTATTR
rm $CORPPATH/${DSTATTR}_tmp.*
