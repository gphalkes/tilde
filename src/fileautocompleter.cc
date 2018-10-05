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
#include <set>
#include <string>

#include "tilde/fileautocompleter.h"
#include "tilde/log.h"

string_list_base_t *file_autocompleter_t::build_autocomplete_list(const text_buffer_t *text,
                                                                  t3widget::text_pos_t *position) {
  text_pos_t completion_end;

  if (current_list != nullptr) {
    current_list.reset();
  }

  const text_coordinate_t cursor = text->get_cursor();
  if (cursor.pos == 0) {
    return nullptr;
  }

  const text_line_t &line = text->get_line_data(cursor.line);

  for (completion_start = line.adjust_position(cursor.pos, -1); completion_start > 0;
       completion_start = line.adjust_position(completion_start, -1)) {
    if (!line.is_alnum(completion_start)) {
      break;
    }
  }
  if (!line.is_alnum(completion_start)) {
    completion_start = line.adjust_position(completion_start, 1);
  }

  for (completion_end = completion_start;
       completion_end < line.get_length() && line.is_alnum(completion_end);
       completion_end = line.adjust_position(completion_end, 1)) {
  }
  string_view current_word =
      string_view(line.get_data()).substr(completion_start, completion_end - completion_start);

  text_coordinate_t start(0, 0);
  text_coordinate_t eof(std::numeric_limits<text_pos_t>::max(),
                        std::numeric_limits<text_pos_t>::max());
  std::string needle(line.get_data(), completion_start, cursor.pos - completion_start);
  std::unique_ptr<finder_t> finder =
      finder_t::create(needle, find_flags_t::ANCHOR_WORD_LEFT, nullptr);
  find_result_t find_result;
  std::set<string_view> result_set;

  while (text->find_limited(finder.get(), start, eof, &find_result)) {
    const text_line_t &matching_line = text->get_line_data(find_result.start.line);
    for (; find_result.end.pos < matching_line.get_length() &&
           matching_line.is_alnum(find_result.end.pos);
         find_result.end.pos = matching_line.adjust_position(find_result.end.pos, 1)) {
    }

    if (find_result.end.pos - find_result.start.pos != cursor.pos - completion_start) {
      string_view word(matching_line.get_data());
      word = word.substr(find_result.start.pos, find_result.end.pos - find_result.start.pos);
      if (word != current_word) {
        result_set.insert(word);
      }
    }

    start.line = find_result.end.line;
    start.pos = find_result.end.pos;
  }

  if (result_set.empty()) {
    return nullptr;
  }

  try {
    current_list.reset(new string_list_t());
  } catch (const std::bad_alloc &) {
    return nullptr;
  }

  for (string_view word : result_set) {
    current_list->push_back(std::string(word));
  }
  *position = completion_start;

  return current_list.get();
}

void file_autocompleter_t::autocomplete(text_buffer_t *text, size_t idx) {
  const text_coordinate_t cursor = text->get_cursor();
  const text_coordinate_t start(cursor.line, completion_start);
  text->replace_block(start, cursor, (*current_list)[idx]);
  current_list.reset();
}
