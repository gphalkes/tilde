/* Copyright (C) 2018 G.P. Halkes
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

#ifndef COPY_FILE_H_
#define COPY_FILE_H_

#include <cstddef>

// Copy file by different methods. The files need not be at the starting position. The postion
// after copy is undefined.
int copy_file_by_sendfile(int src_fd, int dest_fd, size_t bytes_to_copy);
int copy_file_by_copy_file_range(int src_fd, int dest_fd, size_t bytes_to_copy);
int copy_file_by_ficlone(int src_fd, int dest_fd);
int copy_file_by_read_write(int src_fd, int dest_fd);

// Generic copy routine which will try to copy the file using one of the methods above.
int copy_file(int src_fd, int dest_fd);

#endif
