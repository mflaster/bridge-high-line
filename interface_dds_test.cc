#include <iostream>
#include <gtest/gtest.h>

#include "interface_dds.h"
#include "board_state.h"

TEST(InterfaceDdsTest, Basic) {
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S AQ"));
  hands.push_back(ParseHand("S KJ"));
  hands.push_back(ParseHand("S 87"));
  hands.push_back(ParseHand("S 54"));
  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  bs.SetTrump(SPADES);

  OptimalLines lines;
  int ans = DDSFindOptimalLines(bs, 1, &lines, nullptr);
  for (const auto &line : lines) {
    std::cout << "See line: " << line << "\n";
  }
  ASSERT_TRUE(ans == 1);
}

std::shared_ptr<OptimalPlayTree> FindOptimalPlayTree(
    const BoardState &bs, int min_tricks) {
  return OptimalPlayTree::FindOptimalPlayTree(bs, min_tricks);
}

TEST(InterfaceDdsTest, PlayTree) {
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S AK"));
  hands.push_back(ParseHand("S QJ"));
  hands.push_back(ParseHand("S 87"));
  hands.push_back(ParseHand("S 54"));
  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  bs.SetTrump(SPADES);

  std::shared_ptr<OptimalPlayTree> all_plays = FindOptimalPlayTree(
      bs, 1);
  std::cout << "Got " << all_plays->DebugString() << "\n";
  ASSERT_TRUE(all_plays->TotalCount() == 1);

  all_plays->Deepen(10, nullptr);
  std::cout << "Got " << all_plays->DebugString() << "\n";
  ASSERT_TRUE(all_plays->TotalCount() == 2);
}

TEST(InterfaceDdsTest, PlayTree2) {
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S AQ"));
  hands.push_back(ParseHand("S KJ"));
  hands.push_back(ParseHand("S 87"));
  hands.push_back(ParseHand("S 54"));
  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  bs.SetTrump(SPADES);

  std::shared_ptr<OptimalPlayTree> all_plays = FindOptimalPlayTree(
      bs, 1);
  all_plays->Deepen(10, nullptr);
  std::cout << "Got " << all_plays->DebugString() << "\n";
  ASSERT_TRUE(all_plays->TotalCount() == 4);
}

TEST(InterfaceDdsTest, PlayTree3) {
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S AQT"));
  hands.push_back(ParseHand("S KJ9"));
  hands.push_back(ParseHand("S 876"));
  hands.push_back(ParseHand("S 543"));
  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  bs.SetTrump(SPADES);

  std::shared_ptr<OptimalPlayTree> all_plays = FindOptimalPlayTree(bs, 2);
  all_plays->Deepen(10, nullptr);
  std::cout << "Got " << all_plays->DebugString() << "\n";
  ASSERT_TRUE(all_plays->TotalCount() == 10);
}

TEST(InterfaceDdsTest, DISABLED_PlayTree8) {
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("H Q7; D KQT9; C A2; "));
  hands.push_back(ParseHand("H T5; D A754; C Q3; "));
  hands.push_back(ParseHand("H KJ98; D 2; C T94; "));
  hands.push_back(ParseHand("H A4; D J863; C 56; "));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);

  std::shared_ptr<OptimalPlayTree> all_plays = FindOptimalPlayTree(bs, 3);
  SolverCache cache;
  all_plays->Deepen(10, &cache);
  std::cout << "TotalCount " << all_plays->TotalCount() << "\n";
  ASSERT_TRUE(all_plays->TotalCount() > 10);
}

TEST(InterfaceDdsTest, DISABLED_PlayTree11) {
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S 94; H T8; D KQT9; C Q94; "));
  hands.push_back(ParseHand("S KQ; H Q; D A754; C A765; "));
  hands.push_back(ParseHand("S AJ86; H KJ976; D -; C KJ; "));
  hands.push_back(ParseHand("S T75; H A54; D J86; C T8; "));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);

  std::shared_ptr<OptimalPlayTree> all_plays = FindOptimalPlayTree(bs, 7);
  SolverCache cache;
  all_plays->Deepen(1, &cache);

  std::cout << "Got " << all_plays->DebugString() << "\n";
  std::cout << "TotalCount " << all_plays->TotalCount() << "\n";
  all_plays->Deepen(3, &cache);
  std::cout << "TotalCount " << all_plays->TotalCount() << "\n";
  all_plays->Deepen(5, &cache);
  std::cout << "TotalCount " << all_plays->TotalCount() << "\n";
  ASSERT_TRUE(all_plays->TotalCount() > 10);
}

namespace {
struct ScopedDefenderNoise {
  int prev;
  explicit ScopedDefenderNoise(int pct) : prev(GetDefenderNoisePct()) {
    SetDefenderNoisePct(pct);
  }
  ~ScopedDefenderNoise() { SetDefenderNoisePct(prev); }
};
}  // namespace

TEST(InterfaceDdsTest, DefenderSeatRules_SecondHandLowAndEquals) {
  ScopedDefenderNoise no_noise(0);
  // Hand 1 holds touching QJ in 2nd seat (random 50/50 between SQ and SJ);
  // Hand 3 holds touching spot cards 54 in 4th seat (always lowest S4).
  std::vector<uint64_t> hands = {
      ParseHand("S AK"),
      ParseHand("S QJ"),
      ParseHand("S 87"),
      ParseHand("S 54"),
  };
  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  bs.SetTrump(SPADES);

  srand48(42);
  int saw_sj = 0;
  int saw_sq = 0;
  for (int trial = 0; trial < 40; ++trial) {
    OptimalLines lines;
    DDSFindOptimalLines(bs, 2, &lines, nullptr);
    ASSERT_TRUE(lines.size() == 1);
    const std::string &line = *lines.begin();
    if (line == "SA SJ S8 S4 ") {
      ++saw_sj;
    } else if (line == "SA SQ S8 S4 ") {
      ++saw_sq;
    } else {
      FAIL() << "Unexpected line: " << line;
    }
  }
  ASSERT_TRUE(saw_sj > 5);
  ASSERT_TRUE(saw_sq > 5);

  // Touching T9 (one honor T and one spot 9) also randomizes between ST and S9.
  std::vector<uint64_t> t9_hands = {
      ParseHand("S AK"),
      ParseHand("S T9"),
      ParseHand("S 87"),
      ParseHand("S 54"),
  };
  BoardState bs_t9;
  bs_t9.SetHands(t9_hands);
  bs_t9.SetLead(0);
  bs_t9.SetTrump(SPADES);
  int saw_s9 = 0;
  int saw_st = 0;
  for (int trial = 0; trial < 40; ++trial) {
    OptimalLines lines;
    DDSFindOptimalLines(bs_t9, 2, &lines, nullptr);
    if (lines.count("SA S9 S8 S4 ")) ++saw_s9;
    if (lines.count("SA ST S8 S4 ")) ++saw_st;
  }
  ASSERT_TRUE(saw_s9 > 5);
  ASSERT_TRUE(saw_st > 5);
}

TEST(InterfaceDdsTest, DefenderSeatRules_ThirdHandHighAndEquals) {
  ScopedDefenderNoise no_noise(0);
  // Hand 1 leads a small spade (S3), Hand 2 plays low (S2), Hand 3 in 3rd seat
  // holds KQ5 with Hand 0 holding AT9 in 4th seat. Whether 3rd hand plays KQ
  // or 5 yields the same double-dummy trick count (1 trick for E/W), but 3rd
  // hand must play high (never S5) and randomly choose between touching honors
  // SK and SQ.
  std::vector<uint64_t> hands = {
      ParseHand("S AT9"),
      ParseHand("S 743"),
      ParseHand("S 862"),
      ParseHand("S KQ5"),
  };
  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(1);
  bs.SetTrump(NOTRUMP);
  bs.PlayCard(1, Card::ParseStringCard("S3"));
  bs.PlayCard(2, Card::ParseStringCard("S2"));

  srand48(7);
  int saw_sq = 0;
  int saw_sk = 0;
  for (int trial = 0; trial < 40; ++trial) {
    OptimalLines lines;
    DDSFindOptimalLines(bs, 0, &lines, nullptr);
    ASSERT_TRUE(!lines.empty());
    for (const std::string &line : lines) {
      if (line.rfind("S3 S2 SQ ", 0) == 0) {
        ++saw_sq;
      } else if (line.rfind("S3 S2 SK ", 0) == 0) {
        ++saw_sk;
      } else {
        FAIL() << "Unexpected 3rd hand play in line: " << line;
      }
    }
  }
  ASSERT_TRUE(saw_sq > 5);
  ASSERT_TRUE(saw_sk > 5);
}

TEST(InterfaceDdsTest, DefenderSeatRules_FourthHandWinCheaplyAndNoOvertake) {
  ScopedDefenderNoise no_noise(0);
  {  // 4th hand wins instead of ducking and randomizes touching honors
    // Hand 2 leads S8, Hand 3 plays S5, Hand 0 plays ST. Hand 1 in 4th seat
    // holds QJ4 while Hand 2 has SA2 remaining. Winning with SJ/SQ or ducking
    // with S4 all yield 2 tricks for E/W; 4th hand must win (never duck with
    // S4) and randomize between touching honors SJ and SQ.
    std::vector<uint64_t> hands = {
        ParseHand("S T93"),
        ParseHand("S QJ4"),
        ParseHand("S A82"),
        ParseHand("S 765"),
    };
    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(2);
    bs.SetTrump(NOTRUMP);
    bs.PlayCard(2, Card::ParseStringCard("S8"));
    bs.PlayCard(3, Card::ParseStringCard("S5"));
    bs.PlayCard(0, Card::ParseStringCard("ST"));

    srand48(13);
    int saw_sj = 0;
    int saw_sq = 0;
    for (int trial = 0; trial < 40; ++trial) {
      OptimalLines lines;
      DDSFindOptimalLines(bs, 0, &lines, nullptr);
      ASSERT_TRUE(lines.size() == 1);
      if (*lines.begin() == "S8 S5 ST SJ ") {
        ++saw_sj;
      } else if (*lines.begin() == "S8 S5 ST SQ ") {
        ++saw_sq;
      } else {
        FAIL() << "Unexpected 4th hand play: " << *lines.begin();
      }
    }
    ASSERT_TRUE(saw_sj > 5);
    ASSERT_TRUE(saw_sq > 5);
  }

  {  // 4th hand wins as cheaply as possible with non-touching honors
    // Hand 1 in 4th seat holds K J 4 over ST; since K and J are not touching,
    // 4th hand deterministically wins with the cheaper winning card (SJ).
    std::vector<uint64_t> hands = {
        ParseHand("S T93"),
        ParseHand("S KJ4"),
        ParseHand("S A82"),
        ParseHand("S 765"),
    };
    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(2);
    bs.SetTrump(NOTRUMP);
    bs.PlayCard(2, Card::ParseStringCard("S8"));
    bs.PlayCard(3, Card::ParseStringCard("S5"));
    bs.PlayCard(0, Card::ParseStringCard("ST"));

    OptimalLines lines;
    DDSFindOptimalLines(bs, 0, &lines, nullptr);
    ASSERT_TRUE(lines.size() == 1);
    ASSERT_TRUE(*lines.begin() == "S8 S5 ST SJ ");
  }

  {  // 4th hand does not overtake partner's winning card
    // Hand 2 leads S2, Hand 3 (partner) plays SJ which is winning, Hand 0
    // plays S3. Hand 1 in 4th seat holds KQ4 (both overtaking and ducking win
    // all tricks for E/W). 4th hand must play S4 and not overtake partner.
    std::vector<uint64_t> hands = {
        ParseHand("S 983"),
        ParseHand("S KQ4"),
        ParseHand("S T62"),
        ParseHand("S AJ5"),
    };
    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(2);
    bs.SetTrump(NOTRUMP);
    bs.PlayCard(2, Card::ParseStringCard("S2"));
    bs.PlayCard(3, Card::ParseStringCard("SJ"));
    bs.PlayCard(0, Card::ParseStringCard("S3"));

    OptimalLines lines;
    DDSFindOptimalLines(bs, 0, &lines, nullptr);
    ASSERT_TRUE(lines.size() == 1);
    ASSERT_TRUE(*lines.begin() == "S2 SJ S3 S4 ");
  }
}

TEST(InterfaceDdsTest, DefenderSeatRules_FirstHandLeadSelection) {
  ScopedDefenderNoise no_noise(0);
  {  // Prefers leading touching honors over underleading into a tenace
    // West (Hand 3) is on lead in a 7-card ending (like Trick 3 of 9-Card
    // Hearts after DK holds at Trick 2). West holds touching honors DQJ in
    // diamonds (where South has DA5 opposite North's DT) alongside S AJ4
    // (over South's SK9) and C Q4 (under South's CAKJ).
    // All leads yield the same double-dummy trick count for E/W, so West
    // should continue with the top of touching diamond honors (DQ from DQJ)
    // rather than shifting away from an unsupported honor into South's spade
    // or club tenaces.
    std::vector<uint64_t> hands = {
        ParseHand("S 532; H T7; D T; C 6;"),
        ParseHand("S QT6; H -; D 6; C T98;"),
        ParseHand("S K9; H -; D A5; C AKJ;"),
        ParseHand("S AJ4; H -; D QJ; C Q4;"),
    };
    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(3);
    bs.SetTrump(HEARTS);

    OptimalLines lines;
    DDSFindOptimalLines(bs, 0, &lines, nullptr);
    ASSERT_EQ(lines.size(), 1u);
    const std::string &line = *lines.begin();
    ASSERT_EQ(line.rfind("DQ ", 0), 0u)
        << "Expected West to lead top of touching honors DQ, got: " << line;
  }

  {  // Prefers passive spot lead over underleading an unsupported honor
    // East (Hand 1) is on lead holding S AT4 (into South's S K73) and H 87
    // (passive suit with no N/S honor tenace). When both leads are
    // double-dummy optimal, East must lead passively (H7) rather than
    // underleading the Ace of Spades into declarer's King.
    std::vector<uint64_t> hands = {
        ParseHand("S K73; H 2;"),
        ParseHand("S AT4; H 7;"),
        ParseHand("S -; H -; C QJT6;"),
        ParseHand("S 85; H 8; C 7;"),
    };
    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(1);
    bs.SetTrump(CLUBS);

    OptimalLines lines;
    DDSFindOptimalLines(bs, 0, &lines, nullptr);
    ASSERT_TRUE(!lines.empty());
    for (const std::string &line : lines) {
      ASSERT_TRUE(line.rfind("H7 ", 0) == 0);
    }
  }
}

TEST(InterfaceDdsTest, DeclarerSeatRules_PrunesRedundantLosingAndSpotPlays) {
  ScopedDefenderNoise no_noise(0);
  {  // Defender leads HA; North's non-touching losing H8 and H5 collapse to H5
    std::vector<uint64_t> hands = {
        ParseHand("S A; H 85;"),
        ParseHand("S 32; H A;"),
        ParseHand("S K9; H 3;"),
        ParseHand("S 54; H 6;"),
    };
    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(1);
    bs.SetTrump(SPADES);
    bs.PlayCard(1, Card::ParseStringCard("HA"));

    OptimalLines lines;
    DDSFindOptimalLines(bs, 2, &lines, nullptr);
    ASSERT_EQ(lines.size(), 1u);
    ASSERT_EQ(*lines.begin(), "HA H3 H6 H5 ");
  }

  {  // South leads from D 85 opposite void when defenders hold D6; D8 -> D5
    std::vector<uint64_t> hands = {
        ParseHand("S A65; D -;"),
        ParseHand("S 3; D K6;"),
        ParseHand("S 2; D 85;"),
        ParseHand("S 4; D T7;"),
    };
    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(2);
    bs.SetTrump(SPADES);

    OptimalLines lines;
    DDSFindOptimalLines(bs, 3, &lines, nullptr);
    bool saw_d5 = false;
    bool saw_d8 = false;
    for (const std::string &line : lines) {
      if (line.rfind("D5 ", 0) == 0) saw_d5 = true;
      if (line.rfind("D8 ", 0) == 0) saw_d8 = true;
    }
    ASSERT_TRUE(saw_d5);
    ASSERT_FALSE(saw_d8);
  }
}
