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
#include <cstring>

#include "openfiles.h"
#include "option.h"

open_files_t open_files;

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
	for (vector<file_buffer_t *>::iterator iter = files.begin();
			iter != files.end(); iter++)
	{
		if ((*iter)->get_name() != NULL && strcmp(name, (*iter)->get_name()) == 0)
			return iter;
	}
	return files.end();
}

int open_files_t::get_version(void) { return version; }
open_files_t::iterator open_files_t::begin(void) { return files.begin(); }
open_files_t::iterator open_files_t::end(void) { return files.end(); }
open_files_t::reverse_iterator open_files_t::rbegin(void) { return files.rbegin(); }
open_files_t::reverse_iterator open_files_t::rend(void) { return files.rend(); }
file_buffer_t *open_files_t::operator[](size_t idx) { return files[idx]; }

void open_files_t::erase(file_buffer_t *buffer) {
	for (iterator iter = files.begin(); iter != files.end(); iter++) {
		if (*iter == buffer) {
			files.erase(iter);
			return;
		}
	}
}

file_buffer_t *open_files_t::next_buffer(file_buffer_t *start) {
	iterator current = files.begin(), iter;

	if (start != NULL)
		for (; current != files.end() && *current != start; current++) {}

	for (iter = current; iter != files.end(); iter++) {
		if (!(*iter)->has_window())
			return *iter;
	}

	for (iter = files.begin(); iter != current; iter++) {
		if (!(*iter)->has_window())
			return *iter;
	}
	return start;
}

file_buffer_t *open_files_t::previous_buffer(file_buffer_t *start) {
	reverse_iterator current = files.rbegin(), iter;

	if (start != NULL)
		for (; current != files.rend() && *current != start; current++) {}

	for (iter = current; iter != files.rend(); iter++) {
		if (!(*iter)->has_window())
			return *iter;
	}

	for (iter = files.rbegin(); iter != current; iter++) {
		if (!(*iter)->has_window())
			return *iter;
	}
	return start;
}


#if 0
void recent_files_t::push_front(file_buffer_t *text) {
	if (text->get_name() == NULL)
		return;

	for (deque<char *>::iterator iter = names.begin();
			iter != names.end(); iter++)
	{
		if (strcmp(*iter, text->get_name()) == 0)
			return;
	}

	version++;

	if (names.size() == option.max_recent_files) {
		delete names.back();
		names.pop_back();
	}
	names.push_front(strdup(text->get_name()));
	delete text;
}

void recent_files_t::open(size_t idx) {
	string name = names[idx];
	#warning FIXME: also save encoding such that we can pass it here!!
	//openFile(&name, new UTF8Encoding());
	return;
}

void recent_files_t::erase(const char *name) {
	for (deque<char *>::iterator iter = names.begin();
			iter != names.end(); iter++)
	{
		if (strcmp(*iter, name) == 0) {
			delete *iter;
			names.erase(iter);
			version++;
			break;
		}
	}
}

int recent_files_t::get_version(void) { return version; }
iterator recent_files_t::begin(void) { return names.begin(); }
iterator recent_files_t::end(void) { return names.end(); }
#endif