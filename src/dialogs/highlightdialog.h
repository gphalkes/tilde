/* Copyright (C) 2011-2012 G.P. Halkes
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
#ifndef HIGHLIGHTDIALOG_H
#define HIGHLIGHTDIALOG_H

#include <t3widget/widget.h>
#include <t3highlight/highlight.h>

using namespace t3_widget;

typedef cleanup_func_ptr<t3_highlight_lang_t, t3_highlight_free_list>::t cleanup_lang_t;

class highlight_dialog_t : public dialog_t {
	private:
		cleanup_lang_t names;
		list_pane_t *list;

	public:
		highlight_dialog_t(int height, int width);
		virtual bool set_size(optint height, optint width);
		void ok_activated(void);
		void set_selected(const char *lang_file);

	T3_WIDGET_SIGNAL(language_selected, void, t3_highlight_t *);
};

#endif
