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

close_process_t::close_process_t(const callback_t &cb, file_buffer_t *_file) : save_process_t(cb, _file) {
	state = file->is_modified() ? CONFIRM_CLOSE : CLOSE;
	if (!_file->is_modified())
		state = CLOSE;
}

bool close_process_t::step(void) {

	if (state < CONFIRM_CLOSE) {
		if (save_process_t::step()) {
			if (!result)
				return true;
			state = CLOSE;
		} else {
			return false;
		}
	}

	disconnect();
	if (state == CLOSE) {
		//FIXME: add to recent files
		open_files.erase(file);
		/* Can't delete the file_buffer_t here, because on switching buffers the
		   edit_window_t will still want to do some stuff with it. Furthermore,
		   the newly allocated file_buffer_t may be at the same address, causing
		   further difficulties. So that has to be done in the callback :-( */
		result = true;
		return true;
	} else if (state == CONFIRM_CLOSE) {
		string message;
		printf_into(&message, "Save changes to '%s'", file->get_name() == NULL ? "(Untitled)" : file->get_name());
		connections.push_back(close_confirm_dialog->connect_activate(sigc::mem_fun(this, &close_process_t::do_save), 0));
		connections.push_back(close_confirm_dialog->connect_activate(sigc::mem_fun(this, &close_process_t::dont_save), 1));
		connections.push_back(close_confirm_dialog->connect_activate(sigc::mem_fun(this, &close_process_t::abort), 2));
		close_confirm_dialog->set_message(&message);
		close_confirm_dialog->show();
	} else {
		PANIC();
	}
	return false;
}

void close_process_t::do_save(void) {
	state = file->get_name() == NULL ? SELECT_FILE : INITIAL;
	run();
}

void close_process_t::dont_save(void) {
	state = CLOSE;
	run();
}

void close_process_t::execute(const callback_t &cb, file_buffer_t *_file) {
	(new close_process_t(cb, _file))->run();
}

const file_buffer_t *close_process_t::get_file_buffer_ptr(void) {
	return file;
}

exit_process_t::exit_process_t(const callback_t &cb) : stepped_process_t(cb), iter(open_files.begin()), in_step(false), in_save(false) {}

bool exit_process_t::step(void) {
	for (; iter != open_files.end(); iter++) {
		if ((*iter)->is_modified()) {
			string message;
			printf_into(&message, "Save changes to '%s'", (*iter)->get_name() == NULL ? "(Untitled)" : (*iter)->get_name());
			connections.push_back(close_confirm_dialog->connect_activate(sigc::mem_fun(this, &exit_process_t::do_save), 0));
			connections.push_back(close_confirm_dialog->connect_activate(sigc::mem_fun(this, &exit_process_t::dont_save), 1));
			connections.push_back(close_confirm_dialog->connect_activate(sigc::mem_fun(this, &exit_process_t::abort), 2));
			close_confirm_dialog->set_message(&message);
			close_confirm_dialog->show();
			return false;
		}
	}
	exit(EXIT_SUCCESS);
/*	in_step = false;
	return true;*/
}

void exit_process_t::do_save(void) {
	save_process_t::execute(sigc::mem_fun(this, &exit_process_t::save_done), *iter);
}

void exit_process_t::dont_save(void) {
	iter++;
	run();
}

void exit_process_t::save_done(stepped_process_t *process) {
	if (process->get_result()) {
		iter++;
		run();
	} else {
		abort();
	}
}

void exit_process_t::execute(const callback_t &cb) {
	(new exit_process_t(cb))->run();
}

