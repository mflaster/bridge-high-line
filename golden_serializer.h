#pragma once

#include <catch2/catch_all.hpp>
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
  static std::string FormatHands(uint64_t hand);
  static std::string FormatHands(const std::vector<uint64_t> &hands);
  static std::string FormatHands(const std::pair<uint64_t, uint64_t> &hands);
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

class GoldenMatcher : public Catch::Matchers::MatcherBase<std::string> {
 public:
  explicit GoldenMatcher(std::string golden_filename);

  bool match(const std::string &actual) const override;
  std::string describe() const override;

 private:
  std::string golden_filename_;
  mutable std::string failure_message_;
};

#define GET_REQUIRE_GOLDEN_MACRO(_1, _2, _3, _4, NAME, ...) NAME

#define REQUIRE_GOLDEN_2(actual, golden_filename) \
  REQUIRE_THAT(PlayLineGoldenSerializer::SerializeWithHeader(actual), GoldenMatcher(golden_filename))

#define REQUIRE_GOLDEN_3(actual, golden_filename, arg3) \
  REQUIRE_THAT(PlayLineGoldenSerializer::SerializeWithHeader(actual, arg3), GoldenMatcher(golden_filename))

#define REQUIRE_GOLDEN_4(actual, golden_filename, hands, description) \
  REQUIRE_THAT(PlayLineGoldenSerializer::SerializeWithHeader(actual, hands, description), GoldenMatcher(golden_filename))

#define REQUIRE_GOLDEN_LINE_RESULT(...) \
  GET_REQUIRE_GOLDEN_MACRO(__VA_ARGS__, REQUIRE_GOLDEN_4, REQUIRE_GOLDEN_3, REQUIRE_GOLDEN_2)(__VA_ARGS__)

