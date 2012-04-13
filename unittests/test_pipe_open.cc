#include <gtest/gtest.h>
#include <src/pipe_open.hpp>
#include <stdexcept>

TEST(PipeOpen, echo) {
  const char* text = "Hello there";
  const char* echo_cmd[] = { "/bin/echo", text, 0 };
  pipe_open echo_pipe(echo_cmd);
  EXPECT_TRUE(echo_pipe.good());
  std::string res;
  getline(echo_pipe, res);
  EXPECT_EQ(0, res.compare(text));
}

TEST(PipeOpen, echoe) {
  const char* lines[2] = { "Line one", "Second line" };
  const char* echo_cmd[] = { "/bin/echo", "-e", lines[0], "\n", lines[1], 0 };
  pipe_open echo_pipe(echo_cmd);

  int i = 0;
  std::string line;
  while(getline(echo_pipe, line)) {
    ASSERT_GT(2, i);
    // echo added a space between the arguments.
    EXPECT_EQ(strlen(lines[i]) + 1, line.size());
    if(i == 0) {
      EXPECT_EQ(' ', line.at(line.size() - 1));
      line.resize(line.size() - 1);
      EXPECT_STREQ(lines[i], line.c_str());
    } else {
      EXPECT_EQ(' ', line.at(0));
      EXPECT_STREQ(lines[i], line.c_str() + 1);
    }
    ++i;
  }
  EXPECT_EQ(2, i);
}

TEST(PipeOpen, status) {
  const char* true_cmd[] = { "true", 0 };
  const char* false_cmd[] = { "false", 0 };

  pipe_open true_pipe(true_cmd);
  std::pair<int, int> res = true_pipe.status();
  ASSERT_EQ(0, res.first);
  ASSERT_TRUE(WIFEXITED(res.second));
  ASSERT_EQ(0, WEXITSTATUS(res.second));
  
  pipe_open false_pipe(false_cmd);
  res = false_pipe.status();
  ASSERT_EQ(0, res.first);
  ASSERT_TRUE(WIFEXITED(res.second));
  ASSERT_EQ(1, WEXITSTATUS(res.second));
}

TEST(PipeOpen, fail) {
  // I hope this command does not exists
  const char* no_cmd[] = { "qwertyuiopasdfghjklzxcvbnm", 0 };

  EXPECT_THROW(pipe_open no_pipe(no_cmd), std::runtime_error);

  pipe_open no_pipe_no_check(no_cmd, false);
  std::string line;
  EXPECT_FALSE(getline(no_pipe_no_check, line));
  EXPECT_TRUE(line.empty());
  EXPECT_FALSE(no_pipe_no_check.good());
}
