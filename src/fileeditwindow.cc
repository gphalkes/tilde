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
#include "fileeditwindow.h"

file_edit_window_t::file_edit_window_t(file_buffer_t *_text) {
	text_buffer_t *old_text = get_text();
	if (_text == NULL)
		_text = new file_buffer_t();

	_text->set_has_window(true);
	edit_window_t::set_text(_text, _text->get_view_parameters());

	delete old_text;
}

file_edit_window_t::~file_edit_window_t(void) {
	file_buffer_t *_text = (file_buffer_t *) text;
	_text->set_has_window(false);
	save_view_parameters(&_text->view_parameters);
}

void file_edit_window_t::set_text(file_buffer_t *_text) {
	file_buffer_t *old_text = (file_buffer_t *) edit_window_t::get_text();
	old_text->set_has_window(false);
	save_view_parameters(&old_text->view_parameters);
	_text->set_has_window(true);
	edit_window_t::set_text(_text, _text->get_view_parameters());
}

file_buffer_t *file_edit_window_t::get_text(void) const {
	return (file_buffer_t *) edit_window_t::get_text();
}
