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

#include "fileautocompleter.h"
#include "log.h"

string_list_base_t *file_autocompleter_t::build_autocomplete_list(const text_buffer_t *text,
                                                                  int *position) {
  const text_line_t *line;
  int completion_end;
  std::string current_word;

  if (current_list != nullptr) {
    current_list.reset();
  }

  if (text->cursor.pos == 0) {
    return nullptr;
  }

  line = text->get_line_data(text->cursor.line);

  for (completion_start = line->adjust_position(text->cursor.pos, -1); completion_start > 0;
       completion_start = line->adjust_position(completion_start, -1)) {
    if (!line->is_alnum(completion_start)) {
      break;
    }
  }
  if (!line->is_alnum(completion_start)) {
    completion_start = line->adjust_position(completion_start, 1);
  }

  for (completion_end = completion_start;
       completion_end < line->get_length() && line->is_alnum(completion_end);
       completion_end = line->adjust_position(completion_end, 1)) {
  }
  current_word = line->get_data()->substr(completion_start, completion_end - completion_start);

  text_coordinate_t start(0, 0);
  text_coordinate_t eof(INT_MAX, INT_MAX);
  std::string needle(*line->get_data(), completion_start, text->cursor.pos - completion_start);
  finder_t finder(needle, find_flags_t::ANCHOR_WORD_LEFT, nullptr);
  find_result_t find_result;
  std::set<string_view> result_set;

  while (text->find_limited(&finder, start, eof, &find_result)) {
    line = text->get_line_data(find_result.start.line);
    for (; find_result.end.pos < line->get_length() && line->is_alnum(find_result.end.pos);
         find_result.end.pos = line->adjust_position(find_result.end.pos, 1)) {
    }

    if (find_result.end.pos - find_result.start.pos != text->cursor.pos - completion_start) {
      string_view word(*line->get_data());
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
    current_list->push_back(to_string(word));
  }
  *position = completion_start;

  return current_list.get();
}

void file_autocompleter_t::autocomplete(text_buffer_t *text, size_t idx) {
  text_coordinate_t start(text->cursor.line, completion_start);
  text->replace_block(start, text->cursor, &(*current_list)[idx]);
  current_list.reset();
}
