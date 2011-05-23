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

#define BOM_STRING "\xEF\xBB\xBF"

file_buffer_t::file_buffer_t(const char *name, const char *_encoding) : text_buffer_t(name),
		encoding(_encoding), file_has_bom(false)
{
	open_files.push_back(this);
}

rw_result_t file_buffer_t::load(load_state_t *state) {
	string *line;

	if (state->file != this)
		PANIC();

	switch (state->state) {
		case load_state_t::INITIAL: {
			char *_name;
			transcript_t *handle;
			transcript_error_t error;

			if ((_name = canonicalize_path(name)) == NULL)
				return rw_result_t(rw_result_t::ERRNO_ERROR, errno);

			free(name);
			name = _name;

			if ((state->fd = open(name, O_RDONLY)) < 0)
				return rw_result_t(rw_result_t::ERRNO_ERROR, errno);
#warning FIXME: check order of actions
	//perhaps we should try opening the converter earlier
			try {
				handle = transcript_open_converter(encoding, TRANSCRIPT_UTF8, 0, &error);
				if (handle == NULL)
					return rw_result_t(rw_result_t::CONVERSION_OPEN_ERROR, error);
				state->wrapper = new file_read_wrapper_t(state->fd, handle);
				name_line.set_text(name);
			} catch (bad_alloc &ba) {
				return rw_result_t(rw_result_t::ERRNO_ERROR, ENOMEM);
			}
			state->state = load_state_t::READING;
		}
		case load_state_t::READING:
			try {
				while ((line = state->wrapper->read_line()) != NULL) {
					#warning FIXME: check that file is UTF-8 encoded
					if (lines.size() == 0 && line->size() >= 3 && /* encoding->getIndex() == utf8CharacterSetIndex && */
							memcmp(line->c_str(), BOM_STRING, 3) == 0)
					{
						file_has_bom = true;
						line->erase(0, 3);
					}

					/* Allocate a new text_line_t struct and initialize it with the newly read line */
					try {
						lines.push_back(new text_line_t(line));
					} catch (...) {
						delete line;
						return rw_result_t(rw_result_t::ERRNO_ERROR, ENOMEM);
					}
					delete line;
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

rw_result_t file_buffer_t::save(save_state_t *state) {
	size_t idx;
	const char *save_name;

	if (state->file != this)
		PANIC();

	switch (state->state) {
		case save_state_t::INITIAL:
			if (state->name == NULL) {
				if (name == NULL)
					PANIC();
				save_name = name;
			} else {
				save_name = state->name;
			}

			if ((state->real_name = resolve_links(save_name)) == NULL)
				return rw_result_t(rw_result_t::ERRNO_ERROR, ENAMETOOLONG);

			if (stat(state->real_name, &state->file_info) < 0) {
				if (errno == ENOENT) {
					if ((state->fd = creat(state->real_name, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) < 0)
						return rw_result_t(rw_result_t::ERRNO_ERROR, errno);
				} else {
					return rw_result_t(rw_result_t::ERRNO_ERROR, errno);
				}
			} else {
				state->state = save_state_t::ALLOW_OVERWRITE;
				if (state->name != NULL)
					return rw_result_t(rw_result_t::FILE_EXISTS);
				/* Note that we have a new case in the middle of the else statement
				   here. It is an ugly hack, but it does prevent a lot of code
				   duplication and other hacks. */
		case save_state_t::ALLOW_OVERWRITE:
				string tempNameStr = state->real_name;

				if ((idx = tempNameStr.rfind('/')) == string::npos)
					idx = 0;
				else
					idx++;

				tempNameStr.erase(idx);
				try {
					tempNameStr.append(".tildeXXXXXX");
				} catch (bad_alloc &ba) {
					return rw_result_t(rw_result_t::ERRNO_ERROR, ENOMEM);
				}

				/* Unfortunately, we can't pass the c_str result to mkstemp as we are not allowed
				   to change that string. So we'll just have to strdup it :-( */
				if ((state->temp_name = strdup(tempNameStr.c_str())) == NULL)
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
			if (state->encoding != NULL)
				handle = transcript_open_converter(state->encoding, TRANSCRIPT_UTF8, 0, &error);
			else if (encoding != NULL)
				handle = transcript_open_converter(encoding, TRANSCRIPT_UTF8, 0, &error);
			else
				handle = NULL;
			state->wrapper = new file_write_wrapper_t(state->fd, handle);
			state->i = 0;
			state->state = save_state_t::WRITING;
		}
		case save_state_t::WRITING:
			try {
				if (file_has_bom && state->i == 0)
					state->wrapper->write(BOM_STRING, 3);

				if (state->i != 0)
					state->wrapper->write("\n", 1);

				for (; state->i < lines.size(); state->i++)
				{}
				#warning FIXME: write line data!
				//	lines[state->i]->write_line_data(state->wrapper, state->i < lines.size() - 1);
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

			if (state->new_name != NULL) {
				free(name);
				name = state->new_name;
				name_line.set_text(state->new_name);
				state->new_name = NULL;
			}
			undo_list.set_mark();
			last_undo_type = UNDO_NONE;
			break;
		default:
			PANIC();
	}
	return rw_result_t(rw_result_t::SUCCESS);
}


