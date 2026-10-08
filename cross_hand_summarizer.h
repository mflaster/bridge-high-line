#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "card.h"
#include "hand.h"
#include "unknown_cards_invariant.h"

class ConditionalLine;

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
  std::vector<std::shared_ptr<UnknownCardsInvariant>> ExtractInvariants() const;
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
  std::vector<std::shared_ptr<UnknownCardsInvariant>> ExtractInvariants() const;
  std::vector<std::string> ExtractRules() const;

 private:
  void AddOrInvariants(
      const std::vector<std::shared_ptr<HonorPossessionInvariant>>& non_100_candidates,
      std::vector<std::shared_ptr<UnknownCardsInvariant>>& invariants) const;
  void AddAndInvariants(
      const std::vector<std::shared_ptr<HonorPossessionInvariant>>& all_candidates,
      std::vector<std::shared_ptr<UnknownCardsInvariant>>& invariants) const;

  std::string hand1_label_;
  std::string hand2_label_;
  // Map of Suit -> (honor rank 8..12 -> HonorPossessionStats)
  std::map<Suit, std::map<int, HonorPossessionStats>> honor_stats_;
  int total_deals_ = 0;
  std::vector<PairedDeal> deals_;
};

class CrossSuitConditionalRuleExtractor {
 public:
  CrossSuitConditionalRuleExtractor(std::string hand1_label = "hand 1",
                                     std::string hand2_label = "hand 2")
      : hand1_label_(std::move(hand1_label)),
        hand2_label_(std::move(hand2_label)) {}

  void ProcessDeal(const PairedDeal& deal);
  std::vector<std::shared_ptr<UnknownCardsInvariant>> ExtractInvariants() const;
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

std::vector<std::shared_ptr<UnknownCardsInvariant>> FilterUninterestingInvariants(
    std::vector<std::shared_ptr<UnknownCardsInvariant>> candidate_invariants,
    uint64_t unknown_cards,
    int hand1_size,
    const std::string& hand1_label = "hand 1",
    const std::string& hand2_label = "hand 2",
    int num_bifurcations = 200,
    const ConditionalLine* line = nullptr);

std::vector<std::string> FilterUninterestingRules(
    const std::vector<std::string>& candidate_rules,
    uint64_t unknown_cards,
    int hand1_size,
    const std::string& hand1_label = "hand 1",
    const std::string& hand2_label = "hand 2",
    int num_bifurcations = 200,
    const ConditionalLine* line = nullptr);

class CrossHandDealSetSummarizer {
 public:
  CrossHandDealSetSummarizer() = default;

  CrossHandSummaryReport Summarize(
      const std::vector<PairedDeal>& deals,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2",
      const ConditionalLine* line = nullptr,
      const std::vector<PairedDeal>* opposing_deals = nullptr) const;
};
