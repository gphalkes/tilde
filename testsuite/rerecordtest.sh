#!/bin/bash

DIR="`dirname \"$0\"`"
. "$DIR"/_common.sh

confirm() {
	unset CONFIRM
	while [[ -z $CONFIRM ]] ; do
		read CONFIRM
	done
}

echo "!! There is more work to do on this script" >&2

if [ $# -ne 1 ] ; then
	fail "Usage: runtest.sh <dir with test>"
fi

setup_TEST "$1"
setup_vars

[ -d "$TEST.new" ] && rm -rf "$TEST.new"

cd_workdir

rm -rf *

cp -r "$TEST"/* . || fail "Could not copy test"
cd context || fail "Could not cd into context dir"
#FIXME: use correct terminal (which is not currently recorded!)
#FIXME: display the old one with view to compare with the new one. Ask user
#  afterwards if it was correct

tdrerecord -o ../recording.new $REPLAYOPTS ../recording || fail "!! Could not rerecord test"
fixup_test ../recording.new
cd .. || fail "Could not change back to work dir"

tdcompare -v recording recording.new || echo "WARNING: visual differences"

rm context/libt3widgetlog.txt context/log.txt

diff -Nurq context after || fail "!! Resulting files are different" >&2

cp -r "$TEST" "$TEST.new"
mv recording.new "$TEST.new"/recording
dwdiff -Pc -C0 "$TEST"/recording "$TEST.new"/recording

echo "Do you want to save the changes? "

confirm
if [[ $CONFIRM = y ]] ; then
	rm -rf "$TEST"
	mv "$TEST.new" "$TEST"
else
	echo "Do you want to delete the new files? "
	confirm
	if [[ $CONFIRM = y ]] ; then
		rm -rf "$TEST.new"
	fi
fi

exit 0
