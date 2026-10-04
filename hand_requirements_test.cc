#include <catch2/catch_all.hpp>

#include "card.h"
#include "hand.h"
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

TEST_CASE("HonorBitmask_ExtractionAndCounts") {
  // Empty hand
  uint64_t empty_hand = 0;
  REQUIRE(ExtractHonors(empty_hand) == 0ULL);
  REQUIRE(CountHonorsInHand(empty_hand) == 0);

  // Spot cards only (ranks 2-9)
  uint64_t spot_hand = ParseHand("S 98765432 H 98765");
  REQUIRE(ExtractHonors(spot_hand) == 0ULL);
  REQUIRE(CountHonorsInHand(spot_hand) == 0);

  // Hand with specific honors: S A, H K, D Q, C J
  uint64_t honor_hand = ParseHand("S A H K D Q C J");
  uint64_t expected_mask = Card::GetSuitedCard(SPADES, 12) |
                          Card::GetSuitedCard(HEARTS, 11) |
                          Card::GetSuitedCard(DIAMONDS, 10) |
                          Card::GetSuitedCard(CLUBS, 9);
  REQUIRE(ExtractHonors(honor_hand) == expected_mask);
  REQUIRE(CountHonorsInHand(honor_hand) == 4);

  // All honors (A, K, Q, J, 10 across all 4 suits)
  uint64_t all_honors = ParseHand("S AKQJT H AKQJT D AKQJT C AKQJT");
  REQUIRE(ExtractHonors(all_honors) == kHonorMask64);
  REQUIRE(CountHonorsInHand(all_honors) == 20);
}

TEST_CASE("FastHighCardPoints_Calculation") {
  REQUIRE(GetHighCardPoints(0) == 0);

  // Spot cards have 0 HCP
  REQUIRE(GetHighCardPoints(ParseHand("S 98765 H 9876 D 987 C 98")) == 0);

  // 40 HCP hand (all A, K, Q, J)
  REQUIRE(GetHighCardPoints(ParseHand("S AKQJ H AKQJ D AKQJ C AKQJ")) == 40);

  // 10 HCP hand (S AKQ2 = 9, H J = 1)
  REQUIRE(GetHighCardPoints(ParseHand("S AKQ2 H JT9 D 876 C 543")) == 10);

  // Requirement test with AtLeastAces
  uint64_t aces_hand = ParseHand("S A H A D K C Q");
  REQUIRE(AtLeastAces(2).MeetsRequirement(aces_hand) == true);
  REQUIRE(AtLeastAces(3).MeetsRequirement(aces_hand) == false);
}

TEST_CASE("HonorPossessionMatrix_Classification") {
  // Hand 1: S AKQ 2; Hand 2: S JT 3
  uint64_t h1 = ParseHand("S AKQ2");
  uint64_t h2 = ParseHand("S JT3");

  HonorPossessionMatrix matrix = ClassifyHonorPossession(h1, h2);

  REQUIRE(matrix.hand1_honor_count == 3);
  REQUIRE(matrix.hand2_honor_count == 2);
  REQUIRE(matrix.joint_honor_count == 5);
  REQUIRE(matrix.shared_honors == 0ULL);
  REQUIRE(matrix.neither_honors == (kHonorMask64 & ~matrix.joint_honors));

  REQUIRE(matrix.Hand1HasHonor(SPADES, 12) == true);  // Ace
  REQUIRE(matrix.Hand1HasHonor(SPADES, 11) == true);  // King
  REQUIRE(matrix.Hand1HasHonor(SPADES, 10) == true);  // Queen
  REQUIRE(matrix.Hand1HasHonor(SPADES, 9) == false);  // Jack
  REQUIRE(matrix.Hand2HasHonor(SPADES, 9) == true);   // Jack
  REQUIRE(matrix.Hand2HasHonor(SPADES, 8) == true);   // Ten
  REQUIRE(matrix.JointHasHonor(SPADES, 12) == true);

  REQUIRE(matrix.Hand1SuitHonors(SPADES) == 0x1C00);
  REQUIRE(matrix.Hand2SuitHonors(SPADES) == 0x0300);

  // Zero cards hands
  HonorPossessionMatrix empty_matrix = ClassifyHonorPossession(0, 0);
  REQUIRE(empty_matrix.hand1_honors == 0ULL);
  REQUIRE(empty_matrix.hand2_honors == 0ULL);
  REQUIRE(empty_matrix.shared_honors == 0ULL);
  REQUIRE(empty_matrix.joint_honors == 0ULL);
  REQUIRE(empty_matrix.neither_honors == kHonorMask64);
  REQUIRE(empty_matrix.hand1_honor_count == 0);
  REQUIRE(empty_matrix.hand2_honor_count == 0);
  REQUIRE(empty_matrix.joint_honor_count == 0);
}
