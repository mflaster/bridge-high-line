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

TEST_CASE("anti_and_any_of_requirements") {
  uint64_t hand4 = ParseHand("S J543 D 5432");
  uint64_t hand0 = ParseHand("D 5432 C 5432");
  uint64_t hand2 = ParseHand("S 54 D 5432 C 54");

  auto not_1_to_3 = std::make_shared<AntiRequirement>(
      std::make_shared<SuitLength>(SPADES, 1, 3));

  REQUIRE(not_1_to_3->MeetsRequirement(hand4) == true);
  REQUIRE(not_1_to_3->MeetsRequirement(hand0) == true);
  REQUIRE(not_1_to_3->MeetsRequirement(hand2) == false);

  auto zero_or_four = std::make_shared<AnyOfRequirement>(
      std::vector<std::shared_ptr<HandRequirements>>{
          std::make_shared<SuitLength>(SPADES, 0, 0),
          std::make_shared<SuitLength>(SPADES, 4, 4)});

  REQUIRE(zero_or_four->MeetsRequirement(hand4) == true);
  REQUIRE(zero_or_four->MeetsRequirement(hand0) == true);
  REQUIRE(zero_or_four->MeetsRequirement(hand2) == false);
}

