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
#include <climits>
#include <cstring>
#include <new>
#include <t3highlight/highlight.h>

#include "tilde/dialogs/highlightdialog.h"
#include "tilde/main.h"
#include "tilde/util.h"

static bool cmp_lang_names(t3_highlight_lang_t a, t3_highlight_lang_t b) {
  return strcmp(a.name, b.name) < 0;
}

highlight_dialog_t::highlight_dialog_t(int height, int width)
    : dialog_t(height, width, _("Highlighting Language")) {
  t3_highlight_error_t error;

  list = emplace_back<list_pane_t>(true);
  list->set_size(height - 3, width - 2);
  list->set_position(1, 1);
  list->connect_activate([this] { ok_activated(); });

  list->push_back(make_unique<label_t>("Plain Text"));

  names.reset(t3_highlight_list(0, &error));
  if (names == nullptr) {
    if (error.error == T3_ERR_OUT_OF_MEMORY) {
      throw std::bad_alloc();
    }
  } else {
    t3_highlight_lang_t *ptr;
    for (ptr = names.get(); ptr->name != nullptr; ptr++) {
    }
    std::sort(names.get(), ptr, cmp_lang_names);

    for (ptr = names.get(); ptr->name != nullptr; ptr++) {
      list->push_back(make_unique<label_t>(ptr->name));
    }
  }

  /* Resize to list size if there are fewer list items than the size of the
     dialog allows. */
  dialog_t::set_size(height, width);

  button_t *ok_button = emplace_back<button_t>("_OK", true);
  button_t *cancel_button = emplace_back<button_t>("_Cancel", false);

  cancel_button->set_anchor(this,
                            T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
  cancel_button->set_position(-1, -2);
  cancel_button->connect_activate([this] { close(); });

  ok_button->set_anchor(cancel_button, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  ok_button->set_position(0, -2);
  ok_button->connect_activate([this] { ok_activated(); });
}

bool highlight_dialog_t::set_size(optint height, optint width) {
  bool result;

  if (!height.is_valid()) {
    height = window.get_height();
  }
  if (static_cast<int>(list->size()) < height.value() - 3) {
    height = list->size() + 3;
  }

  result = dialog_t::set_size(height, width);
  if (!width.is_valid()) {
    width = window.get_width();
  }
  result &= list->set_size(height.value() - 3, width.value() - 2);
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

  if ((highlight = t3_highlight_load(names.get()[idx - 1].lang_file, map_highlight, nullptr,
                                     T3_HIGHLIGHT_UTF8 | T3_HIGHLIGHT_USE_PATH, &error)) ==
      nullptr) {
    std::string message(_("Error loading highlighting patterns: "));
    message += t3_highlight_strerror(error.error);
    error_dialog->set_message(message);
    error_dialog->center_over(this);
    error_dialog->show();
    return;
  }
  hide();
  language_selected(highlight, names.get()[idx - 1].name);
}

void highlight_dialog_t::set_selected(const char *lang_file) {
  t3_highlight_lang_t *ptr;
  size_t idx;

  if (lang_file == nullptr) {
    list->set_current(0);
    return;
  }

  for (ptr = names.get(), idx = 1; ptr->name != nullptr; ptr++, idx++) {
    if (strcmp(lang_file, ptr->lang_file) == 0) {
      list->set_current(idx);
      return;
    }
  }
  list->set_current(0);
}
