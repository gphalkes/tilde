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
#include "tilde/fileeditwindow.h"
#include "tilde/fileautocompleter.h"
#include "tilde/log.h"
#include "tilde/main.h"

file_edit_window_t::file_edit_window_t(file_buffer_t *_text) {
  text_buffer_t *old_text = get_text();
  if (_text == nullptr) {
    _text = new file_buffer_t();
  }

  _text->set_has_window(true);
  rewrap_connection = _text->connect_rewrap_required(
      bind_front(&file_edit_window_t::force_repaint_to_bottom, this));
  edit_window_t::set_text(_text, _text->get_behavior_parameters());
  edit_window_t::set_autocompleter(new file_autocompleter_t());

  delete old_text;
}

file_edit_window_t::~file_edit_window_t() {
  file_buffer_t *_text = static_cast<file_buffer_t *>(text);
  _text->set_has_window(false);
  save_behavior_parameters(_text->behavior_parameters.get());
  rewrap_connection.disconnect();
}

void file_edit_window_t::draw_info_window() {
  file_buffer_t *_text = static_cast<file_buffer_t *>(text);
  text_line_t *name_line = _text->get_name_line();
  text_line_t::paint_info_t paint_info;
  int name_width = info_window.get_width();
  text_pos_t screen_width = name_line->calculate_screen_width(0, name_line->size(), 1);

  info_window.set_paint(0, 0);
  info_window.set_default_attrs(get_attribute(attribute_t::MENUBAR));

  if (screen_width > name_width) {
    paint_info.start = 0;
    info_window.addstr("..", 0);

    while (screen_width > name_width - 2) {
      screen_width -= name_line->width_at(paint_info.start);
      paint_info.start = name_line->adjust_position(paint_info.start, 1);
    }
    paint_info.size = name_width - 2;
  } else {
    paint_info.start = 0;
    paint_info.size = name_width;
  }
  paint_info.leftcol = 0;
  paint_info.max = std::numeric_limits<text_pos_t>::max();
  paint_info.tabsize = 1;
  paint_info.flags = text_line_t::TAB_AS_CONTROL | text_line_t::SPACECLEAR;
  paint_info.selection_start = -1;
  paint_info.selection_end = -1;
  paint_info.cursor = -1;
  paint_info.normal_attr = 0;
  paint_info.selected_attr = 0;

  name_line->paint_line(&info_window, paint_info);
  info_window.clrtoeol();
}

void file_edit_window_t::set_text(file_buffer_t *_text) {
  file_buffer_t *old_text = static_cast<file_buffer_t *>(edit_window_t::get_text());
  old_text->set_has_window(false);
  save_behavior_parameters(old_text->behavior_parameters.get());
  rewrap_connection.disconnect();
  _text->set_has_window(true);
  rewrap_connection = _text->connect_rewrap_required(
      bind_front(&file_edit_window_t::force_repaint_to_bottom, this));
  edit_window_t::set_text(_text, _text->get_behavior_parameters());
}

file_buffer_t *file_edit_window_t::get_text() const {
  return static_cast<file_buffer_t *>(edit_window_t::get_text());
}

bool file_edit_window_t::process_key(t3widget::key_t key) {
  bool result = edit_window_t::process_key(key);

  if (!result) {
    switch (key) {
      case EKEY_CTRL | ']':
        if (get_text()->goto_matching_brace()) {
          ensure_cursor_on_screen();
          force_redraw();
        }
        return true;
      case EKEY_CTRL | '_':
        get_text()->toggle_line_comment();
        force_redraw();
        return true;
      case EKEY_F2:
        show_character_details();
        return true;
      default:
        break;
    }
  }
  return result;
}

void file_edit_window_t::goto_matching_brace() {
  if (get_text()->goto_matching_brace()) {
    ensure_cursor_on_screen();
    force_redraw();
  }
}

void file_edit_window_t::update_contents() {
  /* Ideally we would only update this when the screen will get updated.
     However, the problem is that we don't know exactly when this will be.
     Simply checking redraw doesn't work, because the contents is redrawn
     every time this is called if the edit window has focus (which we can't
     query at this time). Thus we simply update every time :-(

     Another improvement would be if we could find out the positions of the
     old and new matching brace positions. That would allow more localized
     updates.
  */
  if (get_text()->update_matching_brace()) {
    update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
  }
  edit_window_t::update_contents();
}

void file_edit_window_t::force_repaint_to_bottom(rewrap_type_t type, text_pos_t line,
                                                 text_pos_t pos) {
  (void)type;
  (void)pos;
  update_repaint_lines(line, std::numeric_limits<text_pos_t>::max());
}

void file_edit_window_t::show_character_details() {
  size_t size;
  const char *data = get_text()->get_char_under_cursor(&size);
  if (data != nullptr) {
    character_details_dialog->set_codepoints(data, size);
    character_details_dialog->show();
  }
}

void file_edit_window_t::save_behavior_parameters_in_buffer() {
  save_behavior_parameters(
      static_cast<file_buffer_t *>(edit_window_t::get_text())->behavior_parameters.get());
}
