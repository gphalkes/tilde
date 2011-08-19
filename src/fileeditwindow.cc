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
	save_view_parameters(_text->view_parameters);
}

void file_edit_window_t::draw_info_window(void) {
	file_buffer_t *_text = (file_buffer_t *) text;
	text_line_t *name_line = _text->get_name_line();
	text_line_t::paint_info_t paint_info;
	int name_width = t3_win_get_width(info_window);

	/* FIXME: is it really necessary to do this on each key stroke??? */
	t3_win_set_paint(info_window, 0, 0);
	if (name_line->calculate_screen_width(0, name_line->get_length(), 1) > name_width) {
		t3_win_addstr(info_window, "..", 0);
		paint_info.start = name_line->adjust_position(name_line->get_length(), -(name_width - 2));
		paint_info.size = name_width - 2;
	} else {
		paint_info.start = 0;
		paint_info.size = name_width;
	}
	paint_info.leftcol = 0;
	paint_info.max = INT_MAX;
	paint_info.tabsize = 1;
	paint_info.flags = text_line_t::TAB_AS_CONTROL | text_line_t::SPACECLEAR;
	paint_info.selection_start = -1;
	paint_info.selection_end = -1;
	paint_info.cursor = -1;
	paint_info.normal_attr = 0;
	paint_info.selected_attr = 0;

	name_line->paint_line(info_window, &paint_info);
	t3_win_clrtoeol(info_window);
}

void file_edit_window_t::set_text(file_buffer_t *_text) {
	file_buffer_t *old_text = (file_buffer_t *) edit_window_t::get_text();
	old_text->set_has_window(false);
	save_view_parameters(old_text->view_parameters);
	_text->set_has_window(true);
	edit_window_t::set_text(_text, _text->get_view_parameters());
}

file_buffer_t *file_edit_window_t::get_text(void) const {
	return (file_buffer_t *) edit_window_t::get_text();
}
