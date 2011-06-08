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
#include "log.h"

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

static t3_attr_t attribute_string_to_bin(const char *attr) {
	size_t i;
	for (i = 0; i < sizeof(attribute_map) / sizeof(attribute_map[0]); i++) {
		if (strcmp(attr, attribute_map[i].string) == 0)
			return attribute_map[i].attr;
	}
	return 0;
}

static void read_config_attribute(const config_setting_t *setting, const char *name, opt_t3_attr_t *attr) {
	config_setting_t *attr_setting;
	const char *value;
	t3_attr_t accumulated_attr = 0;
	int i;

	if ((attr_setting = config_setting_get_member(setting, name)) == NULL)
		return;

	if (config_setting_type(attr_setting) == CONFIG_TYPE_STRING) {
		*attr = attribute_string_to_bin(config_setting_get_string(attr_setting));
		return;
	} else if (!config_setting_is_list(attr_setting) && !config_setting_is_array(attr_setting)) {
		return;
	}

	for (i = 0; (value = config_setting_get_string_elem(attr_setting, i)) != NULL; i++)
		accumulated_attr = t3_term_combine_attrs(attribute_string_to_bin(value), accumulated_attr);

	*attr = accumulated_attr;
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

static void read_config_per_config(const config_setting_t *setting, options_t *opts) {
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
	config_t config;
	config_setting_t *term_specific_setting;
	const char *term;

	file += "/.tilde";

	//FIXME: also take "config_version" key into account
	if ((config_file = fopen(file.c_str(), "r")) == NULL)
		return;

	config_init(&config);
	if (config_read_file(&config, file.c_str()) == CONFIG_FALSE) {
		config_read_error = true;
		config_read_error_string = config_error_text(&config);
		config_read_error_line = config_error_line(&config);
		goto end;
	}

	read_config_per_config(config_root_setting(&config), &default_option);

	if ((term_specific_setting = config_setting_get_member(config_root_setting(&config), "terminals")) == NULL)
		goto end;

	if (cli_option.term != NULL)
		term = cli_option.term;
	else if ((term = getenv("TERM")) == NULL)
		goto end;

	if ((term_specific_setting = config_setting_get_member(term_specific_setting, term)) != NULL)
		read_config_per_config(term_specific_setting, &term_specific_option);

end:
	config_destroy(&config);
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
	if (cli_option.wrap)
		option.wrap = true;
	else if (default_option.wrap.is_valid())
		option.wrap = default_option.wrap;

	if (cli_option.color.is_valid())
		option.color = cli_option.color;
	else
		SET_OPT_FROM_FILE(color, true);

	SET_OPT_FROM_FILE(tabsize, 8);
	SET_OPT_FROM_FILE(hide_menubar, false);

	if (default_option.max_recent_files.is_valid())
		option.max_recent_files = default_option.max_recent_files;
	else
		option.max_recent_files = 16;

	if (!cli_option.ask_input_method && term_specific_option.key_timeout.is_valid())
			option.key_timeout = term_specific_option.key_timeout;
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

static void set_config_attribute(config_setting_t *setting, const char *name, opt_t3_attr_t attr) {
	static t3_attr_t attribute_masks[] = {
		T3_ATTR_FG_MASK,
		T3_ATTR_BG_MASK,
		T3_ATTR_UNDERLINE,
		T3_ATTR_BOLD,
		T3_ATTR_REVERSE,
		T3_ATTR_BLINK,
		T3_ATTR_DIM
	};

	config_setting_t *attributes;
	size_t i, j;

	if (!attr.is_valid())
		return;

	if ((attributes = config_setting_get_member(setting, "attributes")) == NULL || !config_setting_is_group(attributes)) {
		config_setting_remove(setting, "attributes");
		attributes = config_setting_add(setting, "attributes", CONFIG_TYPE_GROUP);
	}
	setting = config_setting_add(attributes, name, CONFIG_TYPE_ARRAY);
	for (i = 0; i < sizeof(attribute_masks) / sizeof(attribute_masks[0]); i++) {
		t3_attr_t search = attr & attribute_masks[i];
		for (j = 0; j < sizeof(attribute_map) / sizeof(attribute_map[0]); j++) {
			if (attribute_map[j].attr == search) {
				config_setting_t *attribute_string = config_setting_add(setting, NULL, CONFIG_TYPE_STRING);
				config_setting_set_string(attribute_string, attribute_map[j].string);
				break;
			}
		}
	}
}

#define SET_OPTION(name, create_type, set_type) do { \
	if (options->name.is_valid()) { \
		config_setting_t *tmp = config_setting_add(setting, #name, create_type); \
		config_setting_set_##set_type(tmp, options->name); \
	} \
} while (0)

static void set_config_options(config_setting_t *setting, options_t *options) {
	SET_OPTION(wrap, CONFIG_TYPE_BOOL, bool);
	SET_OPTION(tabsize, CONFIG_TYPE_INT, int);
	SET_OPTION(hide_menubar, CONFIG_TYPE_BOOL, bool);
	SET_OPTION(color, CONFIG_TYPE_BOOL, bool);
	SET_OPTION(max_recent_files, CONFIG_TYPE_INT, int);
	SET_OPTION(key_timeout, CONFIG_TYPE_INT, int);

	set_config_attribute(setting, "non_print", options->non_print);
	set_config_attribute(setting, "selection_cursor", options->selection_cursor);
	set_config_attribute(setting, "selection_cursor2", options->selection_cursor2);
	set_config_attribute(setting, "bad_draw", options->bad_draw);
	set_config_attribute(setting, "text_cursor", options->text_cursor);
	set_config_attribute(setting, "text", options->text);
	set_config_attribute(setting, "text_selected", options->text_selected);
	set_config_attribute(setting, "highlight", options->highlight);
	set_config_attribute(setting, "highlight_selected", options->highlight_selected);
	set_config_attribute(setting, "dialog", options->dialog);
	set_config_attribute(setting, "dialog_selected", options->dialog_selected);
	set_config_attribute(setting, "button", options->button);
	set_config_attribute(setting, "button_selected", options->button_selected);
	set_config_attribute(setting, "scrollbar", options->scrollbar);
	set_config_attribute(setting, "menubar", options->menubar);
	set_config_attribute(setting, "menubar_selected", options->menubar_selected);
	set_config_attribute(setting, "shadow", options->shadow);
}

bool write_config(void) {
	string file, new_file;
	FILE *config_file;
	config_t config;
	const char *term;
	config_setting_t *root_setting, *terminal_setting, *setting;
	int idx;

	file = getenv("HOME");
	file += "/.tilde";

	config_init(&config);
	if ((config_file = fopen(file.c_str(), "r")) != NULL) {
		/* Start by reading the existing configuration. */
		config_read_file(&config, file.c_str());
		fclose(config_file);
	} else if (errno != ENOENT) {
		return false;
	}

	/* Remove everything that is not a terminal spec. */
	root_setting = config_root_setting(&config);
	idx = 0;
	while ((setting = config_setting_get_elem(root_setting, idx)) != NULL) {
		if (strcmp(config_setting_name(setting), "terminals") == 0)
			idx++;
		else
			config_setting_remove_elem(root_setting, idx);
	}

	default_option.key_timeout.unset();
	set_config_options(root_setting, &default_option);
	setting = config_setting_add(root_setting, "config_version", CONFIG_TYPE_INT);
	config_setting_set_int(setting, 1);

	if (cli_option.term != NULL)
		term = cli_option.term;
	else
		term = getenv("TERM");

	if (term != NULL) {
		if ((terminal_setting = config_lookup(&config, "terminals")) == NULL || !config_setting_is_group(terminal_setting)) {
			config_setting_remove(root_setting, "terminals");
			terminal_setting = config_setting_add(root_setting, "terminals", CONFIG_TYPE_GROUP);
		} else {
			config_setting_remove(terminal_setting, term);
		}
		terminal_setting = config_setting_add(terminal_setting, term, CONFIG_TYPE_GROUP);

		term_specific_option.wrap.unset();
		term_specific_option.max_recent_files.unset();

		set_config_options(terminal_setting, &term_specific_option);
	}

	new_file = file + ".new";
	lprintf("Writing config to %s\n", new_file.c_str());
	if (config_write_file(&config, new_file.c_str()) == CONFIG_TRUE && rename(new_file.c_str(), file.c_str()) == 0) {
		config_destroy(&config);
		return true;
	}
	config_destroy(&config);
	return false;
}
