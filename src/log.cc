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
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include "log.h"


#ifdef DEBUG
static FILE *log_file;

static void close_log(void) {
	fclose(log_file);
}

void init_log(void) {
	if (log_file == NULL) {
		log_file = fopen("log.txt", "a");
		if (log_file)
			atexit(close_log);
	}
}

void lprintf(const char *fmt, ...) {
	if (log_file) {
		va_list args;

		va_start(args, fmt);
		vfprintf(log_file, fmt, args);
		fflush(log_file);
		va_end(args);
	}
}

void ldumpstr(const char *str, int length) {
	for (; length > 0; length--, str++) {
		if ((unsigned int) *str < 32)
			fprintf(log_file, "\\x%02X", *str);
		else if (*str == '\\')
			fprintf(log_file, "\\\\");
		else
			fputc(*str, log_file);
	}
	fflush(log_file);
}

void logkeyseq(const char *keys) {
	size_t i;

	if (!log_file)
		return;

	fprintf(log_file, "Unknown key sequence:");
	for (i = 0; i < strlen(keys); i++)
		fprintf(log_file, " %d", keys[i]);
	fprintf(log_file, "\n");
	fflush(log_file);
}
#endif
