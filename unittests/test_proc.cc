#include <sys/wait.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <gtest/gtest.h>
#include <src/proc.hpp>

namespace {
struct proc_file_info_mock : public proc_file_info {
  proc_file_info_mock() : proc_file_info(0) { }
  bool update_file_info(file_list& list, const timespec& stamp) {
    return proc_file_info::update_file_info(list, stamp);
  }
  bool update_file_info(file_info& info, const timespec& stamp, std::istream& in, const bool is_new) {
    return proc_file_info::update_file_info(info, stamp, in, is_new);
  }
};

TEST(PROC, update_file_info_internal) {
  file_info info;
  timespec stamp = {5, 2345 };
  proc_file_info_mock updater;

  {
    info.offset   = -1;
    info.writable = false;
    std::istringstream in ("pos: 10\nflags: 010000\n");
    updater.update_file_info(info, stamp, in, false);
    EXPECT_EQ((off_t)10, info.offset);
    EXPECT_FALSE(info.writable);
  }

  {
    info.offset   = -1;
    info.writable = false;
    std::istringstream in ("pos: 0\nflags: 020\n");
    updater.update_file_info(info, stamp, in, true);
    EXPECT_EQ((off_t)0, info.offset);
    EXPECT_FALSE(info.writable);
  }

  {
    info.offset   = -1;
    info.writable = false;
    std::istringstream in ("pos: -50\nflags: 01001\n");
    updater.update_file_info(info, stamp, in, true);
    EXPECT_EQ((off_t)-50, info.offset);
    EXPECT_TRUE(info.writable);
  }

  {
    info.offset   = -1;
    info.writable = false;
    std::istringstream in ("flags: 01272\npos: 327");
    updater.update_file_info(info, stamp, in, true);
    EXPECT_EQ((off_t)327, info.offset);
    EXPECT_TRUE(info.writable);
  }
}

TEST(PROC, update_file_info_external) {
  const std::string in_file  = "test_infile";
  const std::string out_file = "test_outfile";
  const std::string line     = "Hello the world";
  {
    std::ofstream infile(in_file.c_str());
    infile << line << "\n" << line;
  }

  int pipefd1[2];
  int pipefd2[2];
  char buf;
  ASSERT_EQ(0, pipe(pipefd1));
  ASSERT_EQ(0, pipe(pipefd2));
  pid_t pid = fork();
  ASSERT_LT(-1, pid);
  if(pid == 0) { // child
    close(pipefd1[1]);
    close(pipefd2[0]);
    close(0); close(1); close(2); // no standard descriptors
    std::ifstream in(in_file.c_str());
    in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::ofstream out("test_outfile");
    out << line << line << std::flush;
    close(pipefd2[1]); // Signal parent that we are ready
    read(pipefd1[0], &buf, 1); // Wait for parent to be done
    exit(0);
  }

  close(pipefd1[0]);
  close(pipefd2[1]);
  read(pipefd2[0], &buf, 1); // Wait for child to close its end -> ready to get fd information

  std::vector<file_info> info_files;
  timespec stamp = { 4, 5432 };
  proc_file_info updater(pid);
  ASSERT_TRUE(updater.update_file_info(info_files, stamp));
  ASSERT_EQ((size_t)2, info_files.size());

  struct stat stat_buf;
  std::string p;
  char* pwd(get_current_dir_name());
  ASSERT_EQ(0, stat(in_file.c_str(), &stat_buf));
  EXPECT_LT(-1, info_files[0].fd);
  EXPECT_EQ(stat_buf.st_ino, info_files[0].inode);
  p = std::string(pwd) + "/" + in_file;
  EXPECT_EQ(p, info_files[0].name);
  EXPECT_LT(line.size(), (size_t)info_files[0].offset);
  EXPECT_FALSE(info_files[0].writable);
  EXPECT_TRUE(info_files[0].updated);

  ASSERT_EQ(0, stat(out_file.c_str(), &stat_buf));
  EXPECT_LT(-1, info_files[1].fd);
  EXPECT_EQ(stat_buf.st_ino, info_files[1].inode);
  p = std::string(pwd) + "/" + out_file;
  EXPECT_EQ(p, info_files[1].name);
  EXPECT_EQ(2 * line.size(), (size_t)info_files[1].offset);
  EXPECT_TRUE(info_files[1].writable);
  EXPECT_TRUE(info_files[1].updated);

  free(pwd);
  close(pipefd1[1]); // Signal child to exit
  int status;
  wait(&status);
}
} // namespace
