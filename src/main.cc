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

#ifdef DEBUG
#define _T3_WIDGET_INTERNAL
#define _T3_WIDGET_DEBUG
#include <log.h>
#endif

using namespace std;
using namespace t3_widget;

static open_file_dialog_t open_file_dialog(20, 75);


static void menu_activated(int id);

class main_t : public main_window_base_t {
	private:
		menu_bar_t *menu;
		menu_panel_t *panel;
		menu_item_base_t *remove;

	public:
		main_t(void) {
			menu = new menu_bar_t();
			push_back(menu);
			menu->connect_activate(sigc::ptr_fun(menu_activated));

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
			panel->add_item("_Go to line_t...;gG", "^G", action_id_t::SEARCH_GOTO);

			panel = new menu_panel_t(menu, "_Windows;wW");
			panel->add_item("_Next buffer_t;nN", "F6" , action_id_t::WINDOWS_NEXT_BUFFER);
			panel->add_item("_Previous buffer_t;pP", "S-F6" , action_id_t::WINDOWS_PREV_BUFFER);
			panel->add_item("_Select buffer_t...;sS", NULL, action_id_t::WINDOWS_SELECT);
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
		}

		virtual bool process_key(key_t key) {
			if (key == (EKEY_CTRL | 'q'))
				exit(EXIT_SUCCESS);
			return main_window_base_t::process_key(key);
		}

		virtual bool set_size(optint height, optint width) {
			(void) height;
			menu->set_size(None, width);
			return true;
		}
};

static void menu_activated(int id) {
	lprintf("Menu activated: %d\n", id);
	switch (id) {
		case action_id_t::FILE_EXIT:
			//FIXME: ask whether to save/cancel
			//~ #ifdef DEBUG
			//~ delete editwin;
			//~ #endif
			exit(EXIT_SUCCESS);
			break;
		default:
			break;
	}
}


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
