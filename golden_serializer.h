#pragma once

#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "conditional_line.h"
#include "hand.h"
#include "solver.h"

class PlayLineGoldenSerializer {
 public:
  static std::string Serialize(const ConditionalLine &line);
  static std::string Serialize(const std::vector<ConditionalLine> &lines);
  static std::string Serialize(const std::multiset<int> &tricks_won);
  static std::string Serialize(const std::string &str);
  static std::string Serialize(const OptimalLines &lines);

  // Formatting helpers for hands and setup comments
  static std::string FormatHands(uint64_t hand, uint64_t unknown_cards = 0);
  static std::string FormatHands(uint64_t hand, Suit trump);
  static std::string FormatHands(uint64_t hand, uint64_t unknown_cards, Suit trump);

  static std::string FormatHands(const std::vector<uint64_t> &hands, uint64_t unknown_cards = 0);
  static std::string FormatHands(const std::vector<uint64_t> &hands, Suit trump);
  static std::string FormatHands(const std::vector<uint64_t> &hands, uint64_t unknown_cards, Suit trump);

  static std::string FormatHands(const std::pair<uint64_t, uint64_t> &hands, uint64_t unknown_cards = 0);
  static std::string FormatHands(const std::pair<uint64_t, uint64_t> &hands, Suit trump);
  static std::string FormatHands(const std::pair<uint64_t, uint64_t> &hands, uint64_t unknown_cards, Suit trump);

  static std::string FormatHands(const std::string &hands_str);
  static std::string FormatHands(const char *hands_str);
  static std::string FormatHands(const BoardState &bs);

  static std::string FormatCommentHeader(const std::string &description,
                                         const std::string &formatted_hands = "");

  // Overloaded SerializeWithHeader entry points
  template <typename T>
  static std::string SerializeWithHeader(const T &actual) {
    return Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::string &description) {
    std::string header = FormatCommentHeader(description, "");
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const char *description) {
    return SerializeWithHeader(actual, std::string(description));
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, uint64_t hand) {
    std::string hands_str = FormatHands(hand);
    std::string header = FormatCommentHeader("", hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::vector<uint64_t> &hands) {
    std::string hands_str = FormatHands(hands);
    std::string header = FormatCommentHeader("", hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::pair<uint64_t, uint64_t> &hands) {
    std::string hands_str = FormatHands(hands);
    std::string header = FormatCommentHeader("", hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, uint64_t hand, uint64_t unknown_cards) {
    std::string hands_str = FormatHands(hand, unknown_cards);
    std::string header = FormatCommentHeader("", hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, uint64_t hand, Suit trump) {
    std::string hands_str = FormatHands(hand, trump);
    std::string header = FormatCommentHeader("", hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, uint64_t hand, uint64_t unknown_cards, Suit trump) {
    std::string hands_str = FormatHands(hand, unknown_cards, trump);
    std::string header = FormatCommentHeader("", hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::vector<uint64_t> &hands,
                                         uint64_t unknown_cards) {
    std::string hands_str = FormatHands(hands, unknown_cards);
    std::string header = FormatCommentHeader("", hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::vector<uint64_t> &hands,
                                         Suit trump) {
    std::string hands_str = FormatHands(hands, trump);
    std::string header = FormatCommentHeader("", hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::vector<uint64_t> &hands,
                                         uint64_t unknown_cards, Suit trump) {
    std::string hands_str = FormatHands(hands, unknown_cards, trump);
    std::string header = FormatCommentHeader("", hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::pair<uint64_t, uint64_t> &hands,
                                         uint64_t unknown_cards) {
    std::string hands_str = FormatHands(hands, unknown_cards);
    std::string header = FormatCommentHeader("", hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::pair<uint64_t, uint64_t> &hands,
                                         Suit trump) {
    std::string hands_str = FormatHands(hands, trump);
    std::string header = FormatCommentHeader("", hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::pair<uint64_t, uint64_t> &hands,
                                         uint64_t unknown_cards, Suit trump) {
    std::string hands_str = FormatHands(hands, unknown_cards, trump);
    std::string header = FormatCommentHeader("", hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, uint64_t hand,
                                         const std::string &description) {
    std::string hands_str = FormatHands(hand);
    std::string header = FormatCommentHeader(description, hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, uint64_t hand,
                                         const char *description) {
    return SerializeWithHeader(actual, hand, std::string(description));
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, uint64_t hand, uint64_t unknown_cards,
                                         const std::string &description) {
    std::string hands_str = FormatHands(hand, unknown_cards);
    std::string header = FormatCommentHeader(description, hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, uint64_t hand, uint64_t unknown_cards,
                                         const char *description) {
    return SerializeWithHeader(actual, hand, unknown_cards, std::string(description));
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, uint64_t hand, Suit trump,
                                         const std::string &description) {
    std::string hands_str = FormatHands(hand, trump);
    std::string header = FormatCommentHeader(description, hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, uint64_t hand, Suit trump,
                                         const char *description) {
    return SerializeWithHeader(actual, hand, trump, std::string(description));
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, uint64_t hand, uint64_t unknown_cards,
                                         Suit trump, const std::string &description) {
    std::string hands_str = FormatHands(hand, unknown_cards, trump);
    std::string header = FormatCommentHeader(description, hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, uint64_t hand, uint64_t unknown_cards,
                                         Suit trump, const char *description) {
    return SerializeWithHeader(actual, hand, unknown_cards, trump, std::string(description));
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::vector<uint64_t> &hands,
                                         const std::string &description) {
    std::string hands_str = FormatHands(hands);
    std::string header = FormatCommentHeader(description, hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::vector<uint64_t> &hands,
                                         const char *description) {
    return SerializeWithHeader(actual, hands, std::string(description));
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::vector<uint64_t> &hands,
                                         uint64_t unknown_cards,
                                         const std::string &description) {
    std::string hands_str = FormatHands(hands, unknown_cards);
    std::string header = FormatCommentHeader(description, hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::vector<uint64_t> &hands,
                                         uint64_t unknown_cards,
                                         const char *description) {
    return SerializeWithHeader(actual, hands, unknown_cards, std::string(description));
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::vector<uint64_t> &hands,
                                         Suit trump,
                                         const std::string &description) {
    std::string hands_str = FormatHands(hands, trump);
    std::string header = FormatCommentHeader(description, hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::vector<uint64_t> &hands,
                                         Suit trump,
                                         const char *description) {
    return SerializeWithHeader(actual, hands, trump, std::string(description));
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::vector<uint64_t> &hands,
                                         uint64_t unknown_cards, Suit trump,
                                         const std::string &description) {
    std::string hands_str = FormatHands(hands, unknown_cards, trump);
    std::string header = FormatCommentHeader(description, hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::vector<uint64_t> &hands,
                                         uint64_t unknown_cards, Suit trump,
                                         const char *description) {
    return SerializeWithHeader(actual, hands, unknown_cards, trump, std::string(description));
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::pair<uint64_t, uint64_t> &hands,
                                         const std::string &description) {
    std::string hands_str = FormatHands(hands);
    std::string header = FormatCommentHeader(description, hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::pair<uint64_t, uint64_t> &hands,
                                         const char *description) {
    return SerializeWithHeader(actual, hands, std::string(description));
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::pair<uint64_t, uint64_t> &hands,
                                         uint64_t unknown_cards,
                                         const std::string &description) {
    std::string hands_str = FormatHands(hands, unknown_cards);
    std::string header = FormatCommentHeader(description, hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::pair<uint64_t, uint64_t> &hands,
                                         uint64_t unknown_cards,
                                         const char *description) {
    return SerializeWithHeader(actual, hands, unknown_cards, std::string(description));
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::pair<uint64_t, uint64_t> &hands,
                                         Suit trump,
                                         const std::string &description) {
    std::string hands_str = FormatHands(hands, trump);
    std::string header = FormatCommentHeader(description, hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::pair<uint64_t, uint64_t> &hands,
                                         Suit trump,
                                         const char *description) {
    return SerializeWithHeader(actual, hands, trump, std::string(description));
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::pair<uint64_t, uint64_t> &hands,
                                         uint64_t unknown_cards, Suit trump,
                                         const std::string &description) {
    std::string hands_str = FormatHands(hands, unknown_cards, trump);
    std::string header = FormatCommentHeader(description, hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const std::pair<uint64_t, uint64_t> &hands,
                                         uint64_t unknown_cards, Suit trump,
                                         const char *description) {
    return SerializeWithHeader(actual, hands, unknown_cards, trump, std::string(description));
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const BoardState &bs) {
    std::string hands_str = FormatHands(bs);
    std::string header = FormatCommentHeader("", hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const BoardState &bs,
                                         const std::string &description) {
    std::string hands_str = FormatHands(bs);
    std::string header = FormatCommentHeader(description, hands_str);
    return header + Serialize(actual);
  }

  template <typename T>
  static std::string SerializeWithHeader(const T &actual, const BoardState &bs,
                                         const char *description) {
    return SerializeWithHeader(actual, bs, std::string(description));
  }

 private:
  static std::string SerializeConditionalLineHelper(const ConditionalLine &line, int lvl);
};

class GoldenMatcher {
 public:
  explicit GoldenMatcher(std::string golden_filename);

  bool match(const std::string &actual) const;
  std::string describe() const;

 private:
  std::string golden_filename_;
  mutable std::string failure_message_;
};

::testing::AssertionResult CheckGoldenMatch(
    const char *actual_expr, const char *golden_expr,
    const std::string &actual, const std::string &golden_filename);

#define GET_REQUIRE_GOLDEN_MACRO(_1, _2, _3, _4, _5, _6, NAME, ...) NAME

#define REQUIRE_GOLDEN_2(actual, golden_filename)                      \
  ASSERT_PRED_FORMAT2(                                                 \
      CheckGoldenMatch,                                                \
      PlayLineGoldenSerializer::SerializeWithHeader(actual),           \
      golden_filename)

#define REQUIRE_GOLDEN_3(actual, golden_filename, arg3)                \
  ASSERT_PRED_FORMAT2(                                                 \
      CheckGoldenMatch,                                                \
      PlayLineGoldenSerializer::SerializeWithHeader(actual, arg3),     \
      golden_filename)

#define REQUIRE_GOLDEN_4(actual, golden_filename, hands, description)  \
  ASSERT_PRED_FORMAT2(                                                 \
      CheckGoldenMatch,                                                \
      PlayLineGoldenSerializer::SerializeWithHeader(                   \
          actual, hands, description),                                 \
      golden_filename)

#define REQUIRE_GOLDEN_5(actual, golden_filename, hands, arg4, desc)   \
  ASSERT_PRED_FORMAT2(                                                 \
      CheckGoldenMatch,                                                \
      PlayLineGoldenSerializer::SerializeWithHeader(                   \
          actual, hands, arg4, desc),                                  \
      golden_filename)

#define REQUIRE_GOLDEN_6(actual, golden_filename, hands, unk, trump, desc) \
  ASSERT_PRED_FORMAT2(                                                     \
      CheckGoldenMatch,                                                    \
      PlayLineGoldenSerializer::SerializeWithHeader(                       \
          actual, hands, unk, trump, desc),                                \
      golden_filename)

#define REQUIRE_GOLDEN_LINE_RESULT(...) \
  GET_REQUIRE_GOLDEN_MACRO(__VA_ARGS__, REQUIRE_GOLDEN_6, REQUIRE_GOLDEN_5, REQUIRE_GOLDEN_4, REQUIRE_GOLDEN_3, REQUIRE_GOLDEN_2)(__VA_ARGS__)
