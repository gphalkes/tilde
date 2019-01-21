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
#include <fcntl.h>
#include <unistd.h>

#include "tilde/copy_file.h"
#include "tilde/filebuffer.h"
#include "tilde/fileline.h"
#include "tilde/filestate.h"
#include "tilde/log.h"
#include "tilde/openfiles.h"
#include "tilde/option.h"

#define CREATE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

file_buffer_t::file_buffer_t(string_view _name, string_view _encoding)
    : text_buffer_t(new file_line_factory_t(this)),
      view_parameters(new edit_window_t::view_parameters_t()),
      has_window(false),
      highlight_valid(0),
      highlight_info(nullptr),
      match_line(nullptr),
      last_match(nullptr),
      matching_brace_valid(false) {
  if (_encoding.size() == 0) {
    encoding = "UTF-8";
  } else {
    encoding = std::string(_encoding);
  }

  if (_name.size() == 0) {
    name_line.set_text("(Untitled)");
  } else {
    name = std::string(_name);

    std::string converted_name = convert_lang_codeset(name, true);
    name_line.set_text(converted_name);
  }

  connect_rewrap_required(bind_front(&file_buffer_t::invalidate_highlight, this));

  view_parameters->set_tabsize(option.tabsize);
  view_parameters->set_wrap(option.wrap ? wrap_type_t::WORD : wrap_type_t::NONE);
  view_parameters->set_auto_indent(option.auto_indent);
  view_parameters->set_tab_spaces(option.tab_spaces);
  view_parameters->set_indent_aware_home(option.indent_aware_home);
  view_parameters->set_show_tabs(option.show_tabs);
  open_files.push_back(this);
}

file_buffer_t::~file_buffer_t() {
  open_files.erase(this);
  t3_highlight_free(highlight_info);
  t3_highlight_free_match(last_match);
  delete get_line_factory();
}

rw_result_t file_buffer_t::load(load_process_t *state) {
  t3_highlight_t *highlight = nullptr;
  t3_highlight_lang_t lang;
  t3_bool success = t3_false;
  text_pos_t i;

  if (state->file != this) {
    PANIC();
  }

  switch (state->state) {
    case load_process_t::INITIAL_MISSING_OK:
    case load_process_t::INITIAL: {
      transcript_t *handle;
      transcript_error_t error;

      std::string _name = canonicalize_path(name.c_str());
      if (_name.empty()) {
        if (errno == ENOENT && state->state == load_process_t::INITIAL_MISSING_OK) {
          break;
        }
        return rw_result_t(rw_result_t::ERRNO_ERROR, errno);
      }

      name = _name;
      std::string converted_name = convert_lang_codeset(name, true);
      name_line.set_text(converted_name);

      if ((state->fd = open(name.c_str(), O_RDONLY)) < 0) {
        if (errno == ENOENT && state->state == load_process_t::INITIAL_MISSING_OK) {
          break;
        }
        return rw_result_t(rw_result_t::ERRNO_ERROR, errno);
      }

      try {
        lprintf("Using encoding %s to read %s\n", encoding.c_str(), name.c_str());

        handle = transcript_open_converter(encoding.c_str(), TRANSCRIPT_UTF8, 0, &error);
        if (handle == nullptr) {
          return rw_result_t(rw_result_t::CONVERSION_OPEN_ERROR, error);
        }
        // FIXME: if the new fails, the handle will remain open!
        state->wrapper = new file_read_wrapper_t(state->fd, handle);

      } catch (std::bad_alloc &ba) {
        return rw_result_t(rw_result_t::ERRNO_ERROR, ENOMEM);
      }
      state->state = load_process_t::READING_FIRST;
    }
    case load_process_t::READING:
    case load_process_t::READING_FIRST:
      try {
        while (!state->buffer_used || state->wrapper->fill_buffer(state->wrapper->get_fill())) {
          state->buffer_used = false;
          if (state->state == load_process_t::READING_FIRST) {
            switch (state->bom_state) {
              case load_process_t::UNKNOWN:
                if (state->wrapper->get_fill() >= 3 && transcript_equal(encoding.c_str(), "utf8") &&
                    memcmp(state->wrapper->get_buffer(), "\xef\xbb\xbf", 3) == 0) {
                  return rw_result_t(rw_result_t::BOM_FOUND);
                }
                break;
              case load_process_t::PRESERVE_BOM:
                encoding = "X-UTF-8-BOM";
              /* FALLTHROUGH */
              case load_process_t::REMOVE_BOM:
                try {
                  append_text(string_view(state->wrapper->get_buffer() + 3,
                                          state->wrapper->get_fill() - 3));
                  state->buffer_used = true;
                } catch (...) {
                  return rw_result_t(rw_result_t::ERRNO_ERROR, ENOMEM);
                }
                state->state = load_process_t::READING;
                continue;
              default:
                break;
            }
            state->state = load_process_t::READING;
          }

          try {
            append_text(string_view(state->wrapper->get_buffer(), state->wrapper->get_fill()));
            state->buffer_used = true;
          } catch (...) {
            return rw_result_t(rw_result_t::ERRNO_ERROR, ENOMEM);
          }
        }
        set_cursor({0, 0});
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
  for (i = 0; i < size() && i < 5 && !success; i++) {
    const std::string &line = get_line_data(i).get_data();
    success =
        t3_highlight_detect(line.data(), line.size(), false, T3_HIGHLIGHT_UTF8, &lang, nullptr);
  }
  for (i = size() - 1; i >= 5 && !success; i--) {
    const std::string &line = get_line_data(i).get_data();
    success =
        t3_highlight_detect(line.data(), line.size(), false, T3_HIGHLIGHT_UTF8, &lang, nullptr);
  }
  if (!success) {
    const std::string &line = get_line_data(0).get_data();
    success =
        t3_highlight_detect(line.data(), line.size(), true, T3_HIGHLIGHT_UTF8, &lang, nullptr);
  }
  if (!success) {
    success = t3_highlight_lang_by_filename(name.c_str(), T3_HIGHLIGHT_UTF8, &lang, nullptr);
  }
  if (success) {
    highlight = t3_highlight_load(lang.lang_file, map_highlight, nullptr,
                                  T3_HIGHLIGHT_UTF8 | T3_HIGHLIGHT_USE_PATH, nullptr);
    set_highlight(highlight);
    std::map<std::string, std::string>::iterator iter = option.line_comment_map.find(lang.name);
    if (iter != option.line_comment_map.end()) {
      set_line_comment(iter->second.c_str());
    }
    t3_highlight_free_lang(lang);
  }
  return rw_result_t(rw_result_t::SUCCESS);
}

/* FIXME: try to prevent as many race conditions as possible here. */
rw_result_t file_buffer_t::save(save_as_process_t *state) {
  size_t idx;

  if (state->file != this) {
    PANIC();
  }

  switch (state->state) {
    case save_as_process_t::INITIAL: {
      if (strip_spaces.is_valid() ? strip_spaces.value() : option.strip_spaces) {
        do_strip_spaces();
      }

      transcript_error_t error;
      if (!state->encoding.empty()) {
        if ((state->conversion_handle = transcript_open_converter(
                 state->encoding.c_str(), TRANSCRIPT_UTF8, 0, &error)) == nullptr) {
          return rw_result_t(rw_result_t::CONVERSION_OPEN_ERROR);
        }
        encoding = state->encoding;
      } else if (encoding != "UTF-8") {
        if ((state->conversion_handle = transcript_open_converter(encoding.c_str(), TRANSCRIPT_UTF8,
                                                                  0, &error)) == nullptr) {
          return rw_result_t(rw_result_t::CONVERSION_OPEN_ERROR);
        }
      } else {
        state->conversion_handle = nullptr;
      }
      state->wrapper = t3widget::make_unique<file_write_wrapper_t>(-1, state->conversion_handle);
      state->i = 0;
      state->state = save_as_process_t::OPEN_FILE;
    }
      // FALLTHROUGH
    case save_as_process_t::OPEN_FILE: {
      if (state->conversion_handle) {
        transcript_from_unicode_reset(state->conversion_handle);
      }
      try {
        for (; state->i < size(); state->i++) {
          if (state->i != 0) {
            state->wrapper->write("\n", 1);
          }
          const std::string &data = get_line_data(state->i).get_data();
          /* At this point the wrapper is initialized with -1 as fd, thus it is not writing
             anything. However, it will catch conversion errors, and this will result in asking
             the user what to do. */
          state->wrapper->write(data.data(), data.size());
        }
      } catch (rw_result_t error) {
        return error;
      }
      state->computed_length = state->wrapper->written_size();

      if (state->name.empty()) {
        if (name.empty()) {
          // FIXME: this should return some result instead of crashing!
          PANIC();
        }
        state->save_name = name.c_str();
      } else {
        state->save_name = state->name.c_str();
      }

      state->real_name = canonicalize_path(state->save_name);
      if (state->real_name.empty()) {
        if (errno != ENOENT) {
          return rw_result_t(rw_result_t::ERRNO_ERROR_FILE_UNTOUCHED, errno);
        }
        state->real_name = state->save_name;
      }

      /* This attempts to avoid race conditions, by trying to open with O_CREAT|O_EXCL. This is
         known to have issues on NFSv3, but it's the best we can do. */
      if ((state->fd = open(state->real_name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666)) < 0) {
        struct stat file_info;
        if ((state->fd = open(state->real_name.c_str(), O_RDWR)) >= 0) {
          state->state = save_as_process_t::CREATE_BACKUP;
          if (!state->name.empty()) {
            return rw_result_t(rw_result_t::FILE_EXISTS);
          }
        } else if (stat(state->real_name.c_str(), &file_info) == 0 &&
                   file_info.st_uid == geteuid() && (file_info.st_mode & S_IRUSR) &&
                   !(file_info.st_mode & S_IWUSR)) {
          state->state = save_as_process_t::CHANGE_MODE;
          state->original_mode = file_info.st_mode & 07777;
          return rw_result_t(rw_result_t::READ_ONLY_FILE);
        } else {
          return rw_result_t(rw_result_t::ERRNO_ERROR_FILE_UNTOUCHED, errno);
        }
      }
    }
      if (0) {
          /* The code here is only reachable through the case label. This is an ugly hack to prevent
             executing this code in the normal path, but allow continuing execution after this block
             once the file mode is changed. */
        case save_as_process_t::CHANGE_MODE:
          if (chmod(state->real_name.c_str(), state->original_mode.value() | S_IWUSR)) {
            return rw_result_t(rw_result_t::ERRNO_ERROR_FILE_UNTOUCHED);
          }
          if ((state->fd = open(state->real_name.c_str(), O_RDWR)) < 0) {
            int saved_errno = errno;
            chmod(state->real_name.c_str(), state->original_mode.value());
            return rw_result_t(rw_result_t::ERRNO_ERROR_FILE_UNTOUCHED, saved_errno);
          }
      }
      // FALLTHROUGH
    case save_as_process_t::CREATE_BACKUP: {
      // If the creation of the backup file fails, the user either aborts or allows continuation
      // without completing the backup. Thus the next state is always WRITING.
      state->state = save_as_process_t::WRITING;
      std::string temp_name_str = state->real_name;

      if (option.make_backup) {
        temp_name_str += "~";
        if ((state->backup_fd = open(temp_name_str.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0600)) <
            0) {
          return rw_result_t(rw_result_t::BACKUP_FAILED, errno);
        }
      } else {
        if ((idx = temp_name_str.rfind('/')) == std::string::npos) {
          idx = 0;
        } else {
          idx++;
        }

        temp_name_str.erase(idx);
        temp_name_str.append(".tildeXXXXXX");

        /* Unfortunately, we can't pass the c_str result to mkstemp as we are not allowed to change
           that string. So we'll just have to copy it into a vector :-( */
        std::vector<char> temp_name(temp_name_str.begin(), temp_name_str.end());
        // Ensure nul termination.
        temp_name.push_back(0);
        if ((state->backup_fd = mkstemp(temp_name.data())) >= 0) {
          state->temp_name = temp_name.data();
        } else {
          return rw_result_t(errno == ENOSPC ? rw_result_t::ERRNO_ERROR_FILE_UNTOUCHED
                                             : rw_result_t::BACKUP_FAILED,
                             errno);
        }
      }
      int error = copy_file(state->fd, state->backup_fd);
      if (error != 0) {
        return rw_result_t(
            errno == ENOSPC ? rw_result_t::ERRNO_ERROR_FILE_UNTOUCHED : rw_result_t::BACKUP_FAILED,
            error);
      }
      if (fsync(state->backup_fd) < 0 || close(state->backup_fd) < 0) {
        return rw_result_t(
            errno == ENOSPC ? rw_result_t::ERRNO_ERROR_FILE_UNTOUCHED : rw_result_t::BACKUP_FAILED,
            errno);
      }
      state->backup_saved = true;
      state->backup_fd = -1;
    }
      // FALLTHROUGH
    case save_as_process_t::WRITING: {
#ifdef HAS_POSIX_FALLOCATE
      // Use posix_fallocate to attempt to pre-allocate the required size of the file. If the call
      // fails with ENOSPC or EFBIG, stop writing and report an error to the user. All other error
      // codes are ignored.
      if (posix_fallocate(state->fd, 0, state->computed_length) < 0 &&
          (errno == ENOSPC || errno == EFBIG)) {
        // We want the backup to be removed (if it exists), and we didn't change anything, so we
        // close the file here and set the fd to -1.
        close(state->fd);
        state->fd = -1;
        return rw_result_t(rw_result_t::ERRNO_ERROR_FILE_UNTOUCHED);
      }
#endif
      int conversion_flags = state->wrapper->conversion_flags();
      state->wrapper =
          t3widget::make_unique<file_write_wrapper_t>(state->fd, state->conversion_handle);
      state->wrapper->add_conversion_flags(conversion_flags);
      state->i = 0;
      if (lseek(state->fd, 0, SEEK_SET) < 0) {
        return rw_result_t(rw_result_t::ERRNO_ERROR);
      }
      try {
        for (; state->i < size(); state->i++) {
          if (state->i != 0) {
            state->wrapper->write("\n", 1);
          }
          const std::string &data = get_line_data(state->i).get_data();
          state->wrapper->write(data.data(), data.size());
        }
      } catch (rw_result_t error) {
        // Don't attempt to retry imprecise conversions, as they should have been caught earlier.
        // Also, restarting the conversion may append the current line to an already partially
        // written line.
        if (error == rw_result_t::CONVERSION_IMPRECISE) {
          return rw_result_t(rw_result_t::CONVERSION_ERROR);
        }
        return error;
      }

      // Truncate it to the written size.
      int result;
      while ((result = ftruncate(state->fd, state->wrapper->written_size())) < 0 &&
             errno == EINTR) {
      }
      if (result < 0) {
        return rw_result_t(rw_result_t::ERRNO_ERROR);
      }
      if (fsync(state->fd) < 0) {
        return rw_result_t(rw_result_t::ERRNO_ERROR);
      }
      /* Perform fchmod instead of chmod on the file name, to ensure that we actually change the
         mode on the file we are interested in. However, we only want to report a problem after
         cleaning up the rest, as it is more of an advisory nature. */
      int fchmod_errno = 0;
      if (state->original_mode.is_valid() && fchmod(state->fd, state->original_mode.value()) < 0) {
        fchmod_errno = errno;
      }
      state->original_mode.reset();
      ;
      if (close(state->fd) < 0) {
        return rw_result_t(rw_result_t::ERRNO_ERROR);
      }
      state->fd = -1;

      if (!state->name.empty()) {
        name = state->name;
        std::string converted_name = convert_lang_codeset(name, true);
        name_line.set_text(converted_name);
      }
      set_undo_mark();
      if (fchmod_errno != 0) {
        return rw_result_t(rw_result_t::MODE_RESET_FAILED, fchmod_errno);
      }
      break;
    }
    default:
      PANIC();
  }
  return rw_result_t(rw_result_t::SUCCESS);
}

const std::string &file_buffer_t::get_name() const { return name; }

const char *file_buffer_t::get_encoding() const { return encoding.c_str(); }

text_line_t *file_buffer_t::get_name_line() { return &name_line; }

const edit_window_t::view_parameters_t *file_buffer_t::get_view_parameters() const {
  return view_parameters.get();
}

void file_buffer_t::prepare_paint_line(text_pos_t line) {
  text_pos_t i;

  if (highlight_info == nullptr || highlight_valid >= line) {
    return;
  }

  for (i = highlight_valid >= 0 ? highlight_valid + 1 : 1; i <= line; i++) {
    int state = static_cast<file_line_t *>(get_mutable_line_data(i - 1))->get_highlight_end();
    static_cast<file_line_t *>(get_mutable_line_data(i))->set_highlight_start(state);
  }
  highlight_valid = line;
  match_line = nullptr;
}

void file_buffer_t::set_has_window(bool _has_window) { has_window = _has_window; }

bool file_buffer_t::get_has_window() const { return has_window; }

void file_buffer_t::invalidate_highlight(rewrap_type_t type, text_pos_t line, text_pos_t pos) {
  (void)type;
  (void)pos;
  if (line <= highlight_valid) {
    highlight_valid = line - 1;
  }
}

t3_highlight_t *file_buffer_t::get_highlight() { return highlight_info; }

void file_buffer_t::set_highlight(t3_highlight_t *highlight) {
  if (highlight_info != nullptr) {
    t3_highlight_free(highlight_info);
  }
  highlight_info = highlight;

  if (last_match != nullptr) {
    t3_highlight_free_match(last_match);
    last_match = nullptr;
  }

  match_line = nullptr;
  highlight_valid = 0;

  if (highlight_info != nullptr) {
    last_match = t3_highlight_new_match(highlight_info);
  }
}

bool file_buffer_t::get_strip_spaces() const {
  if (strip_spaces.is_valid()) {
    return strip_spaces.value();
  }
  return option.strip_spaces;
}

void file_buffer_t::set_strip_spaces(bool _strip_spaces) { strip_spaces = _strip_spaces; }

void file_buffer_t::do_strip_spaces() {
  size_t idx, strip_start;
  bool undo_started = false;
  const text_coordinate_t saved_cursor = get_cursor();

  /*FIXME: a better way to do this would be to store the stripped spaces for
     all lines in a single string, delimited by single non-space bytes. If the
     length of the string equals the number of lines, then just don't store
     it. Care has to be taken to maintain the correct cursor position, and
     we have to implement undo/redo separately. */

  for (text_pos_t i = 0; i < size(); i++) {
    const text_line_t &line = get_line_data(i);
    const std::string &str = line.get_data();
    const char *data = str.data();
    strip_start = str.size();
    for (idx = strip_start; idx > 0; idx--) {
      if ((data[idx - 1] & 0xC0) == 0x80) {
        continue;
      }

      if (!line.is_space(idx - 1)) {
        break;
      }

      strip_start = idx - 1;
    }

    if (strip_start != str.size()) {
      text_coordinate_t start, end;
      if (!undo_started) {
        start_undo_block();
        undo_started = true;
      }
      start.line = end.line = i;
      start.pos = strip_start;
      end.pos = str.size();
      delete_block(start, end);

      const text_coordinate_t cursor = get_cursor();
      if (cursor.line == i && static_cast<size_t>(cursor.pos) > strip_start) {
        set_cursor_pos(strip_start);
      }
    }
  }

  if (undo_started) {
    end_undo_block();
  }

  set_cursor(saved_cursor);
  if (saved_cursor.pos > get_line_size(saved_cursor.line)) {
    set_cursor_pos(get_line_size(saved_cursor.line));
  }
}

bool file_buffer_t::find_matching_brace(text_coordinate_t &match_location) {
  const text_coordinate_t cursor = get_cursor();
  file_line_t *line = static_cast<file_line_t *>(get_mutable_line_data(cursor.line));
  char c = line->get_data()[cursor.pos];
  char c_close, check_c;
  bool forward;
  int count;

  /* In the ASCII/Unicode table, the parenthesis are next to each other but
     the other braces are two apart. This is handled by using a switch with a
     fallthrough. */
  c_close = c;
  switch (c) {
    case '}':
    case ']':
      c_close--;
    /* FALLTHROUGH */
    case ')':
      c_close--;
      forward = false;
      break;
    case '{':
    case '[':
      c_close++;
    /* FALLTHROUGH */
    case '(':
      c_close++;
      forward = true;
      break;
    default:
      return false;
  }

  prepare_paint_line(cursor.line);
  /* If the current character is highlighted, it is not considered for brace matching. */
  if (line->get_highlight_idx(cursor.pos) > 0) {
    return false;
  }

  if (forward) {
    text_pos_t current_line = cursor.line;
    text_pos_t i = cursor.pos;
    count = 0;
    /* Use goto here to jump into the loop. The reason for doing this, is
       because we want to start from the location of the cursor, not the
       first character on the line. */
    goto start_search;

    for (; current_line < size(); current_line++) {
      line = static_cast<file_line_t *>(get_mutable_line_data(current_line));
      prepare_paint_line(current_line);
      for (i = 0; i < line->size(); i = line->adjust_position(i, 1)) {
      start_search:
        check_c = line->get_data()[i];
        if ((check_c != c && check_c != c_close) || line->get_highlight_idx(i) > 0) {
          continue;
        }

        if (check_c == c) {
          count++;
        } else {
          if (--count == 0) {
            match_location.pos = i;
            match_location.line = current_line;
            return true;
          }
        }
      }
    }
  } else {
    int open_surplus = 0;
    text_pos_t match_max, marked_pos = -1;
    text_pos_t current_line = cursor.line;
    count = -1;

    /* Finding the closing brace would be easy, if not for the fact that the
       syntax highlighting can only scan forward. If we were to use the simple
       method of simply searching backward, we could hit some seriously nasty
       highlighting cases, due to having to rescan the line for syntax
       highlighting many times. Thus we use a forward pass over the data, with
       a somewhat complicated algorithm to find the correct brace.

       To find the correct opening brace, we keep two separate counts:
       - the count of opening vs. closing braces
       - the surplus of opening braces before the closing brace
       If the open surplus at the closing brace is greater than 0, the
       opening brace is on this line (because we count up to, but not
       including, the closing brace here).
    */
    for (text_pos_t i = 0; i < cursor.pos; i = line->adjust_position(i, 1)) {
      check_c = line->get_data()[i];
      if ((check_c != c && check_c != c_close) || line->get_highlight_idx(i) > 0) {
        continue;
      }

      if (check_c == c) {
        count--;
        if (open_surplus > 0) {
          open_surplus--;
        }
      } else {
        count++;
        open_surplus++;
      }
    }

    if (open_surplus == 0) {
      /* In this case, the opening brace is on a line before the current line.
         So we have to go back, until we find the line which does have the matching
         brace. We now calculate the open_surplus at the end of the line. If it
         is greater than or equal to the number of closing braces we have
         encountered (including the one we are hoping to match), the opening
         brace we are looking for is on the current line. */
      int local_count;

      for (current_line--; current_line >= 0; current_line--) {
        line = static_cast<file_line_t *>(get_mutable_line_data(current_line));
        /* No need to call prepare_paint_line here because we're going backwards. */
        text_pos_t i;
        for (i = 0, local_count = 0, open_surplus = 0; i < line->size(); i++) {
          check_c = line->get_data()[i];
          if ((check_c != c && check_c != c_close) || line->get_highlight_idx(i) > 0) {
            continue;
          }

          if (check_c == c) {
            local_count--;
            if (open_surplus > 0) {
              open_surplus--;
            }
          } else {
            local_count++;
            open_surplus++;
          }
        }

        if (open_surplus >= -count) {
          count += local_count;
          break;
        }
        count += local_count;
      }
      if (current_line < 0) {
        return false;
      }
      match_max = line->size();
    } else {
      match_max = cursor.pos;
    }

    /* At this point the count variable contains the open vs. close brace
       count at the _start_ of the current line, counting back from the
       closing brace we want to find a match for. To make sure we do the
       counting the same way (i.e. closing braces decrease the count, open
       braces increase), we now invert the count, such that when we
       reach a zero count we are at the level where the matching brace is.

       Then we need the last opening brace with a zero count before either
       the closing brace, or the end of the line if the open brace is on a
       different line.
    */
    count = -count;
    for (text_pos_t i = 0; i < match_max; i = line->adjust_position(i, 1)) {
      check_c = line->get_data()[i];
      if ((check_c != c && check_c != c_close) || line->get_highlight_idx(i) > 0) {
        continue;
      }

      if (check_c == c) {
        count--;
      } else {
        if (count++ == 0) {
          marked_pos = i;
        }
      }
    }
    /* As a safety measure, we ensure that last_match has actually been set. */
    if (marked_pos >= 0) {
      match_location.line = current_line;
      match_location.pos = marked_pos;
      return true;
    }
  }

  return false;
}

bool file_buffer_t::goto_matching_brace() {
  text_coordinate_t match_coordinate;
  if (find_matching_brace(match_coordinate)) {
    set_cursor(match_coordinate);
    return true;
  }
  return false;
}

bool file_buffer_t::update_matching_brace() {
  bool old_valid = matching_brace_valid;
  text_coordinate_t old_coordinate = matching_brace_coordinate;

  matching_brace_valid = find_matching_brace(matching_brace_coordinate);

  return old_valid != matching_brace_valid ||
         (old_valid && old_coordinate != matching_brace_coordinate);
}

void file_buffer_t::set_line_comment(const char *text) {
  if (text == nullptr) {
    line_comment.clear();
  } else {
    line_comment = text;
  }
}

text_pos_t starts_with_comment(const std::string &text, const std::string &line_comment) {
  size_t i;
  for (i = 0; i < text.size(); i++) {
    if (text.at(i) != ' ' && text.at(i) != '\t') {
      break;
    }
  }
  if (text.compare(i, line_comment.size(), line_comment) == 0) {
    return i;
  }
  return -1;
}

void file_buffer_t::toggle_line_comment() {
  if (line_comment.empty()) {
    return;
  }

  if (get_selection_mode() == selection_mode_t::NONE) {
    const text_coordinate_t saved_cursor = get_cursor();
    text_pos_t comment_start =
        starts_with_comment(get_line_data(saved_cursor.line).get_data(), line_comment);
    if (comment_start >= 0) {
      delete_block(text_coordinate_t(saved_cursor.line, comment_start),
                   text_coordinate_t(saved_cursor.line, comment_start + line_comment.size()));
      text_pos_t new_pos = saved_cursor.pos;
      if (comment_start < new_pos) {
        new_pos -= std::min<text_pos_t>(line_comment.size(), new_pos - comment_start);
      }
      set_cursor_pos(new_pos);
    } else {
      // FIXME: this causes the cursor position to be recorded incorrectly in the undo
      // information. although one could argue this is to some extent better as it shows the
      // actual edit.
      set_cursor_pos(0);
      insert_block(line_comment);
      set_cursor_pos(saved_cursor.pos + line_comment.size());
    }
  } else {
    text_coordinate_t selection_start = get_selection_start();
    text_coordinate_t selection_end = get_selection_end();
    text_pos_t first_line = std::min(selection_start.line, selection_end.line);
    text_pos_t last_line = std::max(selection_start.line, selection_end.line);
    text_pos_t i;
    for (i = first_line; i <= last_line; i++) {
      text_pos_t comment_start = starts_with_comment(get_line_data(i).get_data(), line_comment);
      if (comment_start < 0) {
        break;
      }
    }
    selection_mode_t old_mode = get_selection_mode();
    // FIXME: The code below contains some hideous hacks to make sure the cursor positioning
    // for undos is as expected. Ideally we'd just call start_undo_block here.
    if (i > last_line) {
      for (i = first_line; i <= last_line; i++) {
        text_pos_t comment_start = starts_with_comment(get_line_data(i).get_data(), line_comment);
        if (i == selection_start.line && comment_start < selection_start.pos) {
          selection_start.pos -=
              std::min<text_pos_t>(line_comment.size(), selection_start.pos - comment_start);
        }
        if (i == selection_end.line && comment_start < selection_end.pos) {
          selection_end.pos -=
              std::min<text_pos_t>(line_comment.size(), selection_end.pos - comment_start);
        }
        if (i == first_line) {
          set_cursor({i, comment_start + static_cast<text_pos_t>(line_comment.size())});
          start_undo_block();
        }
        delete_block(text_coordinate_t(i, comment_start),
                     text_coordinate_t(i, comment_start + line_comment.size()));
      }
    } else {
      for (i = first_line; i <= last_line; i++) {
        set_cursor({i, 0});
        if (i == first_line) {
          start_undo_block();
        }
        insert_block(line_comment);
      }
      selection_start.pos += line_comment.size();
      selection_end.pos += line_comment.size();
    }
    end_undo_block();
    set_selection_mode(selection_mode_t::NONE);
    set_cursor(selection_start);
    set_selection_mode(old_mode);
    set_cursor(selection_end);
    set_selection_end();
  }
}

const char *file_buffer_t::get_char_under_cursor(size_t *size) const {
  const text_coordinate_t cursor = get_cursor();
  const text_line_t &line = get_line_data(cursor.line);
  if (cursor.pos >= line.size()) {
    *size = 1;
    return "\n";
  }
  int end = line.adjust_position(cursor.pos, 1);
  if (cursor.pos == end) {
    return nullptr;
  }
  *size = end - cursor.pos;
  return line.get_data().data() + cursor.pos;
}
