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
#ifndef FILEWRAPPER_H
#define FILEWRAPPER_H

#include <unistd.h>
#include <cerrno>
#include <string>
#include <transcript/transcript.h>

using namespace std;

#define FILE_BUFFER_SIZE 1024
//~ #define FILE_BUFFER_SIZE 102

class buffer_t {
	protected:
		int fill;
		char buffer[FILE_BUFFER_SIZE];

	public:
		buffer_t(void) : fill(0) {}
		virtual ~buffer_t(void) {}
		virtual int get_fill(void) const { return fill; }
		virtual char *get_buffer(void) { return buffer; }
		virtual char operator[](int idx) const { return buffer[idx]; }
		virtual bool fill_buffer(int used) = 0;
};

class read_buffer_t : public buffer_t {
	private:
		int fd;

	public:
		read_buffer_t(int _fd) : fd(_fd) {}
		virtual bool fill_buffer(int used);
};

class transcript_buffer_t : public buffer_t {
	private:
		buffer_t *wrapped_buffer;
		int buffer_index, conversion_flags;
		transcript_t *handle;
		bool at_eof;

	public:
		transcript_buffer_t(buffer_t *_buffer, transcript_t *_handle) : wrapped_buffer(_buffer), buffer_index(0), conversion_flags(0),
			handle(_handle), at_eof(false) {}
		virtual ~transcript_buffer_t(void);
		virtual bool fill_buffer(int used);
};

class file_read_wrapper_t {
	private:
		buffer_t *buffer;
		int buffer_index;
		bool at_eof;
		string *accumulated;

	public:
		file_read_wrapper_t(int fd, transcript_t *handle = NULL);
		~file_read_wrapper_t(void);
		string *read_line(void);
};

class file_write_wrapper_t {
	private:
		int fd, conversion_flags;
		transcript_t *handle;

	public:
		file_write_wrapper_t(int _fd, transcript_t *_handle = NULL) : fd(_fd), conversion_flags(0), handle(_handle) {}
		~file_write_wrapper_t(void);
		void write(const char *buffer, size_t bytes);
};

#endif
