#include <catch2/catch_all.hpp>

#include "deal.h"

TEST_CASE("DealTwo") {
  srand48(1);
  uint64_t cards = ParseHand("H5432 D6 C52");
  REQUIRE(CardsInHand(cards, 0).size() == 7);
  auto [hand1, hand2] = DealTwoHands(cards);
  REQUIRE(CardsInHand(hand1, 0).size() == 3);
  REQUIRE(CardsInHand(hand2, 0).size() == 4);
  REQUIRE(HandDebugString(hand1) == "S -; H 2; D -; C 52; ");
  REQUIRE(HandDebugString(hand2) == "S -; H 543; D 6; C -; ");
}

TEST_CASE("DealTwoWithReqs") {
  srand48(1);

  uint64_t cards = ParseHand("H5432 D6 C52");
  REQUIRE(CardsInHand(cards, 0).size() == 7);

  {
    std::vector<std::shared_ptr<HandRequirements>> reqs1, reqs2;
    auto [hand1, hand2] = DealTwoRequiredHands(cards, reqs1, reqs2);
    REQUIRE(CardsInHand(hand1, 0).size() == 3);
    REQUIRE(CardsInHand(hand2, 0).size() == 4);
    REQUIRE(HandDebugString(hand1) == "S -; H 2; D -; C 52; ");
    REQUIRE(HandDebugString(hand2) == "S -; H 543; D 6; C -; ");
  }
  {
    std::vector<std::shared_ptr<HandRequirements>> reqs1, reqs2;
    reqs2.emplace_back(new SuitAtLeast(HEARTS, 4));
    auto [hand1, hand2] = DealTwoRequiredHands(cards, reqs1, reqs2);
    REQUIRE(CardsInHand(hand1, 0).size() == 3);
    REQUIRE(CardsInHand(hand2, 0).size() == 4);
    REQUIRE(HandDebugString(hand1) == "S -; H -; D 6; C 52; ");
    REQUIRE(HandDebugString(hand2) == "S -; H 5432; D -; C -; ");
  }
}

TEST_CASE("DealMissing") {
  std::vector<std::vector<std::shared_ptr<HandRequirements>>> reqs;
  uint64_t hand = ParseHand("S97; HQJ3; DQ972; CJ876");
  std::vector<uint64_t> hands = DealMissingCards(
      std::vector<uint64_t>({0, hand, 0, 0}), reqs);
  for (uint64_t h : hands) {
    LOG(INFO) << "Dealt hand: " << HandDebugString(h);
  }
}
