/* Copyright (C) 2011 G.P. Halkes
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
#include "openfiles.h"
#include "filestate.h"
#include "log.h"
#include "option.h"

file_buffer_t::file_buffer_t(const char *_name, const char *_encoding) : text_buffer_t(_name),
	view_parameters(option.tabsize, option.wrap ? wrap_type_t::WORD : wrap_type_t::NONE, option.auto_indent), has_window(false)
{
	if (_encoding == NULL)
		encoding = strdup("UTF-8");
	else
		encoding = strdup(_encoding);
	if (encoding == NULL)
		throw bad_alloc();

	if (name == NULL) {
		name_line.set_text("(Untitled)");
	} else {
		string converted_name;
		convert_lang_codeset(name, &converted_name, true);
		name_line.set_text(&converted_name);
	}
	open_files.push_back(this);
}

file_buffer_t::~file_buffer_t(void) {
	free(encoding);
	open_files.erase(this);
}

rw_result_t file_buffer_t::load(load_process_t *state) {
	string *line;

	if (state->file != this)
		PANIC();

	switch (state->state) {
		case load_process_t::INITIAL: {
			char *_name;
			transcript_t *handle;
			transcript_error_t error;

			if ((_name = canonicalize_path(name)) == NULL)
				return rw_result_t(rw_result_t::ERRNO_ERROR, errno);

			free(name);
			name = _name;

			if ((state->fd = open(name, O_RDONLY)) < 0)
				return rw_result_t(rw_result_t::ERRNO_ERROR, errno);

			try {
				string converted_name;
				lprintf("Using encoding %s to read %s\n", encoding, name);

				if (strcmp(encoding, "UTF-8") == 0) {
					handle = NULL;
				} else {
					handle = transcript_open_converter(encoding, TRANSCRIPT_UTF8, 0, &error);
					if (handle == NULL)
						return rw_result_t(rw_result_t::CONVERSION_OPEN_ERROR, error);
				}
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
				//FIXME: use append_text instead, and reset cursor afterwards
				while ((line = state->wrapper->read_line()) != NULL) {
					try {
						lines.back()->set_text(line);
						lines.push_back(new text_line_t());
					} catch (...) {
						delete line;
						return rw_result_t(rw_result_t::ERRNO_ERROR, ENOMEM);
					}
					delete line;
				}
				/* If the file is not empty, remove the last added line, because
				   it is not actually part of the file. */
				if (lines.size() > 1) {
					delete lines.back();
					lines.pop_back();
				}
			} catch (rw_result_t &result) {
				return result;
			}
			break;
		default:
			PANIC();
	}
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

			/* FIXME: to avoid race conditions, it iw probably better to try open with O_CREAT|O_EXCL
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
				free(encoding);
				encoding = strdup(state->encoding.c_str());
			} else if (strcmp(encoding, "UTF-8") != 0) {
				handle = transcript_open_converter(encoding, TRANSCRIPT_UTF8, 0, &error);
			} else {
				handle = NULL;
			}
			state->wrapper = new file_write_wrapper_t(state->fd, handle);
			state->i = 0;
			state->state = save_as_process_t::WRITING;
		}
		case save_as_process_t::WRITING:
			try {
				for (; state->i < lines.size(); state->i++) {
					const string *data;
					if (state->i != 0)
						state->wrapper->write("\n", 1);
					data = lines[state->i]->get_data();
					state->wrapper->write(data->data(), data->size());
				}
			} catch (rw_result_t error) {
				return error;
			}

			fsync(state->fd);
			close(state->fd);
			state->fd = -1;
			if (state->temp_name != NULL) {
				if (rename(state->temp_name, state->real_name) < 0)
					return rw_result_t(rw_result_t::ERRNO_ERROR, errno);
			}

			if (!state->name.empty()) {
				string converted_name;
				free(name);
				name = strdup(state->name.c_str());
				convert_lang_codeset(name, &converted_name, true);
				name_line.set_text(&converted_name);
			}
			undo_list.set_mark();
			last_undo_type = UNDO_NONE;
			break;
		default:
			PANIC();
	}
	return rw_result_t(rw_result_t::SUCCESS);
}

const char *file_buffer_t::get_encoding(void) const {
	return encoding;
}

const edit_window_t::view_parameters_t *file_buffer_t::get_view_parameters(void) const {
	return &view_parameters;
}

void file_buffer_t::set_has_window(bool _has_window) {
	has_window = _has_window;
}

bool file_buffer_t::get_has_window(void) const {
	return has_window;
}
