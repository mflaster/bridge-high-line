#include "cross_hand_summarizer.h"

#include <algorithm>
#include <iostream>
#include <set>
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

  int total_unknown_cards = 0;
  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    auto it = suit_bounds_.find(suit);
    if (it != suit_bounds_.end()) {
      total_unknown_cards += it->second.max_combined;
    }
  }

  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    auto it = suit_bounds_.find(suit);
    if (it == suit_bounds_.end()) continue;

    const auto& bounds = it->second;
    std::string suit_str = SuitDebugString(suit);

    // Skip suit length rules if there are no cards of this suit in the unknown pool (tautology)
    if (bounds.max_combined == 0) {
      continue;
    }

    // Skip suit length rules if ALL unknown cards belong to this single suit (tautology)
    if (bounds.max_combined == total_unknown_cards) {
      continue;
    }

    if (bounds.min_hand1 == bounds.max_hand1) {
      rules.push_back(hand1_label_ + " length in " + suit_str + " is always " + std::to_string(bounds.min_hand1));
    } else {
      rules.push_back(hand1_label_ + " length in " + suit_str + " is between " +
                      std::to_string(bounds.min_hand1) + " and " +
                      std::to_string(bounds.max_hand1));
    }
    if (bounds.min_hand2 == bounds.max_hand2) {
      rules.push_back(hand2_label_ + " length in " + suit_str + " is always " + std::to_string(bounds.min_hand2));
    } else {
      rules.push_back(hand2_label_ + " length in " + suit_str + " is between " +
                      std::to_string(bounds.min_hand2) + " and " +
                      std::to_string(bounds.max_hand2));
    }

    // Note: Combined length ("combined length in S is always X") is omitted as a tautology
    // because every deal in a set is drawn from identical unknown cards.

    // Emit maximum suit length in any hand if hand lengths vary across deals
    if (bounds.min_hand1 != bounds.max_hand1 || bounds.min_hand2 != bounds.max_hand2) {
      int max_either = std::max(bounds.max_hand1, bounds.max_hand2);
      rules.push_back("maximum length in " + suit_str + " in any hand is " + std::to_string(max_either));
    }
  }
  return rules;
}

void HonorPossessionClassifier::ProcessDeal(const PairedDeal& deal) {
  total_deals_++;
  // Filter out spot cards (ranks 2 through 9) using 64-bit honor mask
  uint64_t h1_honors = ExtractHonors(deal.hand1);
  uint64_t h2_honors = ExtractHonors(deal.hand2);

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
        rules.push_back(hand1_label_ + " always contains the " + card_str);
      } else if (stats.hand2_count + stats.both_count == total_deals_ && stats.both_count < total_deals_) {
        rules.push_back(hand2_label_ + " always contains the " + card_str);
      } else if (stats.both_count == total_deals_) {
        rules.push_back("both hands always contain the " + card_str);
      }
    }
  }
  return rules;
}

void CrossSuitConditionalRuleExtractor::ProcessDeal(const PairedDeal& deal) {
  total_deals_++;
  deals_.push_back(deal);
}

std::vector<std::string> CrossSuitConditionalRuleExtractor::ExtractRules() const {
  std::vector<std::string> rules;
  if (total_deals_ == 0 || deals_.empty()) return rules;

  uint64_t unknown_cards = deals_[0].hand1 | deals_[0].hand2;
  int cards_per_hand = PopCount64(deals_[0].hand1);

  // Collect all honor cards (ranks 10, J, Q, K, A) present in unknown_cards
  std::vector<CardType> unknown_card_list;
  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    for (int r = 8; r <= 12; ++r) {
      CardType card_bit = Card::GetSuitedCard(suit, r);
      if ((unknown_cards & card_bit) != 0) {
        unknown_card_list.push_back(card_bit);
      }
    }
  }

  std::set<std::string> emitted_rules;

  for (CardType card : unknown_card_list) {
    // Condition 1: For every DealPair, card must be true for exactly one of the 2 hands.
    // Since card is in unknown_cards and hand1/hand2 partition unknown_cards in every deal,
    // card is held by hand1 XOR hand2 for every DealPair.
    int hand1_deals = 0;
    int hand2_deals = 0;
    for (const auto& deal : deals_) {
      if ((deal.hand1 & card) != 0) {
        hand1_deals++;
      } else {
        hand2_deals++;
      }
    }

    // Skip cards that do not move between hands across deals (i.e. statically fixed in one hand)
    // as those are already covered by unconditional rules.
    if (hand1_deals == 0 || hand2_deals == 0) {
      continue;
    }

    std::string x_str = "the " + Card::PrintCard(card);

    // Evaluate suit length bounds for the hand holding card
    for (int s = 0; s < 4; ++s) {
      Suit suit = static_cast<Suit>(s);
      std::string suit_str = SuitDebugString(suit);

      int min_len_X = 999;
      int max_len_X = -1;
      int min_len_notX = 999;
      int max_len_notX = -1;

      for (const auto& deal : deals_) {
        bool in_hand1 = (deal.hand1 & card) != 0;
        uint64_t handX = in_hand1 ? deal.hand1 : deal.hand2;
        uint64_t handNotX = in_hand1 ? deal.hand2 : deal.hand1;

        int lenX = CardsInSuit(handX, suit);
        int lenNotX = CardsInSuit(handNotX, suit);

        min_len_X = std::min(min_len_X, lenX);
        max_len_X = std::max(max_len_X, lenX);
        min_len_notX = std::min(min_len_notX, lenNotX);
        max_len_notX = std::max(max_len_notX, lenNotX);
      }

      // Check Minimum Suit Length
      if (min_len_X > 0) {
        // Condition 2: Y ("at least min_len_X suit") shouldn't be true for the not-X hand.
        // It is not true for not-X hand if min_len_notX < min_len_X.
        if (min_len_notX < min_len_X) {
          std::string y_str;
          if (min_len_X == max_len_X) {
            y_str = "exactly " + std::to_string(min_len_X) + " " + suit_str;
          } else {
            y_str = "at least " + std::to_string(min_len_X) + " " + suit_str;
          }
          std::string rule = "the hand with " + x_str + " also has " + y_str;
          if (emitted_rules.insert(rule).second) {
            rules.push_back(rule);
          }
        }
      }

      // Check Maximum Suit Length
      if (min_len_X < max_len_X && max_len_X < cards_per_hand) {
        // Condition 2: Y ("at most max_len_X suit") shouldn't be true for the not-X hand.
        // It is not true for not-X hand if max_len_notX > max_len_X.
        if (max_len_notX > max_len_X) {
          std::string y_str = "at most " + std::to_string(max_len_X) + " " + suit_str;
          std::string rule = "the hand with " + x_str + " also has " + y_str;
          if (emitted_rules.insert(rule).second) {
            rules.push_back(rule);
          }
        }
      }
    }

    // Evaluate co-possession of another card c2
    for (CardType card2 : unknown_card_list) {
      if (card == card2) continue;

      bool always_co_held = true;
      for (const auto& deal : deals_) {
        bool in_hand1 = (deal.hand1 & card) != 0;
        uint64_t handX = in_hand1 ? deal.hand1 : deal.hand2;
        if ((handX & card2) == 0) {
          always_co_held = false;
          break;
        }
      }

      if (always_co_held) {
        // Condition 2: Since card2 is in unknown_cards, not-X hand NEVER holds card2.
        std::string y_str = "the " + Card::PrintCard(card2);
        std::string rule = "the hand with " + x_str + " also has " + y_str;
        if (emitted_rules.insert(rule).second) {
          rules.push_back(rule);
        }
      }
    }
  }

  return rules;
}

std::string CrossHandSummaryReport::ToString() const {
  std::string out = "Cross-Hand Deal Set Summary Report\n";
  out += "Total Deals: " + std::to_string(total_deals) + "\n";
  out += "Invariant Rules for " + std::to_string(cards_per_hand) + " card ending:\n";
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
    const std::vector<PairedDeal>& deals,
    const std::string& hand1_label,
    const std::string& hand2_label) const {
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

  report.unknown_cards = expected_overall;
  report.cards_per_hand = CountCardsInHand(deals[0].hand1);

  JointSuitRuleExtractor suit_extractor(hand1_label, hand2_label);
  HonorPossessionClassifier honor_classifier(hand1_label, hand2_label);
  CrossSuitConditionalRuleExtractor conditional_extractor(hand1_label, hand2_label);

  for (const auto& deal : deals) {
    suit_extractor.ProcessDeal(deal);
    honor_classifier.ProcessDeal(deal);
    conditional_extractor.ProcessDeal(deal);
  }

  report.suit_bounds = suit_extractor.GetSuitBounds();
  report.honor_stats = honor_classifier.GetHonorStats();

  auto suit_rules = suit_extractor.ExtractRules();
  auto conditional_rules = conditional_extractor.ExtractRules();
  auto honor_rules = honor_classifier.ExtractRules();

  report.invariant_rules.insert(report.invariant_rules.end(), suit_rules.begin(), suit_rules.end());
  report.invariant_rules.insert(report.invariant_rules.end(), conditional_rules.begin(), conditional_rules.end());
  report.invariant_rules.insert(report.invariant_rules.end(), honor_rules.begin(), honor_rules.end());

  return report;
}
