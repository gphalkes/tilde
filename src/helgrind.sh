#!/bin/bash
# Copyright (C) 2010,2012 G.P. Halkes
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

make -q -C "$DIR" || make -C "$DIR" || exit 1

if ! [ -x "$DIR/.objects/edit" ] ; then
	echo "Could not find edit executable"
	exit
fi

export LD_LIBRARY_PATH="`sed 's/#.*//' $DIR/Makefile | egrep -o -- '-L[^[:space:]]+' | sed -r \"s%-L%$DIR/%g\" | tr '\n' ':' | sed -r 's/:$//'`"
valgrind --tool=helgrind --log-file="$DIR/helgrind.log" "$DIR/.objects/edit" --L=500 "$@"
