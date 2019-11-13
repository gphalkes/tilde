/* Copyright (C) 2011,2018 G.P. Halkes
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
#ifndef OPENFILES_H
#define OPENFILES_H

#include <deque>
#include <t3widget/util.h>
#include <vector>

#include "tilde/util.h"

using namespace t3widget;

class file_buffer_t;

class open_files_t {
 private:
  std::vector<file_buffer_t *> files;
  version_t version;

 public:
  void push_back(file_buffer_t *text);

  using iterator = std::vector<file_buffer_t *>::iterator;
  using reverse_iterator = std::vector<file_buffer_t *>::reverse_iterator;
  size_t size() const;
  bool empty() const;
  iterator erase(iterator position);
  iterator contains(const char *name);
  int get_version();
  iterator begin();
  iterator end();
  reverse_iterator rbegin();
  reverse_iterator rend();
  file_buffer_t *operator[](size_t idx);
  file_buffer_t *back();

  void erase(file_buffer_t *buffer);
  file_buffer_t *next_buffer(file_buffer_t *start);
  file_buffer_t *previous_buffer(file_buffer_t *start);

  void cleanup();
};

class recent_file_info_t {
 private:
  std::string name;
  std::string encoding;
  text_coordinate_t position;

 public:
  explicit recent_file_info_t(file_buffer_t *file);

  const std::string &get_name() const;
  const std::string &get_encoding() const;
  const text_coordinate_t get_position() const;
};

class recent_files_t {
 private:
  std::deque<recent_file_info_t *> names;
  version_t version;

 public:
  void push_front(file_buffer_t *text);
  recent_file_info_t *get_info(size_t idx);
  void erase(recent_file_info_t *info);

  int get_version();
  using iterator = std::deque<recent_file_info_t *>::iterator;
  iterator begin();
  iterator end();

  void cleanup();
};

extern open_files_t open_files;
extern recent_files_t recent_files;

#endif
