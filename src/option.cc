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
#include <t3config/config.h>
#include <t3unicode/unicode.h>
#include <t3window/window.h>
#include <t3widget/widget.h>
#include <transcript/transcript.h>

#include "option.h"
#include "util.h"
#include "optionMacros.h"
#include "log.h"

/* FIXME:
  - verify return values of libconfig functions
*/

using namespace std;
using namespace t3_widget;

#define MAX_TAB 16

#ifdef LIBCONFIG_VER_MAJOR
#define INT_TYPE int
#else
#define INT_TYPE long
#endif

cli_options_t cli_option;
runtime_options_t option; /* Merged version of all possible sources. */

options_t term_specific_option;
options_t default_option;

bool config_read_error;
string config_read_error_string;
int config_read_error_line;

static struct { const char *string; t3_attr_t attr; } attribute_map[] = {
	{ "underline", T3_ATTR_UNDERLINE },
	{ "bold", T3_ATTR_BOLD },
	{ "reverse", T3_ATTR_REVERSE },
	{ "blink", T3_ATTR_BLINK },
	{ "dim", T3_ATTR_DIM },

	{ "fg default", T3_ATTR_FG_DEFAULT },
	{ "fg black", T3_ATTR_FG_BLACK },
	{ "fg red", T3_ATTR_FG_RED },
	{ "fg green", T3_ATTR_FG_GREEN },
	{ "fg yellow", T3_ATTR_FG_YELLOW },
	{ "fg blue", T3_ATTR_FG_BLUE },
	{ "fg magenta", T3_ATTR_FG_MAGENTA },
	{ "fg cyan", T3_ATTR_FG_CYAN },
	{ "fg white", T3_ATTR_FG_WHITE },

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


static t3_bool find_term_config(t3_config_t *config, void *data) {
	t3_config_t *name;

	if (t3_config_get_type(config) != T3_CONFIG_SECTION)
		return t3_false;
	if ((name = t3_config_get(config, "name")) == NULL)
		return t3_false;
	if (t3_config_get_type(name) != T3_CONFIG_STRING)
		return t3_false;
	return strcmp((const char *) data, t3_config_get_string(name)) == 0;
}

static t3_attr_t attribute_string_to_bin(const char *attr) {
	size_t i;
	for (i = 0; i < sizeof(attribute_map) / sizeof(attribute_map[0]); i++) {
		if (strcmp(attr, attribute_map[i].string) == 0)
			return attribute_map[i].attr;
	}
	return 0;
}

static void read_config_attribute(const t3_config_t *config, const char *name, opt_t3_attr_t *attr) {
	t3_config_t *attr_config;
	t3_attr_t accumulated_attr = 0;

	if ((attr_config = t3_config_get(config, name)) == NULL)
		return;

	if (t3_config_get_type(attr_config) == T3_CONFIG_STRING) {
		*attr = attribute_string_to_bin(t3_config_get_string(attr_config));
		return;
	} else if (t3_config_get_type(attr_config) != T3_CONFIG_LIST) {
		return;
	}

	for (attr_config = t3_config_get(attr_config, NULL); attr_config != NULL; attr_config = t3_config_get_next(attr_config)) {
		if (t3_config_get_type(attr_config) == T3_CONFIG_STRING)
			accumulated_attr = t3_term_combine_attrs(attribute_string_to_bin(t3_config_get_string(attr_config)), accumulated_attr);
	}

	*attr = accumulated_attr;
}

#define GET_OPT(name, TYPE, type) do { \
	t3_config_t *tmp; \
	if ((tmp = t3_config_get(config, #name)) != NULL && t3_config_get_type(tmp) == T3_CONFIG_##TYPE) \
		opts->name = t3_config_get_##type(tmp); \
} while(0)

static void read_config_part(const t3_config_t *config, options_t *opts) {
	t3_config_t *attributes;

	GET_OPT(tabsize, INT, int);

	GET_OPT(wrap, BOOL, bool);
	GET_OPT(hide_menubar, BOOL, bool);
	GET_OPT(color, BOOL, bool);
	GET_OPT(auto_indent, BOOL, bool);
	GET_OPT(tab_spaces, BOOL, bool);
	GET_OPT(indent_aware_home, BOOL, bool);

	GET_OPT(max_recent_files, INT, int);
	GET_OPT(key_timeout, INT, int);

	attributes = t3_config_get(config, "attributes");
	if (attributes == NULL)
		return;
	read_config_attribute(attributes, "non_print", &opts->non_print);
	read_config_attribute(attributes, "selection_cursor", &opts->selection_cursor);
	read_config_attribute(attributes, "selection_cursor2", &opts->selection_cursor2);
	read_config_attribute(attributes, "bad_draw", &opts->bad_draw);
	read_config_attribute(attributes, "text_cursor", &opts->text_cursor);
	read_config_attribute(attributes, "text", &opts->text);
	read_config_attribute(attributes, "text_selected", &opts->text_selected);
	read_config_attribute(attributes, "highlight", &opts->highlight);
	read_config_attribute(attributes, "highlight_selected", &opts->highlight_selected);
	read_config_attribute(attributes, "dialog", &opts->dialog);
	read_config_attribute(attributes, "dialog_selected", &opts->dialog_selected);
	read_config_attribute(attributes, "button", &opts->button);
	read_config_attribute(attributes, "button_selected", &opts->button_selected);
	read_config_attribute(attributes, "scrollbar", &opts->scrollbar);
	read_config_attribute(attributes, "menubar", &opts->menubar);
	read_config_attribute(attributes, "menubar_selected", &opts->menubar_selected);
	read_config_attribute(attributes, "shadow", &opts->shadow);
}

static void read_config(void) {
	string file = getenv("HOME");
	FILE *config_file;
	t3_config_error_t error;
	t3_config_t *config;
	t3_config_t *term_specific_config;
	const char *term;

	file += "/.tilderc";

	if ((config_file = fopen(file.c_str(), "r")) == NULL)
		return;

	if ((config = t3_config_read_file(config_file, &error, NULL)) == NULL) {
		config_read_error = true;
		config_read_error_string = t3_config_strerror(error.error);
		config_read_error_line = error.line_number;
		goto end;
	}

	/* Note: when supporting later versions, read the config_version key here.
		config_lookup_int(&config, "config_version", &version)
	   For now, we just try to make the best of it when we encouter a version
	   that we do not know.
	*/

	read_config_part(config, &default_option);

	if ((term_specific_config = t3_config_get(config, "terminals")) == NULL)
		goto end;

	if (cli_option.term != NULL)
		term = cli_option.term;
	else if ((term = getenv("TERM")) == NULL)
		goto end;

	if (!t3_config_is_list(term_specific_config))
		goto end;

	if ((term_specific_config = t3_config_find(term_specific_config, find_term_config, (void *) term, NULL)) != NULL)
		read_config_part(term_specific_config, &term_specific_option);

end:
	t3_config_delete(config);
	fclose(config_file);
}

#define SET_OPT_FROM_FILE(name, deflt) do { \
	if (term_specific_option.name.is_valid()) \
		option.name = term_specific_option.name; \
	else if (default_option.name.is_valid()) \
		option.name = default_option.name; \
	else \
		option.name = deflt; \
} while (0)

static void post_process_options(void) {
	SET_OPT_FROM_FILE(tabsize, 8);

	if (default_option.wrap.is_valid())
		option.wrap = default_option.wrap;

	SET_OPT_FROM_FILE(hide_menubar, false);

	if (cli_option.color.is_valid())
		option.color = cli_option.color;
	else
		SET_OPT_FROM_FILE(color, true);

	SET_OPT_FROM_FILE(tab_spaces, false);

	if (default_option.auto_indent.is_valid())
		option.auto_indent = default_option.auto_indent;
	else
		option.auto_indent = true;

	SET_OPT_FROM_FILE(indent_aware_home, true);

	if (default_option.max_recent_files.is_valid())
		option.max_recent_files = default_option.max_recent_files;
	else
		option.max_recent_files = 16;

	if (!cli_option.ask_input_method && term_specific_option.key_timeout.is_valid())
			option.key_timeout = term_specific_option.key_timeout;
}

static void print_help(void) {
	printf("Usage: tilde [<OPTIONS>] [<FILE...>]\n"
		"  -c,--color           Request color mode, overriding config file.\n"
		"  -b,--black-white     Request black & white mode, overriding config file.\n"
		"  -h,--help            Show this help message.\n"
		"  -I,--select-input-method    Ignore configured input handling method.\n"
		"  -T<term>,--terminal=<term>  Use <term> instead of TERM variable.\n"
	    "  -V,--version         Show version and copyright information.\n"
	);
	exit(EXIT_SUCCESS);
}

static void print_version(void) {
	printf("Tilde version <VERSION>\n"
		"Copyright (c) 2011 G.P. Halkes\n"
		"Tilde is licensed under the GNU General Public License version 3\n"); // @copyright
	printf("Library versions:\n  libt3config %ld.%ld.%ld\n  libt3key (through libt3widget) %ld.%ld.%ld\n  libt3unicode %ld.%ld.%ld\n  libt3window %ld.%ld.%ld\n"
		"  libt3widget %ld.%ld.%ld\n  libtranscript %ld.%ld.%ld\n",
		t3_config_get_version() >> 16, (t3_config_get_version() >> 8) & 0xff, t3_config_get_version() & 0xff,
		t3_widget::get_libt3key_version() >> 16, (t3_widget::get_libt3key_version() >> 8) & 0xff, t3_widget::get_libt3key_version() & 0xff,
		t3_unicode_get_version() >> 16, (t3_unicode_get_version() >> 8) & 0xff, t3_unicode_get_version() & 0xff,
		t3_window_get_version() >> 16, (t3_window_get_version() >> 8) & 0xff, t3_window_get_version() & 0xff,
		t3_widget::get_version() >> 16, (t3_widget::get_version() >> 8) & 0xff, t3_widget::get_version() & 0xff,
		transcript_get_version() >> 16, (transcript_get_version() >> 8) & 0xff, transcript_get_version() & 0xff);
//FIXME: add libpcre and libsigc++ versions (from libt3widget)
	exit(EXIT_SUCCESS);
}

PARSE_FUNCTION(parse_args)

	OPTIONS
		OPTION('h', "help", NO_ARG)
			print_help();
		END_OPTION
		OPTION('v', "version", NO_ARG)
			print_version();
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

	read_config();

	post_process_options();
END_FUNCTION

#define SET_ATTR_FROM_FILE(name, const_name) do { \
	if (term_specific_option.name.is_valid()) \
		set_attribute(const_name, term_specific_option.name); \
	else if (default_option.name.is_valid()) \
		set_attribute(const_name, default_option.name); \
} while (0)

void set_attributes(void) {
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

static void set_config_attribute(t3_config_t *config, const char *name, opt_t3_attr_t attr) {
	static t3_attr_t attribute_masks[] = {
		T3_ATTR_FG_MASK,
		T3_ATTR_BG_MASK,
		T3_ATTR_UNDERLINE,
		T3_ATTR_BOLD,
		T3_ATTR_REVERSE,
		T3_ATTR_BLINK,
		T3_ATTR_DIM
	};

	t3_config_t *attributes;
	size_t i, j;

	if (!attr.is_valid())
		return;

	if ((attributes = t3_config_get(config, "attributes")) == NULL || t3_config_get_type(attributes) != T3_CONFIG_SECTION)
		attributes = t3_config_add_section(config, "attributes", NULL);

	config = t3_config_add_list(attributes, name, NULL);

	for (i = 0; i < sizeof(attribute_masks) / sizeof(attribute_masks[0]); i++) {
		t3_attr_t search = attr & attribute_masks[i];
		for (j = 0; j < sizeof(attribute_map) / sizeof(attribute_map[0]); j++) {
			if (attribute_map[j].attr == search) {
				t3_config_add_string(config, NULL, attribute_map[j].string);
				break;
			}
		}
	}
}

#define SET_OPTION(name, type) do { \
	if (options->name.is_valid()) \
		t3_config_add_##type(config, #name, options->name); \
} while (0)

static void set_config_options(t3_config_t *config, options_t *options) {
	SET_OPTION(tabsize, int);

	SET_OPTION(wrap, bool);
	SET_OPTION(hide_menubar, bool);
	SET_OPTION(color, bool);
	SET_OPTION(tab_spaces, bool);
	SET_OPTION(auto_indent, bool);
	SET_OPTION(indent_aware_home, bool);

	SET_OPTION(max_recent_files, int);
	SET_OPTION(key_timeout, int);

	set_config_attribute(config, "non_print", options->non_print);
	set_config_attribute(config, "selection_cursor", options->selection_cursor);
	set_config_attribute(config, "selection_cursor2", options->selection_cursor2);
	set_config_attribute(config, "bad_draw", options->bad_draw);
	set_config_attribute(config, "text_cursor", options->text_cursor);
	set_config_attribute(config, "text", options->text);
	set_config_attribute(config, "text_selected", options->text_selected);
	set_config_attribute(config, "highlight", options->highlight);
	set_config_attribute(config, "highlight_selected", options->highlight_selected);
	set_config_attribute(config, "dialog", options->dialog);
	set_config_attribute(config, "dialog_selected", options->dialog_selected);
	set_config_attribute(config, "button", options->button);
	set_config_attribute(config, "button_selected", options->button_selected);
	set_config_attribute(config, "scrollbar", options->scrollbar);
	set_config_attribute(config, "menubar", options->menubar);
	set_config_attribute(config, "menubar_selected", options->menubar_selected);
	set_config_attribute(config, "shadow", options->shadow);
}

bool write_config(void) {
	string file, new_file;
	FILE *config_file;
	const char *term;
	t3_config_t *config = NULL, *terminals, *terminal_config;
	int version;

	file = getenv("HOME");
	file += "/.tilderc";

	if ((config_file = fopen(file.c_str(), "r")) != NULL) {
		/* Start by reading the existing configuration. */
		t3_config_error_t error;
		config = t3_config_read_file(config_file, &error, NULL);
		fclose(config_file);
	} else if (errno != ENOENT) {
		return false;
	}

	if (config == NULL || (version = t3_config_get_int(t3_config_get(config, "config_version"))) < 1) {
		/* Clean-out config, because it is an old version. */
		t3_config_delete(config);
		config = t3_config_new();
	} else if (version > 1) {
		/* Don't overwrite config files with newer config version. */
		return false;
	} else {
		//FIXME: validate
	}

	default_option.key_timeout.unset();
	set_config_options(config, &default_option);
	t3_config_add_int(config, "config_version", 1);

	if (cli_option.term != NULL)
		term = cli_option.term;
	else
		term = getenv("TERM");

	if (term != NULL) {
		if ((terminals = t3_config_get(config, "terminals")) == NULL || !t3_config_is_list(terminals))
			terminals = t3_config_add_plist(config, "terminals", NULL);

		terminal_config = t3_config_find(terminals, find_term_config, (void *) term, NULL);
		if (terminal_config == NULL) {
			terminal_config = t3_config_add_section(terminals, NULL, NULL);
			t3_config_add_string(terminal_config, "name", term);
		}

		term_specific_option.wrap.unset();
		term_specific_option.max_recent_files.unset();

		set_config_options(terminal_config, &term_specific_option);
	}

	//FIXME: use mkstemp to make new file
	new_file = file + ".new";
	lprintf("Writing config to %s\n", new_file.c_str());
	if ((config_file = fopen(new_file.c_str(), "w")) == NULL) {
		t3_config_delete(config);
		return false;
	}

	if (t3_config_write_file(config, config_file) == T3_ERR_SUCCESS) {
		//FIXME: fflush and fsync before closing
		fclose(config_file);
		t3_config_delete(config);
		return rename(new_file.c_str(), file.c_str()) == 0;
	}
	fclose(config_file);
	t3_config_delete(config);
	return false;
}
