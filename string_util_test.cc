#include <gtest/gtest.h>

#include "string_util.h"

TEST(StringUtilTest, TrimLeft) {
  ASSERT_TRUE(TrimLeft("") == "");
  ASSERT_TRUE(TrimLeft(" ") == "");
  ASSERT_TRUE(TrimLeft("\t\r\n") == "");
  ASSERT_TRUE(TrimLeft("  hello") == "hello");
  ASSERT_TRUE(TrimLeft("hello  ") == "hello  ");
  ASSERT_TRUE(TrimLeft("  hello  ") == "hello  ");
  ASSERT_TRUE(TrimLeft("\t\n hello world \r\n") == "hello world \r\n");
}

TEST(StringUtilTest, TrimRight) {
  ASSERT_TRUE(TrimRight("") == "");
  ASSERT_TRUE(TrimRight(" ") == "");
  ASSERT_TRUE(TrimRight("\t\r\n") == "");
  ASSERT_TRUE(TrimRight("  hello") == "  hello");
  ASSERT_TRUE(TrimRight("hello  ") == "hello");
  ASSERT_TRUE(TrimRight("  hello  ") == "  hello");
  ASSERT_TRUE(TrimRight("\t\n hello world \r\n") == "\t\n hello world");
}

TEST(StringUtilTest, Trim) {
  ASSERT_TRUE(Trim("") == "");
  ASSERT_TRUE(Trim(" ") == "");
  ASSERT_TRUE(Trim("   ") == "");
  ASSERT_TRUE(Trim("\t\r\n ") == "");
  ASSERT_TRUE(Trim("  hello") == "hello");
  ASSERT_TRUE(Trim("hello  ") == "hello");
  ASSERT_TRUE(Trim("  hello  ") == "hello");
  ASSERT_TRUE(Trim("\t\n hello world \r\n") == "hello world");
  ASSERT_TRUE(Trim("no_whitespace") == "no_whitespace");
}

TEST(StringUtilTest, ToLower) {
  ASSERT_TRUE(ToLower("") == "");
  ASSERT_TRUE(ToLower("HELLO") == "hello");
  ASSERT_TRUE(ToLower("Hello World") == "hello world");
  ASSERT_TRUE(ToLower("abc 123 !@#") == "abc 123 !@#");
  ASSERT_TRUE(ToLower("SPADES") == "spades");
}
