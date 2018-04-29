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
#ifndef FILE_LINE_H
#define FILE_LINE_H

#include <t3widget/textline.h>

#include "filebuffer.h"

class file_line_factory_t;

class file_line_t : public text_line_t {
 protected:
  int highlight_start_state;

 public:
  file_line_t(int buffersize = BUFFERSIZE, file_line_factory_t *_factory = nullptr);
  file_line_t(string_view _buffer, file_line_factory_t *_factory = nullptr);

  void set_highlight_start(int state);
  int get_highlight_end();
  int get_highlight_idx(int i);

 protected:
  t3_attr_t get_base_attr(int i, const paint_info_t &info) override;
};

class file_line_factory_t : public text_line_factory_t {
 private:
  file_buffer_t *file_buffer;

 public:
  file_line_factory_t(file_buffer_t *_file_buffer);
  std::unique_ptr<text_line_t> new_text_line_t(int buffersize = BUFFERSIZE) override;
  std::unique_ptr<text_line_t> new_text_line_t(string_view _buffer) override;
  file_buffer_t *get_file_buffer() const;
};

#endif
