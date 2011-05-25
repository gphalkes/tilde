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
#ifndef UTIL_H
#define UTIL_H

#include <limits.h>
#include <string>

#ifdef __GNUC__
void fatal(const char *fmt, ...) __attribute__((format (printf, 1, 2))) __attribute__((noreturn));
#else
/*@noreturn@*/ void fatal(const char *fmt, ...);
#endif

#include <stdlib.h>
//FIXME: once gettext is up and running, use localized error message
//~ #define PANIC() fatal(_("Program logic error at %s:%d\n"), __FILE__, __LINE__)
#define PANIC() fatal(("Program logic error at %s:%d\n"), __FILE__, __LINE__)
#ifdef DEBUG
#define ASSERT(_condition) do { if (!(_condition)) { fprintf(stderr, "Assertion failure (%s) on %s:%d\n", #_condition, __FILE__, __LINE__); abort(); } } while(0)
#else
#define ASSERT(_condition) do { if (!(_condition)) PANIC(); } while(0)
#endif


#define ENUM(_name, ...) \
class _name { \
	public: \
		enum _values { \
			__VA_ARGS__ \
		}; \
		_name(void) {} \
		_name(_values _value_arg) : _value(_value_arg) {} \
		_values operator =(_values _value_arg) { _value = _value_arg; return _value; } \
		operator int (void) const { return (int) _value; } \
		bool operator == (_values _value_arg) const { return _value == _value_arg; } \
	private: \
		_values _value; \
}

class version_t {
	private:
		int value;
	public:
		version_t(void) : value(INT_MIN) {}
		int operator++(int) {
			if (value == INT_MAX)
				value = INT_MIN;
			else
				value++;
			return value;
		}
		operator int (void) const { return value; }
		bool operator==(int other) { return value == other; }
};

class continuation_t {
	public:
		enum result_t {
			INCOMPLETE,
			COMPLETED,
			ABORTED
		};
		virtual result_t operator()(void) = 0;
		virtual ~continuation_t(void) {};
};

void enable_debugger_on_segfault(const char *_executable);
void set_limits();

char *resolve_links(const char *start_name);
char *canonicalize_path(const char *path);
void printf_into(std::string *message, const char *format, ...);
#endif
