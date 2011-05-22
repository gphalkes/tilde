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
#include "filewindow.h"

/* file_window_t::file_window_t(text_buffer_t *_text) : edit_window_t(_text) { PANIC(); }
void file_window_t::set_text(text_buffer_t *_text) {
	(void) _text;
	PANIC();
} */

file_window_t::file_window_t(file_buffer_t *_text) : edit_window_t(_text != NULL ? _text : _text = new file_buffer_t()) {
	_text->set_show(true);
}

void file_window_t::set_text(file_buffer_t *_text) {
	file_buffer_t *old_text = dynamic_cast<file_buffer_t *>(text);
	if (old_text == NULL)
		PANIC();
	old_text->set_show(false);
	edit_window_t::set_text(_text);
	_text->set_show(true);
}
