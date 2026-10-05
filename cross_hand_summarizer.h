#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "card.h"
#include "hand.h"

// 64-bit mask for honor cards (ranks 10, J, Q, K, A, corresponding to bits 8..12 in each 16-bit suit)
constexpr uint64_t kHonorMask = kHonorMask64;

struct PairedDeal {
  uint64_t hand1 = 0;
  uint64_t hand2 = 0;

  PairedDeal() = default;
  PairedDeal(uint64_t h1, uint64_t h2) : hand1(h1), hand2(h2) {}
  PairedDeal(const std::string& h1_str, const std::string& h2_str)
      : hand1(ParseHand(h1_str)), hand2(ParseHand(h2_str)) {}
};

// --- Invariant Class Hierarchy ---

class Invariant {
 public:
  virtual ~Invariant() = default;

  virtual std::string ToString() const = 0;

  virtual bool HoldsOnDeal(
      const PairedDeal& deal,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2") const = 0;

  static std::unique_ptr<Invariant> FromString(
      const std::string& str,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2");
};

enum class ConditionalTargetType {
  kCard,
  kExactSuitLength,
  kMinSuitLength,
  kMaxSuitLength
};

class CrossSuitConditionalInvariant : public Invariant {
 public:
  CrossSuitConditionalInvariant(CardType condition_card, CardType target_card)
      : condition_card_(condition_card),
        target_type_(ConditionalTargetType::kCard),
        target_card_(target_card) {}

  CrossSuitConditionalInvariant(CardType condition_card,
                                ConditionalTargetType target_type,
                                Suit target_suit,
                                int length)
      : condition_card_(condition_card),
        target_type_(target_type),
        target_suit_(target_suit),
        length_(length) {}

  std::string ToString() const override;

  bool HoldsOnDeal(
      const PairedDeal& deal,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2") const override;

  static std::unique_ptr<CrossSuitConditionalInvariant> FromString(
      const std::string& str,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2");

  CardType condition_card() const { return condition_card_; }
  ConditionalTargetType target_type() const { return target_type_; }
  CardType target_card() const { return target_card_; }
  Suit target_suit() const { return target_suit_; }
  int length() const { return length_; }

 private:
  CardType condition_card_ = 0;
  ConditionalTargetType target_type_ = ConditionalTargetType::kCard;
  CardType target_card_ = 0;
  Suit target_suit_ = Suit::SPADES;
  int length_ = 0;
};

enum class HonorPossessionTargetHand {
  kHand1,
  kHand2,
  kBoth
};

class HonorPossessionInvariant : public Invariant {
 public:
  HonorPossessionInvariant(HonorPossessionTargetHand target_hand,
                           CardType card,
                           std::string label = "")
      : target_hand_(target_hand),
        card_(card),
        label_(std::move(label)) {}

  std::string ToString() const override;

  bool HoldsOnDeal(
      const PairedDeal& deal,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2") const override;

  static std::unique_ptr<HonorPossessionInvariant> FromString(
      const std::string& str,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2");

  HonorPossessionTargetHand target_hand() const { return target_hand_; }
  CardType card() const { return card_; }
  const std::string& label() const { return label_; }

 private:
  HonorPossessionTargetHand target_hand_ = HonorPossessionTargetHand::kHand1;
  CardType card_ = 0;
  std::string label_;
};

class MaxSuitLengthInvariant : public Invariant {
 public:
  MaxSuitLengthInvariant(Suit suit, int max_len)
      : suit_(suit), max_len_(max_len) {}

  std::string ToString() const override;

  bool HoldsOnDeal(
      const PairedDeal& deal,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2") const override;

  static std::unique_ptr<MaxSuitLengthInvariant> FromString(
      const std::string& str,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2");

  Suit suit() const { return suit_; }
  int max_len() const { return max_len_; }

 private:
  Suit suit_ = Suit::SPADES;
  int max_len_ = 0;
};

enum class InvariantHandChoice {
  kHand1,
  kHand2
};

class ExactSuitLengthInvariant : public Invariant {
 public:
  ExactSuitLengthInvariant(InvariantHandChoice hand_choice,
                           Suit suit,
                           int exact_len,
                           std::string label = "")
      : hand_choice_(hand_choice),
        suit_(suit),
        exact_len_(exact_len),
        label_(std::move(label)) {}

  std::string ToString() const override;

  bool HoldsOnDeal(
      const PairedDeal& deal,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2") const override;

  static std::unique_ptr<ExactSuitLengthInvariant> FromString(
      const std::string& str,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2");

  InvariantHandChoice hand_choice() const { return hand_choice_; }
  Suit suit() const { return suit_; }
  int exact_len() const { return exact_len_; }
  const std::string& label() const { return label_; }

 private:
  InvariantHandChoice hand_choice_ = InvariantHandChoice::kHand1;
  Suit suit_ = Suit::SPADES;
  int exact_len_ = 0;
  std::string label_;
};

class SuitLengthRangeInvariant : public Invariant {
 public:
  SuitLengthRangeInvariant(InvariantHandChoice hand_choice,
                           Suit suit,
                           int min_len,
                           int max_len,
                           std::string label = "")
      : hand_choice_(hand_choice),
        suit_(suit),
        min_len_(min_len),
        max_len_(max_len),
        label_(std::move(label)) {}

  std::string ToString() const override;

  bool HoldsOnDeal(
      const PairedDeal& deal,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2") const override;

  static std::unique_ptr<SuitLengthRangeInvariant> FromString(
      const std::string& str,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2");

  InvariantHandChoice hand_choice() const { return hand_choice_; }
  Suit suit() const { return suit_; }
  int min_len() const { return min_len_; }
  int max_len() const { return max_len_; }
  const std::string& label() const { return label_; }

 private:
  InvariantHandChoice hand_choice_ = InvariantHandChoice::kHand1;
  Suit suit_ = Suit::SPADES;
  int min_len_ = 0;
  int max_len_ = 0;
  std::string label_;
};

struct JointSuitBounds {
  int min_hand1 = 999;
  int max_hand1 = -1;
  int min_hand2 = 999;
  int max_hand2 = -1;
  int min_combined = 999;
  int max_combined = -1;
};

struct HonorPossessionStats {
  int hand1_count = 0;
  int hand2_count = 0;
  int both_count = 0;
  int neither_count = 0;
};

class JointSuitRuleExtractor {
 public:
  JointSuitRuleExtractor(std::string hand1_label = "hand 1",
                         std::string hand2_label = "hand 2")
      : hand1_label_(std::move(hand1_label)),
        hand2_label_(std::move(hand2_label)) {}

  void ProcessDeal(const PairedDeal& deal);
  const std::map<Suit, JointSuitBounds>& GetSuitBounds() const { return suit_bounds_; }
  std::vector<std::unique_ptr<Invariant>> ExtractInvariants() const;
  std::vector<std::string> ExtractRules() const;

 private:
  std::string hand1_label_;
  std::string hand2_label_;
  std::map<Suit, JointSuitBounds> suit_bounds_;
  int total_deals_ = 0;
};

class HonorPossessionClassifier {
 public:
  HonorPossessionClassifier(std::string hand1_label = "hand 1",
                            std::string hand2_label = "hand 2")
      : hand1_label_(std::move(hand1_label)),
        hand2_label_(std::move(hand2_label)) {}

  void ProcessDeal(const PairedDeal& deal);
  const std::map<Suit, std::map<int, HonorPossessionStats>>& GetHonorStats() const { return honor_stats_; }
  std::vector<std::unique_ptr<Invariant>> ExtractInvariants() const;
  std::vector<std::string> ExtractRules() const;

 private:
  std::string hand1_label_;
  std::string hand2_label_;
  // Map of Suit -> (honor rank 8..12 -> HonorPossessionStats)
  std::map<Suit, std::map<int, HonorPossessionStats>> honor_stats_;
  int total_deals_ = 0;
};

class CrossSuitConditionalRuleExtractor {
 public:
  CrossSuitConditionalRuleExtractor(std::string hand1_label = "hand 1",
                                     std::string hand2_label = "hand 2")
      : hand1_label_(std::move(hand1_label)),
        hand2_label_(std::move(hand2_label)) {}

  void ProcessDeal(const PairedDeal& deal);
  std::vector<std::unique_ptr<Invariant>> ExtractInvariants() const;
  std::vector<std::string> ExtractRules() const;

 private:
  std::string hand1_label_;
  std::string hand2_label_;
  int total_deals_ = 0;
  std::vector<PairedDeal> deals_;
};

struct CrossHandSummaryReport {
  int total_deals = 0;
  uint64_t unknown_cards = 0;
  int cards_per_hand = 0;
  std::vector<std::string> invariant_rules;
  std::map<Suit, JointSuitBounds> suit_bounds;
  std::map<Suit, std::map<int, HonorPossessionStats>> honor_stats;

  bool HasRule(const std::string& rule_str) const {
    for (const auto& r : invariant_rules) {
      if (r == rule_str || r.find(rule_str) != std::string::npos) {
        return true;
      }
    }
    return false;
  }

  std::string ToString() const;
};

bool EvaluateRuleOnDeal(const std::string& rule,
                        const PairedDeal& deal,
                        const std::string& hand1_label = "hand 1",
                        const std::string& hand2_label = "hand 2");

std::vector<std::unique_ptr<Invariant>> FilterUninterestingInvariants(
    std::vector<std::unique_ptr<Invariant>> candidate_invariants,
    uint64_t unknown_cards,
    int hand1_size,
    const std::string& hand1_label = "hand 1",
    const std::string& hand2_label = "hand 2",
    int num_bifurcations = 200);

std::vector<std::string> FilterUninterestingRules(
    const std::vector<std::string>& candidate_rules,
    uint64_t unknown_cards,
    int hand1_size,
    const std::string& hand1_label = "hand 1",
    const std::string& hand2_label = "hand 2",
    int num_bifurcations = 200);

class CrossHandDealSetSummarizer {
 public:
  CrossHandDealSetSummarizer() = default;

  CrossHandSummaryReport Summarize(
      const std::vector<PairedDeal>& deals,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2") const;
};
