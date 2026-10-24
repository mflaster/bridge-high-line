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
  // Hand 0 (declarer) holds touching AK on lead (plays low SK);
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
    if (line == "SK SJ S7 S4 ") {
      ++saw_sj;
    } else if (line == "SK SQ S7 S4 ") {
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
    if (lines.count("SK S9 S7 S4 ")) ++saw_s9;
    if (lines.count("SK ST S7 S4 ")) ++saw_st;
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

  {  // 3rd hand does not overtake partner's winning S9 with touching ST
    // East (Hand 1) leads S9, South (Hand 2, dummy) plays S4, and West (Hand 3)
    // holds S T532 with North (Hand 0) holding S AK87 in 4th seat. Playing ST
    // on partner's S9 wastes an equal (no card lies between 9 and T), so West
    // must duck with S2 rather than overtaking with ST.
    std::vector<uint64_t> hands = {
        ParseHand("S AK87"),
        ParseHand("S 9; H 432;"),
        ParseHand("S QJ64"),
        ParseHand("S T532"),
    };
    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(1);
    bs.SetTrump(SPADES);
    bs.PlayCard(1, Card::ParseStringCard("S9"));
    bs.PlayCard(2, Card::ParseStringCard("S4"));

    OptimalLines lines;
    DDSFindOptimalLines(bs, 0, &lines, nullptr);
    ASSERT_FALSE(lines.empty());
    for (const std::string &line : lines) {
      ASSERT_EQ(line.rfind("S9 S4 S2 ", 0), 0u)
          << "Expected West to duck with S2 on partner's S9, got: " << line;
    }
  }

  {  // 3rd hand does not overtake partner's S8 with ST when S9 is in dummy
    // East (Hand 1) leads S8, South (Hand 2, dummy) holds S QJ94 and plays S4,
    // and West (Hand 3) holds S T532 with North (Hand 0) holding S AK76. Since
    // S9 is visible in dummy, ST and S8 are touching equals against 4th hand,
    // so West must duck with S2 rather than overtaking partner's S8 with ST.
    std::vector<uint64_t> hands = {
        ParseHand("S AK76"),
        ParseHand("S 8; H 432;"),
        ParseHand("S QJ94"),
        ParseHand("S T532"),
    };
    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(1);
    bs.SetTrump(SPADES);
    bs.PlayCard(1, Card::ParseStringCard("S8"));
    bs.PlayCard(2, Card::ParseStringCard("S4"));

    OptimalLines lines;
    DDSFindOptimalLines(bs, 0, &lines, nullptr);
    ASSERT_FALSE(lines.empty());
    for (const std::string &line : lines) {
      ASSERT_EQ(line.rfind("S8 S4 S2 ", 0), 0u)
          << "Expected West to duck with S2 on partner's S8, got: " << line;
    }
  }

  {  // 1st hand leads low, 2nd hand has Kx and plays low, 3rd hand has AQ
    // East (Hand 1) leads S3, South (Hand 2, dummy) holds S K6 and plays S6,
    // and West (Hand 3) holds S AQ in 3rd seat with North (Hand 0) holding
    // S 84 in 4th seat. Both SA and SQ win both tricks for E/W, and because SK
    // is in 2nd hand (which already played low), 3rd hand finesses SQ rather
    // than playing SA.
    std::vector<uint64_t> hands = {
        ParseHand("S 84"),
        ParseHand("S 53"),
        ParseHand("S K6"),
        ParseHand("S AQ"),
    };
    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(1);
    bs.SetTrump(NOTRUMP);
    bs.PlayCard(1, Card::ParseStringCard("S3"));
    bs.PlayCard(2, Card::ParseStringCard("S6"));

    OptimalLines lines;
    DDSFindOptimalLines(bs, 0, &lines, nullptr);
    ASSERT_FALSE(lines.empty());
    for (const std::string &line : lines) {
      ASSERT_EQ(line.rfind("S3 S6 SQ ", 0), 0u)
          << "Expected West to finesse SQ from AQ when 2nd hand played low "
             "from Kx, got: " << line;
    }
  }

  {  // 1st hand leads Q, 2nd hand plays low from Kx, 3rd hand has Ax -> ducks
    // East (Hand 1) leads SQ (from S QJ), South (Hand 2, dummy) holds S K6 and
    // plays S6, and West (Hand 3) holds S A2 in 3rd seat with North (Hand 0)
    // holding S 84. Both SA and S2 win both tricks for E/W, and because
    // partner's SQ is winning after 2nd hand ducked with SK, West plays low
    // (S2) rather than overtaking partner's SQ with SA.
    std::vector<uint64_t> hands = {
        ParseHand("S 84"),
        ParseHand("S QJ"),
        ParseHand("S K6"),
        ParseHand("S A2"),
    };
    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(1);
    bs.SetTrump(NOTRUMP);
    bs.PlayCard(1, Card::ParseStringCard("SQ"));
    bs.PlayCard(2, Card::ParseStringCard("S6"));

    OptimalLines lines;
    DDSFindOptimalLines(bs, 0, &lines, nullptr);
    ASSERT_FALSE(lines.empty());
    for (const std::string &line : lines) {
      ASSERT_EQ(line.rfind("SQ S6 S2 ", 0), 0u)
          << "Expected West to play low (S2) when partner's SQ is winning "
             "through 2nd hand's Kx, got: " << line;
    }
  }

  {  // 1st hand leads Q, 2nd hand has low cards, 4th hand has K -> 3rd plays A
    // East (Hand 1) leads SQ, South (Hand 2, dummy) holds S 76 and plays S6,
    // West (Hand 3) holds S A2 in 3rd seat, and North (Hand 0) holds S K8 in
    // 4th seat. Even when SA and S2 yield the same double-dummy trick count
    // (1 trick for E/W), 3rd hand must play SA over partner's SQ rather than
    // ducking into 4th hand's SK.
    std::vector<uint64_t> hands = {
        ParseHand("S K8"),
        ParseHand("S Q4"),
        ParseHand("S 76"),
        ParseHand("S A2"),
    };
    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(1);
    bs.SetTrump(NOTRUMP);
    bs.PlayCard(1, Card::ParseStringCard("SQ"));
    bs.PlayCard(2, Card::ParseStringCard("S6"));

    OptimalLines lines;
    DDSFindOptimalLines(bs, 0, &lines, nullptr);
    ASSERT_FALSE(lines.empty());
    for (const std::string &line : lines) {
      ASSERT_EQ(line.rfind("SQ S6 SA ", 0), 0u)
          << "Expected West to play SA when partner leads SQ, 2nd hand has "
             "low cards, and 4th hand holds SK, got: " << line;
    }
  }

  {  // Same three scenarios when West (1st) leads through North (2nd, declarer)
     // to East (3rd) with South (4th, dummy):
    // (a) West leads S3, North holds S K6 and plays S6, East holds S AQ, South
    //     holds S 84 in dummy -> East finesses SQ (seeing South has no SK).
    std::vector<uint64_t> hands_a = {
        ParseHand("S K6"),
        ParseHand("S AQ"),
        ParseHand("S 84"),
        ParseHand("S 53"),
    };
    BoardState bs_a;
    bs_a.SetHands(hands_a);
    bs_a.SetLead(3);
    bs_a.SetTrump(NOTRUMP);
    bs_a.PlayCard(3, Card::ParseStringCard("S3"));
    bs_a.PlayCard(0, Card::ParseStringCard("S6"));

    OptimalLines lines_a;
    DDSFindOptimalLines(bs_a, 0, &lines_a, nullptr);
    ASSERT_FALSE(lines_a.empty());
    for (const std::string &line : lines_a) {
      ASSERT_EQ(line.rfind("S3 S6 SQ ", 0), 0u)
          << "Expected East to finesse SQ from AQ when North played low from "
             "Kx, got: " << line;
    }

    // (b) West leads SQ, North holds S K6 and plays S6, East holds S A2, South
    //     holds S 84 in dummy -> East plays low (S2).
    std::vector<uint64_t> hands_b = {
        ParseHand("S K6"),
        ParseHand("S A2"),
        ParseHand("S 84"),
        ParseHand("S QJ"),
    };
    BoardState bs_b;
    bs_b.SetHands(hands_b);
    bs_b.SetLead(3);
    bs_b.SetTrump(NOTRUMP);
    bs_b.PlayCard(3, Card::ParseStringCard("SQ"));
    bs_b.PlayCard(0, Card::ParseStringCard("S6"));

    OptimalLines lines_b;
    DDSFindOptimalLines(bs_b, 0, &lines_b, nullptr);
    ASSERT_FALSE(lines_b.empty());
    for (const std::string &line : lines_b) {
      ASSERT_EQ(line.rfind("SQ S6 S2 ", 0), 0u)
          << "Expected East to play low (S2) on partner's SQ when North "
             "played low from Kx, got: " << line;
    }

    // (c) West leads SQ, North holds S 76 and plays S6, East holds S A2, South
    //     holds S K8 in dummy -> East overtakes with SA.
    std::vector<uint64_t> hands_c = {
        ParseHand("S 76"),
        ParseHand("S A2"),
        ParseHand("S K8"),
        ParseHand("S Q4"),
    };
    BoardState bs_c;
    bs_c.SetHands(hands_c);
    bs_c.SetLead(3);
    bs_c.SetTrump(NOTRUMP);
    bs_c.PlayCard(3, Card::ParseStringCard("SQ"));
    bs_c.PlayCard(0, Card::ParseStringCard("S6"));

    OptimalLines lines_c;
    DDSFindOptimalLines(bs_c, 0, &lines_c, nullptr);
    ASSERT_FALSE(lines_c.empty());
    for (const std::string &line : lines_c) {
      ASSERT_EQ(line.rfind("SQ S6 SA ", 0), 0u)
          << "Expected East to play SA on partner's SQ when South (4th hand) "
             "holds SK, got: " << line;
    }
  }

  {  // Dummy in 4th seat: 3rd seat only plays high enough to beat dummy
    // (a) West (1st) leads S2, North (2nd) holds S Q6 and plays S6, South
    //     (4th, dummy) visibly holds S T4, and East (3rd) holds S KJ8.
    //     Both SK and SJ tie in double-dummy tricks (1 trick for E/W), and
    //     since East sees dummy's highest card is ST, East wins cheaply with
    //     SJ rather than playing SK.
    std::vector<uint64_t> hands_a = {
        ParseHand("S Q6; H A;"),
        ParseHand("S KJ8"),
        ParseHand("S T4; H 2;"),
        ParseHand("S A52"),
    };
    BoardState bs_a;
    bs_a.SetHands(hands_a);
    bs_a.SetLead(3);
    bs_a.SetTrump(NOTRUMP);
    bs_a.PlayCard(3, Card::ParseStringCard("S2"));
    bs_a.PlayCard(0, Card::ParseStringCard("S6"));

    OptimalLines lines_a;
    DDSFindOptimalLines(bs_a, 0, &lines_a, nullptr);
    ASSERT_FALSE(lines_a.empty());
    for (const std::string &line : lines_a) {
      ASSERT_EQ(line.rfind("S2 S6 SJ ", 0), 0u)
          << "Expected East to play SJ (just above dummy's ST) rather than SK, "
             "got: " << line;
    }

    // (b) West (1st) leads S9, North (2nd) plays S6, South (4th, dummy)
    //     visibly holds only S 74, and East (3rd) holds S A2. Since partner's
    //     S9 already beats dummy's S7, East ducks with S2 rather than
    //     overtaking partner's S9 with SA.
    std::vector<uint64_t> hands_b = {
        ParseHand("S KQ6"),
        ParseHand("S A2; H 2;"),
        ParseHand("S 74; H 3;"),
        ParseHand("S J95"),
    };
    BoardState bs_b;
    bs_b.SetHands(hands_b);
    bs_b.SetLead(3);
    bs_b.SetTrump(NOTRUMP);
    bs_b.PlayCard(3, Card::ParseStringCard("S9"));
    bs_b.PlayCard(0, Card::ParseStringCard("S6"));

    OptimalLines lines_b;
    DDSFindOptimalLines(bs_b, 0, &lines_b, nullptr);
    ASSERT_FALSE(lines_b.empty());
    for (const std::string &line : lines_b) {
      ASSERT_EQ(line.rfind("S9 S6 S2 ", 0), 0u)
          << "Expected East to duck with S2 when partner's S9 already beats "
             "dummy's S74, got: " << line;
    }

    // (c) West (1st) leads S2, North (2nd) puts in S8, East (3rd) holds S KJ9,
    //     and South (4th, dummy) holds S A43. Dummy's SA cannot be beaten by
    //     East anyway, and Dummy's remaining cards are S43, so East plays the
    //     cheapest card that beats North's S8 and Dummy's S4 — namely S9 — to
    //     force out Dummy's SA rather than wasting SK or SJ.
    std::vector<uint64_t> hands_c = {
        ParseHand("S QT8"),
        ParseHand("S KJ9"),
        ParseHand("S A43"),
        ParseHand("S 752"),
    };
    BoardState bs_c;
    bs_c.SetHands(hands_c);
    bs_c.SetLead(3);
    bs_c.SetTrump(NOTRUMP);
    bs_c.PlayCard(3, Card::ParseStringCard("S2"));
    bs_c.PlayCard(0, Card::ParseStringCard("S8"));

    OptimalLines lines_c;
    DDSFindOptimalLines(bs_c, 0, &lines_c, nullptr);
    ASSERT_FALSE(lines_c.empty());
    for (const std::string &line : lines_c) {
      ASSERT_EQ(line.rfind("S2 S8 S9 ", 0), 0u)
          << "Expected East to play S9 from KJ9 against North's S8 and "
             "dummy's A43, got: " << line;
    }
  }
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
  {  // North leads from H QJT: declarer leads the lowest of touching honors
     // (HT rather than HQ).
    SetDefenderNoisePct(0);
    std::vector<uint64_t> hands = {
        ParseHand("S A; H QJT;"),
        ParseHand("S 2; H 432;"),
        ParseHand("S 3; H 987;"),
        ParseHand("S 4; H AK6;"),
    };
    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(0);
    bs.SetTrump(NOTRUMP);

    OptimalLines lines;
    DDSFindOptimalLines(bs, 2, &lines, nullptr);
    bool saw_ht = false;
    bool saw_hq = false;
    for (const std::string &line : lines) {
      if (line.rfind("HT ", 0) == 0) saw_ht = true;
      if (line.rfind("HQ ", 0) == 0) saw_hq = true;
    }
    ASSERT_TRUE(saw_ht);
    ASSERT_FALSE(saw_hq);
  }
  {  // North leads C6, East plays CQ, South holds C KJ: South must win CK and
     // prune underplaying CJ under CQ.
    std::vector<uint64_t> hands = {
        ParseHand("S 32; C 6;"),
        ParseHand("S 54; C Q;"),
        ParseHand("S K; C KJ;"),
        ParseHand("S 6; C 98;"),
    };
    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(0);
    bs.SetTrump(HEARTS);
    bs.PlayCard(0, Card::ParseStringCard("C6"));
    bs.PlayCard(1, Card::ParseStringCard("CQ"));

    OptimalLines lines;
    DDSFindOptimalLines(bs, 2, &lines, nullptr);
    bool saw_ck = false;
    bool saw_cj = false;
    for (const std::string &line : lines) {
      if (line.rfind("C6 CQ CK ", 0) == 0) saw_ck = true;
      if (line.rfind("C6 CQ CJ ", 0) == 0) saw_cj = true;
    }
    ASSERT_TRUE(saw_ck);
    ASSERT_FALSE(saw_cj);
  }
  {  // South leads CK, West discards S4, North holds S 532 (with S4 no longer
     // unplayed): North's DDS equivalence group {S5, S3, S2} lowers to S2.
    std::vector<uint64_t> hands = {
        ParseHand("S 532;"),
        ParseHand("S A; C 83;"),
        ParseHand("S K; C AK;"),
        ParseHand("S QJ4;"),
    };
    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(2);
    bs.SetTrump(HEARTS);
    bs.PlayCard(2, Card::ParseStringCard("CK"));
    bs.PlayCard(3, Card::ParseStringCard("S4"));

    OptimalLines lines;
    DDSFindOptimalLines(bs, 2, &lines, nullptr);
    bool saw_s2 = false;
    bool saw_s5 = false;
    for (const std::string &line : lines) {
      if (line.rfind("CK S4 S2 ", 0) == 0) saw_s2 = true;
      if (line.rfind("CK S4 S5 ", 0) == 0) saw_s5 = true;
    }
    ASSERT_TRUE(saw_s2);
    ASSERT_FALSE(saw_s5);
  }
}

TEST(InterfaceDdsTest, ThirdSeatRuffDoesNotPeekAtFourthHandFollow) {
  ScopedDefenderNoise no_noise(0);
  // Spades are trump. North leads C2 and South (3rd seat) holds S J6 and D A
  // while ST is still outstanding in East's hand and clubs split 1-1 (East CT,
  // West CQ). Double-dummy, S6 wins because West must follow with CQ, but
  // single-dummy West could be void in clubs with ST, so DDSFindOptimalLines
  // must retain SJ alongside S6 instead of peeking at West's club holding.
  {
    std::vector<uint64_t> hands = {
        ParseHand("S A; D 2; C 2;"),
        ParseHand("S T; D 3; C T;"),
        ParseHand("S J6; D A;"),
        ParseHand("H 2; D 4; C Q;"),
    };
    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(0);
    bs.SetTrump(SPADES);

    OptimalLines lines;
    int tricks = DDSFindOptimalLines(bs, 3, &lines, nullptr);
    ASSERT_EQ(tricks, 3);
    bool saw_s6 = false;
    bool saw_sj = false;
    for (const std::string &line : lines) {
      if (line == "C2 CT S6 CQ ") saw_s6 = true;
      if (line == "C2 CT SJ CQ ") saw_sj = true;
    }
    EXPECT_TRUE(saw_s6);
    EXPECT_TRUE(saw_sj);
  }

  // Once East shows out on C2 while CQ is still unplayed, West is publicly
  // known to hold CQ and must follow suit, so S6 is a single-dummy sure win and
  // SJ is pruned.
  {
    std::vector<uint64_t> hands = {
        ParseHand("S A; D 2; C 2;"),
        ParseHand("S T; H 3; D 3;"),
        ParseHand("S J6; D A;"),
        ParseHand("D 4; C QT;"),
    };
    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(0);
    bs.SetTrump(SPADES);
    bs.PlayCard(0, Card::ParseStringCard("C2"));
    bs.PlayCard(1, Card::ParseStringCard("H3"));

    OptimalLines lines;
    int tricks = DDSFindOptimalLines(bs, 3, &lines, nullptr);
    ASSERT_EQ(tricks, 3);
    bool saw_s6 = false;
    bool saw_sj = false;
    for (const std::string &line : lines) {
      if (line.rfind("C2 H3 S6 ", 0) == 0) saw_s6 = true;
      if (line.rfind("C2 H3 SJ ", 0) == 0) saw_sj = true;
    }
    EXPECT_TRUE(saw_s6);
    EXPECT_FALSE(saw_sj);
  }
}
