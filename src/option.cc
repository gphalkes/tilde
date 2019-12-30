/* Copyright (C) 2011-2012,2018 G.P. Halkes
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

#include "tilde/attributemap.h"
#include "tilde/log.h"
#include "tilde/option.h"
#include "tilde/optionMacros.h"
#include "tilde/option_access.h"
#include "tilde/util.h"

using namespace t3widget;

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

static const char config_schema[] = {
#include "config.bytes"
};

static t3_bool find_term_config(const t3_config_t *config, const void *data) {
  if (t3_config_get_type(config) != T3_CONFIG_SECTION) {
    return t3_false;
  }
  return strcmp(reinterpret_cast<const char *>(data),
                t3_config_get_string(t3_config_get(config, "name"))) == 0;
}

static void read_config(std::unique_ptr<FILE, fclose_deleter> config_file) {
  t3_config_error_t error;
  std::unique_ptr<t3_config_t, t3_config_deleter> config;
  std::unique_ptr<t3_config_schema_t, t3_schema_deleter> schema;
  t3_config_t *term_specific_config;
  const char *term;

  config.reset(t3_config_read_file(config_file.get(), &error, nullptr));
  if (config == nullptr) {
    config_read_error = true;
    config_read_error_string = t3_config_strerror(error.error);
    config_read_error_line = error.line_number;
    return;
  }

  schema.reset(t3_config_read_schema_buffer(config_schema, sizeof(config_schema), &error, nullptr));
  if (schema == nullptr) {
    config_read_error = true;
    lprintf("Error loading schema: %d: %s\n", error.line_number, t3_config_strerror(error.error));
    config_read_error_string = t3_config_strerror(
        error.error == T3_ERR_OUT_OF_MEMORY ? T3_ERR_OUT_OF_MEMORY : T3_ERR_INTERNAL);
    config_read_error_line = 0;
    return;
  }

  if (!t3_config_validate(config.get(), schema.get(), &error, 0)) {
    config_read_error = true;
    config_read_error_string = t3_config_strerror(error.error);
    config_read_error_line = error.line_number;
    return;
  }

  get_default_options(config.get());

  for (t3_config_t *lang = t3_config_get(t3_config_get(config.get(), "lang"), nullptr);
       lang != nullptr; lang = t3_config_get_next(lang)) {
    const char *name = t3_config_get_string(t3_config_get(lang, "name"));
    const char *line_comment = t3_config_get_string(t3_config_get(lang, "line_comment"));
    if (name != nullptr && line_comment != nullptr) {
      option.line_comment_map[std::string(name)] = line_comment;
    }
  }

  if ((term_specific_config = t3_config_get(config.get(), "terminals")) == nullptr) {
    return;
  }

  if (cli_option.term != nullptr) {
    term = cli_option.term;
  } else if ((term = getenv("TERM")) == nullptr) {
    return;
  }

  if ((term_specific_config =
           t3_config_find(term_specific_config, find_term_config, term, nullptr)) != nullptr) {
    get_term_options(term_specific_config, &term_specific_option);
  }
}

static void read_base_config_file() {
  std::unique_ptr<FILE, fclose_deleter> config_file;

  config_file.reset(fopen(DATADIR "/base.config", "r"));
  if (config_file == nullptr) {
    lprintf("Failed to open file: %s %m\n", DATADIR "/base.config");
    return;
  }

  read_config(std::move(config_file));
}

static void read_user_config_file() {
  std::unique_ptr<FILE, fclose_deleter> config_file;

  if (cli_option.config_file.is_valid()) {
    config_file.reset(fopen(cli_option.config_file.value().c_str(), "r"));
  } else {
    config_file.reset(t3_config_xdg_open_read(T3_CONFIG_XDG_CONFIG_HOME, "tilde", "config"));
  }

  if (config_file == nullptr) {
    if (errno != ENOENT) {
      config_read_error = true;
      config_read_error_string = strerror(errno);
      config_read_error_line = 0;
    }
    return;
  }
  read_config(std::move(config_file));
}

static void print_help() {
  printf(
      "Usage: tilde [<OPTIONS>] [<FILE...>[:<LINE>[:<COLUMN>]]]\n"
      "  -b,--black-white            Request black & white mode, overriding config file\n"
      "  -c,--color                  Request color mode, overriding config file\n"
      "  -C<file>,--config=<file>    Use <file> as config file\n"
      "  -e<enc>, --encoding=<enc>   Use <enc> as the encoding for loading files from\n"
      "                                the command line\n"
      "  -h,--help                   Show this help message.\n"
      "  -I,--select-input-method    Ignore configured input handling method.\n"
      "  --ignore-running            Ignore instances already running on this terminal\n"
      "  -J,--no-parse-file-position Do not attempt to parse line and column from file\n"
      "                                names passed on the command line\n"
      "  -P,--no-primary-selection   Disable the use of the primary selection (i.e.\n"
      "                                middle mouse-button copy-paste) for external\n"
      "                                clipboards like X11\n"
      "  -T<term>,--terminal=<term>  Use <term> instead of TERM variable\n"
      "  -V,--version                Show version and copyright information\n"
      "  -x,--no-ext-clipboard       Disable the external (X11) clipboard interface\n");
  exit(EXIT_SUCCESS);
}

static void print_version() {
  printf(
      "Tilde version <VERSION>\n"
      "Copyright (c) 2011-2018 G.P. Halkes\n"  // @copyright
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
      t3widget::get_libt3key_version() >> 16, (t3widget::get_libt3key_version() >> 8) & 0xff,
      t3widget::get_libt3key_version() & 0xff, t3widget::get_version() >> 16,
      (t3widget::get_version() >> 8) & 0xff, t3widget::get_version() & 0xff,
      t3_window_get_version() >> 16, (t3_window_get_version() >> 8) & 0xff,
      t3_window_get_version() & 0xff, transcript_get_version() >> 16,
      (transcript_get_version() >> 8) & 0xff, transcript_get_version() & 0xff,
      _libunistring_version >> 8, _libunistring_version & 0xff);
  // FIXME: add libpcre and libsigc++ versions (from libt3widget)
  exit(EXIT_SUCCESS);
}

// clang-format off
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
      if (!optArg) {
        cli_option.encoding = "";
      } else {
        cli_option.encoding = optArg;
      }
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

  read_base_config_file();
  read_user_config_file();

  derive_runtime_options();
END_FUNCTION
// clang-format on

bool write_config() {
  std::unique_ptr<FILE, fclose_deleter> config_file;
  t3_config_write_file_t *new_config_file;
  const char *term;
  std::unique_ptr<t3_config_t, t3_config_deleter> config;
  t3_config_t *terminals, *terminal_config;
  std::unique_ptr<t3_config_schema_t, t3_schema_deleter> schema;
  int version;

  // FIXME: verify return values

  schema.reset(
      t3_config_read_schema_buffer(config_schema, sizeof(config_schema), nullptr, nullptr));
  if (schema == nullptr) {
    return false;
  }

  if (cli_option.config_file.is_valid()) {
    config_file.reset(fopen(cli_option.config_file.value().c_str(), "r"));
  } else {
    config_file.reset(t3_config_xdg_open_read(T3_CONFIG_XDG_CONFIG_HOME, "tilde", "config"));
  }

  if (config_file != nullptr) {
    /* Start by reading the existing configuration. */
    t3_config_error_t error;
    config.reset(t3_config_read_file(config_file.get(), &error, nullptr));
    config_file.reset();
  } else if (errno != ENOENT) {
    return false;
  }

  if (config == nullptr ||
      (version = t3_config_get_int(t3_config_get(config.get(), "config_version"))) < 1) {
    /* Clean-out config, because it is an old version. */
    config.reset(t3_config_new());
  } else if (version > 1) {
    /* Don't overwrite config files with newer config version. */
    return false;
  } else {
    if (!t3_config_validate(config.get(), schema.get(), nullptr, 0)) {
      return false;
    }
  }

  default_option.term_options.key_timeout.reset();
  set_default_options(config.get());

  t3_config_add_int(config.get(), "config_version", 1);

  if (cli_option.term != nullptr) {
    term = cli_option.term;
  } else {
    term = getenv("TERM");
  }

  if (term != nullptr) {
    if ((terminals = t3_config_get(config.get(), "terminals")) == nullptr ||
        !t3_config_is_list(terminals)) {
      terminals = t3_config_add_plist(config.get(), "terminals", nullptr);
    }

    terminal_config = t3_config_find(terminals, find_term_config, term, nullptr);
    if (terminal_config == nullptr) {
      terminal_config = t3_config_add_section(terminals, nullptr, nullptr);
      t3_config_add_string(terminal_config, "name", term);
    }

    set_term_options(terminal_config, term_specific_option);
  }

  /* Validate config using schema, such that we can be sure that we won't
     say it is invalid when reading. That would fit nicely into the
     "bad things" category. */
  if (!t3_config_validate(config.get(), schema.get(), nullptr, 0)) {
    return false;
  }

  if (cli_option.config_file.is_valid()) {
    new_config_file = t3_config_open_write(cli_option.config_file.value().c_str());
  } else {
    new_config_file = t3_config_xdg_open_write(T3_CONFIG_XDG_CONFIG_HOME, "tilde", "config");
  }

  if (new_config_file == nullptr) {
    lprintf("Could not open config file for writing: %m\n");
    return false;
  }

  if (t3_config_write_file(config.get(), t3_config_get_write_file(new_config_file)) ==
      T3_ERR_SUCCESS) {
    return t3_config_close_write(new_config_file, t3_false, t3_true);
  }

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
