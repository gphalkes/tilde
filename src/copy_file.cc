#include "tilde/copy_file.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <limits>
#include <sys/stat.h>
#include <sys/types.h>
#include <t3widget/util.h>
#include <unistd.h>

using namespace t3widget;

static bool rewind_files(int src_fd, int dest_fd) {
  if (lseek(src_fd, 0, SEEK_SET) == (off_t)-1) {
    return false;
  }
  if (lseek(dest_fd, 0, SEEK_SET) == (off_t)-1) {
    return false;
  }
  return true;
}

#if defined(HAS_SENDFILE) && defined(__linux__)
#include <sys/sendfile.h>

int copy_file_by_sendfile(int src_fd, int dest_fd, size_t bytes_to_copy) {
  if (!rewind_files(src_fd, dest_fd)) {
    return errno;
  }

  ssize_t result;
  do {
    result = sendfile(dest_fd, src_fd, nullptr, bytes_to_copy);
    if (result < 0) {
      if (errno == EAGAIN) {
        continue;
      }
    }
    if (result < 0) {
      return errno;
    }
    bytes_to_copy -= result;
  } while (bytes_to_copy > 0);
  return 0;
}
#else
#if defined(TILDE_UNITTEST) && defined(__linux__)
#error Please define HAS_SENDFILE in unit tests
#endif
int copy_file_by_sendfile(int, int, size_t) { return ENOTSUP; }
#endif

#if defined(HAS_COPY_FILE_RANGE)
int copy_file_by_copy_file_range(int src_fd, int dest_fd, size_t bytes_to_copy) {
  if (!rewind_files(src_fd, dest_fd)) {
    return errno;
  }

  ssize_t result;
  do {
    result = copy_file_range(src_fd, nullptr, dest_fd, nullptr, bytes_to_copy, 0);
    if (result < 0) {
      if (errno == EAGAIN) {
        continue;
      }
    }
    if (result < 0) {
      return errno;
    }
    bytes_to_copy -= result;
  } while (bytes_to_copy > 0);
  return 0;
}
#else
#if defined(TILDE_UNITTEST) && defined(__linux__)
#error Please define HAS_COPY_FILE_RANGE in unit tests
#endif
int copy_file_by_copy_file_range(int, int, size_t) { return ENOTSUP; }
#endif

#if defined(HAS_FICLONE)
#include <linux/fs.h>
#include <sys/ioctl.h>

int copy_file_by_ficlone(int src_fd, int dest_fd) {
  if (ioctl(dest_fd, FICLONE, src_fd) == -1) {
    return errno;
  }
  return 0;
}
#else
#if defined(TILDE_UNITTEST) && defined(__linux__)
#error Please define HAS_FICLONE in unit tests
#endif
int copy_file_by_ficlone(int, int) { return ENOTSUP; }
#endif

int copy_file_by_read_write(int src_fd, int dest_fd) {
  if (!rewind_files(src_fd, dest_fd)) {
    return errno;
  }
  // Copy in chunks of 32K. This aims to balance memory use vs. number of operations.
  char buffer[32768];
  while (true) {
    ssize_t read_bytes = nosig_read(src_fd, buffer, sizeof(buffer));
    if (read_bytes < 0) {
      return errno;
    } else if (read_bytes == 0) {
      return 0;
    }

    ssize_t written_bytes = nosig_write(dest_fd, buffer, read_bytes);
    if (written_bytes < 0) {
      return errno;
    }
  }
}

int copy_file(int src_fd, int dest_fd) {
  int result;

  result = copy_file_by_ficlone(src_fd, dest_fd);
  if (result == 0) {
    return result;
  }

  struct stat statbuf;
  if (fstat(src_fd, &statbuf) < 0) {
    return errno;
  }
  // FIXME: these routines may fail if the file changed in between and are now shorter!
  result = copy_file_by_copy_file_range(src_fd, dest_fd, statbuf.st_size);
  if (result != ENOTSUP) {
    return result;
  }
  result = copy_file_by_sendfile(src_fd, dest_fd, statbuf.st_size);
  if (result != ENOTSUP) {
    return result;
  }
  return copy_file_by_read_write(src_fd, dest_fd);
}
