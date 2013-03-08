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
#include <cstring>

#include "openfiles.h"
#include "filebuffer.h"
#include "option.h"

open_files_t open_files;
recent_files_t recent_files;

size_t open_files_t::size(void) { return files.size(); }

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
			version++;
			return;
		}
	}
}

file_buffer_t *open_files_t::next_buffer(file_buffer_t *start) {
	iterator current = files.begin(), iter;

	if (start != NULL)
		for (; current != files.end() && *current != start; current++) {}

	for (iter = current; iter != files.end(); iter++) {
		if (!(*iter)->get_has_window())
			return *iter;
	}

	for (iter = files.begin(); iter != current; iter++) {
		if (!(*iter)->get_has_window())
			return *iter;
	}
	return start;
}

file_buffer_t *open_files_t::previous_buffer(file_buffer_t *start) {
	reverse_iterator current = files.rbegin(), iter;

	if (start != NULL)
		for (; current != files.rend() && *current != start; current++) {}

	for (iter = current; iter != files.rend(); iter++) {
		if (!(*iter)->get_has_window())
			return *iter;
	}

	for (iter = files.rbegin(); iter != current; iter++) {
		if (!(*iter)->get_has_window())
			return *iter;
	}
	return start;
}

void open_files_t::cleanup(void) {
	while (!files.empty())
		delete files.front();
}


recent_file_info_t::recent_file_info_t(file_buffer_t *file) {
	if ((name = strdup_impl(file->get_name())) == NULL)
		throw bad_alloc();
	if ((encoding = strdup_impl(file->get_encoding())) == NULL) {
		free(name);
		throw bad_alloc();
	}
}

recent_file_info_t::~recent_file_info_t(void) {
	free(name);
	free(encoding);
}

const char *recent_file_info_t::get_name(void) const {
	return name;
}

const char *recent_file_info_t::get_encoding(void) const {
	return encoding;
}


void recent_files_t::push_front(file_buffer_t *text) {
	if (text->get_name() == NULL)
		return;

	for (deque<recent_file_info_t *>::iterator iter = names.begin();
			iter != names.end(); iter++)
	{
		if (strcmp((*iter)->get_name(), text->get_name()) == 0)
			return;
	}

	version++;

	if (names.size() == option.max_recent_files) {
		delete names.back();
		names.pop_back();
	}
	names.push_front(new recent_file_info_t(text));
}

recent_file_info_t *recent_files_t::get_info(size_t idx) {
	return names[idx];
}

void recent_files_t::erase(recent_file_info_t *info) {
	for (deque<recent_file_info_t *>::iterator iter = names.begin();
			iter != names.end(); iter++)
	{
		if (*iter == info) {
			delete *iter;
			names.erase(iter);
			version++;
			break;
		}
	}
}

int recent_files_t::get_version(void) { return version; }
recent_files_t::iterator recent_files_t::begin(void) { return names.begin(); }
recent_files_t::iterator recent_files_t::end(void) { return names.end(); }

void recent_files_t::cleanup(void) {
	while (!names.empty()) {
		delete names.back();
		names.pop_back();
	}
}
