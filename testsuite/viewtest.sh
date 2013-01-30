#!/bin/bash

DIR="`dirname \"$0\"`"
. "$DIR/_common.sh"

if [ $# -ne 1 ] ; then
	fail "Usage: viewtest.sh <dir with test>"
fi

setup_TEST "$1"
cd_workdir

rm -rf *

cp -r "$TEST"/* . || fail "Could not copy test"
cd context || fail "Could not cd into context dir"
tdview $REPLAYOPTS ../recording || fail "!! Terminal output is different"
