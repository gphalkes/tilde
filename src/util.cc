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
#include <sys/time.h>
#include <sys/resource.h>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <limits.h>
#include <string>
#include <cstdarg>

#include <t3widget/widget.h>
#include "option.h"

#include "static_assert.h"

using namespace std;
using namespace t3_widget;

#ifdef DEBUG
static char debug_buffer[1024];
static const char *executable;
#endif

/* The 'normal' attributes should always be the first in this list. Other code
   depends on that. */
static const char *highlight_names[] = {
	"normal", "comment", "comment-keyword", "keyword", "number", "string",
	"string-escape", "misc", "variable", "error", "addition", "deletion" };

static_assert(ARRAY_SIZE(highlight_names) <= MAX_HIGHLIGHTS);


stepped_process_t::stepped_process_t(void) : result(true) {}
stepped_process_t::stepped_process_t(const callback_t &cb) : done_cb(cb), result(false) {}

void stepped_process_t::run(void) {
	if (step())
		done(result);
}

void stepped_process_t::abort(void) { done(false); }

void stepped_process_t::disconnect(void) {
	for (list<sigc::connection>::iterator iter = connections.begin();
			iter != connections.end(); iter++)
		(*iter).disconnect();
	connections.clear();
}

void stepped_process_t::done(bool _result) {
	result = _result;
	done_cb(this);
	delete this;
}

stepped_process_t::~stepped_process_t() {
	disconnect();
}

bool stepped_process_t::get_result(void) { return result; }

void stepped_process_t::ignore_result(stepped_process_t *process) { (void) process; }


#ifdef DEBUG
static void start_debugger_on_segfault(int sig) {
	struct rlimit vm_limit;
	(void) sig;

	fprintf(stderr, "Handling signal %d\n", sig);

	signal(SIGSEGV, SIG_DFL);
	signal(SIGABRT, SIG_DFL);

	getrlimit(RLIMIT_AS, &vm_limit);
	vm_limit.rlim_cur = vm_limit.rlim_max;
	setrlimit(RLIMIT_AS, &vm_limit);
	sprintf(debug_buffer, "DISPLAY=:0.0 ddd %s %d", executable, getpid());
	system(debug_buffer);
}

void enable_debugger_on_segfault(const char *_executable) {
	executable = strdup_impl(_executable);
	signal(SIGSEGV, start_debugger_on_segfault);
	signal(SIGABRT, start_debugger_on_segfault);
}

void set_limits() {
	int mb = cli_option.vm_limit == 0 ? 250 : cli_option.vm_limit;
	struct rlimit vm_limit;
	printf("Debug mode: Setting VM limit\n");
	getrlimit(RLIMIT_AS, &vm_limit);
	vm_limit.rlim_cur = mb * 1024 * 1024;
	setrlimit(RLIMIT_AS, &vm_limit);
}
#endif

/** Alert the user of a fatal error and quit.
    @param fmt The format string for the message. See fprintf(3) for details.
    @param ... The arguments for printing.
*/
void fatal(const char *fmt, ...) {
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	exit(EXIT_FAILURE);
}

char *resolve_links(const char *start_name) {
	long buffer_max = pathconf("/", _PC_PATH_MAX);

	if (buffer_max < PATH_MAX)
		buffer_max = PATH_MAX;

	char buffer[buffer_max + 1];
	ssize_t retval;

	while ((retval = readlink(start_name, buffer, buffer_max)) > 0) {
		if (retval == buffer_max)
			return NULL;
		buffer[retval] = 0;
		start_name = buffer;
	}
	return strdup_impl(start_name);
}

char *canonicalize_path(const char *path) {
	string result;
	size_t i;

	if (path[0] != '/') {
		result = get_working_directory();
		result += "/";
	}

	result += path;

	for (i = 0; i < result.size(); ) {
		if (result[i] == '/') {
			if (i + 1 == result.size())
				break;
			if (result[i + 1] == '/') {
				// found //
				result.erase(i + 1, 1);
				continue;
			} else if (result[i + 1] == '.') {
				if (i + 2 == result.size()) {
					result.erase(i);
					break;
				}

				if (result[i + 2] == '/') {
					// found /./
					result.erase(i + 1, 2);
					continue;
				} else if (result[i + 2] == '.' && (i + 3 == result.size() || result[i + 3] == '/')) {
					// found /..
					if (i == 0) {
						/* Erase 3 characters after the '/'. If the string is /.., it will simply
						   erase only the dots. This prevents the situation where we erase the
						   leading '/' only to leave an empty string. */
						result.erase(i + 1, 3);
						continue;
					} else {
						size_t last_slash = result.rfind('/', i - 1);
						result.erase(last_slash + 1, i - last_slash + 3);
						i = last_slash;
						continue;
					}
				}
			}
		}
		i++;
	}

	/* TODO:
		- remove all occurences of // and /./
	    - remove all occurences of ^/..
		- remove all occurences of XXX/../
	*/
	return strdup_impl(result.c_str());
}

#define BUFFER_START_SIZE 256
#define BUFFER_MAX 4096
void printf_into(string *message, const char *format, ...) {
	static cleanup_free_ptr<char>::t message_buffer;
	static int message_buffer_size;

	char *new_message_buffer;
	int result;
	va_list args;

	message->clear();
	va_start(args, format);

	if (message_buffer == NULL) {
		if ((message_buffer = (char *) malloc(BUFFER_START_SIZE)) == NULL) {
			va_end(args);
			return;
		}
		message_buffer_size = BUFFER_START_SIZE;
	}

	result = vsnprintf(message_buffer, message_buffer_size, format, args);
	if (result < 0) {
		va_end(args);
		return;
	}

	result = min(BUFFER_MAX - 1, result);
	if (result < message_buffer_size || (new_message_buffer = (char *) realloc(message_buffer, result + 1)) == NULL) {
		*message = message_buffer;
		va_end(args);
		return;
	}

	message_buffer = new_message_buffer;
	message_buffer_size = result + 1;
	result = vsnprintf(message_buffer, message_buffer_size, format, args);
	va_end(args);
	if (result < 0)
		return;

	*message = message_buffer;
	return;
}

int map_highlight(void *data, const char *name) {
	int i;

	(void) data;

	for (i = 0; (size_t) i < ARRAY_SIZE(highlight_names); i++) {
		if (strcmp(name, highlight_names[i]) == 0)
			return i;
	}
	return 0;
}

const char *reverse_map_highlight(int idx) {
	if (idx < 0 || (size_t) idx >= ARRAY_SIZE(highlight_names))
		return NULL;
	return highlight_names[idx];
}

#ifndef HAS_STRDUP
char *strdup_impl(const char *str) {
	char *result;
	size_t len = strlen(str) + 1;

	if ((result = (char *) malloc(len)) == NULL)
		return NULL;
	memcpy(result, str, len);
	return result;
}
#endif
