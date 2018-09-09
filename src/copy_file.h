#ifndef COPY_FILE_H_
#define COPY_FILE_H_

#include <cstddef>

// Copy file by different methods. The files need not be at the starting position. The postion
// after copy is undefined.
int copy_file_by_sendfile(int src_fd, int dest_fd, size_t bytes_to_copy);
int copy_file_by_copy_file_range(int src_fd, int dest_fd, size_t bytes_to_copy);
int copy_file_by_ficlone(int src_fd, int dest_fd);
int copy_file_by_read_write(int src_fd, int dest_fd);

// Generic copy routine which will try to copy the file using one of the methods above.
int copy_file(int src_fd, int dest_fd);

#endif
