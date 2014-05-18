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
#ifndef FILESTATE_H
#define FILESTATE_H
#include <sys/stat.h>
#include <sys/types.h>
#include <transcript/transcript.h>
#include <sigc++/sigc++.h>
#include <t3widget/widget.h>

#include "util.h"
#include "filewrapper.h"
#include "openfiles.h"

using namespace t3_widget;

class file_buffer_t;

class rw_result_t {
	public:
		enum stop_reason_t {
			SUCCESS,
			FILE_EXISTS,
			FILE_EXISTS_READONLY,
			ERRNO_ERROR,
			CONVERSION_OPEN_ERROR,
			CONVERSION_IMPRECISE,
			CONVERSION_ERROR,
			CONVERSION_ILLEGAL,
			CONVERSION_TRUNCATED,
			BOM_FOUND,
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

class load_process_t : public stepped_process_t {
	friend class file_buffer_t;

	protected:
		enum {
			SELECT_FILE,
			INITIAL,
			INITIAL_MISSING_OK,
			READING_FIRST,
			READING
		} state;

		enum {
			UNKNOWN,
			REMOVE_BOM,
			PRESERVE_BOM,
		} bom_state;

		file_buffer_t *file;
		file_read_wrapper_t *wrapper;
		std::string encoding;
		int fd;
		bool buffer_used;

		load_process_t(const callback_t &cb);
		load_process_t(const callback_t &cb, const char *name, const char *_encoding, bool missing_ok);
		void abort(void);
		virtual bool step(void);
		virtual void file_selected(const std::string *name);
		virtual void encoding_selected(const std::string *_encoding);
		virtual ~load_process_t(void);
		void preserve_bom(void);
		void remove_bom(void);

	public:
		virtual file_buffer_t *get_file_buffer(void);
		static void execute(const callback_t &cb);
		static void execute(const callback_t &cb, const char *name, const char *encoding = NULL, bool missing_ok = false);
};

class save_as_process_t : public stepped_process_t {
	friend class file_buffer_t;

	protected:
		enum {
			SELECT_FILE,
			INITIAL,
			ALLOW_OVERWRITE,
			ALLOW_OVERWRITE_READONLY,
			WRITING
		};
		int state;

		file_buffer_t *file;
		std::string name;
		std::string encoding;

		// State for save file_buffer_t::save function
		const char *save_name;
		cleanup_free_ptr<char>::t real_name;
		char *temp_name;
		int fd;
		int i;
		file_write_wrapper_t *wrapper;
		struct stat file_info;

		save_as_process_t(const callback_t &cb, file_buffer_t *_file);
		virtual bool step(void);
		virtual void file_selected(const std::string *_name);
		virtual void encoding_selected(const std::string *_encoding);
		virtual ~save_as_process_t(void);

	public:
		static void execute(const callback_t &cb, file_buffer_t *_file);
};

class save_process_t : public save_as_process_t {
	protected:
		save_process_t(const callback_t &cb, file_buffer_t *_file);
	public:
		static void execute(const callback_t &cb, file_buffer_t *_file);
};

class close_process_t : public save_process_t {
	protected:
		enum {
			CONFIRM_CLOSE = WRITING + 1,
			CLOSE
		};

		close_process_t(const callback_t &cb, file_buffer_t *_file);
		virtual bool step(void);
		virtual void do_save(void);
		virtual void dont_save(void);

	public:
		static void execute(const callback_t &cb, file_buffer_t *_file);
		virtual const file_buffer_t *get_file_buffer_ptr(void);
};

class exit_process_t : public stepped_process_t {
	protected:
		open_files_t::iterator iter;

		exit_process_t(const callback_t &cb);
		virtual bool step(void);
		virtual void do_save(void);
		virtual void dont_save(void);
		virtual void save_done(stepped_process_t *process);

	public:
		static void execute(const callback_t &cb);
};

class open_recent_process_t : public load_process_t	{
	protected:
		recent_file_info_t *info;

		open_recent_process_t(const callback_t &cb);
		~open_recent_process_t(void);
		virtual void recent_file_selected(recent_file_info_t *_info);
		virtual bool step(void);
	public:
		static void execute(const callback_t &cb);
};

class load_cli_file_process_t : public stepped_process_t {
	protected:
		list<const char *>::const_iterator iter;
		bool in_load, in_step, encoding_selected;
		cleanup_free_ptr<char>::t encoding;

		load_cli_file_process_t(const callback_t &cb);
		virtual bool step(void);
		virtual void load_done(stepped_process_t *process);

		void encoding_selection_done(const std::string *);
		void encoding_selection_canceled(void);
	public:
		static void execute(const callback_t &cb);
};

#endif
