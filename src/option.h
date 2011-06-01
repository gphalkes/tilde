/* Copyright (C) 2011 G.P. Halkes
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 3, as
   published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef OPTIONS_H
#define OPTIONS_H

#include <cstdlib>
#include <list>
using namespace std;

typedef struct {
	bool wrap;
	int tabsize;

	const char *term;
	bool hide_menubar;
	bool color;
#ifdef DEBUG
	bool wait;
	int vm_limit;
	bool start_debugger_on_segfault;
#endif

	list<const char *> files;
	size_t max_recent_files;
} options_t;

extern options_t option;

void parse_args(int argc, char **argv);

#endif
