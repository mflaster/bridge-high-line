#include <catch2/catch_all.hpp>
#include <stdexcept>

#include "cross_hand_summarizer.h"
#include "unknown_cards_invariant.h"

TEST_CASE("AcceptanceCriterion_ExampleHandPairs", "[cross_hand_summarizer]") {
  // All paired deals contain S K432, bifurcated differently
  std::vector<PairedDeal> deals = {
      PairedDeal("S K2", "S 43"),
      PairedDeal("S K3", "S 42"),
      PairedDeal("S K4", "S 32")
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  REQUIRE(report.total_deals == 3);
  REQUIRE(report.cards_per_hand == 2);
  REQUIRE(report.unknown_cards == ParseHand("S K432"));
  REQUIRE(report.HasRule("hand 1 always contains the SK"));
  // Single-suit tautology suppression ensures suit length rules are omitted when all unknown cards are in Spades
  REQUIRE_FALSE(report.HasRule("combined length in S is always 4"));
  REQUIRE_FALSE(report.HasRule("hand 1 length in S is always 2"));
  REQUIRE(report.ToString().find("Invariant Rules for 2 card ending:") != std::string::npos);
}

TEST_CASE("SpotCardFiltering_IgnoresSpots", "[cross_hand_summarizer]") {
  // Hand 1 always has Hearts Ace (H A)
  // Overall set of cards is H A8765432 across all deals
  std::vector<PairedDeal> deals = {
      PairedDeal("H A234", "H 8765"),
      PairedDeal("H A567", "H 8432"),
      PairedDeal("H A8", "H 765432")
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  REQUIRE(report.total_deals == 3);
  REQUIRE(report.HasRule("hand 1 always contains the HA"));
}

TEST_CASE("JointSuitRuleExtractor_SuitBoundsAndTautologySuppression", "[cross_hand_summarizer]") {
  // Spades and Hearts unknown cards
  // Deal 1: Hand 1 = S AK (2), H 2 (1); Hand 2 = S Q (1), H 34 (2)
  // Deal 2: Hand 1 = S AK (2), H 3 (1); Hand 2 = S Q (1), H 24 (2)
  std::vector<PairedDeal> deals = {
      PairedDeal("S AK H 2", "S Q H 34"),
      PairedDeal("S AK H 3", "S Q H 24")
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals, "hand 1", "hand 3");

  REQUIRE(report.total_deals == 2);
  REQUIRE(report.suit_bounds[SPADES].min_hand1 == 2);
  REQUIRE(report.suit_bounds[SPADES].max_hand1 == 2);
  REQUIRE(report.suit_bounds[SPADES].min_hand2 == 1);
  REQUIRE(report.suit_bounds[SPADES].max_hand2 == 1);

  REQUIRE(report.HasRule("hand 1 length in S is always 2"));
  REQUIRE(report.HasRule("hand 3 length in S is always 1"));
  REQUIRE(report.HasRule("hand 1 length in H is always 1"));
  REQUIRE(report.HasRule("hand 3 length in H is always 2"));

  // Verify tautologies are suppressed
  REQUIRE_FALSE(report.HasRule("combined length in S is always 3"));
  REQUIRE_FALSE(report.HasRule("hand 1 length in D is always 0"));
  REQUIRE_FALSE(report.HasRule("hand 3 length in C is always 0"));
}

TEST_CASE("HonorPossessionClassifier_MultipleHonors", "[cross_hand_summarizer]") {
  // Overall set of cards: S AT, H AK, D KQ, C QJT
  std::vector<PairedDeal> deals = {
      PairedDeal("S A H K D Q C J", "S T H A D K C QT"),
      PairedDeal("S A H K D Q C T", "S T H A D K C QJ")
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals, "hand 1", "hand 3");

  REQUIRE(report.total_deals == 2);
  REQUIRE(report.HasRule("hand 1 always contains the SA"));
  REQUIRE(report.HasRule("hand 1 always contains the HK"));
  REQUIRE(report.HasRule("hand 1 always contains the DQ"));
  REQUIRE(report.HasRule("hand 3 always contains the HA"));
  REQUIRE(report.HasRule("hand 3 always contains the DK"));
}

TEST_CASE("MaximumSuitLength_AcrossBifurcations", "[cross_hand_summarizer]") {
  // Test case provided by reviewer:
  // Overall cards across all deals: S 5432, H 5432
  std::vector<PairedDeal> deals = {
      PairedDeal("S 32 H 32", "S 54 H 54"),
      PairedDeal("S 42 H 42", "S 53 H 53"),
      PairedDeal("S 432 H 2", "S 5 H 543")
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  REQUIRE(report.total_deals == 3);
  REQUIRE(report.HasRule("maximum length in S in any hand is 3"));
  REQUIRE(report.HasRule("maximum length in H in any hand is 3"));
}

TEST_CASE("Validation_InvalidDealSetThrows", "[cross_hand_summarizer]") {
  CrossHandDealSetSummarizer summarizer;

  // Overlapping cards in hand 1 and hand 2 in a single deal
  std::vector<PairedDeal> overlapping_deals = {
      PairedDeal("S K2", "S K4")
  };
  REQUIRE_THROWS_AS(summarizer.Summarize(overlapping_deals), std::invalid_argument);

  // Mismatched overall card set across deals
  std::vector<PairedDeal> mismatched_deals = {
      PairedDeal("S K2", "S 34"),
      PairedDeal("S A2", "S 34")
  };
  REQUIRE_THROWS_AS(summarizer.Summarize(mismatched_deals), std::invalid_argument);
}

TEST_CASE("EmptyDealSet", "[cross_hand_summarizer]") {
  std::vector<PairedDeal> deals;

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  REQUIRE(report.total_deals == 0);
  REQUIRE(report.invariant_rules.empty());
}

TEST_CASE("CrossSuitConditionalRuleExtractor_ExtractsHandWithXAlsoHasYRules", "[cross_hand_summarizer]") {
  // Deal 1: Hand 1 = S AK4 H Q43 (3 S, 3 H); Hand 2 = S J3 H JT2 (2 S, 3 H)
  // Deal 2: Hand 1 = S A4 H Q4 (2 S, 2 H); Hand 2 = S KJ3 H JT32 (3 S, 4 H)
  std::vector<PairedDeal> deals = {
      PairedDeal("S AK4 H Q43", "S J3 H JT2"),
      PairedDeal("S A4 H Q4", "S KJ3 H JT32")
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  REQUIRE(report.total_deals == 2);
  REQUIRE(report.HasRule("the hand with the SK also has exactly 3 S"));
}

TEST_CASE("CrossSuitConditionalRuleExtractor_CoPossessionOfCards", "[cross_hand_summarizer]") {
  // S A and S T are always in the same hand across all deals
  std::vector<PairedDeal> deals = {
      PairedDeal("S AT H 5", "S K43 H 2"),
      PairedDeal("S K43 H 2", "S AT H 5")
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  REQUIRE(report.total_deals == 2);
  REQUIRE(report.HasRule("the hand with the SA also has the ST"));
  REQUIRE(report.HasRule("the hand with the ST also has the SA"));
}

TEST_CASE("RandomBifurcation_TautologyElimination", "[cross_hand_summarizer]") {
  // Unknown cards pool: 1 Heart (HT), 3 Spades (S T98), 4 Diamonds (D AKQJ)
  // Total cards = 8, 4 cards per hand.
  // In any bifurcation, whichever hand gets HT has 4 cards, so it ALWAYS has 1 Heart (as HT is the only Heart)
  // and at most 3 Diamonds (since 4 - 1 = 3).
  std::vector<PairedDeal> deals = {
      PairedDeal("S T9 H T D A", "S 8 D KQJ"),
      PairedDeal("S T8 H T D K", "S 9 D AQJ")
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  REQUIRE(report.total_deals == 2);
  // Rule "the hand with the HT also has exactly 1 H" is tautological for ALL random bifurcations and must be omitted
  REQUIRE_FALSE(report.HasRule("the hand with the HT also has exactly 1 H"));
  REQUIRE_FALSE(report.HasRule("the hand with the HT also has at most 3 D"));
}

TEST_CASE("EvaluateRuleOnDeal_DirectTests", "[cross_hand_summarizer]") {
  PairedDeal deal("S AK H Q D 4", "S QJ H 2 D 32");

  REQUIRE(EvaluateRuleOnDeal("the hand with the SA also has the SK", deal));
  REQUIRE(EvaluateRuleOnDeal("the hand with the SA also has exactly 1 H", deal));
  REQUIRE(EvaluateRuleOnDeal("the hand with the SA also has at least 2 S", deal));
  REQUIRE(EvaluateRuleOnDeal("the hand with the SA also has at most 1 D", deal));

  REQUIRE(EvaluateRuleOnDeal("hand 1 always contains the SA", deal));
  REQUIRE(EvaluateRuleOnDeal("hand 2 always contains the SQ", deal));
  REQUIRE_FALSE(EvaluateRuleOnDeal("both hands always contain the SA", deal));

  REQUIRE(EvaluateRuleOnDeal("hand 1 length in S is always 2", deal));
  REQUIRE(EvaluateRuleOnDeal("hand 1 length in S is between 1 and 3", deal));
  REQUIRE(EvaluateRuleOnDeal("maximum length in S in any hand is 2", deal));
}

TEST_CASE("CrossSuitConditionalRuleExtractor_SuppressesTautologiesAndFixedCards", "[cross_hand_summarizer]") {
  // S A is statically fixed in Hand 1 across all deals -> no "the hand with the SA" rule
  // Card CJ is not in unknown cards -> no "the hand with the CJ" rule
  std::vector<PairedDeal> deals = {
      PairedDeal("S A4 H 54", "S T3 H 32"),
      PairedDeal("S A3 H 53", "S T4 H 42")
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  REQUIRE(report.total_deals == 2);
  REQUIRE(report.HasRule("hand 1 always contains the SA"));
  REQUIRE_FALSE(report.HasRule("the hand with the SA"));
  REQUIRE_FALSE(report.HasRule("the hand with the CJ"));
}

TEST_CASE("UnknownCardsInvariantClassHierarchy_FromStringToStringAndHoldsOnDeal", "[cross_hand_summarizer]") {
  PairedDeal deal("S AK H Q D 4", "S QJ H 2 D 32");

  // 1. CrossSuitConditionalInvariant
  {
    std::string rule_str = "the hand with the SA also has the SK";
    auto inv = UnknownCardsInvariant::FromString(rule_str);
    REQUIRE(inv != nullptr);
    REQUIRE(inv->ToString() == rule_str);
    REQUIRE(inv->HoldsOnDeal(deal));

    auto derived = dynamic_cast<CrossSuitCardConditionalInvariant*>(inv.get());
    REQUIRE(derived != nullptr);
    REQUIRE(derived->condition_card() == Card::ParseStringCard("SA"));
    REQUIRE(derived->target_card() == Card::ParseStringCard("SK"));
  }

  {
    std::string rule_str = "the hand with the SA also has exactly 1 H";
    auto inv = UnknownCardsInvariant::FromString(rule_str);
    REQUIRE(inv != nullptr);
    REQUIRE(inv->ToString() == rule_str);
    REQUIRE(inv->HoldsOnDeal(deal));

    auto derived = dynamic_cast<CrossSuitExactSuitLengthInvariant*>(inv.get());
    REQUIRE(derived != nullptr);
    REQUIRE(derived->condition_card() == Card::ParseStringCard("SA"));
    REQUIRE(derived->target_suit() == Suit::HEARTS);
    REQUIRE(derived->length() == 1);
  }

  {
    std::string rule_str = "the hand with the SA also has at least 2 S";
    auto inv = UnknownCardsInvariant::FromString(rule_str);
    REQUIRE(inv != nullptr);
    REQUIRE(inv->ToString() == rule_str);
    REQUIRE(inv->HoldsOnDeal(deal));

    auto derived = dynamic_cast<CrossSuitMinSuitLengthInvariant*>(inv.get());
    REQUIRE(derived != nullptr);
    REQUIRE(derived->condition_card() == Card::ParseStringCard("SA"));
    REQUIRE(derived->target_suit() == Suit::SPADES);
    REQUIRE(derived->min_len() == 2);
  }

  {
    std::string rule_str = "the hand with the SA also has at most 1 D";
    auto inv = UnknownCardsInvariant::FromString(rule_str);
    REQUIRE(inv != nullptr);
    REQUIRE(inv->ToString() == rule_str);
    REQUIRE(inv->HoldsOnDeal(deal));

    auto derived = dynamic_cast<CrossSuitMaxSuitLengthInvariant*>(inv.get());
    REQUIRE(derived != nullptr);
    REQUIRE(derived->condition_card() == Card::ParseStringCard("SA"));
    REQUIRE(derived->target_suit() == Suit::DIAMONDS);
    REQUIRE(derived->max_len() == 1);
  }

  // 2. HonorPossessionInvariant
  {
    std::string rule_str = "hand 1 always contains the SA";
    auto inv = UnknownCardsInvariant::FromString(rule_str);
    REQUIRE(inv != nullptr);
    REQUIRE(inv->ToString() == rule_str);
    REQUIRE(inv->HoldsOnDeal(deal));

    auto derived = HonorPossessionInvariant::FromString(rule_str);
    REQUIRE(derived != nullptr);
    REQUIRE(derived->target_hand() == HonorPossessionTargetHand::kHand1);
    REQUIRE(derived->card() == Card::ParseStringCard("SA"));
  }

  {
    std::string rule_str = "both hands always contain the SA";
    auto inv = UnknownCardsInvariant::FromString(rule_str);
    REQUIRE(inv != nullptr);
    REQUIRE(inv->ToString() == rule_str);
    REQUIRE_FALSE(inv->HoldsOnDeal(deal));
  }

  // 3. MaxSuitLengthInvariant
  {
    std::string rule_str = "maximum length in S in any hand is 2";
    auto inv = UnknownCardsInvariant::FromString(rule_str);
    REQUIRE(inv != nullptr);
    REQUIRE(inv->ToString() == rule_str);
    REQUIRE(inv->HoldsOnDeal(deal));

    auto derived = MaxSuitLengthInvariant::FromString(rule_str);
    REQUIRE(derived != nullptr);
    REQUIRE(derived->suit() == Suit::SPADES);
    REQUIRE(derived->max_len() == 2);
  }

  // 4. ExactSuitLengthInvariant
  {
    std::string rule_str = "hand 1 length in S is always 2";
    auto inv = UnknownCardsInvariant::FromString(rule_str);
    REQUIRE(inv != nullptr);
    REQUIRE(inv->ToString() == rule_str);
    REQUIRE(inv->HoldsOnDeal(deal));

    auto derived = ExactSuitLengthInvariant::FromString(rule_str);
    REQUIRE(derived != nullptr);
    REQUIRE(derived->hand_choice() == InvariantHandChoice::kHand1);
    REQUIRE(derived->suit() == Suit::SPADES);
    REQUIRE(derived->exact_len() == 2);
  }

  // 5. SuitLengthRangeInvariant
  {
    std::string rule_str = "hand 1 length in S is between 1 and 3";
    auto inv = UnknownCardsInvariant::FromString(rule_str);
    REQUIRE(inv != nullptr);
    REQUIRE(inv->ToString() == rule_str);
    REQUIRE(inv->HoldsOnDeal(deal));

    auto derived = SuitLengthRangeInvariant::FromString(rule_str);
    REQUIRE(derived != nullptr);
    REQUIRE(derived->hand_choice() == InvariantHandChoice::kHand1);
    REQUIRE(derived->suit() == Suit::SPADES);
    REQUIRE(derived->min_len() == 1);
    REQUIRE(derived->max_len() == 3);
  }

  // 6. Invalid String Parsing Returns nullptr
  {
    REQUIRE(UnknownCardsInvariant::FromString("this is an invalid rule") == nullptr);
    REQUIRE(UnknownCardsInvariant::FromString("the hand with the SA also has bad format") == nullptr);
    REQUIRE(UnknownCardsInvariant::FromString("hand 1 length in S is between invalid") == nullptr);
    REQUIRE(CrossSuitConditionalInvariant::FromString("hand 1 always contains the SA") == nullptr);
    REQUIRE(HonorPossessionInvariant::FromString("maximum length in S in any hand is 2") == nullptr);
    REQUIRE(MaxSuitLengthInvariant::FromString("hand 1 always contains the SA") == nullptr);
    REQUIRE(ExactSuitLengthInvariant::FromString("hand 1 length in S is between 1 and 3") == nullptr);
    REQUIRE(SuitLengthRangeInvariant::FromString("hand 1 length in S is always 2") == nullptr);
  }
}

