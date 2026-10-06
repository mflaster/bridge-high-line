#include <iostream>
#include <catch2/catch_all.hpp>

#include "golden_serializer.h"
#include "conditional_line.h"

static std::string FormatGroupedTricks(
    const std::vector<std::vector<GeneralizedTrick>> &grouped_tricks) {
  std::string res;
  for (size_t i = 0; i < grouped_tricks.size(); ++i) {
    res += "Group " + std::to_string(i) + ":\n";
    for (const auto &trick : grouped_tricks[i]) {
      res += "  " + trick.ToString() + "\n";
    }
  }
  return res;
}

TEST_CASE("GroupTricks") {
  std::set<int> empty;
  int leader = 0;
  auto input = std::vector<GeneralizedTrick>({
      GeneralizedTrick(GeneralizedDemiStruct("__", "SK", empty),
                       GeneralizedDemiStruct("__", "S4", empty),
                       std::set<int>({0,1,2,3,4}),
                       leader)
    });
  std::vector<std::vector<GeneralizedTrick>> grouped_tricks =
      GroupTricks(input);
  REQUIRE(grouped_tricks.size() >= 1);
  REQUIRE(grouped_tricks[0].size() == 1);
  std::cout << "grouped_tricks[0] " << grouped_tricks[0][0].ToString() << "\n";
  REQUIRE_GOLDEN_LINE_RESULT(FormatGroupedTricks(grouped_tricks), "group_tricks_1.golden", "GroupTricks: grouping single trick");
}

TEST_CASE("GroupTricks2") {
  int leader = 0;
  std::set<int> empty;
  auto input = std::vector<GeneralizedTrick>({
      GeneralizedTrick(GeneralizedDemiStruct("Sx", "SK", empty),
                       GeneralizedDemiStruct("__", "S4", empty),
                       std::set<int>({0,1,2}),
                       leader),
      GeneralizedTrick(GeneralizedDemiStruct("Hx", "SK", empty),
                       GeneralizedDemiStruct("__", "S4", empty),
                       std::set<int>({3,4}),
                       leader),
    });
  std::vector<std::vector<GeneralizedTrick>> grouped_tricks =
      GroupTricks(input);
  for (const auto &grp : grouped_tricks) {
    std::cout << "See group:" << "\n";
    for (const auto &trick : grp) {
      std::cout << "   " << trick.ToString() << "\n";
    }
  }
  REQUIRE(grouped_tricks.size() == 1);
  REQUIRE(grouped_tricks[0].size() == 2);
  REQUIRE_GOLDEN_LINE_RESULT(FormatGroupedTricks(grouped_tricks), "group_tricks_2.golden", "GroupTricks2: grouping spade/heart tricks");
}

TEST_CASE("GroupTricks3") {
  int leader = 0;
  std::set<int> empty;
  auto input = std::vector<GeneralizedTrick>({
      GeneralizedTrick(GeneralizedDemiStruct("Sx", "SK", empty),
                       GeneralizedDemiStruct("__", "S4", empty),
                       std::set<int>({0}),
                       leader),
      GeneralizedTrick(GeneralizedDemiStruct("Hx", "SK", empty),
                       GeneralizedDemiStruct("__", "S4", empty),
                       std::set<int>({1}),
                       leader),
      GeneralizedTrick(GeneralizedDemiStruct("Hx", "SK", empty),
                       GeneralizedDemiStruct("__", "S5", empty),
                       std::set<int>({2}),
                       leader),
      GeneralizedTrick(GeneralizedDemiStruct("__", "SK", empty),
                       GeneralizedDemiStruct("__", "S6", empty),
                       std::set<int>({3}),
                       leader),
      GeneralizedTrick(GeneralizedDemiStruct("__", "SQ", empty),
                       GeneralizedDemiStruct("__", "S7", empty),
                       std::set<int>({4}),
                       leader),
      GeneralizedTrick(GeneralizedDemiStruct("Dx", "SK", empty),
                       GeneralizedDemiStruct("__", "SJ", empty),
                       std::set<int>({5}),
                       leader),
    });
  std::vector<std::vector<GeneralizedTrick>> grouped_tricks =
      GroupTricks(input);
  for (const auto &grp : grouped_tricks) {
    std::cout << "See group:" << "\n";
    for (const auto &trick : grp) {
      std::cout << "   " << trick.ToString() << "\n";
    }
  }
  REQUIRE(grouped_tricks.size() == 4);
  REQUIRE(grouped_tricks[0].size() == 3);
  REQUIRE(grouped_tricks[1].size() == 2);
  REQUIRE_GOLDEN_LINE_RESULT(FormatGroupedTricks(grouped_tricks), "group_tricks_3.golden", "GroupTricks3: grouping multiple trick types");
}

TEST_CASE("GroupTricks4") {
  int leader = 1;
  std::set<int> empty;
  auto input = std::vector<GeneralizedTrick>({
      GeneralizedTrick(GeneralizedDemiStruct("Sx", "SK", empty),
                       GeneralizedDemiStruct("__", "S4", empty),
                       std::set<int>({0}),
                       leader),
      GeneralizedTrick(GeneralizedDemiStruct("Hx", "SK", empty),
                       GeneralizedDemiStruct("Sx", "S4", empty),
                       std::set<int>({1}),
                       leader),
      GeneralizedTrick(GeneralizedDemiStruct("Hx", "SK", empty),
                       GeneralizedDemiStruct("Dx", "S5", empty),
                       std::set<int>({2}),
                       leader),
      GeneralizedTrick(GeneralizedDemiStruct("__", "SK", empty),
                       GeneralizedDemiStruct("__", "S6", empty),
                       std::set<int>({3}),
                       leader),
      GeneralizedTrick(GeneralizedDemiStruct("Cx", "SQ", empty),
                       GeneralizedDemiStruct("__", "S7", empty),
                       std::set<int>({4}),
                       leader),
      GeneralizedTrick(GeneralizedDemiStruct("Dx", "SK", empty),
                       GeneralizedDemiStruct("__", "SJ", empty),
                       std::set<int>({5}),
                       leader),
    });
  std::vector<std::vector<GeneralizedTrick>> grouped_tricks =
      GroupTricks(input);
  for (const auto &grp : grouped_tricks) {
    std::cout << "See group:" << "\n";
    for (const auto &trick : grp) {
      std::cout << "   " << trick.ToString() << "\n";
    }
  }
  REQUIRE(grouped_tricks.size() == 2);
  REQUIRE(grouped_tricks[0].size() == 5);
  REQUIRE(grouped_tricks[1].size() == 1);
  REQUIRE_GOLDEN_LINE_RESULT(FormatGroupedTricks(grouped_tricks), "group_tricks_4.golden", "GroupTricks4: grouping tricks led by West");
}

TEST_CASE("GroupTricksFinesse") {
  int leader = 0;
  std::set<int> empty;
  auto input = std::vector<GeneralizedTrick>({
      GeneralizedTrick(GeneralizedDemiStruct("Sx", "S2", empty),
                       GeneralizedDemiStruct("__", "SJ", empty),
                       std::set<int>({0}),
                       leader),
      GeneralizedTrick(GeneralizedDemiStruct("Sx", "S2", empty),
                       GeneralizedDemiStruct("SQ", "SJ", empty),
                       std::set<int>({1}),
                       leader),
      GeneralizedTrick(GeneralizedDemiStruct("Sx", "S2", empty),
                       GeneralizedDemiStruct("Sx", "SJ", empty),
                       std::set<int>({2}),
                       leader),
      GeneralizedTrick(GeneralizedDemiStruct("Hx", "S2", empty),
                       GeneralizedDemiStruct("Sx", "SK", empty),
                       std::set<int>({3}),
                       leader),
      GeneralizedTrick(GeneralizedDemiStruct("Hx", "S2", empty),
                       GeneralizedDemiStruct("SQ", "SK", empty),
                       std::set<int>({4}),
                       leader),
      GeneralizedTrick(GeneralizedDemiStruct("Sx", "S2", empty),
                       GeneralizedDemiStruct("SQ", "SK", empty),
                       std::set<int>({5}),
                       leader),
    });
  std::vector<std::vector<GeneralizedTrick>> grouped_tricks =
      GroupTricks(input);
  for (const auto &grp : grouped_tricks) {
    std::cout << "See group:" << "\n";
    for (const auto &trick : grp) {
      std::cout << "   " << trick.ToString() << "\n";
    }
  }
  REQUIRE(grouped_tricks.size() == 3);
  REQUIRE(grouped_tricks[0].size() == 3);
  REQUIRE(grouped_tricks[1].size() == 4);
  REQUIRE_GOLDEN_LINE_RESULT(FormatGroupedTricks(grouped_tricks), "group_tricks_finesse.golden", "GroupTricksFinesse: grouping finesse tricks");
}

TEST_CASE("GroupTricksFinesse2") {
  int leader = 0;
  std::set<int> empty;
  // Hands:
  // 0: Sx, SQ
  // 1: SQx, Sx
  // 2: SQ, Sx
  // 3: Hx, SQ
  // 4: Hx SQx
  auto input = std::vector<GeneralizedTrick>({
      GeneralizedTrick(GeneralizedDemiStruct("Sx", "S2", empty),
                       GeneralizedDemiStruct("__", "SJ", empty),
                       std::set<int>({0,1}),
                       leader),
      GeneralizedTrick(GeneralizedDemiStruct("Sx", "S2", empty),
                       GeneralizedDemiStruct("SQ", "SJ", empty),
                       std::set<int>({0}),
                       leader),
      GeneralizedTrick(GeneralizedDemiStruct("Sx", "S2", empty),
                       GeneralizedDemiStruct("Sx", "SJ", empty),
                       std::set<int>({1}),
                       leader),
      GeneralizedTrick(GeneralizedDemiStruct("SQ", "S2", empty),
                       GeneralizedDemiStruct("__", "SK", empty),
                       std::set<int>({2}),
                       leader),
      GeneralizedTrick(GeneralizedDemiStruct("Hx", "S2", empty),
                       GeneralizedDemiStruct("Sx", "SK", empty),
                       std::set<int>({4}),
                       leader),
      GeneralizedTrick(GeneralizedDemiStruct("Hx", "S2", empty),
                       GeneralizedDemiStruct("SQ", "SK", empty),
                       std::set<int>({3}),
                       leader),
      GeneralizedTrick(GeneralizedDemiStruct("Sx", "S2", empty),
                       GeneralizedDemiStruct("SQ", "SK", empty),
                       std::set<int>({0}),
                       leader),
    });
  std::vector<std::vector<GeneralizedTrick>> grouped_tricks =
      GroupTricks(input);
  for (const auto &grp : grouped_tricks) {
    std::cout << "See group:" << "\n";
    for (const auto &trick : grp) {
      std::cout << "   " << trick.ToString() << "\n";
    }
  }
  REQUIRE(grouped_tricks.size() == 2);
  REQUIRE(grouped_tricks[0].size() == 4);
  REQUIRE(grouped_tricks[1].size() == 5);
  REQUIRE_GOLDEN_LINE_RESULT(FormatGroupedTricks(grouped_tricks), "group_tricks_finesse_2.golden", "GroupTricksFinesse2: grouping complex finesse tricks");
}

TEST_CASE("ConditionalLineTotalBoardsFormat") {
  ConditionalLine line(std::set<int>({1, 2, 4, 5, 6}));
  ConditionalLine root;
  root.AddCondition(new WinnerPlayCondition(2), line);

  REQUIRE(root.Describe().find("Covers 5 [1, 2, 4, 5, 6]") == 0);
  REQUIRE(PlayLineGoldenSerializer::Serialize(root).find("Covers 5 [1, 2, 4, 5, 6]") == 0);

  root.set_total_boards(10);
  REQUIRE(root.Describe().find("Covers 5/10 (50.0%) [1, 2, 4, 5, 6]") == 0);
  REQUIRE(PlayLineGoldenSerializer::Serialize(root).find("Covers 5/10 (50.0%) [1, 2, 4, 5, 6]") == 0);
}

TEST_CASE("WinnerPlayConditionDescribeWithWinningCards") {
  WinnerPlayCondition cond1(0, std::set<std::string>{"SQ"});
  REQUIRE(cond1.Describe(true) == "when 0 wins the SQ");
  REQUIRE(cond1.Describe(false) == "if 0 wins the SQ");

  WinnerPlayCondition cond2(1, std::vector<std::string>{"SJ", "SK"});
  REQUIRE(cond2.Describe(true) == "when 1 wins the SJ,SK");
  REQUIRE(cond2.Describe(false) == "if 1 wins the SJ,SK");

  WinnerPlayCondition cond3(2);
  REQUIRE(cond3.Describe(true) == "when 2 wins");
  REQUIRE(cond3.Describe(false) == "if 2 wins");
}

