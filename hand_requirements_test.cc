#include <catch2/catch_all.hpp>

#include "hand_requirements.h"

TEST_CASE("basic") {
  uint64_t hand = ParseHand("S KQJT987 H KQJ; D KQJ; C 9; ");
  REQUIRE(SuitAtLeast(SPADES, 8).MeetsRequirement(hand) == false);
  REQUIRE(SuitAtLeast(SPADES, 7).MeetsRequirement(hand) == true);
  REQUIRE(SuitAtLeast(SPADES, 6).MeetsRequirement(hand) == true);

  REQUIRE(SuitAtLeast(HEARTS, 4).MeetsRequirement(hand) == false);
  REQUIRE(SuitAtLeast(HEARTS, 3).MeetsRequirement(hand) == true);
  REQUIRE(SuitAtLeast(HEARTS, 2).MeetsRequirement(hand) == true);
}
