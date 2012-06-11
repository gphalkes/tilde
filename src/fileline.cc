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
#include "fileline.h"
#include "option.h"

static file_line_factory_t default_file_line_factory(NULL);

file_line_t::file_line_t(int buffersize, file_line_factory_t *_factory) :
		text_line_t(buffersize, _factory == NULL ? &default_file_line_factory : _factory),
		highlight_start_state(0)
{}

file_line_t::file_line_t(const char *_buffer, file_line_factory_t *_factory) :
		text_line_t(_buffer, _factory == NULL ? &default_file_line_factory : _factory),
		highlight_start_state(0)
{}

file_line_t::file_line_t(const char *_buffer, int length, file_line_factory_t *_factory) :
		text_line_t(_buffer, length, _factory == NULL ? &default_file_line_factory : _factory),
		highlight_start_state(0)
{}

file_line_t::file_line_t(const std::string *str, file_line_factory_t *_factory) :
		text_line_t(str, _factory == NULL ? &default_file_line_factory : _factory),
		highlight_start_state(0)
{}

t3_attr_t file_line_t::get_base_attr(int i, const paint_info_t *info) {
	const string *str;
	file_buffer_t *file = ((file_line_factory_t *) factory)->get_file_buffer();
	int attribute_idx;

	if (file == NULL || file->highlight_info == NULL)
		return info->normal_attr;

	str = get_data();
	if ((size_t) i >= str->size())
		return info->normal_attr;

	if (file->match_line != this || (size_t) i < t3_highlight_get_start(file->last_match)) {
		file->match_line = this;
		t3_highlight_reset(file->last_match, highlight_start_state);
	}

	while (t3_highlight_get_end(file->last_match) <= (size_t) i)
		t3_highlight_match(file->last_match, str->data(), str->size());

	attribute_idx = (size_t) i < t3_highlight_get_match_start(file->last_match) ?
		t3_highlight_get_begin_attr(file->last_match) :
		t3_highlight_get_match_attr(file->last_match);

	return option.highlights[attribute_idx];
}

void file_line_t::set_highlight_start(int state) {
	highlight_start_state = state;
}

int file_line_t::get_highlight_end(void) {
	const string *str;

	file_buffer_t *file = ((file_line_factory_t *) factory)->get_file_buffer();
	if (file == NULL || file->highlight_info == NULL)
		return 0;

	if (file->match_line != this) {
		file->match_line = this;
		t3_highlight_reset(file->last_match, highlight_start_state);
	}

	str = get_data();
	while (t3_highlight_match(file->last_match, str->data(), str->size())) {}

	return t3_highlight_get_state(file->last_match);
}

//====================== file_line_factory_t ========================

file_line_factory_t::file_line_factory_t(file_buffer_t *_file_buffer) {
	file_buffer = _file_buffer;
}

text_line_t *file_line_factory_t::new_text_line_t(int buffersize) {
	return new file_line_t(buffersize, this);
}

text_line_t *file_line_factory_t::new_text_line_t(const char *_buffer) {
	return new file_line_t(_buffer, this);
}

text_line_t *file_line_factory_t::new_text_line_t(const char *_buffer, int length) {
	return new file_line_t(_buffer, length, this);
}

text_line_t *file_line_factory_t::new_text_line_t(const std::string *str) {
	return new file_line_t(str, this);
}

file_buffer_t *file_line_factory_t::get_file_buffer(void) const {
	return file_buffer;
}
