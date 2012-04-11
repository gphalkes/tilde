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
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

#include "filebuffer.h"
#include "fileline.h"
#include "openfiles.h"
#include "filestate.h"
#include "log.h"
#include "option.h"

file_buffer_t::file_buffer_t(const char *_name, const char *_encoding) : text_buffer_t(new file_line_factory_t(this)),
		view_parameters(new edit_window_t::view_parameters_t()), has_window(false),
		highlight_valid(0), highlight_info(NULL), match_line(NULL), last_match(NULL)
{
	if (_encoding == NULL)
		encoding = strdup("UTF-8");
	else
		encoding = strdup(_encoding);
	if (encoding == NULL)
		throw bad_alloc();

	if (_name == NULL) {
		name_line.set_text("(Untitled)");
	} else {
		if ((name = strdup(_name)) == NULL)
			throw bad_alloc();

		string converted_name;
		convert_lang_codeset(name, &converted_name, true);
		name_line.set_text(&converted_name);
	}

	connect_rewrap_required(sigc::mem_fun(this, &file_buffer_t::invalidate_highlight));

	view_parameters->set_tabsize(option.tabsize);
	view_parameters->set_wrap(option.wrap ? wrap_type_t::WORD : wrap_type_t::NONE);
	view_parameters->set_auto_indent(option.auto_indent);
	view_parameters->set_tab_spaces(option.tab_spaces);
	view_parameters->set_indent_aware_home(option.indent_aware_home);
	view_parameters->set_show_tabs(option.show_tabs);
	open_files.push_back(this);
}

file_buffer_t::~file_buffer_t(void) {
	open_files.erase(this);
	t3_highlight_free(highlight_info);
	t3_highlight_free_match(last_match);
	delete get_line_factory();
}

rw_result_t file_buffer_t::load(load_process_t *state) {
	const string *line;
	t3_highlight_t *highlight = NULL;
	int i;

	if (state->file != this)
		PANIC();

	switch (state->state) {
		case load_process_t::INITIAL: {
			char *_name;
			transcript_t *handle;
			transcript_error_t error;

			//FIXME: this is not correct for symlinks!!!
			if ((_name = canonicalize_path(name)) == NULL)
				return rw_result_t(rw_result_t::ERRNO_ERROR, errno);

			/* name is a cleanup ptr. */
			name = _name;

			if ((state->fd = open(name, O_RDONLY)) < 0)
				return rw_result_t(rw_result_t::ERRNO_ERROR, errno);

			try {
				string converted_name;
				lprintf("Using encoding %s to read %s\n", encoding(), name());

				handle = transcript_open_converter(encoding, TRANSCRIPT_UTF8, 0, &error);
				if (handle == NULL)
					return rw_result_t(rw_result_t::CONVERSION_OPEN_ERROR, error);
				//FIXME: if the new fails, the handle will remain open!
				state->wrapper = new file_read_wrapper_t(state->fd, handle);

				convert_lang_codeset(name, &converted_name, true);
				name_line.set_text(&converted_name);
			} catch (bad_alloc &ba) {
				return rw_result_t(rw_result_t::ERRNO_ERROR, ENOMEM);
			}
			state->state = load_process_t::READING;
		}
		case load_process_t::READING:
			try {
				while (state->wrapper->fill_buffer(state->wrapper->get_fill())) {
					try {
						append_text(state->wrapper->get_buffer(), state->wrapper->get_fill());
					} catch (...) {
						return rw_result_t(rw_result_t::ERRNO_ERROR, ENOMEM);
					}
				}
				cursor.pos = 0;
				cursor.line = 0;
			} catch (rw_result_t &result) {
				return result;
			}
			break;
		default:
			PANIC();
	}

	/* Automatically load appropriate highlighting patterns if available.
	   Try the following in order:
	   - a vi(m) modeline/Emacs major mode spec in the first five lines
	   - a vi(m) modeline/Emacs major mode spec in the last five lines
	   - a first-line-regex
	   - the file name
	   We use this order, because the file name may be deceptive. For example,
	   a .txt extension may have been added by a web browser. If the user
	   specified the language by hand, that is more likely to be correct than
	   any autodetection based on the first line or the file name.
	*/
	for (i = 0; i < size() && i < 5 && highlight == NULL; i++) {
		line = get_line_data(i)->get_data();
		highlight = t3_highlight_load_by_detect(line->data(), line->size(), false,
			map_highlight, NULL, T3_HIGHLIGHT_UTF8, NULL);
	}
	for (i = size() - 1; i >= 5 && highlight == NULL; i--) {
		line = get_line_data(i)->get_data();
		highlight = t3_highlight_load_by_detect(line->data(), line->size(), false,
			map_highlight, NULL, T3_HIGHLIGHT_UTF8, NULL);
	}
	if (highlight == NULL) {
		line = get_line_data(0)->get_data();
		highlight = t3_highlight_load_by_detect(line->data(), line->size(), true,
			map_highlight, NULL, T3_HIGHLIGHT_UTF8, NULL);
	}
	if (highlight == NULL) {
		highlight = t3_highlight_load_by_filename(name, map_highlight, NULL, T3_HIGHLIGHT_UTF8, NULL);
		last_match = t3_highlight_new_match(highlight);
	}
	if (highlight != NULL)
		set_highlight(highlight);
	return rw_result_t(rw_result_t::SUCCESS);
}

/* FIXME: try to prevent as many race conditions as possible here. */
rw_result_t file_buffer_t::save(save_as_process_t *state) {
	size_t idx;
	const char *save_name;

	if (state->file != this)
		PANIC();

	switch (state->state) {
		case save_as_process_t::INITIAL:
			if (state->name.empty()) {
				if (name == NULL)
					PANIC();
				save_name = name;
			} else {
				save_name = state->name.c_str();
			}

			if ((state->real_name = resolve_links(save_name)) == NULL)
				return rw_result_t(rw_result_t::ERRNO_ERROR, ENAMETOOLONG);

			/* FIXME: to avoid race conditions, it is probably better to try open with O_CREAT|O_EXCL
			   However, this may cause issues with NFS, which is known to have isues with this. */
			if (stat(state->real_name, &state->file_info) < 0) {
				if (errno == ENOENT) {
					if ((state->fd = creat(state->real_name, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) < 0)
						return rw_result_t(rw_result_t::ERRNO_ERROR, errno);
				} else {
					return rw_result_t(rw_result_t::ERRNO_ERROR, errno);
				}
			} else {
				state->state = save_as_process_t::ALLOW_OVERWRITE;
				if (!state->name.empty())
					return rw_result_t(rw_result_t::FILE_EXISTS);
				/* Note that we have a new case in the middle of the else statement
				   here. It is an ugly hack, but it does prevent a lot of code
				   duplication and other hacks. */
		case save_as_process_t::ALLOW_OVERWRITE:
				string temp_name_str = state->real_name;

				if ((idx = temp_name_str.rfind('/')) == string::npos)
					idx = 0;
				else
					idx++;

				temp_name_str.erase(idx);
				try {
					temp_name_str.append(".tildeXXXXXX");
				} catch (bad_alloc &ba) {
					return rw_result_t(rw_result_t::ERRNO_ERROR, ENOMEM);
				}

				/* Unfortunately, we can't pass the c_str result to mkstemp as we are not allowed
				   to change that string. So we'll just have to strdup it :-( */
				if ((state->temp_name = strdup(temp_name_str.c_str())) == NULL)
					return rw_result_t(rw_result_t::ERRNO_ERROR, errno);

				if ((state->fd = mkstemp(state->temp_name)) < 0)
					return rw_result_t(rw_result_t::ERRNO_ERROR, errno);

				// Preserve ownership and attributes
				fchmod(state->fd, state->file_info.st_mode);
				fchown(state->fd, -1, state->file_info.st_gid);
				fchown(state->fd, state->file_info.st_uid, -1);
			}

		{
			transcript_t *handle;
			transcript_error_t error;
			//FIXME: bail out on error
			if (!state->encoding.empty()) {
				handle = transcript_open_converter(state->encoding.c_str(), TRANSCRIPT_UTF8, 0, &error);
				/* encoding is a cleanup ptr. */
				encoding = strdup(state->encoding.c_str());
			} else if (strcmp(encoding, "UTF-8") != 0) {
				handle = transcript_open_converter(encoding, TRANSCRIPT_UTF8, 0, &error);
			} else {
				handle = NULL;
			}
			//FIXME: if the new fails, the handle will remain open!
			//FIXME: if the new fails, this will abort with an exception! This should not happen!
			state->wrapper = new file_write_wrapper_t(state->fd, handle);
			state->i = 0;
			state->state = save_as_process_t::WRITING;
		}
		case save_as_process_t::WRITING:
			if (strip_spaces.is_valid() ? (bool) strip_spaces : option.strip_spaces)
				do_strip_spaces();
			try {
				for (; state->i < size(); state->i++) {
					const string *data;
					if (state->i != 0)
						state->wrapper->write("\n", 1);
					data = get_line_data(state->i)->get_data();
					state->wrapper->write(data->data(), data->size());
				}
			} catch (rw_result_t error) {
				return error;
			}

			fsync(state->fd);
			close(state->fd);
			state->fd = -1;
			if (state->temp_name != NULL) {
				if (option.make_backup) {
					string backup_name = state->real_name;
					backup_name += '~';
					unlink(backup_name.c_str());
					link(state->real_name, backup_name.c_str());
				}
				if (rename(state->temp_name, state->real_name) < 0)
					return rw_result_t(rw_result_t::ERRNO_ERROR, errno);
			}

			if (!state->name.empty()) {
				string converted_name;
				/* name is a cleanup ptr. */
				name = strdup(state->name.c_str());
				convert_lang_codeset(name, &converted_name, true);
				name_line.set_text(&converted_name);
			}
			set_undo_mark();
			break;
		default:
			PANIC();
	}
	return rw_result_t(rw_result_t::SUCCESS);
}

const char *file_buffer_t::get_name(void) const {
	return name();
}

const char *file_buffer_t::get_encoding(void) const {
	return encoding();
}

text_line_t *file_buffer_t::get_name_line(void) {
	return &name_line;
}

const edit_window_t::view_parameters_t *file_buffer_t::get_view_parameters(void) const {
	return view_parameters();
}

void file_buffer_t::prepare_paint_line(int line) {
	int i;

	if (highlight_info == NULL || highlight_valid > line)
		return;

	for (i = highlight_valid + 1; i <= line; i++) {
		int state = ((file_line_t *) get_line_data_nonconst(i - 1))->get_highlight_end();
		((file_line_t *) get_line_data_nonconst(i))->set_highlight_start(state);
	}
	highlight_valid = line;
	match_line = NULL;
}

void file_buffer_t::set_has_window(bool _has_window) {
	has_window = _has_window;
}

bool file_buffer_t::get_has_window(void) const {
	return has_window;
}

void file_buffer_t::invalidate_highlight(rewrap_type_t type, int line, int pos) {
	(void) type;
	(void) pos;
	if (line < highlight_valid)
		highlight_valid = line;
}

t3_highlight_t *file_buffer_t::get_highlight(void) {
	return highlight_info;
}

void file_buffer_t::set_highlight(t3_highlight_t *highlight) {
	if (highlight_info != NULL)
		t3_highlight_free(highlight_info);
	highlight_info = highlight;

	if (last_match != NULL) {
		t3_highlight_free_match(last_match);
		last_match = NULL;
	}

	match_line = NULL;
	highlight_valid = 0;

	if (highlight_info != NULL)
		last_match = t3_highlight_new_match(highlight_info);
}

bool file_buffer_t::get_strip_spaces(void) const {
	if (strip_spaces.is_valid())
		return strip_spaces;
	return option.strip_spaces;
}

void file_buffer_t::set_strip_spaces(bool _strip_spaces) {
	strip_spaces = _strip_spaces;
}

void file_buffer_t::do_strip_spaces(void) {
	size_t idx, strip_start;
	bool undo_started = false;
	text_coordinate_t saved_cursor = cursor;
	int i;

	/*FIXME: a better way to do this would be to store the stripped spaces for
	   all lines in a single string, delimited by single non-space bytes. If the
	   length of the string equals the number of lines, then just don't store
	   it. Care has to be taken to maintain the correct cursor position, and
	   we have to implement undo/redo separately. */

	for (i = 0; i < size() ; i++) {
		const text_line_t *line = get_line_data(i);
		const string *str = line->get_data();
		const char *data = str->data();
		strip_start = str->size();
		for (idx = strip_start; idx > 0; idx--) {
			if ((data[idx - 1] & 0xC0) == 0x80)
				continue;

			if (!line->is_space(idx - 1))
				break;

			strip_start = idx - 1;
		}

		if (strip_start != str->size()) {
			text_coordinate_t start, end;
			if (!undo_started) {
				start_undo_block();
				undo_started = true;
			}
			start.line = end.line = i;
			start.pos = strip_start;
			end.pos = str->size();
			delete_block_internal(start, end, get_undo(UNDO_DELETE_BLOCK, start, end));

			if (cursor.line == i && (size_t) cursor.pos > strip_start)
				cursor.pos = strip_start;
		}
	}

	if (undo_started)
		end_undo_block();

	cursor = saved_cursor;
	if (cursor.pos > get_line_max(cursor.line))
		cursor.pos = get_line_max(cursor.line);
}
