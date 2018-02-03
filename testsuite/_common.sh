

fail() {
	echo "$@" >&2
	exit 1
}

cd_workdir() {
	cd "$DIR" || fail "Could not change to base dir"
	{ [ -d work ] || mkdir work ; } || fail "Could not create work dir"
	cd work || fail "Could not change to work dir"
}

setup_TEST() {
	make --no-print-directory --quiet -C mockdir
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

fixup_test() {
	sed -i -r '/^send.*033\[2;.R/s/send +[0-9]+/send 0/' "$1"
}
