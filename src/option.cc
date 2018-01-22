/* Copyright (C) 2011-2012 G.P. Halkes
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
#include <t3highlight/highlight.h>
#include <t3widget/widget.h>
#include <t3window/window.h>
#include <transcript/transcript.h>
#include <unistring/version.h>

#include "log.h"
#include "option.h"
#include "optionMacros.h"
#include "util.h"

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
std::string config_read_error_string;
int config_read_error_line;

static struct {
  const char *string;
  t3_attr_t attr;
} attribute_map[] = {{"underline", T3_ATTR_UNDERLINE},
                     {"bold", T3_ATTR_BOLD},
                     {"reverse", T3_ATTR_REVERSE},
                     {"blink", T3_ATTR_BLINK},
                     {"dim", T3_ATTR_DIM},

                     {"fg default", T3_ATTR_FG_DEFAULT},
                     {"fg black", T3_ATTR_FG_BLACK},
                     {"fg red", T3_ATTR_FG_RED},
                     {"fg green", T3_ATTR_FG_GREEN},
                     {"fg yellow", T3_ATTR_FG_YELLOW},
                     {"fg blue", T3_ATTR_FG_BLUE},
                     {"fg magenta", T3_ATTR_FG_MAGENTA},
                     {"fg cyan", T3_ATTR_FG_CYAN},
                     {"fg white", T3_ATTR_FG_WHITE},

                     {"bg default", T3_ATTR_BG_DEFAULT},
                     {"bg black", T3_ATTR_BG_BLACK},
                     {"bg red", T3_ATTR_BG_RED},
                     {"bg green", T3_ATTR_BG_GREEN},
                     {"bg yellow", T3_ATTR_BG_YELLOW},
                     {"bg blue", T3_ATTR_BG_BLUE},
                     {"bg magenta", T3_ATTR_BG_MAGENTA},
                     {"bg cyan", T3_ATTR_BG_CYAN},
                     {"bg white", T3_ATTR_BG_WHITE}};

static const char config_schema[] = {
#include "config.bytes"
};

static const char base_config_schema[] = {
#include "config.bytes"
};

static t3_bool find_term_config(const t3_config_t *config, const void *data) {
  if (t3_config_get_type(config) != T3_CONFIG_SECTION) return t3_false;
  return strcmp((const char *)data, t3_config_get_string(t3_config_get(config, "name"))) == 0;
}

static t3_attr_t attribute_string_to_bin(const char *attr) {
  size_t i;
  bool foreground;
  char *endptr;
  int color;

  for (i = 0; i < ARRAY_SIZE(attribute_map); i++) {
    if (strcmp(attr, attribute_map[i].string) == 0) return attribute_map[i].attr;
  }

  if (strncmp(attr, "fg ", 3) == 0)
    foreground = true;
  else if (strncmp(attr, "bg ", 3) == 0)
    foreground = false;
  else
    return 0;

  color = strtol(attr + 3, &endptr, 0);
  if (*endptr != 0) return 0;
  if (color < 0 || color > 255) return 0;
  return foreground ? T3_ATTR_FG(color) : T3_ATTR_BG(color);
}

static void read_config_attribute(const t3_config_t *config, const char *name,
                                  optional<t3_attr_t> *attr) {
  t3_config_t *attr_config;
  t3_attr_t accumulated_attr = 0;

  if ((attr_config = t3_config_get(config, name)) == nullptr) return;

  for (attr_config = t3_config_get(attr_config, nullptr); attr_config != nullptr;
       attr_config = t3_config_get_next(attr_config)) {
    if (t3_config_get_type(attr_config) == T3_CONFIG_STRING)
      accumulated_attr = t3_term_combine_attrs(
          attribute_string_to_bin(t3_config_get_string(attr_config)), accumulated_attr);
  }

  *attr = accumulated_attr;
}

#define GET_OPT(name, TYPE, type)                                                             \
  do {                                                                                        \
    t3_config_t *tmp;                                                                         \
    if ((tmp = t3_config_get(config, #name)) != NULL) opts->name = t3_config_get_##type(tmp); \
  } while (false)
#define GET_ATTRIBUTE(name) read_config_attribute(attributes, #name, &opts->name)
#define GET_HL_ATTRIBUTE(name) \
  read_config_attribute(attributes, name, &opts->highlights[map_highlight(NULL, name)])

static void read_term_config_part(const t3_config_t *config, term_options_t *opts) {
  t3_config_t *attributes;

  GET_OPT(color, BOOL, bool);

  GET_OPT(key_timeout, INT, int);

  attributes = t3_config_get(config, "attributes");
  if (attributes != nullptr) {
    GET_ATTRIBUTE(non_print);
    GET_ATTRIBUTE(text_selection_cursor);
    GET_ATTRIBUTE(text_selection_cursor2);
    GET_ATTRIBUTE(bad_draw);
    GET_ATTRIBUTE(text_cursor);
    GET_ATTRIBUTE(text);
    GET_ATTRIBUTE(text_selected);
    GET_ATTRIBUTE(hotkey_highlight);
    GET_ATTRIBUTE(dialog);
    GET_ATTRIBUTE(dialog_selected);
    GET_ATTRIBUTE(button_selected);
    GET_ATTRIBUTE(scrollbar);
    GET_ATTRIBUTE(menubar);
    GET_ATTRIBUTE(menubar_selected);
    GET_ATTRIBUTE(shadow);
    GET_ATTRIBUTE(meta_text);
    GET_ATTRIBUTE(background);

    GET_ATTRIBUTE(brace_highlight);
  }
  attributes = t3_config_get(config, "highlight_attributes");
  if (attributes != nullptr) {
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

static void read_base_config() {
  cleanup_func2_ptr<FILE, int, fclose>::t config_file;
  t3_config_error_t error;
  cleanup_func_ptr<t3_config_t, t3_config_delete>::t config;
  cleanup_func_ptr<t3_config_schema_t, t3_config_delete_schema>::t schema;

  if ((config_file = fopen(DATADIR "/"
                                   "base.config",
                           "r")) == nullptr) {
    lprintf("Failed to open file: %s %m\n", DATADIR
            "/"
            "base.config");
    return;
  }

  if ((config = t3_config_read_file(config_file, &error, nullptr)) == nullptr) {
    lprintf("Error loading base config: %d: %s\n", error.line_number,
            t3_config_strerror(error.error));
    return;
  }

  if ((schema = t3_config_read_schema_buffer(base_config_schema, sizeof(base_config_schema), &error,
                                             nullptr)) == nullptr) {
    lprintf("Error loading schema: %d: %s\n", error.line_number, t3_config_strerror(error.error));
    return;
  }

  if (!t3_config_validate(config, schema, &error, 0)) {
    lprintf("Error validating base config: %d: %s\n", error.line_number,
            t3_config_strerror(error.error));
    return;
  }

  for (t3_config_t *lang = t3_config_get(t3_config_get(config, "lang"), nullptr); lang != nullptr;
       lang = t3_config_get_next(lang)) {
    const char *name = t3_config_get_string(t3_config_get(lang, "name"));
    const char *line_comment = t3_config_get_string(t3_config_get(lang, "line_comment"));
    if (name != nullptr && line_comment != nullptr)
      option.line_comment_map[std::string(name)] = line_comment;
  }
}

static void read_config() {
  cleanup_func2_ptr<FILE, int, fclose>::t config_file;
  t3_config_error_t error;
  cleanup_func_ptr<t3_config_t, t3_config_delete>::t config;
  cleanup_func_ptr<t3_config_schema_t, t3_config_delete_schema>::t schema;
  t3_config_t *term_specific_config;
  const char *term;

  if (cli_option.config_file == nullptr)
    config_file = t3_config_xdg_open_read(T3_CONFIG_XDG_CONFIG_HOME, "tilde", "config");
  else
    config_file = fopen(cli_option.config_file, "r");

  if (config_file == nullptr) {
    if (errno != ENOENT) {
      config_read_error = true;
      config_read_error_string = strerror(errno);
      config_read_error_line = 0;
    }
    return;
  }

  if ((config = t3_config_read_file(config_file, &error, nullptr)) == nullptr) {
    config_read_error = true;
    config_read_error_string = t3_config_strerror(error.error);
    config_read_error_line = error.line_number;
    return;
  }

  if ((schema = t3_config_read_schema_buffer(config_schema, sizeof(config_schema), &error,
                                             nullptr)) == nullptr) {
    config_read_error = true;
    lprintf("Error loading schema: %d: %s\n", error.line_number, t3_config_strerror(error.error));
    config_read_error_string = t3_config_strerror(
        error.error == T3_ERR_OUT_OF_MEMORY ? T3_ERR_OUT_OF_MEMORY : T3_ERR_INTERNAL);
    config_read_error_line = 0;
    return;
  }

  if (!t3_config_validate(config, schema, &error, 0)) {
    config_read_error = true;
    config_read_error_string = t3_config_strerror(error.error);
    config_read_error_line = error.line_number;
    return;
  }

  read_term_config_part(config, &default_option.term_options);

#define opts (&default_option)
  GET_OPT(wrap, BOOL, bool);
  GET_OPT(auto_indent, BOOL, bool);
  GET_OPT(tab_spaces, BOOL, bool);
  GET_OPT(indent_aware_home, BOOL, bool);
  GET_OPT(show_tabs, BOOL, bool);
  GET_OPT(strip_spaces, BOOL, bool);
  GET_OPT(make_backup, BOOL, bool);
  GET_OPT(hide_menubar, BOOL, bool);
  GET_OPT(parse_file_positions, BOOL, bool);
  GET_OPT(disable_primary_selection_over_ssh, BOOL, bool);

  GET_OPT(tabsize, INT, int);
  GET_OPT(max_recent_files, INT, int);
#undef opts

  for (t3_config_t *lang = t3_config_get(t3_config_get(config, "lang"), nullptr); lang != nullptr;
       lang = t3_config_get_next(lang)) {
    const char *name = t3_config_get_string(t3_config_get(lang, "name"));
    const char *line_comment = t3_config_get_string(t3_config_get(lang, "line_comment"));
    if (name != nullptr && line_comment != nullptr)
      option.line_comment_map[std::string(name)] = line_comment;
  }

  if ((term_specific_config = t3_config_get(config, "terminals")) == nullptr) return;

  if (cli_option.term != nullptr)
    term = cli_option.term;
  else if ((term = getenv("TERM")) == nullptr)
    return;

  if ((term_specific_config =
           t3_config_find(term_specific_config, find_term_config, term, nullptr)) != nullptr)
    read_term_config_part(term_specific_config, &term_specific_option);
}

#define SET_OPT_FROM_FILE(name, deflt)                                        \
  do {                                                                        \
    if (term_specific_option.name.is_valid())                                 \
      option.name = term_specific_option.name;                                \
    else                                                                      \
      option.name = default_option.term_options.name.value_or_default(deflt); \
  } while (false)

#define SET_OPT_FROM_DFLT(name, deflt)                         \
  do {                                                         \
    option.name = default_option.name.value_or_default(deflt); \
  } while (false)

static void post_process_options() {
  SET_OPT_FROM_DFLT(tabsize, 8);
  SET_OPT_FROM_DFLT(hide_menubar, false);

  SET_OPT_FROM_DFLT(wrap, false);

  if (cli_option.color.is_valid())
    option.color = cli_option.color;
  else
    SET_OPT_FROM_FILE(color, true);

  SET_OPT_FROM_DFLT(tab_spaces, false);
  SET_OPT_FROM_DFLT(auto_indent, true);
  SET_OPT_FROM_DFLT(indent_aware_home, true);
  SET_OPT_FROM_DFLT(show_tabs, false);
  SET_OPT_FROM_DFLT(make_backup, false);

  SET_OPT_FROM_DFLT(max_recent_files, 16);
  SET_OPT_FROM_DFLT(strip_spaces, false);

  if (!cli_option.ask_input_method && term_specific_option.key_timeout.is_valid())
    option.key_timeout = term_specific_option.key_timeout;

  SET_OPT_FROM_FILE(highlights[map_highlight(nullptr, "comment")], get_default_attr(COMMENT));
  SET_OPT_FROM_FILE(highlights[map_highlight(nullptr, "comment-keyword")],
                    get_default_attr(COMMENT_KEYWORD));
  SET_OPT_FROM_FILE(highlights[map_highlight(nullptr, "keyword")], get_default_attr(KEYWORD));
  SET_OPT_FROM_FILE(highlights[map_highlight(nullptr, "number")], get_default_attr(NUMBER));
  SET_OPT_FROM_FILE(highlights[map_highlight(nullptr, "string")], get_default_attr(STRING));
  SET_OPT_FROM_FILE(highlights[map_highlight(nullptr, "string-escape")],
                    get_default_attr(STRING_ESCAPE));
  SET_OPT_FROM_FILE(highlights[map_highlight(nullptr, "misc")], get_default_attr(MISC));
  SET_OPT_FROM_FILE(highlights[map_highlight(nullptr, "variable")], get_default_attr(VARIABLE));
  SET_OPT_FROM_FILE(highlights[map_highlight(nullptr, "error")], get_default_attr(ERROR));
  SET_OPT_FROM_FILE(highlights[map_highlight(nullptr, "addition")], get_default_attr(ADDITION));
  SET_OPT_FROM_FILE(highlights[map_highlight(nullptr, "deletion")], get_default_attr(DELETION));
  option.highlights[0] = 0;

  SET_OPT_FROM_FILE(brace_highlight, get_default_attr(BRACE_HIGHLIGHT));
}

static void print_help() {
  printf(
      "Usage: tilde [<OPTIONS>] [<FILE...>]\n"
      "  -b,--black-white            Request black & white mode, overriding config file\n"
      "  -c,--color                  Request color mode, overriding config file\n"
      "  -C<file>,--config=<file>    Use <file> as config file\n"
      "  -e<enc>, --encoding=<enc>   Use <enc> as the encoding for loading files from\n"
      "                                  the command line\n"
      "  -h,--help                   Show this help message.\n"
      "  -I,--select-input-method    Ignore configured input handling method.\n"
      "  --ignore-running            Ignore instances already running on this terminal\n"
      "  -T<term>,--terminal=<term>  Use <term> instead of TERM variable\n"
      "  -V,--version                Show version and copyright information\n"
      "  -x,--no-ext-clipboard       Disable the external (X11) clipboard interface\n");
  exit(EXIT_SUCCESS);
}

static void print_version() {
  printf(
      "Tilde version <VERSION>\n"
      "Copyright (c) 2011-2017 G.P. Halkes\n"  // @copyright
      "Tilde is licensed under the GNU General Public License version 3\n");
  printf(
      "Library versions:\n"
      "  libt3config %ld.%ld.%ld\n  libt3highlight %ld.%ld.%ld\n  libt3key (through libt3widget) "
      "%ld.%ld.%ld\n"
      "  libt3widget %ld.%ld.%ld\n  libt3window %ld.%ld.%ld\n  libtranscript %ld.%ld.%ld\n  "
      "libunistring %d.%d.?\n",
      t3_config_get_version() >> 16, (t3_config_get_version() >> 8) & 0xff,
      t3_config_get_version() & 0xff, t3_highlight_get_version() >> 16,
      (t3_highlight_get_version() >> 8) & 0xff, t3_highlight_get_version() & 0xff,
      t3_widget::get_libt3key_version() >> 16, (t3_widget::get_libt3key_version() >> 8) & 0xff,
      t3_widget::get_libt3key_version() & 0xff, t3_widget::get_version() >> 16,
      (t3_widget::get_version() >> 8) & 0xff, t3_widget::get_version() & 0xff,
      t3_window_get_version() >> 16, (t3_window_get_version() >> 8) & 0xff,
      t3_window_get_version() & 0xff, transcript_get_version() >> 16,
      (transcript_get_version() >> 8) & 0xff, transcript_get_version() & 0xff,
      _libunistring_version >> 8, _libunistring_version & 0xff);
  // FIXME: add libpcre and libsigc++ versions (from libt3widget)
  exit(EXIT_SUCCESS);
}

PARSE_FUNCTION(parse_args)
OPTIONS
OPTION('h', "help", NO_ARG)
print_help();
END_OPTION
OPTION('V', "version", NO_ARG)
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
OPTION('x', "no-ext-clipboard", NO_ARG)
cli_option.disable_external_clipboard = true;
END_OPTION
OPTION('P', "no-primary-selection", NO_ARG)
cli_option.disable_primary_selection = true;
END_OPTION
OPTION('C', "config", REQUIRED_ARG)
cli_option.config_file = optArg;
END_OPTION
OPTION('e', "encoding", OPTIONAL_ARG)
cli_option.encoding = optArg;
END_OPTION
LONG_OPTION("ignore-running", NO_ARG)
cli_option.ignore_running = true;
END_OPTION
OPTION('J', "no-parse-file-position", NO_ARG)
cli_option.disable_file_position_parsing = true;
END_OPTION
#ifdef DEBUG
LONG_OPTION("W", NO_ARG)
cli_option.wait = true;
END_OPTION
LONG_OPTION("L", REQUIRED_ARG)
PARSE_INT(cli_option.vm_limit, -1, INT_MAX / (1024 * 1024));
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

read_base_config();
read_config();

post_process_options();
END_FUNCTION

#define SET_ATTR_FROM_FILE(name, const_name)                       \
  do {                                                             \
    if (term_specific_option.name.is_valid())                      \
      set_attribute(const_name, term_specific_option.name);        \
    else if (default_option.term_options.name.is_valid())          \
      set_attribute(const_name, default_option.term_options.name); \
  } while (false)

void set_attributes() {
  SET_ATTR_FROM_FILE(non_print, attribute_t::NON_PRINT);
  SET_ATTR_FROM_FILE(text_selection_cursor, attribute_t::TEXT_SELECTION_CURSOR);
  SET_ATTR_FROM_FILE(text_selection_cursor2, attribute_t::TEXT_SELECTION_CURSOR2);
  SET_ATTR_FROM_FILE(bad_draw, attribute_t::BAD_DRAW);
  SET_ATTR_FROM_FILE(text_cursor, attribute_t::TEXT_CURSOR);
  SET_ATTR_FROM_FILE(text, attribute_t::TEXT);
  SET_ATTR_FROM_FILE(text_selected, attribute_t::TEXT_SELECTED);
  SET_ATTR_FROM_FILE(hotkey_highlight, attribute_t::HOTKEY_HIGHLIGHT);
  SET_ATTR_FROM_FILE(dialog, attribute_t::DIALOG);
  SET_ATTR_FROM_FILE(dialog_selected, attribute_t::DIALOG_SELECTED);
  SET_ATTR_FROM_FILE(button_selected, attribute_t::BUTTON_SELECTED);
  SET_ATTR_FROM_FILE(scrollbar, attribute_t::SCROLLBAR);
  SET_ATTR_FROM_FILE(menubar, attribute_t::MENUBAR);
  SET_ATTR_FROM_FILE(menubar_selected, attribute_t::MENUBAR_SELECTED);
  SET_ATTR_FROM_FILE(shadow, attribute_t::SHADOW);
  SET_ATTR_FROM_FILE(meta_text, attribute_t::META_TEXT);
  SET_ATTR_FROM_FILE(background, attribute_t::BACKGROUND);
}

static void set_config_attribute(t3_config_t *config, const char *section_name, const char *name,
                                 optional<t3_attr_t> attr) {
  static t3_attr_t attribute_masks[] = {T3_ATTR_FG_MASK, T3_ATTR_BG_MASK, T3_ATTR_UNDERLINE,
                                        T3_ATTR_BOLD,    T3_ATTR_REVERSE, T3_ATTR_BLINK,
                                        T3_ATTR_DIM};

  t3_config_t *attributes;
  size_t i, j;

  if ((attributes = t3_config_get(config, section_name)) == nullptr ||
      t3_config_get_type(attributes) != T3_CONFIG_SECTION) {
    if (!attr.is_valid()) return;
    attributes = t3_config_add_section(config, section_name, nullptr);
  }

  if (!attr.is_valid()) {
    t3_config_erase(attributes, name);
    return;
  }

  config = t3_config_add_list(attributes, name, nullptr);

  for (i = 0; i < ARRAY_SIZE(attribute_masks); i++) {
    t3_attr_t search = attr & attribute_masks[i];
    if (search == 0) continue;

    for (j = 0; j < ARRAY_SIZE(attribute_map); j++) {
      if (attribute_map[j].attr == search) {
        t3_config_add_string(config, nullptr, attribute_map[j].string);
        break;
      }
    }

    if (j == ARRAY_SIZE(attribute_map) &&
        (attribute_masks[i] == T3_ATTR_FG_MASK || attribute_masks[i] == T3_ATTR_BG_MASK)) {
      char color_name_buffer[32];
      if (attribute_masks[i] == T3_ATTR_FG_MASK)
        sprintf(color_name_buffer, "fg %d", (search >> T3_ATTR_COLOR_SHIFT) - 1);
      else
        sprintf(color_name_buffer, "bg %d", (search >> (T3_ATTR_COLOR_SHIFT + 9)) - 1);
      t3_config_add_string(config, nullptr, color_name_buffer);
    }
  }
}

#define SET_OPTION(name, type)                         \
  do {                                                 \
    if (opts->name.is_valid())                         \
      t3_config_add_##type(config, #name, opts->name); \
    else                                               \
      t3_config_erase(config, #name);                  \
  } while (false)
#define SET_ATTRIBUTE(name) set_config_attribute(config, "attributes", #name, opts->name)
#define SET_HL_ATTRIBUTE(x, name) \
  set_config_attribute(config, "highlight_attributes", name, opts->highlights[x])

static void set_term_config_options(t3_config_t *config, term_options_t *opts) {
  SET_OPTION(color, bool);

  SET_OPTION(key_timeout, int);

  SET_ATTRIBUTE(non_print);
  SET_ATTRIBUTE(text_selection_cursor);
  SET_ATTRIBUTE(text_selection_cursor2);
  SET_ATTRIBUTE(bad_draw);
  SET_ATTRIBUTE(text_cursor);
  SET_ATTRIBUTE(text);
  SET_ATTRIBUTE(text_selected);
  SET_ATTRIBUTE(hotkey_highlight);
  SET_ATTRIBUTE(dialog);
  SET_ATTRIBUTE(dialog_selected);
  SET_ATTRIBUTE(button_selected);
  SET_ATTRIBUTE(scrollbar);
  SET_ATTRIBUTE(menubar);
  SET_ATTRIBUTE(menubar_selected);
  SET_ATTRIBUTE(shadow);
  SET_ATTRIBUTE(meta_text);
  SET_ATTRIBUTE(background);

  int i;
  const char *highlight_name;
  for (i = 1; (highlight_name = reverse_map_highlight(i)) != nullptr; i++)
    SET_HL_ATTRIBUTE(i, highlight_name);

  SET_ATTRIBUTE(brace_highlight);

  if (t3_config_get(t3_config_get(config, "highlight_attributes"), nullptr) == nullptr)
    t3_config_erase(config, "highlight_attributes");
  if (t3_config_get(t3_config_get(config, "attributes"), nullptr) == nullptr)
    t3_config_erase(config, "attributes");
}

bool write_config() {
  FILE *config_file;
  t3_config_write_file_t *new_config_file;
  const char *term;
  cleanup_func_ptr<t3_config_t, t3_config_delete>::t config;
  t3_config_t *terminals, *terminal_config;
  cleanup_func_ptr<t3_config_schema_t, t3_config_delete_schema>::t schema;
  int version;

  // FIXME: verify return values

  if ((schema = t3_config_read_schema_buffer(config_schema, sizeof(config_schema), nullptr,
                                             nullptr)) == nullptr)
    return false;

  if (cli_option.config_file == nullptr)
    config_file = t3_config_xdg_open_read(T3_CONFIG_XDG_CONFIG_HOME, "tilde", "config");
  else
    config_file = fopen(cli_option.config_file, "r");

  if (config_file != nullptr) {
    /* Start by reading the existing configuration. */
    t3_config_error_t error;
    config = t3_config_read_file(config_file, &error, nullptr);
    fclose(config_file);
  } else if (errno != ENOENT) {
    return false;
  }

  if (config == nullptr ||
      (version = t3_config_get_int(t3_config_get(config, "config_version"))) < 1) {
    /* Clean-out config, because it is an old version. */
    t3_config_delete(config);
    config = t3_config_new();
  } else if (version > 1) {
    /* Don't overwrite config files with newer config version. */
    return false;
  } else {
    if (!t3_config_validate(config, schema, nullptr, 0)) return false;
  }

  default_option.term_options.key_timeout.unset();
  set_term_config_options(config, &default_option.term_options);

#define opts (&default_option)
  SET_OPTION(wrap, bool);
  SET_OPTION(tab_spaces, bool);
  SET_OPTION(auto_indent, bool);
  SET_OPTION(indent_aware_home, bool);
  SET_OPTION(show_tabs, bool);
  SET_OPTION(strip_spaces, bool);
  SET_OPTION(make_backup, bool);
  SET_OPTION(hide_menubar, bool);
  SET_OPTION(parse_file_positions, bool);
  SET_OPTION(disable_primary_selection_over_ssh, bool);

  SET_OPTION(tabsize, int);
  SET_OPTION(max_recent_files, int);
#undef opts

  t3_config_add_int(config, "config_version", 1);

  if (cli_option.term != nullptr)
    term = cli_option.term;
  else
    term = getenv("TERM");

  if (term != nullptr) {
    if ((terminals = t3_config_get(config, "terminals")) == nullptr ||
        !t3_config_is_list(terminals))
      terminals = t3_config_add_plist(config, "terminals", nullptr);

    terminal_config = t3_config_find(terminals, find_term_config, term, nullptr);
    if (terminal_config == nullptr) {
      terminal_config = t3_config_add_section(terminals, nullptr, nullptr);
      t3_config_add_string(terminal_config, "name", term);
    }

    set_term_config_options(terminal_config, &term_specific_option);
  }

  /* Validate config using schema, such that we can be sure that we won't
     say it is invalid when reading. That would fit nicely into the
     "bad things" category. */
  if (!t3_config_validate(config, schema, nullptr, 0)) {
    t3_config_delete(config);
    return false;
  }

  if (cli_option.config_file == nullptr)
    new_config_file = t3_config_xdg_open_write(T3_CONFIG_XDG_CONFIG_HOME, "tilde", "config");
  else
    new_config_file = t3_config_open_write(cli_option.config_file);

  if (new_config_file == nullptr) {
    lprintf("Could not open config file for writing: %m\n");
    t3_config_delete(config);
    return false;
  }

  if (t3_config_write_file(config, t3_config_get_write_file(new_config_file)) == T3_ERR_SUCCESS)
    return t3_config_close_write(new_config_file, t3_false, t3_true);

  t3_config_close_write(new_config_file, t3_true, t3_true);

  return false;
}

t3_attr_t get_default_attr(attribute_key_t attr) { return get_default_attr(attr, option.color); }
t3_attr_t get_default_attr(attribute_key_t attr, bool color) {
  switch (attr) {
    case DIALOG:
      return get_default_attribute(attribute_t::DIALOG, color);
    case DIALOG_SELECTED:
      return get_default_attribute(attribute_t::DIALOG_SELECTED, color);
    case SHADOW:
      return get_default_attribute(attribute_t::SHADOW, color);
    case BUTTON_SELECTED:
      return get_default_attribute(attribute_t::BUTTON_SELECTED, color);
    case SCROLLBAR:
      return get_default_attribute(attribute_t::SCROLLBAR, color);
    case MENUBAR:
      return get_default_attribute(attribute_t::MENUBAR, color);
    case MENUBAR_SELECTED:
      return get_default_attribute(attribute_t::MENUBAR_SELECTED, color);
    case BACKGROUND:
      return get_default_attribute(attribute_t::BACKGROUND, color);
    case HOTKEY_HIGHLIGHT:
      return get_default_attribute(attribute_t::HOTKEY_HIGHLIGHT, color);
    case BAD_DRAW:
      return get_default_attribute(attribute_t::BAD_DRAW, color);
    case NON_PRINT:
      return get_default_attribute(attribute_t::NON_PRINT, color);

    case TEXT:
      return get_default_attribute(attribute_t::TEXT, color);
    case TEXT_SELECTED:
      return get_default_attribute(attribute_t::TEXT_SELECTED, color);
    case TEXT_CURSOR:
      return get_default_attribute(attribute_t::TEXT_CURSOR, color);
    case TEXT_SELECTION_CURSOR:
      return get_default_attribute(attribute_t::TEXT_SELECTION_CURSOR, color);
    case TEXT_SELECTION_CURSOR2:
      return get_default_attribute(attribute_t::TEXT_SELECTION_CURSOR2, color);
    case META_TEXT:
      return get_default_attribute(attribute_t::META_TEXT, color);

    case BRACE_HIGHLIGHT:
      return color ? T3_ATTR_BOLD : T3_ATTR_BLINK;

    case COMMENT:
      return color ? T3_ATTR_FG_GREEN : 0;
    case COMMENT_KEYWORD:
      return color ? T3_ATTR_FG_YELLOW : 0;
    case KEYWORD:
      return color ? T3_ATTR_FG_CYAN | T3_ATTR_BOLD : T3_ATTR_BOLD;
    case NUMBER:
      return color ? T3_ATTR_FG_WHITE | T3_ATTR_BOLD : 0;
    case STRING:
      return color ? T3_ATTR_FG_MAGENTA | T3_ATTR_BOLD : T3_ATTR_UNDERLINE;
    case STRING_ESCAPE:
      return color ? T3_ATTR_FG_MAGENTA : T3_ATTR_UNDERLINE;
    case MISC:
      return color ? T3_ATTR_FG_YELLOW | T3_ATTR_BOLD : 0;
    case VARIABLE:
      return color ? T3_ATTR_FG_GREEN | T3_ATTR_BOLD : 0;
    case ERROR:
      return color ? T3_ATTR_FG_RED | T3_ATTR_BOLD : 0;
    case ADDITION:
      return color ? T3_ATTR_FG_GREEN | T3_ATTR_BOLD : 0;
    case DELETION:
      return color ? T3_ATTR_FG_RED | T3_ATTR_BOLD : 0;
    default:
      return 0;
  }
}
