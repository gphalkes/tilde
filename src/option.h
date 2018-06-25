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
#ifndef OPTIONS_H
#define OPTIONS_H

#include <cstdlib>
#include <list>
#include <map>
#include <string>
#include <t3widget/util.h>
#include <t3window/window.h>

#include "tilde/util.h"

using namespace t3widget;

struct cli_options_t {
  /* Options to override config file. */
  optional<bool> color;

  const char *term;
  bool ask_input_method;

  std::list<std::string> files;
  bool disable_external_clipboard;
  bool disable_primary_selection;

#ifdef DEBUG
  bool wait;
  int vm_limit;
  bool start_debugger_on_segfault;
#endif
  optional<std::string> config_file;
  optional<std::string> encoding;
  bool ignore_running;
  bool disable_file_position_parsing;
};

struct term_options_t {
  optional<bool> color;

  optional<int> key_timeout;

  optional<t3_attr_t> non_print;
  optional<t3_attr_t> text_selection_cursor;
  optional<t3_attr_t> text_selection_cursor2;
  optional<t3_attr_t> bad_draw;
  optional<t3_attr_t> text_cursor;
  optional<t3_attr_t> text;
  optional<t3_attr_t> text_selected;
  optional<t3_attr_t> hotkey_highlight;

  optional<t3_attr_t> dialog;
  optional<t3_attr_t> dialog_selected;
  optional<t3_attr_t> button_selected;
  optional<t3_attr_t> scrollbar;
  optional<t3_attr_t> menubar;
  optional<t3_attr_t> menubar_selected;

  optional<t3_attr_t> shadow;
  optional<t3_attr_t> meta_text;
  optional<t3_attr_t> background;

  optional<t3_attr_t> highlights[MAX_HIGHLIGHTS];
  optional<t3_attr_t> brace_highlight;
};

struct options_t {
  term_options_t term_options;
  optional<bool> wrap;
  optional<bool> tab_spaces;
  optional<bool> auto_indent;
  optional<bool> indent_aware_home;
  optional<bool> show_tabs;
  optional<bool> strip_spaces;
  optional<bool> make_backup;
  optional<bool> hide_menubar;
  optional<bool> parse_file_positions;
  optional<bool> disable_primary_selection_over_ssh;

  optional<int> tabsize;
  optional<size_t> max_recent_files;
};

struct highlight_attrs_t {
  t3_attr_t comment;
  t3_attr_t comment_keyword;
  t3_attr_t keyword;
  t3_attr_t string;
  t3_attr_t string_escape;
  t3_attr_t number;
  t3_attr_t misc;
};

struct runtime_options_t {
  int tabsize;
  bool wrap;
  bool hide_menubar;
  bool color;
  bool tab_spaces;
  bool auto_indent;
  bool indent_aware_home;
  bool show_tabs;
  bool strip_spaces;
  bool make_backup;
  size_t max_recent_files;
  optional<int> key_timeout;
  t3_attr_t highlights[MAX_HIGHLIGHTS];
  t3_attr_t brace_highlight;

  std::map<std::string, std::string> line_comment_map;
};

enum attribute_key_t {
  DIALOG,
  DIALOG_SELECTED,
  SHADOW,
  BUTTON_SELECTED,
  SCROLLBAR,
  MENUBAR,
  MENUBAR_SELECTED,
  BACKGROUND,
  HOTKEY_HIGHLIGHT,
  BAD_DRAW,
  NON_PRINT,

  TEXT,
  TEXT_SELECTED,
  TEXT_CURSOR,
  TEXT_SELECTION_CURSOR,
  TEXT_SELECTION_CURSOR2,
  META_TEXT,
  BRACE_HIGHLIGHT,

  COMMENT,
  COMMENT_KEYWORD,
  KEYWORD,
  NUMBER,
  STRING,
  STRING_ESCAPE,
  MISC,
  VARIABLE,
  ERROR,
  ADDITION,
  DELETION,
};

extern bool config_read_error;
extern std::string config_read_error_string;
extern int config_read_error_line;

extern cli_options_t cli_option;
extern runtime_options_t option;

extern term_options_t term_specific_option;
extern options_t default_option;

void parse_args(int argc, char **argv);
void set_attributes();
bool write_config();
t3_attr_t get_default_attr(attribute_key_t attr);
t3_attr_t get_default_attr(attribute_key_t attr, bool color);

#endif
