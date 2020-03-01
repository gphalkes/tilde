/* Copyright (C) 2012,2018-2019 G.P. Halkes
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

#include "tilde/dialogs/attributesdialog.h"

#include "tilde/option_access.h"

struct attributes_dialog_t::attribute_access_t {
  std::string name;
  attribute_key_t attribute;
  optional<t3_attr_t> attributes_dialog_t::*dialog_member;
  attribute_test_line_t *attributes_dialog_t::*dialog_line;
  bool text_background;
  WidgetGroup widget_group;
  std::string desc;
};

const attributes_dialog_t::attribute_access_t attributes_dialog_t::attribute_access[] = {
    {"dialog", DIALOG, &attributes_dialog_t::dialog, &attributes_dialog_t::dialog_line, false,
     INTERFACE, "Dialog"},
    {"dialog_selected", DIALOG_SELECTED, &attributes_dialog_t::dialog_selected,
     &attributes_dialog_t::dialog_selected_line, false, INTERFACE, "Dialog selected"},
    {"shadow", SHADOW, &attributes_dialog_t::shadow, &attributes_dialog_t::shadow_line, false,
     INTERFACE, "Shadow"},
    {"background", BACKGROUND, &attributes_dialog_t::background,
     &attributes_dialog_t::background_line, false, INTERFACE, "Background"},
    {"hotkey_highlight", HOTKEY_HIGHLIGHT, &attributes_dialog_t::hotkey_highlight,
     &attributes_dialog_t::hotkey_highlight_line, false, INTERFACE, "Hotkey highlight"},
    {"bad_draw", BAD_DRAW, &attributes_dialog_t::bad_draw, &attributes_dialog_t::bad_draw_line,
     false, INTERFACE, "Badly drawn character"},
    {"non_print", NON_PRINT, &attributes_dialog_t::non_print, &attributes_dialog_t::non_print_line,
     false, INTERFACE, "Unprintable character"},
    {"button_selected", BUTTON_SELECTED, &attributes_dialog_t::button_selected,
     &attributes_dialog_t::button_selected_line, false, INTERFACE, "Button Selected"},
    {"scrollbar", SCROLLBAR, &attributes_dialog_t::scrollbar, &attributes_dialog_t::scrollbar_line,
     false, INTERFACE, "Scrollbar"},
    {"menubar", MENUBAR, &attributes_dialog_t::menubar, &attributes_dialog_t::menubar_line, false,
     INTERFACE, "Menu bar"},
    {"menubar_selected", MENUBAR_SELECTED, &attributes_dialog_t::menubar_selected,
     &attributes_dialog_t::menubar_selected_line, false, INTERFACE, "Menu bar selected"},

    {"text", TEXT, &attributes_dialog_t::text, &attributes_dialog_t::text_line, false, TEXT_AREA,
     "Text"},
    {"text_selected", TEXT_SELECTED, &attributes_dialog_t::text_selected,
     &attributes_dialog_t::text_selected_line, false, TEXT_AREA, "Selected text"},
    {"text_cursor", TEXT_CURSOR, &attributes_dialog_t::text_cursor,
     &attributes_dialog_t::text_cursor_line, false, TEXT_AREA, "Cursor"},
    {"text_selection_cursor", TEXT_SELECTION_CURSOR, &attributes_dialog_t::text_selection_cursor,
     &attributes_dialog_t::text_selection_cursor_line, false, TEXT_AREA, "Selection cursor at end"},
    {"text_selection_cursor2", TEXT_SELECTION_CURSOR2, &attributes_dialog_t::text_selection_cursor2,
     &attributes_dialog_t::text_selection_cursor2_line, false, TEXT_AREA,
     "Selection cursor at start"},
    {"meta_text", META_TEXT, &attributes_dialog_t::meta_text, &attributes_dialog_t::meta_text_line,
     true, TEXT_AREA, "Wrap indicators"},
    {"brace_highlight", BRACE_HIGHLIGHT, &attributes_dialog_t::brace_highlight,
     &attributes_dialog_t::brace_highlight_line, true, TEXT_AREA, "Brace highlight"},

    {"comment", COMMENT, &attributes_dialog_t::comment, &attributes_dialog_t::comment_line, true,
     HIGHLIGHT, "Comment"},
    {"comment-keyword", COMMENT_KEYWORD, &attributes_dialog_t::comment_keyword,
     &attributes_dialog_t::comment_keyword_line, true, HIGHLIGHT, "Comment keyword"},
    {"keyword", KEYWORD, &attributes_dialog_t::keyword, &attributes_dialog_t::keyword_line, true,
     HIGHLIGHT, "Keyword"},
    {"number", NUMBER, &attributes_dialog_t::number, &attributes_dialog_t::number_line, true,
     HIGHLIGHT, "Number"},
    {"string", STRING, &attributes_dialog_t::string, &attributes_dialog_t::string_line, true,
     HIGHLIGHT, "String"},
    {"string-escape", STRING_ESCAPE, &attributes_dialog_t::string_escape,
     &attributes_dialog_t::string_escape_line, true, HIGHLIGHT, "String escape"},
    {"misc", MISC, &attributes_dialog_t::misc, &attributes_dialog_t::misc_line, true, HIGHLIGHT,
     "Miscellaneous"},
    {"variable", VARIABLE, &attributes_dialog_t::variable, &attributes_dialog_t::variable_line,
     true, HIGHLIGHT, "Variable"},
    {"error", ERROR, &attributes_dialog_t::error, &attributes_dialog_t::error_line, true, HIGHLIGHT,
     "Error"},
    {"addition", ADDITION, &attributes_dialog_t::addition, &attributes_dialog_t::addition_line,
     true, HIGHLIGHT, "Addition"},
    {"deletion", DELETION, &attributes_dialog_t::deletion, &attributes_dialog_t::deletion_line,
     true, HIGHLIGHT, "Deletion"},
};

optional<t3_attr_t> term_options_t::*get_term_options_member(const std::string &name) {
  const option_access_t *option_access = get_option_access(name);
  return option_access == nullptr ? nullptr : option_access->t3_attr_t_term_opt;
}

t3widget::expander_t *attributes_dialog_t::new_widget_group(WidgetGroup group,
                                                            const std::string &group_name,
                                                            int width) {
  widget_group_t *widget_group = new widget_group_t();
  int widget_count = 0;

  for (const attribute_access_t &access : attribute_access) {
    if (access.widget_group != group) {
      continue;
    }
    smart_label_t *attribute_label = widget_group->emplace_back<smart_label_t>(access.desc);
    attribute_label->set_position(widget_count, 0);
    button_t *change_button = widget_group->emplace_back<button_t>("Change");
    change_button->set_anchor(widget_group,
                              T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
    change_button->set_position(widget_count, 0);
    change_button->connect_activate([this, &access] { change_button_activated(&access); });
    change_button->connect_move_focus_up([widget_group] { widget_group->focus_previous(); });
    change_button->connect_move_focus_down([widget_group] { widget_group->focus_next(); });
    this->*access.dialog_line = widget_group->emplace_back<attribute_test_line_t>();
    (this->*access.dialog_line)
        ->set_anchor(change_button, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
    (this->*access.dialog_line)->set_position(0, -2);
    widget_count++;
  }
  widget_group->set_size(widget_count, width - 4);
  expander_t *expander = emplace_back<expander_t>(group_name);
  expander->set_child(wrap_unique(widget_group));
  expander->connect_move_focus_up([this] { focus_previous(); });
  expander->connect_move_focus_down([this] { focus_next(); });
  expander_group->add_expander(expander);
  return expander;
}

// FIXME: we may be better of using a list_pane_t for the longer divisions
attributes_dialog_t::attributes_dialog_t(int width)
    : dialog_t(7, width, _("Interface")), defaults(&default_option.term_options) {
  smart_label_t *label = emplace_back<smart_label_t>(_("Color _mode"));
  label->set_position(1, 2);
  color_box = emplace_back<checkbox_t>();
  color_box->set_label(label);
  color_box->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  color_box->set_position(1, -2);
  color_box->connect_move_focus_up([this] { focus_previous(); });
  color_box->connect_move_focus_down([this] { focus_next(); });
  color_box->connect_activate([this] { handle_activate(); });
  color_box->connect_toggled([this] { update_attribute_lines(); });

  expander_group.reset(new expander_group_t());

  interface = new_widget_group(INTERFACE, "_Interface attributes", width);
  interface->set_position(2, 2);

  text_area = new_widget_group(TEXT_AREA, "_Text area attributes", width);
  text_area->set_anchor(interface, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  text_area->set_position(0, 0);

  syntax_highlight = new_widget_group(HIGHLIGHT, "_Syntax highlighting attributes", width);
  syntax_highlight->set_anchor(text_area,
                               T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  syntax_highlight->set_position(0, 0);

  expander_group->connect_expanded([this](bool expanded) { expander_size_change(expanded); });

  button_t *ok_button = emplace_back<button_t>("_Ok", true);
  button_t *reset_to_defaults_button = emplace_back<button_t>(_("_Reset to defaults"));
  button_t *cancel_button = emplace_back<button_t>("_Cancel");

  cancel_button->set_anchor(this,
                            T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
  cancel_button->set_position(-1, -2);
  cancel_button->connect_activate([this] { close(); });
  cancel_button->connect_move_focus_up([this] { focus_previous(); });
  cancel_button->connect_move_focus_up([this] { focus_previous(); });
  cancel_button->connect_move_focus_up([this] { focus_previous(); });
  cancel_button->connect_move_focus_left([this] { focus_previous(); });

  reset_to_defaults_button->set_anchor(cancel_button,
                                       T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  reset_to_defaults_button->set_position(0, -2);
  reset_to_defaults_button->connect_move_focus_up([this] { focus_previous(); });
  reset_to_defaults_button->connect_move_focus_up([this] { focus_previous(); });
  reset_to_defaults_button->connect_move_focus_left([this] { focus_previous(); });
  reset_to_defaults_button->connect_move_focus_right([this] { focus_next(); });
  reset_to_defaults_button->connect_activate([this] {
    reset_values();
    handle_activate();
  });

  ok_button->set_anchor(reset_to_defaults_button,
                        T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  ok_button->set_position(0, -2);
  ok_button->connect_move_focus_up([this] { focus_previous(); });
  ok_button->connect_move_focus_right([this] { focus_next(); });
  ok_button->connect_activate([this] { handle_activate(); });

  picker.reset(new attribute_picker_dialog_t());
  picker->center_over(this);
  picker->connect_attribute_selected(bind_front(&attributes_dialog_t::attribute_selected, this));
  picker->connect_default_selected([this] { default_attribute_selected(); });
}

bool attributes_dialog_t::set_size(optint height, optint width) {
  bool result = true;
  (void)height;
  if (!width.is_valid()) {
    return true;
  }

  result &= interface->set_size(None, width);
  result &= text_area->set_size(None, width);
  result &= syntax_highlight->set_size(None, width);
  return result;
}

void attributes_dialog_t::show() {
  expander_group->collapse();
  dialog_t::show();
}

void attributes_dialog_t::change_button_activated(const attribute_access_t *access) {
  t3_attr_t text_attr;

  text_attr = text.is_valid() ? text.value() : get_default_attr(TEXT, color_box->get_state());

  change_access = access;
  picker->set_base_attributes(access->text_background ? text_attr : 0);
  optional<t3_attr_t> term_options_t::*term_options_member = get_term_options_member(access->name);
  optional<t3_attr_t> default_attribute = term_options_member == nullptr
                                              ? defaults->highlights.lookup_attributes(access->name)
                                              : defaults->*term_options_member;
  picker->set_attribute((this->*access->dialog_member)
                            .value_or(default_attribute.value_or(
                                get_default_attr(access->attribute, color_box->get_state()))));

  picker->show();
}

void attributes_dialog_t::expander_size_change(bool expanded) {
  (void)expanded;
  dialog_t::set_size(4 + expander_group->get_group_height(), None);
}

void attributes_dialog_t::set_values_from_options() {
  const term_options_t *source_options =
      change_defaults ? &default_option.term_options : &term_specific_option;

  color_box->set_state(option.color);

  for (const attribute_access_t &access : attribute_access) {
    optional<t3_attr_t> term_options_t::*term_options_member = get_term_options_member(access.name);
    if (term_options_member == nullptr) {
      this->*access.dialog_member = source_options->highlights.lookup_attributes(access.name);
      if (!(this->*access.dialog_member).is_valid()) {
        this->*access.dialog_member = defaults->highlights.lookup_attributes(access.name);
      }
    } else {
      this->*access.dialog_member = source_options->*term_options_member;
      if (!(this->*access.dialog_member).is_valid()) {
        this->*access.dialog_member = defaults->*term_options_member;
      }
    }
  }

  update_attribute_lines();
}

void attributes_dialog_t::set_options_from_values() {
  if (change_defaults) {
    set_options_from_values(&default_option.term_options);
  } else {
    set_options_from_values(&term_specific_option);
  }
}

void attributes_dialog_t::set_options_from_values(term_options_t *term_options) {
  term_options->color = option.color = color_box->get_state();

  for (const attribute_access_t &access : attribute_access) {
    optional<t3_attr_t> term_options_t::*term_options_member = get_term_options_member(access.name);
    if (term_options_member == nullptr) {
      if ((this->*access.dialog_member).is_valid()) {
        term_options->highlights.insert_mapping(access.name, (this->*access.dialog_member).value());
      } else {
        term_options->highlights.erase_mapping(access.name);
      }
      option.highlights.insert_mapping(
          access.name,
          term_specific_option.highlights.lookup_attributes(access.name)
              .value_or(default_option.term_options.highlights.lookup_attributes(access.name)
                            .value_or(get_default_attr(access.attribute))));
    } else {
      /* Actual setting will be done below by copying to option.brace_highlight and calling
         set_attributes. */
      term_options->*term_options_member = this->*access.dialog_member;
    }
  }
  option.brace_highlight = term_specific_option.brace_highlight.value_or(
      default_option.term_options.brace_highlight.value_or(get_default_attr(BRACE_HIGHLIGHT)));
  set_attributes();

  force_redraw_all();
}

void attributes_dialog_t::update_attribute_lines() {
  t3_attr_t text_attr;
  bool color = color_box->get_state();

  text_attr = text.is_valid() ? text.value() : get_default_attr(TEXT, color);
  for (const attribute_access_t &access : attribute_access) {
    optional<t3_attr_t> term_options_t::*term_options_member = get_term_options_member(access.name);
    optional<t3_attr_t> default_attribute =
        term_options_member == nullptr ? defaults->highlights.lookup_attributes(access.name)
                                       : defaults->*term_options_member;
    (this->*access.dialog_line)
        ->set_attribute(t3_term_combine_attrs(
            (this->*access.dialog_member)
                .value_or(default_attribute.value_or(get_default_attr(access.attribute, color))),
            access.text_background ? text_attr : 0));
  }
}

void attributes_dialog_t::attribute_selected(t3_attr_t attribute) {
  t3_attr_t text_attr;
  text_attr = text.is_valid() ? text.value() : get_default_attr(TEXT, color_box->get_state());

  this->*change_access->dialog_member = attribute;
  (this->*change_access->dialog_line)
      ->set_attribute(
          t3_term_combine_attrs(attribute, change_access->text_background ? text_attr : 0));

  update_attribute_lines();
  picker->hide();
}

void attributes_dialog_t::default_attribute_selected() {
  t3_attr_t text_attr;
  text_attr = text.is_valid() ? text.value() : get_default_attr(TEXT, color_box->get_state());

  (this->*change_access->dialog_member).reset();
  optional<t3_attr_t> term_options_t::*term_options_member =
      get_term_options_member(change_access->name);
  optional<t3_attr_t> default_attribute =
      term_options_member == nullptr ? defaults->highlights.lookup_attributes(change_access->name)
                                     : defaults->*term_options_member;
  (this->*change_access->dialog_line)
      ->set_attribute(t3_term_combine_attrs(default_attribute.value_or(get_default_attr(
                                                change_access->attribute, color_box->get_state())),
                                            change_access->text_background ? text_attr : 0));
  update_attribute_lines();
  picker->hide();
}

void attributes_dialog_t::handle_activate() {
  /* Do required validation here. */
  hide();
  activate();
}

void attributes_dialog_t::set_change_defaults(bool value) {
  change_defaults = value;
  set_title(change_defaults ? _("Interface defaults") : _("Interface"));
  defaults = change_defaults ? &default_term_opts : &default_option.term_options;
}

void attributes_dialog_t::reset_values() {
  for (const attribute_access_t &access : attribute_access) {
    (this->*access.dialog_member).reset();
  }
  update_attribute_lines();
}
