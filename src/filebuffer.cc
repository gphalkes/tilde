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
#ifdef HAS_LIBATTR
#include <attr/libattr.h>
#endif
#ifdef HAS_LIBACL
#include <acl/libacl.h>
#endif

#include "filebuffer.h"
#include "fileline.h"
#include "filestate.h"
#include "log.h"
#include "openfiles.h"
#include "option.h"

#define CREATE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

file_buffer_t::file_buffer_t(const char *_name, const char *_encoding)
    : text_buffer_t(new file_line_factory_t(this)),
      view_parameters(new edit_window_t::view_parameters_t()),
      has_window(false),
      highlight_valid(0),
      highlight_info(nullptr),
      match_line(nullptr),
      last_match(nullptr),
      matching_brace_valid(false) {
  if (_encoding == nullptr || strlen(_encoding) == 0) {
    encoding = "UTF-8";
  } else {
    encoding = _encoding;
  }

  if (_name == nullptr || strlen(_name) == 0) {
    name_line.set_text("(Untitled)");
  } else {
    name = _name;

    std::string converted_name;
    convert_lang_codeset(&name, &converted_name, true);
    name_line.set_text(&converted_name);
  }

  connect_rewrap_required(signals::mem_fun(this, &file_buffer_t::invalidate_highlight));

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
  const std::string *line;
  t3_highlight_t *highlight = nullptr;
  t3_highlight_lang_t lang;
  t3_bool success = t3_false;
  int i;

  if (state->file != this) {
    PANIC();
  }

  switch (state->state) {
    case load_process_t::INITIAL_MISSING_OK:
    case load_process_t::INITIAL: {
      std::string converted_name;
      std::string _name;
      transcript_t *handle;
      transcript_error_t error;

      _name = canonicalize_path(name.c_str());
      if (_name.empty()) {
        if (errno == ENOENT && state->state == load_process_t::INITIAL_MISSING_OK) {
          break;
        }
        return rw_result_t(rw_result_t::ERRNO_ERROR, errno);
      }

      name = _name;
      convert_lang_codeset(&name, &converted_name, true);
      name_line.set_text(&converted_name);

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
                  append_text(state->wrapper->get_buffer() + 3, state->wrapper->get_fill() - 3);
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
            append_text(state->wrapper->get_buffer(), state->wrapper->get_fill());
            state->buffer_used = true;
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
  for (i = 0; i < size() && i < 5 && !success; i++) {
    line = get_line_data(i)->get_data();
    success =
        t3_highlight_detect(line->data(), line->size(), false, T3_HIGHLIGHT_UTF8, &lang, nullptr);
  }
  for (i = size() - 1; i >= 5 && !success; i--) {
    line = get_line_data(i)->get_data();
    success =
        t3_highlight_detect(line->data(), line->size(), false, T3_HIGHLIGHT_UTF8, &lang, nullptr);
  }
  if (!success) {
    line = get_line_data(0)->get_data();
    success =
        t3_highlight_detect(line->data(), line->size(), true, T3_HIGHLIGHT_UTF8, &lang, nullptr);
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
    case save_as_process_t::INITIAL:
      if (state->name.empty()) {
        if (name.empty()) {
          PANIC();
        }
        state->save_name = name.c_str();
      } else {
        state->save_name = state->name.c_str();
      }

      state->real_name = canonicalize_path(state->save_name);
      if (state->real_name.empty()) {
        if (errno != ENOENT) {
          return rw_result_t(rw_result_t::ERRNO_ERROR, errno);
        }
        state->real_name = state->save_name;
      }

      /* FIXME: to avoid race conditions, it is probably better to try open with O_CREAT|O_EXCL
         However, this may cause issues with NFS, which is known to have isues with this. */
      if (stat(state->real_name.c_str(), &state->file_info) < 0) {
        if (errno == ENOENT) {
          if ((state->fd = creat(state->real_name.c_str(), CREATE_MODE)) < 0) {
            return rw_result_t(rw_result_t::ERRNO_ERROR, errno);
          }
        } else {
          return rw_result_t(rw_result_t::ERRNO_ERROR, errno);
        }
      } else {
        state->state = save_as_process_t::ALLOW_OVERWRITE;
        if (!state->name.empty()) {
          return rw_result_t(rw_result_t::FILE_EXISTS);
        }
        /* Note that we have a new case in the middle of the else statement
           here. It is an ugly hack, but it does prevent a lot of code
           duplication and other hacks. */
        case save_as_process_t::ALLOW_OVERWRITE:
          state->state = save_as_process_t::ALLOW_OVERWRITE_READONLY;
          if (access(state->save_name, W_OK) != 0) {
            return rw_result_t(rw_result_t::FILE_EXISTS_READONLY);
          }
        case save_as_process_t::ALLOW_OVERWRITE_READONLY:
          std::string temp_name_str = state->real_name;

          if ((idx = temp_name_str.rfind('/')) == std::string::npos) {
            idx = 0;
          } else {
            idx++;
          }

          temp_name_str.erase(idx);
          try {
            temp_name_str.append(".tildeXXXXXX");
          } catch (std::bad_alloc &ba) {
            return rw_result_t(rw_result_t::ERRNO_ERROR, ENOMEM);
          }

          /* Unfortunately, we can't pass the c_str result to mkstemp as we are not allowed
             to change that string. So we'll just have to copy it into a vector :-( */
          std::vector<char> temp_name(temp_name_str.begin(), temp_name_str.end());
          // Ensure nul termination.
          temp_name.push_back(0);
          /* Attempt to create a temporary file. If this fails, just write the file
             directly. The latter has some risk (e.g. file truncation due to full disk,
             or corruption due to computer crashes), but these are so small that it is
             worth permitting this if we can't create the temporary file. */
          if ((state->fd = mkstemp(temp_name.data())) >= 0) {
            state->temp_name = temp_name.data();
// Preserve ownership and attributes
#ifdef HAS_LIBATTR
            attr_copy_file(state->real_name.c_str(), state->temp_name.c_str(), nullptr, nullptr);
#endif
#ifdef HAS_LIBACL
            perm_copy_file(state->real_name.c_str(), state->temp_name.c_str(), nullptr);
#endif
            fchmod(state->fd, state->file_info.st_mode);
            fchown(state->fd, -1, state->file_info.st_gid);
            fchown(state->fd, state->file_info.st_uid, -1);
          } else {
            state->temp_name.clear();
            if ((state->fd = open(state->real_name.c_str(), O_WRONLY | O_CREAT, CREATE_MODE)) < 0) {
              return rw_result_t(rw_result_t::ERRNO_ERROR, errno);
            }
          }
      }

      {
        transcript_t *handle;
        transcript_error_t error;
        if (!state->encoding.empty()) {
          if ((handle = transcript_open_converter(state->encoding.c_str(), TRANSCRIPT_UTF8, 0,
                                                  &error)) == nullptr) {
            return rw_result_t(rw_result_t::CONVERSION_OPEN_ERROR);
          }
          encoding = state->encoding;
        } else if (encoding != "UTF-8") {
          if ((handle = transcript_open_converter(encoding.c_str(), TRANSCRIPT_UTF8, 0, &error)) ==
              nullptr) {
            return rw_result_t(rw_result_t::CONVERSION_OPEN_ERROR);
          }
        } else {
          handle = nullptr;
        }
        // FIXME: if the new fails, the handle will remain open!
        // FIXME: if the new fails, this will abort with an exception! This should not happen!
        state->wrapper = new file_write_wrapper_t(state->fd, handle);
        state->i = 0;
        state->state = save_as_process_t::WRITING;
      }
    case save_as_process_t::WRITING: {
      if (strip_spaces.is_valid() ? strip_spaces() : option.strip_spaces) {
        do_strip_spaces();
      }
      try {
        for (; state->i < size(); state->i++) {
          const std::string *data;
          if (state->i != 0) {
            state->wrapper->write("\n", 1);
          }
          data = get_line_data(state->i)->get_data();
          state->wrapper->write(data->data(), data->size());
        }
      } catch (rw_result_t error) {
        return error;
      }

      // If the file is being overwritten, truncate it to the written size.
      if (state->temp_name.empty()) {
        off_t curr_pos = lseek(state->fd, 0, SEEK_CUR);
        if (curr_pos >= 0) {
          ftruncate(state->fd, curr_pos);
        }
      }
      fsync(state->fd);
      close(state->fd);
      state->fd = -1;
      if (!state->temp_name.empty()) {
        if (option.make_backup) {
          std::string backup_name = state->real_name;
          backup_name += '~';
          unlink(backup_name.c_str());
          link(state->real_name.c_str(), backup_name.c_str());
        }
        if (rename(state->temp_name.c_str(), state->real_name.c_str()) < 0) {
          return rw_result_t(rw_result_t::ERRNO_ERROR, errno);
        }
      }

      if (!state->name.empty()) {
        std::string converted_name;
        name = state->name;
        convert_lang_codeset(&name, &converted_name, true);
        name_line.set_text(&converted_name);
      }
      set_undo_mark();
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

void file_buffer_t::prepare_paint_line(int line) {
  int i;

  if (highlight_info == nullptr || highlight_valid >= line) {
    return;
  }

  for (i = highlight_valid >= 0 ? highlight_valid + 1 : 1; i <= line; i++) {
    int state = static_cast<file_line_t *>(get_line_data_nonconst(i - 1))->get_highlight_end();
    static_cast<file_line_t *>(get_line_data_nonconst(i))->set_highlight_start(state);
  }
  highlight_valid = line;
  match_line = nullptr;
}

void file_buffer_t::set_has_window(bool _has_window) { has_window = _has_window; }

bool file_buffer_t::get_has_window() const { return has_window; }

void file_buffer_t::invalidate_highlight(rewrap_type_t type, int line, int pos) {
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
    return strip_spaces;
  }
  return option.strip_spaces;
}

void file_buffer_t::set_strip_spaces(bool _strip_spaces) { strip_spaces = _strip_spaces; }

void file_buffer_t::do_strip_spaces() {
  size_t idx, strip_start;
  bool undo_started = false;
  text_coordinate_t saved_cursor = cursor;
  int i;

  /*FIXME: a better way to do this would be to store the stripped spaces for
     all lines in a single string, delimited by single non-space bytes. If the
     length of the string equals the number of lines, then just don't store
     it. Care has to be taken to maintain the correct cursor position, and
     we have to implement undo/redo separately. */

  for (i = 0; i < size(); i++) {
    const text_line_t *line = get_line_data(i);
    const std::string *str = line->get_data();
    const char *data = str->data();
    strip_start = str->size();
    for (idx = strip_start; idx > 0; idx--) {
      if ((data[idx - 1] & 0xC0) == 0x80) {
        continue;
      }

      if (!line->is_space(idx - 1)) {
        break;
      }

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

      if (cursor.line == i && static_cast<size_t>(cursor.pos) > strip_start) {
        cursor.pos = strip_start;
      }
    }
  }

  if (undo_started) {
    end_undo_block();
  }

  cursor = saved_cursor;
  if (cursor.pos > get_line_max(cursor.line)) {
    cursor.pos = get_line_max(cursor.line);
  }
}

bool file_buffer_t::find_matching_brace(text_coordinate_t &match_location) {
  file_line_t *line = static_cast<file_line_t *>(get_mutable_line_data(cursor.line));
  char c = (*(line->get_data()))[cursor.pos];
  char c_close, check_c;
  bool forward;
  int current_line, i, count;

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
    current_line = cursor.line;
    i = cursor.pos;
    count = 0;
    /* Use goto here to jump into the loop. The reason for doing this, is
       because we want to start from the location of the cursor, not the
       first character on the line. */
    goto start_search;

    for (; current_line < size(); current_line++) {
      line = static_cast<file_line_t *>(get_mutable_line_data(current_line));
      prepare_paint_line(current_line);
      for (i = 0; i < line->get_length(); i = line->adjust_position(i, 1)) {
      start_search:
        check_c = (*(line->get_data()))[i];
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
    int match_max, marked_pos = -1;
    current_line = cursor.line;
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
    for (i = 0; i < cursor.pos; i = line->adjust_position(i, 1)) {
      check_c = (*(line->get_data()))[i];
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
        for (i = 0, local_count = 0, open_surplus = 0; i < line->get_length(); i++) {
          check_c = (*(line->get_data()))[i];
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
      match_max = line->get_length();
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
    for (i = 0; i < match_max; i = line->adjust_position(i, 1)) {
      check_c = (*(line->get_data()))[i];
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
    cursor = match_coordinate;
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

int starts_with_comment(const std::string *text, const std::string *line_comment) {
  size_t i;
  for (i = 0; i < text->size(); i++) {
    if (text->at(i) != ' ' && text->at(i) != '\t') {
      break;
    }
  }
  if (text->compare(i, line_comment->size(), *line_comment) == 0) {
    return i;
  }
  return -1;
}

void file_buffer_t::toggle_line_comment() {
  if (line_comment.empty()) {
    return;
  }

  if (get_selection_mode() == selection_mode_t::NONE) {
    const std::string *text = get_line_data(cursor.line)->get_data();
    int comment_start = starts_with_comment(text, &line_comment);
    if (comment_start >= 0) {
      text_coordinate_t saved_cursor = cursor;
      delete_block(text_coordinate_t(cursor.line, comment_start),
                   text_coordinate_t(cursor.line, comment_start + line_comment.size()));
      if (comment_start < saved_cursor.pos) {
        saved_cursor.pos -= std::min<int>(line_comment.size(), saved_cursor.pos - comment_start);
      }
      cursor.pos = saved_cursor.pos;
    } else {
      text_coordinate_t saved_cursor = cursor;
      // FIXME: this causes the cursor position to be recorded incorrectly in the undo information.
      // although one could argue this is to some extent better as it shows the actual edit.
      cursor.pos = 0;
      insert_block(&line_comment);
      cursor.pos = saved_cursor.pos + line_comment.size();
    }
  } else {
    text_coordinate_t selection_start = get_selection_start();
    text_coordinate_t selection_end = get_selection_end();
    int first_line = std::min(selection_start.line, selection_end.line);
    int last_line = std::max(selection_start.line, selection_end.line);
    int i;
    for (i = first_line; i <= last_line; i++) {
      const std::string *text = get_line_data(i)->get_data();
      int comment_start = starts_with_comment(text, &line_comment);
      if (comment_start < 0) {
        break;
      }
    }
    selection_mode_t old_mode = get_selection_mode();
    // FIXME: The code below contains some hideous hacks to make sure the cursor positioning
    // for undos is as expected. Ideally we'd just call start_undo_block here.
    if (i > last_line) {
      for (i = first_line; i <= last_line; i++) {
        const std::string *text = get_line_data(i)->get_data();
        int comment_start = starts_with_comment(text, &line_comment);
        if (i == selection_start.line && comment_start < selection_start.pos) {
          selection_start.pos -=
              std::min<int>(line_comment.size(), selection_start.pos - comment_start);
        }
        if (i == selection_end.line && comment_start < selection_end.pos) {
          selection_end.pos -=
              std::min<int>(line_comment.size(), selection_end.pos - comment_start);
        }
        if (i == first_line) {
          cursor.line = i;
          cursor.pos = comment_start + line_comment.size();
          start_undo_block();
        }
        delete_block(text_coordinate_t(i, comment_start),
                     text_coordinate_t(i, comment_start + line_comment.size()));
      }
    } else {
      for (i = first_line; i <= last_line; i++) {
        cursor.line = i;
        cursor.pos = 0;
        if (i == first_line) {
          start_undo_block();
        }
        insert_block(&line_comment);
      }
      selection_start.pos += line_comment.size();
      selection_end.pos += line_comment.size();
    }
    end_undo_block();
    set_selection_mode(selection_mode_t::NONE);
    cursor = selection_start;
    set_selection_mode(old_mode);
    cursor = selection_end;
    set_selection_end();
  }
}
