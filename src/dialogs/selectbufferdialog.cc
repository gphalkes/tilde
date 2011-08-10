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
#include "dialogs/selectbufferdialog.h"
#include "openfiles.h"

select_buffer_dialog_t::select_buffer_dialog_t(int height, int width) :
	dialog_t(height, width, "Select Buffer"), known_version(INT_MIN)
{
	button_t *ok_button, *cancel_button;

	list = new list_pane_t(true);
	list->set_size(height - 3, width - 2);
	list->set_position(1, 1);
	list->connect_activate(sigc::mem_fun(this, &select_buffer_dialog_t::ok_activated));

	cancel_button = new button_t("_Cancel", false);
	cancel_button->set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
	cancel_button->set_position(-1, -2);
	cancel_button->connect_activate(sigc::mem_fun(this, &select_buffer_dialog_t::close));
	cancel_button->connect_move_focus_left(sigc::mem_fun(this, &select_buffer_dialog_t::focus_previous));
	cancel_button->connect_move_focus_up(
		sigc::bind(sigc::mem_fun(this, &select_buffer_dialog_t::focus_set), list));
	ok_button = new button_t("_OK", true);
	ok_button->set_anchor(cancel_button, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
	ok_button->set_position(0, -2);
	ok_button->connect_activate(sigc::mem_fun(this, &select_buffer_dialog_t::ok_activated));
	cancel_button->connect_move_focus_right(sigc::mem_fun(this, &select_buffer_dialog_t::focus_next));
	cancel_button->connect_move_focus_up(sigc::mem_fun(this, &select_buffer_dialog_t::focus_previous));

	push_back(list);
	push_back(ok_button);
	push_back(cancel_button);
}

bool select_buffer_dialog_t::set_size(optint height, optint width) {
	bool result = true;

	if (!height.is_valid())
		height = t3_win_get_height(window);
	if (!width.is_valid())
		width = t3_win_get_width(window);

	result &= dialog_t::set_size(height, width);

	result &= list->set_size(height - 3, width - 2);
	return result;
}

void select_buffer_dialog_t::show(void) {
	if (open_files.get_version() != known_version) {
		multi_widget_t *multi_widget;
		known_version = open_files.get_version();
		int width = t3_win_get_width(window);

		while (!list->empty()) {
			multi_widget = (multi_widget_t *) list->back();
			list->pop_back();
			delete multi_widget ;
		}

		for (open_files_t::iterator iter = open_files.begin();
				iter != open_files.end(); iter++)
		{
			bullet_t *bullet;
			label_t *label;
			const char *name;

			multi_widget = new multi_widget_t();
			multi_widget->set_size(None, width - 5);
			multi_widget->show();
			bullet = new bullet_t(sigc::mem_fun((*iter), &file_buffer_t::get_has_window));
			multi_widget->push_back(bullet, -1, true, false);
			name = (*iter)->get_name();
			if (name == NULL)
				name = "(Untitled)";
			label = new label_t(name);
			label->set_anchor(bullet, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
			label->set_align(label_t::ALIGN_LEFT_UNDERFLOW);
			multi_widget->push_back(label, 1, true, false);
			list->push_back(multi_widget);
		}
	}
	list->reset();
	dialog_t::show();
}

void select_buffer_dialog_t::ok_activated(void) {
	hide();
	activate(open_files[list->get_current()]);
}
