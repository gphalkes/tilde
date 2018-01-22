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
#include <cerrno>
#include <climits>
#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>

#include <t3widget/widget.h>

#include "option.h"
#include "static_assert.h"

using namespace t3_widget;

#ifdef DEBUG
static char debug_buffer[1024];
static const char *executable;
#endif

/* The 'normal' attributes should always be the first in this list. Other code
   depends on that. */
static const char *highlight_names[] = {"normal",   "comment", "comment-keyword", "keyword",
                                        "number",   "string",  "string-escape",   "misc",
                                        "variable", "error",   "addition",        "deletion"};

static_assert(ARRAY_SIZE(highlight_names) <= MAX_HIGHLIGHTS);

stepped_process_t::stepped_process_t() : result(true) {}
stepped_process_t::stepped_process_t(const callback_t &cb) : done_cb(cb), result(false) {}

void stepped_process_t::run() {
  if (step()) done(result);
}

void stepped_process_t::abort() { done(false); }

void stepped_process_t::disconnect() {
  for (std::list<signals::connection>::iterator iter = connections.begin();
       iter != connections.end(); iter++)
    (*iter).disconnect();
  connections.clear();
}

void stepped_process_t::done(bool _result) {
  result = _result;
  done_cb(this);
  delete this;
}

stepped_process_t::~stepped_process_t() { disconnect(); }

bool stepped_process_t::get_result() { return result; }

void stepped_process_t::ignore_result(stepped_process_t *process) { (void)process; }

#ifdef DEBUG
static void start_debugger_on_segfault(int sig) {
  struct rlimit vm_limit;
  (void)sig;

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
  if (cli_option.vm_limit < 0) return;

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

char *canonicalize_path(const char *path) {
  char *result = realpath(path, nullptr);

#ifdef PATH_MAX
  /* realpath in the POSIX.1-2001 and 2004 specifications is broken, in that
     the size of the buffer for the 2nd parameter is ill defined. However,
     most systems allow a NULL parameter to do automatic allocation. Those
     that don't typically return EINVAL in that case. We can't work around
     this for all situations, but when PATH_MAX is defined, we can. Any
     system that does not define PATH_MAX and does not do automatic allocation
     is broken by design, as that leaves no correct way to allocate a buffer
     with the correct size to store the result.
  */
  if (result == nullptr && path != nullptr && errno == EINVAL) {
    char store_path[PATH_MAX];
    if (realpath(path, store_path) != nullptr) return nullptr;
    return strdup_impl(store_path);
  }
#endif
  return result;
}

#define BUFFER_START_SIZE 256
#define BUFFER_MAX 4096
void printf_into(std::string *message, const char *format, ...) {
  static cleanup_free_ptr<char>::t message_buffer;
  static int message_buffer_size;

  char *new_message_buffer;
  int result;
  va_list args;

  message->clear();
  va_start(args, format);

  if (message_buffer == nullptr) {
    if ((message_buffer = (char *)malloc(BUFFER_START_SIZE)) == nullptr) {
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

  result = std::min(BUFFER_MAX - 1, result);
  if (result < message_buffer_size ||
      (new_message_buffer = (char *)realloc(message_buffer, result + 1)) == nullptr) {
    *message = message_buffer;
    va_end(args);
    return;
  }

  message_buffer = new_message_buffer;
  message_buffer_size = result + 1;
  result = vsnprintf(message_buffer, message_buffer_size, format, args);
  va_end(args);
  if (result < 0) return;

  *message = message_buffer;
  return;
}

int map_highlight(void *data, const char *name) {
  int i;

  (void)data;

  for (i = 0; (size_t)i < ARRAY_SIZE(highlight_names); i++) {
    if (strcmp(name, highlight_names[i]) == 0) return i;
  }
  return 0;
}

const char *reverse_map_highlight(int idx) {
  if (idx < 0 || (size_t)idx >= ARRAY_SIZE(highlight_names)) return nullptr;
  return highlight_names[idx];
}

#ifndef HAS_STRDUP
char *strdup_impl(const char *str) {
  char *result;
  size_t len = strlen(str) + 1;

  if ((result = (char *)malloc(len)) == NULL) return NULL;
  memcpy(result, str, len);
  return result;
}
#endif
