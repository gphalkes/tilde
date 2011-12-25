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
#ifndef OPENFILES_H
#define OPENFILES_H

#include <vector>
#include <deque>

using namespace std;

#include "util.h"

class file_buffer_t;

class open_files_t {
	private:
		vector<file_buffer_t *> files;
		version_t version;

	public:
		void push_back(file_buffer_t *text);

		typedef vector<file_buffer_t *>::iterator iterator;
		typedef vector<file_buffer_t *>::reverse_iterator reverse_iterator;
		size_t size(void);
		iterator erase(iterator position);
		iterator contains(const char *name);
		int get_version(void);
		iterator begin(void);
		iterator end(void);
		reverse_iterator rbegin(void);
		reverse_iterator rend(void);
		file_buffer_t *operator[](size_t idx);

		void erase(file_buffer_t *buffer);
		file_buffer_t *next_buffer(file_buffer_t *start);
		file_buffer_t *previous_buffer(file_buffer_t *start);

		void cleanup(void);
};

class recent_file_info_t {
	private:
		char *name;
		char *encoding;
	public:
		recent_file_info_t(file_buffer_t *file);
		~recent_file_info_t(void);

		const char *get_name(void) const;
		const char *get_encoding(void) const;
};

class recent_files_t {
	private:
		deque<recent_file_info_t *> names;
		version_t version;

	public:
		void push_front(file_buffer_t *text);
		recent_file_info_t *get_info(size_t idx);
		void erase(recent_file_info_t *info);

		int get_version(void);
		typedef deque<recent_file_info_t *>::iterator iterator;
		iterator begin(void);
		iterator end(void);

		void cleanup(void);
};

extern open_files_t open_files;
extern recent_files_t recent_files;

#endif
