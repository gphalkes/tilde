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

using namespace std;
using namespace t3_widget;

static open_file_dialog_t open_file_dialog(20, 75);

class main_t : public main_window_base_t {
	private:
		menu_bar_t *menu;
		menu_panel_t *panel;
		menu_item_base_t *remove;
		split_t *split;

	public:
		main_t(void) {
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
		}

		virtual bool process_key(key_t key) {
			switch (key) {
				case EKEY_CTRL | 'n':          menu_activated(action_id_t::FILE_NEW); break;
				case EKEY_CTRL | 'q':          menu_activated(action_id_t::FILE_EXIT); break;
				case EKEY_F6:                  menu_activated(action_id_t::WINDOWS_NEXT_BUFFER); break;
				case EKEY_F6 | EKEY_SHIFT:     menu_activated(action_id_t::WINDOWS_PREV_BUFFER); break;
				default:
					return main_window_base_t::process_key(key);
			}
			return true;
		}

		virtual bool set_size(optint height, optint width) {
			(void) height;
			menu->set_size(None, width);
			split->set_size(height - !option.hide_menubar, width);
			return true;
		}

	private:
		void menu_activated(int id) {
			switch (id) {
				case action_id_t::FILE_NEW: {
					file_buffer_t *new_text = new file_buffer_t();
					((edit_window_t *) split->get_current())->set_text(new_text);
					break;
				}

				case action_id_t::FILE_EXIT:
					//FIXME: ask whether to save/cancel
					//~ #ifdef DEBUG
					//~ delete editwin;
					//~ #endif
					exit(EXIT_SUCCESS);
					break;

				case action_id_t::EDIT_UNDO:
					((edit_window_t *) split->get_current())->undo();
					break;
				case action_id_t::EDIT_REDO:
					((edit_window_t *) split->get_current())->redo();
					break;
				case action_id_t::EDIT_COPY:
				case action_id_t::EDIT_CUT:
					((edit_window_t *) split->get_current())->cut_copy(id == action_id_t::EDIT_CUT);
					break;
				case action_id_t::EDIT_PASTE:
					((edit_window_t *) split->get_current())->paste();
					break;
				case action_id_t::EDIT_SELECT_ALL:
					((edit_window_t *) split->get_current())->select_all();
					break;
				case action_id_t::EDIT_INSERT_CHAR:
					((edit_window_t *) split->get_current())->insert_special();
					break;

				case action_id_t::SEARCH_GOTO:
					((edit_window_t *) split->get_current())->goto_line();
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
				case action_id_t::WINDOWS_HSPLIT:
				case action_id_t::WINDOWS_VSPLIT: {
					file_buffer_t *new_file = open_files.next_buffer(NULL);
					if (new_file == NULL)
						new_file = new file_buffer_t();
					split->split(new edit_window_t(new_file), id == action_id_t::WINDOWS_HSPLIT);
					break;
				}
				case action_id_t::WINDOWS_MERGE:
					delete split->unsplit();
					break;
				default:
					break;
			}
		}
};

/*
void execute_action(ActionID id, ...) {
	va_list args;
	va_start(args, id);

	switch (id) {
		case ActionID::FILE_SAVE:
			editwin->get_current()->save();
			break;
		case ActionID::FILE_OPEN:
			//FIXME: set directory to dir of current file?
			activate_window(WindowID::OPEN_FILE);
			break;
		case ActionID::FILE_OPEN_RECENT:
			activate_window(WindowID::OPEN_RECENT);
			break;
		case ActionID::FILE_SAVE_AS:
			//FIXME: set directory to dir of current file?
			activate_window(WindowID::SAVE_FILE);
			break;
		case ActionID::FILE_CLOSE:
			editwin->get_current()->close(false);
			break;
		case ActionID::FILE_REPAINT:
			do_resize();
			t3_term_redraw();
			break;
		case ActionID::FILE_SUSPEND:
			deinit_keys();
			t3_term_restore();
		#warning FIXME: should also do terminal specific restore!
			kill(getpid(), SIGSTOP);
			//FIXME: check return values!
			t3_term_init(-1, option.term);
			reinit_keys();
			do_resize();
			break;

		case ActionID::EDIT_UNDO:
			editwin->get_current()->action(EditAction::UNDO);
			break;
		case ActionID::EDIT_REDO:
			editwin->get_current()->action(EditAction::REDO);
			break;
		case ActionID::EDIT_COPY:
			editwin->get_current()->action(EditAction::COPY);
			break;
		case ActionID::EDIT_CUT:
			editwin->get_current()->action(EditAction::CUT);
			break;
		case ActionID::EDIT_PASTE:
			editwin->get_current()->action(EditAction::PASTE);
			break;
		case ActionID::EDIT_SELECT_ALL:
			editwin->get_current()->action(EditAction::SELECT_ALL);
			break;

		case ActionID::EDIT_INSERT_CHAR:
			if (components.back() != insert_char_dialog)
				activate_window(WindowID::INSERT_CHAR);
			break;

		case ActionID::SEARCH_SEARCH:
			activate_window(WindowID::FIND);
			break;
		case ActionID::SEARCH_REPLACE:
			activate_window(WindowID::REPLACE);
			break;
		case ActionID::SEARCH_AGAIN:
			find_next(true);
			break;
		case ActionID::SEARCH_AGAIN_BACKWARD:
			find_next(false);
			break;
		case ActionID::SEARCH_GOTO:
			activate_window(WindowID::GOTO_DIALOG);
			break;

		case ActionID::WINDOWS_SELECT:
			activate_window(WindowID::SELECT_BUFFER);
			break;

		//FIXME should this stay here?
		case ActionID::CLOSE_DISCARD:
			editwin->get_current()->close(true);
			break;

		case ActionID::CALL_CONTINUATION:
			(*va_arg(args, continuation_t *))();
			break;
		case ActionID::DROP_CONTINUATION: {
			continuation_t *cont = va_arg(args, continuation_t *);
			delete cont;
			//delete va_arg(args, continuation_t *);
			break;
		}
		default:
			break;
	}
	va_end(args);
}
*/

void resize(int height, int width) {
	open_file_dialog.set_size(height - 4, width - 5);
}

int main(int argc, char *argv[]) {
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

	main_t main_window;

	set_color_mode(false);
	set_attribute(attribute_t::SELECTION_CURSOR, T3_ATTR_BG_GREEN);

	main_window.show();

	set_key_timeout(100);
	//~ connect_resize(sigc::ptr_fun(resize));
	main_loop();
	return 0;
}
