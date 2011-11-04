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
#include "fileautocompleter.h"
#include "log.h"

string_list_t *file_autocompleter_t::build_autocomplete_list(const text_buffer_t *text) {
	const text_line_t *line;
	int i;

	if (text->cursor.pos == 0)
		return NULL;

	line = text->get_line_data(text->cursor.line);

	for (i = line->adjust_position(text->cursor.pos, -1); i > 0; i = line->adjust_position(i, -1)) {
		if (!line->is_alnum(i) && !((*line->get_data())[i] == '_'))
			break;
	}
	if (!line->is_alnum(i) && !((*line->get_data())[i] == '_'))
		i = line->adjust_position(i, 1);


	lprintf("Word to complete: %.*s\n", text->cursor.pos - i, line->get_data()->c_str() + i);
/*
	text_coordinate_t start(0, 0);
	text_coordinate_t eof(INT_MAX, INT_MAX);

	while (text->find_limited(local_finder, start, eof)) {

	}
*/

	return NULL;
}

string_list_t *file_autocompleter_t::update_autocomplete_list(const text_buffer_t *text) {
	(void) text;
	return NULL;
}

void file_autocompleter_t::autocomplete(text_buffer_t *text, size_t idx) {
	(void) text;
	(void) idx;
}
