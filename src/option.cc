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
#include <t3highlight/highlight.h>
#include <transcript/transcript.h>

#include "option.h"
#include "util.h"
#include "optionMacros.h"
#include "log.h"

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

term_options_t term_specific_option;
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

static const char config_schema[] = {
#include "config.bytes"
};

static t3_bool find_term_config(const t3_config_t *config, const void *data) {
	if (t3_config_get_type(config) != T3_CONFIG_SECTION)
		return t3_false;
	return strcmp((const char *) data, t3_config_get_string(t3_config_get(config, "name"))) == 0;
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

	for (attr_config = t3_config_get(attr_config, NULL); attr_config != NULL; attr_config = t3_config_get_next(attr_config)) {
		if (t3_config_get_type(attr_config) == T3_CONFIG_STRING)
			accumulated_attr = t3_term_combine_attrs(attribute_string_to_bin(t3_config_get_string(attr_config)), accumulated_attr);
	}

	*attr = accumulated_attr;
}

#define GET_OPT(name, TYPE, type) do { \
	t3_config_t *tmp; \
	if ((tmp = t3_config_get(config, #name)) != NULL) \
		opts->name = t3_config_get_##type(tmp); \
} while(0)
#define GET_ATTRIBUTE(name) read_config_attribute(attributes, #name, &opts->name)
#define GET_HL_ATTRIBUTE(name) read_config_attribute(attributes, name, &opts->highlights[map_highlight(NULL, name)])

static void read_term_config_part(const t3_config_t *config, term_options_t *opts) {
	t3_config_t *attributes;

	GET_OPT(tabsize, INT, int);

	GET_OPT(hide_menubar, BOOL, bool);
	GET_OPT(color, BOOL, bool);

	GET_OPT(key_timeout, INT, int);

	attributes = t3_config_get(config, "attributes");
	if (attributes != NULL) {
		GET_ATTRIBUTE(non_print);
		GET_ATTRIBUTE(selection_cursor);
		GET_ATTRIBUTE(selection_cursor2);
		GET_ATTRIBUTE(bad_draw);
		GET_ATTRIBUTE(text_cursor);
		GET_ATTRIBUTE(text);
		GET_ATTRIBUTE(text_selected);
		GET_ATTRIBUTE(highlight);
		GET_ATTRIBUTE(highlight_selected);
		GET_ATTRIBUTE(dialog);
		GET_ATTRIBUTE(dialog_selected);
		GET_ATTRIBUTE(button);
		GET_ATTRIBUTE(button_selected);
		GET_ATTRIBUTE(scrollbar);
		GET_ATTRIBUTE(menubar);
		GET_ATTRIBUTE(menubar_selected);
		GET_ATTRIBUTE(shadow);
	}
	attributes = t3_config_get(config, "highlight_attributes");
	if (attributes != NULL) {
		GET_HL_ATTRIBUTE("comment");
		GET_HL_ATTRIBUTE("comment-keyword");
		GET_HL_ATTRIBUTE("keyword");
		GET_HL_ATTRIBUTE("number");
		GET_HL_ATTRIBUTE("string");
		GET_HL_ATTRIBUTE("string-escape");
		GET_HL_ATTRIBUTE("misc");
		GET_HL_ATTRIBUTE("variable");
		GET_HL_ATTRIBUTE("error");
		GET_HL_ATTRIBUTE("addition");
		GET_HL_ATTRIBUTE("deletion");
		/* NOTE: normal must always be reset, because unknown attributes get mapped to
		   the same index as normal. Therefore, they would overwrite the correct
		   attribute for normal. */
		opts->highlights[0] = 0;
	}
}

static void read_config(void) {
	string file = getenv("HOME");
	FILE *config_file;
	t3_config_error_t error;
	t3_config_t *config;
	t3_config_schema_t *schema = NULL;
	t3_config_t *term_specific_config;
	const char *term;

	file += "/.tilderc";

	if ((config_file = fopen(file.c_str(), "r")) == NULL) {
		if (errno != ENOENT) {
			config_read_error = true;
			config_read_error_string = strerror(errno);
			config_read_error_line = 0;
		}
		return;
	}

	if ((config = t3_config_read_file(config_file, &error, NULL)) == NULL) {
		config_read_error = true;
		config_read_error_string = t3_config_strerror(error.error);
		config_read_error_line = error.line_number;
		goto end;
	}

	if ((schema = t3_config_read_schema_buffer(config_schema, sizeof(config_schema), &error, NULL)) == NULL) {
		config_read_error = true;
		lprintf("Error loading schema: %d: %s\n", error.line_number, t3_config_strerror(error.error));
		config_read_error_string = t3_config_strerror(error.error == T3_ERR_OUT_OF_MEMORY ? T3_ERR_OUT_OF_MEMORY : T3_ERR_INTERNAL);
		config_read_error_line = 0;
		goto end;
	}

	/* Note: when supporting later versions, read the config_version key here.
	      t3_config_get_int(t3_config_get(config, "config_version"))
	*/

	if (!t3_config_validate(config, schema, &error, 0)) {
		config_read_error = true;
		config_read_error_string = t3_config_strerror(error.error);
		config_read_error_line = error.line_number;
		goto end;
	}

	read_term_config_part(config, &default_option.term_options);

	#define opts (&default_option)
	GET_OPT(wrap, BOOL, bool);
	GET_OPT(auto_indent, BOOL, bool);
	GET_OPT(tab_spaces, BOOL, bool);
	GET_OPT(indent_aware_home, BOOL, bool);
	GET_OPT(strip_spaces, BOOL, bool);
	GET_OPT(make_backup, BOOL, bool);

	GET_OPT(max_recent_files, INT, int);
	#undef opts

	if ((term_specific_config = t3_config_get(config, "terminals")) == NULL)
		goto end;

	if (cli_option.term != NULL)
		term = cli_option.term;
	else if ((term = getenv("TERM")) == NULL)
		goto end;

	if ((term_specific_config = t3_config_find(term_specific_config, find_term_config, term, NULL)) != NULL)
		read_term_config_part(term_specific_config, &term_specific_option);

end:
	t3_config_delete(config);
	fclose(config_file);
}

#define SET_OPT_FROM_FILE(name, deflt) do { \
	if (term_specific_option.name.is_valid()) \
		option.name = term_specific_option.name; \
	else if (default_option.term_options.name.is_valid()) \
		option.name = default_option.term_options.name; \
	else \
		option.name = deflt; \
} while (0)

#define SET_OPT_FROM_DFLT(name, deflt) do { \
	if (default_option.name.is_valid()) \
		option.name = default_option.name; \
	else \
		option.name = deflt; \
} while (0)

static void post_process_options(void) {
	SET_OPT_FROM_FILE(tabsize, 8);
	SET_OPT_FROM_FILE(hide_menubar, false);

	SET_OPT_FROM_DFLT(wrap, false);

	if (cli_option.color.is_valid())
		option.color = cli_option.color;
	else
		SET_OPT_FROM_FILE(color, true);

	SET_OPT_FROM_DFLT(tab_spaces, false);
	SET_OPT_FROM_DFLT(auto_indent, true);
	SET_OPT_FROM_DFLT(indent_aware_home, true);
	SET_OPT_FROM_DFLT(make_backup, false);

	SET_OPT_FROM_DFLT(max_recent_files, 16);
	SET_OPT_FROM_DFLT(strip_spaces, false);

	if (!cli_option.ask_input_method && term_specific_option.key_timeout.is_valid())
			option.key_timeout = term_specific_option.key_timeout;

	option.highlights[0] = 0;
	SET_OPT_FROM_FILE(highlights[1], T3_ATTR_FG_GREEN); // comment
	SET_OPT_FROM_FILE(highlights[2], T3_ATTR_FG_YELLOW); // comment-keyword
	SET_OPT_FROM_FILE(highlights[3], T3_ATTR_FG_CYAN | T3_ATTR_BOLD); // keyword
	SET_OPT_FROM_FILE(highlights[4], T3_ATTR_FG_WHITE | T3_ATTR_BOLD); // number
	SET_OPT_FROM_FILE(highlights[5], T3_ATTR_FG_MAGENTA | T3_ATTR_BOLD); // string
	SET_OPT_FROM_FILE(highlights[6], T3_ATTR_FG_MAGENTA); // string escape
	SET_OPT_FROM_FILE(highlights[7], T3_ATTR_FG_YELLOW | T3_ATTR_BOLD); // misc
	SET_OPT_FROM_FILE(highlights[8], T3_ATTR_FG_GREEN | T3_ATTR_BOLD); // variable
	SET_OPT_FROM_FILE(highlights[9], T3_ATTR_FG_RED | T3_ATTR_BOLD); // error
	SET_OPT_FROM_FILE(highlights[10], T3_ATTR_FG_GREEN | T3_ATTR_BOLD); // addition
	SET_OPT_FROM_FILE(highlights[11], T3_ATTR_FG_RED | T3_ATTR_BOLD); // deletion
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
	printf("Library versions:\n"
		"  libt3config %ld.%ld.%ld\n  libt3highlight %ld.%ld.%ld\n  libt3key (through libt3widget) %ld.%ld.%ld\n"
		"  libt3unicode %ld.%ld.%ld\n  libt3window %ld.%ld.%ld\n"
		"  libt3widget %ld.%ld.%ld\n  libtranscript %ld.%ld.%ld\n",
		t3_config_get_version() >> 16, (t3_config_get_version() >> 8) & 0xff, t3_config_get_version() & 0xff,
		t3_highlight_get_version() >> 16, (t3_highlight_get_version() >> 8) & 0xff, t3_highlight_get_version() & 0xff,
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
	else if (default_option.term_options.name.is_valid()) \
		set_attribute(const_name, default_option.term_options.name); \
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

static void set_config_attribute(t3_config_t *config, const char *section_name, const char *name, opt_t3_attr_t attr) {
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

	if ((attributes = t3_config_get(config, section_name)) == NULL || t3_config_get_type(attributes) != T3_CONFIG_SECTION)
		attributes = t3_config_add_section(config, section_name, NULL);

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
	if (opts->name.is_valid()) \
		t3_config_add_##type(config, #name, opts->name); \
} while (0)
#define SET_ATTRIBUTE(name) set_config_attribute(config, "attributes", #name, opts->name)
#define SET_HL_ATTRIBUTE(x, name) set_config_attribute(config, "highlight_attributes", name, opts->highlights[x])

static void set_term_config_options(t3_config_t *config, term_options_t *opts) {
	SET_OPTION(tabsize, int);

	SET_OPTION(hide_menubar, bool);
	SET_OPTION(color, bool);

	SET_OPTION(key_timeout, int);

	SET_ATTRIBUTE(non_print);
	SET_ATTRIBUTE(selection_cursor);
	SET_ATTRIBUTE(selection_cursor2);
	SET_ATTRIBUTE(bad_draw);
	SET_ATTRIBUTE(text_cursor);
	SET_ATTRIBUTE(text);
	SET_ATTRIBUTE(text_selected);
	SET_ATTRIBUTE(highlight);
	SET_ATTRIBUTE(highlight_selected);
	SET_ATTRIBUTE(dialog);
	SET_ATTRIBUTE(dialog_selected);
	SET_ATTRIBUTE(button);
	SET_ATTRIBUTE(button_selected);
	SET_ATTRIBUTE(scrollbar);
	SET_ATTRIBUTE(menubar);
	SET_ATTRIBUTE(menubar_selected);
	SET_ATTRIBUTE(shadow);

	int i;
	const char *highlight_name;
	for (i = 1; (highlight_name = reverse_map_highlight(i)) != NULL; i++)
		SET_HL_ATTRIBUTE(i, highlight_name);
}

bool write_config(void) {
	string file, new_file;
	FILE *config_file;
	const char *term;
	t3_config_t *config = NULL, *terminals, *terminal_config;
	t3_config_schema_t *schema;
	int version;

	file = getenv("HOME");
	file += "/.tilderc";

	//FIXME: verify return values

	if ((schema = t3_config_read_schema_buffer(config_schema, sizeof(config_schema), NULL, NULL)) == NULL)
		return false;

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
		t3_config_delete_schema(schema);
		return false;
	} else {
		if (!t3_config_validate(config, schema, NULL, 0)) {
			t3_config_delete_schema(schema);
			return false;
		}
	}

	default_option.term_options.key_timeout.unset();
	set_term_config_options(config, &default_option.term_options);

	#define opts (&default_option)
	SET_OPTION(wrap, bool);
	SET_OPTION(tab_spaces, bool);
	SET_OPTION(auto_indent, bool);
	SET_OPTION(indent_aware_home, bool);
	SET_OPTION(strip_spaces, bool);
	SET_OPTION(make_backup, bool);

	SET_OPTION(max_recent_files, int);
	#undef opts

	t3_config_add_int(config, "config_version", 1);

	if (cli_option.term != NULL)
		term = cli_option.term;
	else
		term = getenv("TERM");

	if (term != NULL) {
		if ((terminals = t3_config_get(config, "terminals")) == NULL || !t3_config_is_list(terminals))
			terminals = t3_config_add_plist(config, "terminals", NULL);

		terminal_config = t3_config_find(terminals, find_term_config, term, NULL);
		if (terminal_config == NULL) {
			terminal_config = t3_config_add_section(terminals, NULL, NULL);
			t3_config_add_string(terminal_config, "name", term);
		}

		set_term_config_options(terminal_config, &term_specific_option);
	}

	/* Validate config using schema, such that we can be sure that we won't
	   say it is invalid when reading. That would fit nicely into the
	   "bad things" category. */
	if (!t3_config_validate(config, schema, NULL, 0)) {
		t3_config_delete_schema(schema);
		t3_config_delete(config);
		return false;
	}
	t3_config_delete_schema(schema);

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
