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

#include <widget.h>

using namespace t3_widget;

#include "filestate.h"

class file_buffer_t : public text_buffer_t {
	protected:
		const char *encoding;
		bool file_has_bom;

	public:
		file_buffer_t(const char *_name = NULL, const char *_encoding = NULL);
		virtual ~file_buffer_t(void);
		rw_result_t load(load_process_t *state);
		rw_result_t save(save_as_process_t *state);
};

#endif
