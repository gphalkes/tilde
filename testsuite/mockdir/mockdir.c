#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>

static const char mock_dir[] = "/mockdir";
#define DEF_LIBC_FUNC(retval, name, ...) static retval (*real_##name)(__VA_ARGS__)
#define INIT_LIBC_FUNC(name) do { if (real_##name == NULL) real_##name = get_libc_func(#name); } while (0)

DEF_LIBC_FUNC(int, open, const char *name, int flags);
DEF_LIBC_FUNC(char *, getcwd, char *buf, size_t size);
DEF_LIBC_FUNC(char *, realpath, const char *path, char *resolved_path);
DEF_LIBC_FUNC(int, stat, const char *path, struct stat *buf);
DEF_LIBC_FUNC(int, rename, const char *oldpath, const char *newpath);
DEF_LIBC_FUNC(int, link, const char *oldpath, const char *newpath);
DEF_LIBC_FUNC(int, unlink, const char *pathname);
DEF_LIBC_FUNC(int, mkstemp, char *template);
DEF_LIBC_FUNC(int, __xstat, int ver, const char *path, struct stat *buf);
DEF_LIBC_FUNC(int, creat, const char *name, mode_t mode);

/** Alert the user of a fatal error and quit.
    @param fmt The format string for the message. See fprintf(3) for details.
    @param ... The arguments for printing.
*/
void fatal(const char *fmt, ...) {
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	exit(126);
}


static void *get_libc_func(const char *name) {
	static void *libc;

	if (libc == NULL) {
		if ((libc = dlopen("libc.so.6", RTLD_LAZY)) == NULL)
			exit(127);
	}

	return dlsym(libc, name);
}

static char *get_real_name(const char *name) {
	INIT_LIBC_FUNC(getcwd);

	if (strncmp(name, mock_dir, sizeof(mock_dir) - 1) == 0 && name[sizeof(mock_dir) - 1] == '/') {
		char name_buffer[PATH_MAX];
		if (real_getcwd(name_buffer, sizeof(name_buffer)) == NULL)
			fatal("mockdir:%d: getcwd failed: %m\n", __LINE__);
		strcat(name_buffer, name + sizeof(mock_dir) - 1);
		return strdup(name_buffer);
	} else {
		return strdup(name);
	}
}

char *getcwd(char *buf, size_t size) {
	if (size < sizeof(mock_dir)) {
		errno = ERANGE;
		return NULL;
	}
	strcpy(buf, mock_dir);
	return buf;
}

int open(const char *name, int flags) {
	char *real_name;
	int result;

	INIT_LIBC_FUNC(open);

	real_name = get_real_name(name);
	result = real_open(real_name, flags);
	free(real_name);
	return result;
}

char *realpath(const char *path, char *resolved_path) {
	char cwd_buffer[PATH_MAX];
	char *real_name;
	char *result;
	size_t cwd_buffer_len;

	INIT_LIBC_FUNC(realpath);
	INIT_LIBC_FUNC(getcwd);

	if (real_getcwd(cwd_buffer, sizeof(cwd_buffer)) == NULL)
		fatal("mockdir:%d: getcwd failed: %m\n", __LINE__);
	cwd_buffer_len = strlen(cwd_buffer);

	real_name = get_real_name(path);
	result = real_realpath(real_name, resolved_path);
	free(real_name);

	if (result == NULL)
		return NULL;

	if (strncmp(result, cwd_buffer, cwd_buffer_len) == 0 && result[cwd_buffer_len] == '/') {
		if (cwd_buffer_len < sizeof(mock_dir) - 1)
			fatal("mockdir:%d: current working directory name shorter than %s\n", __LINE__, mock_dir);
		memcpy(result, mock_dir, sizeof(mock_dir) - 1);
		memmove(result + sizeof(mock_dir) - 1, result + cwd_buffer_len, strlen(result) - cwd_buffer_len + 1);
	}

	return result;
}

int stat(const char *path, struct stat *buf) {
	char *real_name;
	int result;

	INIT_LIBC_FUNC(stat);

	real_name = get_real_name(path);
	result = real_stat(real_name, buf);
	free(real_name);
	return result;
}

int __xstat(int ver, const char *path, struct stat *buf) {
	char *real_name;
	int result;

	INIT_LIBC_FUNC(__xstat);

	real_name = get_real_name(path);
	result = real___xstat(ver, real_name, buf);
	free(real_name);
	return result;
}

int rename(const char *oldpath, const char *newpath) {
	char *real_oldname, *real_newname;
	int result;

	INIT_LIBC_FUNC(rename);

	real_oldname = get_real_name(oldpath);
	real_newname = get_real_name(newpath);
	result = real_rename(real_oldname, real_newname);
	free(real_oldname);
	free(real_newname);
	return result;
}

int link(const char *oldpath, const char *newpath) {
	char *real_oldname, *real_newname;
	int result;

	INIT_LIBC_FUNC(link);

	real_oldname = get_real_name(oldpath);
	real_newname = get_real_name(newpath);
	result = real_link(real_oldname, real_newname);
	free(real_oldname);
	free(real_newname);
	return result;
}

int unlink(const char *pathname) {
	char *real_name;
	int result;

	INIT_LIBC_FUNC(unlink);

	real_name = get_real_name(pathname);
	result = real_unlink(real_name);
	free(real_name);
	return result;
}

int mkstemp(char *template) {
	char *real_name;
	int result;
	size_t template_len;
	size_t real_name_len;

	INIT_LIBC_FUNC(mkstemp);

	real_name = get_real_name(template);
	result = real_mkstemp(real_name);
	template_len = strlen(template) - 1;
	real_name_len = strlen(real_name) - 1;
	for (; template[template_len] == 'X'; template_len--, real_name_len--)
		template[template_len] = real_name[real_name_len];
	free(real_name);
	return result;
}

int creat(const char *name, mode_t mode) {
	char *real_name;
	int result;

	INIT_LIBC_FUNC(creat);

	real_name = get_real_name(name);
	result = real_creat(real_name, mode);
	free(real_name);
	return result;
}
