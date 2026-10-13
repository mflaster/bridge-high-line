#include <catch2/catch_all.hpp>

#include "string_util.h"

TEST_CASE("TrimLeft") {
  REQUIRE(TrimLeft("") == "");
  REQUIRE(TrimLeft(" ") == "");
  REQUIRE(TrimLeft("\t\r\n") == "");
  REQUIRE(TrimLeft("  hello") == "hello");
  REQUIRE(TrimLeft("hello  ") == "hello  ");
  REQUIRE(TrimLeft("  hello  ") == "hello  ");
  REQUIRE(TrimLeft("\t\n hello world \r\n") == "hello world \r\n");
}

TEST_CASE("TrimRight") {
  REQUIRE(TrimRight("") == "");
  REQUIRE(TrimRight(" ") == "");
  REQUIRE(TrimRight("\t\r\n") == "");
  REQUIRE(TrimRight("  hello") == "  hello");
  REQUIRE(TrimRight("hello  ") == "hello");
  REQUIRE(TrimRight("  hello  ") == "  hello");
  REQUIRE(TrimRight("\t\n hello world \r\n") == "\t\n hello world");
}

TEST_CASE("Trim") {
  REQUIRE(Trim("") == "");
  REQUIRE(Trim(" ") == "");
  REQUIRE(Trim("   ") == "");
  REQUIRE(Trim("\t\r\n ") == "");
  REQUIRE(Trim("  hello") == "hello");
  REQUIRE(Trim("hello  ") == "hello");
  REQUIRE(Trim("  hello  ") == "hello");
  REQUIRE(Trim("\t\n hello world \r\n") == "hello world");
  REQUIRE(Trim("no_whitespace") == "no_whitespace");
}

TEST_CASE("ToLower") {
  REQUIRE(ToLower("") == "");
  REQUIRE(ToLower("HELLO") == "hello");
  REQUIRE(ToLower("Hello World") == "hello world");
  REQUIRE(ToLower("abc 123 !@#") == "abc 123 !@#");
  REQUIRE(ToLower("SPADES") == "spades");
}
