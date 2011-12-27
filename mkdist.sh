#!/bin/bash

cd "`dirname \"$0\"`"

. ../../repo-scripts/mkdist_funcs.sh

setup_hg
get_version_hg
check_mod_hg
build_all
get_sources_hg
make_tmpdir
copy_sources ${SOURCES} ${GENSOURCES} ${AUXSOURCES}
copy_dist_files
copy_files `hg manifest | egrep ^man/`
create_configure

if [[ "${VERSION}" =~ [0-9]{8} ]] ; then
	VERSION_BIN=1
else
	VERSION_BIN="$(printf "0x%02x%02x%02x" $(echo ${VERSION} | tr '.' ' '))"
fi

sed -i "s/<VERSION>/${VERSION}/g;s/<DATE>/${DATE}/g" `find ${TOPDIR} -type f`

OBJECTS="`echo \"${SOURCES} ${GENSOURCES} ${AUXSOURCES}\" | tr ' ' '\n' | sed -r 's%\.objects/%%' | egrep '^src/.*\.cc$' | sed -r 's/\.cc\>/.o/g' | tr '\n' ' '`"

sed -r -i "s%<OBJECTS>%${OBJECTS}%g;
s%<VERSIONINFO>%${VERSIONINFO}%g" ${TOPDIR}/Makefile.in

update_pkgconfig_versions
create_tar
