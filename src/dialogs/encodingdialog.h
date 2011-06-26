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
#ifndef ENCODINGDIALOG_H
#define ENCODINGDIALOG_H

#include <string>
#include <t3widget/widget.h>

using namespace t3_widget;

void init_charsets(void);

class encoding_dialog_t : public dialog_t {
	protected:
		list_pane_t *list;
		separator_t *horizontal_separator;
		text_field_t *manual_entry;
		int selected;
		char *saved_tag;

		void ok_activated(void);
		void selection_changed(void);

	public:
		encoding_dialog_t(int height, int width);
		virtual bool set_size(optint height, optint width);

		void set_encoding(const char *encoding);

	T3_WIDGET_SIGNAL(activate, void, const std::string *);
};


#endif
