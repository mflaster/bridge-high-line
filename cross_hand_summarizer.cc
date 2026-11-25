#include "cross_hand_summarizer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include "conditional_line.h"
#include "glog/logging.h"
#include "unknown_cards_invariant.h"

namespace {

// Integer ceiling quantile: holds_count >= ceil((num / den) * total_deals).
// For 9/10 (90%), total_deals < 10 requires 100% agreement.
inline bool MeetsCeilingQuantile(int holds_count, int total_deals, int num,
                                 int den) {
  return total_deals > 0 &&
         static_cast<int64_t>(holds_count) * den >=
             static_cast<int64_t>(num) * total_deals;
}

inline bool IsBetterRule(const std::string& cand, const std::string& cur) {
  return cand.size() != cur.size() ? cand.size() < cur.size() : cand < cur;
}

std::shared_ptr<UnknownCardsInvariant> MakeCanonicalAnd(
    std::shared_ptr<UnknownCardsInvariant> a,
    std::shared_ptr<UnknownCardsInvariant> b) {
  if (a->ToString() > b->ToString()) std::swap(a, b);
  return std::make_shared<AndInvariant>(std::move(a), std::move(b));
}

std::shared_ptr<UnknownCardsInvariant> MakeCanonicalOr(
    std::shared_ptr<UnknownCardsInvariant> a,
    std::shared_ptr<UnknownCardsInvariant> b) {
  if (a->ToString() > b->ToString()) std::swap(a, b);
  return std::make_shared<OrInvariant>(std::move(a), std::move(b));
}

std::shared_ptr<UnknownCardsInvariant> MakeCanonicalOr3(
    std::shared_ptr<UnknownCardsInvariant> a,
    std::shared_ptr<UnknownCardsInvariant> b,
    std::shared_ptr<UnknownCardsInvariant> c) {
  std::array<std::pair<std::string, std::shared_ptr<UnknownCardsInvariant>>, 3>
      v = {{{a->ToString(), std::move(a)},
            {b->ToString(), std::move(b)},
            {c->ToString(), std::move(c)}}};
  std::sort(v.begin(), v.end(),
            [](const auto& x, const auto& y) { return x.first < y.first; });
  return std::make_shared<OrInvariant>(
      std::make_shared<OrInvariant>(std::move(v[0].second),
                                    std::move(v[1].second)),
      std::move(v[2].second));
}

}  // namespace

// --- Extractors Implementation (90%-95% Quantile Catalog Projections) ---

void JointSuitRuleExtractor::ProcessDeal(const PairedDeal& deal) {
  total_deals_++;
  deals_.push_back(deal);
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
  if (total_deals_ == 0 || deals_.empty()) return invariants;

  AtomicPredicateCatalog catalog(deals_, nullptr, formatter_);
  std::set<std::string> emitted;
  auto emit = [&](const std::shared_ptr<UnknownCardsInvariant>& inv) {
    if (inv && emitted.insert(inv->ToString()).second) invariants.push_back(inv);
  };

  int total_unknown_cards = 0;
  for (const auto& [suit, b] : suit_bounds_) total_unknown_cards += b.max_combined;

  const std::pair<int, int> kQuantiles[] = {{19, 20}, {9, 10}};
  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    auto it = suit_bounds_.find(suit);
    if (it == suit_bounds_.end()) continue;
    int max_comb = it->second.max_combined;
    if (max_comb == 0 || max_comb == total_unknown_cards) continue;

    for (auto choice : {InvariantHandChoice::kHand1, InvariantHandChoice::kHand2}) {
      for (auto [qn, qd] : kQuantiles) {
        std::shared_ptr<UnknownCardsInvariant> best_inv;
        int best_w = 999, best_cnt = -1;
        for (const auto& e : catalog.entries()) {
          if (!MeetsCeilingQuantile(e.target_count, total_deals_, qn, qd)) continue;
          if (auto ex = dynamic_cast<const ExactSuitLengthInvariant*>(e.invariant.get())) {
            if (ex->hand_choice() == choice && ex->suit() == suit &&
                (0 < best_w || e.target_count > best_cnt)) {
              best_w = 0;
              best_cnt = e.target_count;
              best_inv = e.invariant;
            }
          } else if (auto rg = dynamic_cast<const SuitLengthRangeInvariant*>(e.invariant.get())) {
            int w = rg->max_len() - rg->min_len();
            if (rg->hand_choice() == choice && rg->suit() == suit &&
                !(rg->min_len() == 0 && rg->max_len() >= max_comb) &&
                (w < best_w || (w == best_w && e.target_count > best_cnt))) {
              best_w = w;
              best_cnt = e.target_count;
              best_inv = e.invariant;
            }
          }
        }
        emit(best_inv);
      }
    }

    for (auto [qn, qd] : kQuantiles) {
      std::shared_ptr<UnknownCardsInvariant> best_max, best_ge, best_le;
      int best_m = 999, ge_k = -1, le_k = 999;
      for (const auto& e : catalog.entries()) {
        if (!MeetsCeilingQuantile(e.target_count, total_deals_, qn, qd)) continue;
        if (auto mx = dynamic_cast<const MaxSuitLengthInvariant*>(e.invariant.get())) {
          if (mx->suit() == suit && mx->max_len() < best_m) {
            best_m = mx->max_len();
            best_max = e.invariant;
          }
        } else if (auto sl = dynamic_cast<const ShortestLengthInvariant*>(e.invariant.get())) {
          if (sl->suit() != suit) continue;
          if (sl->cmp() == ShortestLengthInvariant::Comparison::kAtLeast &&
              sl->length() > ge_k) {
            ge_k = sl->length();
            best_ge = e.invariant;
          } else if (sl->cmp() == ShortestLengthInvariant::Comparison::kAtMost &&
                     sl->length() < le_k) {
            le_k = sl->length();
            best_le = e.invariant;
          }
        }
      }
      emit(best_max);
      emit(best_ge);
      emit(best_le);
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

std::vector<std::shared_ptr<UnknownCardsInvariant>>
HonorPossessionClassifier::ExtractInvariants() const {
  std::vector<std::shared_ptr<UnknownCardsInvariant>> invariants;
  if (total_deals_ == 0 || deals_.empty()) return invariants;

  AtomicPredicateCatalog catalog(deals_, nullptr, formatter_);
  std::vector<const AtomicPredicateEntry*> non_univ, all_honors;
  std::set<std::pair<Suit, HonorPossessionTargetHand>> spec_cov, shv_emitted;

  for (const auto& e : catalog.entries()) {
    if (e.family != AtomicPredicateFamily::kHonorPossession) continue;
    if (MeetsCeilingQuantile(e.target_count, total_deals_, 9, 10)) {
      invariants.push_back(e.invariant);
      spec_cov.insert({e.honor_suit, e.honor_target});
      if (e.honor_target != HonorPossessionTargetHand::kBoth) all_honors.push_back(&e);
    } else if (e.target_count > 0 &&
               e.honor_target != HonorPossessionTargetHand::kBoth) {
      non_univ.push_back(&e);
      all_honors.push_back(&e);
    }
  }

  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    bool h1_sp = spec_cov.count({suit, HonorPossessionTargetHand::kHand1}) ||
                 spec_cov.count({suit, HonorPossessionTargetHand::kBoth});
    bool h2_sp = spec_cov.count({suit, HonorPossessionTargetHand::kHand2}) ||
                 spec_cov.count({suit, HonorPossessionTargetHand::kBoth});
    const AtomicPredicateEntry *shv1 = nullptr, *shv2 = nullptr, *shvb = nullptr;
    for (const auto& e : catalog.entries()) {
      if (e.family != AtomicPredicateFamily::kSuitHonorVariable ||
          e.honor_suit != suit ||
          !MeetsCeilingQuantile(e.target_count, total_deals_, 9, 10)) {
        continue;
      }
      if (e.honor_target == HonorPossessionTargetHand::kHand1) shv1 = &e;
      if (e.honor_target == HonorPossessionTargetHand::kHand2) shv2 = &e;
      if (e.honor_target == HonorPossessionTargetHand::kBoth) shvb = &e;
    }
    if (shvb && !h1_sp && !h2_sp) {
      invariants.push_back(shvb->invariant);
      shv_emitted.insert({suit, HonorPossessionTargetHand::kBoth});
    } else {
      if (shv1 && !h1_sp) {
        invariants.push_back(shv1->invariant);
        shv_emitted.insert({suit, HonorPossessionTargetHand::kHand1});
      }
      if (shv2 && !h2_sp) {
        invariants.push_back(shv2->invariant);
        shv_emitted.insert({suit, HonorPossessionTargetHand::kHand2});
      }
    }
  }

  std::set<std::string> emitted;
  for (size_t i = 0; i < non_univ.size(); ++i) {
    for (size_t j = i + 1; j < non_univ.size(); ++j) {
      const auto *ea = non_univ[i], *eb = non_univ[j];
      if (ea->composite_invariant->ToString() == eb->composite_invariant->ToString()) continue;
      if (ea->honor_suit == eb->honor_suit && ea->honor_target == eb->honor_target &&
          (shv_emitted.count({ea->honor_suit, ea->honor_target}) ||
           shv_emitted.count({ea->honor_suit, HonorPossessionTargetHand::kBoth}))) {
        continue;
      }
      int cnt = ea->target_bits.OrPopCount(eb->target_bits);
      if (cnt > std::max(ea->target_count, eb->target_count) &&
          MeetsCeilingQuantile(cnt, total_deals_, 9, 10)) {
        auto inv = MakeCanonicalOr(ea->composite_invariant, eb->composite_invariant);
        if (emitted.insert(inv->ToString()).second) invariants.push_back(std::move(inv));
      }
    }
  }
  for (size_t i = 0; i < all_honors.size(); ++i) {
    for (size_t j = i + 1; j < all_honors.size(); ++j) {
      const auto *ea = all_honors[i], *eb = all_honors[j];
      if (ea->composite_invariant->ToString() == eb->composite_invariant->ToString()) continue;
      int cnt = ea->target_bits.AndPopCount(eb->target_bits);
      if (MeetsCeilingQuantile(cnt, total_deals_, 9, 10)) {
        auto inv = MakeCanonicalAnd(ea->composite_invariant, eb->composite_invariant);
        if (emitted.insert(inv->ToString()).second) invariants.push_back(std::move(inv));
      }
    }
  }
  return invariants;
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

  AtomicPredicateCatalog catalog(deals_, nullptr, HandLabelFormatter());
  std::set<std::string> emitted;
  auto emit = [&](const std::shared_ptr<UnknownCardsInvariant>& inv) {
    if (inv && emitted.insert(inv->ToString()).second) invariants.push_back(inv);
  };
  auto not_x_holds_q = [&](CardType c, Suit suit, int cmp, int val) {
    int cnt = 0;
    for (const auto& d : deals_) {
      int len = CardsInSuit(((d.hand1 & c) != 0) ? d.hand2 : d.hand1, suit);
      if ((cmp == 0 && len == val) || (cmp > 0 && len >= val) ||
          (cmp < 0 && len <= val)) {
        ++cnt;
      }
    }
    return MeetsCeilingQuantile(cnt, total_deals_, 9, 10);
  };

  std::set<std::pair<CardType, int>> exact_keys;
  for (const auto& e : catalog.entries()) {
    if (!MeetsCeilingQuantile(e.target_count, total_deals_, 9, 10)) continue;
    if (e.family == AtomicPredicateFamily::kCrossSuitCard) {
      emit(e.invariant);
    } else if (auto ex = dynamic_cast<const CrossSuitExactSuitLengthInvariant*>(
                   e.invariant.get())) {
      if (ex->length() > 0 &&
          !not_x_holds_q(ex->condition_card(), ex->target_suit(), 0, ex->length())) {
        exact_keys.insert({ex->condition_card(), static_cast<int>(ex->target_suit())});
        emit(e.invariant);
      }
    }
  }

  for (auto [qn, qd] : {std::pair<int, int>{19, 20}, std::pair<int, int>{9, 10}}) {
    std::map<std::pair<CardType, int>, std::pair<int, std::shared_ptr<UnknownCardsInvariant>>>
        best_min, best_max;
    for (const auto& e : catalog.entries()) {
      if (e.family != AtomicPredicateFamily::kCrossSuitLength ||
          !MeetsCeilingQuantile(e.target_count, total_deals_, qn, qd)) {
        continue;
      }
      if (auto mn = dynamic_cast<const CrossSuitMinSuitLengthInvariant*>(e.invariant.get())) {
        auto key = std::make_pair(mn->condition_card(), static_cast<int>(mn->target_suit()));
        if (!exact_keys.count(key) &&
            !not_x_holds_q(mn->condition_card(), mn->target_suit(), 1, mn->min_len()) &&
            (!best_min.count(key) || mn->min_len() > best_min[key].first)) {
          best_min[key] = {mn->min_len(), e.invariant};
        }
      } else if (auto mx = dynamic_cast<const CrossSuitMaxSuitLengthInvariant*>(
                     e.invariant.get())) {
        auto key = std::make_pair(mx->condition_card(), static_cast<int>(mx->target_suit()));
        if (!exact_keys.count(key) &&
            !not_x_holds_q(mx->condition_card(), mx->target_suit(), -1, mx->max_len()) &&
            (!best_max.count(key) || mx->max_len() < best_max[key].first)) {
          best_max[key] = {mx->max_len(), e.invariant};
        }
      }
    }
    for (const auto& [k, v] : best_min) emit(v.second);
    for (const auto& [k, v] : best_max) emit(v.second);
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

// --- DealBitset, AtomicPredicateCatalog & SetCoverSynthesizer ---

DealBitset::DealBitset(size_t num_bits)
    : num_bits_(num_bits), num_words_((num_bits + 63) / 64) {
  if (num_words_ > kInlineWords) heap_words_.assign(num_words_, 0ULL);
}

void DealBitset::Set(size_t bit_idx) {
  if (bit_idx < num_bits_) mutable_words()[bit_idx >> 6] |= (1ULL << (bit_idx & 63));
}

bool DealBitset::Test(size_t bit_idx) const {
  return bit_idx < num_bits_ &&
         (words()[bit_idx >> 6] & (1ULL << (bit_idx & 63))) != 0;
}

int DealBitset::PopCount() const {
  int total = 0;
  for (size_t i = 0; i < num_words_; ++i) total += PopCount64(words()[i]);
  return total;
}

bool DealBitset::Empty() const {
  for (size_t i = 0; i < num_words_; ++i) {
    if (words()[i] != 0) return false;
  }
  return true;
}

DealBitset DealBitset::operator&(const DealBitset& other) const {
  DealBitset out(std::max(num_bits_, other.num_bits_));
  size_t min_w = std::min(num_words_, other.num_words_);
  for (size_t i = 0; i < min_w; ++i) out.mutable_words()[i] = words()[i] & other.words()[i];
  return out;
}

DealBitset DealBitset::operator|(const DealBitset& other) const {
  DealBitset out(std::max(num_bits_, other.num_bits_));
  for (size_t i = 0; i < num_words_; ++i) out.mutable_words()[i] |= words()[i];
  for (size_t i = 0; i < other.num_words_; ++i) out.mutable_words()[i] |= other.words()[i];
  return out;
}

int DealBitset::AndPopCount(const DealBitset& other) const {
  int cnt = 0;
  size_t min_w = std::min(num_words_, other.num_words_);
  for (size_t i = 0; i < min_w; ++i) cnt += PopCount64(words()[i] & other.words()[i]);
  return cnt;
}

int DealBitset::OrPopCount(const DealBitset& other) const {
  int cnt = 0;
  size_t min_w = std::min(num_words_, other.num_words_);
  for (size_t i = 0; i < min_w; ++i) cnt += PopCount64(words()[i] | other.words()[i]);
  for (size_t i = min_w; i < num_words_; ++i) cnt += PopCount64(words()[i]);
  for (size_t i = min_w; i < other.num_words_; ++i) cnt += PopCount64(other.words()[i]);
  return cnt;
}

int DealBitset::AndNotCount(const DealBitset& other) const {
  int cnt = 0;
  size_t min_w = std::min(num_words_, other.num_words_);
  for (size_t i = 0; i < min_w; ++i) cnt += PopCount64(words()[i] & ~other.words()[i]);
  for (size_t i = min_w; i < num_words_; ++i) cnt += PopCount64(words()[i]);
  return cnt;
}

bool DealBitset::operator==(const DealBitset& other) const {
  if (num_bits_ != other.num_bits_) return false;
  for (size_t i = 0; i < num_words_; ++i) {
    if (words()[i] != other.words()[i]) return false;
  }
  return true;
}

bool DealBitset::operator<(const DealBitset& other) const {
  if (num_bits_ != other.num_bits_) return num_bits_ < other.num_bits_;
  for (size_t i = 0; i < num_words_; ++i) {
    if (words()[i] != other.words()[i]) return words()[i] < other.words()[i];
  }
  return false;
}

uint64_t DealBitset::Hash(uint64_t h) const noexcept {
  h ^= num_bits_ + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
  for (size_t i = 0; i < num_words_; ++i) {
    h ^= words()[i] + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
  }
  return h;
}

namespace {

struct PrecomputedDeal {
  uint64_t h1 = 0, h2 = 0;
  int len1[4] = {0}, len2[4] = {0};
};

std::vector<PrecomputedDeal> BuildDealFeatures(const std::vector<PairedDeal>& deals) {
  std::vector<PrecomputedDeal> out(deals.size());
  for (size_t i = 0; i < deals.size(); ++i) {
    out[i].h1 = deals[i].hand1;
    out[i].h2 = deals[i].hand2;
    for (int s = 0; s < 4; ++s) {
      out[i].len1[s] = CardsInSuit(deals[i].hand1, static_cast<Suit>(s));
      out[i].len2[s] = CardsInSuit(deals[i].hand2, static_cast<Suit>(s));
    }
  }
  return out;
}

template <typename PredFn>
DealBitset EvalBitset(const std::vector<PrecomputedDeal>& feats, PredFn&& pred) {
  DealBitset bits(feats.size());
  uint64_t* w = bits.mutable_words();
  for (size_t i = 0; i < feats.size(); ++i) {
    if (pred(feats[i])) w[i >> 6] |= (1ULL << (i & 63));
  }
  return bits;
}

inline bool StrictlyBeatsPrecision(int tc_a, int oc_a, int tc_b, int oc_b) {
  int64_t da = static_cast<int64_t>(tc_a) + oc_a;
  int64_t db = static_cast<int64_t>(tc_b) + oc_b;
  if (da <= 0) return false;
  if (db <= 0) return tc_a > 0;
  return static_cast<int64_t>(tc_a) * db > static_cast<int64_t>(tc_b) * da;
}

// One-sided hypergeometric upper-tail p-value P(X >= k_succ) when drawing
// k_draws items from N_tot containing K_succ target items.
double HypergeomUpperTailPValue(int k_succ, int k_draws, int K_succ, int N_tot) {
  static const auto kLogFact = []() {
    std::array<double, 2049> t{};
    for (int i = 1; i <= 2048; ++i) t[i] = t[i - 1] + std::log(static_cast<double>(i));
    return t;
  }();
  auto log_fact = [&](int n) {
    return n <= 0 ? 0.0 : (n <= 2048 ? kLogFact[n] : std::lgamma(n + 1.0));
  };
  auto log_choose = [&](int n, int k) {
    return (k < 0 || k > n) ? -1e300 : log_fact(n) - log_fact(k) - log_fact(n - k);
  };
  if (k_draws <= 0 || K_succ <= 0 || N_tot <= 0) return 1.0;
  int max_x = std::min(k_draws, K_succ);
  if (k_succ > max_x) return 0.0;
  int min_x = std::max(0, k_draws - (N_tot - K_succ));
  if (k_succ <= min_x) return 1.0;
  double denom = log_choose(N_tot, k_draws), p = 0.0;
  for (int x = k_succ; x <= max_x; ++x) {
    p += std::exp(log_choose(K_succ, x) + log_choose(N_tot - K_succ, k_draws - x) - denom);
  }
  return std::min(1.0, p);
}

// True when `card` (rank >= 8) is contiguous down to spot card 9 in unknown_cards.
bool IsSpotConnectedHonor(uint64_t unknown_cards, CardType card) {
  if (card == 0) return false;
  auto [suit, rank] = Card::DecodeCard(card);
  if (rank < 8) return true;
  for (int k = 7; k <= rank; ++k) {
    if ((unknown_cards & Card::GetSuitedCard(suit, k)) == 0) return false;
  }
  return true;
}

bool EntryUsesSpotConnectedHonor(const AtomicPredicateEntry& entry,
                                 uint64_t unknown_cards) {
  if (unknown_cards == 0 || !entry.invariant) return false;
  if (auto hp = dynamic_cast<const HonorPossessionInvariant*>(entry.invariant.get())) {
    return IsSpotConnectedHonor(unknown_cards, hp->card());
  }
  if (entry.family == AtomicPredicateFamily::kSuitHonorVariable) {
    for (int r = 8; r <= 12; ++r) {
      CardType c = Card::GetSuitedCard(entry.honor_suit, r);
      if ((unknown_cards & c) != 0 && IsSpotConnectedHonor(unknown_cards, c)) return true;
    }
  } else if (auto cc = dynamic_cast<const CrossSuitCardConditionalInvariant*>(
                 entry.invariant.get())) {
    return IsSpotConnectedHonor(unknown_cards, cc->condition_card()) ||
           IsSpotConnectedHonor(unknown_cards, cc->target_card());
  } else if (auto cl = dynamic_cast<const CrossSuitConditionalInvariant*>(
                 entry.invariant.get())) {
    return IsSpotConnectedHonor(unknown_cards, cl->condition_card());
  }
  return false;
}

// Rewrites CrossSuitConditionalInvariant(known_card, ...) when paired with an
// HonorPossessionInvariant that fixes which hand holds `known_card`.
std::shared_ptr<UnknownCardsInvariant> RewriteConditionalForKnownHolder(
    const std::shared_ptr<UnknownCardsInvariant>& inv, CardType known_card,
    InvariantHandChoice holder_hand, uint64_t unknown_cards, int hand1_size,
    const HandLabelFormatter& formatter) {
  if (!inv || known_card == 0 || (unknown_cards & known_card) == 0) return inv;
  if (auto and_inv = dynamic_cast<const AndInvariant*>(inv.get())) {
    auto nl = RewriteConditionalForKnownHolder(and_inv->left(), known_card,
                                               holder_hand, unknown_cards,
                                               hand1_size, formatter);
    auto nr = RewriteConditionalForKnownHolder(and_inv->right(), known_card,
                                               holder_hand, unknown_cards,
                                               hand1_size, formatter);
    if (nl == and_inv->left() && nr == and_inv->right()) return inv;
    return nl->ToString() == nr->ToString()
               ? nl
               : MakeCanonicalAnd(std::move(nl), std::move(nr));
  }
  auto cond_inv = dynamic_cast<const CrossSuitConditionalInvariant*>(inv.get());
  if (!cond_inv) return inv;

  std::string comp_lbl = (holder_hand == InvariantHandChoice::kHand1)
                             ? formatter.FormatCompositeHand1()
                             : formatter.FormatCompositeHand2();
  auto hp_target = (holder_hand == InvariantHandChoice::kHand1)
                       ? HonorPossessionTargetHand::kHand1
                       : HonorPossessionTargetHand::kHand2;

  if (auto cc = dynamic_cast<const CrossSuitCardConditionalInvariant*>(inv.get())) {
    if (cc->condition_card() == known_card) {
      return std::make_shared<HonorPossessionInvariant>(hp_target, cc->target_card(), comp_lbl);
    }
    if (cc->target_card() == known_card && (unknown_cards & cc->condition_card()) != 0) {
      return std::make_shared<HonorPossessionInvariant>(hp_target, cc->condition_card(), comp_lbl);
    }
    return inv;
  }
  if (cond_inv->condition_card() != known_card) return inv;

  auto suit_bounds = [&](Suit suit) -> std::pair<int, int> {
    int tot_unk = CountCardsInHand(unknown_cards);
    int h1_sz = (hand1_size > 0) ? hand1_size : (tot_unk / 2);
    int h2_sz = std::max(0, tot_unk - h1_sz);
    int tot_suit = CardsInSuit(unknown_cards, suit);
    if (tot_unk <= 0 || h1_sz <= 0 || h1_sz >= tot_unk) return {0, tot_suit};
    return holder_hand == InvariantHandChoice::kHand1
               ? std::make_pair(std::max(0, tot_suit - h2_sz), std::min(tot_suit, h1_sz))
               : std::make_pair(std::max(0, tot_suit - h1_sz), std::min(tot_suit, h2_sz));
  };

  if (auto ex = dynamic_cast<const CrossSuitExactSuitLengthInvariant*>(inv.get())) {
    return std::make_shared<ExactSuitLengthInvariant>(
        holder_hand, ex->target_suit(), ex->length(), comp_lbl);
  }
  Suit t_suit = Suit::SPADES;
  int lo = 0, hi = 0;
  if (auto mn = dynamic_cast<const CrossSuitMinSuitLengthInvariant*>(inv.get())) {
    t_suit = mn->target_suit();
    std::tie(lo, hi) = suit_bounds(t_suit);
    lo = std::max(lo, mn->min_len());
  } else if (auto mx = dynamic_cast<const CrossSuitMaxSuitLengthInvariant*>(inv.get())) {
    t_suit = mx->target_suit();
    std::tie(lo, hi) = suit_bounds(t_suit);
    hi = std::min(hi, mx->max_len());
  } else {
    return inv;
  }
  if (lo > hi) return inv;
  if (lo == hi) {
    return std::make_shared<ExactSuitLengthInvariant>(holder_hand, t_suit, lo, comp_lbl);
  }
  return std::make_shared<SuitLengthRangeInvariant>(holder_hand, t_suit, lo, hi, comp_lbl);
}

void RewriteAgainstHonor(std::shared_ptr<UnknownCardsInvariant>& tgt,
                         const std::shared_ptr<UnknownCardsInvariant>& src,
                         bool same_holder, uint64_t unk, int h1_sz,
                         const HandLabelFormatter& fmt) {
  auto hp = dynamic_cast<const HonorPossessionInvariant*>(src.get());
  if (!hp || (hp->target_hand() != HonorPossessionTargetHand::kHand1 &&
              hp->target_hand() != HonorPossessionTargetHand::kHand2)) {
    return;
  }
  bool is_h1 = (hp->target_hand() == HonorPossessionTargetHand::kHand1);
  auto holder = (same_holder ? is_h1 : !is_h1) ? InvariantHandChoice::kHand1
                                               : InvariantHandChoice::kHand2;
  tgt = RewriteConditionalForKnownHolder(tgt, hp->card(), holder, unk, h1_sz, fmt);
}

std::shared_ptr<UnknownCardsInvariant> MakeCanonicalComposite(
    std::shared_ptr<UnknownCardsInvariant> a,
    std::shared_ptr<UnknownCardsInvariant> b, bool is_and, uint64_t unk,
    int h1_sz, const HandLabelFormatter& fmt) {
  RewriteAgainstHonor(b, a, is_and, unk, h1_sz, fmt);
  RewriteAgainstHonor(a, b, is_and, unk, h1_sz, fmt);
  if (a->ToString() == b->ToString()) return a;
  return is_and ? MakeCanonicalAnd(std::move(a), std::move(b))
                : MakeCanonicalOr(std::move(a), std::move(b));
}

struct BitsetTripleSig {
  DealBitset target, opposing, eval;
  bool operator==(const BitsetTripleSig& o) const {
    return target == o.target && opposing == o.opposing && eval == o.eval;
  }
};
struct BitsetTripleHash {
  size_t operator()(const BitsetTripleSig& s) const noexcept {
    return static_cast<size_t>(s.eval.Hash(s.opposing.Hash(s.target.Hash())));
  }
};

struct BitsetPairSig {
  DealBitset target, opposing;
  bool operator==(const BitsetPairSig& o) const {
    return target == o.target && opposing == o.opposing;
  }
};
struct BitsetPairHash {
  size_t operator()(const BitsetPairSig& s) const noexcept {
    return static_cast<size_t>(s.opposing.Hash(s.target.Hash()));
  }
};

}  // namespace

AtomicPredicateCatalog::AtomicPredicateCatalog(
    const std::vector<PairedDeal>& target_deals,
    const std::vector<PairedDeal>* opposing_deals,
    const HandLabelFormatter& formatter,
    const std::vector<PairedDeal>* eval_bifurcations)
    : target_size_(static_cast<int>(target_deals.size())),
      opposing_size_(opposing_deals ? static_cast<int>(opposing_deals->size()) : 0),
      formatter_(formatter) {
  if (target_deals.empty()) return;

  for (const auto& d : target_deals) unknown_cards_ |= (d.hand1 | d.hand2);
  if (opposing_deals) {
    for (const auto& d : *opposing_deals) unknown_cards_ |= (d.hand1 | d.hand2);
  }
  cards_per_hand_ = CountCardsInHand(target_deals[0].hand1);

  auto target_feats = BuildDealFeatures(target_deals);
  auto opposing_feats = (opposing_deals && !opposing_deals->empty())
                            ? BuildDealFeatures(*opposing_deals)
                            : std::vector<PrecomputedDeal>{};
  auto eval_feats = (eval_bifurcations && !eval_bifurcations->empty())
                        ? BuildDealFeatures(*eval_bifurcations)
                        : std::vector<PrecomputedDeal>{};

  auto add_entry = [&](AtomicPredicateFamily family, auto&& pred,
                       std::shared_ptr<UnknownCardsInvariant> inv,
                       std::shared_ptr<UnknownCardsInvariant> comp_inv = nullptr,
                       Suit h_suit = Suit::SPADES,
                       HonorPossessionTargetHand h_target =
                           HonorPossessionTargetHand::kHand1,
                       uint8_t pure_suit = 0, uint8_t hand_suit = 0) -> int {
    DealBitset tb = EvalBitset(target_feats, pred);
    int tc = tb.PopCount();
    if (tc == 0) return 0;
    AtomicPredicateEntry e;
    e.target_bits = std::move(tb);
    e.target_count = tc;
    if (!opposing_feats.empty()) {
      e.opposing_bits = EvalBitset(opposing_feats, pred);
      e.opposing_count = e.opposing_bits.PopCount();
    }
    if (!eval_feats.empty()) e.eval_bits = EvalBitset(eval_feats, pred);
    e.invariant = std::move(inv);
    e.composite_invariant = comp_inv ? std::move(comp_inv) : e.invariant;
    e.family = family;
    e.honor_suit = h_suit;
    e.honor_target = h_target;
    e.pure_suit_mask = pure_suit;
    e.hand_suit_mask = hand_suit;
    e.rule = e.invariant->ToString();
    entries_.push_back(std::move(e));
    return tc;
  };

  const std::string l1 = formatter_.FormatHand1(), l2 = formatter_.FormatHand2();
  const std::string l1c = formatter_.FormatCompositeHand1();
  const std::string l2c = formatter_.FormatCompositeHand2();
  const int tot_unk = CountCardsInHand(unknown_cards_);
  const int h1_sz = (cards_per_hand_ > 0) ? cards_per_hand_ : (tot_unk / 2);
  const int h2_sz = std::max(0, tot_unk - h1_sz);
  const bool bounded_hands = (tot_unk > 0 && h1_sz > 0 && h1_sz < tot_unk);

  // 1. HonorPossessionInvariant & SuitHonorVariableInvariant (ranks 8..12 only)
  std::vector<CardType> movable_honors;
  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    uint64_t suit_h_mask = 0x1F00ULL << (s * 16);
    uint64_t present = unknown_cards_ & suit_h_mask;
    if (present == 0) continue;

    for (int r = 8; r <= 12; ++r) {
      CardType card = Card::GetSuitedCard(suit, r);
      if ((unknown_cards_ & card) == 0) continue;
      int h1_cnt = add_entry(
          AtomicPredicateFamily::kHonorPossession,
          [card](const PrecomputedDeal& f) { return (f.h1 & card) != 0; },
          std::make_shared<HonorPossessionInvariant>(HonorPossessionTargetHand::kHand1, card, l1),
          std::make_shared<HonorPossessionInvariant>(HonorPossessionTargetHand::kHand1, card, l1c),
          suit, HonorPossessionTargetHand::kHand1);
      int h2_cnt = add_entry(
          AtomicPredicateFamily::kHonorPossession,
          [card](const PrecomputedDeal& f) { return (f.h2 & card) != 0; },
          std::make_shared<HonorPossessionInvariant>(HonorPossessionTargetHand::kHand2, card, l2),
          std::make_shared<HonorPossessionInvariant>(HonorPossessionTargetHand::kHand2, card, l2c),
          suit, HonorPossessionTargetHand::kHand2);
      add_entry(
          AtomicPredicateFamily::kHonorPossession,
          [card](const PrecomputedDeal& f) { return (f.h1 & card) != 0 && (f.h2 & card) != 0; },
          std::make_shared<HonorPossessionInvariant>(
              HonorPossessionTargetHand::kBoth, card, "both hands"),
          nullptr, suit, HonorPossessionTargetHand::kBoth);
      if (h1_cnt > 0 && h2_cnt > 0) movable_honors.push_back(card);
    }

    for (auto [tgt, lbl, is_h1] :
         {std::tuple{HonorPossessionTargetHand::kHand1, l1, true},
          std::tuple{HonorPossessionTargetHand::kHand2, l2, false}}) {
      add_entry(
          AtomicPredicateFamily::kSuitHonorVariable,
          [suit_h_mask, is_h1](const PrecomputedDeal& f) {
            return ((is_h1 ? f.h1 : f.h2) & suit_h_mask) != 0;
          },
          std::make_shared<SuitHonorVariableInvariant>(tgt, suit, lbl), nullptr, suit, tgt);
    }
    if (PopCount64(present) >= 2) {
      add_entry(
          AtomicPredicateFamily::kSuitHonorVariable,
          [suit_h_mask](const PrecomputedDeal& f) {
            return (f.h1 & suit_h_mask) != 0 && (f.h2 & suit_h_mask) != 0;
          },
          std::make_shared<SuitHonorVariableInvariant>(
              HonorPossessionTargetHand::kBoth, suit, "both hands"),
          nullptr, suit, HonorPossessionTargetHand::kBoth);
    }
  }

  // 2. Suit-length & Shortest-split atoms
  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    int tot_suit = CardsInSuit(unknown_cards_, suit);
    if (tot_suit == 0 || tot_suit == tot_unk) continue;
    uint8_t smask = static_cast<uint8_t>(1u << s);

    int min1 = bounded_hands ? std::max(0, tot_suit - h2_sz) : 0;
    int max1 = bounded_hands ? std::min(tot_suit, h1_sz) : tot_suit;
    int min2 = bounded_hands ? std::max(0, tot_suit - h1_sz) : 0;
    int max2 = bounded_hands ? std::min(tot_suit, h2_sz) : tot_suit;

    for (auto [choice, mn, mx, lbl, clbl, hbit] :
         {std::tuple{InvariantHandChoice::kHand1, min1, max1, l1, l1c,
                     static_cast<uint8_t>(1u << s)},
          std::tuple{InvariantHandChoice::kHand2, min2, max2, l2, l2c,
                     static_cast<uint8_t>(1u << (s + 4))}}) {
      if (mn >= mx) continue;
      bool is_h1 = (choice == InvariantHandChoice::kHand1);
      for (int len = mn; len <= mx; ++len) {
        add_entry(
            AtomicPredicateFamily::kExactSuitLength,
            [s, len, is_h1](const PrecomputedDeal& f) {
              return (is_h1 ? f.len1[s] : f.len2[s]) == len;
            },
            std::make_shared<ExactSuitLengthInvariant>(choice, suit, len, lbl),
            std::make_shared<ExactSuitLengthInvariant>(choice, suit, len, clbl),
            suit, HonorPossessionTargetHand::kHand1, smask, hbit);
      }
      for (int lo = mn; lo <= mx; ++lo) {
        for (int hi = lo + 1; hi <= mx; ++hi) {
          if ((lo == mn && hi == mx) || (lo == 0 && hi == tot_suit)) continue;
          add_entry(
              AtomicPredicateFamily::kSuitLengthRange,
              [s, lo, hi, is_h1](const PrecomputedDeal& f) {
                int v = is_h1 ? f.len1[s] : f.len2[s];
                return v >= lo && v <= hi;
              },
              std::make_shared<SuitLengthRangeInvariant>(choice, suit, lo, hi, lbl),
              std::make_shared<SuitLengthRangeInvariant>(choice, suit, lo, hi, clbl),
              suit, HonorPossessionTargetHand::kHand1, smask, hbit);
        }
      }
    }

    for (int m = (tot_suit + 1) / 2; m < std::max(max1, max2); ++m) {
      add_entry(
          AtomicPredicateFamily::kMaxSuitLength,
          [s, m](const PrecomputedDeal& f) { return std::max(f.len1[s], f.len2[s]) <= m; },
          std::make_shared<MaxSuitLengthInvariant>(suit, m), nullptr,
          suit, HonorPossessionTargetHand::kHand1, smask, 0);
    }

    int min_sh = std::min(min1, min2), max_sh = tot_suit / 2;
    for (int k = std::max(1, min_sh + 1); k <= max_sh; ++k) {
      add_entry(
          AtomicPredicateFamily::kShortestLength,
          [s, k](const PrecomputedDeal& f) { return std::min(f.len1[s], f.len2[s]) >= k; },
          std::make_shared<ShortestLengthInvariant>(
              suit, ShortestLengthInvariant::Comparison::kAtLeast, k),
          nullptr, suit, HonorPossessionTargetHand::kHand1, smask, 0);
    }
    for (int k = min_sh; k < max_sh; ++k) {
      add_entry(
          AtomicPredicateFamily::kShortestLength,
          [s, k](const PrecomputedDeal& f) { return std::min(f.len1[s], f.len2[s]) <= k; },
          std::make_shared<ShortestLengthInvariant>(
              suit, ShortestLengthInvariant::Comparison::kAtMost, k),
          nullptr, suit, HonorPossessionTargetHand::kHand1, smask, 0);
    }
  }

  // 3. Cross-suit conditional atoms (movable honors 8..12 only)
  const int n_t = static_cast<int>(target_feats.size());
  const int n_o = static_cast<int>(opposing_feats.size());
  auto passes_len_cond = [&](auto&& len_pred, int s, CardType cond_card,
                             bool is_off_suit) -> bool {
    if (n_o == 0) return true;
    int tc1 = 0, oc1 = 0, tc2 = 0, oc2 = 0;
    int sp_mt = 0, sp_mo = 0, sp_xt = 0, sp_xo = 0;
    auto tally = [&](const std::vector<PrecomputedDeal>& fs, int& c1_acc,
                     int& c2_acc, int& sp_m, int& sp_x) {
      for (const auto& f : fs) {
        bool p1 = len_pred(f.len1[s]), p2 = len_pred(f.len2[s]);
        bool c1 = (f.h1 & cond_card) != 0, c2 = (f.h2 & cond_card) != 0;
        if (c1 && p1) ++c1_acc;
        else if (c2 && p2) ++c2_acc;
        if (p1 != p2) {
          if ((c1 && p1) || (c2 && p2)) ++sp_m;
          else if (c1 || c2) ++sp_x;
        }
      }
    };
    tally(target_feats, tc1, tc2, sp_mt, sp_xt);
    if (tc1 + tc2 == 0 || sp_mt == 0) return false;
    tally(opposing_feats, oc1, oc2, sp_mo, sp_xo);
    if ((tc1 + oc1 >= 3 && static_cast<int64_t>(tc1) * n_o <= static_cast<int64_t>(oc1) * n_t) ||
        (tc2 + oc2 >= 3 && static_cast<int64_t>(tc2) * n_o <= static_cast<int64_t>(oc2) * n_t)) {
      return false;
    }
    int sp_m = sp_mt + sp_mo, sp_x = sp_xt + sp_xo;
    if (sp_x > 0 && static_cast<int64_t>(sp_mt) * sp_x <= static_cast<int64_t>(sp_xt) * sp_m) {
      return false;
    }
    if (n_t + n_o >= 20) {
      if (HypergeomUpperTailPValue(sp_mt, sp_m, sp_mt + sp_xt, sp_m + sp_x) >
          (is_off_suit ? 0.02 : 0.05)) {
        return false;
      }
      if (is_off_suit &&
          HypergeomUpperTailPValue(tc1 + tc2, tc1 + tc2 + oc1 + oc2, n_t, n_t + n_o) > 0.02) {
        return false;
      }
    }
    return true;
  };

  auto passes_card_cond = [&](auto&& h1_p, auto&& h2_p, bool is_off_suit) -> bool {
    if (n_o == 0) return true;
    int tc1 = 0, oc1 = 0, tc2 = 0, oc2 = 0;
    for (const auto& f : target_feats) {
      if (h1_p(f)) ++tc1;
      else if (h2_p(f)) ++tc2;
    }
    if (tc1 + tc2 == 0) return false;
    for (const auto& f : opposing_feats) {
      if (h1_p(f)) ++oc1;
      else if (h2_p(f)) ++oc2;
    }
    if ((tc1 + oc1 >= 3 && static_cast<int64_t>(tc1) * n_o <= static_cast<int64_t>(oc1) * n_t) ||
        (tc2 + oc2 >= 3 && static_cast<int64_t>(tc2) * n_o <= static_cast<int64_t>(oc2) * n_t)) {
      return false;
    }
    return !(is_off_suit && n_t + n_o >= 20 &&
             HypergeomUpperTailPValue(tc1 + tc2, tc1 + tc2 + oc1 + oc2, n_t, n_t + n_o) > 0.02);
  };

  for (CardType cond_card : movable_honors) {
    Suit cond_suit = Card::DecodeCard(cond_card).first;
    for (int s = 0; s < 4; ++s) {
      Suit suit = static_cast<Suit>(s);
      int tot_suit = CardsInSuit(unknown_cards_, suit);
      if (tot_suit == 0) continue;
      int in_s = (cond_suit == suit) ? 1 : 0;
      int rem_in = tot_suit - in_s, rem_out = (tot_unk - 1) - rem_in;
      auto min_w = [&](int sz) { return sz <= 0 ? in_s : in_s + std::max(0, (sz - 1) - rem_out); };
      auto max_w = [&](int sz) { return sz <= 0 ? in_s : in_s + std::min(rem_in, sz - 1); };
      int min_p = bounded_hands ? std::min(min_w(h1_sz), min_w(h2_sz)) : in_s;
      int max_p = bounded_hands ? std::max(max_w(h1_sz), max_w(h2_sz)) : tot_suit;
      if (min_p >= max_p) continue;
      bool off_suit = (cond_suit != suit);
      auto add_len_cond = [&](auto&& lp, std::shared_ptr<UnknownCardsInvariant> inv) {
        if (passes_len_cond(lp, s, cond_card, off_suit)) {
          add_entry(
              AtomicPredicateFamily::kCrossSuitLength,
              [cond_card, s, lp](const PrecomputedDeal& f) {
                return ((f.h1 & cond_card) != 0 && lp(f.len1[s])) ||
                       ((f.h2 & cond_card) != 0 && lp(f.len2[s]));
              },
              std::move(inv));
        }
      };
      for (int n = min_p; n <= max_p; ++n) {
        add_len_cond([n](int v) { return v == n; },
                     std::make_shared<CrossSuitExactSuitLengthInvariant>(cond_card, suit, n));
      }
      for (int n = min_p + 1; n < max_p; ++n) {
        add_len_cond([n](int v) { return v >= n; },
                     std::make_shared<CrossSuitMinSuitLengthInvariant>(cond_card, suit, n));
        add_len_cond([n](int v) { return v <= n; },
                     std::make_shared<CrossSuitMaxSuitLengthInvariant>(cond_card, suit, n));
      }
    }

    for (int s2 = 0; s2 < 4; ++s2) {
      Suit suit2 = static_cast<Suit>(s2);
      for (int r2 = 8; r2 <= 12; ++r2) {
        CardType tc_card = Card::GetSuitedCard(suit2, r2);
        if (tc_card == cond_card || (unknown_cards_ & tc_card) == 0) continue;
        auto h1c = [cond_card, tc_card](const PrecomputedDeal& f) {
          return (f.h1 & cond_card) != 0 && (f.h1 & tc_card) != 0;
        };
        auto h2c = [cond_card, tc_card](const PrecomputedDeal& f) {
          return (f.h2 & cond_card) != 0 && (f.h2 & tc_card) != 0;
        };
        if (passes_card_cond(h1c, h2c, cond_suit != suit2)) {
          add_entry(AtomicPredicateFamily::kCrossSuitCard,
                    [h1c, h2c](const PrecomputedDeal& f) { return h1c(f) || h2c(f); },
                    std::make_shared<CrossSuitCardConditionalInvariant>(cond_card, tc_card));
        }
      }
    }
  }
}

std::vector<AtomicPredicateEntry>
AtomicPredicateCatalog::DeduplicatedEntries() const {
  std::unordered_map<BitsetTripleSig, size_t, BitsetTripleHash> sig_to_idx;
  sig_to_idx.reserve(entries_.size());
  std::vector<AtomicPredicateEntry> deduped;
  deduped.reserve(entries_.size());

  std::set<std::pair<Suit, HonorPossessionTargetHand>> spec_cov;
  for (const auto& e : entries_) {
    if (e.family == AtomicPredicateFamily::kHonorPossession &&
        MeetsCeilingQuantile(e.target_count, target_size_, 9, 10)) {
      spec_cov.insert({e.honor_suit, e.honor_target});
    }
  }

  for (const auto& e : entries_) {
    if (e.target_count == 0) continue;
    if (e.family == AtomicPredicateFamily::kSuitHonorVariable) {
      bool h1c = spec_cov.count({e.honor_suit, HonorPossessionTargetHand::kHand1}) ||
                 spec_cov.count({e.honor_suit, HonorPossessionTargetHand::kBoth});
      bool h2c = spec_cov.count({e.honor_suit, HonorPossessionTargetHand::kHand2}) ||
                 spec_cov.count({e.honor_suit, HonorPossessionTargetHand::kBoth});
      if ((e.honor_target == HonorPossessionTargetHand::kHand1 && h1c) ||
          (e.honor_target == HonorPossessionTargetHand::kHand2 && h2c) ||
          (e.honor_target == HonorPossessionTargetHand::kBoth &&
           ((h1c && h2c) || (!MeetsCeilingQuantile(e.target_count, target_size_, 9, 10) &&
                             (h1c || h2c))))) {
        continue;
      }
    }
    BitsetTripleSig sig{e.target_bits, e.opposing_bits, e.eval_bits};
    auto [it, inserted] = sig_to_idx.emplace(std::move(sig), deduped.size());
    if (inserted) {
      deduped.push_back(e);
    } else if (IsBetterRule(e.rule, deduped[it->second].rule)) {
      deduped[it->second] = e;
    }
  }
  return deduped;
}

bool SetCoverSynthesizer::PassesUniformRecallOrContrastGate(
    int target_count, int target_total, int opposing_count, int opposing_total) {
  if (target_total <= 0 || target_count <= 0) return false;
  if (opposing_total <= 0) return target_count == target_total;
  if (static_cast<int64_t>(target_count) * opposing_total <=
      static_cast<int64_t>(opposing_count) * target_total) {
    return false;
  }
  bool high_recall = MeetsCeilingQuantile(target_count, target_total, 9, 10) &&
                     static_cast<int64_t>(opposing_count) * 2 <= opposing_total;
  bool high_contrast = MeetsCeilingQuantile(target_count, target_total, 3, 4) &&
                       static_cast<int64_t>(opposing_count) * 10 <= opposing_total;
  return high_recall || high_contrast;
}

std::vector<SynthesizedCandidate> SetCoverSynthesizer::Synthesize(
    const AtomicPredicateCatalog& catalog) {
  std::vector<SynthesizedCandidate> results;
  const int n_t = catalog.target_size(), n_o = catalog.opposing_size();
  const uint64_t unk = catalog.unknown_cards();
  const int h1_sz = catalog.cards_per_hand();
  const auto& fmt = catalog.formatter();
  if (n_t <= 0) return results;

  std::unordered_map<BitsetPairSig, size_t, BitsetPairHash> atom_pair_map;
  std::vector<AtomicPredicateEntry> atoms;
  std::vector<std::string> atom_rules;
  std::set<std::pair<Suit, HonorPossessionTargetHand>> shv_covered;

  for (auto& a : catalog.DeduplicatedEntries()) {
    if (n_o > 0 && EntryUsesSpotConnectedHonor(a, unk)) continue;
    if (PassesUniformRecallOrContrastGate(a.target_count, n_t, a.opposing_count, n_o)) {
      if (a.family == AtomicPredicateFamily::kSuitHonorVariable) {
        shv_covered.insert({a.honor_suit, a.honor_target});
      }
      results.push_back({a.invariant, a.rule, a.target_bits, a.opposing_bits,
                         a.eval_bits, a.target_count, a.opposing_count, 1});
    }
    std::string cr = a.composite_invariant->ToString();
    auto [it, ins] =
        atom_pair_map.emplace(BitsetPairSig{a.target_bits, a.opposing_bits}, atoms.size());
    if (ins) {
      atoms.push_back(std::move(a));
      atom_rules.push_back(std::move(cr));
    } else if (IsBetterRule(cr, atom_rules[it->second])) {
      atoms[it->second] = std::move(a);
      atom_rules[it->second] = std::move(cr);
    }
  }
  if (atoms.empty()) return results;

  struct ClauseNode {
    std::shared_ptr<UnknownCardsInvariant> invariant;
    std::string rule;
    DealBitset target_bits, opposing_bits, eval_bits;
    int target_count = 0, opposing_count = 0, atom_clauses = 1;
    uint8_t pure_suit_mask = 0, hand_suit_mask = 0;
    bool is_single_honor = false;
    Suit honor_suit = Suit::SPADES;
    HonorPossessionTargetHand honor_target = HonorPossessionTargetHand::kHand1;
    size_t i0 = 0, i1 = 0, i2 = 0;
  };

  auto make_atom_node = [&](size_t idx) {
    const auto& a = atoms[idx];
    return ClauseNode{a.composite_invariant,
                      atom_rules[idx],
                      a.target_bits,
                      a.opposing_bits,
                      a.eval_bits,
                      a.target_count,
                      a.opposing_count,
                      1,
                      a.pure_suit_mask,
                      a.hand_suit_mask,
                      a.family == AtomicPredicateFamily::kHonorPossession,
                      a.honor_suit,
                      a.honor_target,
                      idx};
  };
  auto is_subset_dom = [](const ClauseNode& c, const ClauseNode& p) {
    return p.opposing_count <= c.opposing_count &&
           (p.target_count > c.target_count || p.opposing_count < c.opposing_count) &&
           c.target_bits.IsSubsetOf(p.target_bits);
  };
  auto upsert_node = [](std::unordered_map<BitsetPairSig, size_t, BitsetPairHash>& seen,
                        std::vector<ClauseNode>& cands, BitsetPairSig sig, ClauseNode node) {
    auto [it, ins] = seen.emplace(std::move(sig), cands.size());
    if (ins) {
      cands.push_back(std::move(node));
    } else if (IsBetterRule(node.rule, cands[it->second].rule)) {
      cands[it->second] = std::move(node);
    }
  };
  auto advance_beam = [&](std::vector<ClauseNode>& cands,
                          std::vector<std::pair<double, size_t>>& ord) {
    std::stable_sort(ord.begin(), ord.end(), [&](const auto& a, const auto& b) {
      return std::abs(a.first - b.first) > 1e-9
                 ? a.first > b.first
                 : IsBetterRule(cands[a.second].rule, cands[b.second].rule);
    });
    std::vector<ClauseNode> next;
    for (size_t k = 0; k < std::min(kBeamWidth, ord.size()); ++k) {
      auto& item = cands[ord[k].second];
      if (PassesUniformRecallOrContrastGate(item.target_count, n_t, item.opposing_count, n_o)) {
        results.push_back({item.invariant, item.rule, item.target_bits, item.opposing_bits,
                           item.eval_bits, item.target_count, item.opposing_count,
                           item.atom_clauses});
      }
      next.push_back(std::move(item));
    }
    return next;
  };

  // --- Pass 1: Bounded Bitwise AND Beam (width <= 32, clauses <= 3) ---
  std::vector<ClauseNode> sharp_2ands;
  if (n_o > 0) {
    const int64_t total_sim = static_cast<int64_t>(n_t) + n_o;
    std::vector<std::pair<double, size_t>> pool_ord;
    for (size_t i = 0; i < atoms.size(); ++i) {
      const auto& a = atoms[i];
      if (a.target_count <= 0 || (n_t >= 10 && a.target_count < 2)) continue;
      if (static_cast<int64_t>(a.target_count + a.opposing_count) * 20 >= 19 * total_sim) continue;
      if (static_cast<int64_t>(a.target_count) * n_o >
              static_cast<int64_t>(a.opposing_count) * n_t ||
          MeetsCeilingQuantile(a.target_count, n_t, 3, 4)) {
        double rec = static_cast<double>(a.target_count) / n_t;
        double opp_r = static_cast<double>(a.opposing_count) / n_o;
        double sprec = a.target_count / (a.target_count + a.opposing_count + 1.5);
        pool_ord.emplace_back((rec - opp_r) + 0.30 * sprec, i);
      }
    }
    std::stable_sort(pool_ord.begin(), pool_ord.end(), [&](const auto& x, const auto& y) {
      return std::abs(x.first - y.first) > 1e-9
                 ? x.first > y.first
                 : IsBetterRule(atom_rules[x.second], atom_rules[y.second]);
    });

    std::vector<size_t> and_pool;
    std::vector<ClauseNode> and_beam;
    for (size_t k = 0; k < std::min<size_t>(48, pool_ord.size()); ++k) {
      and_pool.push_back(pool_ord[k].second);
      if (k < kBeamWidth) and_beam.push_back(make_atom_node(pool_ord[k].second));
    }

    for (int depth = 2; depth <= kMaxClauses; ++depth) {
      std::unordered_map<BitsetPairSig, size_t, BitsetPairHash> seen;
      std::vector<ClauseNode> cands;
      for (size_t bi = 0; bi < and_beam.size(); ++bi) {
        const auto& b = and_beam[bi];
        if (b.opposing_count == 0) continue;
        for (size_t pk = 0; pk < and_pool.size(); ++pk) {
          if (depth == 2 && pk < and_beam.size() && pk <= bi) continue;
          size_t ci = and_pool[pk];
          if (ci == b.i0 || (depth == 3 && ci == b.i1)) continue;
          const auto& c = atoms[ci];
          if ((b.pure_suit_mask & c.pure_suit_mask) != 0) continue;

          int tc = b.target_bits.AndPopCount(c.target_bits);
          if (tc == 0 || (n_t >= 10 && tc < 2)) continue;
          int oc = b.opposing_bits.AndPopCount(c.opposing_bits);
          if (!StrictlyBeatsPrecision(tc, oc, b.target_count, b.opposing_count) ||
              !StrictlyBeatsPrecision(tc, oc, c.target_count, c.opposing_count)) {
            continue;
          }
          if (total_sim >= 20) {
            int nb = b.target_count + b.opposing_count, nc = c.target_count + c.opposing_count;
            if ((nb >= 15 && HypergeomUpperTailPValue(tc, tc + oc, b.target_count, nb) > 0.02) ||
                (nc >= 15 && HypergeomUpperTailPValue(tc, tc + oc, c.target_count, nc) > 0.02) ||
                HypergeomUpperTailPValue(tc, tc + oc, n_t, static_cast<int>(total_sim)) > 0.005) {
              continue;
            }
          }
          if (depth == 3) {
            const auto &p0 = atoms[b.i0], &p1 = atoms[b.i1];
            if (!StrictlyBeatsPrecision(tc, oc, p0.target_count, p0.opposing_count) ||
                !StrictlyBeatsPrecision(tc, oc, p1.target_count, p1.opposing_count) ||
                !StrictlyBeatsPrecision(tc, oc, p0.target_bits.AndPopCount(c.target_bits),
                                        p0.opposing_bits.AndPopCount(c.opposing_bits)) ||
                !StrictlyBeatsPrecision(tc, oc, p1.target_bits.AndPopCount(c.target_bits),
                                        p1.opposing_bits.AndPopCount(c.opposing_bits))) {
              continue;
            }
          }

          DealBitset tb = b.target_bits & c.target_bits, ob = b.opposing_bits & c.opposing_bits;
          BitsetPairSig sig{tb, ob};
          if (atom_pair_map.count(sig)) continue;
          auto inv = MakeCanonicalComposite(b.invariant, c.composite_invariant, true, unk, h1_sz, fmt);
          std::string rule = inv->ToString();
          upsert_node(seen, cands, std::move(sig),
                      ClauseNode{std::move(inv), std::move(rule), std::move(tb), std::move(ob),
                                 b.eval_bits & c.eval_bits, tc, oc, depth,
                                 static_cast<uint8_t>(b.pure_suit_mask | c.pure_suit_mask),
                                 static_cast<uint8_t>(b.hand_suit_mask | c.hand_suit_mask),
                                 false, Suit::SPADES, HonorPossessionTargetHand::kHand1,
                                 b.i0, depth == 2 ? ci : b.i1, depth == 3 ? ci : 0});
        }
      }

      if (depth == 2 && !cands.empty()) {
        std::vector<std::pair<double, size_t>> sharp_ord;
        for (size_t i = 0; i < cands.size(); ++i) {
          const auto& x = cands[i];
          if (static_cast<int64_t>(x.target_count) * n_o >
                  static_cast<int64_t>(x.opposing_count) * n_t &&
              x.target_count > x.opposing_count &&
              static_cast<int64_t>(x.target_count) * 20 >= n_t &&
              !MeetsCeilingQuantile(x.target_count, n_t, 9, 10)) {
            double sprec = x.target_count / (x.target_count + x.opposing_count + 1.5);
            double rec = static_cast<double>(x.target_count) / n_t;
            double opp_r = static_cast<double>(x.opposing_count) / n_o;
            sharp_ord.emplace_back(sprec * (rec - opp_r) + 0.35 * sprec + 0.25 * rec, i);
          }
        }
        std::stable_sort(sharp_ord.begin(), sharp_ord.end(), [&](const auto& a, const auto& b) {
          return std::abs(a.first - b.first) > 1e-9
                     ? a.first > b.first
                     : cands[a.second].target_count > cands[b.second].target_count;
        });
        for (const auto& [sc, idx] : sharp_ord) {
          if (std::none_of(sharp_2ands.begin(), sharp_2ands.end(),
                           [&](const auto& prev) { return is_subset_dom(cands[idx], prev); })) {
            sharp_2ands.push_back(cands[idx]);
            if (sharp_2ands.size() >= 24) break;
          }
        }
      }

      std::vector<std::pair<double, size_t>> beam_ord;
      for (size_t i = 0; i < cands.size(); ++i) {
        const auto& x = cands[i];
        double sprec = x.target_count / (x.target_count + x.opposing_count + 1.0);
        double rec = static_cast<double>(x.target_count) / n_t;
        double opp_r = static_cast<double>(x.opposing_count) / n_o;
        double boost = MeetsCeilingQuantile(x.target_count, n_t, 3, 4) ? 1.0 : 0.4;
        beam_ord.emplace_back(boost * sprec + 0.55 * rec - 0.55 * opp_r, i);
      }
      and_beam = advance_beam(cands, beam_ord);
    }
  }

  // --- Pass 2: Set-Cover Bitwise OR Beam (width <= 32, total clauses <= 3) ---
  std::vector<ClauseNode> or_clauses;
  {
    std::unordered_map<BitsetPairSig, size_t, BitsetPairHash> cand_map;
    std::vector<ClauseNode> pool;
    for (size_t i = 0; i < atoms.size(); ++i) {
      const auto& a = atoms[i];
      if (a.target_count <= 0 || a.target_count >= n_t) continue;
      if (static_cast<int64_t>(a.target_count) * 20 < n_t) continue;
      if (MeetsCeilingQuantile(a.target_count, n_t, 9, 10)) continue;
      if (n_o > 0 &&
          (static_cast<int64_t>(a.target_count) * n_o <=
               static_cast<int64_t>(a.opposing_count) * n_t ||
           static_cast<int64_t>(a.target_count) * 3 <
               static_cast<int64_t>(a.opposing_count) * 2)) {
        continue;
      }
      cand_map.emplace(BitsetPairSig{a.target_bits, a.opposing_bits}, pool.size());
      pool.push_back(make_atom_node(i));
    }
    for (const auto& sa : sharp_2ands) {
      if (cand_map.emplace(BitsetPairSig{sa.target_bits, sa.opposing_bits}, pool.size()).second) {
        pool.push_back(sa);
      }
    }

    std::vector<std::pair<double, size_t>> seed_ord;
    for (size_t i = 0; i < pool.size(); ++i) {
      double sprec = pool[i].target_count / (pool[i].target_count + pool[i].opposing_count + 1.5);
      double rec = static_cast<double>(pool[i].target_count) / n_t;
      double opp_r = n_o > 0 ? static_cast<double>(pool[i].opposing_count) / n_o : 0.0;
      seed_ord.emplace_back(sprec * (rec - opp_r) + 0.40 * sprec + 0.15 * (rec - opp_r), i);
    }
    std::stable_sort(seed_ord.begin(), seed_ord.end(), [&](const auto& a, const auto& b) {
      return std::abs(a.first - b.first) > 1e-9
                 ? a.first > b.first
                 : IsBetterRule(pool[a.second].rule, pool[b.second].rule);
    });

    std::vector<std::pair<size_t, double>> nondom;
    for (const auto& [sc, idx] : seed_ord) {
      if (std::none_of(nondom.begin(), nondom.end(),
                       [&](const auto& p) { return is_subset_dom(pool[idx], pool[p.first]); })) {
        nondom.emplace_back(idx, sc);
      }
    }

    std::vector<uint8_t> picked(nondom.size(), 0);
    DealBitset covered(n_t);
    for (size_t step = 0; step < std::min(kBeamWidth, nondom.size()); ++step) {
      size_t best_k = nondom.size();
      double best_v = -1e18;
      for (size_t k = 0; k < nondom.size(); ++k) {
        if (picked[k]) continue;
        int uncov = pool[nondom[k].first].target_bits.AndNotCount(covered);
        double v = nondom[k].second + 0.35 * (static_cast<double>(uncov) / n_t);
        if (best_k == nondom.size() || v > best_v + 1e-9) {
          best_v = v;
          best_k = k;
        }
      }
      if (best_k == nondom.size()) break;
      picked[best_k] = 1;
      covered = covered | pool[nondom[best_k].first].target_bits;
      or_clauses.push_back(std::move(pool[nondom[best_k].first]));
    }
  }

  if (or_clauses.size() >= 2) {
    std::vector<ClauseNode> or_beam;
    for (size_t i = 0; i < std::min(kBeamWidth, or_clauses.size()); ++i) {
      ClauseNode st = or_clauses[i];
      st.i0 = i;
      or_beam.push_back(std::move(st));
    }

    auto valid_or_ext = [&](int tc, int oc, int base_tc, int base_oc) -> bool {
      int gain = tc - base_tc;
      if (gain <= 0 || static_cast<int64_t>(gain) * 20 < n_t) return false;
      bool base_hi = MeetsCeilingQuantile(base_tc, n_t, 9, 10) && (n_o == 0 || base_tc > base_oc);
      if (base_hi && !StrictlyBeatsPrecision(tc, oc, base_tc, base_oc)) return false;
      if (n_o > 0) {
        int add_oc = oc - base_oc;
        if (gain <= add_oc) return false;
        if (n_t + n_o >= 20 &&
            HypergeomUpperTailPValue(gain, gain + add_oc, n_t - base_tc,
                                     (n_t - base_tc) + (n_o - base_oc)) > 0.005) {
          return false;
        }
      }
      return true;
    };

    for (int depth = 2; depth <= kMaxClauses; ++depth) {
      std::unordered_map<BitsetPairSig, size_t, BitsetPairHash> seen;
      std::vector<ClauseNode> cands;

      for (const auto& b : or_beam) {
        if (b.target_count >= n_t) continue;
        for (size_t ci = (depth == 2 ? b.i0 + 1 : 0); ci < or_clauses.size(); ++ci) {
          if (ci == b.i0 || (depth == 3 && ci == b.i1)) continue;
          const auto& c = or_clauses[ci];
          if (b.atom_clauses + c.atom_clauses > kMaxClauses) continue;
          if ((b.hand_suit_mask & c.hand_suit_mask) != 0) continue;

          if (depth == 2 && or_clauses[b.i0].is_single_honor && c.is_single_honor) {
            const auto& b0 = or_clauses[b.i0];
            if (b0.honor_suit == c.honor_suit && b0.honor_target == c.honor_target &&
                (shv_covered.count({b0.honor_suit, b0.honor_target}) ||
                 shv_covered.count({b0.honor_suit, HonorPossessionTargetHand::kBoth}))) {
              continue;
            }
          }

          int tc = b.target_bits.OrPopCount(c.target_bits);
          int oc = b.opposing_bits.OrPopCount(c.opposing_bits);
          if (!valid_or_ext(tc, oc, b.target_count, b.opposing_count) ||
              !valid_or_ext(tc, oc, c.target_count, c.opposing_count)) {
            continue;
          }
          if (depth == 3) {
            const auto &p0 = or_clauses[b.i0], &p1 = or_clauses[b.i1];
            if (!valid_or_ext(tc, oc, p0.target_bits.OrPopCount(c.target_bits),
                              p0.opposing_bits.OrPopCount(c.opposing_bits)) ||
                !valid_or_ext(tc, oc, p1.target_bits.OrPopCount(c.target_bits),
                              p1.opposing_bits.OrPopCount(c.opposing_bits))) {
              continue;
            }
          }

          DealBitset tb = b.target_bits | c.target_bits, ob = b.opposing_bits | c.opposing_bits;
          BitsetPairSig sig{tb, ob};
          if (atom_pair_map.count(sig)) continue;

          std::shared_ptr<UnknownCardsInvariant> inv;
          if (depth == 2) {
            inv = MakeCanonicalComposite(or_clauses[b.i0].invariant, c.invariant, false, unk, h1_sz, fmt);
          } else {
            auto v0 = or_clauses[b.i0].invariant, v1 = or_clauses[b.i1].invariant, v2 = c.invariant;
            for (auto* p : {&v0, &v1}) RewriteAgainstHonor(*p, v2, false, unk, h1_sz, fmt);
            for (auto* p : {&v0, &v2}) RewriteAgainstHonor(*p, v1, false, unk, h1_sz, fmt);
            for (auto* p : {&v1, &v2}) RewriteAgainstHonor(*p, v0, false, unk, h1_sz, fmt);
            inv = MakeCanonicalOr3(std::move(v0), std::move(v1), std::move(v2));
          }
          std::string rule = inv->ToString();
          upsert_node(seen, cands, std::move(sig),
                      ClauseNode{std::move(inv), std::move(rule), std::move(tb), std::move(ob),
                                 b.eval_bits | c.eval_bits, tc, oc,
                                 b.atom_clauses + c.atom_clauses, 0,
                                 static_cast<uint8_t>(b.hand_suit_mask | c.hand_suit_mask),
                                 false, Suit::SPADES, HonorPossessionTargetHand::kHand1,
                                 b.i0, depth == 2 ? ci : b.i1, depth == 3 ? ci : 0});
        }
      }

      std::vector<std::pair<double, size_t>> or_ord;
      for (size_t i = 0; i < cands.size(); ++i) {
        const auto& s = cands[i];
        double denom = s.target_count + s.opposing_count;
        double prec = denom > 0 ? s.target_count / denom : 0.0;
        double rec = static_cast<double>(s.target_count) / n_t;
        double opp_r = n_o > 0 ? static_cast<double>(s.opposing_count) / n_o : 0.0;
        bool passes = PassesUniformRecallOrContrastGate(s.target_count, n_t, s.opposing_count, n_o) &&
                      (n_o == 0 || s.target_count > s.opposing_count);
        double pen = (n_o > 0 && opp_r > 0.10 && rec < 0.90 ? 0.30 * (opp_r - 0.10) : 0.0) +
                     (n_o > 0 && prec <= 0.50 ? 0.60 * (0.55 - prec) : 0.0);
        or_ord.emplace_back((passes ? 0.6 : 0.0) + rec + 0.65 * prec - 0.85 * opp_r - pen, i);
      }
      or_beam = advance_beam(cands, or_ord);
    }
  }

  // Final Pareto, 100%-precision subsumption, and marginal-precision pruning
  if (results.size() > 1) {
    const int64_t total_sim = static_cast<int64_t>(n_t) + n_o;
    auto survives_gates = [&](const SynthesizedCandidate& y) {
      if (n_o <= 0) return true;
      int hc = y.target_count + y.opposing_count;
      if (hc <= 0) return false;
      int hpct = static_cast<int>(std::lround(100.0 * hc / total_sim));
      int ppct = static_cast<int>(std::lround(100.0 * y.target_count / hc));
      return static_cast<int64_t>(hc) * 20 < 19 * total_sim && hpct < 95 &&
             y.target_count * 2 > hc && ppct > 50;
    };

    std::vector<SynthesizedCandidate> pruned;
    pruned.reserve(results.size());
    for (size_t i = 0; i < results.size(); ++i) {
      const auto& x = results[i];
      bool dominated = false;
      for (size_t j = 0; j < results.size(); ++j) {
        if (i == j || !survives_gates(results[j])) continue;
        const auto& y = results[j];
        if (x.clause_count <= 1) {
          if (y.target_count > x.target_count && x.target_bits.IsSubsetOf(y.target_bits) &&
              (y.opposing_count == 0 ||
               (y.clause_count <= 1 && y.opposing_count <= x.opposing_count))) {
            dominated = true;
            break;
          }
          continue;
        }
        if (y.clause_count > x.clause_count) continue;
        if ((y.target_count >= x.target_count && y.opposing_count <= x.opposing_count &&
             (y.target_count > x.target_count || y.opposing_count < x.opposing_count ||
              y.clause_count < x.clause_count)) ||
            (n_o > 0 && y.opposing_count == 0 && x.opposing_count > 0 &&
             MeetsCeilingQuantile(y.target_count, n_t, 19, 20)) ||
            (n_o > 0 && y.opposing_count <= 1 &&
             MeetsCeilingQuantile(y.target_count, n_t, 19, 20) &&
             y.target_count >= 19 * y.opposing_count &&
             x.clause_count > y.clause_count && x.target_count + 2 < y.target_count) ||
            (n_o > 0 && x.target_count > y.target_count && x.opposing_count > y.opposing_count &&
             (x.target_count - y.target_count) <= (x.opposing_count - y.opposing_count) &&
             y.target_bits.IsSubsetOf(x.target_bits))) {
          dominated = true;
          break;
        }
      }
      if (!dominated) pruned.push_back(x);
    }
    results = std::move(pruned);
  }
  return results;
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

  for (const auto& deal : deals) {
    suit_extractor.ProcessDeal(deal);
    honor_classifier.ProcessDeal(deal);
  }

  report.suit_bounds = suit_extractor.GetSuitBounds();
  report.honor_stats = honor_classifier.GetHonorStats();

  if (opposing_deals != nullptr) {
    std::vector<PairedDeal> bifurcations = GenerateRandomBifurcations(
        report.unknown_cards, report.cards_per_hand, 32);
    AtomicPredicateCatalog catalog(deals, opposing_deals, formatter, &bifurcations);
    int total_sim = static_cast<int>(deals.size() + opposing_deals->size());

    std::unordered_map<BitsetTripleSig, size_t, BitsetTripleHash> dedup_map;
    std::vector<InvariantStats> deduped;
    for (auto& cand : SetCoverSynthesizer::Synthesize(catalog)) {
      if (!cand.invariant) continue;
      if (!SetCoverSynthesizer::PassesUniformRecallOrContrastGate(
              cand.target_count, static_cast<int>(deals.size()),
              cand.opposing_count, static_cast<int>(opposing_deals->size()))) {
        continue;
      }
      int holds_count = cand.target_count + cand.opposing_count;
      InvariantStats st;
      st.invariant = cand.invariant;
      st.rule = std::move(cand.rule);
      st.holds_count = holds_count;
      st.total_bifurcations = total_sim;
      st.has_line_stats = (line != nullptr || is_failure_mode);
      st.is_failure_mode = is_failure_mode;
      st.success_when_holds_count = cand.target_count;

      if (holds_count * 20 >= 19 * total_sim || st.holds_percent() >= 95) continue;
      int success_pct = static_cast<int>(
          std::lround(100.0 * cand.target_count / std::max(1, holds_count)));
      if (cand.target_count * 2 <= holds_count || success_pct <= 50) continue;

      BitsetTripleSig sig{std::move(cand.target_bits),
                          std::move(cand.opposing_bits),
                          std::move(cand.eval_bits)};
      auto [it, ins] = dedup_map.emplace(std::move(sig), deduped.size());
      if (ins) {
        deduped.push_back(std::move(st));
      } else if (IsBetterRule(st.rule, deduped[it->second].rule)) {
        deduped[it->second] = std::move(st);
      }
    }

    SuppressRedundantSuitHonorVariableInvariants(deduped);
    std::stable_sort(deduped.begin(), deduped.end(),
                     [](const InvariantStats& a, const InvariantStats& b) {
                       double sa = a.holds_count > 0
                                       ? static_cast<double>(a.success_when_holds_count) / a.holds_count
                                       : 0.0;
                       double sb = b.holds_count > 0
                                       ? static_cast<double>(b.success_when_holds_count) / b.holds_count
                                       : 0.0;
                       if (std::abs(sa - sb) > 1e-9) return sa > sb;
                       double ha = a.holds_rate(), hb = b.holds_rate();
                       return std::abs(ha - hb) > 1e-9 ? ha > hb : false;
                     });
    report.invariant_stats = std::move(deduped);
  } else {
    CrossSuitConditionalRuleExtractor conditional_extractor;
    for (const auto& deal : deals) {
      conditional_extractor.ProcessDeal(deal);
    }
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
                                honor_invariants.begin(),
                                honor_invariants.end());

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
