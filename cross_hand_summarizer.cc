#include "cross_hand_summarizer.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <stdexcept>

#include "conditional_line.h"
#include "glog/logging.h"
#include "unknown_cards_invariant.h"

static int CardsInSuit(uint64_t hand, Suit s) {
  uint16_t suit_bits = (hand >> (static_cast<int>(s) * 16)) & 0xffff;
  int count = 0;
  while (suit_bits) {
    count += (suit_bits & 1);
    suit_bits >>= 1;
  }
  return count;
}

// --- HandLabelFormatter Implementation ---

std::string HandLabelFormatter::TranslateCompass(const std::string& raw_label) {
  if (raw_label.empty()) return "";
  if (raw_label == "hand 0" || raw_label == "0" || raw_label == "North" || raw_label == "north" || raw_label == "N") {
    return "N";
  }
  if (raw_label == "hand 1" || raw_label == "1" || raw_label == "East" || raw_label == "east" || raw_label == "E") {
    return "E";
  }
  if (raw_label == "hand 2" || raw_label == "2" || raw_label == "South" || raw_label == "south" || raw_label == "S") {
    return "S";
  }
  if (raw_label == "hand 3" || raw_label == "3" || raw_label == "West" || raw_label == "west" || raw_label == "W") {
    return "W";
  }
  return raw_label;
}

std::string HandLabelFormatter::FormatLabel(const std::string& raw_label) const {
  if (raw_label.empty()) return "";
  if (mode_ == HandLabelMode::kCompass) {
    return TranslateCompass(raw_label);
  }
  return raw_label;
}

std::string HandLabelFormatter::FormatHand1() const {
  return FormatLabel(hand1_label_);
}

std::string HandLabelFormatter::FormatHand2() const {
  return FormatLabel(hand2_label_);
}

std::string HandLabelFormatter::FormatSingleCardHand1() const {
  return FormatHand1();
}

std::string HandLabelFormatter::FormatSingleCardHand2() const {
  return FormatHand2();
}

std::string HandLabelFormatter::FormatCompositeHand1() const {
  if (mode_ == HandLabelMode::kCompass) {
    return FormatHand1();
  }
  return FormatCompassOrVerbatim(hand1_label_);
}

std::string HandLabelFormatter::FormatCompositeHand2() const {
  if (mode_ == HandLabelMode::kCompass) {
    return FormatHand2();
  }
  return FormatCompassOrVerbatim(hand2_label_);
}

std::string HandLabelFormatter::FormatCompassOrVerbatim(const std::string& raw_label) const {
  if (raw_label.empty()) return "";
  if (raw_label == "hand 1") return "E";
  if (raw_label == "hand 2" || raw_label == "hand 3") return "W";
  return raw_label;
}

// --- Extractors Implementation ---

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

std::vector<std::shared_ptr<UnknownCardsInvariant>> JointSuitRuleExtractor::ExtractInvariants() const {
  std::vector<std::shared_ptr<UnknownCardsInvariant>> invariants;
  if (total_deals_ == 0) return invariants;

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

    if (bounds.max_combined == 0 || bounds.max_combined == total_unknown_cards) {
      continue;
    }

    if (bounds.min_hand1 == bounds.max_hand1) {
      invariants.push_back(std::make_shared<ExactSuitLengthInvariant>(
          InvariantHandChoice::kHand1, suit, bounds.min_hand1, formatter_.FormatSingleCardHand1()));
    } else if (bounds.min_hand1 > 0 || bounds.max_hand1 < bounds.max_combined) {
      invariants.push_back(std::make_shared<SuitLengthRangeInvariant>(
          InvariantHandChoice::kHand1, suit, bounds.min_hand1, bounds.max_hand1, formatter_.FormatSingleCardHand1()));
    }

    if (bounds.min_hand2 == bounds.max_hand2) {
      invariants.push_back(std::make_shared<ExactSuitLengthInvariant>(
          InvariantHandChoice::kHand2, suit, bounds.min_hand2, formatter_.FormatSingleCardHand2()));
    } else if (bounds.min_hand2 > 0 || bounds.max_hand2 < bounds.max_combined) {
      invariants.push_back(std::make_shared<SuitLengthRangeInvariant>(
          InvariantHandChoice::kHand2, suit, bounds.min_hand2, bounds.max_hand2, formatter_.FormatSingleCardHand2()));
    }

    int max_either = std::max(bounds.max_hand1, bounds.max_hand2);
    if (max_either < bounds.max_combined) {
      invariants.push_back(std::make_shared<MaxSuitLengthInvariant>(suit, max_either));
    }
  }
  return invariants;
}

std::vector<std::string> JointSuitRuleExtractor::ExtractRules() const {
  std::vector<std::string> rules;
  for (const auto& inv : ExtractInvariants()) {
    if (inv) {
      rules.push_back(inv->ToString());
    }
  }
  return rules;
}

void HonorPossessionClassifier::ProcessDeal(const PairedDeal& deal) {
  total_deals_++;
  deals_.push_back(deal);
  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    for (int r = 8; r <= 12; ++r) {
      CardType card = Card::GetSuitedCard(suit, r);
      bool h1_has = (deal.hand1 & card) != 0;
      bool h2_has = (deal.hand2 & card) != 0;

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

std::vector<std::shared_ptr<UnknownCardsInvariant>> HonorPossessionClassifier::ExtractInvariants() const {
  std::vector<std::shared_ptr<UnknownCardsInvariant>> invariants;
  if (total_deals_ == 0) return invariants;

  std::vector<std::shared_ptr<HonorPossessionInvariant>> non_100_candidates;
  std::vector<std::shared_ptr<HonorPossessionInvariant>> all_candidates;

  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    auto suit_it = honor_stats_.find(suit);
    if (suit_it == honor_stats_.end()) continue;

    for (int r = 8; r <= 12; ++r) {
      auto r_it = suit_it->second.find(r);
      if (r_it == suit_it->second.end()) continue;

      const auto& stats = r_it->second;
      CardType card = Card::GetSuitedCard(suit, r);

      std::string l1_single = formatter_.FormatSingleCardHand1();
      std::string l2_single = formatter_.FormatSingleCardHand2();
      std::string l1_composite = formatter_.FormatCompositeHand1();
      std::string l2_composite = formatter_.FormatCompositeHand2();

      if (stats.hand1_count == total_deals_) {
        invariants.push_back(std::make_shared<HonorPossessionInvariant>(
            HonorPossessionTargetHand::kHand1, card, l1_single));
        all_candidates.push_back(std::make_shared<HonorPossessionInvariant>(
            HonorPossessionTargetHand::kHand1, card, l1_composite));
      } else if (stats.hand2_count == total_deals_) {
        invariants.push_back(std::make_shared<HonorPossessionInvariant>(
            HonorPossessionTargetHand::kHand2, card, l2_single));
        all_candidates.push_back(std::make_shared<HonorPossessionInvariant>(
            HonorPossessionTargetHand::kHand2, card, l2_composite));
      } else if (stats.both_count == total_deals_) {
        invariants.push_back(std::make_shared<HonorPossessionInvariant>(
            HonorPossessionTargetHand::kBoth, card, "both hands"));
      } else if (stats.hand1_count + stats.hand2_count + stats.both_count > 0) {
        auto inv1 = std::make_shared<HonorPossessionInvariant>(
            HonorPossessionTargetHand::kHand1, card, l1_composite);
        auto inv2 = std::make_shared<HonorPossessionInvariant>(
            HonorPossessionTargetHand::kHand2, card, l2_composite);
        non_100_candidates.push_back(inv1);
        non_100_candidates.push_back(inv2);
        all_candidates.push_back(inv1);
        all_candidates.push_back(inv2);
      }
    }
  }

  AddOrInvariants(non_100_candidates, invariants);
  AddAndInvariants(all_candidates, invariants);

  return invariants;
}

void HonorPossessionClassifier::AddOrInvariants(
    const std::vector<std::shared_ptr<HonorPossessionInvariant>>& non_100_candidates,
    std::vector<std::shared_ptr<UnknownCardsInvariant>>& invariants) const {
  std::set<std::string> emitted_or_rules;
  size_t num_non_100 = non_100_candidates.size();
  for (size_t i = 0; i < num_non_100; ++i) {
    for (size_t j = i + 1; j < num_non_100; ++j) {
      auto a = non_100_candidates[i];
      auto b = non_100_candidates[j];
      if (a->ToString() == b->ToString()) {
        continue;
      }
      if (a->ToString() > b->ToString()) {
        std::swap(a, b);
      }
      auto or_inv = std::make_shared<OrInvariant>(a, b);
      std::string rule_str = or_inv->ToString();
      if (emitted_or_rules.count(rule_str) > 0) {
        continue;
      }
      bool holds_for_all = true;
      for (const auto& deal : deals_) {
        if (!or_inv->HoldsOnDeal(deal, formatter_.hand1_label(), formatter_.hand2_label())) {
          holds_for_all = false;
          break;
        }
      }
      if (holds_for_all) {
        emitted_or_rules.insert(rule_str);
        invariants.push_back(or_inv);
      }
    }
  }
}

void HonorPossessionClassifier::AddAndInvariants(
    const std::vector<std::shared_ptr<HonorPossessionInvariant>>& all_candidates,
    std::vector<std::shared_ptr<UnknownCardsInvariant>>& invariants) const {
  std::set<std::string> emitted_and_rules;
  size_t num_all = all_candidates.size();
  for (size_t i = 0; i < num_all; ++i) {
    for (size_t j = i + 1; j < num_all; ++j) {
      auto a = all_candidates[i];
      auto b = all_candidates[j];
      if (a->ToString() == b->ToString()) {
        continue;
      }
      if (a->ToString() > b->ToString()) {
        std::swap(a, b);
      }
      auto and_inv = std::make_shared<AndInvariant>(a, b);
      std::string rule_str = and_inv->ToString();
      if (emitted_and_rules.count(rule_str) > 0) {
        continue;
      }
      bool holds_for_all = true;
      for (const auto& deal : deals_) {
        if (!and_inv->HoldsOnDeal(deal, formatter_.hand1_label(), formatter_.hand2_label())) {
          holds_for_all = false;
          break;
        }
      }
      if (holds_for_all) {
        emitted_and_rules.insert(rule_str);
        invariants.push_back(and_inv);
      }
    }
  }
}

std::vector<std::string> HonorPossessionClassifier::ExtractRules() const {
  std::vector<std::string> rules;
  for (const auto& inv : ExtractInvariants()) {
    if (inv) {
      rules.push_back(inv->ToString());
    }
  }
  return rules;
}

void CrossSuitConditionalRuleExtractor::ProcessDeal(const PairedDeal& deal) {
  total_deals_++;
  deals_.push_back(deal);
}

std::vector<std::shared_ptr<UnknownCardsInvariant>> CrossSuitConditionalRuleExtractor::ExtractInvariants() const {
  std::vector<std::shared_ptr<UnknownCardsInvariant>> invariants;
  if (total_deals_ == 0 || deals_.empty()) return invariants;

  uint64_t unknown_cards = deals_[0].hand1 | deals_[0].hand2;
  int cards_per_hand = CountCardsInHand(deals_[0].hand1);

  std::vector<CardType> unknown_card_list;
  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    for (int r = 8; r <= 12; ++r) {
      CardType card = Card::GetSuitedCard(suit, r);
      if ((unknown_cards & card) != 0) {
        unknown_card_list.push_back(card);
      }
    }
  }

  std::set<std::string> emitted_rules;

  for (CardType card : unknown_card_list) {
    int hand1_deals = 0;
    int hand2_deals = 0;
    for (const auto& deal : deals_) {
      if ((deal.hand1 & card) != 0) {
        hand1_deals++;
      }
      if ((deal.hand2 & card) != 0) {
        hand2_deals++;
      }
    }

    if (hand1_deals == 0 || hand2_deals == 0) {
      continue;
    }

    for (int s = 0; s < 4; ++s) {
      Suit suit = static_cast<Suit>(s);

      int min_len_X = 999;
      int max_len_X = -1;
      int min_len_notX = 999;
      int max_len_notX = -1;

      for (const auto& deal : deals_) {
        uint64_t handX = ((deal.hand1 & card) != 0) ? deal.hand1 : deal.hand2;
        uint64_t handNotX = ((deal.hand1 & card) != 0) ? deal.hand2 : deal.hand1;

        int lenX = CardsInSuit(handX, suit);
        int lenNotX = CardsInSuit(handNotX, suit);

        min_len_X = std::min(min_len_X, lenX);
        max_len_X = std::max(max_len_X, lenX);
        min_len_notX = std::min(min_len_notX, lenNotX);
        max_len_notX = std::max(max_len_notX, lenNotX);
      }

      if (min_len_X > 0) {
        if (min_len_notX < min_len_X) {
          std::shared_ptr<CrossSuitConditionalInvariant> inv;
          if (min_len_X == max_len_X) {
            inv = std::make_shared<CrossSuitExactSuitLengthInvariant>(card, suit, min_len_X);
          } else {
            inv = std::make_shared<CrossSuitMinSuitLengthInvariant>(card, suit, min_len_X);
          }
          std::string rule = inv->ToString();
          if (emitted_rules.insert(rule).second) {
            invariants.push_back(inv);
          }
        }
      }

      if (min_len_X < max_len_X && max_len_X < cards_per_hand) {
        if (max_len_notX > max_len_X) {
          auto inv = std::make_shared<CrossSuitMaxSuitLengthInvariant>(card, suit, max_len_X);
          std::string rule = inv->ToString();
          if (emitted_rules.insert(rule).second) {
            invariants.push_back(inv);
          }
        }
      }
    }

    for (CardType card2 : unknown_card_list) {
      if (card == card2) continue;

      bool always_co_held = true;
      for (const auto& deal : deals_) {
        uint64_t handX = ((deal.hand1 & card) != 0) ? deal.hand1 : deal.hand2;
        if ((handX & card2) == 0) {
          always_co_held = false;
          break;
        }
      }

      if (always_co_held) {
        auto inv = std::make_shared<CrossSuitCardConditionalInvariant>(card, card2);
        std::string rule = inv->ToString();
        if (emitted_rules.insert(rule).second) {
          invariants.push_back(inv);
        }
      }
    }
  }

  return invariants;
}

std::vector<std::string> CrossSuitConditionalRuleExtractor::ExtractRules() const {
  std::vector<std::string> rules;
  for (const auto& inv : ExtractInvariants()) {
    if (inv) {
      rules.push_back(inv->ToString());
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

bool EvaluateRuleOnDeal(const std::string& rule,
                        const PairedDeal& deal,
                        const std::string& hand1_label,
                        const std::string& hand2_label) {
  auto inv = UnknownCardsInvariant::FromString(rule, hand1_label, hand2_label);
  if (!inv) return false;
  return inv->HoldsOnDeal(deal, hand1_label, hand2_label);
}

bool EvaluateRuleOnDeal(const std::string& rule,
                        const PairedDeal& deal,
                        const HandLabelFormatter& formatter) {
  return EvaluateRuleOnDeal(rule, deal, formatter.FormatHand1(), formatter.FormatHand2());
}

std::vector<std::shared_ptr<UnknownCardsInvariant>> FilterUninterestingInvariants(
    std::vector<std::shared_ptr<UnknownCardsInvariant>> candidate_invariants,
    uint64_t unknown_cards,
    int hand1_size,
    const std::string& hand1_label,
    const std::string& hand2_label,
    int num_bifurcations,
    const ConditionalLine* line) {
  if (candidate_invariants.empty() || unknown_cards == 0) {
    return candidate_invariants;
  }

  std::vector<CardType> card_list;
  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    for (int r = 0; r < 13; ++r) {
      CardType card = Card::GetSuitedCard(suit, r);
      if ((unknown_cards & card) != 0) {
        card_list.push_back(card);
      }
    }
  }

  if (hand1_size <= 0 || static_cast<size_t>(hand1_size) >= card_list.size()) {
    return candidate_invariants;
  }

  std::vector<PairedDeal> bifurcations;
  bifurcations.reserve(num_bifurcations);
  std::mt19937 rng(12345);
  std::vector<CardType> cards = card_list;

  for (int k = 0; k < num_bifurcations; ++k) {
    std::shuffle(cards.begin(), cards.end(), rng);
    uint64_t h1 = 0;
    uint64_t h2 = 0;
    for (int i = 0; i < hand1_size; ++i) {
      h1 |= cards[i];
    }
    for (size_t i = hand1_size; i < cards.size(); ++i) {
      h2 |= cards[i];
    }
    bifurcations.emplace_back(h1, h2);
  }

  bool can_eval_line = (line != nullptr && line->has_sample_board());
  if (can_eval_line) {
    CHECK_GT(line->min_tricks(), 0);
  }
  std::vector<int> deal_results(bifurcations.size(), -1);

  std::vector<std::shared_ptr<UnknownCardsInvariant>> filtered;
  for (const auto& inv : candidate_invariants) {
    if (!inv) continue;
    bool true_for_all_random = true;
    bool works_on_false_hand = false;
    for (size_t b_idx = 0; b_idx < bifurcations.size(); ++b_idx) {
      const auto& deal = bifurcations[b_idx];
      if (!inv->HoldsOnDeal(deal, hand1_label, hand2_label)) {
        true_for_all_random = false;
        if (!can_eval_line) {
          break;
        }
        if (deal_results[b_idx] < 0) {
          deal_results[b_idx] = line->ResultsOnHand(deal);
        }
        if (deal_results[b_idx] >= line->min_tricks()) {
          works_on_false_hand = true;
          break;
        }
      }
    }
    if (!true_for_all_random && !works_on_false_hand) {
      filtered.push_back(inv);
    }
  }
  return filtered;
}

std::vector<std::shared_ptr<UnknownCardsInvariant>> FilterUninterestingInvariants(
    std::vector<std::shared_ptr<UnknownCardsInvariant>> candidate_invariants,
    uint64_t unknown_cards,
    int hand1_size,
    const HandLabelFormatter& formatter,
    int num_bifurcations,
    const ConditionalLine* line) {
  return FilterUninterestingInvariants(
      candidate_invariants, unknown_cards, hand1_size,
      formatter.FormatHand1(), formatter.FormatHand2(),
      num_bifurcations, line);
}

std::vector<std::string> FilterUninterestingRules(
    const std::vector<std::string>& candidate_rules,
    uint64_t unknown_cards,
    int hand1_size,
    const std::string& hand1_label,
    const std::string& hand2_label,
    int num_bifurcations,
    const ConditionalLine* line) {
  std::vector<std::shared_ptr<UnknownCardsInvariant>> candidates;
  for (const auto& rule : candidate_rules) {
    if (auto inv = UnknownCardsInvariant::FromString(rule, hand1_label, hand2_label)) {
      candidates.push_back(inv);
    }
  }
  auto filtered = FilterUninterestingInvariants(
      candidates, unknown_cards, hand1_size, hand1_label, hand2_label,
      num_bifurcations, line);
  std::vector<std::string> result;
  for (const auto& inv : filtered) {
    if (inv) {
      result.push_back(inv->ToString());
    }
  }
  return result;
}

std::vector<std::string> FilterUninterestingRules(
    const std::vector<std::string>& candidate_rules,
    uint64_t unknown_cards,
    int hand1_size,
    const HandLabelFormatter& formatter,
    int num_bifurcations,
    const ConditionalLine* line) {
  return FilterUninterestingRules(
      candidate_rules, unknown_cards, hand1_size,
      formatter.FormatHand1(), formatter.FormatHand2(),
      num_bifurcations, line);
}

CrossHandSummaryReport CrossHandDealSetSummarizer::Summarize(
    const std::vector<PairedDeal>& deals,
    const HandLabelFormatter& formatter,
    const ConditionalLine* line) const {
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

  JointSuitRuleExtractor suit_extractor(formatter);
  HonorPossessionClassifier honor_classifier(formatter);
  CrossSuitConditionalRuleExtractor conditional_extractor(formatter);

  for (const auto& deal : deals) {
    suit_extractor.ProcessDeal(deal);
    honor_classifier.ProcessDeal(deal);
    conditional_extractor.ProcessDeal(deal);
  }

  report.suit_bounds = suit_extractor.GetSuitBounds();
  report.honor_stats = honor_classifier.GetHonorStats();

  auto suit_invariants = suit_extractor.ExtractInvariants();
  auto conditional_invariants = conditional_extractor.ExtractInvariants();
  auto honor_invariants = honor_classifier.ExtractInvariants();

  std::vector<std::shared_ptr<UnknownCardsInvariant>> candidate_invariants;
  candidate_invariants.insert(candidate_invariants.end(), suit_invariants.begin(), suit_invariants.end());
  candidate_invariants.insert(candidate_invariants.end(), conditional_invariants.begin(), conditional_invariants.end());
  candidate_invariants.insert(candidate_invariants.end(), honor_invariants.begin(), honor_invariants.end());

  auto filtered_invariants = FilterUninterestingInvariants(
      candidate_invariants, report.unknown_cards, report.cards_per_hand,
      formatter, 200, line);

  report.invariant_rules.clear();
  for (const auto& inv : filtered_invariants) {
    if (inv) {
      report.invariant_rules.push_back(inv->ToString());
    }
  }

  return report;
}

CrossHandSummaryReport CrossHandDealSetSummarizer::Summarize(
    const std::vector<PairedDeal>& deals,
    const std::string& hand1_label,
    const std::string& hand2_label,
    const ConditionalLine* line) const {
  return Summarize(deals, HandLabelFormatter(hand1_label, hand2_label), line);
}
