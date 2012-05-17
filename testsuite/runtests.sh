#!/bin/bash

fail() {
	echo "$@" >&2
	exit 1
}

if [ $# -ne 0 ] && [ $# -ne 1 ] ; then
	fail "Usage: runtests.sh [<dir>]"
fi

DIR="`dirname \"$0\"`"

if [ $# -eq 1 ] ; then
	[ "${1#/}" = "$1" ] && [ "${1#~}" = "$1" ] && TEST="$PWD/$1"
else
	TEST="tests"
fi

cd "$DIR" || fail "Could not change to base dir"

LASTLOG_TIME=`find testlog.txt -printf '%TF@%TT' 2>/dev/null`
if [ $? -eq 0 ] ; then
        mv -f testlog.txt "testlog_$LASTLOG_TIME.txt"
fi

export REPLAYOPTS=-k1

failed=0
total=0
export QUIET=1

for i in `find "$TEST" -maxdepth 1 -mindepth 1 -type d | sort` ; do
	echo "=== Testing $i ===" >&2
	echo "=== Testing $i ==="
	if ! "$DIR"/runtest.sh "$i" ; then
		let failed++
		echo "!! $i failed"
	fi
	let total++
done 2> testlog.txt

if [ "$failed" -eq 0 ] ; then
	echo "All tests passed"
else
	echo "!! $failed out of $total tests failed"
fi
