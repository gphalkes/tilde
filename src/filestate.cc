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
#include <cstring>

#include "filestate.h"
#include "filebuffer.h"
#include "main.h"

load_state_t::load_state_t(const char *name, const char *encoding, const sigc::slot<void, file_buffer_t *> &_callback) :
		state(INITIAL), callback(_callback), file(NULL), wrapper(NULL), fd(-1)
{
	file = new file_buffer_t(name, encoding);
}

continuation_t::result_t load_state_t::operator()(void) {
	rw_result_t result;
	string message;

	switch ((result = file->load(this))) {
		case rw_result_t::SUCCESS:
			callback(file);
			file = NULL;
			delete this;
			return COMPLETED;
		case rw_result_t::ERRNO_ERROR:
			printf_into(&message, "Could not load file: %s", strerror(result.get_errno_error()));
			message_dialog->set_message(&message);
			delete this;
			return ABORTED;
		case rw_result_t::CONVERSION_OPEN_ERROR:
			printf_into(&message, "Could not find a converter for selected encoding: %s",
				transcript_strerror(result.get_transcript_error()));
			message_dialog->set_message(&message);
			delete this;
			return ABORTED;
		case rw_result_t::CONVERSION_ERROR:
			printf_into(&message, "Could not load file in encoding FIXME: %s", transcript_strerror(result.get_transcript_error()));
			message_dialog->set_message(&message);
			delete this;
			return INCOMPLETE;
		case rw_result_t::CONVERSION_IMPRECISE:
			printf_into(&message, "Conversion from encoding FIXME is irreversible");
			continue_abort_dialog->set_message(&message);
			return INCOMPLETE;
		case rw_result_t::CONVERSION_ILLEGAL:
			//FIXME: handle illegal characters in input
		case rw_result_t::CONVERSION_TRUNCATED:
			//FIXME: handle truncated input
		default:
			PANIC();
	}
	delete this;
	return ABORTED;
}

load_state_t::~load_state_t(void) {
	delete file;
	delete wrapper;
	if (fd >= 0)
		close(fd);
}

save_state_t::save_state_t(file_buffer_t *_file, const char *_encoding, const char *_name) :
	state(INITIAL), file(_file), name(_name), encoding(_encoding), new_name(NULL),
	real_name(NULL), temp_name(NULL), fd(-1), wrapper(NULL)
{
	if (name != NULL) {
		if ((new_name = strdup(name)) == NULL)
			throw bad_alloc();
	}
}

continuation_t::result_t save_state_t::operator()(void) {
	rw_result_t result;
	string message;

	switch ((result = file->save(this))) {
		case rw_result_t::SUCCESS:
			delete this;
			return COMPLETED;
		case rw_result_t::FILE_EXISTS:
			printf_into(&message, "File '%s' already exists", new_name);
			continue_abort_dialog->set_message(&message);
			continue_abort_dialog->show();
		#warning FIXME: if the abort button is pressed, the continuation is not cleaned up!
			return INCOMPLETE;
		case rw_result_t::ERRNO_ERROR:
			printf_into(&message, "Could not save file: %s", strerror(result.get_errno_error()));
			message_dialog->set_message(&message);
			delete this;
			return ABORTED;
		case rw_result_t::CONVERSION_ERROR:
			printf_into(&message, "Could not save file in encoding FIXME: %s", transcript_strerror(result.get_transcript_error()));
			message_dialog->set_message(&message);
			delete this;
			return ABORTED;
		case rw_result_t::CONVERSION_IMPRECISE:
			i++;
			printf_into(&message, "Conversion into encoding FIXME is irreversible");
			continue_abort_dialog->set_message(&message);
		#warning FIXME: if the abort button is pressed, the continuation is not cleaned up!
			return INCOMPLETE;
		default:
			PANIC();
	}
	delete this;
	return ABORTED;
}

save_state_t::~save_state_t(void) {
	free(new_name);
	free(real_name);

	if (fd >= 0) {
		close(fd);
		if (temp_name != NULL) {
			unlink(temp_name);
			free(temp_name);
		} else {
			unlink(name);
		}
	}
	delete wrapper;
}
