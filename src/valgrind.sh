#!/bin/bash
# Copyright (C) 2010 G.P. Halkes
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3, as
# published by the Free Software Foundation.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

DIR="`dirname \"$0\"`"

if ! [ -x "$DIR/.objects/edit" ] ; then
	echo "Could not find edit executable"
	exit
fi

export LD_LIBRARY_PATH="$DIR/widget/.libs:$DIR/window/.libs:$DIR/key/.libs:$DIR/unicode/.libs:$DIR/transcript/.libs"
valgrind --tool=memcheck --leak-check=full --show-reachable=yes \
	--log-file="$DIR/valgrind.log" "$DIR/.objects/edit" --L=250 "$@"
