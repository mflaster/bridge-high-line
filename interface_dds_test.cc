#include <iostream>
#include <catch2/catch_all.hpp>

#include "interface_dds.h"
#include "board_state.h"

TEST_CASE("Basic") {
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
  REQUIRE(ans == 1);
}

std::shared_ptr<OptimalPlayTree> FindOptimalPlayTree(
    const BoardState &bs, int min_tricks) {
  return OptimalPlayTree::FindOptimalPlayTree(bs, min_tricks);
}

TEST_CASE("PlayTree") {
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
  REQUIRE(all_plays->TotalCount() == 1);

  all_plays->Deepen(10, nullptr);
  std::cout << "Got " << all_plays->DebugString() << "\n";
  REQUIRE(all_plays->TotalCount() == 2);
}

TEST_CASE("PlayTree2") {
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
  REQUIRE(all_plays->TotalCount() == 4);
}

TEST_CASE("PlayTree3") {
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
  REQUIRE(all_plays->TotalCount() == 10);
}

TEST_CASE("PlayTree8", "[.slow]") {
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
  REQUIRE(all_plays->TotalCount() > 10);
}

TEST_CASE("PlayTree11", "[.slow]") {
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
  REQUIRE(all_plays->TotalCount() > 10);
}

TEST_CASE("DefenderSeatRules_SecondHandLowAndEquals") {
  // Hand 1 holds touching QJ in 2nd seat; Hand 3 holds touching 54 in 4th seat.
  // Both should play the lowest card in their DDS equivalence group (SJ, S4)
  // rather than the highest (SQ, S5).
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

  OptimalLines lines;
  DDSFindOptimalLines(bs, 2, &lines, nullptr);
  REQUIRE(lines.count("SA SJ S8 S4 ") == 1);
}

TEST_CASE("DefenderSeatRules_ThirdHandHighAndEquals") {
  // Hand 1 leads a small spade (S3), Hand 2 plays low (S2), Hand 3 in 3rd seat
  // holds KQ5 with Hand 0 holding AT9 in 4th seat. Whether 3rd hand plays KQ
  // or 5 yields the same double-dummy trick count (1 trick for E/W), but 3rd
  // hand must play high and specifically the lowest of touching honors (SQ).
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

  OptimalLines lines;
  DDSFindOptimalLines(bs, 0, &lines, nullptr);
  REQUIRE(!lines.empty());
  for (const std::string &line : lines) {
    // Prefix is "S3 S2 ", followed by 3rd hand's card.
    REQUIRE(line.rfind("S3 S2 SQ ", 0) == 0);
  }
}

TEST_CASE("DefenderSeatRules_FourthHandWinCheaplyAndNoOvertake") {
  SECTION("4th hand wins as cheaply as possible instead of ducking") {
    // Hand 2 leads S8, Hand 3 plays S5, Hand 0 plays ST. Hand 1 in 4th seat
    // holds QJ4 while Hand 2 has SA2 remaining. Winning with SJ/SQ or ducking
    // with S4 both yield 2 tricks for E/W (only SA for N/S), so all 3 cards
    // tie in DDS; 4th hand must win as cheaply as possible with SJ.
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

    OptimalLines lines;
    DDSFindOptimalLines(bs, 0, &lines, nullptr);
    REQUIRE(lines.size() == 1);
    REQUIRE(*lines.begin() == "S8 S5 ST SJ ");
  }

  SECTION("4th hand does not overtake partner's winning card") {
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
    REQUIRE(lines.size() == 1);
    REQUIRE(*lines.begin() == "S2 SJ S3 S4 ");
  }
}

