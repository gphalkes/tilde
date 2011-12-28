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
#ifndef FILE_BUFFER_H
#define FILE_BUFFER_H

#include <t3widget/widget.h>
#include <t3highlight/highlight.h>

using namespace t3_widget;

#include "filestate.h"

class file_edit_window_t;

class file_buffer_t : public text_buffer_t {
	friend class file_edit_window_t; // Required to access view_parameters and set_has_window
	private:
		cleanup_free_ptr<char>::t name, encoding;
		text_line_t name_line;
		cleanup_ptr<edit_window_t::view_parameters_t>::t view_parameters;
		bool has_window;
		int highlight_valid;
		opt_bool strip_spaces;

	public:
		t3_highlight_t *highlight_info;
		text_line_t *match_line;
		t3_highlight_match_t *last_match;

	private:
		virtual void prepare_paint_line(int line);
		void set_has_window(bool _has_window);
		void invalidate_highlight(rewrap_type_t type, int line, int pos);

	public:
		file_buffer_t(const char *_name = NULL, const char *_encoding = NULL);
		virtual ~file_buffer_t(void);
		rw_result_t load(load_process_t *state);
		rw_result_t save(save_as_process_t *state);

		const char *get_name(void) const;
		const char *get_encoding(void) const;
		const edit_window_t::view_parameters_t *get_view_parameters(void) const;
		text_line_t *get_name_line(void);

		bool get_has_window(void) const;

		t3_highlight_t *get_highlight(void);
		void set_highlight(t3_highlight_t *highlight);

		bool get_strip_spaces(void) const;
		void set_strip_spaces(bool _strip_spaces);

		void do_strip_spaces(void);
};

#endif
