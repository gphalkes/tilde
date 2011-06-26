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
#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <t3widget/widget.h>
using namespace t3_widget;

class about_dialog_t : public dialog_t {
	private:
		text_buffer_t *text;
		text_window_t *text_window;

	public:
		about_dialog_t(int height, int width);
		virtual ~about_dialog_t(void);
		virtual bool set_size(optint height, optint width);
};

#endif

