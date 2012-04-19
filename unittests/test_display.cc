#include <gtest/gtest.h>
#include <src/print_info.hpp>

TEST(Print, int) {
  struct val_str {
    long        val;
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

TEST(Print, double) {
  struct val_str {
    double      val;
    const char* str;
  };
  const val_str tests[] = {
    { 5.32e10,    " 53.2G" },
    { 3e30,       "+infty" },
    { -10e27,     "-infty" },
    { 0.5,        "  500m" },
    { -12345e-15, "-12.3p" },
    { 1e-30,      "    0 " },
    { -1e-40,     "   -0 " },
    { 0.0, 0 }
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

TEST(Print, time) {
  struct val_res {
    const double in;
    const char*  out;
  };
  const val_res tests[] = {
    { 0.523,      "  < 1s" },
    { -0.005432,  "  < 1s" },
    { 5.2,        "  5.2s" },
    { 125,        " 2.08m" },
    { 10000,      " 2.78h" },
    { 100000,     " 1.16d" },
    { 1000000000, " > 10y" },
    { 0.0,        "" }
  };
  for(const val_res* ptr = tests; ptr->in; ++ptr) {
    std::string res = seconds_to_str(ptr->in);
    EXPECT_STREQ(ptr->out, res.c_str());
  }
  
}
