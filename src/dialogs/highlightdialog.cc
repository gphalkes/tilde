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
#include <algorithm>
#include <climits>
#include <cstring>
#include <new>
#include <t3highlight/highlight.h>

#include "dialogs/highlightdialog.h"
#include "main.h"
#include "util.h"

static bool cmp_lang_names(t3_highlight_lang_t a, t3_highlight_lang_t b) {
  return strcmp(a.name, b.name) < 0;
}

highlight_dialog_t::highlight_dialog_t(int height, int width)
    : dialog_t(height, width, "Highlighting Language") {
  button_t *ok_button, *cancel_button;
  t3_highlight_lang_t *ptr;
  label_t *label;
  t3_highlight_error_t error;

  list = new list_pane_t(true);
  list->set_size(height - 3, width - 2);
  list->set_position(1, 1);
  list->connect_activate(signals::mem_fun(this, &highlight_dialog_t::ok_activated));

  label = new label_t("Plain Text");
  list->push_back(label);

  if ((names = t3_highlight_list(0, &error)) == nullptr) {
    if (error.error == T3_ERR_OUT_OF_MEMORY) {
      throw std::bad_alloc();
    }
  } else {
    for (ptr = names; ptr->name != nullptr; ptr++) {
    }
    std::sort((t3_highlight_lang_t *)names, ptr, cmp_lang_names);

    for (ptr = names; ptr->name != nullptr; ptr++) {
      label = new label_t(ptr->name);
      list->push_back(label);
    }
  }

  /* Resize to list size if there are fewer list items than the size of the
     dialog allows. */
  dialog_t::set_size(height, width);

  cancel_button = new button_t("_Cancel", false);
  cancel_button->set_anchor(this,
                            T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
  cancel_button->set_position(-1, -2);
  cancel_button->connect_activate(signals::mem_fun(this, &highlight_dialog_t::close));
  ok_button = new button_t("_OK", true);
  ok_button->set_anchor(cancel_button, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  ok_button->set_position(0, -2);
  ok_button->connect_activate(signals::mem_fun(this, &highlight_dialog_t::ok_activated));

  push_back(list);
  push_back(ok_button);
  push_back(cancel_button);
}

bool highlight_dialog_t::set_size(optint height, optint width) {
  bool result;

  if (!height.is_valid()) {
    height = t3_win_get_height(window);
  }
  if (static_cast<int>(list->size()) < height - 3) {
    height = list->size() + 3;
  }

  result = dialog_t::set_size(height, width);
  if (!width.is_valid()) {
    width = t3_win_get_width(window);
  }
  result &= list->set_size(height - 3, width - 2);
  return result;
}

void highlight_dialog_t::ok_activated() {
  size_t idx = list->get_current();
  t3_highlight_t *highlight;
  t3_highlight_error_t error;

  if (idx == 0) {
    hide();
    language_selected(nullptr, nullptr);
    return;
  }

  if ((highlight = t3_highlight_load(names[idx - 1].lang_file, map_highlight, nullptr,
                                     T3_HIGHLIGHT_UTF8 | T3_HIGHLIGHT_USE_PATH, &error)) ==
      nullptr) {
    std::string message(_("Error loading highlighting patterns: "));
    message += t3_highlight_strerror(error.error);
    error_dialog->set_message(&message);
    error_dialog->center_over(this);
    error_dialog->show();
    return;
  }
  hide();
  language_selected(highlight, names[idx - 1].name);
}

void highlight_dialog_t::set_selected(const char *lang_file) {
  t3_highlight_lang_t *ptr;
  size_t idx;

  if (lang_file == nullptr) {
    list->set_current(0);
    return;
  }

  for (ptr = names, idx = 1; ptr->name != nullptr; ptr++, idx++) {
    if (strcmp(lang_file, ptr->lang_file) == 0) {
      list->set_current(idx);
      return;
    }
  }
  list->set_current(0);
}
