/* Copyright (C) 2011,2018 G.P. Halkes
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
#include <climits>

#include "tilde/dialogs/openrecentdialog.h"
#include "tilde/openfiles.h"

open_recent_dialog_t::open_recent_dialog_t(int height, int width)
    : dialog_t(height, width, "Open Recent"), known_version(INT_MIN) {
  button_t *ok_button, *cancel_button;

  list = new list_pane_t(true);
  list->set_size(height - 3, width - 2);
  list->set_position(1, 1);
  list->connect_activate(signals::mem_fun(this, &open_recent_dialog_t::ok_activated));

  cancel_button = new button_t("_Cancel", false);
  cancel_button->set_anchor(this,
                            T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
  cancel_button->set_position(-1, -2);
  cancel_button->connect_activate(signals::mem_fun(this, &open_recent_dialog_t::close));
  ok_button = new button_t("_OK", true);
  ok_button->set_anchor(cancel_button, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  ok_button->set_position(0, -2);
  ok_button->connect_activate(signals::mem_fun(this, &open_recent_dialog_t::ok_activated));

  push_back(list);
  push_back(ok_button);
  push_back(cancel_button);
}

bool open_recent_dialog_t::set_size(optint height, optint width) {
  bool result = dialog_t::set_size(height, width);
  result &= list->set_size(height - 3, width - 2);
  return result;
}

void open_recent_dialog_t::show() {
  if (recent_files.get_version() != known_version) {
    known_version = recent_files.get_version();

    while (!list->empty()) {
      widget_t *widget = list->back();
      list->pop_back();
      delete widget;
    }

    for (recent_file_info_t *recent_file : recent_files) {
      label_t *label = new label_t(recent_file->get_name().c_str());
      label->set_align(label_t::ALIGN_LEFT_UNDERFLOW);
      list->push_back(label);
    }
  }
  list->reset();
  dialog_t::show();
}

void open_recent_dialog_t::ok_activated() {
  hide();
  if (list->size() > 0) {
    file_selected(recent_files.get_info(list->get_current()));
  }
}
