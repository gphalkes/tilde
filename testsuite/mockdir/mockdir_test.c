/* Copyright (C) 2013 G.P. Halkes
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

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

int main(int argc, char *argv[]) {
	char name_buffer[PATH_MAX];
	struct stat stat_buf;
	int fd;

	(void) argc;
	(void) argv;

	if (getcwd(name_buffer, sizeof(name_buffer)) == NULL)
		fatal("Error in getcwd: %m\n");

	printf("getcwd: %s\n", name_buffer);

	if (realpath("file1", name_buffer) == NULL)
		fatal("Error in realpath: %m\n");

	printf("realpath for file1: %s\n", name_buffer);

	if (stat(name_buffer, &stat_buf) < 0)
		fatal("Error in stat: %m\n");

	if ((fd = open(name_buffer, O_RDONLY)) < 0)
		fatal("Error in open for %s: %m\n", name_buffer);
	close(fd);

	if ((fd = open("/bin/ls", O_RDONLY)) < 0)
		fatal("Error in open for /bin/ls: %m\n");
	close(fd);

	return EXIT_SUCCESS;
}
/* TODO: check the following functions
DEF_LIBC_FUNC(int, stat, const char *path, struct stat *buf);
DEF_LIBC_FUNC(int, rename, const char *oldpath, const char *newpath);
DEF_LIBC_FUNC(int, link, const char *oldpath, const char *newpath);
DEF_LIBC_FUNC(int, unlink, const char *pathname);
DEF_LIBC_FUNC(int, mkstemp, char *template);
DEF_LIBC_FUNC(int, __xstat, int ver, const char *path, struct stat *buf);
DEF_LIBC_FUNC(int, creat, const char *name, mode_t mode);
*/
