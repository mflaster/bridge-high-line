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

namespace {

BoardState MakeBoardState(const std::vector<uint64_t> &hands, int leader,
                          Suit trump) {
  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(leader);
  bs.SetTrump(trump);
  return bs;
}

// Builds a 2-card spade finesse line:
// North (hand 0): S32, South (hand 2): SAQ.
// Trick 1: Lead S2 from North, finesse SQ from South.
//   - If South (2) wins trick 1, cash SA from South.
//   - If West (3) wins trick 1, ruff/win trick 2 with SA in South.
ConditionalLine MakeFinesseLine(const BoardState &sample_bs) {
  std::set<int> deals = {0};

  GeneralizedTrick trick2_2wins(
      GeneralizedDemiStruct("__", "S3", deals),
      GeneralizedDemiStruct("__", "SA", deals), deals, 2);
  ConditionalLine leaf_2wins;
  leaf_2wins.SetAction(trick2_2wins);

  GeneralizedTrick trick2_3wins(
      GeneralizedDemiStruct("__", "SA", deals),
      GeneralizedDemiStruct("__", "S3", deals), deals, 3);
  ConditionalLine leaf_3wins;
  leaf_3wins.SetAction(trick2_3wins);

  GeneralizedTrick trick1(
      GeneralizedDemiStruct("__", "S2", deals),
      GeneralizedDemiStruct("__", "SQ", deals), deals, 0);
  ConditionalLine root;
  root.SetAction(trick1);
  root.AddCondition(new WinnerPlayCondition(2), leaf_2wins);
  root.AddCondition(new WinnerPlayCondition(3), leaf_3wins);
  root.set_min_tricks(2);
  root.set_sample_board(sample_bs);
  return root;
}

}  // namespace

TEST_CASE("ResultsFinesseOnsides") {
  // Hand 0 (North): S32
  // Hand 1 (East):  SK4 (King onside)
  // Hand 2 (South): SAQ
  // Hand 3 (West):  S65
  std::vector<uint64_t> hands = {
      ParseHand("S 32"),
      ParseHand("S K4"),
      ParseHand("S AQ"),
      ParseHand("S 65"),
  };
  BoardState bs = MakeBoardState(hands, 0, SPADES);
  ConditionalLine line = MakeFinesseLine(bs);

  REQUIRE(line.ResultsOnHand(bs) == 2);
  REQUIRE(line.ResultsOnHand(PairedDeal("S K4", "S 65")) == 2);
}

TEST_CASE("ResultsFinesseOffsides") {
  // Hand 0 (North): S32
  // Hand 1 (East):  S65
  // Hand 2 (South): SAQ
  // Hand 3 (West):  SK4 (King offside)
  std::vector<uint64_t> hands_sample = {
      ParseHand("S 32"),
      ParseHand("S K4"),
      ParseHand("S AQ"),
      ParseHand("S 65"),
  };
  BoardState sample_bs = MakeBoardState(hands_sample, 0, SPADES);
  ConditionalLine line = MakeFinesseLine(sample_bs);

  std::vector<uint64_t> hands_offside = {
      ParseHand("S 32"),
      ParseHand("S 65"),
      ParseHand("S AQ"),
      ParseHand("S K4"),
  };
  BoardState bs_offside = MakeBoardState(hands_offside, 0, SPADES);

  REQUIRE(line.ResultsOnHand(bs_offside) == 1);
  REQUIRE(line.ResultsOnHand(PairedDeal("S 65", "S K4")) == 1);
}

