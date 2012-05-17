#!/bin/bash

DIR="`dirname \"$0\"`"
. "$DIR"/_common.sh


echo "!! There is more work to do on this script" >&2

if [ $# -ne 1 ] ; then
	fail "Usage: runtest.sh <dir with test>"
fi

setup_TEST "$1"
cd_workdir

rm -rf *

cp -r "$TEST"/* . || fail "Could not copy test"
cd context || fail "Could not cd into context dir"
#FIXME: use correct terminal (which is not currently recorded!)
#FIXME: display the old one with view to compare with the new one. Ask user
#  afterwards if it was correct

../../../../../record/src/tdrerecord -o ../recording.new $REPLAYOPTS ../recording || fail "!! Could not rerecord test"
cd .. || fail "Could not change back to work dir"

diff -Nur context after || fail "!! Resulting files are different" >&2
mv recording.new recording

mv "$TEST" "$TEST.old"
mkdir "$TEST"
cp -r * "$TEST"
exit 0
