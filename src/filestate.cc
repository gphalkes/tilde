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
#include <cstring>

#include "filestate.h"
#include "filebuffer.h"
#include "main.h"
#include "openfiles.h"
#include "option.h"

load_process_t::load_process_t(const callback_t &cb) : stepped_process_t(cb), state(SELECT_FILE), bom_state(UNKNOWN), file(NULL),
		wrapper(NULL), encoding("UTF-8"), fd(-1), buffer_used(true) {}

load_process_t::load_process_t(const callback_t &cb, const char *name, const char *_encoding) : stepped_process_t(cb), state(INITIAL),
		bom_state(UNKNOWN), file(new file_buffer_t(name, _encoding == NULL ? "UTF-8" : _encoding)), wrapper(NULL),
		encoding(_encoding == NULL ? "UTF-8" : _encoding), fd(-1), buffer_used(true) {}

void load_process_t::abort(void) {
	delete file;
	file = NULL;
	stepped_process_t::abort();
}

bool load_process_t::step(void) {
	string message;
	rw_result_t rw_result;

	if (state == SELECT_FILE) {
		connections.push_back(open_file_dialog->connect_closed(sigc::mem_fun(this, &load_process_t::abort)));
		connections.push_back(open_file_dialog->connect_file_selected(sigc::mem_fun(this, &load_process_t::file_selected)));
		open_file_dialog->reset();
		open_file_dialog->show();
		connections.push_back(encoding_dialog->connect_activate(sigc::mem_fun(this, &load_process_t::encoding_selected)));
		encoding_dialog->set_encoding(encoding.c_str());
		return false;
	}

	disconnect();
	switch ((rw_result = file->load(this))) {
		case rw_result_t::SUCCESS:
			result = true;
			break;
		case rw_result_t::ERRNO_ERROR:
			printf_into(&message, "Could not load file '%s': %s", file->get_name(), strerror(rw_result.get_errno_error()));
			error_dialog->set_message(&message);
			error_dialog->show();
			result = true;
			break;
		case rw_result_t::CONVERSION_OPEN_ERROR:
			printf_into(&message, "Could not find a converter for selected encoding: %s",
				transcript_strerror(rw_result.get_transcript_error()));
			error_dialog->set_message(&message);
			error_dialog->show();
			break;
		case rw_result_t::CONVERSION_ERROR:
			printf_into(&message, "Could not load file in encoding %s: %s", file->get_encoding(), transcript_strerror(rw_result.get_transcript_error()));
			error_dialog->set_message(&message);
			error_dialog->show();
			break;
		case rw_result_t::CONVERSION_IMPRECISE:
			printf_into(&message, "Conversion from encoding %s is irreversible", file->get_encoding());
			connections.push_back(continue_abort_dialog->connect_activate(sigc::mem_fun(this, &load_process_t::run), 0));
			connections.push_back(continue_abort_dialog->connect_activate(sigc::mem_fun(this, &load_process_t::abort), 1));
			continue_abort_dialog->set_message(&message);
			continue_abort_dialog->show();
			return false;
		case rw_result_t::CONVERSION_ILLEGAL:
			printf_into(&message, "Conversion from encoding %s encountered illegal characters", file->get_encoding());
			connections.push_back(continue_abort_dialog->connect_activate(sigc::mem_fun(this, &load_process_t::run), 0));
			connections.push_back(continue_abort_dialog->connect_activate(sigc::mem_fun(this, &load_process_t::abort), 1));
			continue_abort_dialog->set_message(&message);
			continue_abort_dialog->show();
			return false;
		case rw_result_t::CONVERSION_TRUNCATED:
			printf_into(&message, "File appears to be truncated");
			result = true;
			error_dialog->set_message(&message);
			error_dialog->show();
			break;
		case rw_result_t::BOM_FOUND:
			connections.push_back(preserve_bom_dialog->connect_activate(sigc::mem_fun(this, &load_process_t::preserve_bom), 0));
			connections.push_back(preserve_bom_dialog->connect_activate(sigc::mem_fun(this, &load_process_t::remove_bom), 1));
			preserve_bom_dialog->show();
			return false;
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
	if (!result && file != NULL)
		delete file;
	delete wrapper;
	if (fd >= 0)
		close(fd);
}

void load_process_t::preserve_bom(void) {
	//FIXME: set encoding accordingly
	bom_state = PRESERVE_BOM;
	run();
}

void load_process_t::remove_bom(void) {
	bom_state = REMOVE_BOM;
	run();
}

void load_process_t::execute(const callback_t &cb) {
	(new load_process_t(cb))->run();
}

void load_process_t::execute(const callback_t &cb, const char *name, const char *encoding) {
	(new load_process_t(cb, name, encoding))->run();
}

save_as_process_t::save_as_process_t(const callback_t &cb, file_buffer_t *_file) : stepped_process_t(cb), state(SELECT_FILE),
		file(_file), real_name(NULL), temp_name(NULL), fd(-1), wrapper(NULL)
{}

bool save_as_process_t::step(void) {
	string message;
	rw_result_t rw_result;

	if (state == SELECT_FILE) {
		const char *current_name = file->get_name();

		connections.push_back(save_as_dialog->connect_closed(sigc::mem_fun(this, &save_as_process_t::abort)));
		connections.push_back(save_as_dialog->connect_file_selected(sigc::mem_fun(this, &save_as_process_t::file_selected)));

		save_as_dialog->set_file(current_name);
		save_as_dialog->show();
		connections.push_back(encoding_dialog->connect_activate(sigc::mem_fun(this, &save_as_process_t::encoding_selected)));
		encoding_dialog->set_encoding(file->get_encoding());
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
			error_dialog->set_message(&message);
			error_dialog->show();
			break;
		case rw_result_t::CONVERSION_ERROR:
			printf_into(&message, "Could not save file in encoding FIXME: %s", transcript_strerror(rw_result.get_transcript_error()));
			error_dialog->set_message(&message);
			error_dialog->show();
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
	if (file->get_highlight() == NULL && file->get_name() == NULL)
		file->set_highlight(t3_highlight_load_by_filename(name.c_str(), map_highlight, NULL, T3_HIGHLIGHT_UTF8, NULL));
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
		recent_files.push_front(file);
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

exit_process_t::exit_process_t(const callback_t &cb) : stepped_process_t(cb), iter(open_files.begin()) {}

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
	delete this;
	exit_main_loop(EXIT_SUCCESS);
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

open_recent_process_t::open_recent_process_t(const callback_t &cb) : load_process_t(cb) {}

bool open_recent_process_t::step(void) {
	if (state == SELECT_FILE) {
		connections.push_back(open_recent_dialog->connect_file_selected(sigc::mem_fun(this,
			&open_recent_process_t::recent_file_selected)));
		connections.push_back(open_recent_dialog->connect_closed(sigc::mem_fun(this, &open_recent_process_t::abort)));
		open_recent_dialog->show();
		return false;
	}
	return load_process_t::step();
}

void open_recent_process_t::recent_file_selected(recent_file_info_t *_info) {
	info = _info;
	file = new file_buffer_t(info->get_name(), info->get_encoding());
	state = INITIAL;
	run();
}

open_recent_process_t::~open_recent_process_t(void) {
	if (result)
		recent_files.erase(info);
}

void open_recent_process_t::execute(const callback_t &cb) {
	(new open_recent_process_t(cb))->run();
}


load_cli_file_process_t::load_cli_file_process_t(const callback_t &cb) : stepped_process_t(cb),
		iter(cli_option.files.begin()), in_load(false), in_step(false) {}

bool load_cli_file_process_t::step(void) {
	in_step = true;
	while (iter != cli_option.files.end()) {
		in_load = true;
		#warning FIXME: divert to choice for encoding if no value is given for -e
		load_process_t::execute(sigc::mem_fun(this, &load_cli_file_process_t::load_done), *iter, cli_option.encoding.is_valid() ? cli_option.encoding : NULL);
		if (in_load) {
			in_step = false;
			return false;
		}
	}
	result = true;
	return true;
}

void load_cli_file_process_t::load_done(stepped_process_t *process) {
	(void) process;

	in_load = false;
	iter++;
	if (!in_step)
		run();
}

void load_cli_file_process_t::execute(const callback_t &cb) {
	(new load_cli_file_process_t(cb))->run();
}
