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
#include "option.h"
#include "util.h"
#include "optionMacros.h"
#include "main.h"

options_t option;

static void init_options(void) {
	memset(&option, 0, sizeof(option));
	//FIXME: read default parameters from config file
	option.tabsize = 8;
	//~ option.hide_menubar = true;
	option.max_recent_files = 10;
}

PARSE_FUNCTION(parse_args)
	init_options();

	OPTIONS
		OPTION('w', "wrap", NO_ARG)
			option.wrap = true;
		END_OPTION
		OPTION('T', "tabsize", REQUIRED_ARG)
			PARSE_INT(option.tabsize, 1, MAX_TAB);
		END_OPTION
#ifdef DEBUG
		LONG_OPTION("W", NO_ARG)
			option.wait = true;
		END_OPTION
		LONG_OPTION("L", REQUIRED_ARG)
			PARSE_INT(option.vm_limit, 1, INT_MAX / (1024 * 1024));
		END_OPTION
		LONG_OPTION("D", NO_ARG)
			option.start_debugger_on_segfault = true;
		END_OPTION
#endif
		DOUBLE_DASH
			NO_MORE_OPTIONS;
		END_OPTION

		fatal("No such option " OPTFMT "\n", OPTPRARG);
	NO_OPTION
		//FIXME: do proper handling of passed file names
		option.file = optcurrent;
	END_OPTIONS
END_FUNCTION
