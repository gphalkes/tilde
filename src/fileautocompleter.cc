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
#include <set>
#include <string>

#include "fileautocompleter.h"
#include "log.h"

static bool compare_strings(std::string *a, std::string *b) { return a->compare(*b) < 0; }

typedef std::set<std::string *, bool (*)(std::string *, std::string *)> result_set_t;

file_autocompleter_t::file_autocompleter_t(void) : current_list(NULL) {}

string_list_base_t *file_autocompleter_t::build_autocomplete_list(const text_buffer_t *text,
                                                                  int *position) {
  const text_line_t *line;
  int completion_end;
  std::string current_word;

  if (current_list != NULL) {
    delete current_list;
    current_list = NULL;
  }

  if (text->cursor.pos == 0) return NULL;

  line = text->get_line_data(text->cursor.line);

  for (completion_start = line->adjust_position(text->cursor.pos, -1); completion_start > 0;
       completion_start = line->adjust_position(completion_start, -1)) {
    if (!line->is_alnum(completion_start)) break;
  }
  if (!line->is_alnum(completion_start))
    completion_start = line->adjust_position(completion_start, 1);

  for (completion_end = completion_start;
       completion_end < line->get_length() && line->is_alnum(completion_end);
       completion_end = line->adjust_position(completion_end, 1)) {
  }
  current_word = line->get_data()->substr(completion_start, completion_end - completion_start);

  text_coordinate_t start(0, 0);
  text_coordinate_t eof(INT_MAX, INT_MAX);
  std::string needle(*line->get_data(), completion_start, text->cursor.pos - completion_start);
  finder_t finder(&needle, find_flags_t::ANCHOR_WORD_LEFT, NULL);
  find_result_t find_result;
  result_set_t result_set(compare_strings);
  std::string word;

  while (text->find_limited(&finder, start, eof, &find_result)) {
    line = text->get_line_data(find_result.start.line);
    for (; find_result.end.pos < line->get_length() && line->is_alnum(find_result.end.pos);
         find_result.end.pos = line->adjust_position(find_result.end.pos, 1)) {
    }

    if (find_result.end.pos - find_result.start.pos != text->cursor.pos - completion_start) {
      word = line->get_data()->substr(find_result.start.pos,
                                      find_result.end.pos - find_result.start.pos);
      if (result_set.count(&word) == 0 && word != current_word)
        result_set.insert(new std::string(word));
    }

    start.line = find_result.end.line;
    start.pos = find_result.end.pos;
  }

  if (result_set.empty()) return NULL;

  try {
    current_list = new string_list_t();
  } catch (const std::bad_alloc &) {
    current_list = NULL;
    return NULL;
  }

  for (result_set_t::iterator iter = result_set.begin(); iter != result_set.end(); iter++)
    current_list->push_back(*iter);
  *position = completion_start;

  return current_list;
}

void file_autocompleter_t::autocomplete(text_buffer_t *text, size_t idx) {
  text_coordinate_t start(text->cursor.line, completion_start);
  text->replace_block(start, text->cursor, (*current_list)[idx]);
  delete current_list;
  current_list = NULL;
}
