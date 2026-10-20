#include <iostream>
#include <gtest/gtest.h>

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

TEST(ConditionalLineTest, GroupTricks) {
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
  ASSERT_TRUE(grouped_tricks.size() >= 1);
  ASSERT_TRUE(grouped_tricks[0].size() == 1);
  std::cout << "grouped_tricks[0] " << grouped_tricks[0][0].ToString() << "\n";
  REQUIRE_GOLDEN_LINE_RESULT(FormatGroupedTricks(grouped_tricks), "group_tricks_1.golden", "GroupTricks: grouping single trick");
}

TEST(ConditionalLineTest, GroupTricks2) {
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
  ASSERT_TRUE(grouped_tricks.size() == 2);
  ASSERT_TRUE(grouped_tricks[0].size() == 1);
  ASSERT_TRUE(grouped_tricks[1].size() == 1);
  REQUIRE_GOLDEN_LINE_RESULT(FormatGroupedTricks(grouped_tricks), "group_tricks_2.golden", "GroupTricks2: grouping spade/heart tricks");
}

TEST(ConditionalLineTest, GroupTricks3) {
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
  ASSERT_TRUE(grouped_tricks.size() == 4);
  ASSERT_TRUE(grouped_tricks[0].size() == 3);
  ASSERT_TRUE(grouped_tricks[1].size() == 2);
  REQUIRE_GOLDEN_LINE_RESULT(FormatGroupedTricks(grouped_tricks), "group_tricks_3.golden", "GroupTricks3: grouping multiple trick types");
}

TEST(ConditionalLineTest, GroupTricks4) {
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
  ASSERT_TRUE(grouped_tricks.size() == 2);
  ASSERT_TRUE(grouped_tricks[0].size() == 5);
  ASSERT_TRUE(grouped_tricks[1].size() == 1);
  REQUIRE_GOLDEN_LINE_RESULT(FormatGroupedTricks(grouped_tricks), "group_tricks_4.golden", "GroupTricks4: grouping tricks led by West");
}

TEST(ConditionalLineTest, GroupTricksFinesse) {
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
  ASSERT_TRUE(grouped_tricks.size() == 5);
  ASSERT_TRUE(grouped_tricks[0].size() == 2);
  ASSERT_TRUE(grouped_tricks[1].size() == 2);
  REQUIRE_GOLDEN_LINE_RESULT(FormatGroupedTricks(grouped_tricks), "group_tricks_finesse.golden", "GroupTricksFinesse: grouping finesse tricks");
}

TEST(ConditionalLineTest, GroupTricksFinesse2) {
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
  ASSERT_TRUE(grouped_tricks.size() == 4);
  ASSERT_TRUE(grouped_tricks[0].size() == 2);
  ASSERT_TRUE(grouped_tricks[1].size() == 1);
  ASSERT_TRUE(grouped_tricks[2].size() == 1);
  REQUIRE_GOLDEN_LINE_RESULT(FormatGroupedTricks(grouped_tricks), "group_tricks_finesse_2.golden", "GroupTricksFinesse2: grouping complex finesse tricks");
}

TEST(ConditionalLineTest, ConditionalLineTotalBoardsFormat) {
  ConditionalLine line(std::set<int>({1, 2, 4, 5, 6}));
  ConditionalLine root;
  root.AddCondition(new WinnerPlayCondition(2), line);

  ASSERT_TRUE(root.Describe().find("Covers 5 [1, 2, 4, 5, 6]") == 0);
  ASSERT_TRUE(PlayLineGoldenSerializer::Serialize(root).find("Covers 5 [1, 2, 4, 5, 6]") == 0);

  root.set_total_boards(10);
  ASSERT_TRUE(root.Describe().find("Covers 5/10 (50.0%) [1, 2, 4, 5, 6]") == 0);
  ASSERT_TRUE(PlayLineGoldenSerializer::Serialize(root).find("Covers 5/10 (50.0%) [1, 2, 4, 5, 6]") == 0);
}

TEST(ConditionalLineTest, WinnerPlayConditionDescribeWithWinningCards) {
  WinnerPlayCondition cond1(0, std::set<std::string>{"SQ"});
  ASSERT_TRUE(cond1.Describe(true) == "when 0 wins the SQ");
  ASSERT_TRUE(cond1.Describe(false) == "if 0 wins the SQ");

  WinnerPlayCondition cond2(1, std::vector<std::string>{"SJ", "SK"});
  ASSERT_TRUE(cond2.Describe(true) == "when 1 wins the SJ,SK");
  ASSERT_TRUE(cond2.Describe(false) == "if 1 wins the SJ,SK");

  WinnerPlayCondition cond3(2);
  ASSERT_TRUE(cond3.Describe(true) == "when 2 wins");
  ASSERT_TRUE(cond3.Describe(false) == "if 2 wins");
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

TEST(ConditionalLineTest, ResultsFinesseOnsides) {
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

  ASSERT_TRUE(line.ResultsOnHand(bs) == 2);
  ASSERT_TRUE(line.ResultsOnHand(PairedDeal("S K4", "S 65")) == 2);
}

TEST(ConditionalLineTest, ResultsFinesseOffsides) {
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

  ASSERT_TRUE(line.ResultsOnHand(bs_offside) == 1);
  ASSERT_TRUE(line.ResultsOnHand(PairedDeal("S 65", "S K4")) == 1);
}

TEST(ConditionalLineTest, ConditionalLineDescribeSortBranches) {
  ConditionalLine line_b(std::set<int>{2});
  ConditionalLine line_a(std::set<int>{1});

  ConditionalLine root;
  // Add conditions in reverse alphabetical order: 1 (West) wins first, then 0 (North) wins.
  root.AddCondition(new WinnerPlayCondition(1), line_b);
  root.AddCondition(new WinnerPlayCondition(0), line_a);

  std::string unsorted = root.Describe(0, /*sort_branches=*/false);
  std::string sorted = root.Describe(0, /*sort_branches=*/true);

  // Unsorted preserves insertion order (if 1 wins comes before if 0 wins)
  ASSERT_TRUE(unsorted.find("if 1 wins") < unsorted.find("if 0 wins"));

  // Sorted reorders branches deterministically (if 0 wins comes before if 1 wins)
  ASSERT_TRUE(sorted.find("if 0 wins") < sorted.find("if 1 wins"));

  // Verify PlayLineGoldenSerializer delegates to Describe with sort_branches = true
  ASSERT_TRUE(PlayLineGoldenSerializer::Serialize(root) ==
          PlayLineGoldenSerializer::Serialize(sorted));
}

TEST(ConditionalLineTest, IsCardGoodConditionEvaluateAndResults) {
  CardType hk = Card::ParseStringCard("HK");
  IsCardGoodCondition cond_good(hk, true);
  IsCardGoodCondition cond_not_good(hk, false);

  ASSERT_TRUE(cond_good.Describe(false) == "if HK is good");
  ASSERT_TRUE(cond_good.Describe(true) == "when HK is good");
  ASSERT_TRUE(cond_not_good.Describe(false) == "if HK is not good");
  ASSERT_TRUE(cond_not_good.Describe(true) == "when HK is not good");

  std::vector<uint64_t> hands = {
      ParseHand("S A3"),
      ParseHand("S J H Q"),
      ParseHand("S 2 H K"),
      ParseHand("S T H A"),
  };
  BoardState bs = MakeBoardState(hands, 2, NOTRUMP);
  // HA is unplayed in hand 3, so HK is not good.
  ASSERT_TRUE(!cond_good.Evaluate(bs));

  // Build a ConditionalLine at 2-card ending with South (2) on lead:
  // if HK is good -> play HK (throwing S3), then S2 to SA
  // if HK is not good -> play S2 to SA, then S3 (throwing HK)
  std::set<int> deals = {0};
  GeneralizedTrick cash_hk(
      GeneralizedDemiStruct("__", "HK", deals),
      GeneralizedDemiStruct("__", "S3", deals), deals, 2);
  GeneralizedTrick s2_to_sa(
      GeneralizedDemiStruct("__", "S2", deals),
      GeneralizedDemiStruct("__", "SA", deals), deals, 2);
  GeneralizedTrick cash_s3(
      GeneralizedDemiStruct("__", "S3", deals),
      GeneralizedDemiStruct("__", "HK", deals), deals, 0);

  ConditionalLine leaf_sa;
  leaf_sa.SetAction(s2_to_sa);
  ConditionalLine branch_good;
  branch_good.SetAction(cash_hk);
  branch_good.AddCondition(new WinnerPlayCondition(2), leaf_sa);

  ConditionalLine leaf_s3;
  leaf_s3.SetAction(cash_s3);
  ConditionalLine branch_not_good;
  branch_not_good.SetAction(s2_to_sa);
  branch_not_good.AddCondition(new WinnerPlayCondition(0), leaf_s3);

  ConditionalLine root;
  root.AddCondition(new IsCardGoodCondition(hk, true), branch_good);
  root.AddCondition(new IsCardGoodCondition(hk, false), branch_not_good);
  root.set_min_tricks(2);
  root.set_sample_board(bs);

  // When HA is still held by East (hand 3) and spades are 1-1, branch_not_good
  // plays S2 to SA (felling SJ and ST) and cashes S3 for 2 tricks.
  ASSERT_TRUE(root.ResultsOnHand(bs) == 2);

  // When HA has already been discarded by East (e.g., East holds S T9 instead
  // of S T H A), HK is good and branch_good cashes HK then S2 to SA for 2
  // tricks (whereas branch_not_good would only win 1 trick).
  std::vector<uint64_t> hands_ha_gone = {
      ParseHand("S A3"),
      ParseHand("S J H Q"),
      ParseHand("S 2 H K"),
      ParseHand("S T9"),
  };
  BoardState bs_ha_gone = MakeBoardState(hands_ha_gone, 2, NOTRUMP);
  ASSERT_TRUE(cond_good.Evaluate(bs_ha_gone));
  ASSERT_TRUE(root.ResultsOnHand(bs_ha_gone) == 2);
}

TEST(ConditionalLineTest, GeneralizedDemiPlayDiscardNoSuitBranching) {
  GeneralizedDemiPlay demi(HEARTS);
  demi.RecordOptimalPlay("D4", "SA", 0);
  demi.RecordOptimalPlay("CQ", "SA", 1);

  GeneralizedPlaysVector plays = demi.GetGeneralizedPlays(0, SPADES, 20);
  for (const auto &p : plays) {
    ASSERT_TRUE((p.opp_card == "R_" || p.opp_card == "__"));
    ASSERT_EQ(p.boards, (std::set<int>{0, 1}));
  }
}

TEST(ConditionalLineTest, GeneralizedTrickFinderDefenderLeadConsolidation) {
  GeneralizedTrickFinder gtf(HEARTS);
  // West (3) is on lead.
  // Spades: on boards 0, 1, 2, West leads S4, SJ, S6 and East plays S4, SQ, ST
  // (never SA), South plays SK. On board 3, West leads S4, East plays SA, and
  // South ducks with S9.
  gtf.RecordOptimalTrick(3, "S4 S5 S6 SK", 0);
  gtf.RecordOptimalTrick(3, "SJ S5 SQ SK", 1);
  gtf.RecordOptimalTrick(3, "S6 S5 ST SK", 2);
  gtf.RecordOptimalTrick(3, "S4 S5 SA S9", 3);

  // Clubs: on boards 4, 5, West leads low club (C4, C8) and South plays CJ.
  // On board 6, West leads club honor (CQ) and South covers with CA.
  // On board 4, playing CA is also optimal at this trick (creating an
  // overarching (C_) C6 (C_) CA [4,6] trick that overlaps board 4).
  gtf.RecordOptimalTrick(3, "C4 C6 C3 CJ", 4);
  gtf.RecordOptimalTrick(3, "C8 C6 CT CJ", 5);
  gtf.RecordOptimalTrick(3, "CQ C6 C3 CA", 6);
  gtf.RecordOptimalTrick(3, "C4 C6 C3 CA", 4);

  GeneralizedTricksVector tricks = gtf.GetGeneralizedTricks(50);

  // Verify that when West's lead pattern (S_) and (Sx) produce the same d2
  // board set (e.g. board 3 with (SA) S9), the more general (S_) pattern is
  // retained rather than (Sx).
  bool found_s_any_sa = false;
  bool found_s_low_sa = false;
  for (const auto &gt : tricks) {
    if (gt.demi2.opp_card == "SA" && gt.demi2.my_card == "S9") {
      if (gt.demi1.opp_card == "S_") found_s_any_sa = true;
      if (gt.demi1.opp_card == "Sx") found_s_low_sa = true;
    }
  }
  ASSERT_TRUE(found_s_any_sa);
  ASSERT_TRUE(!found_s_low_sa);

  // Verify that GroupTricks produces a 4-branch group covering all 7 boards:
  // (S_) S5 (S_) SK [0,1,2], (S_) S5 (SA) S9 [3], (Cx) C6 (C_) CJ [4,5],
  // and (CX) C6 (Cx) CA [6].
  std::vector<std::vector<GeneralizedTrick>> groups = GroupTricks(tricks);
  bool found_consolidated_group = false;
  for (const auto &grp : groups) {
    std::set<int> covered;
    for (const auto &gt : grp) {
      covered.insert(gt.boards.begin(), gt.boards.end());
    }
    if (covered.size() == 7 && grp.size() == 4) {
      found_consolidated_group = true;
      break;
    }
  }
  ASSERT_TRUE(found_consolidated_group);
}

TEST(ConditionalLineTest, SecondSeatGeneralPatternNotPrunedBeforeFourthSeat) {
  // Models the Restricted Choice Trick 1 multi-variant split:
  //   Variant 0: SA (S_) S6 (Sx) -> Trick 2: ST (SX) SK (S_) -> Trick 3: S5 S9
  //   Variant 1: SA (Sx) S6 (SJ) -> Trick 2: ST (Sx) S5 (__) -> Trick 3: S9 SK
  //   Variant 2: SA (Sx) S6 (SQ) -> Trick 2: ST (Sx) S5 (__) -> Trick 3: S9 SK
  // When East plays a low spade (S3) on SA, Variant 0's (S_) at seat 1 is less
  // specific than Variants 1 & 2's (Sx), but all three play S6 from dummy at
  // seat 2. Variant 0 must not be pruned at seat 1 before West's card at
  // seat 3 is seen.
  std::set<int> deals = {0};

  GeneralizedTrick t3_from_south(
      GeneralizedDemiStruct("__", "S5", deals),
      GeneralizedDemiStruct("__", "S9", deals), deals, 2);
  ConditionalLine leaf_t3_south;
  leaf_t3_south.SetAction(t3_from_south);

  GeneralizedTrick t3_from_north(
      GeneralizedDemiStruct("__", "S9", deals),
      GeneralizedDemiStruct("__", "SK", deals), deals, 0);
  ConditionalLine leaf_t3_north;
  leaf_t3_north.SetAction(t3_from_north);

  // Trick 2 under Variant 0: ST (SX) SK (S_)
  GeneralizedTrick t2_drop(
      GeneralizedDemiStruct("SX", "ST", deals),
      GeneralizedDemiStruct("S_", "SK", deals), deals, 0);
  ConditionalLine node_t2_drop;
  node_t2_drop.SetAction(t2_drop);
  node_t2_drop.AddCondition(new WinnerPlayCondition(2), leaf_t3_south);

  // Trick 2 under Variants 1 & 2: ST (Sx) S5 (__)
  GeneralizedTrick t2_finesse(
      GeneralizedDemiStruct("Sx", "ST", deals),
      GeneralizedDemiStruct("__", "S5", deals), deals, 0);
  ConditionalLine node_t2_finesse;
  node_t2_finesse.SetAction(t2_finesse);
  node_t2_finesse.AddCondition(new WinnerPlayCondition(0), leaf_t3_north);

  // Trick 1 variants
  GeneralizedTrick t1_west_low(
      GeneralizedDemiStruct("S_", "SA", deals),
      GeneralizedDemiStruct("Sx", "S6", deals), deals, 0);
  ConditionalLine var_west_low;
  var_west_low.SetAction(t1_west_low);
  var_west_low.AddCondition(new WinnerPlayCondition(0), node_t2_drop);

  GeneralizedTrick t1_west_sj(
      GeneralizedDemiStruct("Sx", "SA", deals),
      GeneralizedDemiStruct("SJ", "S6", deals), deals, 0);
  ConditionalLine var_west_sj;
  var_west_sj.SetAction(t1_west_sj);
  var_west_sj.AddCondition(new WinnerPlayCondition(0), node_t2_finesse);

  GeneralizedTrick t1_west_sq(
      GeneralizedDemiStruct("Sx", "SA", deals),
      GeneralizedDemiStruct("SQ", "S6", deals), deals, 0);
  ConditionalLine var_west_sq;
  var_west_sq.SetAction(t1_west_sq);
  var_west_sq.AddCondition(new WinnerPlayCondition(0), node_t2_finesse);

  ConditionalLine root;
  root.AddCondition(new TrickPlayCondition("SA (S_) S6 (Sx)"), var_west_low);
  root.AddCondition(new TrickPlayCondition("SA (Sx) S6 (SJ)"), var_west_sj);
  root.AddCondition(new TrickPlayCondition("SA (Sx) S6 (SQ)"), var_west_sq);
  root.set_min_tricks(3);

  // Case 1: Making 2-2 deal where East has J3 and West has Q2.
  // East plays S3 and West plays S2 on Trick 1; must match SA (S_) S6 (Sx)
  // and rise with SK on Trick 2 to win all 3 tricks.
  BoardState bs_2_2 = MakeBoardState(
      {ParseHand("S AT9"), ParseHand("S J3 C 2"),
       ParseHand("S K65"), ParseHand("S Q2 C 3")},
      0, SPADES);
  root.set_sample_board(bs_2_2);
  ASSERT_TRUE(root.ResultsOnHand(bs_2_2) == 3);
  auto trace_2_2 = root.TraceOnBoard(bs_2_2, 0);
  ASSERT_TRUE(trace_2_2.final_ns_tricks == 3);
  ASSERT_TRUE(trace_2_2.steps.size() == 3);
  ASSERT_TRUE(trace_2_2.steps[0].unmatched_seat == -1);
  ASSERT_TRUE(trace_2_2.steps[0].node->do_first().ToString(true) ==
          "SA (S_) S6 (Sx)");

  // Case 2: Failed 3-1 deal where East has QJ3 and West has singleton S2.
  // East plays S3 and West plays S2 on Trick 1. Trick 1 must still match
  // SA (S_) S6 (Sx) rather than reporting West's S2 as an unmatched play on
  // Trick 1; the failure instead occurs on Trick 2 when West discards.
  BoardState bs_3_1_fail = MakeBoardState(
      {ParseHand("S AT9"), ParseHand("S QJ3"),
       ParseHand("S K65"), ParseHand("S 2 C 32")},
      0, SPADES);
  auto trace_fail = root.TraceOnBoard(bs_3_1_fail, 1);
  ASSERT_TRUE(trace_fail.final_ns_tricks == 2);
  ASSERT_TRUE(trace_fail.steps.size() == 2);
  ASSERT_TRUE(trace_fail.steps[0].unmatched_seat == -1);
  ASSERT_TRUE(trace_fail.steps[0].node->do_first().ToString(true) ==
          "SA (S_) S6 (Sx)");
  ASSERT_TRUE(trace_fail.steps[1].unmatched_seat == 3);

  // Case 3: Making 3-1 deal where East has Q32 and West has singleton SJ.
  // East plays S2 and West drops SJ on Trick 1; matches SA (Sx) S6 (SJ) and
  // finesses ST on Trick 2 to win all 3 tricks.
  BoardState bs_3_1_make = MakeBoardState(
      {ParseHand("S AT9"), ParseHand("S Q32"),
       ParseHand("S K65"), ParseHand("S J C 32")},
      0, SPADES);
  ASSERT_TRUE(root.ResultsOnHand(bs_3_1_make) == 3);
  auto trace_make = root.TraceOnBoard(bs_3_1_make, 2);
  ASSERT_TRUE(trace_make.final_ns_tricks == 3);
  ASSERT_TRUE(trace_make.steps.size() == 3);
  ASSERT_TRUE(trace_make.steps[0].unmatched_seat == -1);
  ASSERT_TRUE(trace_make.steps[0].node->do_first().ToString(true) ==
          "SA (Sx) S6 (SJ)");
}

TEST(ConditionalLineTest, AdversarialDefenderUsesStep1Denominator) {
  // 3-card miniature of 9-Card Hearts club suit:
  // South (0): C 6, D 2, S 2
  // East  (1): C QT4
  // North (2): C AKJ
  // West  (3): C 98, S A
  // Trump: NOTRUMP. Against omniscient declarer, C4, CT, and CQ all allow
  // 3 tricks (0 for defense), so all three are DDS-optimal for East.
  BoardState bs = MakeBoardState(
      {ParseHand("C 6 D 2 S 2"), ParseHand("C QT4"),
       ParseHand("C AKJ"), ParseHand("C 98 S A")},
      0, NOTRUMP);

  GeneralizedTrickFinder finder(NOTRUMP);
  for (int b = 0; b < 10; ++b) {
    finder.RecordOptimalTrick(0, "C6 C4 CJ C8", b);
  }
  for (int b = 10; b < 14; ++b) {
    finder.RecordOptimalTrick(0, "C6 CT CA C8", b);
    finder.RecordOptimalTrick(0, "C6 CT CJ C8", b);
  }
  for (int b = 14; b < 16; ++b) {
    finder.RecordOptimalTrick(0, "C6 CT CJ C8", b);
  }
  for (int b = 16; b < 18; ++b) {
    finder.RecordOptimalTrick(0, "C6 CQ CA C8", b);
  }

  GeneralizedTricksVector tricks = finder.GetGeneralizedTricks(50, true);
  GeneralizedTrick gt_cx_cj;
  GeneralizedTrick gt_ct_ca;
  GeneralizedTrick gt_cq_ca;
  for (const auto &gt : tricks) {
    if (gt.demi1.my_card == "C6" && gt.demi1.opp_card == "Cx" &&
        gt.demi2.my_card == "CJ" && gt.demi2.opp_card == "C_") {
      gt_cx_cj = gt;
    } else if (gt.demi1.my_card == "C6" && gt.demi1.opp_card == "CT" &&
               gt.demi2.my_card == "CA" && gt.demi2.opp_card == "C_") {
      gt_ct_ca = gt;
    } else if (gt.demi1.my_card == "C6" && gt.demi1.opp_card == "CQ" &&
               gt.demi2.my_card == "CA" && gt.demi2.opp_card == "C_") {
      gt_cq_ca = gt;
    }
  }
  ASSERT_TRUE(gt_cx_cj.NumBoards() == 10);
  ASSERT_TRUE(gt_cx_cj.opp1_step1_boards == 10);
  ASSERT_TRUE(gt_ct_ca.NumBoards() == 4);
  ASSERT_TRUE(gt_ct_ca.opp1_step1_boards == 6);
  ASSERT_TRUE(gt_cq_ca.NumBoards() == 2);
  ASSERT_TRUE(gt_cq_ca.opp1_step1_boards == 2);

  std::set<int> b_cx = gt_cx_cj.boards;
  std::set<int> b_ct = gt_ct_ca.boards;
  std::set<int> b_cq = gt_cq_ca.boards;

  ConditionalLine leaf_cx(b_cx);
  ConditionalLine t3_cx;
  t3_cx.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("__", "CK", b_cx),
      GeneralizedDemiStruct("__", "D2", b_cx), b_cx, 2));
  t3_cx.AddCondition(new WinnerPlayCondition(2), leaf_cx);
  ConditionalLine t2_cx;
  t2_cx.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("__", "CA", b_cx),
      GeneralizedDemiStruct("__", "S2", b_cx), b_cx, 2));
  t2_cx.AddCondition(new WinnerPlayCondition(2), t3_cx);
  ConditionalLine t1_cx;
  t1_cx.SetAction(gt_cx_cj);
  t1_cx.AddCondition(new WinnerPlayCondition(2), t2_cx);

  ConditionalLine leaf_ct(b_ct);
  ConditionalLine t3_ct;
  t3_ct.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("__", "CJ", b_ct),
      GeneralizedDemiStruct("__", "D2", b_ct), b_ct, 2));
  t3_ct.AddCondition(new WinnerPlayCondition(2), leaf_ct);
  ConditionalLine t2_ct;
  t2_ct.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("__", "CK", b_ct),
      GeneralizedDemiStruct("__", "S2", b_ct), b_ct, 2));
  t2_ct.AddCondition(new WinnerPlayCondition(2), t3_ct);
  ConditionalLine t1_ct;
  t1_ct.SetAction(gt_ct_ca);
  t1_ct.AddCondition(new WinnerPlayCondition(2), t2_ct);

  ConditionalLine leaf_cq(b_cq);
  ConditionalLine t3_cq;
  t3_cq.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("__", "CJ", b_cq),
      GeneralizedDemiStruct("__", "D2", b_cq), b_cq, 2));
  t3_cq.AddCondition(new WinnerPlayCondition(2), leaf_cq);
  ConditionalLine t2_cq;
  t2_cq.SetAction(GeneralizedTrick(
      GeneralizedDemiStruct("__", "CK", b_cq),
      GeneralizedDemiStruct("__", "S2", b_cq), b_cq, 2));
  t2_cq.AddCondition(new WinnerPlayCondition(2), t3_cq);
  ConditionalLine t1_cq;
  t1_cq.SetAction(gt_cq_ca);
  t1_cq.AddCondition(new WinnerPlayCondition(2), t2_cq);

  ConditionalLine root;
  root.AddCondition(new TrickPlayCondition("C6 (Cx) CJ (C_)"), t1_cx);
  root.AddCondition(new TrickPlayCondition("C6 (CT) CA (C_)"), t1_ct);
  root.AddCondition(new TrickPlayCondition("C6 (CQ) CA (C_)"), t1_cq);
  root.set_min_tricks(3);
  root.set_sample_board(bs);

  // Because C6 (CT) CA only solved 4/6 Step-1 CT boards (66.7%) vs 10/10
  // (100%) for C6 (Cx) CJ, adversarial East plays CT from C QT4 instead of
  // C4, forcing declarer to rise with CA and setting up East's CQ.
  ASSERT_TRUE(root.ResultsOnHandAdversarial(bs) == 2);

  // Before rescoring, Step-1 TraceOnBoard plays C4 and wins 3 tricks, and
  // board 0 is in the Cx leaf.
  ASSERT_TRUE(root.ResultsOnHand(bs) == 3);
  ASSERT_TRUE(root.TraceOnBoard(bs, 0).final_ns_tricks == 3);
  ASSERT_TRUE(root.CoveredFinalBoards() == 16);

  // Board 1: East holds C 954 and West holds C 87 S A, so C6 (Cx) CJ makes
  // 3 tricks even under adversarial defense.
  BoardState bs_make = MakeBoardState(
      {ParseHand("C 6 D 2 S 2"), ParseHand("C 954"),
       ParseHand("C AKJ"), ParseHand("C 87 S A")},
      0, NOTRUMP);

  root.DeepCloneChildren();
  ConditionalLine::RescoreBoardTrace trace0;
  ASSERT_TRUE(root.ResultsOnHandAdversarial(bs, nullptr, &trace0) == 2);
  ConditionalLine::RescoreBoardTrace trace1;
  ASSERT_TRUE(root.ResultsOnHandAdversarial(bs_make, nullptr, &trace1) == 3);
  ASSERT_TRUE(trace1.leaf != nullptr);

  std::unordered_map<const ConditionalLine *, std::set<int>> leaf_boards;
  leaf_boards[trace1.leaf] = {1};
  root.ApplyRescoredBoards(leaf_boards);

  ASSERT_TRUE(root.has_rescored_boards());
  ASSERT_TRUE(root.step1_covered_boards() == 16);
  ASSERT_TRUE(root.CoveredFinalBoards() == 1);
  ASSERT_TRUE(root.CoveredBoardsSet() == std::set<int>{1});
  ASSERT_TRUE(trace1.leaf->final_covered_boards() == std::set<int>{1});

  // After rescoring, TraceOnBoard on failed board 0 must also use the
  // adversarial defender (East plays CT, winning only 2 tricks), never
  // replaying the Step-1 C4 path to 3 tricks.
  auto trace_fail = root.TraceOnBoard(bs, 0);
  ASSERT_TRUE(trace_fail.final_ns_tricks == 2);
  ASSERT_TRUE(!trace_fail.steps.empty());
  ASSERT_TRUE(trace_fail.steps[0].trick_str.rfind("C6 CT ", 0) == 0);
}

TEST(ConditionalLineTest, AdversarialDefenderCountsUnmakeableBoards) {
  // 5-card Trick-2 position from simple_9_card_hearts (West = 3 on lead, H
  // trump, N/S need 4 tricks).
  // North (0): S 5432, H A
  // South (2): S K6, C AK2
  // On bs_make (East has SA, West has SQJ and C T98), both C8 and SJ are
  // DDS-optimal for West (each holds N/S to 4 tricks).
  // On bs_unmakeable (West has SA and C T98, East has SQJ9), only a club lead
  // holds N/S to 3 tricks; a spade lead lets N/S win 4 tricks.
  BoardState bs_make = MakeBoardState(
      {ParseHand("S 5432 H A"), ParseHand("S AT9 C 76"),
       ParseHand("S K6 C AK2"), ParseHand("S QJ C T98")},
      3, HEARTS);
  BoardState bs_unmakeable = MakeBoardState(
      {ParseHand("S 5432 H A"), ParseHand("S QJ9 C 76"),
       ParseHand("S K6 C AK2"), ParseHand("S AT C T98")},
      3, HEARTS);

  GeneralizedTrickFinder finder(HEARTS);
  for (int b = 0; b < 2; ++b) {
    finder.RecordOptimalTrick(3, "C8 S2 C6 CA", b);
  }
  for (int b = 2; b < 5; ++b) {
    finder.RecordOptimalTrick(3, "CT S2 C6 CA", b);
  }
  for (int b = 5; b < 7; ++b) {
    finder.RecordOptimalTrick(3, "SJ S2 SA S6", b);
  }
  for (int b = 7; b < 10; ++b) {
    finder.RecordOptimalTrick(3, "S9 S2 SA S6", b);
  }
  GeneralizedTricksVector tricks = finder.GetGeneralizedTricks(50, true);
  GeneralizedTrick gt_club;
  GeneralizedTrick gt_spade;
  for (const auto &gt : tricks) {
    if (gt.demi1.opp_card == "C_" && gt.demi1.my_card == "S2") {
      gt_club = gt;
    } else if (gt.demi1.opp_card == "S_" && gt.demi1.my_card == "S2") {
      gt_spade = gt;
    }
  }
  ASSERT_TRUE(gt_club.opp1_step1_boards == 5);
  ASSERT_TRUE(gt_spade.opp1_step1_boards == 5);

  ConditionalLine leaf_club(gt_club.boards);
  ConditionalLine node_club;
  node_club.SetAction(gt_club);
  node_club.AddCondition(new WinnerPlayCondition(2), leaf_club);

  ConditionalLine leaf_spade(gt_spade.boards);
  ConditionalLine node_spade;
  node_spade.SetAction(gt_spade);
  node_spade.AddCondition(new WinnerPlayCondition(1), leaf_spade);

  ConditionalLine root;
  root.AddCondition(new TrickPlayCondition("(C_) S2 (C_) CA"), node_club);
  root.AddCondition(new TrickPlayCondition("(S_) S2 (SA) S6"), node_spade);
  root.set_min_tricks(4);
  root.set_sample_board(bs_make);
  root.DeepCloneChildren();

  // Trace the unmakeable board (not in Step 1's solvable set) and apply its
  // Step-1 defender choices to the denominator counts.
  ConditionalLine::Step1DefenderCardCache step1_cache;
  ConditionalLine::UnmakeableBoardTrace unmakeable_trace;
  root.TraceUnmakeableBoard(bs_unmakeable, /*board_num=*/99,
                            /*in_step1_set=*/false, &step1_cache,
                            &unmakeable_trace);
  ASSERT_TRUE(unmakeable_trace.opp1_nodes.size() == 1);

  std::unordered_map<const ConditionalLine *, size_t> opp1_counts;
  std::unordered_map<const ConditionalLine *, size_t> opp2_counts;
  std::unordered_map<const ConditionalLine *, std::array<size_t, 4>>
      winner_counts;
  for (const ConditionalLine *n : unmakeable_trace.opp1_nodes) {
    ++opp1_counts[n];
  }
  root.ApplyUnmakeableBoardCounts(opp1_counts, opp2_counts, winner_counts);

  // With the unmakeable board counted on (C_), (C_) has 5/6 success vs 5/5 on
  // (S_), so adversarial West leads a club on bs_make and reaches leaf_club.
  ConditionalLine::RescoreBoardTrace rescore_trace;
  root.ResultsOnHandAdversarial(bs_make, nullptr, &rescore_trace);
  ASSERT_TRUE(rescore_trace.leaf != nullptr);
  ASSERT_TRUE(rescore_trace.leaf->step1_covered_boards() == 5);
  ASSERT_TRUE(opp1_counts.begin()->first->do_first().demi1.opp_card == "C_");
  ASSERT_TRUE(rescore_trace.leaf ==
          opp1_counts.begin()->first->conditions()[0].second.get());
}

TEST(ConditionalLineTest,
     DefenderLeadSingleSuitPatternRelaxedAgainstWildcardSibling) {
  // Mirrors Trick #6 in 9-Card Hearts where East is on lead:
  // Sibling 1: (SX) SK (S_) S5 -> if S wins, continue to next trick.
  // Sibling 2: (__) D5 (__) HT -> if N wins (ruffing in dummy), continue.
  // When East holds a small spade S4 (which wouldn't match SX), DeepCloneChildren
  // relaxes (SX) to (S_) because it is the only S-lead branch alongside (__),
  // and AreCandidateDeclarerCardsLegalForTrick prevents (__) D5 (__) HT from
  // matching a spade lead when South and North hold spades.
  BoardState bs_s_wins = MakeBoardState(
      {ParseHand("S 5 H T D 6 C 6"), ParseHand("S 94 C J9"),
       ParseHand("S K D 765"), ParseHand("S 83 D T9")},
      1, HEARTS);
  BoardState bs_w_wins_sa = MakeBoardState(
      {ParseHand("S 5 H T D 6 C 6"), ParseHand("S 94 C J9"),
       ParseHand("S K D 765"), ParseHand("S A D T98")},
      1, HEARTS);

  GeneralizedTrick gt_spade(
      {"SX", "SK", {0}}, {"S_", "S5", {0}}, {0}, 1);
  ConditionalLine leaf_spade({0});
  ConditionalLine node_spade;
  node_spade.SetAction(gt_spade);
  node_spade.AddCondition(new WinnerPlayCondition(2), leaf_spade);

  GeneralizedTrick gt_other(
      {"__", "D5", {1}}, {"__", "HT", {1}}, {1}, 1);
  ConditionalLine leaf_other({1});
  ConditionalLine node_other;
  node_other.SetAction(gt_other);
  node_other.AddCondition(new WinnerPlayCondition(0), leaf_other);

  ConditionalLine root;
  root.AddCondition(new TrickPlayCondition("(SX) SK (S_) S5"), node_spade);
  root.AddCondition(new TrickPlayCondition("(__) D5 (__) HT"), node_other);
  root.set_min_tricks(1);
  root.set_sample_board(bs_s_wins);
  root.DeepCloneChildren();

  // Verify DeepCloneChildren relaxed (SX) to (S_).
  ASSERT_EQ(root.conditions()[0].second->do_first().demi1.opp_card, "S_");
  ASSERT_EQ(root.conditions()[0].first->Describe(false),
            "after (S_) SK (S_) S5");

  // On bs_s_wins, East leads S4, South plays SK and wins Trick #1!
  ConditionalLine::RescoreBoardTrace trace_win;
  int tricks_won = root.ResultsOnHandAdversarial(bs_s_wins, nullptr, &trace_win);
  EXPECT_GE(tricks_won, 1);
  EXPECT_EQ(trace_win.leaf,
            root.conditions()[0].second->conditions()[0].second.get());

  // On bs_w_wins_sa, East leads S4, South plays SK, West overtakes with SA.
  // TraceOnBoard must record the failure on node_spade (not node_other!).
  ConditionalLine::BoardPlayTrace trace_fail = root.TraceOnBoard(bs_w_wins_sa);
  ASSERT_EQ(trace_fail.steps.size(), 1u);
  EXPECT_EQ(trace_fail.steps[0].node, root.conditions()[0].second.get());
  EXPECT_EQ(trace_fail.steps[0].trick_str, "S4 SK SA S5");
  EXPECT_EQ(trace_fail.steps[0].winner, 3);
}

TEST(ConditionalLineTest, SLtHonorPatternsAndCardsAreRedundant) {
  // Verify DX and Dx are disjoint (non-redundant).
  EXPECT_FALSE(CardsAreRedundant("DX", "Dx"));
  EXPECT_FALSE(CardsAreRedundant("Dx", "DX"));

  // Verify S<H patterns overlap iff their rank masks intersect.
  EXPECT_TRUE(CardsAreRedundant("S<J", "ST"));
  EXPECT_TRUE(CardsAreRedundant("S<J", "Sx"));
  EXPECT_FALSE(CardsAreRedundant("S<J", "SJ"));
  EXPECT_FALSE(CardsAreRedundant("S<J", "SQ"));

  EXPECT_TRUE(CardsAreRedundant("S<K", "SQ"));
  EXPECT_TRUE(CardsAreRedundant("S<K", "SJ"));
  EXPECT_TRUE(CardsAreRedundant("S<K", "S<Q"));
  EXPECT_FALSE(CardsAreRedundant("S<K", "SK"));
  EXPECT_FALSE(CardsAreRedundant("S<K", "SA"));

  EXPECT_TRUE(CardsAreRedundant("S<A", "SK"));
  EXPECT_FALSE(CardsAreRedundant("S<A", "SA"));

  // Cross-suit patterns are never redundant (except "__", and "R_" is only
  // redundant with another "R_").
  EXPECT_FALSE(CardsAreRedundant("S<K", "H<K"));
  EXPECT_TRUE(CardsAreRedundant("__", "S<K"));
  EXPECT_FALSE(CardsAreRedundant("__", "R_"));
  EXPECT_TRUE(CardsAreRedundant("R_", "R_"));

  // Verify GeneralizesTo for S<H patterns.
  EXPECT_TRUE(GeneralizesTo("S2", "S<J"));
  EXPECT_TRUE(GeneralizesTo("ST", "S<J"));
  EXPECT_FALSE(GeneralizesTo("SJ", "S<J"));
  EXPECT_TRUE(GeneralizesTo("SJ", "S<Q"));
  EXPECT_FALSE(GeneralizesTo("SQ", "S<Q"));
  EXPECT_TRUE(GeneralizesTo("SQ", "S<K"));
  EXPECT_FALSE(GeneralizesTo("SK", "S<K"));
  EXPECT_TRUE(GeneralizesTo("SK", "S<A"));
  EXPECT_FALSE(GeneralizesTo("SA", "S<A"));
}

TEST(ConditionalLineTest, GeneralizedDemiPlayEmitsSLtHonor) {
  GeneralizedDemiPlay demi(NOTRUMP);
  // Record plays with S4 (low), ST (ten), and SJ (jack) paired with SQ,
  // and SQ paired with BOTH SA and SQ on board 3 (so S_ has 4 boards while
  // S<Q has 3 boards and S<J has 2 boards).
  demi.RecordOptimalPlay("S4", "SQ", 0);
  demi.RecordOptimalPlay("ST", "SQ", 1);
  demi.RecordOptimalPlay("SJ", "SQ", 2);
  demi.RecordOptimalPlay("SQ", "SA", 3);
  demi.RecordOptimalPlay("SQ", "SQ", 4);

  GeneralizedPlaysVector plays = demi.GetGeneralizedPlays(0, SPADES, 20);
  bool found_s_lt_q = false;
  bool found_s_lt_j = false;
  for (const auto &p : plays) {
    if (p.my_card == "SQ" && p.opp_card == "S<Q") {
      found_s_lt_q = true;
      EXPECT_EQ(p.boards, (std::set<int>{0, 1, 2}));
    }
    if (p.my_card == "SQ" && p.opp_card == "S<J") {
      found_s_lt_j = true;
      EXPECT_EQ(p.boards, (std::set<int>{0, 1}));
    }
  }
  EXPECT_TRUE(found_s_lt_q);
  EXPECT_TRUE(found_s_lt_j);
}

TEST(ConditionalLineTest, CollapseUniformInSuitSplitsRelaxesSingleBranch) {
  std::set<int> deals = {0, 1};
  GeneralizedTrick gt_sub(
      GeneralizedDemiStruct("D<K", "DA", deals),
      GeneralizedDemiStruct("D<J", "CK", deals), deals, 0);
  ConditionalLine leaf(deals);
  ConditionalLine child;
  child.SetAction(gt_sub);
  child.AddCondition(new WinnerPlayCondition(0), leaf);

  ConditionalLine root;
  root.AddCondition(new TrickPlayCondition("DA (D<K) CK (D<J)"), child);
  root.set_min_tricks(1);

  std::vector<BoardState> boards;
  std::vector<int> active_boards = {0, 1};
  root.CollapseUniformInSuitSplits(boards, active_boards, 1, nullptr);

  ASSERT_EQ(root.conditions().size(), 1u);
  EXPECT_EQ(root.conditions()[0].second->do_first().demi1.opp_card, "D_");
  EXPECT_EQ(root.conditions()[0].second->do_first().demi2.opp_card, "D_");
  EXPECT_EQ(root.conditions()[0].first->Describe(false),
            "after DA (D_) CK (D_)");
}

TEST(ConditionalLineTest,
     GroupTricksOnlyBranchesWhenDeclarerPlayChanges) {
  std::set<int> b0 = {0};
  std::set<int> b1 = {1};

  // Declarer leads (leader = 0):
  // Same my1 ("S3") and same my2 ("SJ") with disjoint 2nd-seat ("Sx" vs "ST")
  // must NOT group together into a branching trick.
  GeneralizedTrick decl_same_my2_a({"Sx", "S3", b0}, {"__", "SJ", b0}, b0, 0);
  GeneralizedTrick decl_same_my2_b({"ST", "S3", b1}, {"__", "SJ", b1}, b1, 0);
  EXPECT_EQ(GroupTricks({decl_same_my2_a, decl_same_my2_b}).size(), 2u);

  // Disjoint 2nd-seat ("S<Q" vs "SQ") WITH different 3rd-seat ("SJ" vs "SA")
  // DOES group into a single branching trick.
  GeneralizedTrick decl_diff_my2_a({"S<Q", "S3", b0}, {"__", "SJ", b0}, b0, 0);
  GeneralizedTrick decl_diff_my2_b({"SQ", "S3", b1}, {"__", "SA", b1}, b1, 0);
  auto grp_decl_diff = GroupTricks({decl_diff_my2_a, decl_diff_my2_b});
  ASSERT_EQ(grp_decl_diff.size(), 1u);
  EXPECT_EQ(grp_decl_diff[0].size(), 2u);

  // Disjoint 4th-seat ("Sx" vs "SQ") when declarer leads NEVER branches.
  GeneralizedTrick decl_4th_a({"Sx", "S2", b0}, {"Sx", "SJ", b0}, b0, 0);
  GeneralizedTrick decl_4th_b({"Sx", "S2", b1}, {"SQ", "SJ", b1}, b1, 0);
  EXPECT_EQ(GroupTricks({decl_4th_a, decl_4th_b}).size(), 2u);

  // Defender leads (leader = 1):
  // Same opening-lead suit ("Hx" vs "HT") and identical declarer cards
  // ("H9" and "H8") must NOT branch (Hand 6 regression case).
  GeneralizedTrick def_same_suit_a({"Hx", "H9", b0}, {"H_", "H8", b0}, b0, 1);
  GeneralizedTrick def_same_suit_b({"HT", "H9", b1}, {"H_", "H8", b1}, b1, 1);
  EXPECT_EQ(GroupTricks({def_same_suit_a, def_same_suit_b}).size(), 2u);

  // Different opening-lead suit ("S_" vs "H_") DOES branch even if declarer
  // discards the same cards.
  GeneralizedTrick def_diff_suit_a({"S_", "C2", b0}, {"S_", "D2", b0}, b0, 1);
  GeneralizedTrick def_diff_suit_b({"H_", "C2", b1}, {"H_", "D2", b1}, b1, 1);
  auto grp_def_suit = GroupTricks({def_diff_suit_a, def_diff_suit_b});
  ASSERT_EQ(grp_def_suit.size(), 1u);
  EXPECT_EQ(grp_def_suit[0].size(), 2u);

  // Same opening-lead suit ("Hx" vs "HQ") where 2nd-seat declarer plays a
  // different card ("H9" vs "HA") DOES branch.
  GeneralizedTrick def_diff_my1_a({"Hx", "H9", b0}, {"H_", "H8", b0}, b0, 1);
  GeneralizedTrick def_diff_my1_b({"HQ", "HA", b1}, {"H_", "H8", b1}, b1, 1);
  auto grp_def_my1 = GroupTricks({def_diff_my1_a, def_diff_my1_b});
  ASSERT_EQ(grp_def_my1.size(), 1u);
  EXPECT_EQ(grp_def_my1[0].size(), 2u);
}

TEST(ConditionalLineTest, PlayerDiscardedConditionEvaluateAndResults) {
  PlayerDiscardedCondition cond_e_c(1, CLUBS, true);
  PlayerDiscardedCondition cond_not_e_c(1, CLUBS, false);
  EXPECT_TRUE(cond_e_c.IsStateCondition());
  EXPECT_EQ(cond_e_c.Describe(false), "if E discarded on C");
  EXPECT_EQ(cond_e_c.Describe(true), "when E discarded on C");
  EXPECT_EQ(cond_not_e_c.Describe(false), "if E did not discard on C");
  EXPECT_EQ(cond_not_e_c.Describe(true), "when E did not discard on C");

  // Board 0: East (1) discards S3 on Trick 1 when CA is led in Hearts trump.
  BoardState bs_discard_s = MakeBoardState(
      {ParseHand("C A; S A;"), ParseHand("S 32;"),
       ParseHand("C 2; S 2;"), ParseHand("C 43;")},
      0, HEARTS);
  // Board 1: East (1) ruffs with H2 on Trick 1 when CA is led in Hearts trump
  // (a ruff is NOT a discard).
  BoardState bs_ruff = MakeBoardState(
      {ParseHand("C A; S A;"), ParseHand("H 2; S 3;"),
       ParseHand("C 2; S 2;"), ParseHand("C 43;")},
      0, HEARTS);
  // Board 2: East (1) follows suit with C5 on Trick 1.
  BoardState bs_follow = MakeBoardState(
      {ParseHand("C A; S A;"), ParseHand("C 5; S 3;"),
       ParseHand("C 2; S 2;"), ParseHand("C 43;")},
      0, HEARTS);

  bs_discard_s.PlayCard(0, Card::ParseStringCard("CA"));
  bs_discard_s.PlayCard(1, Card::ParseStringCard("S3"));
  bs_discard_s.PlayCard(2, Card::ParseStringCard("C2"));
  bs_discard_s.PlayCard(3, Card::ParseStringCard("C3"));
  EXPECT_TRUE(bs_discard_s.HasPlayerDiscardedOnSuit(1, CLUBS));
  EXPECT_FALSE(bs_discard_s.HasPlayerDiscardedOnSuit(1, SPADES));
  EXPECT_TRUE(cond_e_c.EvaluateState(bs_discard_s));
  EXPECT_FALSE(cond_not_e_c.EvaluateState(bs_discard_s));

  bs_ruff.PlayCard(0, Card::ParseStringCard("CA"));
  bs_ruff.PlayCard(1, Card::ParseStringCard("H2"));
  bs_ruff.PlayCard(2, Card::ParseStringCard("C2"));
  bs_ruff.PlayCard(3, Card::ParseStringCard("C3"));
  EXPECT_FALSE(bs_ruff.HasPlayerDiscardedOnSuit(1, CLUBS));
  EXPECT_FALSE(bs_ruff.HasPlayerDiscardedOnSuit(1, HEARTS));
  EXPECT_FALSE(cond_e_c.EvaluateState(bs_ruff));
  EXPECT_TRUE(cond_not_e_c.EvaluateState(bs_ruff));

  bs_follow.PlayCard(0, Card::ParseStringCard("CA"));
  bs_follow.PlayCard(1, Card::ParseStringCard("C5"));
  bs_follow.PlayCard(2, Card::ParseStringCard("C2"));
  bs_follow.PlayCard(3, Card::ParseStringCard("C3"));
  EXPECT_FALSE(bs_follow.HasPlayerDiscardedOnSuit(1, CLUBS));
  EXPECT_FALSE(cond_e_c.EvaluateState(bs_follow));
  EXPECT_TRUE(cond_not_e_c.EvaluateState(bs_follow));
}

TEST(ConditionalLineTest, ExamplePlayFollowsMostCommonBranch) {
  GeneralizedDemiStruct d1_t1("S_", "S3", {0, 1, 2});
  GeneralizedDemiStruct d2_t1("S_", "SQ", {0, 1, 2});
  d2_t1.board_to_trick = {
      {0, "S3 S7 SQ SK"},
      {1, "S3 S7 SQ S8"},
      {2, "S3 S9 SQ S7"},
  };
  GeneralizedTrick gt1(d1_t1, d2_t1, {0, 1, 2}, 0);

  // Branch A: 3 wins (1 board: {0})
  GeneralizedDemiStruct d1_a("S_", "SA", {0});
  GeneralizedDemiStruct d2_a("S_", "S2", {0});
  d2_a.board_to_trick = {{0, "S8 SA S9 S2"}};
  ConditionalLine branch_a;
  branch_a.SetAction(GeneralizedTrick(d1_a, d2_a, {0}, 3));
  branch_a.AddCondition(new WinnerPlayCondition(0), ConditionalLine({0}));

  // Branch B: 2 wins (2 boards: {1, 2}) -> most common branch
  GeneralizedDemiStruct d1_b("S_", "SA", {1, 2});
  GeneralizedDemiStruct d2_b("S_", "S2", {1, 2});
  d2_b.board_to_trick = {
      {1, "SA S9 S2 SK"},
      {2, "SA S8 S2 SK"},
  };
  ConditionalLine branch_b;
  branch_b.SetAction(GeneralizedTrick(d1_b, d2_b, {1, 2}, 2));
  branch_b.AddCondition(new WinnerPlayCondition(2), ConditionalLine({1, 2}));

  ConditionalLine root;
  root.SetAction(gt1);
  root.AddCondition(new WinnerPlayCondition(3), branch_a);
  root.AddCondition(new WinnerPlayCondition(2), branch_b);

  EXPECT_EQ(root.ExamplePlay(true),
            "Example Play (Board 1):\n"
            "  #1 (N) S3 S7 SQ* S8\n"
            "  #2 (S) SA* S9 S2 SK\n");
  EXPECT_EQ(root.ExamplePlay(false), "");

  // Populate failure traces: 1 board (3) fails when E wins SK at Trick #1,
  // and 2 boards (4, 5) fail when W wins SK at Trick #1.
  std::map<int, ConditionalLine::BoardPlayTrace> traces;
  ConditionalLine::PlayedTrickStep step3;
  step3.leader = 0;
  step3.trick_str = "S3 SK SQ S7";
  step3.winner = 1;
  step3.winning_card = "SK";
  step3.node = &root;
  traces[3] = ConditionalLine::BoardPlayTrace{3, {step3}, 1, nullptr};
  root.set_board_traces(traces);
  // With only 1 failure board, ExamplePlay(false) returns empty string.
  EXPECT_EQ(root.ExamplePlay(false), "");

  ConditionalLine::PlayedTrickStep step4;
  step4.leader = 0;
  step4.trick_str = "S3 S7 SQ SK";
  step4.winner = 3;
  step4.winning_card = "SK";
  step4.node = &root;
  traces[4] = ConditionalLine::BoardPlayTrace{4, {step4}, 1, nullptr};

  ConditionalLine::PlayedTrickStep step5 = step4;
  step5.trick_str = "S3 S8 SQ SK";
  traces[5] = ConditionalLine::BoardPlayTrace{5, {step5}, 1, nullptr};
  root.set_board_traces(traces);

  EXPECT_EQ(root.ExamplePlay(false),
            "Example Failed Play (Board 4):\n"
            "  #1 (N) S3 S7 SQ SK*\n");
}


