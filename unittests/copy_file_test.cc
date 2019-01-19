#include <cstring>
#include <fcntl.h>
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <t3widget/util.h>
#include <unistd.h>

#include "tilde/copy_file.h"

DEFINE_string(reflink_fs_dir, "", "Directory on a file system that supports reflinks.");
DEFINE_string(non_reflink_fs_dir, "", "Directory on a file system that does not support reflinks.");

namespace {

class QcheckIo {
 public:
  virtual ~QcheckIo() {}
  virtual QcheckIo& operator<<(const std::string &str) = 0;
  virtual QcheckIo& operator<<(ssize_t i) = 0;
  virtual QcheckIo& operator<<(size_t i) = 0;
};

class NoOpQcheckIo : public QcheckIo {
 public:
  QcheckIo& operator<<(const std::string &) override { return *this; }
  QcheckIo& operator<<(ssize_t) override { return *this; }
  QcheckIo& operator<<(size_t) override { return *this; }
};

class FatalQcheckIo : public QcheckIo {
 public:
  FatalQcheckIo(const char *condition) {
    std::cerr << "Check failed: " << condition;
  }
  ~FatalQcheckIo() {
    std::cerr << "\n";
    std::cerr.flush();
    abort();
  }
  QcheckIo& operator<<(const std::string &str) override {
    NextOutput();
    std::cerr << str;
    return *this;
  }
  QcheckIo& operator<<(ssize_t i) override { NextOutput(); std::cerr << i; return *this; }
  QcheckIo& operator<<(size_t i) override { NextOutput(); std::cerr << i; return *this; }

 private:
  void NextOutput() {
    if (!started_) {
      started_ = true;
      std::cerr << ": ";
    }
  }

  bool started_ = false;
};

class QcheckIoWrapper : public QcheckIo {
 public:
  QcheckIoWrapper(std::unique_ptr<QcheckIo> wrapped) : wrapped_(std::move(wrapped)) {}
  QcheckIo& operator<<(const std::string &str) override {
    (*wrapped_) << str;
    return *this;
  }
  QcheckIo& operator<<(ssize_t i) override { (*wrapped_) << i; return *this; }
  QcheckIo& operator<<(size_t i) override { (*wrapped_) << i; return *this; }
 private:
  std::unique_ptr<QcheckIo> wrapped_;
};

QcheckIoWrapper QcheckImpl(bool success, const char *condition) {
  if (success) {
    return QcheckIoWrapper(std::unique_ptr<QcheckIo>(new NoOpQcheckIo));
  }
  return QcheckIoWrapper(std::unique_ptr<QcheckIo>(new FatalQcheckIo(condition)));
}

#define QCHECK(condition) QcheckImpl(condition, #condition)

bool FileCopied(const std::string &a, const std::string &b) {
  int fd_a = open(a.c_str(), O_RDONLY);
  int fd_b = open(b.c_str(), O_RDONLY);

  QCHECK(fd_a != -1) << "Could not open " << a;
  QCHECK(fd_b != -1) << "Could not open " << b;

  char buffer_a[4096];
  char buffer_b[4096];
  size_t offset = 0;
  while (true) {
    ssize_t read_a = t3widget::nosig_read(fd_a, buffer_a, sizeof(buffer_a));
    ssize_t read_b = t3widget::nosig_read(fd_b, buffer_b, sizeof(buffer_b));

    if (read_a != read_b) {
      std::cerr << "File " << a << " and file " << b << " have different lengths. Read results at offset " << offset << ": a=" << read_a << " b=" << read_b << "\n";
      return false;
    }
    if (read_a == 0) {
      return true;
    }
    if (std::memcmp(buffer_a, buffer_b, read_a) != 0) {
      std::cerr << "File " << a << " and file " << b << " have different contents when reading at offset " << offset << "\n";
      return false;
    }
    offset += read_a;
  }
}

std::pair<std::string, int> CreateFile(const std::string &dir) {
  std::string name = dir + "/copy_file_test_XXXXXX";
  /* Unfortunately, we can't pass the c_str result to mkstemp as we are not allowed
     to change that string. So we'll just have to copy it into a vector :-( */
  std::vector<char> temp_name(name.begin(), name.end());
  // Ensure nul termination.
  temp_name.push_back(0);

  int fd = mkstemp(temp_name.data());
  QCHECK(fd >= 0) << "Could not create a temporary file with pattern " << name;
  name.assign(temp_name.data(), temp_name.size());
  return {name, fd};
}

std::pair<std::string, int> CreateFileWithContent(std::string content, const std::string &dir) {
  auto name_and_fd = CreateFile(dir);

  QCHECK(t3widget::nosig_write(name_and_fd.second, content.data(), content.size()) == static_cast<ssize_t>(content.size())) <<
      "Failed to write full content to file (" << strerror(errno) << ", " << name_and_fd.first << ")";
  QCHECK(fsync(name_and_fd.second) == 0);
  return name_and_fd;
}

void FillWithRandomData(const std::pair<std::string, int> &name_and_fd, size_t size) {
  char buffer[4096];
  while (size > 0) {
    for (size_t i = 0; i < sizeof(buffer); ++i) {
      buffer[i] = std::rand();
    }
    size_t to_write = std::min(size, sizeof(buffer));
    QCHECK(t3widget::nosig_write(name_and_fd.second, buffer, to_write) == static_cast<ssize_t>(to_write)) <<
        "Failed to write full content to file (" << strerror(errno) << ", " << name_and_fd.first << ")";
    size -= to_write;
  }
}

class CopyFileTest : public ::testing::Test {
 protected:
  ~CopyFileTest() {
    close(src_name_and_fd_.second);
    close(dest_name_and_fd_.second);
    unlink(src_name_and_fd_.first.c_str());
    unlink(dest_name_and_fd_.first.c_str());
  }
  std::pair<std::string, int> src_name_and_fd_{"", -1};
  std::pair<std::string, int> dest_name_and_fd_{"", -1};
};

// ======================= read_write ========================================
TEST_F(CopyFileTest, ReadWriteEmptyFile) {
  src_name_and_fd_ = CreateFile(FLAGS_non_reflink_fs_dir);
  dest_name_and_fd_ = CreateFile(FLAGS_non_reflink_fs_dir);

  EXPECT_EQ(copy_file_by_read_write(src_name_and_fd_.second, dest_name_and_fd_.second), 0);
  EXPECT_TRUE(FileCopied(src_name_and_fd_.first, dest_name_and_fd_.first));
}

TEST_F(CopyFileTest, ReadWriteWithContent) {
  src_name_and_fd_ = CreateFileWithContent("abcd", FLAGS_non_reflink_fs_dir);
  dest_name_and_fd_ = CreateFile(FLAGS_non_reflink_fs_dir);

  EXPECT_EQ(copy_file_by_read_write(src_name_and_fd_.second, dest_name_and_fd_.second), 0);
  EXPECT_TRUE(FileCopied(src_name_and_fd_.first, dest_name_and_fd_.first));
}

TEST_F(CopyFileTest, ReadWriteWithContentCrossFs) {
  src_name_and_fd_ = CreateFileWithContent("abcd", FLAGS_reflink_fs_dir);
  dest_name_and_fd_ = CreateFile(FLAGS_non_reflink_fs_dir);

  EXPECT_EQ(copy_file_by_read_write(src_name_and_fd_.second, dest_name_and_fd_.second), 0);
  EXPECT_TRUE(FileCopied(src_name_and_fd_.first, dest_name_and_fd_.first));
}

TEST_F(CopyFileTest, ReadWriteWithLargeContent) {
  src_name_and_fd_ = CreateFile(FLAGS_non_reflink_fs_dir);
  FillWithRandomData(src_name_and_fd_, 37456);
  dest_name_and_fd_ = CreateFile(FLAGS_non_reflink_fs_dir);

  EXPECT_EQ(copy_file_by_read_write(src_name_and_fd_.second, dest_name_and_fd_.second), 0);
  EXPECT_TRUE(FileCopied(src_name_and_fd_.first, dest_name_and_fd_.first));
}

// ======================= sendfile ==========================================
TEST_F(CopyFileTest, SendfileEmptyFile) {
  src_name_and_fd_ = CreateFile(FLAGS_non_reflink_fs_dir);
  dest_name_and_fd_ = CreateFile(FLAGS_non_reflink_fs_dir);

  EXPECT_EQ(copy_file_by_sendfile(src_name_and_fd_.second, dest_name_and_fd_.second, 0), 0);
  EXPECT_TRUE(FileCopied(src_name_and_fd_.first, dest_name_and_fd_.first));
}

TEST_F(CopyFileTest, SendfileWithContent) {
  src_name_and_fd_ = CreateFileWithContent("abcd", FLAGS_non_reflink_fs_dir);
  dest_name_and_fd_ = CreateFile(FLAGS_non_reflink_fs_dir);

  EXPECT_EQ(copy_file_by_sendfile(src_name_and_fd_.second, dest_name_and_fd_.second, 4), 0);
  EXPECT_TRUE(FileCopied(src_name_and_fd_.first, dest_name_and_fd_.first));
}

TEST_F(CopyFileTest, SendfileWithContentCrossFs) {
  src_name_and_fd_ = CreateFileWithContent("abcd", FLAGS_reflink_fs_dir);
  dest_name_and_fd_ = CreateFile(FLAGS_non_reflink_fs_dir);

  EXPECT_EQ(copy_file_by_sendfile(src_name_and_fd_.second, dest_name_and_fd_.second, 4), 0);
  EXPECT_TRUE(FileCopied(src_name_and_fd_.first, dest_name_and_fd_.first));
}

TEST_F(CopyFileTest, SendfileWithContentReflinkFs) {
  src_name_and_fd_ = CreateFileWithContent("abcd", FLAGS_reflink_fs_dir);
  dest_name_and_fd_ = CreateFile(FLAGS_reflink_fs_dir);

  EXPECT_EQ(copy_file_by_sendfile(src_name_and_fd_.second, dest_name_and_fd_.second, 4), 0);
  EXPECT_TRUE(FileCopied(src_name_and_fd_.first, dest_name_and_fd_.first));
}

// ======================= copy_file_range ===================================
TEST_F(CopyFileTest, CopyFileRangeEmptyFile) {
  src_name_and_fd_ = CreateFile(FLAGS_non_reflink_fs_dir);
  dest_name_and_fd_ = CreateFile(FLAGS_non_reflink_fs_dir);

  EXPECT_EQ(copy_file_by_copy_file_range(src_name_and_fd_.second, dest_name_and_fd_.second, 0), 0);
  EXPECT_TRUE(FileCopied(src_name_and_fd_.first, dest_name_and_fd_.first));
}

TEST_F(CopyFileTest, CopyFileRangeWithContent) {
  src_name_and_fd_ = CreateFileWithContent("abcd", FLAGS_non_reflink_fs_dir);
  dest_name_and_fd_ = CreateFile(FLAGS_non_reflink_fs_dir);

  EXPECT_EQ(copy_file_by_copy_file_range(src_name_and_fd_.second, dest_name_and_fd_.second, 4), 0);
  EXPECT_TRUE(FileCopied(src_name_and_fd_.first, dest_name_and_fd_.first));
}

TEST_F(CopyFileTest, CopyFileRangeWithContentCrossFs) {
  src_name_and_fd_ = CreateFileWithContent("abcd", FLAGS_reflink_fs_dir);
  dest_name_and_fd_ = CreateFile(FLAGS_non_reflink_fs_dir);

  EXPECT_EQ(copy_file_by_copy_file_range(src_name_and_fd_.second, dest_name_and_fd_.second, 4), EXDEV);
}

TEST_F(CopyFileTest, CopyFileRangeWithContentReflinkFs) {
  src_name_and_fd_ = CreateFileWithContent("abcd", FLAGS_reflink_fs_dir);
  dest_name_and_fd_ = CreateFile(FLAGS_reflink_fs_dir);

  EXPECT_EQ(copy_file_by_copy_file_range(src_name_and_fd_.second, dest_name_and_fd_.second, 4), 0);
  EXPECT_TRUE(FileCopied(src_name_and_fd_.first, dest_name_and_fd_.first));
}

// ======================= ficlone ===========================================
TEST_F(CopyFileTest, FicloneWithContentReflinkFs) {
  src_name_and_fd_ = CreateFileWithContent("abcd", FLAGS_reflink_fs_dir);
  dest_name_and_fd_ = CreateFile(FLAGS_reflink_fs_dir);

  EXPECT_EQ(copy_file_by_ficlone(src_name_and_fd_.second, dest_name_and_fd_.second), 0);
  EXPECT_TRUE(FileCopied(src_name_and_fd_.first, dest_name_and_fd_.first));
}

TEST_F(CopyFileTest, FicloneWithContentCrossFs) {
  src_name_and_fd_ = CreateFileWithContent("abcd", FLAGS_reflink_fs_dir);
  dest_name_and_fd_ = CreateFile(FLAGS_non_reflink_fs_dir);

  EXPECT_EQ(copy_file_by_ficlone(src_name_and_fd_.second, dest_name_and_fd_.second), EXDEV);
}

TEST_F(CopyFileTest, FicloneWithContentNonReflinkFs) {
  src_name_and_fd_ = CreateFileWithContent("abcd", FLAGS_non_reflink_fs_dir);
  dest_name_and_fd_ = CreateFile(FLAGS_non_reflink_fs_dir);

  EXPECT_EQ(copy_file_by_ficlone(src_name_and_fd_.second, dest_name_and_fd_.second), ENOTSUP);
}

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  if (FLAGS_reflink_fs_dir.empty()) {
    const char *env = getenv("BTRFS_MOUNT");
    if (env) FLAGS_reflink_fs_dir = env;
  }
  if (FLAGS_non_reflink_fs_dir.empty()) {
    const char *env = getenv("EXT3_MOUNT");
    if (env) FLAGS_non_reflink_fs_dir = env;
  }

  QCHECK(!FLAGS_reflink_fs_dir.empty()) << "--reflink_fs_dir or BTRFS_MOUNT must be set";
  QCHECK(!FLAGS_non_reflink_fs_dir.empty()) << "--non_reflink_fs_dir or EXT3_MOUNT must be set";

  struct stat reflink_stat;
  struct stat non_reflink_stat;
  QCHECK(stat(FLAGS_reflink_fs_dir.c_str(), &reflink_stat) == 0);
  QCHECK(stat(FLAGS_non_reflink_fs_dir.c_str(), &non_reflink_stat) == 0);
  QCHECK(reflink_stat.st_dev != non_reflink_stat.st_dev) << "reflink enabled and non-reflink enabled file systems must be different";
  return RUN_ALL_TESTS();
}

