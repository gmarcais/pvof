#include <gtest/gtest.h>
#include <src/lsof.hpp>
#include <src/timespec.hpp>

TEST(LSOF, find_file_in_list) {
  file_list list;
  file_info f;

  f.fd    = 0;
  f.inode = 314;
  list.push_back(f);

  f.fd    = 5;
  f.inode = 271;
  list.push_back(f);

  EXPECT_EQ((size_t)2, list.size());
  auto s1 = find_file_in_list(list, 0, 314);
  ASSERT_NE(list.end(), s1);
  EXPECT_EQ(0, s1->fd);
  EXPECT_EQ((ino_t)314, s1->inode);

  auto s2 = find_file_in_list(list, 5, 271);
  ASSERT_NE(list.end(), s2);
  EXPECT_EQ(5, s2->fd);
  EXPECT_EQ((ino_t)271, s2->inode);

  auto s3 = find_file_in_list(list, 5, 314);
  ASSERT_EQ(list.end(), s3);
}

TEST(LSOF, parse_line) {
  const char* lines[] = {
    "p9784\0\n",
    "fcwd\0a \0i2\0\n",
    "frtd\0a \0i2\0\n",
    "ftxt\0a \0i760219\0\n",
    "fmem\0a \0i9183059\0\n",
    "fmem\0a \0i135483\0\n",
    "fmem\0a \0i134896\0\n",
    "f0\0a \0tREG\0o0t0\0i234381\0\n",
    "f1\0au\0tCHR\0o0t0\0i234381\0\n",
    "f10\0ar\0tREG\0s40382\0o0x2345a\0i5505713\0n/home/gus/Documents/test\0\n",
    0
  };
  for(const char** ptr = lines; *ptr; ++ptr) {
    std::string line(*ptr, (const char*)memchr(*ptr, '\n', 1024) - *ptr);
    file_info f;
    bool res = parse_line(line, f);
    EXPECT_EQ(ptr - lines == 9, res);
    if(res) {
      EXPECT_EQ(10, f.fd);
      EXPECT_EQ((ino_t)5505713, f.inode);
      EXPECT_EQ((off_t)0x2345a, f.offset);
      EXPECT_EQ((off_t)40382, f.size);
      EXPECT_STREQ("/home/gus/Documents/test", f.name.c_str());
    }
  }
}

TEST(LSOF, update_file_info) {
  std::stringstream lsof_stream;
  const char* lines[] = {
    "p31415\0\n",
    "fcwd\0a \0i2\0\n",
    "frtd\0a \0i2\0\n",
    "f2\0ar\0o0x2345\0i9876\0\n",
    "f10\0ar\0o0t58\0i452\0\n",
    0
  };
  for(const char** ptr = lines; *ptr; ++ptr)
    lsof_stream << std::string(*ptr, (const char*)memchr(*ptr, '\n', 1024) - *ptr + 1);

  timespec stamp = { 5, 2345 };
  file_list list;
  bool res = update_file_info(lsof_stream, list, stamp);
  EXPECT_TRUE(res);
  ASSERT_EQ((size_t)2, list.size());
  EXPECT_TRUE(list[0].updated);
  EXPECT_EQ(stamp, list[0].stamp);
  EXPECT_TRUE(list[1].updated);
  EXPECT_EQ(stamp, list[1].stamp);
  EXPECT_EQ((off_t)58, list[1].offset);

  std::stringstream lsof_stream2;
  const char* lines2[] = {
    "f10\0ar\0o0x435678\0i452\0\n",
    "f11\0ar\0o0\0i1\0\n",
    0
  };
  timespec new_stamp = stamp + 5;
  for(const char** ptr = lines2; *ptr; ++ptr)
    lsof_stream2 << std::string(*ptr, (const char*)memchr(*ptr, '\n', 1024) - *ptr + 1);
  res = update_file_info(lsof_stream2, list, new_stamp);
  EXPECT_TRUE(res);
  ASSERT_EQ((size_t)3, list.size());
  EXPECT_FALSE(list[0].updated);
  EXPECT_EQ(stamp, list[0].stamp);
  EXPECT_TRUE(list[1].updated);
  EXPECT_EQ(new_stamp, list[1].stamp);
  EXPECT_EQ((off_t)0x435678, list[1].offset);
  EXPECT_TRUE(list[2].updated);
  EXPECT_EQ(new_stamp, list[2].stamp);

  std::stringstream lsof_stream3;
  const char* lines3[] = {
    "f10\0ar\0s1024\0i452\0n/path/to/nname10\0\n",
    "f2\0ar\0s123456\0i9876\0nrelative (on /raid)\0\n",
    0
  };
  for(const char** ptr = lines3; *ptr; ++ptr)
    lsof_stream3 << std::string(*ptr, (const char*)memchr(*ptr, '\n', 1024) - *ptr + 1);
  res = update_file_names(lsof_stream3, list);
  EXPECT_TRUE(res);
  ASSERT_EQ((size_t)3, list.size());
  EXPECT_STREQ("/path/to/nname10", list[1].name.c_str());
  EXPECT_EQ((off_t)1024, list[1].size);
  EXPECT_STREQ("relative (on /raid)", list[0].name.c_str());
  EXPECT_EQ((off_t)123456, list[0].size);
  EXPECT_TRUE(list[2].name.empty());
}
