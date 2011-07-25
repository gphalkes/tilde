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
#include <t3window/window.h>

#include "util.h"

OPT_TYPE(bool);
OPT_TYPE(size_t);
OPT_TYPE(t3_attr_t);
OPT_TYPE(int);

typedef struct {
	/* Options to override config file. */
	opt_bool color;

	const char *term;
	bool ask_input_method;

	std::list<const char *> files;

#ifdef DEBUG
	bool wait;
	int vm_limit;
	bool start_debugger_on_segfault;
#endif

} cli_options_t;

typedef struct {
	opt_bool wrap;
	opt_bool tab_spaces;
	opt_int tabsize;

	opt_bool hide_menubar;
	opt_bool color;
	opt_bool auto_indent;

	opt_size_t max_recent_files;

	opt_int key_timeout;

	opt_t3_attr_t non_print;
	opt_t3_attr_t selection_cursor;
	opt_t3_attr_t selection_cursor2;
	opt_t3_attr_t bad_draw;
	opt_t3_attr_t text_cursor;
	opt_t3_attr_t text;
	opt_t3_attr_t text_selected;
	/* High-light attributes for hot keys. */
	opt_t3_attr_t highlight;
	opt_t3_attr_t highlight_selected;

	opt_t3_attr_t dialog;
	opt_t3_attr_t dialog_selected;
	opt_t3_attr_t button;
	opt_t3_attr_t button_selected;
	opt_t3_attr_t scrollbar;
	opt_t3_attr_t menubar;
	opt_t3_attr_t menubar_selected;

	opt_t3_attr_t shadow;
} options_t;

typedef struct {
	bool wrap;
	bool tab_spaces;
	int tabsize;
	bool hide_menubar;
	bool color;
	bool auto_indent;
	size_t max_recent_files;
	opt_int key_timeout;
} runtime_options_t;

extern cli_options_t cli_option;
extern runtime_options_t option;

extern options_t term_specific_option;
extern options_t default_option;

void parse_args(int argc, char **argv);
void set_attributes(void);
bool write_config(void);
#endif
