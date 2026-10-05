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
