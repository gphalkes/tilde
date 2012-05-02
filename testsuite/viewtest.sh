#!/bin/bash

fail() {
	echo "$@" >&2
	exit 1
}

if [ $# -ne 1 ] ; then
	fail "Usage: runtest.sh <dir with test>"
fi

[ "${1#/}" = "$1" ] && [ "${1#~}" = "$1" ] && TEST="$PWD/$1"

DIR="`dirname \"$0\"`"

cd "$DIR" || fail "Could not change to base dir"
{ [ -d work ] || mkdir work ; } || fail "Could not create work dir"
cd work || fail "Could not change to work dir"

rm -rf *

cp -r "$TEST"/* . || fail "Could not copy test"
cd context || fail "Could not cd into context dir"
../../../../record/src/view $REPLAYOPTS ../recording || fail "!! Terminal output is different"
