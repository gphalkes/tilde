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

	if (realpath("mockdir_test.c", name_buffer) == NULL)
		fatal("Error in realpath: %m\n");

	printf("realpath for mockdir_test.c: %s\n", name_buffer);

	if (stat(name_buffer, &stat_buf) < 0)
		fatal("Error in stat: %m\n");

	printf("opening %s\n", name_buffer);
	if ((fd = open(name_buffer, O_RDONLY)) < 0)
		fatal("Error in open: %m\n");
	close(fd);

	printf("opening /bin/ls\n");
	if ((fd = open("/bin/ls", O_RDONLY)) < 0)
		fatal("Error in open: %m\n");
	close(fd);

	return EXIT_SUCCESS;
}
/* TODO
DEF_LIBC_FUNC(int, stat, const char *path, struct stat *buf);
DEF_LIBC_FUNC(int, rename, const char *oldpath, const char *newpath);
DEF_LIBC_FUNC(int, link, const char *oldpath, const char *newpath);
DEF_LIBC_FUNC(int, unlink, const char *pathname);
DEF_LIBC_FUNC(int, mkstemp, char *template);
DEF_LIBC_FUNC(int, __xstat, int ver, const char *path, struct stat *buf);
DEF_LIBC_FUNC(int, creat, const char *name, mode_t mode);
*/
