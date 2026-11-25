#include <algorithm>
#include <iostream>
#include <gtest/gtest.h>
#include <stdexcept>

#include "conditional_line.h"
#include "cross_hand_summarizer.h"
#include "unknown_cards_invariant.h"

TEST(CrossHandSummarizerTest, HonorPossessionClassifier_OrInvariant) {
  std::vector<PairedDeal> deals = {
    PairedDeal("S K64; H 65;", "S 75; H K74;"),
    PairedDeal("S K754; H K;", "S 6; H 7654;"),
    PairedDeal("S 75; H K76;", "S K64; H 54;")
  };
  HonorPossessionClassifier classifier("hand 1", "hand 3");
  for (const auto& deal : deals) {
    classifier.ProcessDeal(deal);
  }
  auto invs = classifier.ExtractInvariants();
  bool found_or = false;
  for (const auto& inv : invs) {
    std::cout << "Extracted inv: " << inv->ToString() << "\n";
    if (inv->ToString() == "OrInvariant(E has the HK, E has the SK)") {
      found_or = true;
    }
  }
  ASSERT_TRUE(found_or);
}

TEST(CrossHandSummarizerTest, HonorPossessionClassifier_AndInvariant) {
  std::vector<PairedDeal> deals = {
    PairedDeal("S K64; H K5;", "S 75; H 74;"),
    PairedDeal("S K754; H K;", "S 6; H 7654;")
  };
  HonorPossessionClassifier classifier("hand 1", "hand 3");
  for (const auto& deal : deals) {
    classifier.ProcessDeal(deal);
  }
  auto invs = classifier.ExtractInvariants();
  bool found_and = false;
  for (const auto& inv : invs) {
    if (inv->ToString() == "AndInvariant(E has the HK, E has the SK)") {
      found_and = true;
    }
  }
  ASSERT_TRUE(found_and);
}

TEST(CrossHandSummarizerTest, AcceptanceCriterion_ExampleHandPairs) {
  // All paired deals contain S K432, bifurcated differently
  std::vector<PairedDeal> deals = {
      PairedDeal("S K2", "S 43"),
      PairedDeal("S K3", "S 42"),
      PairedDeal("S K4", "S 32")
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  ASSERT_TRUE(report.total_deals == 3);
  ASSERT_TRUE(report.cards_per_hand == 2);
  ASSERT_TRUE(report.unknown_cards == ParseHand("S K432"));
  ASSERT_TRUE(report.HasRule("hand 1 always contains the SK"));
  // Single-suit tautology suppression ensures suit length rules are omitted when all unknown cards are in Spades
  ASSERT_FALSE(report.HasRule("combined length in S is always 4"));
  ASSERT_FALSE(report.HasRule("hand 1 length in S is always 2"));
  ASSERT_TRUE(report.ToString().find("Invariant Rules for 2 card ending:") != std::string::npos);
}

TEST(CrossHandSummarizerTest, SpotCardFiltering_IgnoresSpots) {
  // Hand 1 always has Hearts Ace (H A)
  // Overall set of cards is H A8765432 across all deals
  std::vector<PairedDeal> deals = {
      PairedDeal("H A234", "H 8765"),
      PairedDeal("H A567", "H 8432"),
      PairedDeal("H A8", "H 765432")
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  ASSERT_TRUE(report.total_deals == 3);
  ASSERT_TRUE(report.HasRule("hand 1 always contains the HA"));
}

TEST(CrossHandSummarizerTest, JointSuitRuleExtractor_SuitBoundsAndTautologySuppression) {
  // Spades and Hearts unknown cards
  // Deal 1: Hand 1 = S AK (2), H 2 (1); Hand 2 = S Q (1), H 34 (2)
  // Deal 2: Hand 1 = S AK (2), H 3 (1); Hand 2 = S Q (1), H 24 (2)
  std::vector<PairedDeal> deals = {
      PairedDeal("S AK H 2", "S Q H 34"),
      PairedDeal("S AK H 3", "S Q H 24")
  };

  JointSuitRuleExtractor extractor("hand 1", "hand 3");
  for (const auto& d : deals) {
    extractor.ProcessDeal(d);
  }
  auto raw_rules = extractor.ExtractRules();
  auto has_raw_rule = [&](const std::string& r) {
    return std::find(raw_rules.begin(), raw_rules.end(), r) != raw_rules.end();
  };
  ASSERT_TRUE(has_raw_rule("hand 1 length in S is always 2"));
  ASSERT_TRUE(has_raw_rule("hand 3 length in S is always 1"));
  ASSERT_TRUE(has_raw_rule("hand 1 length in H is always 1"));
  ASSERT_TRUE(has_raw_rule("hand 3 length in H is always 2"));

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals, "hand 1", "hand 3");

  ASSERT_TRUE(report.total_deals == 2);
  ASSERT_TRUE(report.suit_bounds[SPADES].min_hand1 == 2);
  ASSERT_TRUE(report.suit_bounds[SPADES].max_hand1 == 2);
  ASSERT_TRUE(report.suit_bounds[SPADES].min_hand2 == 1);
  ASSERT_TRUE(report.suit_bounds[SPADES].max_hand2 == 1);

  // Across 3v3 deals of S AKQ H 234, all 4 exact suit-length rules are
  // equivalent and deduplicate to a single canonical rule.
  ASSERT_TRUE(report.HasRule("hand 1 length in H is always 1"));
  ASSERT_FALSE(report.HasRule("hand 1 length in S is always 2"));
  ASSERT_FALSE(report.HasRule("hand 3 length in S is always 1"));
  ASSERT_FALSE(report.HasRule("hand 3 length in H is always 2"));

  // Verify tautologies are suppressed
  ASSERT_FALSE(report.HasRule("combined length in S is always 3"));
  ASSERT_FALSE(report.HasRule("hand 1 length in D is always 0"));
  ASSERT_FALSE(report.HasRule("hand 3 length in C is always 0"));
}

TEST(CrossHandSummarizerTest, HonorPossessionClassifier_MultipleHonors) {
  // Overall set of cards: S AT, H AK, D KQ, C QJT
  std::vector<PairedDeal> deals = {
      PairedDeal("S A H K D Q C J", "S T H A D K C QT"),
      PairedDeal("S A H K D Q C T", "S T H A D K C QJ")
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals, "hand 1", "hand 3");

  ASSERT_TRUE(report.total_deals == 2);
  ASSERT_TRUE(report.HasRule("hand 1 always contains the SA"));
  ASSERT_TRUE(report.HasRule("hand 1 always contains the HK"));
  ASSERT_TRUE(report.HasRule("hand 1 always contains the DQ"));
  ASSERT_TRUE(report.HasRule("hand 3 always contains the HA"));
  ASSERT_TRUE(report.HasRule("hand 3 always contains the DK"));
}

TEST(CrossHandSummarizerTest, MaximumSuitLength_AcrossBifurcations) {
  // Overall cards: S 432, H 432 (6 cards total, 3 per hand).
  // Max length 2 in S (i.e. not 3-0) holds on 18/20 = 90% < 95% of deals.
  std::vector<PairedDeal> deals = {
      PairedDeal("S 32 H 2", "S 4 H 43"),
      PairedDeal("S 42 H 3", "S 3 H 42"),
      PairedDeal("S 2 H 32", "S 43 H 4")
  };

  JointSuitRuleExtractor extractor;
  for (const auto& d : deals) {
    extractor.ProcessDeal(d);
  }
  auto raw_rules = extractor.ExtractRules();
  ASSERT_TRUE(std::find(raw_rules.begin(), raw_rules.end(),
                    "maximum length in S in any hand is 2") != raw_rules.end());
  ASSERT_TRUE(std::find(raw_rules.begin(), raw_rules.end(),
                    "maximum length in H in any hand is 2") != raw_rules.end());

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  ASSERT_TRUE(report.total_deals == 3);
  // Deduplication collapses equivalent 2-1/1-2 rules to ShortestLength(H) >= 1.
  ASSERT_TRUE(report.HasRule("ShortestLength(H) >= 1"));
  ASSERT_FALSE(report.HasRule("maximum length in S in any hand is 2"));
  ASSERT_FALSE(report.HasRule("maximum length in H in any hand is 2"));

  // Overall cards: S 5432, H 5432 (8 cards total, 4 per hand).
  // Max length 3 in S (i.e. not 4-0) holds on 68/70 = 97.1% >= 95% of deals,
  // so it is discarded as boring.
  std::vector<PairedDeal> boring_deals = {
      PairedDeal("S 32 H 32", "S 54 H 54"),
      PairedDeal("S 42 H 42", "S 53 H 53"),
      PairedDeal("S 432 H 2", "S 5 H 543")
  };
  CrossHandSummaryReport boring_report = summarizer.Summarize(boring_deals);
  ASSERT_FALSE(boring_report.HasRule("maximum length in S in any hand is 3"));
  ASSERT_FALSE(boring_report.HasRule("maximum length in H in any hand is 3"));
  ASSERT_FALSE(boring_report.HasRule("ShortestLength(S) >= 1"));
  ASSERT_FALSE(boring_report.HasRule("ShortestLength(H) >= 1"));
}

TEST(CrossHandSummarizerTest, Validation_InvalidDealSetThrows) {
  CrossHandDealSetSummarizer summarizer;

  // Overlapping cards in hand 1 and hand 2 in a single deal
  std::vector<PairedDeal> overlapping_deals = {
      PairedDeal("S K2", "S K4")
  };
  EXPECT_THROW(summarizer.Summarize(overlapping_deals), std::invalid_argument);

  // Mismatched overall card set across deals
  std::vector<PairedDeal> mismatched_deals = {
      PairedDeal("S K2", "S 34"),
      PairedDeal("S A2", "S 34")
  };
  EXPECT_THROW(summarizer.Summarize(mismatched_deals), std::invalid_argument);
}

TEST(CrossHandSummarizerTest, EmptyDealSet) {
  std::vector<PairedDeal> deals;

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  ASSERT_TRUE(report.total_deals == 0);
  ASSERT_TRUE(report.invariant_rules.empty());
}

TEST(CrossHandSummarizerTest, CrossSuitConditionalRuleExtractor_ExtractsHandWithXAlsoHasYRules) {
  // Deal 1: Hand 1 = S AK4 H Q43 (3 S, 3 H); Hand 2 = S J3 H JT2 (2 S, 3 H)
  // Deal 2: Hand 1 = S A4 H Q4 (2 S, 2 H); Hand 2 = S KJ3 H JT32 (3 S, 4 H)
  std::vector<PairedDeal> deals = {
      PairedDeal("S AK4 H Q43", "S J3 H JT2"),
      PairedDeal("S A4 H Q4", "S KJ3 H JT32")
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  ASSERT_TRUE(report.total_deals == 2);
  ASSERT_TRUE(report.HasRule("the hand with the SK also has exactly 3 S"));
}

TEST(CrossHandSummarizerTest, CrossSuitConditionalRuleExtractor_CoPossessionOfCards) {
  // S A and S T are always in the same hand across all deals
  std::vector<PairedDeal> deals = {
      PairedDeal("S AT H 5", "S K43 H 2"),
      PairedDeal("S K43 H 2", "S AT H 5")
  };

  CrossSuitConditionalRuleExtractor extractor;
  for (const auto& d : deals) {
    extractor.ProcessDeal(d);
  }
  auto raw_rules = extractor.ExtractRules();
  ASSERT_TRUE(std::find(raw_rules.begin(), raw_rules.end(),
                    "the hand with the SA also has the ST") != raw_rules.end());
  ASSERT_TRUE(std::find(raw_rules.begin(), raw_rules.end(),
                    "the hand with the ST also has the SA") != raw_rules.end());

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  ASSERT_TRUE(report.total_deals == 2);
  // Symmetric co-possession rules are equivalent and deduplicated to one rule.
  ASSERT_TRUE(report.HasRule("the hand with the SA also has the ST"));
  ASSERT_FALSE(report.HasRule("the hand with the ST also has the SA"));
}

TEST(CrossHandSummarizerTest, RandomBifurcation_TautologyElimination) {
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

  ASSERT_TRUE(report.total_deals == 2);
  // Rule "the hand with the HT also has exactly 1 H" is tautological for ALL random bifurcations and must be omitted
  ASSERT_FALSE(report.HasRule("the hand with the HT also has exactly 1 H"));
  ASSERT_FALSE(report.HasRule("the hand with the HT also has at most 3 D"));
}

TEST(CrossHandSummarizerTest, EvaluateRuleOnDeal_DirectTests) {
  PairedDeal deal("S AK H Q D 4", "S QJ H 2 D 32");

  ASSERT_TRUE(EvaluateRuleOnDeal("the hand with the SA also has the SK", deal));
  ASSERT_TRUE(EvaluateRuleOnDeal("the hand with the SA also has exactly 1 H", deal));
  ASSERT_TRUE(EvaluateRuleOnDeal("the hand with the SA also has at least 2 S", deal));
  ASSERT_TRUE(EvaluateRuleOnDeal("the hand with the SA also has at most 1 D", deal));

  ASSERT_TRUE(EvaluateRuleOnDeal("hand 1 always contains the SA", deal));
  ASSERT_TRUE(EvaluateRuleOnDeal("hand 2 always contains the SQ", deal));
  ASSERT_FALSE(EvaluateRuleOnDeal("both hands always contain the SA", deal));

  ASSERT_TRUE(EvaluateRuleOnDeal("hand 1 length in S is always 2", deal));
  ASSERT_TRUE(EvaluateRuleOnDeal("hand 1 length in S is between 1 and 3", deal));
  ASSERT_TRUE(EvaluateRuleOnDeal("maximum length in S in any hand is 2", deal));
}

TEST(CrossHandSummarizerTest, CrossSuitConditionalRuleExtractor_SuppressesTautologiesAndFixedCards) {
  // S A is statically fixed in Hand 1 across all deals -> no "the hand with the SA" rule
  // Card CJ is not in unknown cards -> no "the hand with the CJ" rule
  std::vector<PairedDeal> deals = {
      PairedDeal("S A4 H 54", "S T3 H 32"),
      PairedDeal("S A3 H 53", "S T4 H 42")
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  ASSERT_TRUE(report.total_deals == 2);
  ASSERT_TRUE(report.HasRule("hand 1 always contains the SA"));
  ASSERT_FALSE(report.HasRule("the hand with the SA"));
  ASSERT_FALSE(report.HasRule("the hand with the CJ"));
}

TEST(CrossHandSummarizerTest, UnknownCardsInvariantClassHierarchy_FromStringToStringAndHoldsOnDeal) {
  PairedDeal deal("S AK H Q D 4", "S QJ H 2 D 32");

  // 1. CrossSuitConditionalInvariant
  {
    std::string rule_str = "the hand with the SA also has the SK";
    auto inv = UnknownCardsInvariant::FromString(rule_str);
    ASSERT_TRUE(inv != nullptr);
    ASSERT_TRUE(inv->ToString() == rule_str);
    ASSERT_TRUE(inv->HoldsOnDeal(deal));

    auto derived = dynamic_cast<CrossSuitCardConditionalInvariant*>(inv.get());
    ASSERT_TRUE(derived != nullptr);
    ASSERT_TRUE(derived->condition_card() == Card::ParseStringCard("SA"));
    ASSERT_TRUE(derived->target_card() == Card::ParseStringCard("SK"));
  }

  {
    std::string rule_str = "the hand with the SA also has exactly 1 H";
    auto inv = UnknownCardsInvariant::FromString(rule_str);
    ASSERT_TRUE(inv != nullptr);
    ASSERT_TRUE(inv->ToString() == rule_str);
    ASSERT_TRUE(inv->HoldsOnDeal(deal));

    auto derived = dynamic_cast<CrossSuitExactSuitLengthInvariant*>(inv.get());
    ASSERT_TRUE(derived != nullptr);
    ASSERT_TRUE(derived->condition_card() == Card::ParseStringCard("SA"));
    ASSERT_TRUE(derived->target_suit() == Suit::HEARTS);
    ASSERT_TRUE(derived->length() == 1);
  }

  {
    std::string rule_str = "the hand with the SA also has at least 2 S";
    auto inv = UnknownCardsInvariant::FromString(rule_str);
    ASSERT_TRUE(inv != nullptr);
    ASSERT_TRUE(inv->ToString() == rule_str);
    ASSERT_TRUE(inv->HoldsOnDeal(deal));

    auto derived = dynamic_cast<CrossSuitMinSuitLengthInvariant*>(inv.get());
    ASSERT_TRUE(derived != nullptr);
    ASSERT_TRUE(derived->condition_card() == Card::ParseStringCard("SA"));
    ASSERT_TRUE(derived->target_suit() == Suit::SPADES);
    ASSERT_TRUE(derived->min_len() == 2);
  }

  {
    std::string rule_str = "the hand with the SA also has at most 1 D";
    auto inv = UnknownCardsInvariant::FromString(rule_str);
    ASSERT_TRUE(inv != nullptr);
    ASSERT_TRUE(inv->ToString() == rule_str);
    ASSERT_TRUE(inv->HoldsOnDeal(deal));

    auto derived = dynamic_cast<CrossSuitMaxSuitLengthInvariant*>(inv.get());
    ASSERT_TRUE(derived != nullptr);
    ASSERT_TRUE(derived->condition_card() == Card::ParseStringCard("SA"));
    ASSERT_TRUE(derived->target_suit() == Suit::DIAMONDS);
    ASSERT_TRUE(derived->max_len() == 1);
  }

  // 2. HonorPossessionInvariant
  {
    std::string rule_str = "hand 1 always contains the SA";
    auto inv = UnknownCardsInvariant::FromString(rule_str);
    ASSERT_TRUE(inv != nullptr);
    ASSERT_TRUE(inv->ToString() == rule_str);
    ASSERT_TRUE(inv->HoldsOnDeal(deal));

    auto derived = HonorPossessionInvariant::FromString(rule_str);
    ASSERT_TRUE(derived != nullptr);
    ASSERT_TRUE(derived->target_hand() == HonorPossessionTargetHand::kHand1);
    ASSERT_TRUE(derived->card() == Card::ParseStringCard("SA"));
  }

  {
    std::string rule_str = "both hands always contain the SA";
    auto inv = UnknownCardsInvariant::FromString(rule_str);
    ASSERT_TRUE(inv != nullptr);
    ASSERT_TRUE(inv->ToString() == rule_str);
    ASSERT_FALSE(inv->HoldsOnDeal(deal));
  }

  // 2b. OrInvariant
  {
    std::string rule_str = "OrInvariant(E has the HK, E has the SK)";
    auto inv = UnknownCardsInvariant::FromString(rule_str);
    ASSERT_TRUE(inv != nullptr);
    ASSERT_TRUE(inv->ToString() == rule_str);

    auto or_inv = std::dynamic_pointer_cast<OrInvariant>(inv);
    ASSERT_TRUE(or_inv != nullptr);
    ASSERT_TRUE(or_inv->left() != nullptr);
    ASSERT_TRUE(or_inv->right() != nullptr);
    ASSERT_TRUE(or_inv->left()->ToString() == "E has the HK");
    ASSERT_TRUE(or_inv->right()->ToString() == "E has the SK");

    PairedDeal deal_with_hk("S 32; H K2; D 43; C 2;", "S AQ; H A3; D QJ; C 3;");
    PairedDeal deal_with_sk("S K2; H 32; D 43; C 2;", "S AQ; H A3; D QJ; C 3;");
    PairedDeal deal_with_neither("S 32; H 42; D 43; C 2;", "S AK; H AK; D QJ; C 3;");

    ASSERT_TRUE(inv->HoldsOnDeal(deal_with_hk));
    ASSERT_TRUE(inv->HoldsOnDeal(deal_with_sk));
    ASSERT_FALSE(inv->HoldsOnDeal(deal_with_neither));
  }
  {
    std::string rule_str = "maximum length in S in any hand is 2";
    auto inv = UnknownCardsInvariant::FromString(rule_str);
    ASSERT_TRUE(inv != nullptr);
    ASSERT_TRUE(inv->ToString() == rule_str);
    ASSERT_TRUE(inv->HoldsOnDeal(deal));

    auto derived = MaxSuitLengthInvariant::FromString(rule_str);
    ASSERT_TRUE(derived != nullptr);
    ASSERT_TRUE(derived->suit() == Suit::SPADES);
    ASSERT_TRUE(derived->max_len() == 2);
  }

  // 4. ExactSuitLengthInvariant
  {
    std::string rule_str = "hand 1 length in S is always 2";
    auto inv = UnknownCardsInvariant::FromString(rule_str);
    ASSERT_TRUE(inv != nullptr);
    ASSERT_TRUE(inv->ToString() == rule_str);
    ASSERT_TRUE(inv->HoldsOnDeal(deal));

    auto derived = ExactSuitLengthInvariant::FromString(rule_str);
    ASSERT_TRUE(derived != nullptr);
    ASSERT_TRUE(derived->hand_choice() == InvariantHandChoice::kHand1);
    ASSERT_TRUE(derived->suit() == Suit::SPADES);
    ASSERT_TRUE(derived->exact_len() == 2);
  }

  // 5. SuitLengthRangeInvariant
  {
    std::string rule_str = "hand 1 length in S is between 1 and 3";
    auto inv = UnknownCardsInvariant::FromString(rule_str);
    ASSERT_TRUE(inv != nullptr);
    ASSERT_TRUE(inv->ToString() == rule_str);
    ASSERT_TRUE(inv->HoldsOnDeal(deal));

    auto derived = SuitLengthRangeInvariant::FromString(rule_str);
    ASSERT_TRUE(derived != nullptr);
    ASSERT_TRUE(derived->hand_choice() == InvariantHandChoice::kHand1);
    ASSERT_TRUE(derived->suit() == Suit::SPADES);
    ASSERT_TRUE(derived->min_len() == 1);
    ASSERT_TRUE(derived->max_len() == 3);
  }

  // 6. Invalid String Parsing Returns nullptr
  {
    ASSERT_TRUE(UnknownCardsInvariant::FromString("this is an invalid rule") == nullptr);
    ASSERT_TRUE(UnknownCardsInvariant::FromString("the hand with the SA also has bad format") == nullptr);
    ASSERT_TRUE(UnknownCardsInvariant::FromString("hand 1 length in S is between invalid") == nullptr);
    ASSERT_TRUE(CrossSuitConditionalInvariant::FromString("hand 1 always contains the SA") == nullptr);
    ASSERT_TRUE(HonorPossessionInvariant::FromString("maximum length in S in any hand is 2") == nullptr);
    ASSERT_TRUE(MaxSuitLengthInvariant::FromString("hand 1 always contains the SA") == nullptr);
    ASSERT_TRUE(ExactSuitLengthInvariant::FromString("hand 1 length in S is between 1 and 3") == nullptr);
    ASSERT_TRUE(SuitLengthRangeInvariant::FromString("hand 1 length in S is always 2") == nullptr);
  }
}

TEST(CrossHandSummarizerTest, FilterUninterestingInvariants_LineFiltering) {
  // 2-card ending in Spades:
  // North (0): S32, South (2): SAQ. Unknown cards: SKT54.
  // Line finesses SQ on trick 1 and cashes SA on trick 2 (requires 2 tricks).
  // Deals passed to Summarize have East (hand 1) holding SK and ST,
  // while West (hand 3) holds S54.
  // - "hand 1 always contains the SK" is required (if West has SK, line only
  //   takes 1 trick).
  // - "hand 1 always contains the ST" is incidental (if West has ST and East
  //   has SK, the line still takes 2 tricks).
  std::vector<uint64_t> sample_hands = {
      ParseHand("S 32"),
      ParseHand("S KT"),
      ParseHand("S AQ"),
      ParseHand("S 54"),
  };
  BoardState sample_bs;
  sample_bs.SetHands(sample_hands);
  sample_bs.SetLead(0);
  sample_bs.SetTrump(SPADES);

  std::set<int> deal_ids = {0};
  GeneralizedTrick trick2_2wins(
      GeneralizedDemiStruct("__", "S3", deal_ids),
      GeneralizedDemiStruct("__", "SA", deal_ids), deal_ids, 2);
  ConditionalLine leaf_2wins;
  leaf_2wins.SetAction(trick2_2wins);

  GeneralizedTrick trick2_3wins(
      GeneralizedDemiStruct("__", "SA", deal_ids),
      GeneralizedDemiStruct("__", "S3", deal_ids), deal_ids, 3);
  ConditionalLine leaf_3wins;
  leaf_3wins.SetAction(trick2_3wins);

  GeneralizedTrick trick1(
      GeneralizedDemiStruct("__", "S2", deal_ids),
      GeneralizedDemiStruct("__", "SQ", deal_ids), deal_ids, 0);
  ConditionalLine line;
  line.SetAction(trick1);
  line.AddCondition(new WinnerPlayCondition(2), leaf_2wins);
  line.AddCondition(new WinnerPlayCondition(3), leaf_3wins);
  line.set_min_tricks(2);
  line.set_sample_board(sample_bs);

  std::vector<PairedDeal> deals = {
      PairedDeal("S KT", "S 54"),
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report_no_line =
      summarizer.Summarize(deals, "hand 1", "hand 3", nullptr);
  ASSERT_TRUE(report_no_line.HasRule("hand 1 always contains the SK"));
  ASSERT_TRUE(report_no_line.HasRule("hand 1 always contains the ST"));

  CrossHandSummaryReport report_with_line =
      summarizer.Summarize(deals, "hand 1", "hand 3", &line);
  ASSERT_TRUE(report_with_line.invariant_stats.size() == 2);
  // "hand 1 always contains the SK" holds ~50% and succeeds 100% when it holds,
  // so it sorts first ahead of AndInvariant (which holds ~17%), while ST (~35%
  // success when holds <= 50%) is dropped.
  ASSERT_TRUE(report_with_line.invariant_stats[0].rule ==
          "hand 1 always contains the SK");
  ASSERT_TRUE(report_with_line.invariant_stats[0].success_when_holds_percent() ==
          100);
  ASSERT_TRUE(report_with_line.invariant_stats[1].rule ==
          "AndInvariant(E has the SK, E has the ST)");
  ASSERT_TRUE(report_with_line.invariant_stats[1].success_when_holds_percent() ==
          100);
  ASSERT_TRUE(report_with_line.invariant_stats[0].holds_count >
          report_with_line.invariant_stats[1].holds_count);
  ASSERT_FALSE(report_with_line.HasRule("hand 1 always contains the ST"));
  ASSERT_TRUE(report_with_line.invariant_stats[0].holds_percent() >= 40);
  ASSERT_TRUE(report_with_line.invariant_stats[0].holds_percent() <= 60);
  ASSERT_TRUE(report_with_line.HasRule(
      report_with_line.invariant_stats[0].FormatRuleWithStats()));
}

TEST(CrossHandSummarizerTest, UnknownCardsInvariant_Invert) {
  // Unknown cards: 6 Spades (SAKQJT9), 6 Hearts (HAKQJT9), 8 Diamonds
  // (DAKQJT987). Total 20 cards, hand 1 size = 10, hand 2 size = 10.
  uint64_t unknown_cards =
      ParseHand("S AKQJT9 H AKQJT9 D AKQJT987");
  int hand1_size = 10;

  std::vector<PairedDeal> sample_deals = {
      PairedDeal("S AKQJ H AK D AKQJ", "S T9 H QJT9 D T987"),
      PairedDeal("S T9 H AKQJ D AKQJ", "S AKQJ H T9 D T987"),
      PairedDeal("S AKQ H AKQ D AKQJ", "S JT9 H JT9 D T987"),
      PairedDeal("S AKQJT9 H A D AKQ", "S - H KQJT9 D JT987"),
  };

  auto verify_invert_semantics = [&](const UnknownCardsInvariant& inv) {
    auto inv_ptr = inv.Invert(unknown_cards, hand1_size, "W", "E");
    ASSERT_TRUE(inv_ptr != nullptr);
    for (const auto& d : sample_deals) {
      ASSERT_TRUE(inv_ptr->HoldsOnDeal(d) == !inv.HoldsOnDeal(d));
    }
  };

  // 1. HonorPossessionInvariant: (HA in W) <-> (HA in E)
  {
    HonorPossessionInvariant ha_w(HonorPossessionTargetHand::kHand1,
                                  Card::ParseStringCard("HA"), "W");
    auto inv = ha_w.Invert(unknown_cards, hand1_size, "W", "E");
    ASSERT_TRUE(inv != nullptr);
    ASSERT_TRUE(inv->ToString() == "E has the HA");
    verify_invert_semantics(ha_w);
  }

  // 2. SuitLengthRangeInvariant: 4+ spades in W (4..6) -> 0..3 in W -> 3..6 E
  {
    SuitLengthRangeInvariant w_4plus_s(InvariantHandChoice::kHand1,
                                       Suit::SPADES, 4, 6, "W");
    auto inv = w_4plus_s.Invert(unknown_cards, hand1_size, "W", "E");
    ASSERT_TRUE(inv != nullptr);
    ASSERT_TRUE(inv->ToString() == "W length in S is between 0 and 3");
    verify_invert_semantics(w_4plus_s);
    // Also check equivalence to 3..6 in E on all sample deals.
    SuitLengthRangeInvariant e_3plus_s(InvariantHandChoice::kHand2,
                                       Suit::SPADES, 3, 6, "E");
    for (const auto& d : sample_deals) {
      ASSERT_TRUE(inv->HoldsOnDeal(d) == e_3plus_s.HoldsOnDeal(d));
    }
  }

  // 3. ExactSuitLengthInvariant at boundary vs interior
  {
    ExactSuitLengthInvariant w_0_s(InvariantHandChoice::kHand1, Suit::SPADES, 0,
                                   "W");
    auto inv0 = w_0_s.Invert(unknown_cards, hand1_size, "W", "E");
    ASSERT_TRUE(inv0 != nullptr);
    ASSERT_TRUE(inv0->ToString() == "W length in S is between 1 and 6");
    verify_invert_semantics(w_0_s);

    ExactSuitLengthInvariant w_3_s(InvariantHandChoice::kHand1, Suit::SPADES, 3,
                                   "W");
    auto inv3 = w_3_s.Invert(unknown_cards, hand1_size, "W", "E");
    ASSERT_TRUE(inv3 != nullptr);
    ASSERT_TRUE(inv3->ToString() ==
            "OrInvariant(W length in S is between 0 and 2, "
            "W length in S is between 4 and 6)");
    verify_invert_semantics(w_3_s);
  }

  // 4. MaxSuitLengthInvariant
  {
    MaxSuitLengthInvariant max_4_s(Suit::SPADES, 4);
    auto inv = max_4_s.Invert(unknown_cards, hand1_size, "W", "E");
    ASSERT_TRUE(inv != nullptr);
    ASSERT_TRUE(inv->ToString() ==
            "OrInvariant(E length in S is between 5 and 6, "
            "W length in S is between 5 and 6)");
    verify_invert_semantics(max_4_s);
  }

  // 5. CrossSuitMin / Max / Exact / Card
  {
    CrossSuitMinSuitLengthInvariant min_rule(Card::ParseStringCard("HA"),
                                             Suit::SPADES, 4);
    auto inv_min = min_rule.Invert(unknown_cards, hand1_size, "W", "E");
    ASSERT_TRUE(inv_min != nullptr);
    ASSERT_TRUE(inv_min->ToString() ==
            "the hand with the HA also has at most 3 S");
    verify_invert_semantics(min_rule);

    CrossSuitMaxSuitLengthInvariant max_rule(Card::ParseStringCard("HA"),
                                             Suit::SPADES, 3);
    auto inv_max = max_rule.Invert(unknown_cards, hand1_size, "W", "E");
    ASSERT_TRUE(inv_max != nullptr);
    ASSERT_TRUE(inv_max->ToString() ==
            "the hand with the HA also has at least 4 S");
    verify_invert_semantics(max_rule);

    CrossSuitExactSuitLengthInvariant exact_rule(Card::ParseStringCard("HA"),
                                                 Suit::SPADES, 3);
    auto inv_exact = exact_rule.Invert(unknown_cards, hand1_size, "W", "E");
    ASSERT_TRUE(inv_exact != nullptr);
    verify_invert_semantics(exact_rule);

    CrossSuitCardConditionalInvariant card_rule(Card::ParseStringCard("HA"),
                                                Card::ParseStringCard("SK"));
    auto inv_card = card_rule.Invert(unknown_cards, hand1_size, "W", "E");
    ASSERT_TRUE(inv_card != nullptr);
    verify_invert_semantics(card_rule);
  }

  // 6. AndInvariant <-> OrInvariant
  {
    auto left = std::make_shared<HonorPossessionInvariant>(
        HonorPossessionTargetHand::kHand1, Card::ParseStringCard("SK"), "E");
    auto right = std::make_shared<HonorPossessionInvariant>(
        HonorPossessionTargetHand::kHand1, Card::ParseStringCard("HK"), "E");
    AndInvariant and_inv(left, right);
    auto inv = and_inv.Invert(unknown_cards, hand1_size, "E", "W");
    ASSERT_TRUE(inv != nullptr);
    ASSERT_TRUE(inv->ToString() ==
            "OrInvariant(W has the HK, W has the SK)");
    for (const auto& d : sample_deals) {
      ASSERT_TRUE(inv->HoldsOnDeal(d) == !and_inv.HoldsOnDeal(d));
    }
  }
}

TEST(CrossHandSummarizerTest, Summarize_OpposingDealsFiltersRedundantSingleAtoms) {
  // Covered deals: East (hand 1) has BOTH SK and HK.
  std::vector<PairedDeal> covered_deals = {
      PairedDeal("S K32 H K32", "S Q45 H Q45"),
      PairedDeal("S K45 H K45", "S Q32 H Q32"),
  };
  // Opposing (failed) deals: East is missing at least one of SK or HK.
  std::vector<PairedDeal> opposing_deals = {
      PairedDeal("S K32 H Q32", "S Q45 H K45"),
      PairedDeal("S Q32 H K32", "S K45 H Q45"),
      PairedDeal("S Q32 H Q32", "S K45 H K45"),
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(
      covered_deals, "hand 1", "hand 3", nullptr, &opposing_deals);

  // AndInvariant(E has the HK, E has the SK) holds on 2/5 (40%) of the
  // simulated deals and succeeds on 2/2 (100%) when it holds, so it sorts
  // ahead of the individual SK and HK rules (which hold on 3/5 = 60% and
  // succeed on 2/3 = 67% when they hold).
  int and_idx = -1, sk_idx = -1, hk_idx = -1;
  for (size_t i = 0; i < report.invariant_stats.size(); ++i) {
    const auto &st = report.invariant_stats[i];
    if (st.rule == "AndInvariant(E has the HK, E has the SK)") {
      and_idx = static_cast<int>(i);
      ASSERT_TRUE(st.holds_count == 2);
      ASSERT_TRUE(st.total_bifurcations == 5);
      ASSERT_TRUE(st.success_when_holds_count == 2);
      ASSERT_TRUE(st.holds_percent() == 40);
    } else if (st.rule == "hand 1 always contains the SK") {
      sk_idx = static_cast<int>(i);
      ASSERT_TRUE(st.holds_count == 3);
      ASSERT_TRUE(st.total_bifurcations == 5);
      ASSERT_TRUE(st.success_when_holds_count == 2);
      ASSERT_TRUE(st.holds_percent() == 60);
    } else if (st.rule == "hand 1 always contains the HK") {
      hk_idx = static_cast<int>(i);
      ASSERT_TRUE(st.holds_count == 3);
      ASSERT_TRUE(st.total_bifurcations == 5);
      ASSERT_TRUE(st.success_when_holds_count == 2);
      ASSERT_TRUE(st.holds_percent() == 60);
    }
  }
  ASSERT_TRUE(and_idx >= 0);
  ASSERT_TRUE(sk_idx > and_idx);
  ASSERT_TRUE(hk_idx > and_idx);
}

TEST(CrossHandSummarizerTest, HandLabelFormatter_DirectUnitTests) {
  {  // Default / Verbatim Mode
    HandLabelFormatter formatter("hand 1", "hand 3", HandLabelMode::kVerbatim);
    ASSERT_TRUE(formatter.hand1_label() == "hand 1");
    ASSERT_TRUE(formatter.hand2_label() == "hand 3");
    ASSERT_TRUE(formatter.mode() == HandLabelMode::kVerbatim);

    ASSERT_TRUE(formatter.FormatHand1() == "hand 1");
    ASSERT_TRUE(formatter.FormatHand2() == "hand 3");
    ASSERT_TRUE(formatter.FormatCompositeHand1() == "E");
    ASSERT_TRUE(formatter.FormatCompositeHand2() == "W");
  }

  {  // Compass Mode
    HandLabelFormatter formatter("hand 1", "hand 3", HandLabelMode::kCompass);
    ASSERT_TRUE(formatter.mode() == HandLabelMode::kCompass);

    ASSERT_TRUE(formatter.FormatHand1() == "E");
    ASSERT_TRUE(formatter.FormatHand2() == "W");
    ASSERT_TRUE(formatter.FormatCompositeHand1() == "E");
    ASSERT_TRUE(formatter.FormatCompositeHand2() == "W");
  }

  {  // Standard Compass Input Mapping
    HandLabelFormatter formatter("North", "South", HandLabelMode::kCompass);
    ASSERT_TRUE(formatter.FormatHand1() == "N");
    ASSERT_TRUE(formatter.FormatHand2() == "S");

    ASSERT_TRUE(HandLabelFormatter::TranslateCompass("hand 0") == "N");
    ASSERT_TRUE(HandLabelFormatter::TranslateCompass("hand 1") == "E");
    ASSERT_TRUE(HandLabelFormatter::TranslateCompass("hand 2") == "S");
    ASSERT_TRUE(HandLabelFormatter::TranslateCompass("hand 3") == "W");
    ASSERT_TRUE(HandLabelFormatter::TranslateCompass("0") == "N");
    ASSERT_TRUE(HandLabelFormatter::TranslateCompass("1") == "E");
    ASSERT_TRUE(HandLabelFormatter::TranslateCompass("2") == "S");
    ASSERT_TRUE(HandLabelFormatter::TranslateCompass("3") == "W");
  }

  {  // Verbatim Mode Preserves Custom and Directional Labels
    HandLabelFormatter formatter("North", "South", HandLabelMode::kVerbatim);
    ASSERT_TRUE(formatter.FormatHand1() == "North");
    ASSERT_TRUE(formatter.FormatHand2() == "South");
    ASSERT_TRUE(formatter.FormatCompositeHand1() == "North");
    ASSERT_TRUE(formatter.FormatCompositeHand2() == "South");
  }

  {  // Custom Unrecognized Labels Default to Verbatim Passthrough
    HandLabelFormatter formatter("Declarer", "Dummy", HandLabelMode::kCompass);
    ASSERT_TRUE(formatter.FormatHand1() == "Declarer");
    ASSERT_TRUE(formatter.FormatHand2() == "Dummy");
    ASSERT_TRUE(formatter.FormatCompositeHand1() == "Declarer");
    ASSERT_TRUE(formatter.FormatCompositeHand2() == "Dummy");
  }

  {  // Empty String Handling
    HandLabelFormatter formatter("", "", HandLabelMode::kCompass);
    ASSERT_TRUE(formatter.FormatHand1() == "");
    ASSERT_TRUE(formatter.FormatHand2() == "");
    ASSERT_TRUE(formatter.FormatCompositeHand1() == "");
    ASSERT_TRUE(formatter.FormatCompositeHand2() == "");
    ASSERT_TRUE(formatter.FormatLabel("") == "");
    ASSERT_TRUE(HandLabelFormatter::TranslateCompass("") == "");
  }
}

TEST(CrossHandSummarizerTest, CrossHandDealSetSummarizer_CompassVsVerbatimMode) {
  std::vector<PairedDeal> deals = {
      PairedDeal("S K2", "S 43"),
      PairedDeal("S K3", "S 42"),
      PairedDeal("S K4", "S 32"),
  };

  CrossHandDealSetSummarizer summarizer;

  {  // Verbatim Mode Summary
    HandLabelFormatter verbatim_fmt(
        "hand 1", "hand 3", HandLabelMode::kVerbatim);
    CrossHandSummaryReport report = summarizer.Summarize(deals, verbatim_fmt);
    ASSERT_TRUE(report.HasRule("hand 1 always contains the SK"));
  }

  {  // Compass Mode Summary
    HandLabelFormatter compass_fmt("hand 1", "hand 3", HandLabelMode::kCompass);
    CrossHandSummaryReport report = summarizer.Summarize(deals, compass_fmt);
    ASSERT_TRUE(report.HasRule("E has the SK"));
  }

  {  // Compass Mode With Opposing Deals
    std::vector<PairedDeal> opposing_deals = {
        PairedDeal("S 43", "S K2"),
        PairedDeal("S 42", "S K3"),
    };
    HandLabelFormatter compass_fmt("hand 1", "hand 3", HandLabelMode::kCompass);
    CrossHandSummaryReport report =
        summarizer.Summarize(deals, compass_fmt, nullptr, &opposing_deals);
    ASSERT_TRUE(report.HasRule("E has the SK"));
  }
}

TEST(CrossHandSummarizerTest, Extractors_DirectConstructionWithFormatter) {
  PairedDeal deal("S K2; H A", "S 43; H K");
  HandLabelFormatter compass_fmt("hand 1", "hand 3", HandLabelMode::kCompass);

  JointSuitRuleExtractor suit_extractor(compass_fmt);
  suit_extractor.ProcessDeal(deal);
  auto suit_invs = suit_extractor.ExtractInvariants();
  ASSERT_FALSE(suit_invs.empty());
  ASSERT_TRUE(suit_invs[0]->ToString() == "E length in S is always 2");

  HonorPossessionClassifier honor_classifier(compass_fmt);
  honor_classifier.ProcessDeal(deal);
  auto honor_invs = honor_classifier.ExtractInvariants();
  ASSERT_FALSE(honor_invs.empty());
  ASSERT_TRUE(honor_invs[0]->ToString() == "E has the SK");

  CrossSuitConditionalRuleExtractor cond_extractor(compass_fmt);
  cond_extractor.ProcessDeal(deal);
  auto cond_invs = cond_extractor.ExtractInvariants();
  ASSERT_TRUE(cond_invs.empty());
}

TEST(CrossHandSummarizerTest, EvaluateRuleAndFromString_CompassMapping) {
  PairedDeal deal("S AK2; C 43", "S 43; C AK2");
  HandLabelFormatter compass_fmt("hand 1", "hand 3", HandLabelMode::kCompass);

  {  // Exact suit length invariant parsing with raw labels
    auto inv = UnknownCardsInvariant::FromString(
        "E length in S is always 3", "hand 1", "hand 3");
    ASSERT_TRUE(inv != nullptr);
    ASSERT_TRUE(inv->HoldsOnDeal(deal));
  }

  {  // Suit length range invariant parsing with raw labels
    auto inv = UnknownCardsInvariant::FromString(
        "E length in S is between 2 and 4", "hand 1", "hand 3");
    ASSERT_TRUE(inv != nullptr);
    ASSERT_TRUE(inv->HoldsOnDeal(deal));
  }

  {  // EvaluateRuleOnDeal with HandLabelFormatter
    ASSERT_TRUE(EvaluateRuleOnDeal("E has the SA", deal, compass_fmt));
    ASSERT_TRUE(EvaluateRuleOnDeal("E length in S is always 3", deal, compass_fmt));
    ASSERT_FALSE(
        EvaluateRuleOnDeal("W length in S is always 3", deal, compass_fmt));
  }
}

TEST(CrossHandSummarizerTest, InvariantStats_EvaluationSortingAndInlineFormat) {
  // Unknown cards: S K, S 4, S 3, S 2, H 4, H 3, H 2, D 4, D 3, D 2 (10 cards,
  // 5 per hand).
  // Covered deals: Hand 1 always has SK, and Hand 1 has at least 1 Heart.
  std::vector<PairedDeal> deals = {
      PairedDeal("S K4 H 43 D 4", "S 32 H 2 D 32"),
      PairedDeal("S K3 H 42 D 3", "S 42 H 3 D 42"),
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  ASSERT_FALSE(report.invariant_stats.empty());
  ASSERT_TRUE(report.invariant_stats.size() == report.invariant_rules.size());

  // Verify every reported invariant holds on < 95% of random bifurcations,
  // has inline "(holds: X%)" formatting, and round-trips through FromString.
  for (size_t i = 0; i < report.invariant_stats.size(); ++i) {
    const auto &st = report.invariant_stats[i];
    ASSERT_TRUE(st.total_bifurcations == 200);
    ASSERT_TRUE(st.holds_percent() < 95);
    ASSERT_TRUE(report.invariant_rules[i] == st.FormatRuleWithStats());
    auto parsed = UnknownCardsInvariant::FromString(report.invariant_rules[i]);
    ASSERT_TRUE(parsed != nullptr);
    ASSERT_TRUE(parsed->ToString() == st.rule);
  }

  // Test failure mode summary over simulated deals (e.g. Simple Finesse where
  // 3/10 boards succeed when West has SK and 7/10 fail when East has SK).
  std::vector<PairedDeal> failure_deals = {
      PairedDeal("S K4", "S 32"),
      PairedDeal("S K3", "S 42"),
      PairedDeal("S K2", "S 43"),
  };
  std::vector<PairedDeal> covered_deals = {
      PairedDeal("S 32", "S K4"),
      PairedDeal("S 42", "S K3"),
  };
  CrossHandSummaryReport failure_report = summarizer.Summarize(
      failure_deals, "hand 1", "hand 3", nullptr, &covered_deals,
      /*is_failure_mode=*/true);
  ASSERT_TRUE(failure_report.invariant_stats.size() == 1);
  ASSERT_TRUE(failure_report.invariant_stats[0].is_failure_mode);
  ASSERT_TRUE(failure_report.invariant_stats[0].holds_percent() == 60);
  ASSERT_TRUE(failure_report.invariant_stats[0].success_when_holds_percent() ==
          100);
  ASSERT_TRUE(failure_report.invariant_rules[0] ==
          "hand 1 always contains the SK (holds: 60%, failures when holds: "
          "100%)");
  auto parsed_fail =
      UnknownCardsInvariant::FromString(failure_report.invariant_rules[0]);
  ASSERT_TRUE(parsed_fail != nullptr);
  ASSERT_TRUE(parsed_fail->ToString() == "hand 1 always contains the SK");
}

TEST(CrossHandSummarizerTest, HonorPossessionClassifier_NearUniversalTolerance) {
  // 20 deals where 19/20 (95%) satisfy:
  // - single honor: E (hand 1) has DA
  // - OrInvariant(E has the CQ, E has the SA)
  // and 1 outlier deal (#20) violates both.
  std::vector<PairedDeal> deals;
  for (int i = 0; i < 10; ++i) {
    deals.emplace_back("S A2 D A C 32", "S 43 D 2 C Q4");
  }
  for (int i = 0; i < 9; ++i) {
    deals.emplace_back("S 42 D A C Q2", "S A3 D 2 C 43");
  }
  // 1 outlier deal out of 20 (95% holds rate on the 20-deal set):
  deals.emplace_back("S 42 D 2 C 43", "S A3 D A C Q2");

  HonorPossessionClassifier classifier("hand 1", "hand 3");
  for (const auto& d : deals) {
    classifier.ProcessDeal(d);
  }
  auto invs = classifier.ExtractInvariants();
  bool found_da = false;
  bool found_or = false;
  for (const auto& inv : invs) {
    if (inv->ToString() == "hand 1 always contains the DA") {
      found_da = true;
    } else if (inv->ToString() == "OrInvariant(E has the CQ, E has the SA)") {
      found_or = true;
    }
  }
  ASSERT_TRUE(found_da);
  ASSERT_TRUE(found_or);
}

TEST(CrossHandSummarizerTest, Summarize_IncludesInvertedOpposingInvariants) {
  // 21 covered deals:
  // - 10 deals where E has SA and W has CQ
  // - 9 deals where E has CQ and W has SA
  // - 2 outlier deals where W has BOTH SA and CQ (so Or(E has CQ, E has SA)
  //   holds on 19/21 = 90.5% < 95% of covered_deals, meaning it is not
  //   extracted directly from covered_deals alone).
  // 7 opposing (failure) deals:
  // - All 7/7 (100%) have W holding BOTH SA and CQ:
  //   AndInvariant(W has the CQ, W has the SA).
  // Inverting the opposing AndInvariant yields OrInvariant(E has the CQ, E has
  // the SA), which holds on 19/28 (68%) of all deals and succeeds on 19/19
  // (100%) when it holds.
  std::vector<PairedDeal> covered_deals;
  for (int i = 0; i < 10; ++i) {
    covered_deals.emplace_back("S A2 C 32", "S 43 C Q4");
  }
  for (int i = 0; i < 9; ++i) {
    covered_deals.emplace_back("S 42 C Q2", "S A3 C 43");
  }
  for (int i = 0; i < 2; ++i) {
    covered_deals.emplace_back("S 42 C 43", "S A3 C Q2");
  }

  std::vector<PairedDeal> failure_deals;
  for (int i = 0; i < 7; ++i) {
    failure_deals.emplace_back("S 43 C 43", "S A2 C Q2");
  }

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport success_report = summarizer.Summarize(
      covered_deals, "hand 1", "hand 3", nullptr, &failure_deals,
      /*is_failure_mode=*/false);

  ASSERT_FALSE(success_report.invariant_stats.empty());
  ASSERT_TRUE(success_report.invariant_stats[0].rule ==
          "OrInvariant(E has the CQ, E has the SA)");
  ASSERT_TRUE(success_report.invariant_stats[0].holds_count == 19);
  ASSERT_TRUE(success_report.invariant_stats[0].total_bifurcations == 28);
  ASSERT_TRUE(success_report.invariant_stats[0].success_when_holds_count == 19);
  ASSERT_TRUE(success_report.invariant_stats[0].holds_percent() == 68);
}

TEST(CrossHandSummarizerTest, Summarize_OpposingDealsDoesNotDoubleFilterWithRandomBifurcations) {
  // Unknown cards from the safety_play preset: S KQJT987 H KQJT987 D QJT9
  // (18 cards, 9 per hand). On the 200 synthetic random bifurcations from
  // rng(12345), "maximum length in D in any hand is 3" holds on 190/200 (95%),
  // which would be discarded by the standalone random-bifurcation filter,
  // even when the simulated deal set has 85% non-4-0 (covered) and 15% 4-0
  // (failure) deals.
  std::vector<PairedDeal> covered_deals;
  for (int i = 0; i < 45; ++i) {
    covered_deals.emplace_back("S KQJT H KQJ D QJ", "S 987 H T987 D T9");
  }
  for (int i = 0; i < 40; ++i) {
    covered_deals.emplace_back("S KQJ H KQJ D QJT", "S T987 H T987 D 9");
  }
  std::vector<PairedDeal> failure_deals;
  for (int i = 0; i < 8; ++i) {
    failure_deals.emplace_back("S KQJ H KQ D QJT9", "S T987 H JT987");
  }
  for (int i = 0; i < 7; ++i) {
    failure_deals.emplace_back("S T987 H JT987", "S KQJ H KQ D QJT9");
  }

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(
      covered_deals, "hand 1", "hand 3", nullptr, &failure_deals,
      /*is_failure_mode=*/false);

  ASSERT_TRUE(report.HasRule("ShortestLength(D) >= 1"));
  ASSERT_FALSE(report.HasRule("maximum length in D in any hand is 3"));
  ASSERT_FALSE(report.HasRule("hand 1 length in D is between 1 and 3"));
  ASSERT_FALSE(report.HasRule("hand 3 length in D is between 1 and 3"));
  ASSERT_FALSE(report.HasRule("the hand with the DT also has at most 3 D"));
  ASSERT_FALSE(report.HasRule("the hand with the DJ also has at most 3 D"));
  ASSERT_FALSE(report.HasRule("the hand with the DQ also has at most 3 D"));
}

TEST(CrossHandSummarizerTest, ShortestLengthInvariant_RoundTripInvertAndSummarize) {
  auto ge_inv = UnknownCardsInvariant::FromString("ShortestLength(C) >= 3");
  ASSERT_TRUE(ge_inv != nullptr);
  ASSERT_TRUE(ge_inv->ToString() == "ShortestLength(C) >= 3");

  auto le_inv = UnknownCardsInvariant::FromString("ShortestLength(C) <= 2");
  ASSERT_TRUE(le_inv != nullptr);
  ASSERT_TRUE(le_inv->ToString() == "ShortestLength(C) <= 2");

  PairedDeal deal_53("S T98 H KQ9 D KQJ C AT8", "S 4 H 652 D T96 C 76432");
  PairedDeal deal_62("S T984 H KQ9 D KQJ C AT", "H 652 D T96 C 876432");
  ASSERT_TRUE(ge_inv->HoldsOnDeal(deal_53));
  ASSERT_FALSE(ge_inv->HoldsOnDeal(deal_62));
  ASSERT_FALSE(le_inv->HoldsOnDeal(deal_53));
  ASSERT_TRUE(le_inv->HoldsOnDeal(deal_62));

  uint64_t unk = deal_53.hand1 | deal_53.hand2;
  auto inv_of_ge = ge_inv->Invert(unk, 12);
  ASSERT_TRUE(inv_of_ge != nullptr);
  ASSERT_TRUE(inv_of_ge->ToString() == "ShortestLength(C) <= 2");

  auto inv_of_le = le_inv->Invert(unk, 12);
  ASSERT_TRUE(inv_of_le != nullptr);
  ASSERT_TRUE(inv_of_le->ToString() == "ShortestLength(C) >= 3");

  // Simulate a 12-card ending where 41/46 covered deals have 4-4 or 5-3 clubs
  // (89.1% of covered deals, 100% make rate when ShortestLength(C) >= 3 holds)
  // and 5/46 covered deals + 4/4 failure deals have 6-2 or 7-1 clubs.
  std::vector<PairedDeal> covered_deals;
  for (int i = 0; i < 41; ++i) {
    covered_deals.push_back(deal_53);
  }
  for (int i = 0; i < 5; ++i) {
    covered_deals.push_back(deal_62);
  }
  std::vector<PairedDeal> failure_deals;
  for (int i = 0; i < 4; ++i) {
    failure_deals.push_back(deal_62);
  }

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(
      covered_deals, "hand 1", "hand 3", nullptr, &failure_deals,
      /*is_failure_mode=*/false);
  ASSERT_TRUE(report.HasRule("ShortestLength(C) >= 3"));
}

TEST(CrossHandSummarizerTest, SuitHonorVariableInvariant_DirectMethods) {
  // Test FromString and ToString
  auto inv1 =
      UnknownCardsInvariant::FromString("hand 1 always contains Spade $honor");
  ASSERT_TRUE(inv1 != nullptr);
  ASSERT_TRUE(inv1->ToString() == "hand 1 always contains Spade $honor");

  auto inv_legacy =
      UnknownCardsInvariant::FromString("Spade $honor is always in hand 1");
  ASSERT_TRUE(inv_legacy != nullptr);
  ASSERT_TRUE(inv_legacy->ToString() == "hand 1 always contains Spade $honor");

  auto inv2 = UnknownCardsInvariant::FromString("E has Heart $honor");
  ASSERT_TRUE(inv2 != nullptr);
  ASSERT_TRUE(inv2->ToString() == "E has Heart $honor");

  auto inv3 = UnknownCardsInvariant::FromString(
      "both hands always contain Diamond $honor");
  ASSERT_TRUE(inv3 != nullptr);
  ASSERT_TRUE(inv3->ToString() == "both hands always contain Diamond $honor");

  // Test HoldsOnDeal
  // Hand 1 has Spade Ace, Hand 2 has Heart Ace
  PairedDeal deal1("S A3 H 2", "S 42 H A");
  ASSERT_TRUE(inv1->HoldsOnDeal(deal1));
  ASSERT_FALSE(inv3->HoldsOnDeal(deal1));

  // Both hands have Spade and Heart honors
  PairedDeal deal2("S K H K", "S Q H Q");
  auto inv_both_spades = UnknownCardsInvariant::FromString(
      "both hands always contain Spade $honor");
  ASSERT_TRUE(inv_both_spades->HoldsOnDeal(deal2));

  // Test Invert with single unknown honor in suit
  uint64_t unknown_single = ParseHand("S A42 H QJ32");
  auto inv1_inv_single = inv1->Invert(unknown_single, 4, "hand 1", "hand 2");
  ASSERT_TRUE(inv1_inv_single != nullptr);
  ASSERT_TRUE(inv1_inv_single->ToString() == "hand 2 always contains the SA");

  // Test Invert with multiple unknown honors in suit
  uint64_t unknown_multi = ParseHand("S AK42 H QJ32");
  auto inv1_inv_multi = inv1->Invert(unknown_multi, 4, "hand 1", "hand 2");
  ASSERT_TRUE(inv1_inv_multi != nullptr);
  ASSERT_TRUE(inv1_inv_multi->ToString() ==
          "AndInvariant(hand 2 always contains the SK, "
          "hand 2 always contains the SA)");
}

TEST(CrossHandSummarizerTest, HonorPossessionClassifier_VariableSuitHonorExtraction) {
  std::vector<PairedDeal> deals = {
      PairedDeal("H K85 D 8", "H Q7 D 7"),
      PairedDeal("H Q85 D 8", "H K7 D 7"),
      PairedDeal("H KQ8 D 8", "H 75 D 7")
  };

  HonorPossessionClassifier classifier("hand 1", "hand 2");
  for (const auto& d : deals) {
    classifier.ProcessDeal(d);
  }
  auto direct_invs = classifier.ExtractInvariants();
  bool found_shv = false;
  bool found_or = false;
  for (const auto& inv : direct_invs) {
    if (inv->ToString() == "hand 1 always contains Heart $honor") {
      found_shv = true;
    } else if (inv->ToString() ==
               "OrInvariant(hand 1 always contains the HK, "
               "hand 1 always contains the HQ)") {
      found_or = true;
    }
  }
  ASSERT_TRUE(found_shv);
  ASSERT_FALSE(found_or);

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  ASSERT_TRUE(report.total_deals == 3);
  ASSERT_FALSE(report.HasRule("hand 1 always contains the HK"));
  ASSERT_FALSE(report.HasRule("hand 1 always contains the HQ"));
  ASSERT_TRUE(report.HasRule("hand 1 always contains Heart $honor"));
  // 2-honor OrInvariant is suppressed as redundant with suit honor rule
  ASSERT_FALSE(report.HasRule(
      "hand 1 always contains the HK or hand 1 always contains the HQ"));
}

TEST(CrossHandSummarizerTest, HonorPossessionClassifier_SpecificHonorSuppression) {
  std::vector<PairedDeal> deals = {
      PairedDeal("H K85 D 8", "H Q7 D 7"),
      PairedDeal("H K85 D 8", "H Q7 D 7")
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  ASSERT_TRUE(report.total_deals == 2);
  ASSERT_TRUE(report.HasRule("hand 1 always contains the HK"));
  ASSERT_TRUE(report.HasRule("hand 2 always contains the HQ"));
  ASSERT_FALSE(report.HasRule("hand 1 always contains Heart $honor"));
  ASSERT_FALSE(report.HasRule("hand 2 always contains Heart $honor"));
  ASSERT_FALSE(report.HasRule("both hands always contain Heart $honor"));
}

TEST(CrossHandSummarizerTest, HonorPossessionClassifier_AsymmetricSpecificHonorSuppression) {
  std::vector<PairedDeal> deals = {
      PairedDeal("H KJ85 D 8", "H Q7 D 7"),
      PairedDeal("H KQ85 D 8", "H J7 D 7")
  };

  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals);

  ASSERT_TRUE(report.total_deals == 2);
  ASSERT_TRUE(report.HasRule("hand 1 always contains the HK"));
  ASSERT_FALSE(report.HasRule("both hands always contain Heart $honor"));
  ASSERT_TRUE(report.HasRule("hand 2 always contains Heart $honor"));
}

TEST(CrossHandSummarizerTest, HonorPossessionClassifier_CompassDirectionFormatting) {
  std::vector<PairedDeal> deals = {
      PairedDeal("H K85 D 8", "H Q7 D 7"),
      PairedDeal("H Q85 D 8", "H K7 D 7"),
      PairedDeal("H KQ8 D 8", "H 75 D 7")
  };

  HandLabelFormatter formatter("1", "3", HandLabelMode::kCompass);
  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport report = summarizer.Summarize(deals, formatter);

  ASSERT_TRUE(report.total_deals == 3);
  ASSERT_TRUE(report.HasRule("E has Heart $honor"));
}

#include <chrono>
#include <cmath>

namespace {

std::vector<PairedDeal> MakeDeterministicBifurcations(uint64_t unk_mask,
                                                      int h1_size, int count,
                                                      uint64_t seed) {
  std::vector<CardType> cards;
  for (int s = 0; s < 4; ++s) {
    for (int r = 0; r <= 12; ++r) {
      CardType c = Card::GetSuitedCard(static_cast<Suit>(s), r);
      if ((unk_mask & c) != 0) cards.push_back(c);
    }
  }
  std::vector<PairedDeal> out;
  out.reserve(count);
  for (int iter = 0; iter < count; ++iter) {
    for (size_t i = cards.size() - 1; i > 0; --i) {
      seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
      std::swap(cards[i], cards[(seed >> 32) % (i + 1)]);
    }
    uint64_t h1 = 0, h2 = 0;
    for (size_t i = 0; i < cards.size(); ++i) {
      (static_cast<int>(i) < h1_size ? h1 : h2) |= cards[i];
    }
    out.emplace_back(h1, h2);
  }
  return out;
}

void VerifyReportStatsAndRoundTrip(const CrossHandSummaryReport& report,
                                   const std::string& l1 = "hand 1",
                                   const std::string& l2 = "hand 3") {
  for (const auto& st : report.invariant_stats) {
    EXPECT_LT(st.holds_percent(), 95);
    int prec = static_cast<int>(
        std::lround(100.0 * st.success_when_holds_count / st.holds_count));
    EXPECT_GT(prec, 50);
    ASSERT_NE(UnknownCardsInvariant::FromString(st.invariant->ToString(), l1, l2),
              nullptr);
  }
}

}  // namespace

TEST(CrossHandSummarizerTest, DealBitset_InlineAndHeapOperations) {
  for (size_t n : {size_t{65}, size_t{200}, size_t{320}}) {
    DealBitset a(n), b(n);
    for (size_t idx : {size_t{0}, size_t{63}, n / 2, n - 1}) a.Set(idx);
    for (size_t idx : {size_t{63}, n / 2, size_t{17}}) b.Set(idx);

    EXPECT_EQ(a.PopCount(), 4);
    EXPECT_EQ(b.PopCount(), 3);
    EXPECT_TRUE(a.Test(0) && a.Test(n - 1) && !a.Test(17));

    DealBitset inter = a & b, uni = a | b;
    EXPECT_EQ(inter.PopCount(), 2);
    EXPECT_EQ(a.AndPopCount(b), 2);
    EXPECT_EQ(uni.PopCount(), 5);
    EXPECT_EQ(a.OrPopCount(b), 5);
    EXPECT_EQ(a.AndNotCount(b), 2);
    EXPECT_TRUE(inter.IsSubsetOf(a) && inter.IsSubsetOf(b));
    EXPECT_TRUE(!a.IsSubsetOf(b) && a.IsSubsetOf(uni));
  }
}

TEST(CrossHandSummarizerTest, AtomicPredicateCatalog_HonorsOnlyAndBitsetEquivalence) {
  std::vector<PairedDeal> target_deals = {
      PairedDeal("S K92 H Q8 D 4", "H 3 D JT C A75"),
      PairedDeal("S K2 H Q83 D J", "S 9 D T4 C A75"),
      PairedDeal("S 92 H Q3 D JT", "S K H 8 D 4 C A75"),
      PairedDeal("S K9 H 83 C A7", "S 2 H Q D JT4 C 5")};
  std::vector<PairedDeal> opposing_deals = {
      PairedDeal("S 92 H 83 D T4", "S K H Q D J C A75"),
      PairedDeal("D JT4 C A75", "S K92 H Q83")};

  HandLabelFormatter fmt("hand 1", "hand 3");
  AtomicPredicateCatalog catalog(target_deals, &opposing_deals, fmt);
  EXPECT_FALSE(catalog.entries().empty());

  bool saw_honor = false, saw_suit_len = false, saw_shortest = false, saw_cross = false;
  for (const auto& entry : catalog.entries()) {
    ASSERT_NE(entry.invariant, nullptr);
    for (char s : {'S', 'H', 'D', 'C'}) {
      for (char r = '2'; r <= '9'; ++r) {
        std::string spot = {s, r};
        EXPECT_EQ(entry.rule.find(spot), std::string::npos) << entry.rule;
      }
    }
    auto parsed = UnknownCardsInvariant::FromString(
        entry.rule, fmt.hand1_label(), fmt.hand2_label());
    ASSERT_NE(parsed, nullptr) << entry.rule;

    for (size_t i = 0; i < target_deals.size(); ++i) {
      bool exp = entry.invariant->HoldsOnDeal(target_deals[i], fmt.hand1_label(),
                                              fmt.hand2_label());
      EXPECT_EQ(entry.target_bits.Test(i), exp) << entry.rule;
      EXPECT_EQ(parsed->HoldsOnDeal(target_deals[i], fmt.hand1_label(), fmt.hand2_label()), exp);
    }
    for (size_t i = 0; i < opposing_deals.size(); ++i) {
      EXPECT_EQ(entry.opposing_bits.Test(i),
                entry.invariant->HoldsOnDeal(opposing_deals[i], fmt.hand1_label(),
                                             fmt.hand2_label()));
    }
    saw_honor |= (entry.family == AtomicPredicateFamily::kHonorPossession ||
                  entry.family == AtomicPredicateFamily::kSuitHonorVariable);
    saw_suit_len |= (entry.family == AtomicPredicateFamily::kExactSuitLength ||
                     entry.family == AtomicPredicateFamily::kSuitLengthRange ||
                     entry.family == AtomicPredicateFamily::kMaxSuitLength);
    saw_shortest |= (entry.family == AtomicPredicateFamily::kShortestLength);
    saw_cross |= (entry.family == AtomicPredicateFamily::kCrossSuitCard ||
                  entry.family == AtomicPredicateFamily::kCrossSuitLength);
  }
  EXPECT_TRUE(saw_honor && saw_suit_len && saw_shortest && saw_cross);
}

TEST(CrossHandSummarizerTest, UniformContrastGate_UnlocksNonShortestFamilies) {
  // 16/20 (80%) target deals vs 1/20 (5%) opposing deals have HA with singleton H.
  std::vector<PairedDeal> target_deals, opposing_deals;
  for (int i = 0; i < 8; ++i) {
    target_deals.emplace_back("S 876 H A D 54", "S 5 H 432 D 32");
    target_deals.emplace_back("S 5 H 432 D 32", "S 876 H A D 54");
  }
  for (int i = 0; i < 2; ++i) {
    target_deals.emplace_back("S 87 H A43 D 5", "S 65 H 2 D 432");
    target_deals.emplace_back("S 65 H 2 D 432", "S 87 H A43 D 5");
  }
  opposing_deals.emplace_back("S 876 H A D 54", "S 5 H 432 D 32");
  for (int i = 0; i < 19; ++i) {
    opposing_deals.push_back((i % 2 == 0) ? PairedDeal("S 87 H A43 D 5", "S 65 H 2 D 432")
                                          : PairedDeal("S 65 H 2 D 432", "S 87 H A43 D 5"));
  }

  CrossHandDealSetSummarizer summarizer;
  auto report = summarizer.Summarize(target_deals, HandLabelFormatter("hand 1", "hand 3"),
                                     nullptr, &opposing_deals, false);
  EXPECT_TRUE(report.HasRule("the hand with the HA also has exactly 1 H"));
  VerifyReportStatsAndRoundTrip(report);
}

TEST(CrossHandSummarizerTest, AndBeamPass_SynthesizesDirectConjunctionWithoutInvert) {
  std::vector<PairedDeal> fail_deals, succ_deals;
  for (int i = 0; i < 10; ++i) {
    fail_deals.emplace_back("S 43 H 54 D 54 C 43", "S A2 H 32 D 32 C Q2");
    succ_deals.emplace_back("S A2 H 54 D 54 C Q2", "S 43 H 32 D 32 C 43");
    succ_deals.emplace_back("S A2 H 54 D 54 C 43", "S 43 H 32 D 32 C Q2");
    succ_deals.emplace_back("S 43 H 54 D 54 C Q2", "S A2 H 32 D 32 C 43");
  }

  HandLabelFormatter fmt("hand 1", "hand 3");
  CrossHandDealSetSummarizer summarizer;
  auto fail_report = summarizer.Summarize(fail_deals, fmt, nullptr, &succ_deals, true);
  auto succ_report = summarizer.Summarize(succ_deals, fmt, nullptr, &fail_deals, false);

  EXPECT_TRUE(fail_report.HasRule("AndInvariant(W has the CQ, W has the SA)"));
  for (const auto& st : fail_report.invariant_stats) {
    const std::string s = st.invariant->ToString();
    EXPECT_TRUE(s != "W has the CQ" && s != "hand 3 always contains the CQ" &&
                s != "W has the SA" && s != "hand 3 always contains the SA");
  }
  EXPECT_TRUE(succ_report.HasRule("OrInvariant(E has the CQ, E has the SA)"));
}

TEST(CrossHandSummarizerTest,
     SetCoverOrBeamPass_SynthesizesThreeClauseDisjunctionAndPrunesDominated) {
  std::vector<PairedDeal> fail_deals, succ_deals;
  for (int i = 0; i < 5; ++i) {
    fail_deals.emplace_back("S 76543 H 43 D K C 43", "S 2 H 7652 D Q43 C A2");
    fail_deals.emplace_back("S 2 H 7652 D Q43 C 43", "S 76543 H 43 D K C A2");
    fail_deals.emplace_back("S 765 H 765 D KQ C 43", "S 432 H 432 D 43 C A2");
    fail_deals.emplace_back("S 765 H 765 D 43 C 43", "S 432 H 432 D KQ C A2");
    fail_deals.emplace_back("S 765 H 765 D K4 C A4", "S 432 H 432 D Q3 C 32");
    fail_deals.emplace_back("S 765 H 765 D Q4 C A4", "S 432 H 432 D K3 C 32");
  }
  for (int i = 0; i < 30; ++i) {
    succ_deals.emplace_back("S 765 H 765 D K4 C 43", "S 432 H 432 D Q3 C A2");
    succ_deals.emplace_back("S 765 H 7654 D Q C 43", "S 432 H 32 D K43 C A2");
    succ_deals.emplace_back("S 765 H 43 D K43 C 43", "S 432 H 7652 D Q C A2");
  }

  HandLabelFormatter fmt("hand 1", "hand 3");
  CrossHandDealSetSummarizer summarizer;
  CrossHandSummaryReport fail_report;
  double best_ms = 1e9;
  for (int trial = 0; trial < 3; ++trial) {
    auto t0 = std::chrono::steady_clock::now();
    fail_report = summarizer.Summarize(fail_deals, fmt, nullptr, &succ_deals, true);
    best_ms = std::min(best_ms, std::chrono::duration<double, std::milli>(
                                    std::chrono::steady_clock::now() - t0)
                                    .count());
  }
  EXPECT_LT(best_ms, 5.0);

  const std::string expected_3or =
      "OrInvariant(OrInvariant(E has the CA, ShortestLength(S) <= 1), "
      "the hand with the DK also has the DQ)";
  EXPECT_TRUE(fail_report.HasRule(expected_3or));
  VerifyReportStatsAndRoundTrip(fail_report);
  for (const auto& st : fail_report.invariant_stats) {
    if (st.invariant->ToString() != expected_3or) {
      EXPECT_EQ(st.invariant->ToString().find(expected_3or), std::string::npos);
    }
  }
}

TEST(CrossHandSummarizerTest,
     QuantileExtractors_CeilingQuantileToleranceAndSmallSampleExactness) {
  std::vector<PairedDeal> twenty_deals(19, PairedDeal("S 43 H K D 43", "S 2 H 2 D 2 C 43"));
  twenty_deals.emplace_back("S 4 H 2 D 43 C 4", "S 32 H K D 2 C 3");

  HandLabelFormatter fmt("hand 1", "hand 3");
  JointSuitRuleExtractor suit_ext(fmt);
  HonorPossessionClassifier honor_cls(fmt);
  for (const auto& d : twenty_deals) {
    suit_ext.ProcessDeal(d);
    honor_cls.ProcessDeal(d);
  }
  EXPECT_EQ(suit_ext.GetSuitBounds().at(Suit::SPADES).min_hand1, 1);
  EXPECT_EQ(suit_ext.GetSuitBounds().at(Suit::SPADES).max_hand1, 2);

  auto has_rule = [](const auto& invs, const std::string& target) {
    return std::any_of(invs.begin(), invs.end(),
                       [&](const auto& inv) { return inv->ToString() == target; });
  };
  EXPECT_TRUE(has_rule(suit_ext.ExtractInvariants(), "hand 1 length in S is always 2"));
  EXPECT_TRUE(has_rule(honor_cls.ExtractInvariants(), "hand 1 always contains the HK"));

  CrossSuitConditionalRuleExtractor movable_cond_ext;
  for (int i = 0; i < 10; ++i)
    movable_cond_ext.ProcessDeal(PairedDeal("S 43 H K D 43", "S 2 H 2 D 2 C 43"));
  for (int i = 0; i < 9; ++i)
    movable_cond_ext.ProcessDeal(PairedDeal("S 432 H 2 C 4", "H K D 432 C 3"));
  movable_cond_ext.ProcessDeal(PairedDeal("S 4 H 2 D 43 C 4", "S 32 H K D 2 C 3"));
  EXPECT_TRUE(has_rule(movable_cond_ext.ExtractInvariants(),
                       "the hand with the HK also has at least 2 D"));

  // < 10 deals (8/9 = 88.9%): ceiling quantile requires 9/9 = 100% agreement.
  std::vector<PairedDeal> nine_deals(8, PairedDeal("S 43 H K D 43", "S 2 H 2 D 2 C 43"));
  nine_deals.emplace_back("S 4 H 2 D 43 C 4", "S 32 H K D 2 C 3");
  JointSuitRuleExtractor small_suit(fmt);
  HonorPossessionClassifier small_honor(fmt);
  for (const auto& d : nine_deals) {
    small_suit.ProcessDeal(d);
    small_honor.ProcessDeal(d);
  }
  EXPECT_FALSE(has_rule(small_suit.ExtractInvariants(), "hand 1 length in S is always 2"));
  EXPECT_FALSE(has_rule(small_honor.ExtractInvariants(), "hand 1 always contains the HK"));
}

TEST(CrossHandSummarizerTest,
     SetCoverSynthesizer_NineCardHeartsSmallSampleOverfitAndSpotConnectedSuppression) {
  const uint64_t unk_mask = PairedDeal("S AQJT64 D KQJ976 C QT9843", "").hand1;
  const CardType cq = Card::GetSuitedCard(Suit::CLUBS, 10);
  const CardType sa = Card::GetSuitedCard(Suit::SPADES, 12);

  std::vector<PairedDeal> succ_deals, fail_deals;
  int trap_wins = 0;
  for (const auto& d : MakeDeterministicBifurcations(unk_mask, 9, 400, 20261122ULL)) {
    if (succ_deals.size() >= 23 && fail_deals.size() >= 7) break;
    int w_clubs = CardsInSuit(d.hand2, Suit::CLUBS);
    if (((d.hand1 & cq) != 0) || ((d.hand1 & sa) != 0)) {
      if ((d.hand2 & sa) != 0 && w_clubs <= 2 && succ_deals.size() < 23) {
        succ_deals.push_back(d);
        ++trap_wins;
      } else if (succ_deals.size() < 19 || (trap_wins >= 4 && succ_deals.size() < 23)) {
        succ_deals.push_back(d);
      }
    } else if (w_clubs >= 3 && fail_deals.size() < 7) {
      fail_deals.push_back(d);
    }
  }
  ASSERT_EQ(succ_deals.size(), 23u);
  ASSERT_EQ(fail_deals.size(), 7u);
  ASSERT_GE(trap_wins, 4);

  HandLabelFormatter fmt("hand 1", "hand 3");
  CrossHandDealSetSummarizer summarizer;
  auto succ_report = summarizer.Summarize(succ_deals, fmt, nullptr, &fail_deals, false);
  auto fail_report = summarizer.Summarize(fail_deals, fmt, nullptr, &succ_deals, true);

  ASSERT_EQ(succ_report.invariant_stats.size(), 1u);
  EXPECT_EQ(succ_report.invariant_stats[0].invariant->ToString(),
            "OrInvariant(E has the CQ, E has the SA)");
  EXPECT_EQ(succ_report.invariant_stats[0].holds_count, 23);
  EXPECT_EQ(succ_report.invariant_stats[0].success_when_holds_count, 23);
  ASSERT_FALSE(fail_report.invariant_stats.empty());
  EXPECT_EQ(fail_report.invariant_stats[0].invariant->ToString(),
            "AndInvariant(W has the CQ, W has the SA)");
}

TEST(CrossHandSummarizerTest,
     SetCoverSynthesizer_RewritesConditionalPairedWithHonorPossession) {
  const uint64_t unk_mask = PairedDeal("S A654 D 65432 C 65432", "").hand1;
  const CardType sa = Card::GetSuitedCard(Suit::SPADES, 12);

  std::vector<PairedDeal> succ_deals, fail_deals;
  int w_sa_short_c = 0;
  for (const auto& d : MakeDeterministicBifurcations(unk_mask, 7, 500, 123456ULL)) {
    if (succ_deals.size() >= 60 && fail_deals.size() >= 20) break;
    int w_clubs = CardsInSuit(d.hand2, Suit::CLUBS);
    bool e_sa = (d.hand1 & sa) != 0;
    if (e_sa || w_clubs <= 1) {
      if (!e_sa && succ_deals.size() < 60) {
        succ_deals.push_back(d);
        ++w_sa_short_c;
      } else if (e_sa && (succ_deals.size() < 42 || (w_sa_short_c >= 18 && succ_deals.size() < 60))) {
        succ_deals.push_back(d);
      }
    } else if (fail_deals.size() < 20) {
      fail_deals.push_back(d);
    }
  }
  ASSERT_EQ(succ_deals.size(), 60u);
  ASSERT_EQ(fail_deals.size(), 20u);

  HandLabelFormatter fmt("hand 1", "hand 3");
  CrossHandDealSetSummarizer summarizer;
  auto succ_report = summarizer.Summarize(succ_deals, fmt, nullptr, &fail_deals, false);
  EXPECT_TRUE(succ_report.HasRule("OrInvariant(E has the SA, E length in C is between 4 and 5)") ||
              succ_report.HasRule("OrInvariant(E has the SA, W length in C is between 0 and 1)"));
  VerifyReportStatsAndRoundTrip(succ_report);
  for (const auto& st : succ_report.invariant_stats) {
    const std::string s = st.invariant->ToString();
    if (s.find("OrInvariant(") != std::string::npos && s.find("E has the SA") != std::string::npos) {
      EXPECT_EQ(s.find("the hand with the SA"), std::string::npos) << s;
    }
  }

  const uint64_t unk_qj = PairedDeal("S QJ65 D 5432 C 5432", "").hand1;
  const CardType sj = Card::GetSuitedCard(Suit::SPADES, 9);
  const CardType sq = Card::GetSuitedCard(Suit::SPADES, 10);
  std::vector<PairedDeal> qj_fail, qj_succ;
  for (const auto& d : MakeDeterministicBifurcations(unk_qj, 6, 300, 654321ULL)) {
    if (qj_fail.size() >= 20 && qj_succ.size() >= 60) break;
    if ((d.hand2 & sj) != 0 && (d.hand2 & sq) != 0) {
      if (qj_fail.size() < 20) qj_fail.push_back(d);
    } else if (qj_succ.size() < 60) {
      qj_succ.push_back(d);
    }
  }
  auto qj_report = summarizer.Summarize(qj_fail, fmt, nullptr, &qj_succ, true);
  EXPECT_TRUE(qj_report.HasRule("AndInvariant(W has the SJ, W has the SQ)"));
  for (const auto& st : qj_report.invariant_stats) {
    EXPECT_EQ(st.invariant->ToString().find("the hand with the S"), std::string::npos);
  }
}

