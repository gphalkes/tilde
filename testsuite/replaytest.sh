#!/bin/bash

DIR="`dirname \"$0\"`"
. "$DIR/_common.sh"

if [ $# -ne 1 ] ; then
	fail "Usage: replaytest.sh <dir with test>"
fi

setup_TEST "$1"
cd_workdir

rm -rf *

cp -r "$TEST"/* . || fail "Could not copy test"
cd context || fail "Could not cd into context dir"
../../../../../record/src/tdreplay -d -a 'xmessage "Difference detected"' -e interact $REPLAYOPTS ../recording || fail "!! Terminal output is different"
