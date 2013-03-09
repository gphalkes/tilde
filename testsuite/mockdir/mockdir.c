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
#include <errno.h>
#include <limits.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>

static const char mock_dir[] = "/mockdir";
static const size_t mock_dir_len = sizeof(mock_dir) - 1;

static char *base_dir;
static size_t base_dir_len;

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


#define FATAL(...) fatal(__LINE__, __VA_ARGS__)
#ifdef __GNUC__
void fatal(int line, const char *fmt, ...) __attribute__((format (printf, 2, 3))) __attribute__((noreturn));
#else
/*@noreturn@*/ void fatal(int line, const char *fmt, ...);
#endif

/** Alert the user of a fatal error and quit.
    @param fmt The format string for the message. See fprintf(3) for details.
    @param ... The arguments for printing.
*/
void fatal(int line, const char *fmt, ...) {
	va_list args;

	fprintf(stderr, "mockdir:%d: ", line);
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	abort();
}

static char *strcat_auto(const char *a, const char *b) {
	char *result;

	if ((result = malloc(strlen(a) + strlen(b) + 1)) == NULL)
		FATAL("out of memory\n");

	return strcat(strcpy(result, a), b);
}

static char *safe_strdup(const char *str) {
	char *result = strdup(str);
	if (result == NULL)
		FATAL("out of memory\n");
	return result;
}

static void *get_libc_func(const char *name) {
	static void *libc;

	if (libc == NULL) {
		if ((libc = dlopen("libc.so.6", RTLD_LAZY)) == NULL)
			FATAL("dlopen failed: %s\n", dlerror());
	}

	return dlsym(libc, name);
}

static void init_base_dir(void) {
	if (base_dir == NULL) {
		if ((base_dir = getenv("MOCK_DIR")) == NULL) {
			INIT_LIBC_FUNC(getcwd);
			if ((base_dir = real_getcwd(NULL, 0)) == NULL)
				FATAL("getcwd failed: %m\n");
		} else {
			INIT_LIBC_FUNC(realpath);
			base_dir = real_realpath(base_dir, NULL);
		}
		base_dir_len = strlen(base_dir);
	}
}

static char *get_real_name(const char *name) {
	char *name_buffer;

	if (strncmp(name, mock_dir, mock_dir_len) == 0 && name[mock_dir_len] == '/') {
		init_base_dir();
		name_buffer = strcat_auto(base_dir, name + mock_dir_len);
		return name_buffer;
	} else {
		return safe_strdup(name);
	}
}

static char *get_mock_name(const char *name) {
	char *result;
	size_t name_len;

	init_base_dir();

	name_len = strlen(name);

	if (base_dir_len > name_len)
		result = safe_strdup(name);
	else if (strncmp(name, base_dir, base_dir_len) == 0 && (name[base_dir_len] == '/' || base_dir_len == name_len))
		result = strcat_auto(mock_dir, name + base_dir_len);
	else
		result = safe_strdup(name);
	return result;
}

char *getcwd(char *buf, size_t size) {
	char *result, *mock_name;

	INIT_LIBC_FUNC(getcwd);

	result = real_getcwd(NULL, 0);
	mock_name = get_mock_name(result);
	free(result);

	if (buf == NULL)
		return mock_name;

	if (size < strlen(mock_name) + 1) {
		errno = ERANGE;
		return NULL;
	}
	strcpy(buf, mock_name);
	free(mock_name);
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
	char *real_name, *mock_name;
	char *result;

	INIT_LIBC_FUNC(realpath);
	INIT_LIBC_FUNC(getcwd);

	real_name = get_real_name(path);
	result = real_realpath(real_name, resolved_path);
	free(real_name);

	if (result == NULL)
		return NULL;

	mock_name = get_mock_name(result);
	if (resolved_path == NULL) {
		free(result);
		return mock_name;
	} else {
		if (strlen(mock_name) + 1 < PATH_MAX) {
			strcpy(resolved_path, mock_name);
			free(mock_name);
			return resolved_path;
		} else {
			free(mock_name);
			errno = ENAMETOOLONG;
			return NULL;
		}
	}
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

#ifdef __GNUC__
void __attribute__((constructor)) lib_init(void);
void __attribute__((destructor)) lib_fini(void);
#else
#define lib_init _init
#define lib_fini _fini
#endif

void lib_init(void) {
	init_base_dir();
}

void lib_fini(void) {
	free(base_dir);
}
