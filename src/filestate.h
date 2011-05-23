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
#ifndef FILESTATE_H
#define FILESTATE_H
#include <sys/stat.h>
#include <sys/types.h>
#include <transcript.h>
#include <sigc++/sigc++.h>

#include "util.h"
#include "filewrapper.h"

class file_buffer_t;

class rw_result_t {
	public:
		enum stop_reason_t {
			SUCCESS,
			FILE_EXISTS,
			ERRNO_ERROR,
			CONVERSION_OPEN_ERROR,
			CONVERSION_IMPRECISE,
			CONVERSION_ERROR,
			CONVERSION_ILLEGAL,
			CONVERSION_TRUNCATED
		};

	private:
		stop_reason_t reason;
		union {
			int errno_error;
			transcript_error_t transcript_error;
		};

	public:
		rw_result_t(void) : reason(SUCCESS) {}
		rw_result_t(stop_reason_t _reason) : reason(_reason) {}
		rw_result_t(stop_reason_t _reason, int _errno_error) : reason(_reason), errno_error(_errno_error) {}
		rw_result_t(stop_reason_t _reason, transcript_error_t _transcript_error) : reason(_reason), transcript_error(_transcript_error) {}
		int get_errno_error(void) { return errno_error; }
		transcript_error_t get_transcript_error(void) { return transcript_error; }
		operator int (void) const { return (int) reason; }
};

class load_state_t : public continuation_t {
	friend class file_buffer_t;

	private:
		enum {
			INITIAL,
			READING
		} state;

		sigc::slot<void, file_buffer_t *> callback;
		file_buffer_t *file;
		file_read_wrapper_t *wrapper;
		int fd;

	public:
		load_state_t(const char *name, const char *encoding, const sigc::slot<void, file_buffer_t *> &_callback);
		virtual bool operator()(void);
		virtual ~load_state_t(void);
};

class save_state_t : public continuation_t {
	friend class file_buffer_t;

	private:
		enum {
			INITIAL,
			ALLOW_OVERWRITE,
			WRITING
		} state;

		file_buffer_t *file;
		const char *name;
		const char *encoding;
		char *new_name;

		// State for save file_buffer_t::save function
		char *real_name;
		char *temp_name;
		int fd;
		size_t i;
		file_write_wrapper_t *wrapper;
		struct stat file_info;

	public:
		save_state_t(file_buffer_t *_file, const char *_name = NULL, const char *_encoding = NULL);
		virtual bool operator()(void);
		virtual ~save_state_t(void);
};

#endif
