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
#include "tilde/openfiles.h"
#include "tilde/option.h"

open_files_t open_files;
recent_files_t recent_files;

size_t open_files_t::size() const { return files.size(); }
bool open_files_t::empty() const { return files.empty(); }

void open_files_t::push_back(file_buffer_t *text) {
  files.push_back(text);
  version++;
}

open_files_t::iterator open_files_t::erase(open_files_t::iterator position) {
  open_files_t::iterator result = files.erase(position);
  version++;
  return result;
}

open_files_t::iterator open_files_t::contains(const char *name) {
  /* FIXME: this should really check for ino+dev equality to see if the file
     is already open. */
  for (std::vector<file_buffer_t *>::iterator iter = files.begin(); iter != files.end(); iter++) {
    if (!(*iter)->get_name().empty() && (*iter)->get_name() == name) {
      return iter;
    }
  }
  return files.end();
}

int open_files_t::get_version() { return version; }
open_files_t::iterator open_files_t::begin() { return files.begin(); }
open_files_t::iterator open_files_t::end() { return files.end(); }
open_files_t::reverse_iterator open_files_t::rbegin() { return files.rbegin(); }
open_files_t::reverse_iterator open_files_t::rend() { return files.rend(); }
file_buffer_t *open_files_t::operator[](size_t idx) { return files[idx]; }
file_buffer_t *open_files_t::back() { return files.back(); }

void open_files_t::erase(file_buffer_t *buffer) {
  for (iterator iter = files.begin(); iter != files.end(); iter++) {
    if (*iter == buffer) {
      files.erase(iter);
      version++;
      return;
    }
  }
}

file_buffer_t *open_files_t::next_buffer(file_buffer_t *start) {
  iterator current = files.begin(), iter;

  if (start != nullptr) {
    for (; current != files.end() && *current != start; current++) {
    }
  }

  for (iter = current; iter != files.end(); iter++) {
    if (!(*iter)->get_has_window()) {
      return *iter;
    }
  }

  for (iter = files.begin(); iter != current; iter++) {
    if (!(*iter)->get_has_window()) {
      return *iter;
    }
  }
  return start;
}

file_buffer_t *open_files_t::previous_buffer(file_buffer_t *start) {
  reverse_iterator current = files.rbegin(), iter;

  if (start != nullptr) {
    for (; current != files.rend() && *current != start; current++) {
    }
  }

  for (iter = current; iter != files.rend(); iter++) {
    if (!(*iter)->get_has_window()) {
      return *iter;
    }
  }

  for (iter = files.rbegin(); iter != current; iter++) {
    if (!(*iter)->get_has_window()) {
      return *iter;
    }
  }
  return start;
}

void open_files_t::cleanup() {
  while (!files.empty()) {
    delete files.front();
  }
}

recent_file_info_t::recent_file_info_t(file_buffer_t *file)
    : name(file->get_name()), encoding(file->get_encoding()) {}

const std::string &recent_file_info_t::get_name() const { return name; }

const std::string &recent_file_info_t::get_encoding() const { return encoding; }

void recent_files_t::push_front(file_buffer_t *text) {
  if (text->get_name().empty()) {
    return;
  }

  for (recent_file_info_t *name : names) {
    if (name->get_name() == text->get_name()) {
      return;
    }
  }

  version++;

  if (names.size() == option.max_recent_files) {
    delete names.back();
    names.pop_back();
  }
  names.push_front(new recent_file_info_t(text));
}

recent_file_info_t *recent_files_t::get_info(size_t idx) { return names[idx]; }

void recent_files_t::erase(recent_file_info_t *info) {
  for (std::deque<recent_file_info_t *>::iterator iter = names.begin(); iter != names.end();
       iter++) {
    if (*iter == info) {
      delete *iter;
      names.erase(iter);
      version++;
      break;
    }
  }
}

int recent_files_t::get_version() { return version; }
recent_files_t::iterator recent_files_t::begin() { return names.begin(); }
recent_files_t::iterator recent_files_t::end() { return names.end(); }

void recent_files_t::cleanup() {
  while (!names.empty()) {
    delete names.back();
    names.pop_back();
  }
}
