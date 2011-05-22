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
#include "option.h"

static char debug_buffer[1024];
static const char *executable;

static void start_debugger_on_segfault(int sig) {
	struct rlimit vm_limit;
	(void) sig;

	fprintf(stderr, "Handling signal\n");

	signal(SIGSEGV, SIG_DFL);
	signal(SIGABRT, SIG_DFL);

	getrlimit(RLIMIT_AS, &vm_limit);
	vm_limit.rlim_cur = vm_limit.rlim_max;
	setrlimit(RLIMIT_AS, &vm_limit);
	sprintf(debug_buffer, "DISPLAY=:0.0 ddd %s %d", executable, getpid());
	system(debug_buffer);
}

void enable_debugger_on_segfault(const char *_executable) {
	executable = strdup(_executable);
	signal(SIGSEGV, start_debugger_on_segfault);
	signal(SIGABRT, start_debugger_on_segfault);
}

#ifdef DEBUG
void set_limits() {
	int mb = option.vm_limit == 0 ? 250 : option.vm_limit;
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
