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
#include "filebuffer.h"
#include "openfiles.h"

file_buffer_t::file_buffer_t(const char *name) : text_buffer_t(name), show(false) {
	open_files.push_back(this);
}

void file_buffer_t::set_show(bool _show) { show = _show; }
bool file_buffer_t::get_show(void) { return show; }
