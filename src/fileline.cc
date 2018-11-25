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
#include "tilde/fileline.h"
#include "tilde/option.h"

static file_line_factory_t default_file_line_factory(nullptr);

file_line_t::file_line_t(int buffersize, file_line_factory_t *_factory)
    : text_line_t(buffersize, _factory == nullptr ? &default_file_line_factory : _factory),
      highlight_start_state(0) {}

file_line_t::file_line_t(string_view _buffer, file_line_factory_t *_factory)
    : text_line_t(_buffer, _factory == nullptr ? &default_file_line_factory : _factory),
      highlight_start_state(0) {}

int file_line_t::get_highlight_idx(text_pos_t i) const {
  file_buffer_t *file = static_cast<file_line_factory_t *>(get_line_factory())->get_file_buffer();

  if (file == nullptr || file->highlight_info == nullptr) {
    return -1;
  }

  const std::string &str = get_data();
  if (static_cast<size_t>(i) >= str.size()) {
    return -1;
  }

  if (file->match_line != this ||
      static_cast<size_t>(i) < t3_highlight_get_start(file->last_match)) {
    file->match_line = this;
    t3_highlight_reset(file->last_match, highlight_start_state);
  }

  while (t3_highlight_get_end(file->last_match) <= static_cast<size_t>(i)) {
    t3_highlight_match(file->last_match, str.data(), str.size());
  }

  return static_cast<size_t>(i) < t3_highlight_get_match_start(file->last_match)
             ? t3_highlight_get_begin_attr(file->last_match)
             : t3_highlight_get_match_attr(file->last_match);
}

t3_attr_t file_line_t::get_base_attr(text_pos_t i, const paint_info_t &info) const {
  file_buffer_t *file = static_cast<file_line_factory_t *>(get_line_factory())->get_file_buffer();
  int idx = get_highlight_idx(i);
  t3_attr_t result = idx < 0 ? info.normal_attr : option.highlights[idx];

  if (file->matching_brace_valid &&
      (i == info.cursor || (this == &file->get_line_data(file->matching_brace_coordinate.line) &&
                            i == file->matching_brace_coordinate.pos))) {
    result = t3_term_combine_attrs(result, option.brace_highlight);
  }

  return result;
}

void file_line_t::set_highlight_start(int state) { highlight_start_state = state; }

int file_line_t::get_highlight_end() {
  file_buffer_t *file = static_cast<file_line_factory_t *>(get_line_factory())->get_file_buffer();
  if (file == nullptr || file->highlight_info == nullptr) {
    return 0;
  }

  if (file->match_line != this) {
    file->match_line = this;
    t3_highlight_reset(file->last_match, highlight_start_state);
  }

  const std::string &str = get_data();
  while (t3_highlight_match(file->last_match, str.data(), str.size())) {
  }

  return t3_highlight_get_state(file->last_match);
}

//====================== file_line_factory_t ========================

file_line_factory_t::file_line_factory_t(file_buffer_t *_file_buffer) {
  file_buffer = _file_buffer;
}

std::unique_ptr<text_line_t> file_line_factory_t::new_text_line_t(int buffersize) {
  return t3widget::make_unique<file_line_t>(buffersize, this);
}

std::unique_ptr<text_line_t> file_line_factory_t::new_text_line_t(string_view _buffer) {
  return t3widget::make_unique<file_line_t>(_buffer, this);
}

file_buffer_t *file_line_factory_t::get_file_buffer() const { return file_buffer; }
