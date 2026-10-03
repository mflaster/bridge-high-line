#pragma once

#include <catch2/catch_all.hpp>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "conditional_line.h"
#include "solver.h"

class PlayLineGoldenSerializer {
 public:
  static std::string Serialize(const ConditionalLine &line);
  static std::string Serialize(const std::vector<ConditionalLine> &lines);
  static std::string Serialize(const std::multiset<int> &tricks_won);
  static std::string Serialize(const std::string &str);
  static std::string Serialize(const OptimalLines &lines);

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

#define REQUIRE_GOLDEN_LINE_RESULT(actual, golden_filename) \
  REQUIRE_THAT(PlayLineGoldenSerializer::Serialize(actual), GoldenMatcher(golden_filename))
