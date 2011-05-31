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
};

#if 0
class recent_files_t {
	private:
		deque<char *> names;
		version_t version;

	public:
		void push_front(file_buffer_t *text);
		void open(size_t idx);
		void erase(const char *name);

		int get_version(void);
		typedef deque<char *>::iterator iterator;
		iterator begin(void);
		iterator end(void);
};
#endif

extern open_files_t open_files;

#endif
