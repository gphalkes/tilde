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
#include "aboutdialog.h"

about_dialog_t::about_dialog_t(int height, int width) : dialog_t(height, width, "About") {
	text_window_t *text_window;
	button_t *close_button;

	text = new text_buffer_t();

	text->append_text("Tilde - The intuitive text editor\n\nVersion <VERSION>\nCopyright (c) 2011 G.P. Halkes\n\n"
		"The Tilde text editor is licensed under the GNU General Public License version 3. "
		"You should have received a copy of the GNU General Public License along with this program. "
		"If not, see <http://www.gnu.org/licenses/>.");
	text_window = new text_window_t(text, false);
	text_window->set_size(height - 3, width - 2);
	text_window->set_position(1, 1);
	text_window->set_enabled(false);

	close_button = new button_t("Close");
	close_button->set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMCENTER) | T3_CHILD(T3_ANCHOR_BOTTOMCENTER));
	close_button->set_position(-1, 0);
	close_button->connect_activate(sigc::mem_fun(this, &about_dialog_t::close));

	push_back(text_window);
	push_back(close_button);
}

about_dialog_t::~about_dialog_t(void) {
	delete text;
}

bool about_dialog_t::set_size(optint height, optint width) {
	return true;
}

