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
#ifndef SPLIT_H
#define SPLIT_H

#include "widget/widget.h"
using namespace t3_widget;

class split_t : public widget_t, public container_t {
	private:
		widgets_t widgets;
		widgets_t::iterator current;
		bool horizontal, focus;

	public:
		split_t(widget_t *widget, bool _horizontal);
		virtual ~split_t(void);
		virtual bool process_key(key_t key);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);

		void split(widget_t *widget, bool _horizontal);
		bool unsplit(void);
		bool next(void);
		bool previous(void);
		widget_t *get_current(void);
};

#endif
