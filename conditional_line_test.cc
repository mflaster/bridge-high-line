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
  // The third group plays SK rather than SJ over a low card on board 0: a
  // different play that happens to be feasible on an already-grouped board,
  // so it must not be dropped as redundant.
  REQUIRE(grouped_tricks.size() == 3);
  REQUIRE(grouped_tricks[0].size() == 4);
  REQUIRE(grouped_tricks[1].size() == 5);
  REQUIRE(grouped_tricks[2].size() == 4);
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

TEST_CASE("ConditionalLineDescribeSortBranches") {
  ConditionalLine line_b(std::set<int>{2});
  ConditionalLine line_a(std::set<int>{1});

  ConditionalLine root;
  // Add conditions in reverse alphabetical order: 1 (West) wins first, then 0 (North) wins.
  root.AddCondition(new WinnerPlayCondition(1), line_b);
  root.AddCondition(new WinnerPlayCondition(0), line_a);

  std::string unsorted = root.Describe(0, /*sort_branches=*/false);
  std::string sorted = root.Describe(0, /*sort_branches=*/true);

  // Unsorted preserves insertion order (if 1 wins comes before if 0 wins)
  REQUIRE(unsorted.find("if 1 wins") < unsorted.find("if 0 wins"));

  // Sorted reorders branches deterministically (if 0 wins comes before if 1 wins)
  REQUIRE(sorted.find("if 0 wins") < sorted.find("if 1 wins"));

  // Verify PlayLineGoldenSerializer delegates to Describe with sort_branches = true
  REQUIRE(PlayLineGoldenSerializer::Serialize(root) ==
          PlayLineGoldenSerializer::Serialize(sorted));
}

TEST_CASE("IsCardGoodConditionEvaluateAndResults") {
  CardType hk = Card::ParseStringCard("HK");
  IsCardGoodCondition cond_good(hk, true);
  IsCardGoodCondition cond_not_good(hk, false);

  REQUIRE(cond_good.Describe(false) == "if HK is good");
  REQUIRE(cond_good.Describe(true) == "when HK is good");
  REQUIRE(cond_not_good.Describe(false) == "if HK is not good");
  REQUIRE(cond_not_good.Describe(true) == "when HK is not good");

  std::vector<uint64_t> hands = {
      ParseHand("S A3"),
      ParseHand("S J H Q"),
      ParseHand("S 2 H K"),
      ParseHand("S T H A"),
  };
  BoardState bs = MakeBoardState(hands, 2, NOTRUMP);
  // HA is unplayed in hand 3, so HK is not good.
  REQUIRE(!cond_good.Evaluate(bs));

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
  REQUIRE(root.ResultsOnHand(bs) == 2);

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
  REQUIRE(cond_good.Evaluate(bs_ha_gone));
  REQUIRE(root.ResultsOnHand(bs_ha_gone) == 2);
}

TEST_CASE("GeneralizedDemiPlayDiscardNoSuitBranching") {
  GeneralizedDemiPlay demi(HEARTS);
  demi.RecordOptimalPlay("D4", "SA", 0);
  demi.RecordOptimalPlay("CQ", "SA", 1);

  GeneralizedPlaysVector plays = demi.GetGeneralizedPlays(0, SPADES, 20);
  for (const auto &p : plays) {
    REQUIRE((p.opp_card == "R_" || p.opp_card == "__"));
    REQUIRE(p.boards == std::set<int>{0, 1});
  }
}

TEST_CASE("GeneralizedTrickFinderDefenderLeadConsolidation") {
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
  REQUIRE(found_s_any_sa);
  REQUIRE(!found_s_low_sa);

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
  REQUIRE(found_consolidated_group);
}
