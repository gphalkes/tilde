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
#ifndef FILE_EDIT_WINDOW_T
#define FILE_EDIT_WINDOW_T

#include <t3widget/widget.h>
#include "filebuffer.h"

using namespace t3_widget;

class file_edit_window_t : public edit_window_t {
	public:
		file_edit_window_t(file_buffer_t *_text = NULL);
		virtual ~file_edit_window_t(void);
		virtual void draw_info_window(void);
		virtual bool process_key(t3_widget::key_t key);
		virtual void update_contents(void);

		void set_text(file_buffer_t *_text);
		file_buffer_t *get_text(void) const;
		void goto_matching_brace(void);
};

#endif
