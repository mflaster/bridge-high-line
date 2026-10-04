#include <catch2/catch_all.hpp>

#include "cross_hand_summarizer.h"

TEST_CASE("AcceptanceCriterion_ExampleHandPairs", "[cross_hand_summarizer]") {
  std::vector<PairedDeal> deals = {
      PairedDeal("S K2", "S 34"),
      PairedDeal("S K3", "S 42"),
      PairedDeal("S K4", "S 32")
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  REQUIRE(report.total_deals == 3);
  REQUIRE(report.HasRule("hand 1 always contains the SK"));
}

TEST_CASE("SpotCardFiltering_IgnoresSpots", "[cross_hand_summarizer]") {
  // Hand 1 always has Hearts Ace (H A) and varying spot cards (2, 3, 4, 5, 6, 7, 8, 9)
  // Hand 2 has varying spot cards in Hearts
  std::vector<PairedDeal> deals = {
      PairedDeal("H A2", "H 34"),
      PairedDeal("H A56", "H 78"),
      PairedDeal("H A9", "H 234")
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  REQUIRE(report.total_deals == 3);
  REQUIRE(report.HasRule("hand 1 always contains the HA"));
}

TEST_CASE("JointSuitRuleExtractor_SuitBoundsAndFit", "[cross_hand_summarizer]") {
  // Spades fit: Hand 1 has 5 spades, Hand 2 has 4 spades -> combined 9 spades in all deals
  std::vector<PairedDeal> deals = {
      PairedDeal("S AKQJT", "S 9876"),
      PairedDeal("S A5432", "S K654")
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
}

TEST_CASE("HonorPossessionClassifier_MultipleHonors", "[cross_hand_summarizer]") {
  std::vector<PairedDeal> deals = {
      PairedDeal("S A H K D Q C J", "S T H A D K C Q"),
      PairedDeal("S A H K D Q C T", "S J H A D K C 2")
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

TEST_CASE("EmptyDealSet", "[cross_hand_summarizer]") {
  std::vector<PairedDeal> deals;

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  REQUIRE(report.total_deals == 0);
  REQUIRE(report.invariant_rules.empty());
}
