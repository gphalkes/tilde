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
#ifndef MAIN_H
#define MAIN_H

#include <t3widget/widget.h>
using namespace t3_widget;

#include "tilde/dialogs/characterdetailsdialog.h"
#include "tilde/dialogs/encodingdialog.h"
#include "tilde/dialogs/openrecentdialog.h"

extern message_dialog_t *continue_abort_dialog;
extern open_file_dialog_t *open_file_dialog;
extern save_as_dialog_t *save_as_dialog;
extern message_dialog_t *close_confirm_dialog;
extern message_dialog_t *error_dialog;
extern open_recent_dialog_t *open_recent_dialog;
extern encoding_dialog_t *encoding_dialog;
extern message_dialog_t *preserve_bom_dialog;
extern character_details_dialog_t *character_details_dialog;

#endif
