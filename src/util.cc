/* Copyright (C) 2011-2012,2018 G.P. Halkes
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

#include "tilde/option.h"
#include "tilde/static_assert.h"

using namespace t3widget;

#ifdef DEBUG
static char debug_buffer[1024];
static const std::string *executable;
#endif

stepped_process_t::stepped_process_t() : result(true) {}
stepped_process_t::stepped_process_t(const callback_t &cb) : done_cb(cb), result(true) {}

void stepped_process_t::run() {
  in_step = true;
  bool step_result = step();
  in_step = false;
  if (step_result) {
    done();
  }
}

void stepped_process_t::abort() {
  result = false;
  if (!in_step) {
    done();
  }
}

void stepped_process_t::done() {
  cleanup();
  /* Ensure that this object is always deleted. Simply calling delete after the done_cb won't do the
     trick, as it is not guaranteed that done_cb actually returns. */
  std::unique_ptr<stepped_process_t> deleter(this);
  done_cb(this);
}

stepped_process_t::~stepped_process_t() {
  for (t3widget::connection_t &iter : connections) {
    iter.disconnect();
  }
  connections.clear();
}

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
  sprintf(debug_buffer, "DISPLAY=:0.0 ddd %s %d", executable->c_str(), getpid());
  system(debug_buffer);
}

void enable_debugger_on_segfault(const char *_executable) {
  executable = new std::string(_executable);
  signal(SIGSEGV, start_debugger_on_segfault);
  signal(SIGABRT, start_debugger_on_segfault);
}

void set_limits() {
  if (cli_option.vm_limit < 0) {
    return;
  }

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

std::string canonicalize_path(const char *path) {
  char *realpath_result = realpath(path, nullptr);

#ifdef PATH_MAX
  /* realpath in the POSIX.1-2001 and 2004 specifications is broken, in that the size of the buffer
     for the 2nd parameter is ill defined. However, most systems allow a nullptr parameter to do
     automatic allocation. Those that don't typically return EINVAL in that case. We can't work
     around this for all situations, but when PATH_MAX is defined, we can. Any system that does not
     define PATH_MAX and does not do automatic allocation is broken by design, as that leaves no
     correct way to allocate a buffer with the correct size to store the result. */
  if (realpath_result == nullptr && path != nullptr && errno == EINVAL) {
    char store_path[PATH_MAX];
    if (realpath(path, store_path) != nullptr) {
      return "";
    }
    return store_path;
  }
#endif
  if (realpath_result == nullptr) {
    return "";
  }
  std::string result = realpath_result;
  free(realpath_result);
  return result;
}

#define BUFFER_START_SIZE 256
#define BUFFER_MAX 4096
void printf_into(std::string *message, const char *format, ...) {
  static std::vector<char> message_buffer(BUFFER_START_SIZE);

  int result;
  va_list args;

  message->clear();
  va_start(args, format);

  result = vsnprintf(message_buffer.data(), message_buffer.size(), format, args);
  if (result < 0) {
    va_end(args);
    return;
  }

  result = std::min(BUFFER_MAX - 1, result);
  if (result < static_cast<int>(message_buffer.size())) {
    *message = message_buffer.data();
    va_end(args);
    return;
  }

  message_buffer.resize(result + 1);
  result = vsnprintf(message_buffer.data(), message_buffer.size(), format, args);
  va_end(args);
  if (result < 0) {
    return;
  }

  *message = message_buffer.data();
}

int map_highlight(void *, const char *name) {
  return option.highlights.lookup_mapping(name).value_or(0);
}
