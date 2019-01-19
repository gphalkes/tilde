#!/bin/bash

DIR="`dirname \"$0\"`"
. "$DIR/_common.sh"

if [ $# -ne 1 ] ; then
	fail "Usage: runtest.sh <dir with test>"
fi

setup_vars
setup_TEST "$1"
cd_workdir

rm -rf *

cp -r "$TEST"/* . || fail "Could not copy test"
cd context || fail "Could not cd into context dir"
tdreplay -l../replay.log $REPLAYOPTS ../recording || fail "!! Terminal output is different"
cd .. || fail "Could not change back to work dir"

mv context/libt3widgetlog.txt context/log.txt . 2>/dev/null
diff -Nur context after || fail "!! Resulting files are different" >&2

[ "$QUIET" = 1 ] || echo "Test passed" >&2
exit 0
