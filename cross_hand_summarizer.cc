#include "cross_hand_summarizer.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <stdexcept>

#include "conditional_line.h"
#include "glog/logging.h"
#include "unknown_cards_invariant.h"

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
    shortest_counts_[suit][std::min(len1, len2)]++;
  }
}

std::vector<std::shared_ptr<UnknownCardsInvariant>>
JointSuitRuleExtractor::ExtractInvariants() const {
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

  std::string l1 = formatter_.FormatHand1();
  std::string l2 = formatter_.FormatHand2();

  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    auto it = suit_bounds_.find(suit);
    if (it == suit_bounds_.end()) continue;

    const auto& bounds = it->second;

    if (bounds.max_combined == 0 ||
        bounds.max_combined == total_unknown_cards) {
      continue;
    }

    if (bounds.min_hand1 == bounds.max_hand1) {
      invariants.push_back(std::make_shared<ExactSuitLengthInvariant>(
          InvariantHandChoice::kHand1, suit, bounds.min_hand1, l1));
    } else if (bounds.min_hand1 > 0 ||
               bounds.max_hand1 < bounds.max_combined) {
      invariants.push_back(std::make_shared<SuitLengthRangeInvariant>(
          InvariantHandChoice::kHand1, suit, bounds.min_hand1,
          bounds.max_hand1, l1));
    }

    if (bounds.min_hand2 == bounds.max_hand2) {
      invariants.push_back(std::make_shared<ExactSuitLengthInvariant>(
          InvariantHandChoice::kHand2, suit, bounds.min_hand2, l2));
    } else if (bounds.min_hand2 > 0 ||
               bounds.max_hand2 < bounds.max_combined) {
      invariants.push_back(std::make_shared<SuitLengthRangeInvariant>(
          InvariantHandChoice::kHand2, suit, bounds.min_hand2,
          bounds.max_hand2, l2));
    }

    int max_either = std::max(bounds.max_hand1, bounds.max_hand2);
    if (max_either < bounds.max_combined) {
      invariants.push_back(
          std::make_shared<MaxSuitLengthInvariant>(suit, max_either));
    }

    int max_shortest = bounds.max_combined / 2;
    auto sc_it = shortest_counts_.find(suit);
    if (max_shortest >= 1 && sc_it != shortest_counts_.end()) {
      const auto& sc = sc_it->second;
      int best_ge = -1;
      for (int k = max_shortest; k >= 1; --k) {
        int count_ge = 0;
        for (const auto& [len, cnt] : sc) {
          if (len >= k) count_ge += cnt;
        }
        if (count_ge * 10 >= 9 * total_deals_) {
          best_ge = k;
          break;
        }
      }
      if (best_ge >= 1) {
        invariants.push_back(std::make_shared<ShortestLengthInvariant>(
            suit, ShortestLengthInvariant::Comparison::kAtLeast, best_ge));
      }
      int strict_min_shortest = bounds.max_combined - max_either;
      if (strict_min_shortest >= 1 && strict_min_shortest != best_ge) {
        invariants.push_back(std::make_shared<ShortestLengthInvariant>(
            suit, ShortestLengthInvariant::Comparison::kAtLeast,
            strict_min_shortest));
      }

      for (int k = 0; k < max_shortest; ++k) {
        int count_le = 0;
        for (const auto& [len, cnt] : sc) {
          if (len <= k) count_le += cnt;
        }
        if (count_le * 10 >= 9 * total_deals_) {
          invariants.push_back(std::make_shared<ShortestLengthInvariant>(
              suit, ShortestLengthInvariant::Comparison::kAtMost, k));
          break;
        }
      }
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

    uint64_t suit_honor_mask = 0x1F00ULL << (s * 16);
    bool h1_has_honor = (deal.hand1 & suit_honor_mask) != 0;
    bool h2_has_honor = (deal.hand2 & suit_honor_mask) != 0;

    auto& suit_stats = suit_honor_stats_[suit];
    if (h1_has_honor && h2_has_honor) {
      suit_stats.both_count++;
    } else if (h1_has_honor) {
      suit_stats.hand1_count++;
    } else if (h2_has_honor) {
      suit_stats.hand2_count++;
    } else {
      suit_stats.neither_count++;
    }
  }
}

static bool HoldsOnNearAllDeals(int holds_count, int total_deals) {
  return total_deals > 0 && holds_count * 20 >= 19 * total_deals;
}

std::vector<std::shared_ptr<UnknownCardsInvariant>>
HonorPossessionClassifier::ExtractInvariants() const {
  std::vector<std::shared_ptr<UnknownCardsInvariant>> invariants;
  if (total_deals_ == 0) return invariants;

  std::vector<std::shared_ptr<HonorPossessionInvariant>> non_100_candidates;
  std::vector<std::shared_ptr<HonorPossessionInvariant>> all_candidates;

  std::string l1_single = formatter_.FormatHand1();
  std::string l2_single = formatter_.FormatHand2();
  std::string l1_composite = formatter_.FormatCompositeHand1();
  std::string l2_composite = formatter_.FormatCompositeHand2();

  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    auto suit_it = honor_stats_.find(suit);
    if (suit_it == honor_stats_.end()) continue;

    bool h1_has_specific_honor_rule = false;
    bool h2_has_specific_honor_rule = false;

    for (int r = 8; r <= 12; ++r) {
      auto r_it = suit_it->second.find(r);
      if (r_it == suit_it->second.end()) continue;

      const auto& stats = r_it->second;
      CardType card = Card::GetSuitedCard(suit, r);

      if (HoldsOnNearAllDeals(stats.hand1_count, total_deals_)) {
        invariants.push_back(std::make_shared<HonorPossessionInvariant>(
            HonorPossessionTargetHand::kHand1, card, l1_single));
        all_candidates.push_back(std::make_shared<HonorPossessionInvariant>(
            HonorPossessionTargetHand::kHand1, card, l1_composite));
        h1_has_specific_honor_rule = true;
      } else if (HoldsOnNearAllDeals(stats.hand2_count, total_deals_)) {
        invariants.push_back(std::make_shared<HonorPossessionInvariant>(
            HonorPossessionTargetHand::kHand2, card, l2_single));
        all_candidates.push_back(std::make_shared<HonorPossessionInvariant>(
            HonorPossessionTargetHand::kHand2, card, l2_composite));
        h2_has_specific_honor_rule = true;
      } else if (HoldsOnNearAllDeals(stats.both_count, total_deals_)) {
        invariants.push_back(std::make_shared<HonorPossessionInvariant>(
            HonorPossessionTargetHand::kBoth, card, "both hands"));
        h1_has_specific_honor_rule = true;
        h2_has_specific_honor_rule = true;
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

    auto suit_stats_it = suit_honor_stats_.find(suit);
    if (suit_stats_it != suit_honor_stats_.end()) {
      const auto& stats = suit_stats_it->second;
      int h1_total = stats.hand1_count + stats.both_count;
      int h2_total = stats.hand2_count + stats.both_count;
      bool h1_holds = HoldsOnNearAllDeals(h1_total, total_deals_);
      bool h2_holds = HoldsOnNearAllDeals(h2_total, total_deals_);
      bool both_holds = HoldsOnNearAllDeals(stats.both_count, total_deals_);

      if (both_holds && !h1_has_specific_honor_rule &&
          !h2_has_specific_honor_rule) {
        invariants.push_back(std::make_shared<SuitHonorVariableInvariant>(
            HonorPossessionTargetHand::kBoth, suit, "both hands"));
      } else {
        if (h1_holds && !h1_has_specific_honor_rule) {
          invariants.push_back(std::make_shared<SuitHonorVariableInvariant>(
              HonorPossessionTargetHand::kHand1, suit, l1_single));
        }
        if (h2_holds && !h2_has_specific_honor_rule) {
          invariants.push_back(std::make_shared<SuitHonorVariableInvariant>(
              HonorPossessionTargetHand::kHand2, suit, l2_single));
        }
      }
    }
  }

  AddOrInvariants(non_100_candidates, invariants);
  AddAndInvariants(all_candidates, invariants);

  return invariants;
}

void HonorPossessionClassifier::AddOrInvariants(
    const std::vector<std::shared_ptr<HonorPossessionInvariant>>&
        non_100_candidates,
    std::vector<std::shared_ptr<UnknownCardsInvariant>>& invariants) const {
  std::set<std::pair<Suit, HonorPossessionTargetHand>> covered_by_shv;
  for (const auto& inv : invariants) {
    if (auto shv =
            std::dynamic_pointer_cast<SuitHonorVariableInvariant>(inv)) {
      covered_by_shv.insert({shv->suit(), shv->target_hand()});
    }
  }

  std::set<std::string> emitted_or_rules;
  size_t num_non_100 = non_100_candidates.size();
  for (size_t i = 0; i < num_non_100; ++i) {
    for (size_t j = i + 1; j < num_non_100; ++j) {
      auto a = non_100_candidates[i];
      auto b = non_100_candidates[j];
      if (a->ToString() == b->ToString()) {
        continue;
      }
      Suit s1 = Card::DecodeCard(a->card()).first;
      Suit s2 = Card::DecodeCard(b->card()).first;
      if (s1 == s2 && a->target_hand() == b->target_hand()) {
        Suit s = s1;
        HonorPossessionTargetHand target = a->target_hand();
        if (covered_by_shv.count({s, target}) > 0 ||
            covered_by_shv.count({s, HonorPossessionTargetHand::kBoth}) > 0) {
          continue;
        }
      }
      if (a->ToString() > b->ToString()) {
        std::swap(a, b);
      }
      auto or_inv = std::make_shared<OrInvariant>(a, b);
      std::string rule_str = or_inv->ToString();
      if (emitted_or_rules.count(rule_str) > 0) {
        continue;
      }
      int holds_count = 0;
      for (const auto& deal : deals_) {
        if (or_inv->HoldsOnDeal(deal)) {
          holds_count++;
        }
      }
      if (HoldsOnNearAllDeals(holds_count, total_deals_)) {
        emitted_or_rules.insert(rule_str);
        invariants.push_back(or_inv);
      }
    }
  }
}

void HonorPossessionClassifier::AddAndInvariants(
    const std::vector<std::shared_ptr<HonorPossessionInvariant>>&
        all_candidates,
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
      int holds_count = 0;
      for (const auto& deal : deals_) {
        if (and_inv->HoldsOnDeal(deal)) {
          holds_count++;
        }
      }
      if (HoldsOnNearAllDeals(holds_count, total_deals_)) {
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

std::vector<std::shared_ptr<UnknownCardsInvariant>>
CrossSuitConditionalRuleExtractor::ExtractInvariants() const {
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
        uint64_t handNotX =
            ((deal.hand1 & card) != 0) ? deal.hand2 : deal.hand1;

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
            inv = std::make_shared<CrossSuitExactSuitLengthInvariant>(
                card, suit, min_len_X);
          } else {
            inv = std::make_shared<CrossSuitMinSuitLengthInvariant>(
                card, suit, min_len_X);
          }
          std::string rule = inv->ToString();
          if (emitted_rules.insert(rule).second) {
            invariants.push_back(inv);
          }
        }
      }

      if (min_len_X < max_len_X && max_len_X < cards_per_hand) {
        if (max_len_notX > max_len_X) {
          auto inv = std::make_shared<CrossSuitMaxSuitLengthInvariant>(
              card, suit, max_len_X);
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
        auto inv =
            std::make_shared<CrossSuitCardConditionalInvariant>(card, card2);
        std::string rule = inv->ToString();
        if (emitted_rules.insert(rule).second) {
          invariants.push_back(inv);
        }
      }
    }
  }

  return invariants;
}

std::vector<std::string> CrossSuitConditionalRuleExtractor::ExtractRules()
    const {
  std::vector<std::string> rules;
  for (const auto& inv : ExtractInvariants()) {
    if (inv) {
      rules.push_back(inv->ToString());
    }
  }
  return rules;
}

int InvariantStats::holds_percent() const {
  return static_cast<int>(std::lround(holds_rate() * 100.0));
}

int InvariantStats::success_when_holds_percent() const {
  return static_cast<int>(std::lround(success_when_holds_rate() * 100.0));
}

std::string InvariantStats::FormatRuleWithStats() const {
  if (total_bifurcations <= 0) {
    return rule;
  }
  std::string out =
      rule + " (holds: " + std::to_string(holds_percent()) + "%";
  if (has_line_stats) {
    out += (is_failure_mode ? ", failures when holds: "
                            : ", success when holds: ") +
           std::to_string(success_when_holds_percent()) + "%";
  }
  out += ")";
  return out;
}

std::string CrossHandSummaryReport::ToString() const {
  std::string out = "Cross-Hand Deal Set Summary Report\n";
  out += "Total Deals: " + std::to_string(total_deals) + "\n";
  out += "Invariant Rules for " + std::to_string(cards_per_hand) +
         " card ending:\n";
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
  return EvaluateRuleOnDeal(
      rule, deal, formatter.FormatHand1(), formatter.FormatHand2());
}

static std::vector<PairedDeal> GenerateRandomBifurcations(
    uint64_t unknown_cards, int hand1_size, int num_bifurcations) {
  std::vector<PairedDeal> bifurcations;
  if (unknown_cards == 0 || hand1_size <= 0 || num_bifurcations <= 0) {
    return bifurcations;
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

  if (static_cast<size_t>(hand1_size) >= card_list.size()) {
    return bifurcations;
  }

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
  return bifurcations;
}

static void SuppressRedundantSuitHonorVariableInvariants(
    std::vector<InvariantStats>& stats_list) {
  std::set<std::pair<Suit, HonorPossessionTargetHand>> covered_suits;
  std::set<std::pair<Suit, HonorPossessionTargetHand>> covered_by_shv;

  for (const auto& st : stats_list) {
    if (auto hp = std::dynamic_pointer_cast<HonorPossessionInvariant>(
            st.invariant)) {
      Suit s = Card::DecodeCard(hp->card()).first;
      covered_suits.insert({s, hp->target_hand()});
    } else if (auto shv =
                   std::dynamic_pointer_cast<SuitHonorVariableInvariant>(
                       st.invariant)) {
      covered_by_shv.insert({shv->suit(), shv->target_hand()});
    }
  }

  if (!covered_suits.empty() || !covered_by_shv.empty()) {
    std::vector<InvariantStats> pruned;
    pruned.reserve(stats_list.size());
    for (auto& st : stats_list) {
      if (auto shv = std::dynamic_pointer_cast<SuitHonorVariableInvariant>(
              st.invariant)) {
        Suit s = shv->suit();
        HonorPossessionTargetHand target = shv->target_hand();
        if (target == HonorPossessionTargetHand::kHand1) {
          if (covered_suits.count(
                  {s, HonorPossessionTargetHand::kHand1}) > 0 ||
              covered_suits.count(
                  {s, HonorPossessionTargetHand::kBoth}) > 0 ||
              covered_by_shv.count(
                  {s, HonorPossessionTargetHand::kBoth}) > 0) {
            continue;
          }
        } else if (target == HonorPossessionTargetHand::kHand2) {
          if (covered_suits.count(
                  {s, HonorPossessionTargetHand::kHand2}) > 0 ||
              covered_suits.count(
                  {s, HonorPossessionTargetHand::kBoth}) > 0 ||
              covered_by_shv.count(
                  {s, HonorPossessionTargetHand::kBoth}) > 0) {
            continue;
          }
        } else if (target == HonorPossessionTargetHand::kBoth) {
          bool h1_cov =
              covered_suits.count(
                  {s, HonorPossessionTargetHand::kHand1}) > 0 ||
              covered_suits.count(
                  {s, HonorPossessionTargetHand::kBoth}) > 0 ||
              covered_by_shv.count(
                  {s, HonorPossessionTargetHand::kHand1}) > 0;
          bool h2_cov =
              covered_suits.count(
                  {s, HonorPossessionTargetHand::kHand2}) > 0 ||
              covered_suits.count(
                  {s, HonorPossessionTargetHand::kBoth}) > 0 ||
              covered_by_shv.count(
                  {s, HonorPossessionTargetHand::kHand2}) > 0;
          if (covered_suits.count(
                  {s, HonorPossessionTargetHand::kBoth}) > 0 ||
              (h1_cov && h2_cov)) {
            continue;
          }
        }
      } else if (auto or_inv = std::dynamic_pointer_cast<OrInvariant>(
                     st.invariant)) {
        auto hp1 = std::dynamic_pointer_cast<HonorPossessionInvariant>(
            or_inv->left());
        auto hp2 = std::dynamic_pointer_cast<HonorPossessionInvariant>(
            or_inv->right());
        if (hp1 && hp2) {
          Suit s1 = Card::DecodeCard(hp1->card()).first;
          Suit s2 = Card::DecodeCard(hp2->card()).first;
          if (s1 == s2 && hp1->target_hand() == hp2->target_hand()) {
            Suit s = s1;
            HonorPossessionTargetHand target = hp1->target_hand();
            if (covered_by_shv.count({s, target}) > 0 ||
                covered_by_shv.count(
                    {s, HonorPossessionTargetHand::kBoth}) > 0) {
              continue;
            }
          }
        }
      }
      pruned.push_back(std::move(st));
    }
    stats_list = std::move(pruned);
  }
}

static std::vector<InvariantStats> DeduplicateEquivalentInvariants(
    std::vector<InvariantStats> stats_list,
    const std::vector<PairedDeal>& eval_deals,
    const std::string& hand1_label,
    const std::string& hand2_label) {
  if (stats_list.size() <= 1 || eval_deals.empty()) {
    return stats_list;
  }

  auto is_better_rep = [](const InvariantStats& candidate,
                          const InvariantStats& current) {
    if (candidate.rule.size() != current.rule.size()) {
      return candidate.rule.size() < current.rule.size();
    }
    return candidate.rule < current.rule;
  };

  std::vector<std::vector<bool>> signatures;
  std::vector<InvariantStats> deduped;
  signatures.reserve(stats_list.size());
  deduped.reserve(stats_list.size());

  for (auto& st : stats_list) {
    if (!st.invariant) continue;
    std::vector<bool> sig(eval_deals.size(), false);
    for (size_t i = 0; i < eval_deals.size(); ++i) {
      sig[i] =
          st.invariant->HoldsOnDeal(eval_deals[i], hand1_label, hand2_label);
    }

    auto it = std::find(signatures.begin(), signatures.end(), sig);
    if (it == signatures.end()) {
      signatures.push_back(std::move(sig));
      deduped.push_back(std::move(st));
    } else {
      size_t idx = static_cast<size_t>(std::distance(signatures.begin(), it));
      if (is_better_rep(st, deduped[idx])) {
        deduped[idx] = std::move(st);
      }
    }
  }

  return deduped;
}

std::vector<InvariantStats> EvaluateCandidateInvariants(
    std::vector<std::shared_ptr<UnknownCardsInvariant>> candidate_invariants,
    uint64_t unknown_cards,
    int hand1_size,
    const std::string& hand1_label,
    const std::string& hand2_label,
    int num_bifurcations,
    const ConditionalLine* line,
    bool is_failure_mode) {
  std::vector<InvariantStats> stats_list;
  if (candidate_invariants.empty() || unknown_cards == 0) {
    for (const auto& inv : candidate_invariants) {
      if (!inv) continue;
      InvariantStats st;
      st.invariant = inv;
      st.rule = inv->ToString();
      st.is_failure_mode = is_failure_mode;
      stats_list.push_back(std::move(st));
    }
    SuppressRedundantSuitHonorVariableInvariants(stats_list);
    return stats_list;
  }

  std::vector<PairedDeal> bifurcations =
      GenerateRandomBifurcations(unknown_cards, hand1_size, num_bifurcations);
  if (bifurcations.empty()) {
    for (const auto& inv : candidate_invariants) {
      if (!inv) continue;
      InvariantStats st;
      st.invariant = inv;
      st.rule = inv->ToString();
      st.is_failure_mode = is_failure_mode;
      stats_list.push_back(std::move(st));
    }
    SuppressRedundantSuitHonorVariableInvariants(stats_list);
    return stats_list;
  }

  bool can_eval_line = (line != nullptr && line->has_sample_board());
  if (can_eval_line) {
    CHECK_GT(line->min_tricks(), 0);
  }
  std::vector<int> deal_results(bifurcations.size(), -1);

  for (const auto& inv : candidate_invariants) {
    if (!inv) continue;
    std::vector<size_t> holding_indices;
    holding_indices.reserve(bifurcations.size());
    for (size_t b_idx = 0; b_idx < bifurcations.size(); ++b_idx) {
      if (inv->HoldsOnDeal(bifurcations[b_idx], hand1_label, hand2_label)) {
        holding_indices.push_back(b_idx);
      }
    }

    int holds_count = static_cast<int>(holding_indices.size());
    int total_bif = static_cast<int>(bifurcations.size());

    InvariantStats st;
    st.invariant = inv;
    st.rule = inv->ToString();
    st.holds_count = holds_count;
    st.total_bifurcations = total_bif;
    st.has_line_stats = can_eval_line;
    st.is_failure_mode = is_failure_mode;

    // Discard "boring" invariants that hold on >= 95% of random bifurcations.
    if (holds_count * 20 >= 19 * total_bif || st.holds_percent() >= 95) {
      continue;
    }

    if (can_eval_line) {
      int match_count = 0;
      for (size_t b_idx : holding_indices) {
        if (deal_results[b_idx] < 0) {
          deal_results[b_idx] = line->ResultsOnHand(bifurcations[b_idx]);
        }
        bool succeeded = (deal_results[b_idx] >= line->min_tricks());
        if (is_failure_mode ? !succeeded : succeeded) {
          match_count++;
        }
      }
      st.success_when_holds_count = match_count;
      if (match_count * 2 <= holds_count ||
          st.success_when_holds_percent() <= 50) {
        continue;
      }
    }

    stats_list.push_back(std::move(st));
  }

  if (can_eval_line) {
    std::stable_sort(stats_list.begin(), stats_list.end(),
                     [](const InvariantStats& a, const InvariantStats& b) {
                       double sa = a.success_when_holds_rate();
                       double sb = b.success_when_holds_rate();
                       if (std::abs(sa - sb) > 1e-9) {
                         return sa > sb;
                       }
                       double ha = a.holds_rate();
                       double hb = b.holds_rate();
                       if (std::abs(ha - hb) > 1e-9) {
                         return ha > hb;
                       }
                       return false;
                     });
  }

  SuppressRedundantSuitHonorVariableInvariants(stats_list);

  return DeduplicateEquivalentInvariants(
      std::move(stats_list), bifurcations, hand1_label, hand2_label);
}

std::vector<InvariantStats> EvaluateCandidateInvariants(
    std::vector<std::shared_ptr<UnknownCardsInvariant>> candidate_invariants,
    uint64_t unknown_cards,
    int hand1_size,
    const HandLabelFormatter& formatter,
    int num_bifurcations,
    const ConditionalLine* line,
    bool is_failure_mode) {
  return EvaluateCandidateInvariants(
      std::move(candidate_invariants), unknown_cards, hand1_size,
      formatter.FormatHand1(), formatter.FormatHand2(), num_bifurcations, line,
      is_failure_mode);
}

std::vector<std::shared_ptr<UnknownCardsInvariant>>
FilterUninterestingInvariants(
    std::vector<std::shared_ptr<UnknownCardsInvariant>> candidate_invariants,
    uint64_t unknown_cards,
    int hand1_size,
    const std::string& hand1_label,
    const std::string& hand2_label,
    int num_bifurcations,
    const ConditionalLine* line) {
  auto stats_list = EvaluateCandidateInvariants(
      std::move(candidate_invariants), unknown_cards, hand1_size, hand1_label,
      hand2_label, num_bifurcations, line);
  std::vector<std::shared_ptr<UnknownCardsInvariant>> filtered;
  filtered.reserve(stats_list.size());
  for (const auto& st : stats_list) {
    if (st.invariant) {
      filtered.push_back(st.invariant);
    }
  }
  return filtered;
}

std::vector<std::shared_ptr<UnknownCardsInvariant>>
FilterUninterestingInvariants(
    std::vector<std::shared_ptr<UnknownCardsInvariant>> candidate_invariants,
    uint64_t unknown_cards,
    int hand1_size,
    const HandLabelFormatter& formatter,
    int num_bifurcations,
    const ConditionalLine* line) {
  return FilterUninterestingInvariants(
      std::move(candidate_invariants), unknown_cards, hand1_size,
      formatter.FormatHand1(), formatter.FormatHand2(), num_bifurcations, line);
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
    if (auto inv =
            UnknownCardsInvariant::FromString(rule, hand1_label, hand2_label)) {
      candidates.push_back(inv);
    }
  }
  auto stats_list = EvaluateCandidateInvariants(
      candidates, unknown_cards, hand1_size, hand1_label, hand2_label,
      num_bifurcations, line);
  std::vector<std::string> result;
  for (const auto& st : stats_list) {
    result.push_back(st.FormatRuleWithStats());
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
      candidate_rules, unknown_cards, hand1_size, formatter.FormatHand1(),
      formatter.FormatHand2(), num_bifurcations, line);
}

static std::vector<std::shared_ptr<UnknownCardsInvariant>>
ExtractAllCandidateInvariants(const std::vector<PairedDeal>& deals,
                              const HandLabelFormatter& formatter) {
  JointSuitRuleExtractor suit_extractor(formatter);
  HonorPossessionClassifier honor_classifier(formatter);
  CrossSuitConditionalRuleExtractor conditional_extractor;
  for (const auto& deal : deals) {
    suit_extractor.ProcessDeal(deal);
    honor_classifier.ProcessDeal(deal);
    conditional_extractor.ProcessDeal(deal);
  }
  auto suit_invariants = suit_extractor.ExtractInvariants();
  auto conditional_invariants = conditional_extractor.ExtractInvariants();
  auto honor_invariants = honor_classifier.ExtractInvariants();

  std::vector<std::shared_ptr<UnknownCardsInvariant>> candidates;
  candidates.insert(candidates.end(), suit_invariants.begin(),
                    suit_invariants.end());
  candidates.insert(candidates.end(), conditional_invariants.begin(),
                    conditional_invariants.end());
  candidates.insert(candidates.end(), honor_invariants.begin(),
                    honor_invariants.end());
  return candidates;
}

CrossHandSummaryReport CrossHandDealSetSummarizer::Summarize(
    const std::vector<PairedDeal>& deals,
    const HandLabelFormatter& formatter,
    const ConditionalLine* line,
    const std::vector<PairedDeal>* opposing_deals,
    bool is_failure_mode) const {
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
          "All PairedDeals in a deal set must have the identical overall set "
          "of cards.");
    }
  }

  report.unknown_cards = expected_overall;
  report.cards_per_hand = CountCardsInHand(deals[0].hand1);

  JointSuitRuleExtractor suit_extractor(formatter);
  HonorPossessionClassifier honor_classifier(formatter);
  CrossSuitConditionalRuleExtractor conditional_extractor;

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
  candidate_invariants.insert(candidate_invariants.end(),
                              suit_invariants.begin(), suit_invariants.end());
  candidate_invariants.insert(candidate_invariants.end(),
                              conditional_invariants.begin(),
                              conditional_invariants.end());
  candidate_invariants.insert(candidate_invariants.end(),
                              honor_invariants.begin(), honor_invariants.end());

  std::string fmt_h1 = formatter.FormatHand1();
  std::string fmt_h2 = formatter.FormatHand2();

  if (opposing_deals != nullptr) {
    if (!opposing_deals->empty()) {
      std::set<std::string> seen_rules;
      for (const auto& inv : candidate_invariants) {
        if (inv) {
          seen_rules.insert(inv->ToString());
        }
      }
      auto opposing_invariants =
          ExtractAllCandidateInvariants(*opposing_deals, formatter);
      for (const auto& op_inv : opposing_invariants) {
        if (!op_inv) continue;
        auto inv_op = op_inv->Invert(report.unknown_cards,
                                     report.cards_per_hand, fmt_h1, fmt_h2);
        if (inv_op && seen_rules.insert(inv_op->ToString()).second) {
          candidate_invariants.push_back(inv_op);
        }
      }
    }

    int total_sim =
        static_cast<int>(deals.size() + opposing_deals->size());
    std::vector<InvariantStats> sim_stats;
    for (const auto& inv : candidate_invariants) {
      if (!inv) continue;
      int holds_in_deals = 0;
      for (const auto& d : deals) {
        if (inv->HoldsOnDeal(d, fmt_h1, fmt_h2)) {
          holds_in_deals++;
        }
      }
      int holds_in_opposing = 0;
      for (const auto& d : *opposing_deals) {
        if (inv->HoldsOnDeal(d, fmt_h1, fmt_h2)) {
          holds_in_opposing++;
        }
      }
      // Require the invariant to hold on at least 90% of the target deal set,
      // or for ShortestLengthInvariant to hold on at least 75% of the target
      // deal set while its negation holds on at least 90% of opposing_deals.
      bool holds_on_most_deals =
          holds_in_deals * 10 >= 9 * static_cast<int>(deals.size());
      bool sufficient_against_opposing =
          std::dynamic_pointer_cast<ShortestLengthInvariant>(inv) != nullptr &&
          !opposing_deals->empty() &&
          holds_in_deals * 4 >= 3 * static_cast<int>(deals.size()) &&
          holds_in_opposing * 10 <= static_cast<int>(opposing_deals->size());
      if (!holds_on_most_deals && !sufficient_against_opposing) {
        continue;
      }

      int holds_count = holds_in_deals + holds_in_opposing;

      InvariantStats st;
      st.invariant = inv;
      st.rule = inv->ToString();
      st.holds_count = holds_count;
      st.total_bifurcations = total_sim;
      st.has_line_stats = (line != nullptr || is_failure_mode);
      st.is_failure_mode = is_failure_mode;
      st.success_when_holds_count = holds_in_deals;

      // Discard "boring" invariants that hold on >= 95% of simulated deals.
      if (holds_count * 20 >= 19 * total_sim || st.holds_percent() >= 95) {
        continue;
      }

      // Discard invariants where success/failure_when_holds is <= 50%.
      int success_pct =
          holds_count > 0
              ? static_cast<int>(
                    std::lround(100.0 * holds_in_deals / holds_count))
              : 0;
      if (holds_in_deals * 2 <= holds_count || success_pct <= 50) {
        continue;
      }

      sim_stats.push_back(std::move(st));
    }

    std::stable_sort(sim_stats.begin(), sim_stats.end(),
                     [](const InvariantStats& a, const InvariantStats& b) {
                       double sa = a.holds_count > 0
                                       ? static_cast<double>(
                                             a.success_when_holds_count) /
                                             a.holds_count
                                       : 0.0;
                       double sb = b.holds_count > 0
                                       ? static_cast<double>(
                                             b.success_when_holds_count) /
                                             b.holds_count
                                       : 0.0;
                       if (std::abs(sa - sb) > 1e-9) {
                         return sa > sb;
                       }
                       double ha = a.holds_rate();
                       double hb = b.holds_rate();
                       if (std::abs(ha - hb) > 1e-9) {
                         return ha > hb;
                       }
                       return false;
                     });

    std::vector<PairedDeal> eval_deals = GenerateRandomBifurcations(
        report.unknown_cards, report.cards_per_hand, 200);
    eval_deals.insert(eval_deals.end(), deals.begin(), deals.end());
    eval_deals.insert(eval_deals.end(), opposing_deals->begin(),
                      opposing_deals->end());

    SuppressRedundantSuitHonorVariableInvariants(sim_stats);
    report.invariant_stats = DeduplicateEquivalentInvariants(
        std::move(sim_stats), eval_deals, fmt_h1, fmt_h2);
  } else {
    report.invariant_stats = EvaluateCandidateInvariants(
        candidate_invariants, report.unknown_cards, report.cards_per_hand,
        formatter, 200, line, is_failure_mode);
  }

  report.invariant_rules.clear();
  for (const auto& st : report.invariant_stats) {
    report.invariant_rules.push_back(st.FormatRuleWithStats());
  }

  return report;
}

CrossHandSummaryReport CrossHandDealSetSummarizer::Summarize(
    const std::vector<PairedDeal>& deals,
    const std::string& hand1_label,
    const std::string& hand2_label,
    const ConditionalLine* line,
    const std::vector<PairedDeal>* opposing_deals,
    bool is_failure_mode) const {
  return Summarize(deals, HandLabelFormatter(hand1_label, hand2_label), line,
                   opposing_deals, is_failure_mode);
}

