

fail() {
	echo "$@" >&2
	exit 1
}

setup_ldlibrary_path() {
	RELDIR=../../../../
	export LD_LIBRARY_PATH=$RELDIR/config/src/.libs:$RELDIR/highlight/src/.libs:$RELDIR/key/src/.libs:$RELDIR/window/src/.libs:$RELDIR/widget/src/.libs:$RELDIR/../transcript/src/.libs
}

cd_workdir() {
	cd "$DIR" || fail "Could not change to base dir"
	{ [ -d work ] || mkdir work ; } || fail "Could not create work dir"
	cd work || fail "Could not change to work dir"
}

setup_TEST() {
	if [ "${1#/}" = "$1" ] && [ "${1#~/}" = "$1" ] ; then
		TEST="$PWD/$1"
	elif [ "${1#~/}" != "$1" ] ; then
		TEST="$HOME${1#~}"
	else
		TEST="$1"
	fi

	while [ "${TEST%/}" != "$TEST" ] ; do
		TEST="${TEST%/}"
	done
}
