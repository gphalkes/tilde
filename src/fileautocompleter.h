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
#ifndef FILE_AUTOCOMPLETER_H
#define FILE_AUTOCOMPLETER_H

#include <t3widget/widget.h>

using namespace t3_widget;

class file_autocompleter_t : public autocompleter_t {
	public:
		string_list_t *build_autocomplete_list(const text_buffer_t *text);
		string_list_t *update_autocomplete_list(const text_buffer_t *text);
		void autocomplete(text_buffer_t *text, size_t idx);
};

#endif
