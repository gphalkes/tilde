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
#include <t3widget/widget.h>

#include "option.h"
#include "action.h"
#include "filebuffer.h"
#include "fileeditwindow.h"
#include "openfiles.h"
#include "dialogs/selectbufferdialog.h"
#include "dialogs/encodingdialog.h"
#include "dialogs/openrecentdialog.h"
#include "dialogs/aboutdialog.h"
#include "dialogs/optionsdialog.h"
#include "log.h"

using namespace std;
using namespace t3_widget;

#define MESSAGE_DIALOG_WIDTH 50

message_dialog_t *continue_abort_dialog;
open_file_dialog_t *open_file_dialog;
save_as_dialog_t *save_as_dialog;
message_dialog_t *close_confirm_dialog;
message_dialog_t *error_dialog;
open_recent_dialog_t *open_recent_dialog;
encoding_dialog_t *encoding_dialog;

static dialog_t *input_selection_dialog;

static void input_selection_complete(bool selection_made);
static void configure_input(bool cancel_selects_default);

class main_t : public main_window_base_t {
	private:
		menu_bar_t *menu;
		menu_panel_t *panel;
		menu_item_base_t *remove;
		split_t *split;

		select_buffer_dialog_t *select_buffer_dialog;
		about_dialog_t *about_dialog;
		options_dialog_t *options_dialog;

	public:
		main_t(void);
		virtual bool process_key(t3_widget::key_t key);
		virtual bool set_size(optint height, optint width);
		void load_cli_files_done(stepped_process_t *process);

	private:
		file_edit_window_t *get_current(void) { return (file_edit_window_t *) split->get_current(); }
		void menu_activated(int id);
		void switch_buffer(file_buffer_t *buffer);
		void switch_to_new_buffer(stepped_process_t *process);
		void close_cb(stepped_process_t *process);
		void set_options(void);
};

static main_t *main_window;


main_t::main_t(void) {
	button_t *encoding_button;
	file_edit_window_t *edit;

	menu = new menu_bar_t(option.hide_menubar);
	menu->set_size(None, t3_win_get_width(window));
	push_back(menu);
	menu->connect_activate(sigc::mem_fun(this, &main_t::menu_activated));

	panel = new menu_panel_t("_File", menu);
	panel->add_item("_New", "^N", action_id_t::FILE_NEW);
	panel->add_item("_Open...", "^O", action_id_t::FILE_OPEN);
	panel->add_item("Open _Recent...", NULL, action_id_t::FILE_OPEN_RECENT);
	panel->add_item("_Close", "^W", action_id_t::FILE_CLOSE);
	panel->add_item("_Save", "^S", action_id_t::FILE_SAVE);
	panel->add_item("Save _As...", NULL, action_id_t::FILE_SAVE_AS);
	panel->add_separator();
	panel->add_item("Re_draw Screen", NULL, action_id_t::FILE_REPAINT);
	panel->add_item("S_uspend", NULL, action_id_t::FILE_SUSPEND);
	panel->add_item("E_xit", "^Q", action_id_t::FILE_EXIT);

	panel = new menu_panel_t("_Edit", menu);
	panel->add_item("_Undo", "^Z", action_id_t::EDIT_UNDO);
	panel->add_item("_Redo", "^Y", action_id_t::EDIT_REDO);
	panel->add_separator();
	panel->add_item("_Copy", "^C", action_id_t::EDIT_COPY);
	panel->add_item("Cu_t", "^X", action_id_t::EDIT_CUT);
	panel->add_item("_Paste", "^V", action_id_t::EDIT_PASTE);
	panel->add_item("Select _All", "^A", action_id_t::EDIT_SELECT_ALL);
	panel->add_item("_Mark Selection", "^Space", action_id_t::EDIT_MARK);
	panel->add_item("_Insert Character...", "F9", action_id_t::EDIT_INSERT_CHAR);
	panel->add_separator();
	panel->add_item("In_dent Selection", "Tab", action_id_t::EDIT_INDENT_SELECTION);
	panel->add_item("U_nindent Selection", "S-Tab", action_id_t::EDIT_UNINDENT_SELECTION);

	panel = new menu_panel_t("_Search", menu);
	panel->add_item("_Find...", "^F", action_id_t::SEARCH_SEARCH);
	panel->add_item("Find _Next", "F3", action_id_t::SEARCH_AGAIN);
	panel->add_item("Find _Previous", "S-F3", action_id_t::SEARCH_AGAIN_BACKWARD);
	panel->add_item("_Replace...", "^R", action_id_t::SEARCH_REPLACE);
	panel->add_item("_Go to Line...", "^G", action_id_t::SEARCH_GOTO);

	panel = new menu_panel_t("_Window", menu);
	panel->add_item("_Next Buffer", "F6" , action_id_t::WINDOWS_NEXT_BUFFER);
	panel->add_item("_Previous Buffer", "S-F6" , action_id_t::WINDOWS_PREV_BUFFER);
	panel->add_item("_Select Buffer...", NULL, action_id_t::WINDOWS_SELECT);
	panel->add_separator();
	panel->add_item("Split _Horizontal", NULL, action_id_t::WINDOWS_HSPLIT);
	panel->add_item("Split _Vertical", NULL, action_id_t::WINDOWS_VSPLIT);
	panel->add_item("_Close Window", NULL, action_id_t::WINDOWS_MERGE);
	panel->add_item("Next Window", "F8", action_id_t::WINDOWS_NEXT_WINDOW);
	panel->add_item("Previous Window", "S-F8", action_id_t::WINDOWS_PREV_WINDOW);

	panel = new menu_panel_t("_Options", menu);
	panel->add_item("_Input Handling", NULL, action_id_t::OPTIONS_INPUT);
	panel->add_item("_Buffer Options", NULL, action_id_t::OPTIONS_BUFFER);

	panel = new menu_panel_t("_Help", menu);
	//~ panel->add_item("_Help", "F1", action_id_t::HELP_HELP);
	panel->add_item("_About", NULL, action_id_t::HELP_ABOUT);

	edit = new file_edit_window_t();
	split = new split_t(edit);
	split->set_position(!option.hide_menubar, 0);
	split->set_size(t3_win_get_height(window) - !option.hide_menubar, t3_win_get_width(window));
	push_back(split);

	select_buffer_dialog = new select_buffer_dialog_t(11, t3_win_get_width(window) - 4);
	select_buffer_dialog->center_over(this);
	select_buffer_dialog->connect_activate(sigc::mem_fun(this, &main_t::switch_buffer));

	continue_abort_dialog = new message_dialog_t(MESSAGE_DIALOG_WIDTH, "Question", "_Continue", "_Abort", NULL);
	continue_abort_dialog->center_over(this);

	encoding_dialog = new encoding_dialog_t(t3_win_get_height(window) - 8, t3_win_get_width(window) - 8);
	encoding_dialog->center_over(this);

	open_file_dialog = new open_file_dialog_t(t3_win_get_height(window) - 4, t3_win_get_width(window) - 4);
	open_file_dialog->center_over(this);
	open_file_dialog->set_file(NULL);
	encoding_button = new button_t("_Encoding");
	encoding_button->connect_activate(sigc::mem_fun(encoding_dialog, &encoding_dialog_t::show));
	open_file_dialog->set_options_widget(encoding_button);

	save_as_dialog = new save_as_dialog_t(t3_win_get_height(window) - 4, t3_win_get_width(window) - 4);
	save_as_dialog->center_over(this);
	encoding_button = new button_t("_Encoding");
	encoding_button->connect_activate(sigc::mem_fun(encoding_dialog, &encoding_dialog_t::show));
	save_as_dialog->set_options_widget(encoding_button);

	close_confirm_dialog = new message_dialog_t(MESSAGE_DIALOG_WIDTH, "Confirm", "_Yes", "_No", "_Cancel", NULL);
	close_confirm_dialog->center_over(this);

	error_dialog = new message_dialog_t(MESSAGE_DIALOG_WIDTH, "Error", "Close", NULL);
	error_dialog->center_over(this);

	open_recent_dialog = new open_recent_dialog_t(11, t3_win_get_width(window) - 4);
	open_recent_dialog->center_over(this);

	about_dialog = new about_dialog_t(15, 45);
	about_dialog->center_over(this);

	options_dialog = new options_dialog_t();
	options_dialog->center_over(this);
	options_dialog->connect_activate(sigc::mem_fun(this, &main_t::set_options));
}

bool main_t::process_key(t3_widget::key_t key) {
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
	result &= open_recent_dialog->set_size(11, width - 4);
	result &= encoding_dialog->set_size(min(height - 8, 16), min(width - 8, 72));
	return true;
}

void main_t::load_cli_files_done(stepped_process_t *process) {
	(void) process;
	if (open_files.size() > 1) {
		file_buffer_t *buffer = get_current()->get_text();
		menu_activated(action_id_t::WINDOWS_NEXT_BUFFER);
		ASSERT(buffer != get_current()->get_text());
		delete buffer;
	}
}

void main_t::menu_activated(int id) {
	switch (id) {
		case action_id_t::FILE_NEW: {
			file_buffer_t *new_text = new file_buffer_t();
			get_current()->set_text(new_text);
			break;
		}

		case action_id_t::FILE_OPEN: {
			const char *name = get_current()->get_text()->get_name();
			if (name != NULL) {
				open_file_dialog->set_file(name);
				// Because set_file also selects the named file if possible, we need to reset the dialog
				open_file_dialog->reset();
			}
			load_process_t::execute(sigc::mem_fun(this, &main_t::switch_to_new_buffer));
			break;
		}

		case action_id_t::FILE_CLOSE:
			close_process_t::execute(sigc::mem_fun(this, &main_t::close_cb), get_current()->get_text());
			break;
		case action_id_t::FILE_SAVE:
			save_process_t::execute(sigc::ptr_fun(stepped_process_t::ignore_result), get_current()->get_text());
			break;
		case action_id_t::FILE_SAVE_AS:
			save_as_process_t::execute(sigc::ptr_fun(stepped_process_t::ignore_result), get_current()->get_text());
			break;
		case action_id_t::FILE_OPEN_RECENT:
			open_recent_process_t::execute(sigc::mem_fun(this, &main_t::switch_to_new_buffer));
			break;
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
		case action_id_t::EDIT_MARK:
			get_current()->process_key(0);
			break;
		case action_id_t::EDIT_INSERT_CHAR:
			get_current()->insert_special();
			break;
		case action_id_t::EDIT_INDENT_SELECTION:
			get_current()->indent_selection();
			break;
		case action_id_t::EDIT_UNINDENT_SELECTION:
			get_current()->unindent_selection();
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
			file_edit_window_t *current = (file_edit_window_t *) split->get_current();
			current->set_text(open_files.next_buffer(current->get_text()));
			break;
		}
		case action_id_t::WINDOWS_PREV_BUFFER: {
			file_edit_window_t *current = (file_edit_window_t *) split->get_current();
			current->set_text(open_files.previous_buffer(current->get_text()));
			break;
		}
		case action_id_t::WINDOWS_SELECT:
			select_buffer_dialog->show();
			break;
		case action_id_t::WINDOWS_HSPLIT:
		case action_id_t::WINDOWS_VSPLIT: {
			file_buffer_t *new_file = open_files.next_buffer(NULL);
			// If new_file is NULL, a new file_buffer_t will be created
			split->split(new file_edit_window_t(new_file), id == action_id_t::WINDOWS_HSPLIT);
			break;
		}
		case action_id_t::WINDOWS_NEXT_WINDOW:
			split->next();
			break;
		case action_id_t::WINDOWS_PREV_WINDOW:
			split->previous();
			break;
		case action_id_t::WINDOWS_MERGE: {
			widget_t *widget = split->unsplit();
			if (widget == NULL) {
				message_dialog->set_message("Can not close the last window.");
				message_dialog->center_over(this);
				message_dialog->show();
			}
			break;
		}
		case action_id_t::OPTIONS_INPUT:
			configure_input(false);
			break;
		case action_id_t::OPTIONS_BUFFER:
			options_dialog->set_values_from_view(get_current());
			options_dialog->show();
			break;

		case action_id_t::HELP_ABOUT:
			about_dialog->show();
			break;
		default:
			break;
	}
}

void main_t::switch_buffer(file_buffer_t *buffer) {
	/* If the buffer is already opened in another file_edit_window_t, switch to that
	   window. */
	if (buffer->get_has_window()) {
		while (get_current()->get_text() != buffer)
			split->next();
	} else {
		get_current()->set_text(buffer);
	}
}

void main_t::switch_to_new_buffer(stepped_process_t *process) {
	const file_buffer_t *text;
	file_buffer_t *buffer;

	if (!process->get_result())
		return;

	buffer = ((load_process_t *) process)->get_file_buffer();
	if (buffer->get_has_window())
		return;

	text = get_current()->get_text();
	get_current()->set_text(buffer);
	//FIXME: buffer should not be closed if the user specifically created it by asking for a new file!
	if (text->get_name() == NULL && !text->is_modified())
		delete text;
}

void main_t::close_cb(stepped_process_t *process) {
	file_buffer_t *text;

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

void main_t::set_options(void) {
	options_dialog->set_view_values(get_current());
}

static void configure_input(bool cancel_selects_default) {
	input_selection_dialog_t *input_selection;
	delete input_selection_dialog;

	input_selection = new input_selection_dialog_t(20, 70);
	input_selection->connect_intuitive_activated(sigc::bind(sigc::ptr_fun(input_selection_complete), true));
	input_selection->connect_compromise_activated(sigc::bind(sigc::ptr_fun(input_selection_complete), true));
	input_selection->connect_no_timeout_activated(sigc::bind(sigc::ptr_fun(input_selection_complete), true));
	input_selection->connect_closed(sigc::bind(sigc::ptr_fun(input_selection_complete), cancel_selects_default));
	input_selection->center_over(main_window);
	input_selection->show();
	input_selection_dialog = input_selection;
}

static void input_selection_complete(bool selection_made) {
	delete input_selection_dialog;
	input_selection_dialog = NULL;
	if (selection_made) {
		term_specific_option.key_timeout = get_key_timeout();
		write_config();
	}
}


int main(int argc, char *argv[]) {
	complex_error_t result;

	init_log();
	setlocale(LC_ALL, "");
	//FIXME: call this when internationalization is started. Requires #include <libintl.h>
	// bind_textdomain_codeset("UTF-8");
	parse_args(argc, argv);

#ifdef DEBUG
	if (cli_option.start_debugger_on_segfault)
		enable_debugger_on_segfault(argv[0]);

	set_limits();
	if (cli_option.wait) {
		printf("Debug mode: Waiting for keypress to allow debugger attach\n");
		getchar();
	}
#endif

	if (!(result = init(cli_option.term)).get_success()) {
		fprintf(stderr, "Error: %s\n", result.get_string());
		fprintf(stderr, "init failed\n");
		exit(EXIT_FAILURE);
	}

	init_charsets();
	main_window = new main_t();

	set_color_mode(option.color);
	set_attributes();

	main_window->show();

	if (option.key_timeout.is_valid()) {
		set_key_timeout(option.key_timeout);
	} else {
		message_dialog_t *input_message = new message_dialog_t(70, _("Input Handling"), _("_Default"), _("_Configure"), NULL);
		input_message->set_message("You have not configured the input handling method for this terminal type yet. "
			"By default Tilde uses a workable compromise, which requires you to press esacpe twice to close a menu or dialog "
			"and allows you to access menus by pressing Escape followed by a letter. "
			"Choose 'Configure' below to select a different input handling method.");
		input_message->connect_activate(sigc::bind(sigc::ptr_fun(input_selection_complete), true), 0);
		input_message->connect_activate(sigc::bind(sigc::ptr_fun(configure_input), true), 1);
		input_message->connect_closed(sigc::bind(sigc::ptr_fun(input_selection_complete), true));
		input_message->center_over(main_window);
		input_message->show();
		input_selection_dialog = input_message;
		set_key_timeout(-1000);
	}

	load_cli_file_process_t::execute(sigc::mem_fun(main_window, &main_t::load_cli_files_done));
	main_loop();
	return 0;
}
