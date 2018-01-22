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
#ifndef FILE_BUFFER_H
#define FILE_BUFFER_H

#include <t3highlight/highlight.h>
#include <t3widget/widget.h>

using namespace t3_widget;

#include "filestate.h"

class file_edit_window_t;

class file_buffer_t : public text_buffer_t {
  friend class file_edit_window_t;  // Required to access view_parameters and set_has_window
  friend class file_line_t;

 private:
  cleanup_free_ptr<char>::t name, encoding;
  text_line_t name_line;
  cleanup_ptr<edit_window_t::view_parameters_t>::t view_parameters;
  bool has_window;
  int highlight_valid;
  optional<bool> strip_spaces;
  t3_highlight_t *highlight_info;
  text_line_t *match_line;
  t3_highlight_match_t *last_match;
  bool matching_brace_valid;
  text_coordinate_t matching_brace_coordinate;
  std::string line_comment;

 private:
  void prepare_paint_line(int line) override;
  void set_has_window(bool _has_window);
  void invalidate_highlight(rewrap_type_t type, int line, int pos);
  bool find_matching_brace(text_coordinate_t &match_location);

 public:
  file_buffer_t(const char *_name = nullptr, const char *_encoding = nullptr);
  ~file_buffer_t() override;
  rw_result_t load(load_process_t *state);
  rw_result_t save(save_as_process_t *state);

  const char *get_name() const;
  const char *get_encoding() const;
  const edit_window_t::view_parameters_t *get_view_parameters() const;
  text_line_t *get_name_line();

  bool get_has_window() const;

  t3_highlight_t *get_highlight();
  void set_highlight(t3_highlight_t *highlight);

  bool get_strip_spaces() const;
  void set_strip_spaces(bool _strip_spaces);

  void do_strip_spaces();

  bool goto_matching_brace();
  /** Update the matching brace information in the file_buffer_t.

      @return A boolean indicating whether the matching brace information changed.
  */
  bool update_matching_brace();

  void set_line_comment(const char *text);
  void toggle_line_comment();
};

#endif
