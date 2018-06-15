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
#include <cstring>

#include "tilde/filebuffer.h"
#include "tilde/filestate.h"
#include "tilde/main.h"
#include "tilde/openfiles.h"
#include "tilde/option.h"

load_process_t::load_process_t(const callback_t &cb)
    : stepped_process_t(cb),
      state(SELECT_FILE),
      bom_state(UNKNOWN),
      file(nullptr),
      wrapper(nullptr),
      encoding("UTF-8"),
      fd(-1),
      buffer_used(true) {}

load_process_t::load_process_t(const callback_t &cb, const char *name, const char *_encoding,
                               bool missing_ok)
    : stepped_process_t(cb),
      state(missing_ok ? INITIAL_MISSING_OK : INITIAL),
      bom_state(UNKNOWN),
      file(new file_buffer_t(name, _encoding == nullptr ? "UTF-8" : _encoding)),
      wrapper(nullptr),
      encoding(_encoding == nullptr ? "UTF-8" : _encoding),
      fd(-1),
      buffer_used(true) {}

void load_process_t::abort() {
  delete file;
  file = nullptr;
  stepped_process_t::abort();
}

bool load_process_t::step() {
  std::string message;
  rw_result_t rw_result;

  if (state == SELECT_FILE) {
    connections.push_back(
        open_file_dialog->connect_file_selected(bind_front(&load_process_t::file_selected, this)));
    connections.push_back(open_file_dialog->connect_closed([this] { abort(); }));
    open_file_dialog->reset();
    open_file_dialog->show();
    connections.push_back(
        encoding_dialog->connect_activate(bind_front(&load_process_t::encoding_selected, this)));
    encoding_dialog->set_encoding(encoding.c_str());
    return false;
  }

  disconnect();
  switch ((rw_result = file->load(this))) {
    case rw_result_t::SUCCESS:
      result = true;
      break;
    case rw_result_t::ERRNO_ERROR:
      printf_into(&message, "Could not load file '%s': %s", file->get_name().c_str(),
                  strerror(rw_result.get_errno_error()));
      error_dialog->set_message(message);
      error_dialog->show();
      result = true;
      break;
    case rw_result_t::CONVERSION_OPEN_ERROR:
      printf_into(&message, "Could not find a converter for selected encoding: %s",
                  transcript_strerror(rw_result.get_transcript_error()));
      delete file;
      file = nullptr;
      error_dialog->set_message(message);
      error_dialog->show();
      break;
    case rw_result_t::CONVERSION_ERROR:
      printf_into(&message, "Could not load file in encoding %s: %s", file->get_encoding(),
                  transcript_strerror(rw_result.get_transcript_error()));
      delete file;
      file = nullptr;
      error_dialog->set_message(message);
      error_dialog->show();
      break;
    case rw_result_t::CONVERSION_IMPRECISE:
      printf_into(&message, "Conversion from encoding %s is irreversible", file->get_encoding());
      connections.push_back(continue_abort_dialog->connect_activate([this] { run(); }, 0));
      connections.push_back(continue_abort_dialog->connect_activate([this] { abort(); }, 1));
      connections.push_back(continue_abort_dialog->connect_closed([this] { abort(); }));
      continue_abort_dialog->set_message(message);
      continue_abort_dialog->show();
      return false;
    case rw_result_t::CONVERSION_ILLEGAL:
      printf_into(&message, "Conversion from encoding %s encountered illegal characters",
                  file->get_encoding());
      connections.push_back(continue_abort_dialog->connect_activate([this] { run(); }, 0));
      connections.push_back(continue_abort_dialog->connect_activate([this] { abort(); }, 1));
      connections.push_back(continue_abort_dialog->connect_closed([this] { abort(); }));
      continue_abort_dialog->set_message(message);
      continue_abort_dialog->show();
      return false;
    case rw_result_t::CONVERSION_TRUNCATED:
      printf_into(&message, "File appears to be truncated");
      result = true;
      error_dialog->set_message(message);
      error_dialog->show();
      break;
    case rw_result_t::BOM_FOUND:
      connections.push_back(preserve_bom_dialog->connect_activate([this] { preserve_bom(); }, 0));
      connections.push_back(preserve_bom_dialog->connect_activate([this] { remove_bom(); }, 1));
      connections.push_back(preserve_bom_dialog->connect_closed([this] { preserve_bom(); }));
      preserve_bom_dialog->show();
      return false;
    default:
      PANIC();
  }
  return true;
}

void load_process_t::file_selected(const std::string &name) {
  open_files_t::iterator iter;
  if ((iter = open_files.contains(name.c_str())) != open_files.end()) {
    file = *iter;
    done(true);
    return;
  }

  file = new file_buffer_t(name, encoding);
  state = INITIAL;
  run();
}

void load_process_t::encoding_selected(const std::string *_encoding) { encoding = *_encoding; }

file_buffer_t *load_process_t::get_file_buffer() {
  if (result) {
    return file;
  }
  return nullptr;
}

load_process_t::~load_process_t() {
#ifdef DEBUG
  ASSERT(result || file == nullptr);
#endif
  delete wrapper;
  if (fd >= 0) {
    close(fd);
  }
}

void load_process_t::preserve_bom() {
  // FIXME: set encoding accordingly
  bom_state = PRESERVE_BOM;
  run();
}

void load_process_t::remove_bom() {
  bom_state = REMOVE_BOM;
  run();
}

void load_process_t::execute(const callback_t &cb) { (new load_process_t(cb))->run(); }

void load_process_t::execute(const callback_t &cb, const char *name, const char *encoding,
                             bool missing_ok) {
  (new load_process_t(cb, name, encoding, missing_ok))->run();
}

save_as_process_t::save_as_process_t(const callback_t &cb, file_buffer_t *_file,
                                     bool _allow_highlight_change)
    : stepped_process_t(cb),
      state(SELECT_FILE),
      file(_file),
      allow_highlight_change(_allow_highlight_change),
      highlight_changed(false),
      save_name(nullptr),
      fd(-1),
      wrapper(nullptr) {}

bool save_as_process_t::step() {
  std::string message;
  rw_result_t rw_result;

  if (state == SELECT_FILE) {
    const char *current_name = file->get_name().c_str();

    connections.push_back(
        save_as_dialog->connect_file_selected(bind_front(&save_as_process_t::file_selected, this)));
    connections.push_back(save_as_dialog->connect_closed([this] { abort(); }));

    save_as_dialog->set_file(current_name);
    save_as_dialog->show();
    connections.push_back(
        encoding_dialog->connect_activate(bind_front(&save_as_process_t::encoding_selected, this)));
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
      connections.push_back(continue_abort_dialog->connect_activate([this] { run(); }, 0));
      connections.push_back(continue_abort_dialog->connect_activate([this] { abort(); }, 1));
      connections.push_back(continue_abort_dialog->connect_closed([this] { abort(); }));
      continue_abort_dialog->set_message(message);
      continue_abort_dialog->show();
      return false;
    case rw_result_t::FILE_EXISTS_READONLY:
      printf_into(&message, "File '%s' is readonly", save_name);
      connections.push_back(continue_abort_dialog->connect_activate([this] { run(); }, 0));
      connections.push_back(continue_abort_dialog->connect_activate([this] { abort(); }, 1));
      connections.push_back(continue_abort_dialog->connect_closed([this] { abort(); }));
      continue_abort_dialog->set_message(message);
      continue_abort_dialog->show();
      return false;
    case rw_result_t::ERRNO_ERROR:
      printf_into(&message, "Could not save file: %s", strerror(rw_result.get_errno_error()));
      error_dialog->set_message(message);
      error_dialog->show();
      break;
    case rw_result_t::CONVERSION_ERROR:
      if (encoding.empty()) {
        encoding = file->get_encoding();
      }
      printf_into(&message, "Could not save file in encoding %s: %s", encoding.c_str(),
                  transcript_strerror(rw_result.get_transcript_error()));
      error_dialog->set_message(message);
      error_dialog->show();
      break;
    case rw_result_t::CONVERSION_IMPRECISE:
      if (encoding.empty()) {
        encoding = file->get_encoding();
      }
      i++;
      printf_into(&message, "Conversion into encoding %s is irreversible", encoding.c_str());
      connections.push_back(continue_abort_dialog->connect_activate([this] { run(); }, 0));
      connections.push_back(continue_abort_dialog->connect_activate([this] { abort(); }, 1));
      connections.push_back(continue_abort_dialog->connect_closed([this] { abort(); }));
      continue_abort_dialog->set_message(message);
      continue_abort_dialog->show();
      return false;
    default:
      PANIC();
  }
  return true;
}

void save_as_process_t::file_selected(const std::string &_name) {
  name = _name;
  state = INITIAL;
  if (allow_highlight_change) {
    highlight_changed = true;
    file->set_highlight(t3_highlight_load_by_filename(name.c_str(), map_highlight, nullptr,
                                                      T3_HIGHLIGHT_UTF8, nullptr));
  }
  run();
}

void save_as_process_t::encoding_selected(const std::string *_encoding) { encoding = *_encoding; }

save_as_process_t::~save_as_process_t() {
  if (fd >= 0) {
    close(fd);
    if (temp_name.empty()) {
      unlink(name.c_str());
    } else {
      unlink(temp_name.c_str());
    }
  }
  delete wrapper;
}

void save_as_process_t::execute(const callback_t &cb, file_buffer_t *_file) {
  (new save_as_process_t(cb, _file))->run();
}

bool save_as_process_t::get_highlight_changed() const { return highlight_changed; }

save_process_t::save_process_t(const callback_t &cb, file_buffer_t *_file)
    : save_as_process_t(cb, _file, false) {
  if (!file->get_name().empty()) {
    state = INITIAL;
  }
}

void save_process_t::execute(const callback_t &cb, file_buffer_t *_file) {
  (new save_process_t(cb, _file))->run();
}

close_process_t::close_process_t(const callback_t &cb, file_buffer_t *_file)
    : save_process_t(cb, _file) {
  state = file->is_modified() ? CONFIRM_CLOSE : CLOSE;
  if (!_file->is_modified()) {
    state = CLOSE;
  }
}

bool close_process_t::step() {
  if (state < CONFIRM_CLOSE) {
    if (save_process_t::step()) {
      if (!result) {
        return true;
      }
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
    std::string message;
    printf_into(&message, "Save changes to '%s'",
                file->get_name().empty() ? "(Untitled)" : file->get_name().c_str());
    connections.push_back(close_confirm_dialog->connect_activate([this] { do_save(); }, 0));
    connections.push_back(close_confirm_dialog->connect_activate([this] { dont_save(); }, 1));
    connections.push_back(close_confirm_dialog->connect_activate([this] { abort(); }, 2));
    connections.push_back(close_confirm_dialog->connect_closed([this] { abort(); }));
    close_confirm_dialog->set_message(message);
    close_confirm_dialog->show();
  } else {
    PANIC();
  }
  return false;
}

void close_process_t::do_save() {
  state = file->get_name().empty() ? SELECT_FILE : INITIAL;
  run();
}

void close_process_t::dont_save() {
  state = CLOSE;
  run();
}

void close_process_t::execute(const callback_t &cb, file_buffer_t *_file) {
  (new close_process_t(cb, _file))->run();
}

const file_buffer_t *close_process_t::get_file_buffer_ptr() { return file; }

exit_process_t::exit_process_t(const callback_t &cb)
    : stepped_process_t(cb), iter(open_files.begin()) {}

bool exit_process_t::step() {
  for (; iter != open_files.end(); iter++) {
    if ((*iter)->is_modified()) {
      std::string message;
      printf_into(&message, "Save changes to '%s'",
                  (*iter)->get_name().empty() ? "(Untitled)" : (*iter)->get_name().c_str());
      connections.push_back(close_confirm_dialog->connect_activate([this] { do_save(); }, 0));
      connections.push_back(close_confirm_dialog->connect_activate([this] { dont_save(); }, 1));
      connections.push_back(close_confirm_dialog->connect_activate([this] { abort(); }, 2));
      connections.push_back(close_confirm_dialog->connect_closed([this] { abort(); }));
      close_confirm_dialog->set_message(message);
      close_confirm_dialog->show();
      return false;
    }
  }
  delete this;
  exit_main_loop(EXIT_SUCCESS);
  /*	in_step = false;
          return true;*/
}

void exit_process_t::do_save() {
  save_process_t::execute(bind_front(&exit_process_t::save_done, this), *iter);
}

void exit_process_t::dont_save() {
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

void exit_process_t::execute(const callback_t &cb) { (new exit_process_t(cb))->run(); }

open_recent_process_t::open_recent_process_t(const callback_t &cb) : load_process_t(cb) {}

bool open_recent_process_t::step() {
  if (state == SELECT_FILE) {
    connections.push_back(open_recent_dialog->connect_file_selected(
        bind_front(&open_recent_process_t::recent_file_selected, this)));
    connections.push_back(open_recent_dialog->connect_closed([this] { abort(); }));
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

open_recent_process_t::~open_recent_process_t() {
  if (result) {
    recent_files.erase(info);
  }
}

void open_recent_process_t::execute(const callback_t &cb) {
  (new open_recent_process_t(cb))->run();
}

load_cli_file_process_t::load_cli_file_process_t(const callback_t &cb)
    : stepped_process_t(cb),
      iter(cli_option.files.begin()),
      in_load(false),
      in_step(false),
      encoding_selected(false) {}

bool load_cli_file_process_t::step() {
  if (!encoding_selected) {
    encoding_selected = true;
    if (cli_option.encoding.is_valid()) {
      if (cli_option.encoding.value() == nullptr) {
        encoding_dialog->set_encoding("UTF-8");
        encoding_dialog->connect_activate(
            bind_front(&load_cli_file_process_t::encoding_selection_done, this));
        encoding_dialog->connect_closed([this] { run(); });
        encoding_dialog->show();
        return false;
      } else {
        encoding = cli_option.encoding.value();
      }
    }
  }

  in_step = true;
  while (iter != cli_option.files.end()) {
    int line = -1, pos = -1;
    std::string filename = *iter;
    if (default_option.parse_file_positions.value_or(true) &&
        !cli_option.disable_file_position_parsing) {
      attempt_file_position_parse(&filename, &line, &pos);
    }

    in_load = true;
    load_process_t::execute(bind_front(&load_cli_file_process_t::load_done, this), filename.c_str(),
                            encoding.c_str(), true);
    if (in_load) {
      in_step = false;
      return false;
    }
    open_files.back()->goto_pos(line, pos);
  }
  result = true;
  return true;
}

void load_cli_file_process_t::load_done(stepped_process_t *process) {
  (void)process;

  in_load = false;
  iter++;
  if (!in_step) {
    run();
  }
}

void load_cli_file_process_t::execute(const callback_t &cb) {
  (new load_cli_file_process_t(cb))->run();
}

void load_cli_file_process_t::encoding_selection_done(const std::string *_encoding) {
  encoding = _encoding->c_str();
  run();
}

static bool is_ascii_digit(int c) { return c >= '0' && c <= '9'; }

void load_cli_file_process_t::attempt_file_position_parse(std::string *filename, int *line,
                                                          int *pos) {
  size_t idx = filename->size();
  if (idx < 3) {
    return;
  }
  --idx;

  // Skip trailing colon. These will typically occur when copying error messages.
  if ((*filename)[idx] == ':') {
    --idx;
  }

  while (idx > 0 && is_ascii_digit((*filename)[idx])) {
    --idx;
  }
  if (idx == 0) {
    return;
  }
  if ((*filename)[idx] != ':') {
    return;
  }

  *line = atoi(filename->c_str() + idx + 1);
  filename->erase(idx);

  --idx;
  while (idx > 0 && is_ascii_digit((*filename)[idx])) {
    --idx;
  }
  if (idx == 0 || (*filename)[idx] != ':') {
    return;
  }
  *pos = *line;
  *line = atoi(filename->c_str() + idx + 1);
  filename->erase(idx);
}
