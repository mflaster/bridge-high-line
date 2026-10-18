#include <gtest/gtest.h>

#include "card.h"
#include "hand.h"
#include "hand_requirements.h"

TEST(HandRequirementsTest, basic) {
  uint64_t hand = ParseHand("S KQJT987 H KQJ; D KQJ; C 9; ");
  ASSERT_TRUE(SuitAtLeast(SPADES, 8).MeetsRequirement(hand) == false);
  ASSERT_TRUE(SuitAtLeast(SPADES, 7).MeetsRequirement(hand) == true);
  ASSERT_TRUE(SuitAtLeast(SPADES, 6).MeetsRequirement(hand) == true);

  ASSERT_TRUE(SuitAtLeast(HEARTS, 4).MeetsRequirement(hand) == false);
  ASSERT_TRUE(SuitAtLeast(HEARTS, 3).MeetsRequirement(hand) == true);
  ASSERT_TRUE(SuitAtLeast(HEARTS, 2).MeetsRequirement(hand) == true);
}

TEST(HandRequirementsTest, anti_and_any_of_requirements) {
  uint64_t hand4 = ParseHand("S J543 D 5432");
  uint64_t hand0 = ParseHand("D 5432 C 5432");
  uint64_t hand2 = ParseHand("S 54 D 5432 C 54");

  auto not_1_to_3 = std::make_shared<AntiRequirement>(
      std::make_shared<SuitLength>(SPADES, 1, 3));

  ASSERT_TRUE(not_1_to_3->MeetsRequirement(hand4) == true);
  ASSERT_TRUE(not_1_to_3->MeetsRequirement(hand0) == true);
  ASSERT_TRUE(not_1_to_3->MeetsRequirement(hand2) == false);

  auto zero_or_four = std::make_shared<AnyOfRequirement>(
      std::vector<std::shared_ptr<HandRequirements>>{
          std::make_shared<SuitLength>(SPADES, 0, 0),
          std::make_shared<SuitLength>(SPADES, 4, 4)});

  ASSERT_TRUE(zero_or_four->MeetsRequirement(hand4) == true);
  ASSERT_TRUE(zero_or_four->MeetsRequirement(hand0) == true);
  ASSERT_TRUE(zero_or_four->MeetsRequirement(hand2) == false);
}

TEST(HandRequirementsTest, HonorBitmask_ExtractionAndCounts) {
  // Empty hand
  uint64_t empty_hand = 0;
  ASSERT_TRUE(ExtractHonors(empty_hand) == 0ULL);
  ASSERT_TRUE(CountHonorsInHand(empty_hand) == 0);

  // Spot cards only (ranks 2-9)
  uint64_t spot_hand = ParseHand("S 98765432 H 98765");
  ASSERT_TRUE(ExtractHonors(spot_hand) == 0ULL);
  ASSERT_TRUE(CountHonorsInHand(spot_hand) == 0);

  // Hand with specific honors: S A, H K, D Q, C J
  uint64_t honor_hand = ParseHand("S A H K D Q C J");
  uint64_t expected_mask = Card::GetSuitedCard(SPADES, 12) |
                          Card::GetSuitedCard(HEARTS, 11) |
                          Card::GetSuitedCard(DIAMONDS, 10) |
                          Card::GetSuitedCard(CLUBS, 9);
  ASSERT_TRUE(ExtractHonors(honor_hand) == expected_mask);
  ASSERT_TRUE(CountHonorsInHand(honor_hand) == 4);

  // All honors (A, K, Q, J, 10 across all 4 suits)
  uint64_t all_honors = ParseHand("S AKQJT H AKQJT D AKQJT C AKQJT");
  ASSERT_TRUE(ExtractHonors(all_honors) == kHonorMask64);
  ASSERT_TRUE(CountHonorsInHand(all_honors) == 20);
}

TEST(HandRequirementsTest, FastHighCardPoints_Calculation) {
  ASSERT_TRUE(GetHighCardPoints(0) == 0);

  // Spot cards have 0 HCP
  ASSERT_TRUE(GetHighCardPoints(ParseHand("S 98765 H 9876 D 987 C 98")) == 0);

  // 40 HCP hand (all A, K, Q, J)
  ASSERT_TRUE(GetHighCardPoints(ParseHand("S AKQJ H AKQJ D AKQJ C AKQJ")) == 40);

  // 10 HCP hand (S AKQ2 = 9, H J = 1)
  ASSERT_TRUE(GetHighCardPoints(ParseHand("S AKQ2 H JT9 D 876 C 543")) == 10);

  // Requirement test with AtLeastAces
  uint64_t aces_hand = ParseHand("S A H A D K C Q");
  ASSERT_TRUE(AtLeastAces(2).MeetsRequirement(aces_hand) == true);
  ASSERT_TRUE(AtLeastAces(3).MeetsRequirement(aces_hand) == false);
}

TEST(HandRequirementsTest, HonorPossessionMatrix_Classification) {
  // Hand 1: S AKQ 2; Hand 2: S JT 3
  uint64_t h1 = ParseHand("S AKQ2");
  uint64_t h2 = ParseHand("S JT3");

  HonorPossessionMatrix matrix = ClassifyHonorPossession(h1, h2);

  ASSERT_TRUE(matrix.hand1_honor_count == 3);
  ASSERT_TRUE(matrix.hand2_honor_count == 2);
  ASSERT_TRUE(matrix.joint_honor_count == 5);
  ASSERT_TRUE(matrix.shared_honors == 0ULL);
  ASSERT_TRUE(matrix.neither_honors == (kHonorMask64 & ~matrix.joint_honors));

  ASSERT_TRUE(matrix.Hand1HasHonor(SPADES, 12) == true);  // Ace
  ASSERT_TRUE(matrix.Hand1HasHonor(SPADES, 11) == true);  // King
  ASSERT_TRUE(matrix.Hand1HasHonor(SPADES, 10) == true);  // Queen
  ASSERT_TRUE(matrix.Hand1HasHonor(SPADES, 9) == false);  // Jack
  ASSERT_TRUE(matrix.Hand2HasHonor(SPADES, 9) == true);   // Jack
  ASSERT_TRUE(matrix.Hand2HasHonor(SPADES, 8) == true);   // Ten
  ASSERT_TRUE(matrix.JointHasHonor(SPADES, 12) == true);

  ASSERT_TRUE(matrix.Hand1SuitHonors(SPADES) == 0x1C00);
  ASSERT_TRUE(matrix.Hand2SuitHonors(SPADES) == 0x0300);

  // Zero cards hands
  HonorPossessionMatrix empty_matrix = ClassifyHonorPossession(0, 0);
  ASSERT_TRUE(empty_matrix.hand1_honors == 0ULL);
  ASSERT_TRUE(empty_matrix.hand2_honors == 0ULL);
  ASSERT_TRUE(empty_matrix.shared_honors == 0ULL);
  ASSERT_TRUE(empty_matrix.joint_honors == 0ULL);
  ASSERT_TRUE(empty_matrix.neither_honors == kHonorMask64);
  ASSERT_TRUE(empty_matrix.hand1_honor_count == 0);
  ASSERT_TRUE(empty_matrix.hand2_honor_count == 0);
  ASSERT_TRUE(empty_matrix.joint_honor_count == 0);
}

TEST(HandRequirementsTest, ToString_and_FromString) {
  // 1. SuitAtLeast
  SuitAtLeast sal(SPADES, 4);
  ASSERT_TRUE(sal.ToString() == "SuitAtLeast(Spades, 4)");
  auto sal_parsed = HandRequirements::FromString("SuitAtLeast(Spades, 4)");
  ASSERT_TRUE(sal_parsed != nullptr);
  ASSERT_TRUE(sal_parsed->ToString() == "SuitAtLeast(Spades, 4)");

  // 2. SuitLength
  SuitLength sl(HEARTS, 1, 3);
  ASSERT_TRUE(sl.ToString() == "SuitLength(Hearts, 1, 3)");
  auto sl_parsed = HandRequirements::FromString("SuitLength(Hearts, 1, 3)");
  ASSERT_TRUE(sl_parsed != nullptr);
  ASSERT_TRUE(sl_parsed->ToString() == "SuitLength(Hearts, 1, 3)");

  // 3. BalancedHand
  BalancedHand bh;
  ASSERT_TRUE(bh.ToString() == "BalancedHand()");
  auto bh_parsed = HandRequirements::FromString("BalancedHand()");
  ASSERT_TRUE(bh_parsed != nullptr);
  ASSERT_TRUE(bh_parsed->ToString() == "BalancedHand()");

  // 4. AntiRequirement
  auto anti = std::make_shared<AntiRequirement>(std::make_shared<SuitLength>(SPADES, 1, 3));
  ASSERT_TRUE(anti->ToString() == "Anti(SuitLength(Spades, 1, 3))");
  auto anti_parsed = HandRequirements::FromString("Anti(SuitLength(Spades,1,3))");
  ASSERT_TRUE(anti_parsed != nullptr);
  ASSERT_TRUE(anti_parsed->ToString() == "Anti(SuitLength(Spades, 1, 3))");

  // 5. HighCardRange
  HighCardRange hcr(15, 17);
  ASSERT_TRUE(hcr.ToString() == "HighCardRange(15, 17)");
  auto hcr_parsed = HandRequirements::FromString("HighCardRange(15, 17)");
  ASSERT_TRUE(hcr_parsed != nullptr);
  ASSERT_TRUE(hcr_parsed->ToString() == "HighCardRange(15, 17)");

  // 6. HandHasCard
  HandHasCard hhc(Card::ParseStringCard("SK"));
  ASSERT_TRUE(hhc.ToString() == "HandHasCard(SK)");
  auto hhc_parsed = HandRequirements::FromString("HandHasCard(SK)");
  ASSERT_TRUE(hhc_parsed != nullptr);
  ASSERT_TRUE(hhc_parsed->ToString() == "HandHasCard(SK)");

  // 7. AtLeastAces
  AtLeastAces ala(2);
  ASSERT_TRUE(ala.ToString() == "AtLeastAces(2)");
  auto ala_parsed = HandRequirements::FromString("AtLeastAces(2)");
  ASSERT_TRUE(ala_parsed != nullptr);
  ASSERT_TRUE(ala_parsed->ToString() == "AtLeastAces(2)");

  // 8. AnyOfRequirement
  std::vector<std::shared_ptr<HandRequirements>> reqs;
  reqs.push_back(std::make_shared<SuitLength>(SPADES, 0, 0));
  reqs.push_back(std::make_shared<SuitLength>(SPADES, 4, 4));
  AnyOfRequirement anyof(reqs);
  ASSERT_TRUE(anyof.ToString() == "AnyOf(SuitLength(Spades, 0, 0), SuitLength(Spades, 4, 4))");
  auto anyof_parsed = HandRequirements::FromString("AnyOf(SuitLength(Spades, 0, 0), SuitLength(Spades, 4, 4))");
  ASSERT_TRUE(anyof_parsed != nullptr);
  ASSERT_TRUE(anyof_parsed->ToString() == "AnyOf(SuitLength(Spades, 0, 0), SuitLength(Spades, 4, 4))");

  // Verify behavior equivalence on sample hand
  uint64_t hand4 = ParseHand("S J543 D 5432");
  uint64_t hand2 = ParseHand("S 54 D 5432 C 54");
  ASSERT_TRUE(anti_parsed->MeetsRequirement(hand4) == true);
  ASSERT_TRUE(anti_parsed->MeetsRequirement(hand2) == false);
}
