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

#include <cstdlib>
#include <widget.h>

#include "option.h"
#include "action.h"
#include "filebuffer.h"
#include "openfiles.h"
#include "dialogs/selectbufferdialog.h"
#include "dialogs/encodingdialog.h"
#include "log.h"

using namespace std;
using namespace t3_widget;

#define MESSAGE_DIALOG_WIDTH 50

message_dialog_t *continue_abort_dialog;
open_file_dialog_t *open_file_dialog;
save_as_dialog_t *save_as_dialog;
message_dialog_t *close_confirm_dialog;
message_dialog_t *error_dialog;

class main_t : public main_window_base_t {
	private:
		menu_bar_t *menu;
		menu_panel_t *panel;
		menu_item_base_t *remove;
		split_t *split;

		select_buffer_dialog_t *select_buffer_dialog;

	public:
		main_t(void);
		virtual bool process_key(key_t key);
		virtual bool set_size(optint height, optint width);

	private:
		edit_window_t *get_current(void) { return (edit_window_t *) split->get_current(); }
		void menu_activated(int id);
		void switch_buffer(file_buffer_t *buffer);
		void switch_to_new_buffer(stepped_process_t *process);
		void close_cb(stepped_process_t *process);
};

main_t::main_t(void) {
	menu = new menu_bar_t(option.hide_menubar);
	push_back(menu);
	menu->connect_activate(sigc::mem_fun(this, &main_t::menu_activated));

	panel = new menu_panel_t(menu, "_File;Ff");
	panel->add_item("_New;nN", "^N", action_id_t::FILE_NEW);
	panel->add_item("_Open...;oO", "^O", action_id_t::FILE_OPEN);
	panel->add_item("Open _Recent...;rR", NULL, action_id_t::FILE_OPEN_RECENT);
	panel->add_item("_Close;cC", "^W", action_id_t::FILE_CLOSE);
	panel->add_item("_Save;sS", "^S", action_id_t::FILE_SAVE);
	panel->add_item("Save _As...;aA", NULL, action_id_t::FILE_SAVE_AS);
	panel->add_separator();
	panel->add_item("Re_draw Screen;dD", NULL, action_id_t::FILE_REPAINT);
	panel->add_item("S_uspend;uU", NULL, action_id_t::FILE_SUSPEND);
	panel->add_item("E_xit;xX", "^Q", action_id_t::FILE_EXIT);

	panel = new menu_panel_t(menu, "_Edit;Ee");
	panel->add_item("_Undo;uU", "^Z", action_id_t::EDIT_UNDO);
	panel->add_item("_Redo;rR", "^Y", action_id_t::EDIT_REDO);
	panel->add_separator();
	panel->add_item("_Copy;cC", "^C", action_id_t::EDIT_COPY);
	panel->add_item("Cu_t;tT", "^X", action_id_t::EDIT_CUT);
	panel->add_item("_Paste;pP", "^V", action_id_t::EDIT_PASTE);
	panel->add_item("Select _All;aA", "^A", action_id_t::EDIT_SELECT_ALL);
	panel->add_item("_Insert Character...;iI", "F9", action_id_t::EDIT_INSERT_CHAR);

	panel = new menu_panel_t(menu, "_Search;sS");
	panel->add_item("_Find...;fF", "^F", action_id_t::SEARCH_SEARCH);
	panel->add_item("Find _Next;nN", "F3", action_id_t::SEARCH_AGAIN);
	panel->add_item("Find _Previous;pP", "S-F3", action_id_t::SEARCH_AGAIN_BACKWARD);
	panel->add_item("_Replace...;rR", "^R", action_id_t::SEARCH_REPLACE);
	panel->add_item("_Go to Line...;gG", "^G", action_id_t::SEARCH_GOTO);

	panel = new menu_panel_t(menu, "_Windows;wW");
	panel->add_item("_Next Buffer;nN", "F6" , action_id_t::WINDOWS_NEXT_BUFFER);
	panel->add_item("_Previous Buffer;pP", "S-F6" , action_id_t::WINDOWS_PREV_BUFFER);
	panel->add_item("_Select Buffer...;sS", NULL, action_id_t::WINDOWS_SELECT);
	panel->add_separator();
	panel->add_item("Split _Horizontal;hH", NULL, action_id_t::WINDOWS_HSPLIT);
	panel->add_item("Split _Vertical;vV", NULL, action_id_t::WINDOWS_VSPLIT);
	panel->add_item("_Close Window;cC", NULL, action_id_t::WINDOWS_MERGE);
	panel->add_item("Next Window", "F8", action_id_t::WINDOWS_NEXT_WINDOW);
	panel->add_item("Previous Window", "S-F8", action_id_t::WINDOWS_PREV_WINDOW);

	panel = new menu_panel_t(menu, "_Options;oO");
	panel->add_item("_Tabs...;tT", NULL, action_id_t::OPTIONS_TABS);
	panel->add_item("_Keys...;kK", NULL, action_id_t::OPTIONS_KEYS);

	panel = new menu_panel_t(menu, "_Help;hH");
	panel->add_item("_Help;hH", "F1", action_id_t::HELP_HELP);
	panel->add_item("_About;aA", NULL, action_id_t::HELP_ABOUT);

	edit_window_t *edit = new edit_window_t(new file_buffer_t()); //FIXME: load text
	split = new split_t(edit, true);
	split->set_position(!option.hide_menubar, 0);
	split->set_size(t3_win_get_height(window) - !option.hide_menubar, t3_win_get_width(window));
	push_back(split);

	select_buffer_dialog = new select_buffer_dialog_t(11, t3_win_get_width(window) - 4);
	select_buffer_dialog->center_over(this);
	select_buffer_dialog->connect_activate(sigc::mem_fun(this, &main_t::switch_buffer));

	continue_abort_dialog = new message_dialog_t(MESSAGE_DIALOG_WIDTH, "Question", "_Continue;cC", "_Abort;aA", NULL);
	continue_abort_dialog->center_over(this);

	string wd = get_working_directory();
	open_file_dialog = new open_file_dialog_t(t3_win_get_height(window) - 4, t3_win_get_width(window) - 4);
	open_file_dialog->center_over(this);
	open_file_dialog->change_dir(&wd);

	save_as_dialog = new save_as_dialog_t(t3_win_get_height(window) - 4, t3_win_get_width(window) - 4);
	save_as_dialog->center_over(this);
	save_as_dialog->change_dir(&wd);

	close_confirm_dialog = new message_dialog_t(MESSAGE_DIALOG_WIDTH, "Confirm", "_Yes;yY", "_No;nN", "_Cancel;cC", NULL);
	close_confirm_dialog->center_over(this);

	error_dialog = new message_dialog_t(MESSAGE_DIALOG_WIDTH, "Error", "Ok;oO", NULL);
	error_dialog->center_over(this);
}

bool main_t::process_key(key_t key) {
	switch (key) {
		case EKEY_CTRL | 'n':          menu_activated(action_id_t::FILE_NEW); break;
		case EKEY_CTRL | 'o':          menu_activated(action_id_t::FILE_OPEN); break;
		case EKEY_CTRL | 'w':          menu_activated(action_id_t::FILE_CLOSE); break;
		case EKEY_CTRL | 's':          menu_activated(action_id_t::FILE_SAVE); break;
		case EKEY_CTRL | 'q':          menu_activated(action_id_t::FILE_EXIT); break;
		case EKEY_F6:                  menu_activated(action_id_t::WINDOWS_NEXT_BUFFER); break;
		case EKEY_F6 | EKEY_SHIFT:     menu_activated(action_id_t::WINDOWS_PREV_BUFFER); break;
		default:
			return main_window_base_t::process_key(key);
	}
	return true;
}

bool main_t::set_size(optint height, optint width) {
	bool result;
	(void) height;

	result = menu->set_size(None, width);
	result &= split->set_size(height - !option.hide_menubar, width);
	result &= select_buffer_dialog->set_size(None, width - 4);
	result &= open_file_dialog->set_size(height - 4, width - 4);
	result &= save_as_dialog->set_size(height - 4, width - 4);
	return true;
}

void main_t::menu_activated(int id) {
	switch (id) {
		case action_id_t::FILE_NEW: {
			file_buffer_t *new_text = new file_buffer_t();
			get_current()->set_text(new_text);
			break;
		}

		case action_id_t::FILE_OPEN:
			load_process_t::execute(sigc::mem_fun(this, &main_t::switch_to_new_buffer));
			break;

		case action_id_t::FILE_CLOSE:
			close_process_t::execute(sigc::mem_fun(this, &main_t::close_cb), (file_buffer_t *) get_current()->get_text());
			break;
		case action_id_t::FILE_SAVE:
			save_process_t::execute(sigc::ptr_fun(stepped_process_t::ignore_result), (file_buffer_t *) get_current()->get_text());
			break;
		case action_id_t::FILE_SAVE_AS:
			save_as_process_t::execute(sigc::ptr_fun(stepped_process_t::ignore_result), (file_buffer_t *) get_current()->get_text());
			break;

/*
		case ActionID::FILE_OPEN_RECENT:
			activate_window(WindowID::OPEN_RECENT);
			break;
*/
		case action_id_t::FILE_REPAINT:
			t3_widget::redraw();
			break;
		case action_id_t::FILE_SUSPEND:
			suspend();
			break;

		case action_id_t::FILE_EXIT:
			exit_process_t::execute(sigc::ptr_fun(stepped_process_t::ignore_result));
			//~ #ifdef DEBUG
			//~ delete editwin;
			//~ #endif
			break;

		case action_id_t::EDIT_UNDO:
			get_current()->undo();
			break;
		case action_id_t::EDIT_REDO:
			get_current()->redo();
			break;
		case action_id_t::EDIT_COPY:
		case action_id_t::EDIT_CUT:
			get_current()->cut_copy(id == action_id_t::EDIT_CUT);
			break;
		case action_id_t::EDIT_PASTE:
			get_current()->paste();
			break;
		case action_id_t::EDIT_SELECT_ALL:
			get_current()->select_all();
			break;
		case action_id_t::EDIT_INSERT_CHAR:
			get_current()->insert_special();
			break;

		case action_id_t::SEARCH_SEARCH:
		case action_id_t::SEARCH_REPLACE:
			get_current()->find_replace(id == action_id_t::SEARCH_REPLACE);
			break;
		case action_id_t::SEARCH_AGAIN:
		case action_id_t::SEARCH_AGAIN_BACKWARD:
			get_current()->find_next(id == action_id_t::SEARCH_AGAIN_BACKWARD);
			break;
		case action_id_t::SEARCH_GOTO:
			get_current()->goto_line();
			break;

		case action_id_t::WINDOWS_NEXT_BUFFER: {
			edit_window_t *current = (edit_window_t *) split->get_current();
			current->set_text(open_files.next_buffer((file_buffer_t *) current->get_text()));
			break;
		}
		case action_id_t::WINDOWS_PREV_BUFFER: {
			edit_window_t *current = (edit_window_t *) split->get_current();
			current->set_text(open_files.previous_buffer((file_buffer_t *) current->get_text()));
			break;
		}
		case action_id_t::WINDOWS_SELECT:
			select_buffer_dialog->show();
			break;
		case action_id_t::WINDOWS_HSPLIT:
		case action_id_t::WINDOWS_VSPLIT: {
			file_buffer_t *new_file = open_files.next_buffer(NULL);
			if (new_file == NULL)
				new_file = new file_buffer_t();
			split->split(new edit_window_t(new_file), id == action_id_t::WINDOWS_HSPLIT);
			break;
		}
		case action_id_t::WINDOWS_NEXT_WINDOW:
			split->next();
			break;
		case action_id_t::WINDOWS_PREV_WINDOW:
			split->previous();
			break;
		case action_id_t::WINDOWS_MERGE:
			delete split->unsplit();
			break;
		default:
			break;
	}
}

void main_t::switch_buffer(file_buffer_t *buffer) {
	/* FIXME: for now we simply ignore the request if the buffer is already shown,
	   but it would be better to force the other window to go another buffer and
	   take over the requested buffer. However, we don't know which other window
	   has the buffer, so at this point this is impossible.

	   Even if we did know, we would still have to temporarily set the current
	   window to an empty file, then force the other window to go to the next
	   buffer, set the desired buffer in the current window and destroy the
	   temporary buffer. All of this such that we avoid two windows pointing
	   at the same buffer.
	*/
	if (buffer->has_window())
		return;
	get_current()->set_text(buffer);
}

void main_t::switch_to_new_buffer(stepped_process_t *process) {
	const text_buffer_t *text;
	file_buffer_t *buffer;

	if (!process->get_result())
		return;

	buffer = ((load_process_t *) process)->get_file_buffer();
	if (buffer->has_window())
		return;

	text = get_current()->get_text();
	get_current()->set_text(buffer);
	//FIXME: buffer should not be closed if the user specifically created it by asking for a new file!
	if (text->get_name() == NULL && !text->is_modified())
		delete text;
}

void main_t::close_cb(stepped_process_t *process) {
	text_buffer_t *text;

	if (!process->get_result())
		return;

	text = get_current()->get_text();
	if (((close_process_t *) process)->get_file_buffer_ptr() != text)
		PANIC();

	menu_activated(action_id_t::WINDOWS_NEXT_BUFFER);
	if (get_current()->get_text() == text)
		get_current()->set_text(new file_buffer_t());
	delete text;
}

int main(int argc, char *argv[]) {
	init_log();
	setlocale(LC_ALL, "");
	//FIXME: call this when internationalization is started. Requires #include <libintl.h>
	// bind_textdomain_codeset("UTF-8");
	parse_args(argc, argv);

	if (option.start_debugger_on_segfault)
		enable_debugger_on_segfault(argv[0]);

#ifdef DEBUG
	set_limits();
	if (option.wait) {
		printf("Debug mode: Waiting for keypress to allow debugger attach\n");
		getchar();
	}
#endif

	complex_error_t result;
	if (!(result = init()).get_success()) {
		fprintf(stderr, "Error: %s\n", result.get_string());
		fprintf(stderr, "init failed\n");
		exit(EXIT_FAILURE);
	}

	init_charsets();
	main_t main_window;

	set_color_mode(false);
	set_attribute(attribute_t::SELECTION_CURSOR, T3_ATTR_BG_GREEN);

	main_window.show();

	set_key_timeout(100);
	main_loop();
	return 0;
}
