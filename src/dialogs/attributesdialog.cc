/* Copyright (C) 2012,2018 G.P. Halkes
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

#define START_WIDGET_GROUP                               \
  {                                                      \
    widget_group_t *widget_group = new widget_group_t(); \
    int widget_count = 0;
#define END_WIDGET_GROUP(name, var)                         \
  widget_group->set_size(widget_count, width - 4);          \
  var = new expander_t(name);                               \
  var->set_child(widget_group);                             \
  var->connect_move_focus_up([this] { focus_previous(); }); \
  var->connect_move_focus_down([this] { focus_next(); });   \
  expander_group->add_expander(var);                        \
  }

#define ADD_ATTRIBUTE_ENTRY(name, sym, widget_name)                                           \
  do {                                                                                        \
    std::unique_ptr<smart_label_t> attribute_label(new smart_label_t(name));                  \
    attribute_label->set_position(widget_count, 0);                                           \
    widget_group->add_child(std::move(attribute_label));                                      \
    std::unique_ptr<button_t> change_button(new button_t("Change"));                          \
    change_button->set_anchor(widget_group,                                                   \
                              T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));  \
    change_button->set_position(widget_count, 0);                                             \
    change_button->connect_activate([this] { change_button_activated(sym); });                \
    change_button->connect_move_focus_up([widget_group] { widget_group->focus_previous(); }); \
    change_button->connect_move_focus_down([widget_group] { widget_group->focus_next(); });   \
    widget_name = new attribute_test_line_t();                                                \
    widget_name->set_anchor(change_button.get(),                                              \
                            T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));     \
    widget_name->set_position(0, -2);                                                         \
    widget_group->add_child(std::move(change_button));                                        \
    widget_group->add_child(wrap_unique(widget_name));                                        \
    widget_count++;                                                                           \
  } while (false)

// FIXME: we may be better of using a list_pane_t for the longer divisions
attributes_dialog_t::attributes_dialog_t(int width) : dialog_t(7, width, _("Interface")) {
  smart_label_t *label;
  button_t *ok_button, *cancel_button, *save_defaults_button;

  label = new smart_label_t(_("Color _mode"));
  label->set_position(1, 2);
  push_back(label);
  color_box = new checkbox_t();
  color_box->set_label(label);
  color_box->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  color_box->set_position(1, -2);
  color_box->connect_move_focus_up([this] { focus_previous(); });
  color_box->connect_move_focus_down([this] { focus_next(); });
  color_box->connect_activate([this] { handle_activate(); });
  color_box->connect_toggled([this] { update_attribute_lines(); });

  expander_group.reset(new expander_group_t());

  START_WIDGET_GROUP
  ADD_ATTRIBUTE_ENTRY("Dialog", DIALOG, dialog_line);
  ADD_ATTRIBUTE_ENTRY("Dialog selected", DIALOG_SELECTED, dialog_selected_line);
  ADD_ATTRIBUTE_ENTRY("Shadow", SHADOW, shadow_line);
  ADD_ATTRIBUTE_ENTRY("Background", BACKGROUND, background_line);
  ADD_ATTRIBUTE_ENTRY("Hotkey highlight", HOTKEY_HIGHLIGHT, hotkey_highlight_line);
  ADD_ATTRIBUTE_ENTRY("Badly drawn character", BAD_DRAW, bad_draw_line);
  ADD_ATTRIBUTE_ENTRY("Unprintable character", NON_PRINT, non_print_line);
  ADD_ATTRIBUTE_ENTRY("Button selected", BUTTON_SELECTED, button_selected_line);
  ADD_ATTRIBUTE_ENTRY("Scrollbar", SCROLLBAR, scrollbar_line);
  ADD_ATTRIBUTE_ENTRY("Menu bar", MENUBAR, menubar_line);
  ADD_ATTRIBUTE_ENTRY("Menu bar selected", MENUBAR_SELECTED, menubar_selected_line);
  END_WIDGET_GROUP("_Interface attributes", interface)
  interface->set_position(2, 2);

  START_WIDGET_GROUP
  ADD_ATTRIBUTE_ENTRY("Text", TEXT, text_line);
  ADD_ATTRIBUTE_ENTRY("Selected text", TEXT_SELECTED, text_selected_line);
  ADD_ATTRIBUTE_ENTRY("Cursor", TEXT_CURSOR, text_cursor_line);
  ADD_ATTRIBUTE_ENTRY("Selection cursor at end", TEXT_SELECTION_CURSOR, text_selection_cursor_line);
  ADD_ATTRIBUTE_ENTRY("Selection cursor at start", TEXT_SELECTION_CURSOR2,
                      text_selection_cursor2_line);
  ADD_ATTRIBUTE_ENTRY("Meta text", META_TEXT, meta_text_line);
  ADD_ATTRIBUTE_ENTRY("Brace highlight", BRACE_HIGHLIGHT, brace_highlight_line);
  END_WIDGET_GROUP("_Text area attributes", text_area)
  text_area->set_anchor(interface, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  text_area->set_position(0, 0);

  START_WIDGET_GROUP
  ADD_ATTRIBUTE_ENTRY("Comment", COMMENT, comment_line);
  ADD_ATTRIBUTE_ENTRY("Comment keyword", COMMENT_KEYWORD, comment_keyword_line);
  ADD_ATTRIBUTE_ENTRY("Keyword", KEYWORD, keyword_line);
  ADD_ATTRIBUTE_ENTRY("Number", NUMBER, number_line);
  ADD_ATTRIBUTE_ENTRY("String", STRING, string_line);
  ADD_ATTRIBUTE_ENTRY("String escape", STRING_ESCAPE, string_escape_line);
  ADD_ATTRIBUTE_ENTRY("Miscelaneous", MISC, misc_line);
  ADD_ATTRIBUTE_ENTRY("Variable", VARIABLE, variable_line);
  ADD_ATTRIBUTE_ENTRY("Error", ERROR, error_line);
  ADD_ATTRIBUTE_ENTRY("Addition", ADDITION, addition_line);
  ADD_ATTRIBUTE_ENTRY("Deletion", DELETION, deletion_line);
  END_WIDGET_GROUP("_Syntax highlighting attributes", syntax_highlight)
  syntax_highlight->set_anchor(text_area,
                               T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  syntax_highlight->set_position(0, 0);

  expander_group->connect_expanded([this](bool expanded) { expander_size_change(expanded); });

  cancel_button = new button_t("_Cancel");
  cancel_button->set_anchor(this,
                            T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
  cancel_button->set_position(-1, -2);
  cancel_button->connect_activate([this] { close(); });
  cancel_button->connect_move_focus_up([this] { focus_previous(); });
  cancel_button->connect_move_focus_up([this] { focus_previous(); });
  cancel_button->connect_move_focus_up([this] { focus_previous(); });
  cancel_button->connect_move_focus_left([this] { focus_previous(); });

  save_defaults_button = new button_t("Save as _defaults");
  save_defaults_button->set_anchor(cancel_button,
                                   T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  save_defaults_button->set_position(0, -2);
  save_defaults_button->connect_move_focus_up([this] { focus_previous(); });
  save_defaults_button->connect_move_focus_up([this] { focus_previous(); });
  save_defaults_button->connect_move_focus_left([this] { focus_previous(); });
  save_defaults_button->connect_move_focus_right([this] { focus_next(); });
  save_defaults_button->connect_activate([this] { handle_save_defaults(); });

  ok_button = new button_t("_Ok", true);
  ok_button->set_anchor(save_defaults_button,
                        T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  ok_button->set_position(0, -2);
  ok_button->connect_move_focus_up([this] { focus_previous(); });
  ok_button->connect_move_focus_right([this] { focus_next(); });
  ok_button->connect_activate([this] { handle_activate(); });

  push_back(color_box);
  push_back(interface);
  push_back(text_area);
  push_back(syntax_highlight);
  push_back(ok_button);
  push_back(save_defaults_button);
  push_back(cancel_button);

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

void attributes_dialog_t::change_button_activated(attribute_key_t attribute) {
  t3_attr_t text_attr;

  text_attr = text.is_valid() ? text.value() : get_default_attr(TEXT, color_box->get_state());
  change_attribute = attribute;

  switch (attribute) {
#define SET_WITH_DEFAULT(name, attr)                                                         \
  case attr:                                                                                 \
    picker->set_base_attributes(0);                                                          \
    picker->set_attribute(name.is_valid() ? name.value()                                     \
                                          : get_default_attr(attr, color_box->get_state())); \
    break;
    SET_WITH_DEFAULT(dialog, DIALOG);
    SET_WITH_DEFAULT(dialog_selected, DIALOG_SELECTED);
    SET_WITH_DEFAULT(shadow, SHADOW);
    SET_WITH_DEFAULT(background, BACKGROUND);
    SET_WITH_DEFAULT(hotkey_highlight, HOTKEY_HIGHLIGHT);
    SET_WITH_DEFAULT(bad_draw, BAD_DRAW);
    SET_WITH_DEFAULT(non_print, NON_PRINT);
    SET_WITH_DEFAULT(button_selected, BUTTON_SELECTED);
    SET_WITH_DEFAULT(scrollbar, SCROLLBAR);
    SET_WITH_DEFAULT(menubar, MENUBAR);
    SET_WITH_DEFAULT(menubar_selected, MENUBAR_SELECTED);

    SET_WITH_DEFAULT(text, TEXT);
    SET_WITH_DEFAULT(text_selected, TEXT_SELECTED);
    SET_WITH_DEFAULT(text_cursor, TEXT_CURSOR);
    SET_WITH_DEFAULT(text_selection_cursor, TEXT_SELECTION_CURSOR);
    SET_WITH_DEFAULT(text_selection_cursor2, TEXT_SELECTION_CURSOR2);
#undef SET_WITH_DEFAULT

#define SET_WITH_DEFAULT(name, attr)                                                         \
  case attr:                                                                                 \
    picker->set_base_attributes(text_attr);                                                  \
    picker->set_attribute(name.is_valid() ? name.value()                                     \
                                          : get_default_attr(attr, color_box->get_state())); \
    break;
    SET_WITH_DEFAULT(meta_text, META_TEXT);
    SET_WITH_DEFAULT(brace_highlight, BRACE_HIGHLIGHT);

    SET_WITH_DEFAULT(comment, COMMENT);
    SET_WITH_DEFAULT(comment_keyword, COMMENT_KEYWORD);
    SET_WITH_DEFAULT(keyword, KEYWORD);
    SET_WITH_DEFAULT(number, NUMBER);
    SET_WITH_DEFAULT(string, STRING);
    SET_WITH_DEFAULT(string_escape, STRING_ESCAPE);
    SET_WITH_DEFAULT(misc, MISC);
    SET_WITH_DEFAULT(variable, VARIABLE);
    SET_WITH_DEFAULT(error, ERROR);
    SET_WITH_DEFAULT(addition, ADDITION);
    SET_WITH_DEFAULT(deletion, DELETION);
#undef SET_WITH_DEFAULT
    default:
// This means we somehow got a bad attribute key, which is a logic error.
// However, we don't want to crash on this (at least outside of debug mode).
#ifdef DEBUG
      PANIC();
#endif
      break;
  }
  picker->show();
}

void attributes_dialog_t::expander_size_change(bool expanded) {
  (void)expanded;
  dialog_t::set_size(4 + expander_group->get_group_height(), None);
}

void attributes_dialog_t::set_values_from_options() {
  color_box->set_state(option.color);

#define SET_OPTION_VALUE(name)                                     \
  do {                                                             \
    name = term_specific_option.name;                              \
    if (!name.is_valid()) name = default_option.term_options.name; \
  } while (false)

  SET_OPTION_VALUE(dialog);
  SET_OPTION_VALUE(dialog_selected);
  SET_OPTION_VALUE(shadow);
  SET_OPTION_VALUE(background);
  SET_OPTION_VALUE(hotkey_highlight);
  SET_OPTION_VALUE(bad_draw);
  SET_OPTION_VALUE(non_print);
  SET_OPTION_VALUE(button_selected);
  SET_OPTION_VALUE(scrollbar);
  SET_OPTION_VALUE(menubar);
  SET_OPTION_VALUE(menubar_selected);

  SET_OPTION_VALUE(text);
  SET_OPTION_VALUE(text_selected);
  SET_OPTION_VALUE(text_cursor);
  SET_OPTION_VALUE(text_selection_cursor);
  SET_OPTION_VALUE(text_selection_cursor2);
  SET_OPTION_VALUE(meta_text);
  SET_OPTION_VALUE(brace_highlight);
#undef SET_OPTION_VALUE

#define SET_HIGHLIGHT_OPTION_VALUE(name, highlight_name)                                     \
  do {                                                                                       \
    name = term_specific_option.highlights[map_highlight(nullptr, highlight_name)];          \
    if (!name.is_valid())                                                                    \
      name = default_option.term_options.highlights[map_highlight(nullptr, highlight_name)]; \
  } while (false)
  SET_HIGHLIGHT_OPTION_VALUE(comment, "comment");
  SET_HIGHLIGHT_OPTION_VALUE(comment_keyword, "comment-keyword");
  SET_HIGHLIGHT_OPTION_VALUE(keyword, "keyword");
  SET_HIGHLIGHT_OPTION_VALUE(number, "number");
  SET_HIGHLIGHT_OPTION_VALUE(string, "string");
  SET_HIGHLIGHT_OPTION_VALUE(string_escape, "string-escape");
  SET_HIGHLIGHT_OPTION_VALUE(misc, "misc");
  SET_HIGHLIGHT_OPTION_VALUE(variable, "variable");
  SET_HIGHLIGHT_OPTION_VALUE(error, "error");
  SET_HIGHLIGHT_OPTION_VALUE(addition, "addition");
  SET_HIGHLIGHT_OPTION_VALUE(deletion, "deletion");
#undef SET_HIGHLIGHT_OPTION_VALUE
  update_attribute_lines();
}

void attributes_dialog_t::set_term_options_from_values() {
  set_options_from_values(&term_specific_option);
}

void attributes_dialog_t::set_default_options_from_values() {
  set_options_from_values(&default_option.term_options);

  // Make this terminal obey the defaults.
  term_specific_option.color = nullopt;
  term_specific_option.non_print = nullopt;
  term_specific_option.text_selection_cursor = nullopt;
  term_specific_option.text_selection_cursor2 = nullopt;
  term_specific_option.bad_draw = nullopt;
  term_specific_option.text_cursor = nullopt;
  term_specific_option.text = nullopt;
  term_specific_option.text_selected = nullopt;
  term_specific_option.hotkey_highlight = nullopt;

  term_specific_option.dialog = nullopt;
  term_specific_option.dialog_selected = nullopt;
  term_specific_option.button_selected = nullopt;
  term_specific_option.scrollbar = nullopt;
  term_specific_option.menubar = nullopt;
  term_specific_option.menubar_selected = nullopt;

  term_specific_option.shadow = nullopt;
  term_specific_option.meta_text = nullopt;
  term_specific_option.background = nullopt;

  for (optional<t3_attr_t> &highlight : term_specific_option.highlights) {
    highlight = nullopt;
  }
  term_specific_option.brace_highlight = nullopt;
}

void attributes_dialog_t::set_options_from_values(term_options_t *term_options) {
  term_options->color = option.color = color_box->get_state();

#define SET_WITH_DEFAULT(name, attr)                                                              \
  do {                                                                                            \
    term_options->name = name;                                                                    \
    set_attribute(                                                                                \
        attribute_t::attr,                                                                        \
        name.is_valid() ? name.value() : get_default_attribute(attribute_t::attr, option.color)); \
  } while (false)
  SET_WITH_DEFAULT(dialog, DIALOG);
  SET_WITH_DEFAULT(dialog_selected, DIALOG_SELECTED);
  SET_WITH_DEFAULT(shadow, SHADOW);
  SET_WITH_DEFAULT(background, BACKGROUND);
  SET_WITH_DEFAULT(hotkey_highlight, HOTKEY_HIGHLIGHT);
  SET_WITH_DEFAULT(bad_draw, BAD_DRAW);
  SET_WITH_DEFAULT(non_print, NON_PRINT);
  SET_WITH_DEFAULT(button_selected, BUTTON_SELECTED);
  SET_WITH_DEFAULT(scrollbar, SCROLLBAR);
  SET_WITH_DEFAULT(menubar, MENUBAR);
  SET_WITH_DEFAULT(menubar_selected, MENUBAR_SELECTED);
  SET_WITH_DEFAULT(text, TEXT);
  SET_WITH_DEFAULT(text_selected, TEXT_SELECTED);
  SET_WITH_DEFAULT(text_cursor, TEXT_CURSOR);
  SET_WITH_DEFAULT(text_selection_cursor, TEXT_SELECTION_CURSOR);
  SET_WITH_DEFAULT(text_selection_cursor2, TEXT_SELECTION_CURSOR2);
  SET_WITH_DEFAULT(meta_text, META_TEXT);
#undef SET_WITH_DEFAULT

  term_options->brace_highlight = brace_highlight;
  option.brace_highlight =
      brace_highlight.is_valid() ? brace_highlight.value() : get_default_attr(BRACE_HIGHLIGHT);

#define SET_WITH_DEFAULT(name, attr)                                                            \
  do {                                                                                          \
    int highlight_idx = map_highlight(nullptr, #name);                                          \
    term_options->highlights[highlight_idx] = name;                                             \
    option.highlights[highlight_idx] = name.is_valid() ? name.value() : get_default_attr(attr); \
  } while (false)
  SET_WITH_DEFAULT(comment, COMMENT);
  {
    int highlight_idx = map_highlight(nullptr, "comment-keyword");
    term_options->highlights[highlight_idx] = comment_keyword;
    option.highlights[highlight_idx] =
        comment_keyword.is_valid() ? comment_keyword.value() : get_default_attr(COMMENT_KEYWORD);
  }
  SET_WITH_DEFAULT(keyword, KEYWORD);
  SET_WITH_DEFAULT(number, NUMBER);
  SET_WITH_DEFAULT(string, STRING);
  {
    int highlight_idx = map_highlight(nullptr, "string-escape");
    term_options->highlights[highlight_idx] = string_escape;
    option.highlights[highlight_idx] =
        string_escape.is_valid() ? string_escape.value() : get_default_attr(STRING_ESCAPE);
  }
  SET_WITH_DEFAULT(misc, MISC);
  SET_WITH_DEFAULT(variable, VARIABLE);
  SET_WITH_DEFAULT(error, ERROR);
  SET_WITH_DEFAULT(addition, ADDITION);
  SET_WITH_DEFAULT(deletion, DELETION);
#undef SET_WITH_DEFAULT
  option.highlights[0] = 0;
  force_redraw_all();
}

void attributes_dialog_t::update_attribute_lines() {
  t3_attr_t text_attr;
  bool color = color_box->get_state();

#define SET_WITH_DEFAULT(name, attr) \
  name##_line->set_attribute(name.is_valid() ? name.value() : get_default_attr(attr, color))
  SET_WITH_DEFAULT(dialog, DIALOG);
  SET_WITH_DEFAULT(dialog_selected, DIALOG_SELECTED);
  SET_WITH_DEFAULT(shadow, SHADOW);
  SET_WITH_DEFAULT(background, BACKGROUND);
  SET_WITH_DEFAULT(hotkey_highlight, HOTKEY_HIGHLIGHT);
  SET_WITH_DEFAULT(bad_draw, BAD_DRAW);
  SET_WITH_DEFAULT(non_print, NON_PRINT);
  SET_WITH_DEFAULT(button_selected, BUTTON_SELECTED);
  SET_WITH_DEFAULT(scrollbar, SCROLLBAR);
  SET_WITH_DEFAULT(menubar, MENUBAR);
  SET_WITH_DEFAULT(menubar_selected, MENUBAR_SELECTED);

  SET_WITH_DEFAULT(text, TEXT);
  SET_WITH_DEFAULT(text_selected, TEXT_SELECTED);
  SET_WITH_DEFAULT(text_cursor, TEXT_CURSOR);
  SET_WITH_DEFAULT(text_selection_cursor, TEXT_SELECTION_CURSOR);
  SET_WITH_DEFAULT(text_selection_cursor2, TEXT_SELECTION_CURSOR2);
#undef SET_WITH_DEFAULT

  text_attr = text.is_valid() ? text.value() : get_default_attr(TEXT, color);

#define SET_WITH_DEFAULT(name, attr)                \
  name##_line->set_attribute(t3_term_combine_attrs( \
      name.is_valid() ? name.value() : get_default_attr(attr, color), text_attr))
  SET_WITH_DEFAULT(meta_text, META_TEXT);
  SET_WITH_DEFAULT(brace_highlight, BRACE_HIGHLIGHT);

  SET_WITH_DEFAULT(comment, COMMENT);
  SET_WITH_DEFAULT(comment_keyword, COMMENT_KEYWORD);
  SET_WITH_DEFAULT(keyword, KEYWORD);
  SET_WITH_DEFAULT(number, NUMBER);
  SET_WITH_DEFAULT(string, STRING);
  SET_WITH_DEFAULT(string_escape, STRING_ESCAPE);
  SET_WITH_DEFAULT(misc, MISC);
  SET_WITH_DEFAULT(variable, VARIABLE);
  SET_WITH_DEFAULT(error, ERROR);
  SET_WITH_DEFAULT(addition, ADDITION);
  SET_WITH_DEFAULT(deletion, DELETION);
#undef SET_WITH_DEFAULT
}

void attributes_dialog_t::attribute_selected(t3_attr_t attribute) {
  t3_attr_t text_attr;
  text_attr = text.is_valid() ? text.value() : get_default_attr(TEXT, color_box->get_state());

  switch (change_attribute) {
#define SET_WITH_DEFAULT(name, attr)       \
  case attr:                               \
    name = attribute;                      \
    name##_line->set_attribute(attribute); \
    break;
    SET_WITH_DEFAULT(dialog, DIALOG);
    SET_WITH_DEFAULT(dialog_selected, DIALOG_SELECTED);
    SET_WITH_DEFAULT(shadow, SHADOW);
    SET_WITH_DEFAULT(background, BACKGROUND);
    SET_WITH_DEFAULT(hotkey_highlight, HOTKEY_HIGHLIGHT);
    SET_WITH_DEFAULT(bad_draw, BAD_DRAW);
    SET_WITH_DEFAULT(non_print, NON_PRINT);
    SET_WITH_DEFAULT(button_selected, BUTTON_SELECTED);
    SET_WITH_DEFAULT(scrollbar, SCROLLBAR);
    SET_WITH_DEFAULT(menubar, MENUBAR);
    SET_WITH_DEFAULT(menubar_selected, MENUBAR_SELECTED);

    SET_WITH_DEFAULT(text, TEXT);
    SET_WITH_DEFAULT(text_selected, TEXT_SELECTED);
    SET_WITH_DEFAULT(text_cursor, TEXT_CURSOR);
    SET_WITH_DEFAULT(text_selection_cursor, TEXT_SELECTION_CURSOR);
    SET_WITH_DEFAULT(text_selection_cursor2, TEXT_SELECTION_CURSOR2);

#undef SET_WITH_DEFAULT

#define SET_WITH_DEFAULT(name, attr)                                         \
  case attr:                                                                 \
    name = attribute;                                                        \
    name##_line->set_attribute(t3_term_combine_attrs(attribute, text_attr)); \
    break;
    SET_WITH_DEFAULT(meta_text, META_TEXT);
    SET_WITH_DEFAULT(brace_highlight, BRACE_HIGHLIGHT);

    SET_WITH_DEFAULT(comment, COMMENT);
    SET_WITH_DEFAULT(comment_keyword, COMMENT_KEYWORD);
    SET_WITH_DEFAULT(keyword, KEYWORD);
    SET_WITH_DEFAULT(number, NUMBER);
    SET_WITH_DEFAULT(string, STRING);
    SET_WITH_DEFAULT(string_escape, STRING_ESCAPE);
    SET_WITH_DEFAULT(misc, MISC);
    SET_WITH_DEFAULT(variable, VARIABLE);
    SET_WITH_DEFAULT(error, ERROR);
    SET_WITH_DEFAULT(addition, ADDITION);
    SET_WITH_DEFAULT(deletion, DELETION);
#undef SET_WITH_DEFAULT
    default:
// This means we somehow got a bad attribute key, which is a logic error.
// However, we don't want to crash on this (at least outside of debug mode).
#ifdef DEBUG
      PANIC();
#endif
      break;
  }
  picker->hide();
}

void attributes_dialog_t::default_attribute_selected() {
  t3_attr_t text_attr;
  text_attr = text.is_valid() ? text.value() : get_default_attr(TEXT, color_box->get_state());

  switch (change_attribute) {
#define SET_DEFAULT(name, attr)                                                                   \
  case attr:                                                                                      \
    name = default_option.term_options.name;                                                      \
    name##_line->set_attribute(name.is_valid() ? name.value()                                     \
                                               : get_default_attr(attr, color_box->get_state())); \
    break;
    SET_DEFAULT(dialog, DIALOG);
    SET_DEFAULT(dialog_selected, DIALOG_SELECTED);
    SET_DEFAULT(shadow, SHADOW);
    SET_DEFAULT(background, BACKGROUND);
    SET_DEFAULT(hotkey_highlight, HOTKEY_HIGHLIGHT);
    SET_DEFAULT(bad_draw, BAD_DRAW);
    SET_DEFAULT(non_print, NON_PRINT);
    SET_DEFAULT(button_selected, BUTTON_SELECTED);
    SET_DEFAULT(scrollbar, SCROLLBAR);
    SET_DEFAULT(menubar, MENUBAR);
    SET_DEFAULT(menubar_selected, MENUBAR_SELECTED);

    SET_DEFAULT(text, TEXT);
    SET_DEFAULT(text_selected, TEXT_SELECTED);
    SET_DEFAULT(text_cursor, TEXT_CURSOR);
    SET_DEFAULT(text_selection_cursor, TEXT_SELECTION_CURSOR);
    SET_DEFAULT(text_selection_cursor2, TEXT_SELECTION_CURSOR2);

#undef SET_DEFAULT

#define SET_DEFAULT(name, attr)                                                            \
  case attr:                                                                               \
    name.reset();                                                                          \
    name##_line->set_attribute(                                                            \
        t3_term_combine_attrs(get_default_attr(attr, color_box->get_state()), text_attr)); \
    break;
    SET_DEFAULT(meta_text, META_TEXT);
    SET_DEFAULT(brace_highlight, BRACE_HIGHLIGHT);

    SET_DEFAULT(comment, COMMENT);
    SET_DEFAULT(comment_keyword, COMMENT_KEYWORD);
    SET_DEFAULT(keyword, KEYWORD);
    SET_DEFAULT(number, NUMBER);
    SET_DEFAULT(string, STRING);
    SET_DEFAULT(string_escape, STRING_ESCAPE);
    SET_DEFAULT(misc, MISC);
    SET_DEFAULT(variable, VARIABLE);
    SET_DEFAULT(error, ERROR);
    SET_DEFAULT(addition, ADDITION);
    SET_DEFAULT(deletion, DELETION);
#undef SET_DEFAULT
    default:
// This means we somehow got a bad attribute key, which is a logic error.
// However, we don't want to crash on this (at least outside of debug mode).
#ifdef DEBUG
      PANIC();
#endif
      break;
  }
  picker->hide();
}

void attributes_dialog_t::handle_activate() {
  /* Do required validation here. */
  hide();
  activate();
}

void attributes_dialog_t::handle_save_defaults() {
  /* Do required validation here. */
  hide();
  save_defaults();
}
