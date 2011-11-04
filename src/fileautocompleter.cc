/* Copyright (C) 2011 G.P. Halkes
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
#include <string>
#include <set>
#include "fileautocompleter.h"
#include "log.h"

using namespace std;

static bool compare_strings(string *a, string *b) {
	return a->compare(*b) < 0;
}

typedef set<string *, bool(*)(string *, string *)> result_set_t;

string_list_t *file_autocompleter_t::build_autocomplete_list(const text_buffer_t *text, int *position) {
	const text_line_t *line;
	int i;

	if (text->cursor.pos == 0)
		return NULL;

	line = text->get_line_data(text->cursor.line);

	for (i = line->adjust_position(text->cursor.pos, -1); i > 0; i = line->adjust_position(i, -1)) {
		if (!line->is_alnum(i))
			break;
	}
	if (!line->is_alnum(i))
		i = line->adjust_position(i, 1);


	lprintf("Word to complete: %.*s\n", text->cursor.pos - i, line->get_data()->data() + i);

	text_coordinate_t start(0, 0);
	text_coordinate_t eof(INT_MAX, INT_MAX);
	string needle(*line->get_data(), i, text->cursor.pos - i);
	finder_t finder(&needle, find_flags_t::ANCHOR_WORD_LEFT, NULL);
	find_result_t find_result;
	result_set_t result_set(compare_strings);
	string word;

	while (text->find_limited(&finder, start, eof, &find_result)) {
		line = text->get_line_data(find_result.line);
		for (; find_result.end < text->get_line_max(find_result.line) &&
				line->is_alnum(find_result.end);
				find_result.end = line->adjust_position(find_result.end, 1))
		{}

		if (find_result.end - find_result.start != text->cursor.pos - i) {
			word = line->get_data()->substr(find_result.start, find_result.end - find_result.start);
			if (result_set.count(&word) == 0)
				result_set.insert(new string(word));
		}

		start.line = find_result.line;
		start.pos = find_result.end;
	}

	if (result_set.empty())
		return NULL;

	string_list_t *list = new string_list_t();

	for (result_set_t::iterator iter = result_set.begin(); iter != result_set.end(); iter++)
		list->push_back(*iter);
	*position = text->cursor.pos - i;

	return list;
}

void file_autocompleter_t::autocomplete(text_buffer_t *text, size_t idx) {
	(void) text;
	(void) idx;
}
