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
#ifndef FILE_LINE_H
#define FILE_LINE_H

#include <t3widget/textline.h>
#include "filebuffer.h"

class file_line_factory_t;

class file_line_t : public text_line_t {
	protected:
		int highlight_start_state;

	public:
		file_line_t(int buffersize = BUFFERSIZE, file_line_factory_t *_factory = NULL);
		file_line_t(const char *_buffer, file_line_factory_t *_factory = NULL);
		file_line_t(const char *_buffer, int length, file_line_factory_t *_factory = NULL);
		file_line_t(const std::string *str, file_line_factory_t *_factory = NULL);

	protected:
		virtual t3_attr_t get_base_attr(int i, const paint_info_t *info);
};

class file_line_factory_t : public text_line_factory_t {
	private:
		file_buffer_t *file_buffer;
	public:
		file_line_factory_t(file_buffer_t *_file_buffer);
		virtual text_line_t *new_text_line_t(int buffersize = BUFFERSIZE);
		virtual text_line_t *new_text_line_t(const char *_buffer);
		virtual text_line_t *new_text_line_t(const char *_buffer, int length);
		virtual text_line_t *new_text_line_t(const std::string *str);
		file_buffer_t *get_file_buffer(void) const;
};

#endif

