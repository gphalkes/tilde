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
#ifndef OPENRECENTDIALOG_H
#define OPENRECENTDIALOG_H

#include <t3widget/widget.h>
using namespace t3_widget;

#include "openfiles.h"

class open_recent_dialog_t : public dialog_t {
	private:
		list_pane_t *list;
		int known_version;

	public:
		open_recent_dialog_t(int height, int width);
		virtual bool set_size(optint height, optint width);
		virtual void show(void);
		virtual void ok_activated(void);

	T3_WIDGET_SIGNAL(file_selected, void, recent_file_info_t *);
};


#endif
