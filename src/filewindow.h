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
#ifndef FILE_WINDOW_H
#define FILE_WINDOW_H

#include <widget.h>

#include "filebuffer.h"

using namespace t3_widget;

class file_window_t : public edit_window_t {
/*	private:
		file_window_t(text_buffer_t *_text = NULL);
		virtual void set_text(text_buffer_t *_text);*/
	public:
		file_window_t(file_buffer_t *_text = NULL);
		virtual void set_text(file_buffer_t *_text);
};

#endif
