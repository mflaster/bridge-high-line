#include "cross_hand_summarizer.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

static int CardsInSuit(uint64_t hand, Suit s) {
  uint16_t suit_bits = (hand >> (static_cast<int>(s) * 16)) & 0xffff;
  int count = 0;
  while (suit_bits) {
    count += (suit_bits & 1);
    suit_bits >>= 1;
  }
  return count;
}

void JointSuitRuleExtractor::ProcessDeal(const PairedDeal& deal) {
  total_deals_++;
  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    int len1 = CardsInSuit(deal.hand1, suit);
    int len2 = CardsInSuit(deal.hand2, suit);
    int combined = len1 + len2;

    auto& bounds = suit_bounds_[suit];
    bounds.min_hand1 = std::min(bounds.min_hand1, len1);
    bounds.max_hand1 = std::max(bounds.max_hand1, len1);
    bounds.min_hand2 = std::min(bounds.min_hand2, len2);
    bounds.max_hand2 = std::max(bounds.max_hand2, len2);
    bounds.min_combined = std::min(bounds.min_combined, combined);
    bounds.max_combined = std::max(bounds.max_combined, combined);
  }
}

std::vector<std::string> JointSuitRuleExtractor::ExtractRules() const {
  std::vector<std::string> rules;
  if (total_deals_ == 0) return rules;

  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    auto it = suit_bounds_.find(suit);
    if (it == suit_bounds_.end()) continue;

    const auto& bounds = it->second;
    std::string suit_str = SuitDebugString(suit);

    if (bounds.min_hand1 == bounds.max_hand1) {
      rules.push_back("hand 1 length in " + suit_str + " is always " + std::to_string(bounds.min_hand1));
    } else {
      rules.push_back("hand 1 length in " + suit_str + " is between " +
                      std::to_string(bounds.min_hand1) + " and " +
                      std::to_string(bounds.max_hand1));
    }
    if (bounds.min_hand2 == bounds.max_hand2) {
      rules.push_back("hand 2 length in " + suit_str + " is always " + std::to_string(bounds.min_hand2));
    } else {
      rules.push_back("hand 2 length in " + suit_str + " is between " +
                      std::to_string(bounds.min_hand2) + " and " +
                      std::to_string(bounds.max_hand2));
    }
    if (bounds.min_combined == bounds.max_combined) {
      rules.push_back("combined length in " + suit_str + " is always " + std::to_string(bounds.min_combined));
    } else {
      rules.push_back("combined length in " + suit_str + " is between " +
                      std::to_string(bounds.min_combined) + " and " +
                      std::to_string(bounds.max_combined));
    }
    if (bounds.min_combined >= 8) {
      rules.push_back("combined length in " + suit_str + " has an 8+ card fit");
    }

    if (bounds.max_combined > 0) {
      int max_either = std::max(bounds.max_hand1, bounds.max_hand2);
      rules.push_back("maximum length in " + suit_str + " in any hand is " + std::to_string(max_either));
    }
  }
  return rules;
}

void HonorPossessionClassifier::ProcessDeal(const PairedDeal& deal) {
  total_deals_++;
  // Filter out spot cards (ranks 2 through 9) using 64-bit honor mask
  uint64_t h1_honors = deal.hand1 & kHonorMask;
  uint64_t h2_honors = deal.hand2 & kHonorMask;

  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    for (int r = 8; r <= 12; ++r) {
      uint64_t honor_bit = Card::GetSuitedCard(suit, r);
      bool h1_has = (h1_honors & honor_bit) != 0;
      bool h2_has = (h2_honors & honor_bit) != 0;

      auto& stats = honor_stats_[suit][r];
      if (h1_has && h2_has) {
        stats.both_count++;
      } else if (h1_has) {
        stats.hand1_count++;
      } else if (h2_has) {
        stats.hand2_count++;
      } else {
        stats.neither_count++;
      }
    }
  }
}

std::vector<std::string> HonorPossessionClassifier::ExtractRules() const {
  std::vector<std::string> rules;
  if (total_deals_ == 0) return rules;

  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    auto suit_it = honor_stats_.find(suit);
    if (suit_it == honor_stats_.end()) continue;

    // Evaluate in descending rank order: A, K, Q, J, 10
    for (int r = 12; r >= 8; --r) {
      auto rank_it = suit_it->second.find(r);
      if (rank_it == suit_it->second.end()) continue;

      const auto& stats = rank_it->second;
      std::string card_str = SuitDebugString(suit) + Card::PrintCardValue(r);

      if (stats.hand1_count + stats.both_count == total_deals_ && stats.both_count < total_deals_) {
        rules.push_back("hand 1 always contains the " + card_str);
      } else if (stats.hand2_count + stats.both_count == total_deals_ && stats.both_count < total_deals_) {
        rules.push_back("hand 2 always contains the " + card_str);
      } else if (stats.both_count == total_deals_) {
        rules.push_back("both hands always contain the " + card_str);
      }
    }
  }
  return rules;
}

std::string CrossHandSummaryReport::ToString() const {
  std::string out = "Cross-Hand Deal Set Summary Report\n";
  out += "Total Deals: " + std::to_string(total_deals) + "\n";
  out += "Invariant Rules:\n";
  if (invariant_rules.empty()) {
    out += "  (None)\n";
  } else {
    for (const auto& rule : invariant_rules) {
      out += "  - " + rule + "\n";
    }
  }
  return out;
}

CrossHandSummaryReport CrossHandDealSetSummarizer::Summarize(
    const std::vector<PairedDeal>& deals) const {
  CrossHandSummaryReport report;
  report.total_deals = static_cast<int>(deals.size());
  if (deals.empty()) {
    return report;
  }

  uint64_t expected_overall = deals[0].hand1 | deals[0].hand2;
  for (const auto& deal : deals) {
    if ((deal.hand1 & deal.hand2) != 0) {
      throw std::invalid_argument(
          "PairedDeal has overlapping cards between hand1 and hand2.");
    }
    if ((deal.hand1 | deal.hand2) != expected_overall) {
      throw std::invalid_argument(
          "All PairedDeals in a deal set must have the identical overall set of cards.");
    }
  }

  JointSuitRuleExtractor suit_extractor;
  HonorPossessionClassifier honor_classifier;

  for (const auto& deal : deals) {
    suit_extractor.ProcessDeal(deal);
    honor_classifier.ProcessDeal(deal);
  }

  report.suit_bounds = suit_extractor.GetSuitBounds();
  report.honor_stats = honor_classifier.GetHonorStats();

  auto suit_rules = suit_extractor.ExtractRules();
  auto honor_rules = honor_classifier.ExtractRules();

  report.invariant_rules.insert(report.invariant_rules.end(), suit_rules.begin(), suit_rules.end());
  report.invariant_rules.insert(report.invariant_rules.end(), honor_rules.begin(), honor_rules.end());

  return report;
}
