#include "cross_hand_summarizer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "conditional_line.h"
#include "glog/logging.h"
#include "unknown_cards_invariant.h"

static std::vector<PairedDeal> GenerateRandomBifurcations(
    uint64_t unknown_cards, int hand1_size, int num_bifurcations);

// Integer ceiling quantile check: holds_count >= ceil((num / den) * total)
// Equivalent to holds_count * den >= num * total for positive integers.
// For 9/10 (90%), total < 10 requires holds_count == total (100% agreement).
static inline bool MeetsCeilingQuantile(int holds_count,
                                        int total_deals,
                                        int num,
                                        int den) {
  return total_deals > 0 &&
         static_cast<int64_t>(holds_count) * den >=
             static_cast<int64_t>(num) * total_deals;
}

// --- DealBitset Implementation ---

DealBitset::DealBitset(size_t num_bits)
    : num_bits_(num_bits), num_words_((num_bits + 63) / 64) {
  if (num_words_ > kInlineWords) {
    heap_words_.assign(num_words_, 0ULL);
  }
}

void DealBitset::Set(size_t bit_idx) {
  if (bit_idx >= num_bits_) return;
  mutable_words()[bit_idx >> 6] |= (1ULL << (bit_idx & 63));
}

bool DealBitset::Test(size_t bit_idx) const {
  if (bit_idx >= num_bits_) return false;
  return (words()[bit_idx >> 6] & (1ULL << (bit_idx & 63))) != 0;
}

int DealBitset::PopCount() const {
  const uint64_t* w = words();
  int total = 0;
  for (size_t i = 0; i < num_words_; ++i) {
    total += PopCount64(w[i]);
  }
  return total;
}

bool DealBitset::Empty() const {
  const uint64_t* w = words();
  for (size_t i = 0; i < num_words_; ++i) {
    if (w[i] != 0) return false;
  }
  return true;
}

DealBitset DealBitset::operator&(const DealBitset& other) const {
  size_t n = std::max(num_bits_, other.num_bits_);
  DealBitset out(n);
  size_t min_w = std::min(num_words_, other.num_words_);
  const uint64_t* wa = words();
  const uint64_t* wb = other.words();
  uint64_t* wo = out.mutable_words();
  for (size_t i = 0; i < min_w; ++i) {
    wo[i] = wa[i] & wb[i];
  }
  return out;
}

DealBitset DealBitset::operator|(const DealBitset& other) const {
  size_t n = std::max(num_bits_, other.num_bits_);
  DealBitset out(n);
  const uint64_t* wa = words();
  const uint64_t* wb = other.words();
  uint64_t* wo = out.mutable_words();
  for (size_t i = 0; i < num_words_; ++i) {
    wo[i] |= wa[i];
  }
  for (size_t i = 0; i < other.num_words_; ++i) {
    wo[i] |= wb[i];
  }
  return out;
}

int DealBitset::AndPopCount(const DealBitset& other) const {
  const uint64_t* wa = words();
  const uint64_t* wb = other.words();
  size_t min_w = std::min(num_words_, other.num_words_);
  int count = 0;
  for (size_t i = 0; i < min_w; ++i) {
    count += PopCount64(wa[i] & wb[i]);
  }
  return count;
}

int DealBitset::OrPopCount(const DealBitset& other) const {
  const uint64_t* wa = words();
  const uint64_t* wb = other.words();
  size_t min_w = std::min(num_words_, other.num_words_);
  int count = 0;
  for (size_t i = 0; i < min_w; ++i) {
    count += PopCount64(wa[i] | wb[i]);
  }
  for (size_t i = min_w; i < num_words_; ++i) {
    count += PopCount64(wa[i]);
  }
  for (size_t i = min_w; i < other.num_words_; ++i) {
    count += PopCount64(wb[i]);
  }
  return count;
}

int DealBitset::AndNotCount(const DealBitset& other) const {
  const uint64_t* wa = words();
  const uint64_t* wb = other.words();
  int count = 0;
  size_t min_w = std::min(num_words_, other.num_words_);
  for (size_t i = 0; i < min_w; ++i) {
    count += PopCount64(wa[i] & ~wb[i]);
  }
  for (size_t i = min_w; i < num_words_; ++i) {
    count += PopCount64(wa[i]);
  }
  return count;
}

bool DealBitset::IsSubsetOf(const DealBitset& other) const {
  return AndNotCount(other) == 0;
}

bool DealBitset::operator==(const DealBitset& other) const {
  if (num_bits_ != other.num_bits_) return false;
  const uint64_t* wa = words();
  const uint64_t* wb = other.words();
  for (size_t i = 0; i < num_words_; ++i) {
    if (wa[i] != wb[i]) return false;
  }
  return true;
}

bool DealBitset::operator<(const DealBitset& other) const {
  if (num_bits_ != other.num_bits_) return num_bits_ < other.num_bits_;
  const uint64_t* wa = words();
  const uint64_t* wb = other.words();
  for (size_t i = 0; i < num_words_; ++i) {
    if (wa[i] != wb[i]) return wa[i] < wb[i];
  }
  return false;
}

// --- AtomicPredicateCatalog Implementation ---

namespace {

struct PrecomputedDealFeatures {
  uint64_t h1 = 0;
  uint64_t h2 = 0;
  int len1[4] = {0, 0, 0, 0};
  int len2[4] = {0, 0, 0, 0};
};

std::vector<PrecomputedDealFeatures> BuildDealFeatures(
    const std::vector<PairedDeal>& deals) {
  std::vector<PrecomputedDealFeatures> feats(deals.size());
  for (size_t i = 0; i < deals.size(); ++i) {
    feats[i].h1 = deals[i].hand1;
    feats[i].h2 = deals[i].hand2;
    for (int s = 0; s < 4; ++s) {
      Suit suit = static_cast<Suit>(s);
      feats[i].len1[s] = CardsInSuit(deals[i].hand1, suit);
      feats[i].len2[s] = CardsInSuit(deals[i].hand2, suit);
    }
  }
  return feats;
}

template <typename PredFn>
DealBitset EvalPredicateBitset(const std::vector<PrecomputedDealFeatures>& feats,
                               PredFn&& pred) {
  DealBitset bits(feats.size());
  uint64_t* w = bits.mutable_words();
  for (size_t i = 0; i < feats.size(); ++i) {
    if (pred(feats[i])) {
      w[i >> 6] |= (1ULL << (i & 63));
    }
  }
  return bits;
}

bool IsBetterCanonicalRule(const std::string& cand, const std::string& cur) {
  if (cand.size() != cur.size()) return cand.size() < cur.size();
  return cand < cur;
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
  std::vector<std::pair<std::string, std::shared_ptr<UnknownCardsInvariant>>> v =
      {{a->ToString(), std::move(a)},
       {b->ToString(), std::move(b)},
       {c->ToString(), std::move(c)}};
  std::sort(v.begin(), v.end(),
            [](const auto& x, const auto& y) { return x.first < y.first; });
  auto left = std::make_shared<OrInvariant>(std::move(v[0].second),
                                            std::move(v[1].second));
  return std::make_shared<OrInvariant>(std::move(left), std::move(v[2].second));
}

std::shared_ptr<UnknownCardsInvariant> MakeCanonicalAnd3(
    std::shared_ptr<UnknownCardsInvariant> a,
    std::shared_ptr<UnknownCardsInvariant> b,
    std::shared_ptr<UnknownCardsInvariant> c) {
  std::vector<std::pair<std::string, std::shared_ptr<UnknownCardsInvariant>>> v =
      {{a->ToString(), std::move(a)},
       {b->ToString(), std::move(b)},
       {c->ToString(), std::move(c)}};
  std::sort(v.begin(), v.end(),
            [](const auto& x, const auto& y) { return x.first < y.first; });
  auto left = std::make_shared<AndInvariant>(std::move(v[0].second),
                                             std::move(v[1].second));
  return std::make_shared<AndInvariant>(std::move(left),
                                        std::move(v[2].second));
}

// Strict precision comparison: tc_a / (tc_a + oc_a) > tc_b / (tc_b + oc_b)
inline bool StrictlyBeatsPrecision(int tc_a, int oc_a, int tc_b, int oc_b) {
  int64_t denom_a = static_cast<int64_t>(tc_a) + oc_a;
  int64_t denom_b = static_cast<int64_t>(tc_b) + oc_b;
  if (denom_a <= 0) return false;
  if (denom_b <= 0) return tc_a > 0;
  return static_cast<int64_t>(tc_a) * denom_b >
         static_cast<int64_t>(tc_b) * denom_a;
}

struct BitsetSignature {
  DealBitset target;
  DealBitset opposing;
  DealBitset eval;

  bool operator==(const BitsetSignature& o) const {
    return target == o.target && opposing == o.opposing && eval == o.eval;
  }

  bool operator<(const BitsetSignature& o) const {
    if (target != o.target) return target < o.target;
    if (opposing != o.opposing) return opposing < o.opposing;
    return eval < o.eval;
  }
};

struct BitsetSignatureHash {
  size_t operator()(const BitsetSignature& s) const noexcept {
    uint64_t h = 0x9e3779b97f4a7c15ULL;
    auto mix_bits = [&](const DealBitset& b) {
      h ^= b.size() + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
      const uint64_t* w = b.words();
      size_t nw = b.num_words();
      for (size_t i = 0; i < nw; ++i) {
        h ^= w[i] + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
      }
    };
    mix_bits(s.target);
    mix_bits(s.opposing);
    mix_bits(s.eval);
    return static_cast<size_t>(h);
  }
};

struct PairBitsetSignature {
  DealBitset target;
  DealBitset opposing;

  bool operator==(const PairBitsetSignature& o) const {
    return target == o.target && opposing == o.opposing;
  }
};

struct PairBitsetSignatureHash {
  size_t operator()(const PairBitsetSignature& s) const noexcept {
    uint64_t h = 0x9e3779b97f4a7c15ULL;
    auto mix_bits = [&](const DealBitset& b) {
      h ^= b.size() + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
      const uint64_t* w = b.words();
      size_t nw = b.num_words();
      for (size_t i = 0; i < nw; ++i) {
        h ^= w[i] + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
      }
    };
    mix_bits(s.target);
    mix_bits(s.opposing);
    return static_cast<size_t>(h);
  }
};

// Compare canonical 2-clause or 3-clause part lists without allocating strings:
// shorter total length wins; ties broken lexicographically on sorted part names.
bool IsBetterPartPair(const std::string& a0,
                      const std::string& a1,
                      const std::string& b0,
                      const std::string& b1) {
  size_t la = a0.size() + a1.size();
  size_t lb = b0.size() + b1.size();
  if (la != lb) return la < lb;
  const std::string& a_lo = (a0 < a1) ? a0 : a1;
  const std::string& a_hi = (a0 < a1) ? a1 : a0;
  const std::string& b_lo = (b0 < b1) ? b0 : b1;
  const std::string& b_hi = (b0 < b1) ? b1 : b0;
  if (a_lo != b_lo) return a_lo < b_lo;
  return a_hi < b_hi;
}

bool IsBetterPartTriple(const std::string& a0,
                        const std::string& a1,
                        const std::string& a2,
                        const std::string& b0,
                        const std::string& b1,
                        const std::string& b2) {
  size_t la = a0.size() + a1.size() + a2.size();
  size_t lb = b0.size() + b1.size() + b2.size();
  if (la != lb) return la < lb;
  const std::string* av[3] = {&a0, &a1, &a2};
  const std::string* bv[3] = {&b0, &b1, &b2};
  auto cmp = [](const std::string* x, const std::string* y) { return *x < *y; };
  std::sort(av, av + 3, cmp);
  std::sort(bv, bv + 3, cmp);
  for (int i = 0; i < 3; ++i) {
    if (*av[i] != *bv[i]) return *av[i] < *bv[i];
  }
  return false;
}

}  // namespace

AtomicPredicateCatalog::AtomicPredicateCatalog(
    const std::vector<PairedDeal>& target_deals,
    const std::vector<PairedDeal>* opposing_deals,
    const HandLabelFormatter& formatter,
    const std::vector<PairedDeal>* eval_bifurcations)
    : target_size_(static_cast<int>(target_deals.size())),
      opposing_size_(opposing_deals ? static_cast<int>(opposing_deals->size())
                                    : 0),
      formatter_(formatter) {
  if (target_deals.empty()) return;

  unknown_cards_ = 0;
  for (const auto& d : target_deals) {
    unknown_cards_ |= (d.hand1 | d.hand2);
  }
  if (opposing_deals) {
    for (const auto& d : *opposing_deals) {
      unknown_cards_ |= (d.hand1 | d.hand2);
    }
  }
  cards_per_hand_ = CountCardsInHand(target_deals[0].hand1);

  auto target_feats = BuildDealFeatures(target_deals);
  std::vector<PrecomputedDealFeatures> opposing_feats;
  if (opposing_deals && !opposing_deals->empty()) {
    opposing_feats = BuildDealFeatures(*opposing_deals);
  }
  std::vector<PrecomputedDealFeatures> eval_feats;
  if (eval_bifurcations && !eval_bifurcations->empty()) {
    eval_feats = BuildDealFeatures(*eval_bifurcations);
  }

  auto add_lazy_entry = [&](AtomicPredicateFamily family,
                            auto&& pred,
                            auto&& make_invs,
                            Suit h_suit = Suit::SPADES,
                            HonorPossessionTargetHand h_target =
                                HonorPossessionTargetHand::kHand1) -> int {
    DealBitset tb = EvalPredicateBitset(target_feats, pred);
    int tc = tb.PopCount();
    if (tc == 0) return 0;
    AtomicPredicateEntry e;
    e.target_bits = std::move(tb);
    e.target_count = tc;
    if (!opposing_feats.empty()) {
      e.opposing_bits = EvalPredicateBitset(opposing_feats, pred);
      e.opposing_count = e.opposing_bits.PopCount();
    } else {
      e.opposing_bits = DealBitset(0);
      e.opposing_count = 0;
    }
    if (!eval_feats.empty()) {
      e.eval_bits = EvalPredicateBitset(eval_feats, pred);
    }
    auto [inv, comp_inv] = make_invs();
    e.invariant = std::move(inv);
    e.composite_invariant = comp_inv ? std::move(comp_inv) : e.invariant;
    e.family = family;
    e.honor_suit = h_suit;
    e.honor_target = h_target;
    e.rule = e.invariant->ToString();
    entries_.push_back(std::move(e));
    return tc;
  };

  std::string l1 = formatter_.FormatHand1();
  std::string l2 = formatter_.FormatHand2();
  std::string l1_comp = formatter_.FormatCompositeHand1();
  std::string l2_comp = formatter_.FormatCompositeHand2();

  int total_unknown = CountCardsInHand(unknown_cards_);
  int h1_size = (cards_per_hand_ > 0) ? cards_per_hand_ : (total_unknown / 2);
  int h2_size = std::max(0, total_unknown - h1_size);

  // 1. HonorPossessionInvariant & SuitHonorVariableInvariant (ranks 8..12 only)
  std::vector<CardType> movable_honors;
  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    uint64_t suit_honor_mask = 0x1F00ULL << (s * 16);
    uint64_t present_suit_honors = unknown_cards_ & suit_honor_mask;
    if (present_suit_honors == 0) continue;

    for (int r = 8; r <= 12; ++r) {
      CardType card = Card::GetSuitedCard(suit, r);
      if ((unknown_cards_ & card) == 0) continue;

      int h1_cnt = add_lazy_entry(
          AtomicPredicateFamily::kHonorPossession,
          [card](const PrecomputedDealFeatures& f) {
            return (f.h1 & card) != 0;
          },
          [&]() -> std::pair<std::shared_ptr<UnknownCardsInvariant>,
                             std::shared_ptr<UnknownCardsInvariant>> {
            return {std::make_shared<HonorPossessionInvariant>(
                        HonorPossessionTargetHand::kHand1, card, l1),
                    std::make_shared<HonorPossessionInvariant>(
                        HonorPossessionTargetHand::kHand1, card, l1_comp)};
          },
          suit, HonorPossessionTargetHand::kHand1);

      int h2_cnt = add_lazy_entry(
          AtomicPredicateFamily::kHonorPossession,
          [card](const PrecomputedDealFeatures& f) {
            return (f.h2 & card) != 0;
          },
          [&]() -> std::pair<std::shared_ptr<UnknownCardsInvariant>,
                             std::shared_ptr<UnknownCardsInvariant>> {
            return {std::make_shared<HonorPossessionInvariant>(
                        HonorPossessionTargetHand::kHand2, card, l2),
                    std::make_shared<HonorPossessionInvariant>(
                        HonorPossessionTargetHand::kHand2, card, l2_comp)};
          },
          suit, HonorPossessionTargetHand::kHand2);

      add_lazy_entry(
          AtomicPredicateFamily::kHonorPossession,
          [card](const PrecomputedDealFeatures& f) {
            return (f.h1 & card) != 0 && (f.h2 & card) != 0;
          },
          [&]() -> std::pair<std::shared_ptr<UnknownCardsInvariant>,
                             std::shared_ptr<UnknownCardsInvariant>> {
            auto inv = std::make_shared<HonorPossessionInvariant>(
                HonorPossessionTargetHand::kBoth, card, "both hands");
            return {inv, inv};
          },
          suit, HonorPossessionTargetHand::kBoth);

      if (h1_cnt > 0 && h2_cnt > 0) {
        movable_honors.push_back(card);
      }
    }

    add_lazy_entry(
        AtomicPredicateFamily::kSuitHonorVariable,
        [suit_honor_mask](const PrecomputedDealFeatures& f) {
          return (f.h1 & suit_honor_mask) != 0;
        },
        [&]() -> std::pair<std::shared_ptr<UnknownCardsInvariant>,
                           std::shared_ptr<UnknownCardsInvariant>> {
          auto inv = std::make_shared<SuitHonorVariableInvariant>(
              HonorPossessionTargetHand::kHand1, suit, l1);
          return {inv, inv};
        },
        suit, HonorPossessionTargetHand::kHand1);

    add_lazy_entry(
        AtomicPredicateFamily::kSuitHonorVariable,
        [suit_honor_mask](const PrecomputedDealFeatures& f) {
          return (f.h2 & suit_honor_mask) != 0;
        },
        [&]() -> std::pair<std::shared_ptr<UnknownCardsInvariant>,
                           std::shared_ptr<UnknownCardsInvariant>> {
          auto inv = std::make_shared<SuitHonorVariableInvariant>(
              HonorPossessionTargetHand::kHand2, suit, l2);
          return {inv, inv};
        },
        suit, HonorPossessionTargetHand::kHand2);

    if (PopCount64(present_suit_honors) >= 2) {
      add_lazy_entry(
          AtomicPredicateFamily::kSuitHonorVariable,
          [suit_honor_mask](const PrecomputedDealFeatures& f) {
            return (f.h1 & suit_honor_mask) != 0 &&
                   (f.h2 & suit_honor_mask) != 0;
          },
          [&]() -> std::pair<std::shared_ptr<UnknownCardsInvariant>,
                             std::shared_ptr<UnknownCardsInvariant>> {
            auto inv = std::make_shared<SuitHonorVariableInvariant>(
                HonorPossessionTargetHand::kBoth, suit, "both hands");
            return {inv, inv};
          },
          suit, HonorPossessionTargetHand::kBoth);
    }
  }

  // 2. Suit-length & Shortest-split atoms
  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    int total_suit = CardsInSuit(unknown_cards_, suit);
    if (total_suit == 0 || total_suit == total_unknown) {
      continue;
    }

    int min1 = (total_unknown > 0 && h1_size > 0 && h1_size < total_unknown)
                   ? std::max(0, total_suit - h2_size)
                   : 0;
    int max1 = (total_unknown > 0 && h1_size > 0 && h1_size < total_unknown)
                   ? std::min(total_suit, h1_size)
                   : total_suit;
    int min2 = (total_unknown > 0 && h1_size > 0 && h1_size < total_unknown)
                   ? std::max(0, total_suit - h1_size)
                   : 0;
    int max2 = (total_unknown > 0 && h1_size > 0 && h1_size < total_unknown)
                   ? std::min(total_suit, h2_size)
                   : total_suit;

    if (min1 < max1) {
      for (int len = min1; len <= max1; ++len) {
        add_lazy_entry(
            AtomicPredicateFamily::kExactSuitLength,
            [s, len](const PrecomputedDealFeatures& f) {
              return f.len1[s] == len;
            },
            [&]() -> std::pair<std::shared_ptr<UnknownCardsInvariant>,
                               std::shared_ptr<UnknownCardsInvariant>> {
              auto inv = std::make_shared<ExactSuitLengthInvariant>(
                  InvariantHandChoice::kHand1, suit, len, l1);
              return {inv, inv};
            });
      }
      for (int lo = min1; lo <= max1; ++lo) {
        for (int hi = lo + 1; hi <= max1; ++hi) {
          if (lo == min1 && hi == max1) continue;
          if (lo == 0 && hi == total_suit) continue;
          add_lazy_entry(
              AtomicPredicateFamily::kSuitLengthRange,
              [s, lo, hi](const PrecomputedDealFeatures& f) {
                return f.len1[s] >= lo && f.len1[s] <= hi;
              },
              [&]() -> std::pair<std::shared_ptr<UnknownCardsInvariant>,
                                 std::shared_ptr<UnknownCardsInvariant>> {
                auto inv = std::make_shared<SuitLengthRangeInvariant>(
                    InvariantHandChoice::kHand1, suit, lo, hi, l1);
                return {inv, inv};
              });
        }
      }
    }

    if (min2 < max2) {
      for (int len = min2; len <= max2; ++len) {
        add_lazy_entry(
            AtomicPredicateFamily::kExactSuitLength,
            [s, len](const PrecomputedDealFeatures& f) {
              return f.len2[s] == len;
            },
            [&]() -> std::pair<std::shared_ptr<UnknownCardsInvariant>,
                               std::shared_ptr<UnknownCardsInvariant>> {
              auto inv = std::make_shared<ExactSuitLengthInvariant>(
                  InvariantHandChoice::kHand2, suit, len, l2);
              return {inv, inv};
            });
      }
      for (int lo = min2; lo <= max2; ++lo) {
        for (int hi = lo + 1; hi <= max2; ++hi) {
          if (lo == min2 && hi == max2) continue;
          if (lo == 0 && hi == total_suit) continue;
          add_lazy_entry(
              AtomicPredicateFamily::kSuitLengthRange,
              [s, lo, hi](const PrecomputedDealFeatures& f) {
                return f.len2[s] >= lo && f.len2[s] <= hi;
              },
              [&]() -> std::pair<std::shared_ptr<UnknownCardsInvariant>,
                                 std::shared_ptr<UnknownCardsInvariant>> {
                auto inv = std::make_shared<SuitLengthRangeInvariant>(
                    InvariantHandChoice::kHand2, suit, lo, hi, l2);
                return {inv, inv};
              });
        }
      }
    }

    int min_max_either = (total_suit + 1) / 2;
    int max_max_either = std::max(max1, max2);
    for (int m = min_max_either; m < max_max_either; ++m) {
      add_lazy_entry(
          AtomicPredicateFamily::kMaxSuitLength,
          [s, m](const PrecomputedDealFeatures& f) {
            return std::max(f.len1[s], f.len2[s]) <= m;
          },
          [&]() -> std::pair<std::shared_ptr<UnknownCardsInvariant>,
                             std::shared_ptr<UnknownCardsInvariant>> {
            auto inv = std::make_shared<MaxSuitLengthInvariant>(suit, m);
            return {inv, inv};
          });
    }

    int max_shortest = total_suit / 2;
    int min_shortest = std::min(min1, min2);
    for (int k = std::max(1, min_shortest + 1); k <= max_shortest; ++k) {
      add_lazy_entry(
          AtomicPredicateFamily::kShortestLength,
          [s, k](const PrecomputedDealFeatures& f) {
            return std::min(f.len1[s], f.len2[s]) >= k;
          },
          [&]() -> std::pair<std::shared_ptr<UnknownCardsInvariant>,
                             std::shared_ptr<UnknownCardsInvariant>> {
            auto inv = std::make_shared<ShortestLengthInvariant>(
                suit, ShortestLengthInvariant::Comparison::kAtLeast, k);
            return {inv, inv};
          });
    }
    for (int k = min_shortest; k < max_shortest; ++k) {
      add_lazy_entry(
          AtomicPredicateFamily::kShortestLength,
          [s, k](const PrecomputedDealFeatures& f) {
            return std::min(f.len1[s], f.len2[s]) <= k;
          },
          [&]() -> std::pair<std::shared_ptr<UnknownCardsInvariant>,
                             std::shared_ptr<UnknownCardsInvariant>> {
            auto inv = std::make_shared<ShortestLengthInvariant>(
                suit, ShortestLengthInvariant::Comparison::kAtMost, k);
            return {inv, inv};
          });
    }
  }

  // 3. Cross-suit conditional atoms (honors 8..12 only, movable across hands)
  for (CardType cond_card : movable_honors) {
    Suit cond_suit = Card::DecodeCard(cond_card).first;
    for (int s = 0; s < 4; ++s) {
      Suit suit = static_cast<Suit>(s);
      int total_suit = CardsInSuit(unknown_cards_, suit);
      if (total_suit == 0) continue;

      int in_suit = (cond_suit == suit) ? 1 : 0;
      int rem_in_suit = total_suit - in_suit;
      int rem_outside = (total_unknown - 1) - rem_in_suit;

      auto min_with_sz = [&](int sz) {
        if (sz <= 0) return in_suit;
        return in_suit + std::max(0, (sz - 1) - rem_outside);
      };
      auto max_with_sz = [&](int sz) {
        if (sz <= 0) return in_suit;
        return in_suit + std::min(rem_in_suit, sz - 1);
      };

      int min_p = (total_unknown > 0 && h1_size > 0 && h1_size < total_unknown)
                      ? std::min(min_with_sz(h1_size), min_with_sz(h2_size))
                      : in_suit;
      int max_p = (total_unknown > 0 && h1_size > 0 && h1_size < total_unknown)
                      ? std::max(max_with_sz(h1_size), max_with_sz(h2_size))
                      : total_suit;

      if (min_p >= max_p) continue;

      for (int n = min_p; n <= max_p; ++n) {
        add_lazy_entry(
            AtomicPredicateFamily::kCrossSuitLength,
            [cond_card, s, n](const PrecomputedDealFeatures& f) {
              if ((f.h1 & cond_card) != 0) return f.len1[s] == n;
              if ((f.h2 & cond_card) != 0) return f.len2[s] == n;
              return false;
            },
            [&]() -> std::pair<std::shared_ptr<UnknownCardsInvariant>,
                               std::shared_ptr<UnknownCardsInvariant>> {
              auto inv = std::make_shared<CrossSuitExactSuitLengthInvariant>(
                  cond_card, suit, n);
              return {inv, inv};
            });
      }
      for (int n = min_p + 1; n < max_p; ++n) {
        add_lazy_entry(
            AtomicPredicateFamily::kCrossSuitLength,
            [cond_card, s, n](const PrecomputedDealFeatures& f) {
              if ((f.h1 & cond_card) != 0) return f.len1[s] >= n;
              if ((f.h2 & cond_card) != 0) return f.len2[s] >= n;
              return false;
            },
            [&]() -> std::pair<std::shared_ptr<UnknownCardsInvariant>,
                               std::shared_ptr<UnknownCardsInvariant>> {
              auto inv = std::make_shared<CrossSuitMinSuitLengthInvariant>(
                  cond_card, suit, n);
              return {inv, inv};
            });

        add_lazy_entry(
            AtomicPredicateFamily::kCrossSuitLength,
            [cond_card, s, n](const PrecomputedDealFeatures& f) {
              if ((f.h1 & cond_card) != 0) return f.len1[s] <= n;
              if ((f.h2 & cond_card) != 0) return f.len2[s] <= n;
              return false;
            },
            [&]() -> std::pair<std::shared_ptr<UnknownCardsInvariant>,
                               std::shared_ptr<UnknownCardsInvariant>> {
              auto inv = std::make_shared<CrossSuitMaxSuitLengthInvariant>(
                  cond_card, suit, n);
              return {inv, inv};
            });
      }
    }

    for (int s2 = 0; s2 < 4; ++s2) {
      Suit suit2 = static_cast<Suit>(s2);
      for (int r2 = 8; r2 <= 12; ++r2) {
        CardType target_card = Card::GetSuitedCard(suit2, r2);
        if (target_card == cond_card || (unknown_cards_ & target_card) == 0) {
          continue;
        }
        add_lazy_entry(
            AtomicPredicateFamily::kCrossSuitCard,
            [cond_card, target_card](const PrecomputedDealFeatures& f) {
              if ((f.h1 & cond_card) != 0) {
                return (f.h1 & target_card) != 0;
              }
              if ((f.h2 & cond_card) != 0) {
                return (f.h2 & target_card) != 0;
              }
              return false;
            },
            [&]() -> std::pair<std::shared_ptr<UnknownCardsInvariant>,
                               std::shared_ptr<UnknownCardsInvariant>> {
              auto inv = std::make_shared<CrossSuitCardConditionalInvariant>(
                  cond_card, target_card);
              return {inv, inv};
            });
      }
    }
  }
}

std::vector<AtomicPredicateEntry>
AtomicPredicateCatalog::DeduplicatedEntries() const {
  std::unordered_map<BitsetSignature, size_t, BitsetSignatureHash> sig_to_idx;
  sig_to_idx.reserve(entries_.size());
  std::vector<AtomicPredicateEntry> deduped;
  deduped.reserve(entries_.size());

  // Track specific near-universal honor holdings so redundant $honor variables
  // do not leak into standalone or compound synthesis.
  std::set<std::pair<Suit, HonorPossessionTargetHand>> specific_honor_covered;
  for (const auto& e : entries_) {
    if (e.family == AtomicPredicateFamily::kHonorPossession &&
        MeetsCeilingQuantile(e.target_count, target_size_, 9, 10)) {
      specific_honor_covered.insert({e.honor_suit, e.honor_target});
    }
  }

  for (const auto& e : entries_) {
    if (e.target_count == 0) continue;
    if (e.family == AtomicPredicateFamily::kSuitHonorVariable) {
      Suit s = e.honor_suit;
      auto t = e.honor_target;
      if (t == HonorPossessionTargetHand::kHand1 &&
          (specific_honor_covered.count(
               {s, HonorPossessionTargetHand::kHand1}) > 0 ||
           specific_honor_covered.count(
               {s, HonorPossessionTargetHand::kBoth}) > 0)) {
        continue;
      }
      if (t == HonorPossessionTargetHand::kHand2 &&
          (specific_honor_covered.count(
               {s, HonorPossessionTargetHand::kHand2}) > 0 ||
           specific_honor_covered.count(
               {s, HonorPossessionTargetHand::kBoth}) > 0)) {
        continue;
      }
      if (t == HonorPossessionTargetHand::kBoth) {
        bool h1_cov = specific_honor_covered.count(
                          {s, HonorPossessionTargetHand::kHand1}) > 0 ||
                      specific_honor_covered.count(
                          {s, HonorPossessionTargetHand::kBoth}) > 0;
        bool h2_cov = specific_honor_covered.count(
                          {s, HonorPossessionTargetHand::kHand2}) > 0 ||
                      specific_honor_covered.count(
                          {s, HonorPossessionTargetHand::kBoth}) > 0;
        if (h1_cov && h2_cov) continue;
        if (!MeetsCeilingQuantile(e.target_count, target_size_, 9, 10) &&
            (h1_cov || h2_cov)) {
          continue;
        }
      }
    }

    BitsetSignature sig{e.target_bits, e.opposing_bits, e.eval_bits};
    auto it = sig_to_idx.find(sig);
    if (it == sig_to_idx.end()) {
      sig_to_idx.emplace(std::move(sig), deduped.size());
      deduped.push_back(e);
    } else {
      size_t idx = it->second;
      if (IsBetterCanonicalRule(e.rule, deduped[idx].rule)) {
        deduped[idx] = e;
      }
    }
  }
  return deduped;
}

// --- SetCoverSynthesizer Implementation ---

bool SetCoverSynthesizer::PassesUniformRecallOrContrastGate(
    int target_count,
    int target_total,
    int opposing_count,
    int opposing_total) {
  if (target_total <= 0 || target_count <= 0) return false;
  if (opposing_total <= 0) {
    return target_count == target_total;
  }
  if (static_cast<int64_t>(target_count) * opposing_total <=
      static_cast<int64_t>(opposing_count) * target_total) {
    return false;
  }
  bool high_recall = MeetsCeilingQuantile(target_count, target_total, 9, 10);
  bool high_contrast =
      MeetsCeilingQuantile(target_count, target_total, 3, 4) &&
      static_cast<int64_t>(opposing_count) * 10 <= opposing_total;
  return high_recall || high_contrast;
}

std::vector<SynthesizedCandidate> SetCoverSynthesizer::Synthesize(
    const AtomicPredicateCatalog& catalog) {
  std::vector<SynthesizedCandidate> results;
  const int n_t = catalog.target_size();
  const int n_o = catalog.opposing_size();
  if (n_t <= 0) return results;

  std::vector<AtomicPredicateEntry> raw_atoms = catalog.DeduplicatedEntries();
  if (raw_atoms.empty()) return results;

  std::unordered_map<PairBitsetSignature, size_t, PairBitsetSignatureHash>
      atom_pair_map;
  atom_pair_map.reserve(raw_atoms.size());
  std::vector<AtomicPredicateEntry> atoms;
  std::vector<std::string> atom_comp_rules;
  atoms.reserve(raw_atoms.size());
  atom_comp_rules.reserve(raw_atoms.size());
  std::set<std::pair<Suit, HonorPossessionTargetHand>> shv_covered;

  for (auto& a : raw_atoms) {
    if (a.family == AtomicPredicateFamily::kSuitHonorVariable &&
        PassesUniformRecallOrContrastGate(a.target_count, n_t, a.opposing_count,
                                          n_o)) {
      shv_covered.insert({a.honor_suit, a.honor_target});
    }
    if (PassesUniformRecallOrContrastGate(a.target_count, n_t, a.opposing_count,
                                          n_o)) {
      SynthesizedCandidate sc;
      sc.invariant = a.invariant;
      sc.rule = a.rule;
      sc.target_bits = a.target_bits;
      sc.opposing_bits = a.opposing_bits;
      sc.eval_bits = a.eval_bits;
      sc.target_count = a.target_count;
      sc.opposing_count = a.opposing_count;
      sc.clause_count = 1;
      results.push_back(std::move(sc));
    }

    std::string comp_rule = (a.composite_invariant == a.invariant)
                                ? a.rule
                                : a.composite_invariant->ToString();
    PairBitsetSignature psig{a.target_bits, a.opposing_bits};
    auto it = atom_pair_map.find(psig);
    if (it == atom_pair_map.end()) {
      atom_pair_map.emplace(std::move(psig), atoms.size());
      atoms.push_back(std::move(a));
      atom_comp_rules.push_back(std::move(comp_rule));
    } else {
      size_t idx = it->second;
      if (IsBetterCanonicalRule(comp_rule, atom_comp_rules[idx])) {
        atoms[idx] = std::move(a);
        atom_comp_rules[idx] = std::move(comp_rule);
      }
    }
  }

  struct BeamNode {
    std::shared_ptr<UnknownCardsInvariant> invariant;
    std::string rule;
    DealBitset target_bits;
    DealBitset opposing_bits;
    DealBitset eval_bits;
    int target_count = 0;
    int opposing_count = 0;
    size_t p0 = 0;
    size_t p1 = 0;
    size_t p2 = 0;
    int arity = 1;
  };

  // --- Pass 1: Width-Bounded Bitwise AND Beam Pass (width <= 32, depth <= 3) ---
  std::vector<BeamNode> sharp_2ands;
  if (n_o > 0) {
    const int64_t total_sim = static_cast<int64_t>(n_t) + n_o;
    std::vector<std::pair<double, size_t>> scored_and_pool;
    scored_and_pool.reserve(atoms.size());
    for (size_t i = 0; i < atoms.size(); ++i) {
      const auto& a = atoms[i];
      if (a.target_count <= 0) continue;
      if (n_t >= 10 && a.target_count < 2) continue;
      // Exclude overall >=95% near-tautologies from conjunction seeding
      if (static_cast<int64_t>(a.target_count + a.opposing_count) * 20 >=
          19 * total_sim) {
        continue;
      }
      if (static_cast<int64_t>(a.target_count) * n_o >
              static_cast<int64_t>(a.opposing_count) * n_t ||
          MeetsCeilingQuantile(a.target_count, n_t, 3, 4)) {
        double rec = static_cast<double>(a.target_count) / n_t;
        double opp_r = static_cast<double>(a.opposing_count) / n_o;
        double sprec = static_cast<double>(a.target_count) /
                       (a.target_count + a.opposing_count + 1.5);
        double score = (rec - opp_r) + 0.30 * sprec;
        scored_and_pool.emplace_back(score, i);
      }
    }

    std::stable_sort(scored_and_pool.begin(), scored_and_pool.end(),
                     [&](const auto& x, const auto& y) {
                       if (std::abs(x.first - y.first) > 1e-9) {
                         return x.first > y.first;
                       }
                       return IsBetterCanonicalRule(atom_comp_rules[x.second],
                                                    atom_comp_rules[y.second]);
                     });
    std::vector<size_t> and_pool_idx;
    const size_t pool_cap = std::min<size_t>(48, scored_and_pool.size());
    and_pool_idx.reserve(pool_cap);
    for (size_t k = 0; k < pool_cap; ++k) {
      and_pool_idx.push_back(scored_and_pool[k].second);
    }

    std::vector<BeamNode> and_beam;
    and_beam.reserve(std::min(kBeamWidth, and_pool_idx.size()));
    for (size_t k = 0; k < std::min(kBeamWidth, and_pool_idx.size()); ++k) {
      size_t idx = and_pool_idx[k];
      const auto& a = atoms[idx];
      BeamNode node;
      node.invariant = a.composite_invariant;
      node.rule = atom_comp_rules[idx];
      node.target_bits = a.target_bits;
      node.opposing_bits = a.opposing_bits;
      node.eval_bits = a.eval_bits;
      node.target_count = a.target_count;
      node.opposing_count = a.opposing_count;
      node.p0 = idx;
      node.arity = 1;
      and_beam.push_back(std::move(node));
    }

    for (int depth = 2; depth <= kMaxClauses; ++depth) {
      constexpr size_t kTableMask = 4095;
      std::vector<int32_t> hash_buckets(kTableMask + 1, -1);
      std::vector<BeamNode> cands;
      cands.reserve(256);
      PairBitsetSignatureHash hasher;

      for (size_t bi = 0; bi < and_beam.size(); ++bi) {
        const auto& b = and_beam[bi];
        if (b.opposing_count == 0) {
          continue;  // Already 100% precision; conjunction cannot beat precision
        }
        for (size_t pk = 0; pk < and_pool_idx.size(); ++pk) {
          if (depth == 2 && pk < and_beam.size() && pk <= bi) {
            continue;
          }
          size_t c_idx = and_pool_idx[pk];
          if (c_idx == b.p0 || (depth == 3 && c_idx == b.p1)) {
            continue;
          }
          const auto& c = atoms[c_idx];
          int tc = b.target_bits.AndPopCount(c.target_bits);
          if (tc == 0 || (n_t >= 10 && tc < 2)) continue;
          int oc = b.opposing_bits.AndPopCount(c.opposing_bits);

          // Requirement 6: Conjunction must strictly beat all sub-clauses in precision
          if (!StrictlyBeatsPrecision(tc, oc, b.target_count,
                                      b.opposing_count) ||
              !StrictlyBeatsPrecision(tc, oc, c.target_count,
                                      c.opposing_count)) {
            continue;
          }
          if (depth == 3) {
            const auto& p0 = atoms[b.p0];
            const auto& p1 = atoms[b.p1];
            if (!StrictlyBeatsPrecision(tc, oc, p0.target_count,
                                        p0.opposing_count) ||
                !StrictlyBeatsPrecision(tc, oc, p1.target_count,
                                        p1.opposing_count)) {
              continue;
            }
            int stc0 = p0.target_bits.AndPopCount(c.target_bits);
            int soc0 = p0.opposing_bits.AndPopCount(c.opposing_bits);
            if (!StrictlyBeatsPrecision(tc, oc, stc0, soc0)) continue;
            int stc1 = p1.target_bits.AndPopCount(c.target_bits);
            int soc1 = p1.opposing_bits.AndPopCount(c.opposing_bits);
            if (!StrictlyBeatsPrecision(tc, oc, stc1, soc1)) continue;
          }

          DealBitset tb = b.target_bits & c.target_bits;
          DealBitset ob = b.opposing_bits & c.opposing_bits;
          PairBitsetSignature sig{std::move(tb), std::move(ob)};
          if (atom_pair_map.find(sig) != atom_pair_map.end()) continue;

          size_t slot = hasher(sig) & kTableMask;
          int32_t found = -1;
          while (hash_buckets[slot] >= 0) {
            int32_t idx = hash_buckets[slot];
            if (cands[idx].target_bits == sig.target &&
                cands[idx].opposing_bits == sig.opposing) {
              found = idx;
              break;
            }
            slot = (slot + 1) & kTableMask;
          }
          if (found < 0) {
            BeamNode node;
            node.target_bits = std::move(sig.target);
            node.opposing_bits = std::move(sig.opposing);
            node.target_count = tc;
            node.opposing_count = oc;
            node.p0 = b.p0;
            node.p1 = (depth == 2) ? c_idx : b.p1;
            node.p2 = (depth == 3) ? c_idx : 0;
            node.arity = depth;
            hash_buckets[slot] = static_cast<int32_t>(cands.size());
            cands.push_back(std::move(node));
          } else {
            BeamNode& cur = cands[found];
            bool better =
                (depth == 2)
                    ? IsBetterPartPair(atom_comp_rules[b.p0],
                                       atom_comp_rules[c_idx],
                                       atom_comp_rules[cur.p0],
                                       atom_comp_rules[cur.p1])
                    : IsBetterPartTriple(
                          atom_comp_rules[b.p0], atom_comp_rules[b.p1],
                          atom_comp_rules[c_idx], atom_comp_rules[cur.p0],
                          atom_comp_rules[cur.p1], atom_comp_rules[cur.p2]);
            if (better) {
              cur.p0 = b.p0;
              cur.p1 = (depth == 2) ? c_idx : b.p1;
              cur.p2 = (depth == 3) ? c_idx : 0;
            }
          }
        }
      }

      auto materialize_and = [&](BeamNode& node) {
        if (!node.invariant) {
          node.invariant =
              (node.arity == 2)
                  ? MakeCanonicalAnd(atoms[node.p0].composite_invariant,
                                     atoms[node.p1].composite_invariant)
                  : MakeCanonicalAnd3(atoms[node.p0].composite_invariant,
                                      atoms[node.p1].composite_invariant,
                                      atoms[node.p2].composite_invariant);
          node.rule = node.invariant->ToString();
          node.eval_bits =
              (node.arity == 2)
                  ? (atoms[node.p0].eval_bits & atoms[node.p1].eval_bits)
                  : ((atoms[node.p0].eval_bits & atoms[node.p1].eval_bits) &
                     atoms[node.p2].eval_bits);
        }
      };

      if (depth == 2 && !cands.empty()) {
        // Select non-dominated positive-lift >50%-precision 2-ANDs for OR set cover
        std::vector<std::pair<double, size_t>> sharp_order;
        sharp_order.reserve(cands.size());
        for (size_t i = 0; i < cands.size(); ++i) {
          const auto& x = cands[i];
          if (static_cast<int64_t>(x.target_count) * n_o >
                  static_cast<int64_t>(x.opposing_count) * n_t &&
              x.target_count > x.opposing_count &&
              static_cast<int64_t>(x.target_count) * 20 >= n_t &&
              !MeetsCeilingQuantile(x.target_count, n_t, 9, 10)) {
            double sprec = static_cast<double>(x.target_count) /
                           (x.target_count + x.opposing_count + 1.5);
            double rec = static_cast<double>(x.target_count) / n_t;
            double opp_r = static_cast<double>(x.opposing_count) / n_o;
            double sc = sprec * (rec - opp_r) + 0.35 * sprec + 0.25 * rec;
            sharp_order.emplace_back(sc, i);
          }
        }
        std::stable_sort(sharp_order.begin(), sharp_order.end(),
                         [&](const auto& a, const auto& b) {
                           if (std::abs(a.first - b.first) > 1e-9) {
                             return a.first > b.first;
                           }
                           return cands[a.second].target_count >
                                  cands[b.second].target_count;
                         });
        sharp_2ands.reserve(std::min<size_t>(24, sharp_order.size()));
        for (const auto& [sc, idx] : sharp_order) {
          auto& cand = cands[idx];
          bool dom = false;
          for (const auto& prev : sharp_2ands) {
            if (prev.opposing_count <= cand.opposing_count &&
                (prev.target_count > cand.target_count ||
                 prev.opposing_count < cand.opposing_count) &&
                cand.target_bits.IsSubsetOf(prev.target_bits)) {
              dom = true;
              break;
            }
          }
          if (dom) continue;
          materialize_and(cand);
          sharp_2ands.push_back(cand);
          if (sharp_2ands.size() >= 24) break;
        }
      }

      std::vector<std::pair<double, size_t>> beam_order;
      beam_order.reserve(cands.size());
      for (size_t i = 0; i < cands.size(); ++i) {
        const auto& x = cands[i];
        double sprec = static_cast<double>(x.target_count) /
                       (x.target_count + x.opposing_count + 1.0);
        double rec = static_cast<double>(x.target_count) / n_t;
        double opp_r = static_cast<double>(x.opposing_count) / n_o;
        double gate_boost =
            MeetsCeilingQuantile(x.target_count, n_t, 3, 4) ? 1.0 : 0.4;
        beam_order.emplace_back(gate_boost * sprec + 0.55 * rec - 0.55 * opp_r,
                                i);
      }
      std::stable_sort(
          beam_order.begin(), beam_order.end(),
          [&](const auto& a, const auto& b) {
            if (std::abs(a.first - b.first) > 1e-9) return a.first > b.first;
            const auto& x = cands[a.second];
            const auto& y = cands[b.second];
            return (depth == 2)
                       ? IsBetterPartPair(atom_comp_rules[x.p0],
                                          atom_comp_rules[x.p1],
                                          atom_comp_rules[y.p0],
                                          atom_comp_rules[y.p1])
                       : IsBetterPartTriple(
                             atom_comp_rules[x.p0], atom_comp_rules[x.p1],
                             atom_comp_rules[x.p2], atom_comp_rules[y.p0],
                             atom_comp_rules[y.p1], atom_comp_rules[y.p2]);
          });

      const size_t keep = std::min(kBeamWidth, beam_order.size());
      std::vector<BeamNode> next_beam;
      next_beam.reserve(keep);
      for (size_t k = 0; k < keep; ++k) {
        auto& item = cands[beam_order[k].second];
        materialize_and(item);
        if (PassesUniformRecallOrContrastGate(item.target_count, n_t,
                                              item.opposing_count, n_o)) {
          SynthesizedCandidate sc;
          sc.invariant = item.invariant;
          sc.rule = item.rule;
          sc.target_bits = item.target_bits;
          sc.opposing_bits = item.opposing_bits;
          sc.eval_bits = item.eval_bits;
          sc.target_count = item.target_count;
          sc.opposing_count = item.opposing_count;
          sc.clause_count = depth;
          results.push_back(std::move(sc));
        }
        next_beam.push_back(std::move(item));
      }
      and_beam = std::move(next_beam);
    }
  }

  // --- Pass 2: Depth-3 Set-Cover Bitwise OR Beam Pass (width <= 32) ---
  struct OrClause {
    std::shared_ptr<UnknownCardsInvariant> invariant;
    std::string rule;
    DealBitset target_bits;
    DealBitset opposing_bits;
    DealBitset eval_bits;
    int target_count = 0;
    int opposing_count = 0;
    int atom_clauses = 1;
    bool is_single_honor = false;
    Suit honor_suit = Suit::SPADES;
    HonorPossessionTargetHand honor_target = HonorPossessionTargetHand::kHand1;
  };

  std::vector<OrClause> or_clauses;
  {
    auto clause_rank_fn = [&](int tc, int oc) {
      double sprec = static_cast<double>(tc) / (tc + oc + 1.5);
      double rec = static_cast<double>(tc) / n_t;
      double opp_r = n_o > 0 ? static_cast<double>(oc) / n_o : 0.0;
      return sprec * (rec - opp_r) + 0.40 * sprec + 0.15 * (rec - opp_r);
    };

    std::unordered_map<PairBitsetSignature, size_t, PairBitsetSignatureHash>
        cand_or_map;
    std::vector<OrClause> cand_or_pool;
    cand_or_pool.reserve(48);

    for (size_t i = 0; i < atoms.size(); ++i) {
      const auto& a = atoms[i];
      if (a.target_count <= 0 || a.target_count >= n_t) continue;
      // Require each OR seed clause to cover at least 5% of target deals
      // (eliminating 1-deal flukes on larger partitions while keeping 2/20 on 7s_1021).
      if (static_cast<int64_t>(a.target_count) * 20 < n_t) continue;
      if (MeetsCeilingQuantile(a.target_count, n_t, 9, 10)) continue;
      if (n_o > 0) {
        if (static_cast<int64_t>(a.target_count) * n_o <=
            static_cast<int64_t>(a.opposing_count) * n_t) {
          continue;
        }
        // Avoid seeding OR set-cover with heavily diluted (<40% precision) atoms
        if (static_cast<int64_t>(a.target_count) * 3 <
            static_cast<int64_t>(a.opposing_count) * 2) {
          continue;
        }
      }
      OrClause cl;
      cl.invariant = a.composite_invariant;
      cl.rule = atom_comp_rules[i];
      cl.target_bits = a.target_bits;
      cl.opposing_bits = a.opposing_bits;
      cl.eval_bits = a.eval_bits;
      cl.target_count = a.target_count;
      cl.opposing_count = a.opposing_count;
      cl.atom_clauses = 1;
      cl.is_single_honor =
          (a.family == AtomicPredicateFamily::kHonorPossession);
      cl.honor_suit = a.honor_suit;
      cl.honor_target = a.honor_target;
      cand_or_map.emplace(
          PairBitsetSignature{cl.target_bits, cl.opposing_bits},
          cand_or_pool.size());
      cand_or_pool.push_back(std::move(cl));
    }

    for (const auto& sa : sharp_2ands) {
      PairBitsetSignature psig{sa.target_bits, sa.opposing_bits};
      if (cand_or_map.find(psig) != cand_or_map.end()) continue;
      OrClause cl;
      cl.invariant = sa.invariant;
      cl.rule = sa.rule;
      cl.target_bits = sa.target_bits;
      cl.opposing_bits = sa.opposing_bits;
      cl.eval_bits = sa.eval_bits;
      cl.target_count = sa.target_count;
      cl.opposing_count = sa.opposing_count;
      cl.atom_clauses = 2;
      cand_or_map.emplace(std::move(psig), cand_or_pool.size());
      cand_or_pool.push_back(std::move(cl));
    }

    std::vector<std::pair<double, size_t>> or_seed_order;
    or_seed_order.reserve(cand_or_pool.size());
    for (size_t i = 0; i < cand_or_pool.size(); ++i) {
      or_seed_order.emplace_back(
          clause_rank_fn(cand_or_pool[i].target_count,
                         cand_or_pool[i].opposing_count),
          i);
    }
    std::stable_sort(
        or_seed_order.begin(), or_seed_order.end(),
        [&](const auto& a, const auto& b) {
          if (std::abs(a.first - b.first) > 1e-9) return a.first > b.first;
          return IsBetterCanonicalRule(cand_or_pool[a.second].rule,
                                       cand_or_pool[b.second].rule);
        });

    // Prune Pareto-dominated seed clauses before greedy set-cover selection
    std::vector<size_t> nondom_idx;
    nondom_idx.reserve(or_seed_order.size());
    std::vector<double> nondom_base_score;
    nondom_base_score.reserve(or_seed_order.size());
    for (const auto& [sc, idx] : or_seed_order) {
      const auto& cand = cand_or_pool[idx];
      bool dom = false;
      for (size_t prev_idx : nondom_idx) {
        const auto& prev = cand_or_pool[prev_idx];
        if (prev.opposing_count <= cand.opposing_count &&
            (prev.target_count > cand.target_count ||
             prev.opposing_count < cand.opposing_count) &&
            cand.target_bits.IsSubsetOf(prev.target_bits)) {
          dom = true;
          break;
        }
      }
      if (!dom) {
        nondom_idx.push_back(idx);
        nondom_base_score.push_back(sc);
      }
    }

    // Greedy uncovered-target set-cover selection up to kBeamWidth (32) clauses
    const size_t max_or_seeds = std::min(kBeamWidth, nondom_idx.size());
    or_clauses.reserve(max_or_seeds);
    std::vector<uint8_t> picked(nondom_idx.size(), 0);
    DealBitset covered_targets(n_t);
    for (size_t step = 0; step < max_or_seeds; ++step) {
      size_t best_k = nondom_idx.size();
      double best_val = -1e18;
      for (size_t k = 0; k < nondom_idx.size(); ++k) {
        if (picked[k]) continue;
        const auto& cand = cand_or_pool[nondom_idx[k]];
        int uncov = cand.target_bits.AndNotCount(covered_targets);
        double val =
            nondom_base_score[k] + 0.35 * (static_cast<double>(uncov) / n_t);
        if (best_k == nondom_idx.size() || val > best_val + 1e-9) {
          best_val = val;
          best_k = k;
        }
      }
      if (best_k == nondom_idx.size()) break;
      picked[best_k] = 1;
      covered_targets =
          covered_targets | cand_or_pool[nondom_idx[best_k]].target_bits;
      or_clauses.push_back(std::move(cand_or_pool[nondom_idx[best_k]]));
    }
  }

  if (or_clauses.size() >= 2) {
    struct OrBeamState {
      std::shared_ptr<UnknownCardsInvariant> invariant;
      std::string rule;
      DealBitset target_bits;
      DealBitset opposing_bits;
      DealBitset eval_bits;
      int target_count = 0;
      int opposing_count = 0;
      size_t c0 = 0;
      size_t c1 = 0;
      size_t c2 = 0;
      int depth = 1;
    };

    auto or_beam_score = [&](const OrBeamState& s) {
      double denom = s.target_count + s.opposing_count;
      double prec = denom > 0 ? s.target_count / denom : 0.0;
      double rec = static_cast<double>(s.target_count) / n_t;
      double opp_r = n_o > 0 ? static_cast<double>(s.opposing_count) / n_o : 0.0;
      bool passes =
          PassesUniformRecallOrContrastGate(s.target_count, n_t,
                                            s.opposing_count, n_o) &&
          (n_o == 0 || s.target_count > s.opposing_count);
      double penalty = 0.0;
      if (n_o > 0 && opp_r > 0.10 && rec < 0.90) {
        penalty += 0.30 * (opp_r - 0.10);
      }
      if (n_o > 0 && prec <= 0.50) {
        penalty += 0.60 * (0.55 - prec);
      }
      return (passes ? 0.6 : 0.0) + rec + 0.65 * prec - 0.85 * opp_r - penalty;
    };

    std::vector<OrBeamState> or_beam;
    or_beam.reserve(std::min(kBeamWidth, or_clauses.size()));
    for (size_t i = 0; i < std::min(kBeamWidth, or_clauses.size()); ++i) {
      const auto& c = or_clauses[i];
      OrBeamState st;
      st.invariant = c.invariant;
      st.rule = c.rule;
      st.target_bits = c.target_bits;
      st.opposing_bits = c.opposing_bits;
      st.eval_bits = c.eval_bits;
      st.target_count = c.target_count;
      st.opposing_count = c.opposing_count;
      st.c0 = i;
      st.depth = 1;
      or_beam.push_back(std::move(st));
    }

    for (int depth = 2; depth <= kMaxClauses; ++depth) {
      constexpr size_t kTableMask = 4095;
      std::vector<int32_t> hash_buckets(kTableMask + 1, -1);
      std::vector<OrBeamState> cands;
      cands.reserve(256);
      PairBitsetSignatureHash hasher;

      for (const auto& b : or_beam) {
        // Dominated super-clause guard: if b already covers >=90% of target
        // deals (with >50% precision when opposing deals exist), super-clauses
        // are dominated unless they strictly improve precision.
        bool b_high_recall = MeetsCeilingQuantile(b.target_count, n_t, 9, 10) &&
                             (n_o == 0 || b.target_count > b.opposing_count);
        if (b.target_count >= n_t) continue;

        size_t start_c = (depth == 2) ? (b.c0 + 1) : 0;
        for (size_t c_idx = start_c; c_idx < or_clauses.size(); ++c_idx) {
          if (c_idx == b.c0 || (depth == 3 && c_idx == b.c1)) {
            continue;
          }
          const auto& c = or_clauses[c_idx];

          // Suppress redundant 2-honor same-suit OR when $honor variable holds
          if (depth == 2 && or_clauses[b.c0].is_single_honor &&
              c.is_single_honor) {
            const auto& b0 = or_clauses[b.c0];
            if (b0.honor_suit == c.honor_suit &&
                b0.honor_target == c.honor_target) {
              if (shv_covered.count({b0.honor_suit, b0.honor_target}) > 0 ||
                  shv_covered.count(
                      {b0.honor_suit, HonorPossessionTargetHand::kBoth}) > 0) {
                continue;
              }
            }
          }

          int tc = b.target_bits.OrPopCount(c.target_bits);
          int gain_b = tc - b.target_count;
          int gain_c = tc - c.target_count;

          // Requirement 6: Disjunction must strictly beat all sub-clauses in
          // recall, and each added branch must cover >=5% newly-covered deals.
          if (gain_b <= 0 || gain_c <= 0) continue;
          if (static_cast<int64_t>(gain_b) * 20 < n_t ||
              static_cast<int64_t>(gain_c) * 20 < n_t) {
            continue;
          }

          int oc = b.opposing_bits.OrPopCount(c.opposing_bits);

          if (b_high_recall &&
              !StrictlyBeatsPrecision(tc, oc, b.target_count,
                                      b.opposing_count)) {
            continue;
          }
          bool c_high_recall =
              MeetsCeilingQuantile(c.target_count, n_t, 9, 10) &&
              (n_o == 0 || c.target_count > c.opposing_count);
          if (c_high_recall &&
              !StrictlyBeatsPrecision(tc, oc, c.target_count,
                                      c.opposing_count)) {
            continue;
          }

          // Marginal newly-covered deals must beat 50% precision when opposing exists
          if (n_o > 0) {
            if (gain_b <= (oc - b.opposing_count)) continue;
            if (gain_c <= (oc - c.opposing_count)) continue;
          }

          if (depth == 3) {
            const auto& p0 = or_clauses[b.c0];
            const auto& p1 = or_clauses[b.c1];
            int tc02 = p0.target_bits.OrPopCount(c.target_bits);
            int tc12 = p1.target_bits.OrPopCount(c.target_bits);
            int gain02 = tc - tc02;
            int gain12 = tc - tc12;
            if (gain02 <= 0 || gain12 <= 0) continue;
            if (static_cast<int64_t>(gain02) * 20 < n_t ||
                static_cast<int64_t>(gain12) * 20 < n_t) {
              continue;
            }
            if (n_o > 0) {
              int oc02 = p0.opposing_bits.OrPopCount(c.opposing_bits);
              int oc12 = p1.opposing_bits.OrPopCount(c.opposing_bits);
              if (gain02 <= (oc - oc02) || gain12 <= (oc - oc12)) continue;
              if (MeetsCeilingQuantile(tc02, n_t, 9, 10) && tc02 > oc02 &&
                  !StrictlyBeatsPrecision(tc, oc, tc02, oc02)) {
                continue;
              }
              if (MeetsCeilingQuantile(tc12, n_t, 9, 10) && tc12 > oc12 &&
                  !StrictlyBeatsPrecision(tc, oc, tc12, oc12)) {
                continue;
              }
            } else {
              if (MeetsCeilingQuantile(tc02, n_t, 9, 10) ||
                  MeetsCeilingQuantile(tc12, n_t, 9, 10)) {
                continue;
              }
            }
          }

          DealBitset tb = b.target_bits | c.target_bits;
          DealBitset ob = b.opposing_bits | c.opposing_bits;
          PairBitsetSignature sig{std::move(tb), std::move(ob)};
          if (atom_pair_map.find(sig) != atom_pair_map.end()) continue;

          size_t slot = hasher(sig) & kTableMask;
          int32_t found = -1;
          while (hash_buckets[slot] >= 0) {
            int32_t idx = hash_buckets[slot];
            if (cands[idx].target_bits == sig.target &&
                cands[idx].opposing_bits == sig.opposing) {
              found = idx;
              break;
            }
            slot = (slot + 1) & kTableMask;
          }
          if (found < 0) {
            OrBeamState st;
            st.target_bits = std::move(sig.target);
            st.opposing_bits = std::move(sig.opposing);
            st.target_count = tc;
            st.opposing_count = oc;
            st.c0 = b.c0;
            st.c1 = (depth == 2) ? c_idx : b.c1;
            st.c2 = (depth == 3) ? c_idx : 0;
            st.depth = depth;
            hash_buckets[slot] = static_cast<int32_t>(cands.size());
            cands.push_back(std::move(st));
          } else {
            OrBeamState& cur = cands[found];
            bool better =
                (depth == 2)
                    ? IsBetterPartPair(or_clauses[b.c0].rule,
                                       or_clauses[c_idx].rule,
                                       or_clauses[cur.c0].rule,
                                       or_clauses[cur.c1].rule)
                    : IsBetterPartTriple(
                          or_clauses[b.c0].rule, or_clauses[b.c1].rule,
                          or_clauses[c_idx].rule, or_clauses[cur.c0].rule,
                          or_clauses[cur.c1].rule, or_clauses[cur.c2].rule);
            if (better) {
              cur.c0 = b.c0;
              cur.c1 = (depth == 2) ? c_idx : b.c1;
              cur.c2 = (depth == 3) ? c_idx : 0;
            }
          }
        }
      }

      std::vector<std::pair<double, size_t>> or_order;
      or_order.reserve(cands.size());
      for (size_t i = 0; i < cands.size(); ++i) {
        or_order.emplace_back(or_beam_score(cands[i]), i);
      }
      std::stable_sort(
          or_order.begin(), or_order.end(),
          [&](const auto& a, const auto& b) {
            if (std::abs(a.first - b.first) > 1e-9) return a.first > b.first;
            const auto& x = cands[a.second];
            const auto& y = cands[b.second];
            return (depth == 2)
                       ? IsBetterPartPair(or_clauses[x.c0].rule,
                                          or_clauses[x.c1].rule,
                                          or_clauses[y.c0].rule,
                                          or_clauses[y.c1].rule)
                       : IsBetterPartTriple(
                             or_clauses[x.c0].rule, or_clauses[x.c1].rule,
                             or_clauses[x.c2].rule, or_clauses[y.c0].rule,
                             or_clauses[y.c1].rule, or_clauses[y.c2].rule);
          });

      const size_t keep = std::min(kBeamWidth, or_order.size());
      std::vector<OrBeamState> next_or_beam;
      next_or_beam.reserve(keep);
      for (size_t k = 0; k < keep; ++k) {
        auto& item = cands[or_order[k].second];
        item.invariant =
            (depth == 2)
                ? MakeCanonicalOr(or_clauses[item.c0].invariant,
                                  or_clauses[item.c1].invariant)
                : MakeCanonicalOr3(or_clauses[item.c0].invariant,
                                   or_clauses[item.c1].invariant,
                                   or_clauses[item.c2].invariant);
        item.rule = item.invariant->ToString();
        item.eval_bits =
            (depth == 2)
                ? (or_clauses[item.c0].eval_bits | or_clauses[item.c1].eval_bits)
                : ((or_clauses[item.c0].eval_bits |
                    or_clauses[item.c1].eval_bits) |
                   or_clauses[item.c2].eval_bits);
        if (PassesUniformRecallOrContrastGate(item.target_count, n_t,
                                              item.opposing_count, n_o)) {
          SynthesizedCandidate sc;
          sc.invariant = item.invariant;
          sc.rule = item.rule;
          sc.target_bits = item.target_bits;
          sc.opposing_bits = item.opposing_bits;
          sc.eval_bits = item.eval_bits;
          sc.target_count = item.target_count;
          sc.opposing_count = item.opposing_count;
          sc.clause_count =
              or_clauses[item.c0].atom_clauses +
              or_clauses[item.c1].atom_clauses +
              (depth == 3 ? or_clauses[item.c2].atom_clauses : 0);
          results.push_back(std::move(sc));
        }
        next_or_beam.push_back(std::move(item));
      }
      or_beam = std::move(next_or_beam);
    }
  }

  // Final Pareto & marginal-precision dominance pruning across compound rules:
  // Prune any compound candidate X (clause_count >= 2) when another qualifying,
  // non-boring, >50%-precision candidate Y (with Y.clause_count <= X.clause_count)
  // achieves >= recall and <= opposing hold (strictly better in recall, opposing
  // hold, or clause count), or already covers >=95% of target deals at 100%
  // precision, or X extends Y with <=50% marginal precision.
  if (results.size() > 1) {
    const int64_t total_sim = static_cast<int64_t>(n_t) + n_o;
    auto survives_post_gates = [&](const SynthesizedCandidate& y) {
      if (n_o <= 0) return true;
      int hc = y.target_count + y.opposing_count;
      if (hc <= 0) return false;
      int holds_pct = static_cast<int>(std::lround(100.0 * hc / total_sim));
      if (static_cast<int64_t>(hc) * 20 >= 19 * total_sim || holds_pct >= 95) {
        return false;
      }
      int prec_pct =
          static_cast<int>(std::lround(100.0 * y.target_count / hc));
      if (y.target_count * 2 <= hc || prec_pct <= 50) {
        return false;
      }
      return true;
    };

    std::vector<SynthesizedCandidate> pruned;
    pruned.reserve(results.size());
    for (size_t i = 0; i < results.size(); ++i) {
      const auto& x = results[i];
      if (x.clause_count <= 1) {
        pruned.push_back(x);
        continue;
      }
      bool dominated = false;
      for (size_t j = 0; j < results.size(); ++j) {
        if (i == j) continue;
        const auto& y = results[j];
        if (y.clause_count > x.clause_count) continue;
        if (!survives_post_gates(y)) continue;

        if (y.target_count >= x.target_count &&
            y.opposing_count <= x.opposing_count &&
            (y.target_count > x.target_count ||
             y.opposing_count < x.opposing_count ||
             y.clause_count < x.clause_count)) {
          dominated = true;
          break;
        }
        if (n_o > 0 && y.opposing_count == 0 && x.opposing_count > 0 &&
            MeetsCeilingQuantile(y.target_count, n_t, 19, 20)) {
          dominated = true;
          break;
        }
        if (n_o > 0 && x.target_count > y.target_count &&
            x.opposing_count > y.opposing_count &&
            (x.target_count - y.target_count) <=
                (x.opposing_count - y.opposing_count) &&
            y.target_bits.IsSubsetOf(x.target_bits)) {
          dominated = true;
          break;
        }
      }
      if (!dominated) {
        pruned.push_back(x);
      }
    }
    results = std::move(pruned);
  }

  return results;
}

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
    if (inv && emitted.insert(inv->ToString()).second) {
      invariants.push_back(inv);
    }
  };

  auto project_hand_suit = [&](InvariantHandChoice choice, Suit suit,
                               int max_combined, int q_num, int q_den) {
    std::shared_ptr<UnknownCardsInvariant> best_inv;
    int best_width = 999;
    int best_count = -1;
    for (const auto& e : catalog.entries()) {
      if (!MeetsCeilingQuantile(e.target_count, total_deals_, q_num, q_den)) {
        continue;
      }
      if (e.family == AtomicPredicateFamily::kExactSuitLength) {
        auto ex = std::dynamic_pointer_cast<ExactSuitLengthInvariant>(
            e.invariant);
        if (ex && ex->hand_choice() == choice && ex->suit() == suit) {
          if (0 < best_width || (0 == best_width && e.target_count > best_count)) {
            best_width = 0;
            best_count = e.target_count;
            best_inv = e.invariant;
          }
        }
      } else if (e.family == AtomicPredicateFamily::kSuitLengthRange) {
        auto rg = std::dynamic_pointer_cast<SuitLengthRangeInvariant>(
            e.invariant);
        if (rg && rg->hand_choice() == choice && rg->suit() == suit) {
          if (rg->min_len() == 0 && rg->max_len() >= max_combined) continue;
          int w = rg->max_len() - rg->min_len();
          if (w < best_width || (w == best_width && e.target_count > best_count)) {
            best_width = w;
            best_count = e.target_count;
            best_inv = e.invariant;
          }
        }
      }
    }
    if (best_inv) emit(best_inv);
  };

  int total_unknown_cards = 0;
  for (int s = 0; s < 4; ++s) {
    auto it = suit_bounds_.find(static_cast<Suit>(s));
    if (it != suit_bounds_.end()) {
      total_unknown_cards += it->second.max_combined;
    }
  }

  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    auto it = suit_bounds_.find(suit);
    if (it == suit_bounds_.end()) continue;
    const auto& bounds = it->second;
    if (bounds.max_combined == 0 ||
        bounds.max_combined == total_unknown_cards) {
      continue;
    }

    // Project tightest 95% and 90% quantile intervals for Hand 1 and Hand 2
    project_hand_suit(InvariantHandChoice::kHand1, suit, bounds.max_combined,
                      19, 20);
    project_hand_suit(InvariantHandChoice::kHand1, suit, bounds.max_combined,
                      9, 10);
    project_hand_suit(InvariantHandChoice::kHand2, suit, bounds.max_combined,
                      19, 20);
    project_hand_suit(InvariantHandChoice::kHand2, suit, bounds.max_combined,
                      9, 10);

    // Project tightest MaxSuitLengthInvariant at 95% and 90% quantiles
    for (auto [q_num, q_den] :
         {std::pair<int, int>{19, 20}, std::pair<int, int>{9, 10}}) {
      std::shared_ptr<UnknownCardsInvariant> best_max;
      int best_m = 999;
      for (const auto& e : catalog.entries()) {
        if (e.family != AtomicPredicateFamily::kMaxSuitLength) continue;
        if (!MeetsCeilingQuantile(e.target_count, total_deals_, q_num, q_den)) {
          continue;
        }
        auto mx =
            std::dynamic_pointer_cast<MaxSuitLengthInvariant>(e.invariant);
        if (mx && mx->suit() == suit && mx->max_len() < best_m) {
          best_m = mx->max_len();
          best_max = e.invariant;
        }
      }
      if (best_max) emit(best_max);
    }

    // Project tightest ShortestLengthInvariant (kAtLeast and kAtMost) at 90% & 95%
    for (auto [q_num, q_den] :
         {std::pair<int, int>{9, 10}, std::pair<int, int>{19, 20}}) {
      std::shared_ptr<UnknownCardsInvariant> best_ge;
      int best_ge_k = -1;
      std::shared_ptr<UnknownCardsInvariant> best_le;
      int best_le_k = 999;
      for (const auto& e : catalog.entries()) {
        if (e.family != AtomicPredicateFamily::kShortestLength) continue;
        if (!MeetsCeilingQuantile(e.target_count, total_deals_, q_num, q_den)) {
          continue;
        }
        auto sl =
            std::dynamic_pointer_cast<ShortestLengthInvariant>(e.invariant);
        if (!sl || sl->suit() != suit) continue;
        if (sl->cmp() == ShortestLengthInvariant::Comparison::kAtLeast &&
            sl->length() > best_ge_k) {
          best_ge_k = sl->length();
          best_ge = e.invariant;
        } else if (sl->cmp() == ShortestLengthInvariant::Comparison::kAtMost &&
                   sl->length() < best_le_k) {
          best_le_k = sl->length();
          best_le = e.invariant;
        }
      }
      if (best_ge) emit(best_ge);
      if (best_le) emit(best_le);
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

  std::vector<const AtomicPredicateEntry*> non_universal_honors;
  std::vector<const AtomicPredicateEntry*> all_honor_atoms;
  std::set<std::pair<Suit, HonorPossessionTargetHand>> specific_covered;

  for (const auto& e : catalog.entries()) {
    if (e.family != AtomicPredicateFamily::kHonorPossession) continue;
    auto hp =
        std::dynamic_pointer_cast<HonorPossessionInvariant>(e.invariant);
    if (!hp) continue;
    Suit s = Card::DecodeCard(hp->card()).first;

    if (MeetsCeilingQuantile(e.target_count, total_deals_, 9, 10)) {
      invariants.push_back(e.invariant);
      specific_covered.insert({s, hp->target_hand()});
      if (hp->target_hand() != HonorPossessionTargetHand::kBoth) {
        all_honor_atoms.push_back(&e);
      }
    } else if (e.target_count > 0 &&
               hp->target_hand() != HonorPossessionTargetHand::kBoth) {
      non_universal_honors.push_back(&e);
      all_honor_atoms.push_back(&e);
    }
  }

  // Project SuitHonorVariableInvariant when not covered by specific honor rules
  std::set<std::pair<Suit, HonorPossessionTargetHand>> shv_emitted;
  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    bool h1_spec =
        specific_covered.count({suit, HonorPossessionTargetHand::kHand1}) > 0 ||
        specific_covered.count({suit, HonorPossessionTargetHand::kBoth}) > 0;
    bool h2_spec =
        specific_covered.count({suit, HonorPossessionTargetHand::kHand2}) > 0 ||
        specific_covered.count({suit, HonorPossessionTargetHand::kBoth}) > 0;

    const AtomicPredicateEntry* shv_h1 = nullptr;
    const AtomicPredicateEntry* shv_h2 = nullptr;
    const AtomicPredicateEntry* shv_both = nullptr;
    for (const auto& e : catalog.entries()) {
      if (e.family != AtomicPredicateFamily::kSuitHonorVariable) continue;
      auto shv =
          std::dynamic_pointer_cast<SuitHonorVariableInvariant>(e.invariant);
      if (!shv || shv->suit() != suit) continue;
      if (!MeetsCeilingQuantile(e.target_count, total_deals_, 9, 10)) continue;
      if (shv->target_hand() == HonorPossessionTargetHand::kHand1) shv_h1 = &e;
      if (shv->target_hand() == HonorPossessionTargetHand::kHand2) shv_h2 = &e;
      if (shv->target_hand() == HonorPossessionTargetHand::kBoth) shv_both = &e;
    }

    if (shv_both && !h1_spec && !h2_spec) {
      invariants.push_back(shv_both->invariant);
      shv_emitted.insert({suit, HonorPossessionTargetHand::kBoth});
    } else {
      if (shv_h1 && !h1_spec) {
        invariants.push_back(shv_h1->invariant);
        shv_emitted.insert({suit, HonorPossessionTargetHand::kHand1});
      }
      if (shv_h2 && !h2_spec) {
        invariants.push_back(shv_h2->invariant);
        shv_emitted.insert({suit, HonorPossessionTargetHand::kHand2});
      }
    }
  }

  // Project non-dominated 2-clause OrInvariant over catalog honor bitsets
  std::set<std::string> emitted_or_rules;
  for (size_t i = 0; i < non_universal_honors.size(); ++i) {
    for (size_t j = i + 1; j < non_universal_honors.size(); ++j) {
      const auto* ea = non_universal_honors[i];
      const auto* eb = non_universal_honors[j];
      auto a = std::dynamic_pointer_cast<HonorPossessionInvariant>(
          ea->composite_invariant);
      auto b = std::dynamic_pointer_cast<HonorPossessionInvariant>(
          eb->composite_invariant);
      if (!a || !b || a->ToString() == b->ToString()) continue;

      Suit s1 = Card::DecodeCard(a->card()).first;
      Suit s2 = Card::DecodeCard(b->card()).first;
      if (s1 == s2 && a->target_hand() == b->target_hand()) {
        if (shv_emitted.count({s1, a->target_hand()}) > 0 ||
            shv_emitted.count({s1, HonorPossessionTargetHand::kBoth}) > 0) {
          continue;
        }
      }

      int or_count = (ea->target_bits | eb->target_bits).PopCount();
      if (or_count <= std::max(ea->target_count, eb->target_count)) {
        continue;
      }
      if (MeetsCeilingQuantile(or_count, total_deals_, 9, 10)) {
        auto or_inv = MakeCanonicalOr(a, b);
        std::string rule_str = or_inv->ToString();
        if (emitted_or_rules.insert(rule_str).second) {
          invariants.push_back(std::move(or_inv));
        }
      }
    }
  }

  // Project 2-clause AndInvariant over catalog honor bitsets
  std::set<std::string> emitted_and_rules;
  for (size_t i = 0; i < all_honor_atoms.size(); ++i) {
    for (size_t j = i + 1; j < all_honor_atoms.size(); ++j) {
      const auto* ea = all_honor_atoms[i];
      const auto* eb = all_honor_atoms[j];
      auto a = ea->composite_invariant;
      auto b = eb->composite_invariant;
      if (!a || !b || a->ToString() == b->ToString()) continue;

      int and_count = (ea->target_bits & eb->target_bits).PopCount();
      if (MeetsCeilingQuantile(and_count, total_deals_, 9, 10)) {
        auto and_inv = MakeCanonicalAnd(a, b);
        std::string rule_str = and_inv->ToString();
        if (emitted_and_rules.insert(rule_str).second) {
          invariants.push_back(std::move(and_inv));
        }
      }
    }
  }

  return invariants;
}

void HonorPossessionClassifier::AddOrInvariants(
    const std::vector<std::shared_ptr<HonorPossessionInvariant>>&
        /*non_100_candidates*/,
    std::vector<std::shared_ptr<UnknownCardsInvariant>>& /*invariants*/) const {
}

void HonorPossessionClassifier::AddAndInvariants(
    const std::vector<std::shared_ptr<HonorPossessionInvariant>>&
        /*all_candidates*/,
    std::vector<std::shared_ptr<UnknownCardsInvariant>>& /*invariants*/) const {
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
  std::set<std::string> emitted_rules;
  auto emit = [&](const std::shared_ptr<UnknownCardsInvariant>& inv) {
    if (inv && emitted_rules.insert(inv->ToString()).second) {
      invariants.push_back(inv);
    }
  };

  auto not_x_holds_quantile = [&](CardType cond_card, Suit suit, int cmp_mode,
                                  int val) {
    int count = 0;
    for (const auto& d : deals_) {
      uint64_t hand_not_x = ((d.hand1 & cond_card) != 0) ? d.hand2 : d.hand1;
      int len = CardsInSuit(hand_not_x, suit);
      if (cmp_mode == 0 && len == val) count++;
      if (cmp_mode == 1 && len >= val) count++;
      if (cmp_mode == -1 && len <= val) count++;
    }
    return MeetsCeilingQuantile(count, total_deals_, 9, 10);
  };

  // Group conditional length entries by (condition_card, target_suit)
  std::set<std::pair<CardType, int>> exact_emitted;
  for (const auto& e : catalog.entries()) {
    if (e.family == AtomicPredicateFamily::kCrossSuitCard) {
      if (MeetsCeilingQuantile(e.target_count, total_deals_, 9, 10)) {
        emit(e.invariant);
      }
    } else if (e.family == AtomicPredicateFamily::kCrossSuitLength) {
      if (auto ex = std::dynamic_pointer_cast<CrossSuitExactSuitLengthInvariant>(
              e.invariant)) {
        if (ex->length() > 0 &&
            MeetsCeilingQuantile(e.target_count, total_deals_, 9, 10) &&
            !not_x_holds_quantile(ex->condition_card(), ex->target_suit(), 0,
                                  ex->length())) {
          exact_emitted.insert(
              {ex->condition_card(), static_cast<int>(ex->target_suit())});
          emit(e.invariant);
        }
      }
    }
  }

  // For (condition_card, suit) without an exact length rule, project tightest
  // 95% and 90% quantile min/max conditional bounds.
  for (auto [q_num, q_den] :
       {std::pair<int, int>{19, 20}, std::pair<int, int>{9, 10}}) {
    std::map<std::pair<CardType, int>, std::shared_ptr<UnknownCardsInvariant>>
        best_min;
    std::map<std::pair<CardType, int>, int> best_min_val;
    std::map<std::pair<CardType, int>, std::shared_ptr<UnknownCardsInvariant>>
        best_max;
    std::map<std::pair<CardType, int>, int> best_max_val;

    for (const auto& e : catalog.entries()) {
      if (e.family != AtomicPredicateFamily::kCrossSuitLength) continue;
      if (!MeetsCeilingQuantile(e.target_count, total_deals_, q_num, q_den)) {
        continue;
      }
      if (auto mn = std::dynamic_pointer_cast<CrossSuitMinSuitLengthInvariant>(
              e.invariant)) {
        auto key = std::make_pair(mn->condition_card(),
                                  static_cast<int>(mn->target_suit()));
        if (exact_emitted.count(key) > 0) continue;
        if (not_x_holds_quantile(mn->condition_card(), mn->target_suit(), 1,
                                 mn->min_len())) {
          continue;
        }
        if (best_min_val.find(key) == best_min_val.end() ||
            mn->min_len() > best_min_val[key]) {
          best_min_val[key] = mn->min_len();
          best_min[key] = e.invariant;
        }
      } else if (auto mx =
                     std::dynamic_pointer_cast<CrossSuitMaxSuitLengthInvariant>(
                         e.invariant)) {
        auto key = std::make_pair(mx->condition_card(),
                                  static_cast<int>(mx->target_suit()));
        if (exact_emitted.count(key) > 0) continue;
        if (not_x_holds_quantile(mx->condition_card(), mx->target_suit(), -1,
                                 mx->max_len())) {
          continue;
        }
        if (best_max_val.find(key) == best_max_val.end() ||
            mx->max_len() < best_max_val[key]) {
          best_max_val[key] = mx->max_len();
          best_max[key] = e.invariant;
        }
      }
    }

    for (const auto& [k, inv] : best_min) emit(inv);
    for (const auto& [k, inv] : best_max) emit(inv);
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
    AtomicPredicateCatalog catalog(deals, opposing_deals, formatter,
                                   &bifurcations);
    std::vector<SynthesizedCandidate> synthesized =
        SetCoverSynthesizer::Synthesize(catalog);

    int total_sim =
        static_cast<int>(deals.size() + opposing_deals->size());
    std::vector<InvariantStats> sim_stats;
    std::vector<BitsetSignature> sim_sigs;
    sim_stats.reserve(synthesized.size());
    sim_sigs.reserve(synthesized.size());

    for (auto& cand : synthesized) {
      if (!cand.invariant) continue;
      int holds_in_deals = cand.target_count;
      int holds_in_opposing = cand.opposing_count;

      // Uniform bitset recall and high-contrast gate across all predicate
      // families and compound rules.
      if (!SetCoverSynthesizer::PassesUniformRecallOrContrastGate(
              holds_in_deals, static_cast<int>(deals.size()), holds_in_opposing,
              static_cast<int>(opposing_deals->size()))) {
        continue;
      }

      int holds_count = holds_in_deals + holds_in_opposing;

      InvariantStats st;
      st.invariant = cand.invariant;
      st.rule = std::move(cand.rule);
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
      sim_sigs.push_back(BitsetSignature{
          std::move(cand.target_bits), std::move(cand.opposing_bits),
          std::move(cand.eval_bits)});
    }

    // Deduplicate equivalent invariants directly on precomputed bitset
    // signatures before sorting, picking the shortest canonical rule string.
    std::unordered_map<BitsetSignature, size_t, BitsetSignatureHash> dedup_map;
    dedup_map.reserve(sim_stats.size());
    std::vector<InvariantStats> deduped;
    deduped.reserve(sim_stats.size());
    for (size_t i = 0; i < sim_stats.size(); ++i) {
      auto it = dedup_map.find(sim_sigs[i]);
      if (it == dedup_map.end()) {
        dedup_map.emplace(std::move(sim_sigs[i]), deduped.size());
        deduped.push_back(std::move(sim_stats[i]));
      } else {
        size_t idx = it->second;
        if (IsBetterCanonicalRule(sim_stats[i].rule, deduped[idx].rule)) {
          deduped[idx] = std::move(sim_stats[i]);
        }
      }
    }

    SuppressRedundantSuitHonorVariableInvariants(deduped);

    std::stable_sort(deduped.begin(), deduped.end(),
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

