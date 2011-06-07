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
#include <libconfig.h>

#include "option.h"
#include "util.h"
#include "optionMacros.h"

using namespace std;
using namespace t3_widget;

#define MAX_TAB 16

cli_options_t cli_option;
runtime_options_t option; /* Merged version of all possible sources. */

options_t term_specific_option;
options_t default_option;

bool config_read_error;
string config_read_error_string;
int config_read_error_line;

static struct { const char *string; t3_attr_t attr; } map[] = {
	{ "underline", T3_ATTR_UNDERLINE },
	{ "bold", T3_ATTR_BOLD },
	{ "reverse", T3_ATTR_REVERSE },
	{ "blink", T3_ATTR_BLINK },
	{ "dim", T3_ATTR_DIM },

	{ "fg default", T3_ATTR_FG_DEFAULT },
	{ "black", T3_ATTR_FG_BLACK },
	{ "red", T3_ATTR_FG_RED },
	{ "green", T3_ATTR_FG_GREEN },
	{ "yellow", T3_ATTR_FG_YELLOW },
	{ "blue", T3_ATTR_FG_BLUE },
	{ "magenta", T3_ATTR_FG_MAGENTA },
	{ "cyan", T3_ATTR_FG_CYAN },
	{ "white", T3_ATTR_FG_WHITE },

	{ "bg default", T3_ATTR_BG_DEFAULT },
	{ "bg black", T3_ATTR_BG_BLACK },
	{ "bg red", T3_ATTR_BG_RED },
	{ "bg green", T3_ATTR_BG_GREEN },
	{ "bg yellow", T3_ATTR_BG_YELLOW },
	{ "bg blue", T3_ATTR_BG_BLUE },
	{ "bg magenta", T3_ATTR_BG_MAGENTA },
	{ "bg cyan", T3_ATTR_BG_CYAN },
	{ "bg white", T3_ATTR_BG_WHITE }
};

static t3_attr_t attribute_string_to_bin(const char *attr) {
	size_t i;
	for (i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
		if (strcmp(attr, map[i].string) == 0)
			return map[i].attr;
	}
	return 0;
}

static void read_config_attribute(const config_setting_t *setting, const char *name, opt_t3_attr_t *attr) {
	config_setting_t *list;
	const char *value;
	int i;

	if (config_setting_lookup_string(setting, name, &value) == CONFIG_TRUE) {
		*attr = attribute_string_to_bin(value);
		return;
	}

	if ((list = config_setting_get_member(setting, name)) == NULL)
		return;

	for (i = 0; (value = config_setting_get_string_elem(list, i)) != NULL; i++) {
		t3_attr_t next_attr = attribute_string_to_bin(value);
		*attr = t3_term_combine_attrs(next_attr, *attr);
	}
}

#define GET_OPT_BOOL(name) do { \
	int tmp; \
	if (config_setting_lookup_bool(setting, #name, &tmp) == CONFIG_TRUE) \
		opts->name = tmp; \
} while(0)

#define GET_OPT_INT(name) do { \
	long tmp; \
	if (config_setting_lookup_int(setting, #name, &tmp) == CONFIG_TRUE) \
		opts->name = tmp; \
} while(0)

static void read_options_per_config(const config_setting_t *setting, options_t *opts) {
	config_setting_t *attributes;
	GET_OPT_BOOL(wrap);
	GET_OPT_INT(tabsize);
	GET_OPT_BOOL(hide_menubar);
	GET_OPT_BOOL(color);
	GET_OPT_INT(max_recent_files);
	GET_OPT_INT(key_timeout);

	attributes = config_setting_get_member(setting, "attributes");
	if (attributes == NULL)
		return;
	read_config_attribute(attributes, "non_print", &opts->non_print);
}


static void read_options(void) {
	string file = getenv("HOME");
	FILE *config_file;
	config_t config;
	config_setting_t *term_specific_setting;

	file += "/.tilde";

	if ((config_file = fopen(file.c_str(), "r")) == NULL) {
		if (errno == ENOENT) {
			if ((config_file = fopen(file.c_str(), "w+")) != NULL) {
				fprintf(config_file, "# Default empty config file\n");
				fclose(config_file);
			}
		}
		return;
	}

	config_init(&config);
	if (config_read_file(&config, file.c_str()) == CONFIG_FALSE) {
		config_read_error = true;
		config_read_error_string = config_error_text(&config);
		config_read_error_line = config_error_line(&config);
		config_destroy(&config);
		return;
	}

	read_options_per_config(config_root_setting(&config), &default_option);
	#warning FIXME: do not use the result of getenv directly!!! It may be NULL!
	if ((term_specific_setting = config_setting_get_member(config_root_setting(&config), getenv("TERM"))) != NULL)
		read_options_per_config(term_specific_setting, &term_specific_option);
	config_destroy(&config);
}

#define SET_OPT_FROM_FILE(name, deflt) do { \
	if (term_specific_option.name.is_valid()) \
		option.name = term_specific_option.name; \
	else if (default_option.name.is_valid()) \
		option.name = default_option.name; \
	else \
		option.name = deflt; \
} while (0)

#define SET_ATTR_FROM_FILE(name, const_name) do { \
	if (term_specific_option.name.is_valid()) \
		set_attribute(const_name, term_specific_option.name); \
	else if (default_option.name.is_valid()) \
		set_attribute(const_name, default_option.name); \
} while (0)

static void post_process_options(void) {
	//FIXME: doesn't make much sense to do this by terminal
	if (cli_option.wrap)
		option.wrap = true;
	else
		SET_OPT_FROM_FILE(wrap, false);

	if (cli_option.color.is_valid())
		option.color = cli_option.color;
	else
		SET_OPT_FROM_FILE(color, true);

	SET_OPT_FROM_FILE(tabsize, 8);
	SET_OPT_FROM_FILE(hide_menubar, false);
	//FIXME: doesn't make much sense to do this by terminal
	SET_OPT_FROM_FILE(max_recent_files, 16);

	if (!cli_option.ask_input_method && term_specific_option.key_timeout.is_valid())
			option.key_timeout = term_specific_option.key_timeout;

	SET_ATTR_FROM_FILE(non_print, attribute_t::NON_PRINT);
	SET_ATTR_FROM_FILE(selection_cursor, attribute_t::SELECTION_CURSOR);
	SET_ATTR_FROM_FILE(selection_cursor2, attribute_t::SELECTION_CURSOR2);
	SET_ATTR_FROM_FILE(bad_draw, attribute_t::BAD_DRAW);
	SET_ATTR_FROM_FILE(text_cursor, attribute_t::TEXT_CURSOR);
	SET_ATTR_FROM_FILE(text, attribute_t::TEXT);
	SET_ATTR_FROM_FILE(text_selected, attribute_t::TEXT_SELECTED);
	SET_ATTR_FROM_FILE(highlight, attribute_t::HIGHLIGHT);
	SET_ATTR_FROM_FILE(highlight_selected, attribute_t::HIGHLIGHT_SELECTED);
	SET_ATTR_FROM_FILE(dialog, attribute_t::DIALOG);
	SET_ATTR_FROM_FILE(dialog_selected, attribute_t::DIALOG_SELECTED);
	SET_ATTR_FROM_FILE(button, attribute_t::BUTTON);
	SET_ATTR_FROM_FILE(button_selected, attribute_t::BUTTON_SELECTED);
	SET_ATTR_FROM_FILE(scrollbar, attribute_t::SCROLLBAR);
	SET_ATTR_FROM_FILE(menubar, attribute_t::MENUBAR);
	SET_ATTR_FROM_FILE(menubar_selected, attribute_t::MENUBAR_SELECTED);
	SET_ATTR_FROM_FILE(shadow, attribute_t::SHADOW);
}

PARSE_FUNCTION(parse_args)

	OPTIONS
		OPTION('w', "wrap", NO_ARG)
			cli_option.wrap = true;
		END_OPTION
		OPTION('T', "term", REQUIRED_ARG)
			cli_option.term = optArg;
		END_OPTION
		OPTION('b', "black-white", NO_ARG)
			cli_option.color = false;
		END_OPTION
		OPTION('c', "color", NO_ARG)
			cli_option.color = true;
		END_OPTION
		OPTION('I', "select-input-method", NO_ARG)
			cli_option.ask_input_method = true;
		END_OPTION
#ifdef DEBUG
		LONG_OPTION("W", NO_ARG)
			cli_option.wait = true;
		END_OPTION
		LONG_OPTION("L", REQUIRED_ARG)
			PARSE_INT(cli_option.vm_limit, 1, INT_MAX / (1024 * 1024));
		END_OPTION
		LONG_OPTION("D", NO_ARG)
			cli_option.start_debugger_on_segfault = true;
		END_OPTION
#endif
		DOUBLE_DASH
			NO_MORE_OPTIONS;
		END_OPTION

		fatal("No such option " OPTFMT "\n", OPTPRARG);
	NO_OPTION
		cli_option.files.push_back(optcurrent);
	END_OPTIONS

	read_options();

	post_process_options();
END_FUNCTION

