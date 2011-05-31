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
#include "openfiles.h"

#warning FIXME: center message dialog

load_process_t::load_process_t(const callback_t &cb) : stepped_process_t(cb), state(SELECT_FILE), file(NULL), wrapper(NULL),
		encoding("UTF-8"), fd(-1) {}

bool load_process_t::step(void) {
	string message;
	rw_result_t rw_result;

	if (state == SELECT_FILE) {
		connections.push_back(open_file_dialog->connect_closed(sigc::mem_fun(this, &load_process_t::abort)));
		connections.push_back(open_file_dialog->connect_file_selected(sigc::mem_fun(this, &load_process_t::file_selected)));
		open_file_dialog->show();
		//~ connections.push_back(encoding_dialog->connect_activate(sigc::mem_fun(this, &load_process_t::encoding_selected)));
		//~ encoding_dialog->set_encoding(encoding->c_str());
		return false;
	}

	disconnect();
	switch ((rw_result = file->load(this))) {
		case rw_result_t::SUCCESS:
			result = true;
			break;
		case rw_result_t::ERRNO_ERROR:
			printf_into(&message, "Could not load file: %s", strerror(rw_result.get_errno_error()));
			message_dialog->set_message(&message);
			message_dialog->show();
			break;
		case rw_result_t::CONVERSION_OPEN_ERROR:
			printf_into(&message, "Could not find a converter for selected encoding: %s",
				transcript_strerror(rw_result.get_transcript_error()));
			message_dialog->set_message(&message);
			message_dialog->show();
			break;
		case rw_result_t::CONVERSION_ERROR:
			printf_into(&message, "Could not load file in encoding FIXME: %s", transcript_strerror(rw_result.get_transcript_error()));
			message_dialog->set_message(&message);
			message_dialog->show();
			break;
		case rw_result_t::CONVERSION_IMPRECISE:
			printf_into(&message, "Conversion from encoding FIXME is irreversible");
			continue_abort_dialog->set_message(&message);
			continue_abort_dialog->show();
			return false;
		case rw_result_t::CONVERSION_ILLEGAL:
			//FIXME: handle illegal characters in input
		case rw_result_t::CONVERSION_TRUNCATED:
			//FIXME: handle truncated input
		default:
			PANIC();
	}
	return true;
}

void load_process_t::file_selected(const string *name) {
	open_files_t::iterator iter;
	if ((iter = open_files.contains(name->c_str())) != open_files.end()) {
		file = *iter;
		done(true);
		return;
	}

	file = new file_buffer_t(name->c_str(), encoding.c_str());
	state = INITIAL;
	run();
}

void load_process_t::encoding_selected(const string *_encoding) {
	encoding = *_encoding;
}

file_buffer_t *load_process_t::get_file_buffer(void) {
	if (result)
		return file;
	return NULL;
}

load_process_t::~load_process_t(void) {
	if (!result)
		delete file;
	delete wrapper;
	if (fd >= 0)
		close(fd);
}

void load_process_t::execute(const callback_t &cb) {
	(new load_process_t(cb))->run();
}


save_as_process_t::save_as_process_t(const callback_t &cb, file_buffer_t *_file) : stepped_process_t(cb), state(SELECT_FILE),
		file(_file), real_name(NULL), temp_name(NULL), fd(-1), wrapper(NULL)
{}

bool save_as_process_t::step(void) {
	string message;
	rw_result_t rw_result;

	if (state == SELECT_FILE) {
		connections.push_back(save_as_dialog->connect_closed(sigc::mem_fun(this, &save_as_process_t::abort)));
		connections.push_back(save_as_dialog->connect_file_selected(sigc::mem_fun(this, &save_as_process_t::file_selected)));
		save_as_dialog->show();
		//~ connections.push_back(encoding_dialog->connect_activate(sigc::mem_fun(this, &save_as_process_t::encoding_selected)));
		//~ encoding_dialog->set_encoding(file->get_encoding());
		return false;
	}

	disconnect();
	switch ((rw_result = file->save(this))) {
		case rw_result_t::SUCCESS:
			result = true;
			break;
		case rw_result_t::FILE_EXISTS:
			printf_into(&message, "File '%s' already exists", name.c_str());
			connections.push_back(continue_abort_dialog->connect_activate(sigc::mem_fun(this, &save_as_process_t::run), 0));
			connections.push_back(continue_abort_dialog->connect_activate(sigc::mem_fun(this, &save_as_process_t::abort), 1));
			continue_abort_dialog->set_message(&message);
			continue_abort_dialog->show();
			return false;
		case rw_result_t::ERRNO_ERROR:
			printf_into(&message, "Could not save file: %s", strerror(rw_result.get_errno_error()));
			message_dialog->set_message(&message);
			message_dialog->show();
			break;
		case rw_result_t::CONVERSION_ERROR:
			printf_into(&message, "Could not save file in encoding FIXME: %s", transcript_strerror(rw_result.get_transcript_error()));
			message_dialog->set_message(&message);
			message_dialog->show();
			break;
		case rw_result_t::CONVERSION_IMPRECISE:
			i++;
			printf_into(&message, "Conversion into encoding FIXME is irreversible");
			connections.push_back(continue_abort_dialog->connect_activate(sigc::mem_fun(this, &save_as_process_t::run), 0));
			connections.push_back(continue_abort_dialog->connect_activate(sigc::mem_fun(this, &save_as_process_t::abort), 1));
			continue_abort_dialog->set_message(&message);
			continue_abort_dialog->show();
			return false;
		default:
			PANIC();
	}
	return true;
}

void save_as_process_t::file_selected(const std::string *_name) {
	name = *_name;
	state = INITIAL;
	run();
}

void save_as_process_t::encoding_selected(const std::string *_encoding) {
	encoding = *_encoding;
}

save_as_process_t::~save_as_process_t(void) {
	free(real_name);

	if (fd >= 0) {
		close(fd);
		if (temp_name != NULL) {
			unlink(temp_name);
			free(temp_name);
		} else {
			unlink(name.c_str());
		}
	}
	delete wrapper;
}

void save_as_process_t::execute(const callback_t &cb, file_buffer_t *_file) {
	(new save_as_process_t(cb, _file))->run();
}

save_process_t::save_process_t(const callback_t &cb, file_buffer_t *_file) : save_as_process_t(cb, _file) {
	if (file->get_name() != NULL)
		state = INITIAL;
}

void save_process_t::execute(const callback_t &cb, file_buffer_t *_file) {
	(new save_process_t(cb, _file))->run();
}
