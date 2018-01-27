/* Copyright (C) 2011-2012 G.P. Halkes
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

#include <csignal>
#include <cstdlib>
#include <memory.h>
#include <t3widget/key_binding.h>
#include <t3widget/widget.h>

#include "action.h"
#include "dialogs/attributesdialog.h"
#include "dialogs/encodingdialog.h"
#include "dialogs/highlightdialog.h"
#include "dialogs/openrecentdialog.h"
#include "dialogs/optionsdialog.h"
#include "dialogs/selectbufferdialog.h"
#include "filebuffer.h"
#include "fileeditwindow.h"
#include "log.h"
#include "openfiles.h"
#include "option.h"
#include "string_util.h"

using namespace t3_widget;

#define MESSAGE_DIALOG_WIDTH 50
#define ATTRIBUTES_DIALOG_WIDTH 50

message_dialog_t *continue_abort_dialog;
open_file_dialog_t *open_file_dialog;
save_as_dialog_t *save_as_dialog;
message_dialog_t *close_confirm_dialog;
message_dialog_t *error_dialog;
open_recent_dialog_t *open_recent_dialog;
encoding_dialog_t *encoding_dialog;
message_dialog_t *preserve_bom_dialog;

static dialog_t *input_selection_dialog;

static std::list<window_component_t *> discard_list;

static std::string runfile_name;

static void input_selection_complete(bool selection_made);
static void configure_input(bool cancel_selects_default);
static void sync_updates();

class main_t : public main_window_base_t {
 private:
  menu_bar_t *menu;
  menu_panel_t *panel;
  split_t *split;

  select_buffer_dialog_t *select_buffer_dialog;
  message_dialog_t *about_dialog;
  buffer_options_dialog_t *buffer_options_dialog, *default_options_dialog;
  misc_options_dialog_t *misc_options_dialog;
  highlight_dialog_t *highlight_dialog;
  attributes_dialog_t *attributes_dialog;

 public:
  main_t();
  ~main_t() override;
  bool process_key(t3_widget::key_t key) override;
  bool set_size(optint height, optint width) override;
  void load_cli_files_done(stepped_process_t *process);

 private:
  file_edit_window_t *get_current() {
    return static_cast<file_edit_window_t *>(split->get_current());
  }
  void menu_activated(int id);
  void switch_buffer(file_buffer_t *buffer);
  void switch_to_new_buffer(stepped_process_t *process);
  void close_cb(stepped_process_t *process);
  void set_buffer_options();
  void set_default_options();
  void set_interface_options();
  void set_default_interface_options();
  void set_misc_options();
  void set_highlight(t3_highlight_t *highlight, const char *name);
  void save_as_done(stepped_process_t *process);

  static key_bindings_t<action_id_t> key_bindings;
};

static main_t *main_window;

key_bindings_t<action_id_t> main_t::key_bindings{
    {action_id_t::FILE_NEW, "FileNew", {EKEY_CTRL | 'n'}},
    {action_id_t::FILE_OPEN, "FileOpen", {EKEY_CTRL | 'o'}},
    {action_id_t::FILE_CLOSE, "FileClose", {EKEY_CTRL | 'w'}},
    {action_id_t::FILE_SAVE, "FileSave", {EKEY_CTRL | 's'}},
    {action_id_t::FILE_EXIT, "Exit", {EKEY_CTRL | 'q'}},
    {action_id_t::WINDOWS_NEXT_BUFFER, "NextBuffer", {EKEY_F6, EKEY_META | '6'}},
    {action_id_t::WINDOWS_PREV_BUFFER, "PreviousBuffer", {EKEY_F6 | EKEY_SHIFT}},
};

main_t::main_t() {
  button_t *encoding_button;
  file_edit_window_t *edit;

  menu = new menu_bar_t(option.hide_menubar);
  menu->set_size(None, t3_win_get_width(window));
  push_back(menu);
  menu->connect_activate(signals::mem_fun(this, &main_t::menu_activated));

  panel = new menu_panel_t("_File", menu);
  panel->add_item("_New", "^N", action_id_t::FILE_NEW);
  panel->add_item("_Open...", "^O", action_id_t::FILE_OPEN);
  panel->add_item("Open _Recent...", nullptr, action_id_t::FILE_OPEN_RECENT);
  panel->add_item("_Close", "^W", action_id_t::FILE_CLOSE);
  panel->add_item("_Save", "^S", action_id_t::FILE_SAVE);
  panel->add_item("Save _As...", nullptr, action_id_t::FILE_SAVE_AS);
  panel->add_separator();
  panel->add_item("Re_draw Screen", nullptr, action_id_t::FILE_REPAINT);
  panel->add_item("S_uspend", nullptr, action_id_t::FILE_SUSPEND);
  panel->add_item("E_xit", "^Q", action_id_t::FILE_EXIT);

  panel = new menu_panel_t("_Edit", menu);
  panel->add_item("_Undo", "^Z", action_id_t::EDIT_UNDO);
  panel->add_item("_Redo", "^Y", action_id_t::EDIT_REDO);
  panel->add_separator();
  panel->add_item("Cu_t", "^X", action_id_t::EDIT_CUT);
  panel->add_item("_Copy", "^C", action_id_t::EDIT_COPY);
  panel->add_item("_Paste", "^V", action_id_t::EDIT_PASTE);
  panel->add_separator();
  panel->add_item("Select _All", "^A", action_id_t::EDIT_SELECT_ALL);
  panel->add_item("_Mark Selection", "^T", action_id_t::EDIT_MARK);
  panel->add_item("Paste _Selection", "S-Ins", action_id_t::EDIT_PASTE_SELECTION);
  panel->add_separator();
  panel->add_item("_Delete Line", "^K", action_id_t::EDIT_DELETE_LINE);
  panel->add_item("Insert C_haracter...", "F9", action_id_t::EDIT_INSERT_CHAR);
  panel->add_item("T_oggle INS/OVR", "Ins", action_id_t::EDIT_TOGGLE_INSERT);

  panel = new menu_panel_t("_Search", menu);
  panel->add_item("_Find...", "^F", action_id_t::SEARCH_SEARCH);
  panel->add_item("Find _Next", "F3", action_id_t::SEARCH_AGAIN);
  panel->add_item("Find _Previous", "S-F3", action_id_t::SEARCH_AGAIN_BACKWARD);
  panel->add_item("_Replace...", "^R", action_id_t::SEARCH_REPLACE);
  panel->add_separator();
  panel->add_item("_Go to Line...", "^G", action_id_t::SEARCH_GOTO);
  panel->add_item("Go to matching _brace", "^]", action_id_t::SEARCH_GOTO_MATCHING_BRACE);

  panel = new menu_panel_t("_Window", menu);
  panel->add_item("_Next Buffer", "F6", action_id_t::WINDOWS_NEXT_BUFFER);
  panel->add_item("_Previous Buffer", "S-F6", action_id_t::WINDOWS_PREV_BUFFER);
  panel->add_item("_Select Buffer...", nullptr, action_id_t::WINDOWS_SELECT);
  panel->add_separator();
  panel->add_item("Split _Horizontal", nullptr, action_id_t::WINDOWS_HSPLIT);
  panel->add_item("Split _Vertical", nullptr, action_id_t::WINDOWS_VSPLIT);
  panel->add_item("_Close Window", nullptr, action_id_t::WINDOWS_MERGE);
  panel->add_item("Next Window", "F8", action_id_t::WINDOWS_NEXT_WINDOW);
  panel->add_item("Previous Window", "S-F8", action_id_t::WINDOWS_PREV_WINDOW);

  panel = new menu_panel_t("_Tools", menu);
  panel->add_item("_Highlighting...", nullptr, action_id_t::TOOLS_HIGHLIGHTING);
  panel->add_item("_Strip trailing spaces", nullptr, action_id_t::TOOLS_STRIP_SPACES);
  panel->add_item("_Autocomplete", "C-Space", action_id_t::TOOLS_AUTOCOMPLETE);
  panel->add_item("_Toggle line comment", "C-/", action_id_t::TOOLS_TOGGLE_LINE_COMMENT);
  panel->add_item("_Indent Selection", "Tab", action_id_t::TOOLS_INDENT_SELECTION);
  panel->add_item("_Unindent Selection", "S-Tab", action_id_t::TOOLS_UNINDENT_SELECTION);

  panel = new menu_panel_t("_Options", menu);
  panel->add_item("Input _Handling...", nullptr, action_id_t::OPTIONS_INPUT);
  panel->add_item("_Current Buffer...", nullptr, action_id_t::OPTIONS_BUFFER);
  panel->add_item("Buffer _Defaults...", nullptr, action_id_t::OPTIONS_DEFAULTS);
  panel->add_item("_Interface...", nullptr, action_id_t::OPTIONS_INTERFACE);

  panel->add_item("_Miscellaneous...", nullptr, action_id_t::OPTIONS_MISC);

  panel = new menu_panel_t("_Help", menu);
  // FIXME: reinstate when help is actually available.
  //~ panel->add_item("_Help", "F1", action_id_t::HELP_HELP);
  panel->add_item("_About", nullptr, action_id_t::HELP_ABOUT);

  edit = new file_edit_window_t();
  split = new split_t(edit);
  split->set_position(!option.hide_menubar, 0);
  split->set_size(t3_win_get_height(window) - !option.hide_menubar, t3_win_get_width(window));
  push_back(split);

  select_buffer_dialog = new select_buffer_dialog_t(11, t3_win_get_width(window) - 4);
  select_buffer_dialog->center_over(this);
  select_buffer_dialog->connect_activate(signals::mem_fun(this, &main_t::switch_buffer));

  continue_abort_dialog =
      new message_dialog_t(MESSAGE_DIALOG_WIDTH, "Question", "_Continue", "_Abort", NULL);
  continue_abort_dialog->center_over(this);

  encoding_dialog =
      new encoding_dialog_t(t3_win_get_height(window) - 8, t3_win_get_width(window) - 8);
  encoding_dialog->center_over(this);

  open_file_dialog =
      new open_file_dialog_t(t3_win_get_height(window) - 4, t3_win_get_width(window) - 4);
  open_file_dialog->center_over(this);
  open_file_dialog->set_file(nullptr);
  encoding_button = new button_t("_Encoding");
  encoding_button->connect_activate(signals::mem_fun(encoding_dialog, &encoding_dialog_t::show));
  open_file_dialog->set_options_widget(encoding_button);

  save_as_dialog =
      new save_as_dialog_t(t3_win_get_height(window) - 4, t3_win_get_width(window) - 4);
  save_as_dialog->center_over(this);
  encoding_button = new button_t("_Encoding");
  encoding_button->connect_activate(signals::mem_fun(encoding_dialog, &encoding_dialog_t::show));
  save_as_dialog->set_options_widget(encoding_button);

  close_confirm_dialog =
      new message_dialog_t(MESSAGE_DIALOG_WIDTH, "Confirm", "_Yes", "_No", "_Cancel", NULL);
  close_confirm_dialog->center_over(this);

  error_dialog = new message_dialog_t(MESSAGE_DIALOG_WIDTH, "Error", "Close", NULL);
  error_dialog->center_over(this);

  open_recent_dialog = new open_recent_dialog_t(11, t3_win_get_width(window) - 4);
  open_recent_dialog->center_over(this);

  about_dialog = new message_dialog_t(45, "About", "Close", NULL);
  about_dialog->center_over(this);
  about_dialog->set_max_text_height(13);
  about_dialog->set_message(
      "Tilde - The intuitive text editor\n\nVersion <VERSION>\nCopyright (c) 2011-2017 G.P. "
      "Halkes\n\n"  // @copyright
      "The Tilde text editor is licensed under the GNU General Public License version 3. "
      "You should have received a copy of the GNU General Public License along with this program. "
      "If not, see <http://www.gnu.org/licenses/>.");

  buffer_options_dialog = new buffer_options_dialog_t("Current Buffer");
  buffer_options_dialog->center_over(this);
  buffer_options_dialog->connect_activate(signals::mem_fun(this, &main_t::set_buffer_options));

  default_options_dialog = new buffer_options_dialog_t("Buffer Defaults");
  default_options_dialog->center_over(this);
  default_options_dialog->connect_activate(signals::mem_fun(this, &main_t::set_default_options));

  misc_options_dialog = new misc_options_dialog_t("Miscellaneous");
  misc_options_dialog->center_over(this);
  misc_options_dialog->connect_activate(signals::mem_fun(this, &main_t::set_misc_options));

  highlight_dialog = new highlight_dialog_t(t3_win_get_height(window) - 4, 40);
  highlight_dialog->center_over(this);
  highlight_dialog->connect_language_selected(signals::mem_fun(this, &main_t::set_highlight));

  preserve_bom_dialog = new message_dialog_t(MESSAGE_DIALOG_WIDTH, "Question", "_Yes", "_No", NULL);
  preserve_bom_dialog->set_message(
      "The file starts with a Byte Order Mark (BOM). "
      "This is used on some platforms to recognise UTF-8 encoded files. On Unix-like systems "
      "however, the presence of the BOM is undesirable. Do you want to preserve the BOM?");
  preserve_bom_dialog->center_over(this);

  attributes_dialog = new attributes_dialog_t(ATTRIBUTES_DIALOG_WIDTH);
  attributes_dialog->center_over(this);
  attributes_dialog->connect_activate(signals::mem_fun(this, &main_t::set_interface_options));
  attributes_dialog->connect_save_defaults(
      signals::mem_fun(this, &main_t::set_default_interface_options));
}

main_t::~main_t() {
#ifdef TILDE_DEBUG
  delete select_buffer_dialog;
  delete about_dialog;
  delete buffer_options_dialog, delete default_options_dialog;
  delete attributes_dialog;
  delete misc_options_dialog;
  delete highlight_dialog;
#endif
}

bool main_t::process_key(t3_widget::key_t key) {
  optional<action_id_t> action = key_bindings.find_action(key);
  if (action.is_valid()) {
    menu_activated(action());
  } else {
    return main_window_base_t::process_key(key);
  }
  return true;
}

bool main_t::set_size(optint height, optint width) {
  bool result;

  result = menu->set_size(None, width);
  result &= split->set_size(height - !option.hide_menubar, width);
  result &= select_buffer_dialog->set_size(None, width - 4);
  result &= open_file_dialog->set_size(height - 4, width - 4);
  result &= save_as_dialog->set_size(height - 4, width - 4);
  result &= open_recent_dialog->set_size(11, width - 4);
  result &= encoding_dialog->set_size(std::min(height - 8, 16), std::min(width - 8, 72));
  result &= highlight_dialog->set_size(height - 4, None);
  if (input_selection_dialog != nullptr &&
      dynamic_cast<input_selection_dialog_t *>(input_selection_dialog) != nullptr) {
    int is_width = std::min(std::max(width - 16, 40), 100);
    int is_height = std::min(std::max(height - 3, 15), 3200 / is_width);
    result &= input_selection_dialog->set_size(is_height, is_width);
  }
  return result;
}

void main_t::load_cli_files_done(stepped_process_t *process) {
  (void)process;
  if (open_files.size() > 1) {
    file_buffer_t *buffer = get_current()->get_text();
    menu_activated(action_id_t::WINDOWS_NEXT_BUFFER);
    ASSERT(buffer != get_current()->get_text());
    delete buffer;
  }
  /* FIXME: we should really come up with a better way, then just assuming that
     when an error_line is present that we should jump there. */
  if (config_read_error_line != 0) {
    get_current()->goto_line(config_read_error_line);
    config_read_error_line = 0;
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
      if (name != nullptr) {
        open_file_dialog->set_file(name);
        // Because set_file also selects the named file if possible, we need to reset the dialog
        open_file_dialog->reset();
      }
      load_process_t::execute(signals::mem_fun(this, &main_t::switch_to_new_buffer));
      break;
    }

    case action_id_t::FILE_CLOSE:
      close_process_t::execute(signals::mem_fun(this, &main_t::close_cb),
                               get_current()->get_text());
      break;
    case action_id_t::FILE_SAVE:
      save_process_t::execute(signals::mem_fun(this, &main_t::save_as_done),
                              get_current()->get_text());
      break;
    case action_id_t::FILE_SAVE_AS:
      save_as_process_t::execute(signals::mem_fun(this, &main_t::save_as_done),
                                 get_current()->get_text());
      break;
    case action_id_t::FILE_OPEN_RECENT:
      open_recent_process_t::execute(signals::mem_fun(this, &main_t::switch_to_new_buffer));
      break;
    case action_id_t::FILE_REPAINT:
      t3_widget::redraw();
      break;
    case action_id_t::FILE_SUSPEND:
      suspend();
      break;

    case action_id_t::FILE_EXIT:
      exit_process_t::execute(signals::ptr_fun(stepped_process_t::ignore_result));
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
    case action_id_t::EDIT_PASTE_SELECTION:
      get_current()->paste_selection();
      break;
    case action_id_t::EDIT_INSERT_CHAR:
      get_current()->insert_special();
      break;
    case action_id_t::TOOLS_INDENT_SELECTION:
      get_current()->indent_selection();
      break;
    case action_id_t::TOOLS_UNINDENT_SELECTION:
      get_current()->unindent_selection();
      break;
    case action_id_t::EDIT_TOGGLE_INSERT:
      get_current()->process_key(EKEY_INS);
      break;
    case action_id_t::EDIT_DELETE_LINE:
      get_current()->delete_line();
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
    case action_id_t::SEARCH_GOTO_MATCHING_BRACE:
      get_current()->goto_matching_brace();
      break;

    case action_id_t::WINDOWS_NEXT_BUFFER: {
      file_edit_window_t *current = static_cast<file_edit_window_t *>(split->get_current());
      current->set_text(open_files.next_buffer(current->get_text()));
      break;
    }
    case action_id_t::WINDOWS_PREV_BUFFER: {
      file_edit_window_t *current = static_cast<file_edit_window_t *>(split->get_current());
      current->set_text(open_files.previous_buffer(current->get_text()));
      break;
    }
    case action_id_t::WINDOWS_SELECT:
      select_buffer_dialog->show();
      break;
    case action_id_t::WINDOWS_HSPLIT:
    case action_id_t::WINDOWS_VSPLIT: {
      file_buffer_t *new_file = open_files.next_buffer(nullptr);
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
      file_edit_window_t *widget = static_cast<file_edit_window_t *>(split->unsplit());
      if (widget == nullptr) {
        message_dialog->set_message("Can not close the last window.");
        message_dialog->center_over(this);
        message_dialog->show();
      } else {
        file_buffer_t *text = widget->get_text();
        if (text->get_name() == nullptr && !text->is_modified()) {
          delete text;
        }
        delete widget;
      }
      break;
    }

    case action_id_t::TOOLS_HIGHLIGHTING:
      highlight_dialog->set_selected(
          t3_highlight_get_langfile(get_current()->get_text()->get_highlight()));
      highlight_dialog->show();
      break;
    case action_id_t::TOOLS_STRIP_SPACES:
      get_current()->get_text()->do_strip_spaces();
      break;
    case action_id_t::TOOLS_AUTOCOMPLETE:
      get_current()->autocomplete();
      break;
    case action_id_t::TOOLS_TOGGLE_LINE_COMMENT:
      get_current()->get_text()->toggle_line_comment();
      break;

    case action_id_t::OPTIONS_INPUT:
      configure_input(false);
      break;
    case action_id_t::OPTIONS_BUFFER:
      buffer_options_dialog->set_values_from_view(get_current());
      buffer_options_dialog->show();
      break;
    case action_id_t::OPTIONS_DEFAULTS:
      default_options_dialog->set_values_from_options();
      default_options_dialog->show();
      break;
    case action_id_t::OPTIONS_INTERFACE:
      attributes_dialog->set_values_from_options();
      attributes_dialog->show();
      break;
    case action_id_t::OPTIONS_MISC:
      misc_options_dialog->set_values_from_options();
      misc_options_dialog->show();
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
    while (get_current()->get_text() != buffer) {
      split->next();
    }
  } else {
    get_current()->set_text(buffer);
  }
}

void main_t::switch_to_new_buffer(stepped_process_t *process) {
  const file_buffer_t *text;
  file_buffer_t *buffer;

  if (!process->get_result()) {
    return;
  }

  buffer = static_cast<load_process_t *>(process)->get_file_buffer();
  if (buffer->get_has_window()) {
    return;
  }

  text = get_current()->get_text();
  get_current()->set_text(buffer);
  // FIXME: buffer should not be closed if the user specifically created it by asking for a new
  // file!
  if (text->get_name() == nullptr && !text->is_modified()) {
    delete text;
  }
}

void main_t::close_cb(stepped_process_t *process) {
  file_buffer_t *text;

  if (!process->get_result()) {
    return;
  }

  text = get_current()->get_text();
  if (((close_process_t *)process)->get_file_buffer_ptr() != text) {
    PANIC();
  }

  menu_activated(action_id_t::WINDOWS_NEXT_BUFFER);
  if (get_current()->get_text() == text) {
    get_current()->set_text(new file_buffer_t());
  }
  delete text;
}

void main_t::set_buffer_options() { buffer_options_dialog->set_view_values(get_current()); }

void main_t::set_default_options() {
  default_options_dialog->set_options_from_values();
  write_config();
}

void main_t::set_interface_options() {
  /* First set color mode, because that resets all the attributes to the defaults. */
  set_color_mode(option.color);
  attributes_dialog->set_term_options_from_values();
  write_config();
}

void main_t::set_default_interface_options() {
  /* First set color mode, because that resets all the attributes to the defaults. */
  set_color_mode(option.color);
  attributes_dialog->set_default_options_from_values();
  write_config();
}

void main_t::set_misc_options() {
  misc_options_dialog->set_options_from_values();
  split->set_size(t3_win_get_height(window) - !option.hide_menubar, None);
  split->set_position(!option.hide_menubar, 0);
  menu->set_hidden(option.hide_menubar);
  write_config();
}

void main_t::set_highlight(t3_highlight_t *highlight, const char *name) {
  get_current()->get_text()->set_highlight(highlight);
  if (name == nullptr) {
    get_current()->get_text()->set_line_comment(nullptr);
  } else {
    std::map<std::string, std::string>::iterator iter = option.line_comment_map.find(name);
    if (iter == option.line_comment_map.end()) {
      get_current()->get_text()->set_line_comment(nullptr);
    } else {
      get_current()->get_text()->set_line_comment(iter->second.c_str());
    }
  }
  get_current()->force_redraw();
}

void main_t::save_as_done(stepped_process_t *process) {
  get_current()->draw_info_window();
  if (reinterpret_cast<save_as_process_t *>(process)->get_highlight_changed()) {
    get_current()->force_redraw();
  }
}

static void configure_input(bool cancel_selects_default) {
  input_selection_dialog_t *input_selection;
  int height, width, is_width, is_height;

  discard_list.push_back(input_selection_dialog);
  signal_update();

  t3_term_get_size(&height, &width);
  is_width = std::min(std::max(width - 16, 40), 100);
  is_height = std::min(std::max(height - 3, 15), 3200 / is_width);

  input_selection = new input_selection_dialog_t(is_height, is_width);
  input_selection->connect_activate(
      signals::bind(signals::ptr_fun(input_selection_complete), true));
  input_selection->connect_closed(
      signals::bind(signals::ptr_fun(input_selection_complete), cancel_selects_default));
  input_selection->center_over(main_window);
  input_selection->show();
  input_selection_dialog = input_selection;
}

static void input_selection_complete(bool selection_made) {
  discard_list.push_back(input_selection_dialog);
  signal_update();

  input_selection_dialog = nullptr;
  if (selection_made) {
    term_specific_option.key_timeout = get_key_timeout();
    write_config();
  }
}

static void sync_updates() {
  if (!discard_list.empty()) {
    for (window_component_t *iter : discard_list) {
      delete iter;
    }
    discard_list.clear();
  }
}

static std::string escape_illegal_chars(const std::string &str) {
  static const char *ILLEGAL_CHARS = "%<>:\"'/\\|?*'";
  std::string result;
  result.reserve(str.size());
  for (char c : str) {
    if (strchr(ILLEGAL_CHARS, c) != nullptr) {
      char buffer[4];
      sprintf(buffer, "%%%02X", c);
      result.append(buffer);
    } else {
      result.append(1, c);
    }
  }
  return result;
}

static std::string get_run_file_name() {
  static const char *TMP_DIR_BASE = "/tmp/tilde-";
  char hostname[128];
  std::string linkname;
  cleanup_free_ptr<char>::t path;
  char *ttyname_str;

  if (gethostname(hostname, sizeof(hostname)) == -1) {
    return "";
  }
  /* Ensure that the hostname string is terminated. */
  hostname[sizeof(hostname) - 1] = 0;
  if ((ttyname_str = ttyname(STDIN_FILENO)) == nullptr) {
    return "";
  }

  path = t3_config_xdg_get_path(T3_CONFIG_XDG_RUNTIME_DIR, "tilde", 0);

  if (path == nullptr) {
    linkname = strings::Cat(TMP_DIR_BASE, "-", geteuid());
  } else {
    linkname = path;
  }

  mkdir(path, S_IRWXU);

  strings::Append(&linkname, "/", hostname, ":", escape_illegal_chars(ttyname_str));
  return linkname;
}

static void check_if_already_running() {
  std::string name;
  char pid_str[16];
  ssize_t readlink_result;

  if (cli_option.ignore_running) {
    return;
  }

  name = get_run_file_name();
  if (name.empty()) {
    return;
  }

  if ((readlink_result = readlink(name.c_str(), pid_str, sizeof(pid_str) - 1)) > 0) {
    char *endptr;
    pid_str[readlink_result] = 0;
    int other_pid = strtol(pid_str, &endptr, 10);
    if (*endptr == 0) {
      if (kill(other_pid, 0) == 0) {
        printf(
            "Another instance of Tilde was detected running on this terminal. Use fg to bring it "
            "to the foreground, or "
            "start Tilde with --ignore-running to start a new instance.\n");
        exit(EXIT_FAILURE);
      }
    }
  }

  sprintf(pid_str, "%u", getpid());
  unlink(name.c_str());
  if (symlink(pid_str, name.c_str()) == 0) {
    runfile_name = name;
    return;
  }
}

static void terminate_handler(int sig) {
  lprintf("received signal %d\n", sig);
  t3_widget::async_safe_exit_main_loop(sig + 128);
}

static void setup_term_signal_handler(int sig) {
  struct sigaction sa;
  sa.sa_handler = ::terminate_handler;
  sigemptyset(&sa.sa_mask);
  sigaddset(&sa.sa_mask, sig);
  sa.sa_flags = SA_RESETHAND;
  sigaction(sig, &sa, nullptr);
}

static void setup_ign_signal_handler(int sig) {
  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sigaddset(&sa.sa_mask, sig);
  sa.sa_flags = 0;
  sigaction(sig, &sa, nullptr);
}

static void setup_signal_handlers() {
  setup_term_signal_handler(SIGHUP);
  setup_term_signal_handler(SIGTERM);
  setup_ign_signal_handler(SIGUSR1);
  setup_ign_signal_handler(SIGUSR2);
}

int main(int argc, char *argv[]) {
  complex_error_t result;
  std::unique_ptr<init_parameters_t> params(init_parameters_t::create());
  std::string config_file_name;

  init_log();
  setlocale(LC_ALL, "");
  // FIXME: call this when internationalization is started. Requires #include <libintl.h>
  // bind_textdomain_codeset("UTF-8");

  parse_args(argc, argv);
  check_if_already_running();

#ifdef DEBUG
  if (cli_option.start_debugger_on_segfault) {
    enable_debugger_on_segfault(argv[0]);
  }

  set_limits();
  if (cli_option.wait) {
    printf("Debug mode: Waiting for keypress to allow debugger attach\n");
    getchar();
  }
#endif
  params->term = cli_option.term;
  params->program_name = "Tilde";
  params->disable_external_clipboard = cli_option.disable_external_clipboard;
  if ((default_option.disable_primary_selection_over_ssh.value_or_default(false) &&
       getenv("SSH_TTY") != nullptr) ||
      cli_option.disable_primary_selection) {
    t3_widget::set_primary_selection_mode(false);
  }

  if (!(result = init(params.get())).get_success()) {
    fprintf(stderr, "Error: %s\n", result.get_string());
    fprintf(stderr, "init failed\n");
    exit(EXIT_FAILURE);
  }

  params.reset();

  connect_update_notification(signals::ptr_fun(sync_updates));

  init_charsets();
  main_window = new main_t();

  set_color_mode(option.color);
  set_attributes();

  main_window->show();

  if (option.key_timeout.is_valid()) {
    set_key_timeout(option.key_timeout);
  } else if (config_read_error) {
    std::string message = "Error loading configuration file ";
    if (!cli_option.config_file.is_valid()) {
      cleanup_free_ptr<char>::t file_name(
          t3_config_xdg_get_path(T3_CONFIG_XDG_CONFIG_HOME, "tilde", 0));
      cli_option.config_file = strings::Cat(file_name.get(), "/config");
    }

    strings::Append(&message, cli_option.config_file, ": ", config_read_error_string);
    if (config_read_error_line != 0) {
      strings::Append(&message, " at line ", config_read_error_line);
    }
    message +=
        ".\nInput handling and all other settings have been set to their defaults for this "
        "session.";
    set_key_timeout(-1000);

    /* For parse errors, duplicate keys, invalid keys, constraint violations,
       etc., load the config file in the text editor and jump to the correct
       line. We won't load any other files in this case.
    */
    if (config_read_error_line != 0) {
      cli_option.files.clear();
      cli_option.files.push_back(cli_option.config_file().c_str());
    }

    message_dialog->set_message(&message);
    message_dialog->center_over(main_window);
    message_dialog->show();
  } else {
    set_key_timeout(-1000);
    message_dialog_t *input_message =
        new message_dialog_t(70, _("Input Handling"), _("Close"), _("Configure"), NULL);
    input_message->set_message(
        "You have not configured the input handling for this terminal type yet. "
        "This means you:\n"
        "- can access menus by both Meta+<letter> and Esc <letter>\n"
        "- must press escape twice to close a menu or dialog\n\n"
        "You can change the input handling by selecting the \"Options\" menu "
        "and choosing \"Input Handling\", or by choosing \"Configure\" below.");
    input_message->connect_activate(signals::bind(signals::ptr_fun(input_selection_complete), true),
                                    0);
    input_message->connect_activate(signals::bind(signals::ptr_fun(configure_input), true), 1);
    input_message->connect_closed(signals::bind(signals::ptr_fun(input_selection_complete), true));
    input_message->center_over(main_window);
    input_message->show();
    input_selection_dialog = input_message;
  }

  load_cli_file_process_t::execute(signals::mem_fun(main_window, &main_t::load_cli_files_done));
  setup_signal_handlers();
  int retval = main_loop();
#ifdef TILDE_DEBUG
  delete continue_abort_dialog;
  delete open_file_dialog;
  delete save_as_dialog;
  delete close_confirm_dialog;
  delete error_dialog;
  delete open_recent_dialog;
  delete encoding_dialog;
  delete main_window;
  delete preserve_bom_dialog;
  recent_files.cleanup();
  open_files.cleanup();
  cleanup();
  transcript_finalize();
#endif
  if (runfile_name.empty()) {
    unlink(runfile_name.c_str());
  }
  if (retval > 128) {
    fprintf(stderr, "Killed by signal %d\n", retval - 128);
  }
  return retval;
}
