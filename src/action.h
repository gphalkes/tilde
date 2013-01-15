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
#ifndef ACTION_H
#define ACTION_H

#include "util.h"

ENUM(action_id_t,
	ACTION_NONE,
	/* Menu actions */
	FILE_NEW,
	FILE_OPEN,
	FILE_OPEN_RECENT,
	FILE_CLOSE,
	FILE_SAVE,
	FILE_SAVE_AS,
	FILE_REPAINT,
	FILE_SUSPEND,
	FILE_EXIT,
	EDIT_COPY,
	EDIT_CUT,
	EDIT_PASTE,
	EDIT_UNDO,
	EDIT_REDO,
	EDIT_SELECT_ALL,
	EDIT_MARK,
	EDIT_INSERT_CHAR,
	EDIT_INDENT_SELECTION,
	EDIT_UNINDENT_SELECTION,
	EDIT_TOGGLE_INSERT,
	SEARCH_SEARCH,
	SEARCH_AGAIN,
	SEARCH_AGAIN_BACKWARD,
	SEARCH_REPLACE,
	SEARCH_GOTO,
	SEARCH_GOTO_MATCHING_BRACE,
	OPTIONS_INPUT,
	OPTIONS_BUFFER,
	OPTIONS_DEFAULTS,
	OPTIONS_INTERFACE,
	OPTIONS_MISC,
	WINDOWS_NEXT_BUFFER,
	WINDOWS_PREV_BUFFER,
	WINDOWS_SELECT,
	WINDOWS_HSPLIT,
	WINDOWS_VSPLIT,
	WINDOWS_MERGE,
	WINDOWS_NEXT_WINDOW,
	WINDOWS_PREV_WINDOW,
	HELP_HELP,
	HELP_ABOUT,
	TOOLS_HIGHLIGHTING,
	TOOLS_STRIP_SPACES,
	TOOLS_AUTOCOMPLETE,
);

#endif
