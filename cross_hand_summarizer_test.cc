#include <catch2/catch_all.hpp>
#include <stdexcept>

#include "cross_hand_summarizer.h"

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
  REQUIRE(report.HasRule("hand 1 always contains the SK"));
  REQUIRE(report.HasRule("maximum length in S in any hand is 2"));
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

TEST_CASE("JointSuitRuleExtractor_SuitBoundsAndFit", "[cross_hand_summarizer]") {
  // Spades fit: Overall set S AKQJT9876 (9 spades) in all deals
  std::vector<PairedDeal> deals = {
      PairedDeal("S AKQJT", "S 9876"),
      PairedDeal("S AKQ98", "S JT76")
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  REQUIRE(report.total_deals == 2);
  REQUIRE(report.suit_bounds[SPADES].min_hand1 == 5);
  REQUIRE(report.suit_bounds[SPADES].max_hand1 == 5);
  REQUIRE(report.suit_bounds[SPADES].min_hand2 == 4);
  REQUIRE(report.suit_bounds[SPADES].max_hand2 == 4);
  REQUIRE(report.suit_bounds[SPADES].min_combined == 9);
  REQUIRE(report.suit_bounds[SPADES].max_combined == 9);

  REQUIRE(report.HasRule("combined length in S is always 9"));
  REQUIRE(report.HasRule("combined length in S has an 8+ card fit"));
  REQUIRE(report.HasRule("maximum length in S in any hand is 5"));
}

TEST_CASE("HonorPossessionClassifier_MultipleHonors", "[cross_hand_summarizer]") {
  // Overall set of cards: S AT, H AK, D KQ, C QJT
  std::vector<PairedDeal> deals = {
      PairedDeal("S A H K D Q C J", "S T H A D K C QT"),
      PairedDeal("S A H K D Q C T", "S T H A D K C QJ")
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  REQUIRE(report.total_deals == 2);
  REQUIRE(report.HasRule("hand 1 always contains the SA"));
  REQUIRE(report.HasRule("hand 1 always contains the HK"));
  REQUIRE(report.HasRule("hand 1 always contains the DQ"));
  REQUIRE(report.HasRule("hand 2 always contains the HA"));
  REQUIRE(report.HasRule("hand 2 always contains the DK"));
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
