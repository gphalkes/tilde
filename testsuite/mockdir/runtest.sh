#!/bin/bash

die() {
	echo "$*" >&2
	exit 1
}

cd "$(dirname "$0")" || die "could not cd to correct dir"

rm -rf work || die "could not remove stale work dir"
mkdir -p work/dir1 || die "could not create work/dir1"
echo "foo" > work/file1 || die "could not create work/file1"

cd work || die "could not cd to work dir"

{
	{ ../mockdir_test | sed -r "s%$PWD%{CWD}%" ; } || die "mockdir_test failed"
	LD_PRELOAD=../.libs/libmockdir.so ../mockdir_test || die "mockdir_test/getcwd failed"
	LD_PRELOAD=../.libs/libmockdir.so MOCK_DIR=.. ../mockdir_test || die "mockdir_test/.. failed"
} > testlog.txt

diff -u ../expected_result.txt testlog.txt || die "test failed"
exit 0

