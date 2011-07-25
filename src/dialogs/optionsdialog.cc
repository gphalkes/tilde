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
#include <algorithm>

#include "dialogs/optionsdialog.h"
#include "option.h"
#include "main.h"

using namespace std;

static key_t number_keys[] = { '0', '1', '2', '3' ,'4', '5', '6', '7', '8', '9' };

options_dialog_t::options_dialog_t(void) : dialog_t(7, 25, "Options") {
	smart_label_t *label;
	int width;
	button_t *ok_button, *cancel_button;

	label = new smart_label_t(_("_Tab size"));
	label->set_position(1, 2);
	push_back(label);
	tabsize_field = new text_field_t();
	tabsize_field->set_key_filter(number_keys, sizeof(number_keys) / sizeof(number_keys[0]), true);
	tabsize_field->set_label(label);
	tabsize_field->set_size(1, 5);
	tabsize_field->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
	tabsize_field->set_position(1, -2);
	tabsize_field->connect_move_focus_down(sigc::mem_fun(this, &options_dialog_t::focus_next));
	tabsize_field->connect_activate(sigc::mem_fun(this, &options_dialog_t::handle_activate));
	push_back(tabsize_field);

	width = label->get_width() + 2 + 5;

	label = new smart_label_t(_("Tab uses _spaces"));
	label->set_position(2, 2);
	push_back(label);
	tab_spaces_box = new checkbox_t();
	tab_spaces_box->set_label(label);
	tab_spaces_box->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
	tab_spaces_box->set_position(2, -2);
	tab_spaces_box->connect_move_focus_up(sigc::mem_fun(this, &options_dialog_t::focus_previous));
	tab_spaces_box->connect_move_focus_down(sigc::mem_fun(this, &options_dialog_t::focus_next));
	tab_spaces_box->connect_activate(sigc::mem_fun(this, &options_dialog_t::handle_activate));
	push_back(tab_spaces_box);

	width = max(label->get_width() + 2 + 3, width);

	label = new smart_label_t(_("_Wrap text"));
	label->set_position(3, 2);
	push_back(label);
	wrap_box = new checkbox_t();
	wrap_box->set_label(label);
	wrap_box->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
	wrap_box->set_position(3, -2);
	wrap_box->connect_move_focus_up(sigc::mem_fun(this, &options_dialog_t::focus_previous));
	wrap_box->connect_move_focus_down(sigc::mem_fun(this, &options_dialog_t::focus_next));
	wrap_box->connect_activate(sigc::mem_fun(this, &options_dialog_t::handle_activate));
	push_back(wrap_box);

	width = max(label->get_width() + 2 + 3, width);

	label = new smart_label_t(_("_Automatic indentation"));
	label->set_position(4, 2);
	push_back(label);
	auto_indent_box = new checkbox_t();
	auto_indent_box->set_label(label);
	auto_indent_box->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
	auto_indent_box->set_position(4, -2);
	auto_indent_box->connect_move_focus_up(sigc::mem_fun(this, &options_dialog_t::focus_previous));
	auto_indent_box->connect_move_focus_down(sigc::mem_fun(this, &options_dialog_t::focus_next));
	auto_indent_box->connect_activate(sigc::mem_fun(this, &options_dialog_t::handle_activate));
	push_back(auto_indent_box);

	width = max(label->get_width() + 2 + 3, width);

	cancel_button = new button_t("_Cancel");
	cancel_button->set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
	cancel_button->set_position(-1, -2);
	cancel_button->connect_activate(sigc::mem_fun(this, &options_dialog_t::close));
	cancel_button->connect_move_focus_up(sigc::mem_fun(this, &options_dialog_t::focus_previous));
	cancel_button->connect_move_focus_up(sigc::mem_fun(this, &options_dialog_t::focus_previous));
	cancel_button->connect_move_focus_left(sigc::mem_fun(this, &options_dialog_t::focus_previous));

	ok_button = new button_t("_Ok", true);
	ok_button->set_anchor(cancel_button, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
	ok_button->set_position(0, -2);
	ok_button->connect_move_focus_up(sigc::mem_fun(this, &options_dialog_t::focus_previous));
	ok_button->connect_move_focus_right(sigc::mem_fun(this, &options_dialog_t::focus_next));
	ok_button->connect_activate(sigc::mem_fun(this, &options_dialog_t::handle_activate));

	push_back(ok_button);
	push_back(cancel_button);

	width = max(ok_button->get_width() + cancel_button->get_width() + 4, width);

	set_size(None, width + 4);
}

void options_dialog_t::set_values_from_view(edit_window_t *view) {
	char tabsize_text[20];
	sprintf(tabsize_text, "%d", view->get_tabsize());
	tabsize_field->set_text(tabsize_text);
	tab_spaces_box->set_state(view->get_tab_spaces());
	wrap_box->set_state(view->get_wrap());
	auto_indent_box->set_state(view->get_auto_indent());
}

void options_dialog_t::set_view_values(edit_window_t *view) {
	int tabsize = atoi(tabsize_field->get_text()->c_str());
	if (tabsize > 0 || tabsize < 17)
		view->set_tabsize(tabsize);
	view->set_wrap(wrap_box->get_state() ? wrap_type_t::WORD : wrap_type_t::NONE);
	view->set_tab_spaces(tab_spaces_box->get_state());
	view->set_auto_indent(auto_indent_box->get_state());
}

void options_dialog_t::handle_activate(void) {
	int tabsize = atoi(tabsize_field->get_text()->c_str());
	if (tabsize < 1 || tabsize > 16) {
		error_dialog->set_message(_("Tab size out of range (must be between 1 and 16 inclusive)."));
		error_dialog->center_over(this);
		error_dialog->show();
		return;
	}
	hide();
	activate();
}

