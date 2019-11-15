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
#ifndef FILE_EDIT_WINDOW_T
#define FILE_EDIT_WINDOW_T

#include "tilde/filebuffer.h"
#include <t3widget/widget.h>

using namespace t3widget;

class file_edit_window_t : public edit_window_t {
 private:
  connection_t rewrap_connection;
  void force_repaint_to_bottom(rewrap_type_t type, text_pos_t line, text_pos_t pos);

 public:
  explicit file_edit_window_t(file_buffer_t *_text = nullptr);
  ~file_edit_window_t() override;
  void draw_info_window() override;
  bool process_key(t3widget::key_t key) override;
  void update_contents() override;

  void set_text(file_buffer_t *_text);
  file_buffer_t *get_text() const;
  void goto_matching_brace();
  void show_character_details();
  void save_view_parameters_in_buffer();
};

#endif
