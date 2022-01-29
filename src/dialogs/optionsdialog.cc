/* Copyright (C) 2011-2012,2018 G.P. Halkes
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

#include "tilde/dialogs/optionsdialog.h"
#include "tilde/main.h"
#include "tilde/option.h"

static t3widget::key_t number_keys[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

buffer_options_dialog_t::buffer_options_dialog_t(optional<std::string> _title)
    : dialog_t(10, 25, std::move(_title)) {
  smart_label_t *label = emplace_back<smart_label_t>(_("_Tab size"));
  label->set_position(1, 2);
  tabsize_field = emplace_back<text_field_t>();
  tabsize_field->set_key_filter(number_keys, ARRAY_SIZE(number_keys), true);
  tabsize_field->set_label(label);
  tabsize_field->set_size(1, 5);
  tabsize_field->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  tabsize_field->set_position(1, -2);
  tabsize_field->connect_move_focus_down([this] { focus_next(); });
  tabsize_field->connect_activate([this] { handle_activate(); });

  int width = label->get_width() + 2 + 5;

  label = emplace_back<smart_label_t>(_("Tab uses _spaces"));
  label->set_position(2, 2);
  tab_spaces_box = emplace_back<checkbox_t>();
  tab_spaces_box->set_label(label);
  tab_spaces_box->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  tab_spaces_box->set_position(2, -2);
  tab_spaces_box->connect_move_focus_up([this] { focus_previous(); });
  tab_spaces_box->connect_move_focus_down([this] { focus_next(); });
  tab_spaces_box->connect_activate([this] { handle_activate(); });

  width = std::max<int>(label->get_width() + 2 + 3, width);

  label = emplace_back<smart_label_t>(_("_Wrap text"));
  label->set_position(3, 2);
  wrap_box = emplace_back<checkbox_t>();
  wrap_box->set_label(label);
  wrap_box->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  wrap_box->set_position(3, -2);
  wrap_box->connect_move_focus_up([this] { focus_previous(); });
  wrap_box->connect_move_focus_down([this] { focus_next(); });
  wrap_box->connect_activate([this] { handle_activate(); });

  width = std::max<int>(label->get_width() + 2 + 3, width);

  label = emplace_back<smart_label_t>(_("_Automatic indentation"));
  label->set_position(4, 2);
  auto_indent_box = emplace_back<checkbox_t>();
  auto_indent_box->set_label(label);
  auto_indent_box->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  auto_indent_box->set_position(4, -2);
  auto_indent_box->connect_move_focus_up([this] { focus_previous(); });
  auto_indent_box->connect_move_focus_down([this] { focus_next(); });
  auto_indent_box->connect_activate([this] { handle_activate(); });

  width = std::max<int>(label->get_width() + 2 + 3, width);

  label = emplace_back<smart_label_t>(_("_Indent aware home key"));
  label->set_position(5, 2);
  indent_aware_home_box = emplace_back<checkbox_t>();
  indent_aware_home_box->set_label(label);
  indent_aware_home_box->set_anchor(this,
                                    T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  indent_aware_home_box->set_position(5, -2);
  indent_aware_home_box->connect_move_focus_up([this] { focus_previous(); });
  indent_aware_home_box->connect_move_focus_down([this] { focus_next(); });
  indent_aware_home_box->connect_activate([this] { handle_activate(); });

  width = std::max<int>(label->get_width() + 2 + 3, width);

  label = emplace_back<smart_label_t>(_("S_how tabs"));
  label->set_position(6, 2);
  show_tabs_box = emplace_back<checkbox_t>();
  show_tabs_box->set_label(label);
  show_tabs_box->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  show_tabs_box->set_position(6, -2);
  show_tabs_box->connect_move_focus_up([this] { focus_previous(); });
  show_tabs_box->connect_move_focus_down([this] { focus_next(); });
  show_tabs_box->connect_activate([this] { handle_activate(); });

  width = std::max<int>(label->get_width() + 2 + 3, width);

  label = emplace_back<smart_label_t>(_("St_rip trailing spaces on save"));
  label->set_position(7, 2);
  strip_spaces_box = emplace_back<checkbox_t>();
  strip_spaces_box->set_label(label);
  strip_spaces_box->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  strip_spaces_box->set_position(7, -2);
  strip_spaces_box->connect_move_focus_up([this] { focus_previous(); });
  strip_spaces_box->connect_move_focus_down([this] { focus_next(); });
  strip_spaces_box->connect_activate([this] { handle_activate(); });

  width = std::max<int>(label->get_width() + 2 + 3, width);

  button_t *ok_button = emplace_back<button_t>("_Ok", true);
  button_t *cancel_button = emplace_back<button_t>("_Cancel");

  cancel_button->set_anchor(this,
                            T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
  cancel_button->set_position(-1, -2);
  cancel_button->connect_activate([this] { close(); });
  cancel_button->connect_move_focus_up([this] { focus_previous(); });
  cancel_button->connect_move_focus_up([this] { focus_previous(); });
  cancel_button->connect_move_focus_left([this] { focus_previous(); });

  ok_button->set_anchor(cancel_button, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  ok_button->set_position(0, -2);
  ok_button->connect_move_focus_up([this] { focus_previous(); });
  ok_button->connect_move_focus_right([this] { focus_next(); });
  ok_button->connect_activate([this] { handle_activate(); });

  width = std::max(ok_button->get_width() + cancel_button->get_width() + 4, width);

  dialog_t::set_size(None, width + 4);
}

void buffer_options_dialog_t::set_values_from_view(file_edit_window_t *view) {
  char tabsize_text[20];
  sprintf(tabsize_text, "%d", view->get_tabsize());
  tabsize_field->set_text(tabsize_text);

  tab_spaces_box->set_state(view->get_tab_spaces());
  wrap_box->set_state(view->get_wrap() != wrap_type_t::NONE);
  auto_indent_box->set_state(view->get_auto_indent());
  indent_aware_home_box->set_state(view->get_indent_aware_home());
  show_tabs_box->set_state(view->get_show_tabs());
  strip_spaces_box->set_state(view->get_text()->get_strip_spaces());
}

void buffer_options_dialog_t::set_view_values(file_edit_window_t *view) {
  int tabsize = atoi(tabsize_field->get_text().c_str());
  if (tabsize > 0 && tabsize < 17) {
    view->set_tabsize(tabsize);
  }
  view->set_wrap(wrap_box->get_state() ? wrap_type_t::WORD : wrap_type_t::NONE);
  view->set_tab_spaces(tab_spaces_box->get_state());
  view->set_auto_indent(auto_indent_box->get_state());
  view->set_indent_aware_home(indent_aware_home_box->get_state());
  view->set_show_tabs(show_tabs_box->get_state());
  view->get_text()->set_strip_spaces(strip_spaces_box->get_state());
}

void buffer_options_dialog_t::set_values_from_options() {
  char tabsize_text[20];
  sprintf(tabsize_text, "%d", option.tabsize);
  tabsize_field->set_text(tabsize_text);

  tab_spaces_box->set_state(option.tab_spaces);
  wrap_box->set_state(option.wrap);
  auto_indent_box->set_state(option.auto_indent);
  indent_aware_home_box->set_state(option.indent_aware_home);
  show_tabs_box->set_state(option.show_tabs);
  strip_spaces_box->set_state(option.strip_spaces);
}

void buffer_options_dialog_t::set_options_from_values() {
  int tabsize = atoi(tabsize_field->get_text().c_str());
  if (tabsize > 0 && tabsize < 17) {
    default_option.tabsize = option.tabsize = tabsize;
  }
  default_option.wrap = option.wrap = wrap_box->get_state();
  default_option.tab_spaces = option.tab_spaces = tab_spaces_box->get_state();
  default_option.auto_indent = option.auto_indent = auto_indent_box->get_state();
  default_option.indent_aware_home = option.indent_aware_home = indent_aware_home_box->get_state();
  default_option.show_tabs = option.show_tabs = show_tabs_box->get_state();
  default_option.strip_spaces = option.strip_spaces = strip_spaces_box->get_state();
}

void buffer_options_dialog_t::handle_activate() {
  int tabsize = atoi(tabsize_field->get_text().c_str());
  if (tabsize < 1 || tabsize > 16) {
    error_dialog->set_message(_("Tab size out of range (must be between 1 and 16 inclusive)."));
    error_dialog->center_over(this);
    error_dialog->show();
    return;
  }
  hide();
  activate();
}

//===============================================================

misc_options_dialog_t::misc_options_dialog_t(optional<std::string> _title)
    : dialog_t(9, 26, std::move(_title)) {
  smart_label_t *label;
  int width = 0;

  label = emplace_back<smart_label_t>(_("_Hide menu bar"));
  label->set_position(1, 2);
  hide_menu_box = emplace_back<checkbox_t>();
  hide_menu_box->set_label(label);
  hide_menu_box->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  hide_menu_box->set_position(1, -2);
  hide_menu_box->connect_move_focus_up([this] { focus_previous(); });
  hide_menu_box->connect_move_focus_down([this] { focus_next(); });
  hide_menu_box->connect_activate([this] { handle_activate(); });

  width = std::max<int>(label->get_width() + 2 + 3, width);

  label = emplace_back<smart_label_t>(_("_Make backup on save"));
  label->set_position(2, 2);
  save_backup_box = emplace_back<checkbox_t>();
  save_backup_box->set_label(label);
  save_backup_box->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  save_backup_box->set_position(2, -2);
  save_backup_box->connect_move_focus_up([this] { focus_previous(); });
  save_backup_box->connect_move_focus_down([this] { focus_next(); });
  save_backup_box->connect_activate([this] { handle_activate(); });

  width = std::max<int>(label->get_width() + 2 + 3, width);

  label = emplace_back<smart_label_t>(_("_Parse file positions"));
  label->set_position(3, 2);
  parse_file_positions_box = emplace_back<checkbox_t>();
  parse_file_positions_box->set_label(label);
  parse_file_positions_box->set_anchor(
      this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  parse_file_positions_box->set_position(3, -2);
  parse_file_positions_box->connect_move_focus_up([this] { focus_previous(); });
  parse_file_positions_box->connect_move_focus_down([this] { focus_next(); });
  parse_file_positions_box->connect_activate([this] { handle_activate(); });

  width = std::max<int>(label->get_width() + 2 + 3, width);

  label = emplace_back<smart_label_t>(_("Disable primary selection over _SSH"));
  label->set_position(4, 2);
  disable_selection_over_ssh_box = emplace_back<checkbox_t>();
  disable_selection_over_ssh_box->set_label(label);
  disable_selection_over_ssh_box->set_anchor(
      this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  disable_selection_over_ssh_box->set_position(4, -2);
  disable_selection_over_ssh_box->connect_move_focus_up([this] { focus_previous(); });
  disable_selection_over_ssh_box->connect_move_focus_down([this] { focus_next(); });
  disable_selection_over_ssh_box->connect_activate([this] { handle_activate(); });

  width = std::max<int>(label->get_width() + 2 + 3, width);

  label = emplace_back<smart_label_t>(_("Save recent _files list"));
  label->set_position(5, 2);
  save_recent_files_box = emplace_back<checkbox_t>();
  save_recent_files_box->set_label(label);
  save_recent_files_box->set_anchor(this,
                                    T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  save_recent_files_box->set_position(5, -2);
  save_recent_files_box->connect_move_focus_up([this] { focus_previous(); });
  save_recent_files_box->connect_move_focus_down([this] { focus_next(); });
  save_recent_files_box->connect_activate([this] { handle_activate(); });

  width = std::max<int>(label->get_width() + 2 + 3, width);

  label = emplace_back<smart_label_t>(_("_Restore cursor position on open"));
  label->set_position(6, 2);
  restore_cursor_position_box = emplace_back<checkbox_t>();
  restore_cursor_position_box->set_label(label);
  restore_cursor_position_box->set_anchor(
      this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  restore_cursor_position_box->set_position(6, -2);
  restore_cursor_position_box->connect_move_focus_up([this] { focus_previous(); });
  restore_cursor_position_box->connect_move_focus_down([this] { focus_next(); });
  restore_cursor_position_box->connect_activate([this] { handle_activate(); });

  width = std::max<int>(label->get_width() + 2 + 3, width);

  button_t *ok_button = emplace_back<button_t>("_Ok", true);
  button_t *cancel_button = emplace_back<button_t>("_Cancel");

  cancel_button->set_anchor(this,
                            T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
  cancel_button->set_position(-1, -2);
  cancel_button->connect_activate([this] { close(); });
  cancel_button->connect_move_focus_up([this] { focus_previous(); });
  cancel_button->connect_move_focus_up([this] { focus_previous(); });
  cancel_button->connect_move_focus_left([this] { focus_previous(); });

  ok_button->set_anchor(cancel_button, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  ok_button->set_position(0, -2);
  ok_button->connect_move_focus_up([this] { focus_previous(); });
  ok_button->connect_move_focus_right([this] { focus_next(); });
  ok_button->connect_activate([this] { handle_activate(); });

  width = std::max(ok_button->get_width() + cancel_button->get_width() + 4, width);

  dialog_t::set_size(None, width + 4);
}

void misc_options_dialog_t::set_values_from_options() {
  hide_menu_box->set_state(option.hide_menubar);
  save_backup_box->set_state(option.make_backup);
  /* parse_file_positions and disable_primary_selection_over_ssh only affect start-up, so there is
     no value in the option struct. */
  parse_file_positions_box->set_state(default_option.parse_file_positions.value_or(true));
  disable_selection_over_ssh_box->set_state(
      default_option.disable_primary_selection_over_ssh.value_or(false));
  save_recent_files_box->set_state(option.save_recent_files);
  restore_cursor_position_box->set_state(option.restore_cursor_position);
}

void misc_options_dialog_t::set_options_from_values() {
  default_option.hide_menubar = option.hide_menubar = hide_menu_box->get_state();
  default_option.make_backup = option.make_backup = save_backup_box->get_state();
  /* parse_file_positions and disable_primary_selection_over_ssh only affect start-up, so there is
     no value in the option struct. */
  default_option.parse_file_positions = parse_file_positions_box->get_state();
  default_option.disable_primary_selection_over_ssh = disable_selection_over_ssh_box->get_state();
  if (!cli_option.disable_primary_selection && getenv("SSH_TTY") != nullptr) {
    t3widget::set_primary_selection_mode(
        !default_option.disable_primary_selection_over_ssh.value_or(false));
  }
  default_option.save_recent_files = option.save_recent_files = save_recent_files_box->get_state();
  default_option.restore_cursor_position = option.restore_cursor_position =
      restore_cursor_position_box->get_state();
}

void misc_options_dialog_t::handle_activate() {
  /* Do required validation here. */
  hide();
  activate();
}
