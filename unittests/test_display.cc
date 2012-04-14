#include <gtest/gtest.h>
#include <src/print_info.hpp>

TEST(Print, int) {
  struct val_str {
    long val;
    const char* str;
  };
  const val_str tests[] = {
    { 5,             "    5 " },
    { 1000,          "    1k" },
    { 23680,         " 23.7k" },
    { 5001200,       "    5M" },
    { 432123456789L, "  432G" },
    { -123456,       " -123k" },
    { 0, "" }
  };

  for(const val_str *t = tests; t->val; ++t) {
    std::string res = numerical_field_to_str(t->val);
    EXPECT_STREQ(t->str, res.c_str());
  }
}

TEST(Print, shorten_string) {
  struct val_res {
    const char* in;
    const char* out;
  };
  const val_res tests[] = {
    { "0", "0" },
    { "01234567", "01234567" },
    { "0123456789", "0123456789" },
    { "0123456789a", "...456789a" },
    { "0123456789abcdefg", "...abcdefg" },
    { "", "" }
  };
  for(const val_res* ptr = tests; strlen(ptr->in); ++ptr) {
    std::string res = shorten_string(ptr->in, 10);
    EXPECT_STREQ(ptr->out, res.c_str());
  }
}
