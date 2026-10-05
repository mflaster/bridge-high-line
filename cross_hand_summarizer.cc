#include "cross_hand_summarizer.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <random>
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

// --- Invariant Implementation ---

std::unique_ptr<Invariant> Invariant::FromString(
    const std::string& str,
    const std::string& hand1_label,
    const std::string& hand2_label) {
  if (auto res = CrossSuitConditionalInvariant::FromString(str, hand1_label, hand2_label)) {
    return res;
  }
  if (auto res = HonorPossessionInvariant::FromString(str, hand1_label, hand2_label)) {
    return res;
  }
  if (auto res = MaxSuitLengthInvariant::FromString(str, hand1_label, hand2_label)) {
    return res;
  }
  if (auto res = ExactSuitLengthInvariant::FromString(str, hand1_label, hand2_label)) {
    return res;
  }
  if (auto res = SuitLengthRangeInvariant::FromString(str, hand1_label, hand2_label)) {
    return res;
  }
  return nullptr;
}

// --- CrossSuitConditionalInvariant Implementation ---

std::unique_ptr<CrossSuitConditionalInvariant>
CrossSuitConditionalInvariant::FromString(const std::string& str,
                                           const std::string& /* hand1_label */,
                                           const std::string& /* hand2_label */) {
  try {
    if (str.rfind("the hand with the ", 0) != 0) return nullptr;
    size_t pos1 = 18; // length of "the hand with the "
    size_t pos_also = str.find(" also has ", pos1);
    if (pos_also == std::string::npos) return nullptr;

    std::string card1_str = str.substr(pos1, pos_also - pos1);
    if (card1_str.size() != 2) return nullptr;

    CardType card1 = Card::ParseStringCard(card1_str);
    std::string target_str = str.substr(pos_also + 10); // length of " also has "

    if (target_str.rfind("the ", 0) == 0) {
      std::string card2_str = target_str.substr(4);
      if (card2_str.size() != 2) return nullptr;
      CardType card2 = Card::ParseStringCard(card2_str);
      return std::make_unique<CrossSuitConditionalInvariant>(card1, card2);
    } else if (target_str.rfind("exactly ", 0) == 0) {
      size_t space_pos = target_str.find(' ', 8);
      if (space_pos == std::string::npos) return nullptr;
      int n = std::stoi(target_str.substr(8, space_pos - 8));
      std::string s_str = target_str.substr(space_pos + 1);
      Suit s = Card::ParseSuit(s_str);
      return std::make_unique<CrossSuitConditionalInvariant>(
          card1, ConditionalTargetType::kExactSuitLength, s, n);
    } else if (target_str.rfind("at least ", 0) == 0) {
      size_t space_pos = target_str.find(' ', 9);
      if (space_pos == std::string::npos) return nullptr;
      int n = std::stoi(target_str.substr(9, space_pos - 9));
      std::string s_str = target_str.substr(space_pos + 1);
      Suit s = Card::ParseSuit(s_str);
      return std::make_unique<CrossSuitConditionalInvariant>(
          card1, ConditionalTargetType::kMinSuitLength, s, n);
    } else if (target_str.rfind("at most ", 0) == 0) {
      size_t space_pos = target_str.find(' ', 8);
      if (space_pos == std::string::npos) return nullptr;
      int n = std::stoi(target_str.substr(8, space_pos - 8));
      std::string s_str = target_str.substr(space_pos + 1);
      Suit s = Card::ParseSuit(s_str);
      return std::make_unique<CrossSuitConditionalInvariant>(
          card1, ConditionalTargetType::kMaxSuitLength, s, n);
    }
  } catch (...) {
    return nullptr;
  }
  return nullptr;
}

std::string CrossSuitConditionalInvariant::ToString() const {
  std::string res = "the hand with the " + Card::PrintCard(condition_card_) + " also has ";
  switch (target_type_) {
    case ConditionalTargetType::kCard:
      res += "the " + Card::PrintCard(target_card_);
      break;
    case ConditionalTargetType::kExactSuitLength:
      res += "exactly " + std::to_string(length_) + " " + SuitDebugString(target_suit_);
      break;
    case ConditionalTargetType::kMinSuitLength:
      res += "at least " + std::to_string(length_) + " " + SuitDebugString(target_suit_);
      break;
    case ConditionalTargetType::kMaxSuitLength:
      res += "at most " + std::to_string(length_) + " " + SuitDebugString(target_suit_);
      break;
  }
  return res;
}

bool CrossSuitConditionalInvariant::HoldsOnDeal(
    const PairedDeal& deal,
    const std::string& /* hand1_label */,
    const std::string& /* hand2_label */) const {
  uint64_t target_hand = 0;
  if ((deal.hand1 & condition_card_) != 0) {
    target_hand = deal.hand1;
  } else if ((deal.hand2 & condition_card_) != 0) {
    target_hand = deal.hand2;
  } else {
    return false;
  }

  switch (target_type_) {
    case ConditionalTargetType::kCard:
      return (target_hand & target_card_) != 0;
    case ConditionalTargetType::kExactSuitLength:
      return CardsInSuit(target_hand, target_suit_) == length_;
    case ConditionalTargetType::kMinSuitLength:
      return CardsInSuit(target_hand, target_suit_) >= length_;
    case ConditionalTargetType::kMaxSuitLength:
      return CardsInSuit(target_hand, target_suit_) <= length_;
  }
  return false;
}

// --- HonorPossessionInvariant Implementation ---

std::unique_ptr<HonorPossessionInvariant>
HonorPossessionInvariant::FromString(const std::string& str,
                                     const std::string& hand1_label,
                                     const std::string& hand2_label) {
  try {
    size_t pos = str.find(" always contains the ");
    size_t sub_len = 21; // length of " always contains the "
    if (pos == std::string::npos) {
      pos = str.find(" always contain the ");
      sub_len = 20; // length of " always contain the "
    }
    if (pos == std::string::npos) return nullptr;

    std::string label = str.substr(0, pos);
    std::string card_str = str.substr(pos + sub_len);
    if (card_str.size() != 2) return nullptr;

    CardType card = Card::ParseStringCard(card_str);
    HonorPossessionTargetHand target = HonorPossessionTargetHand::kHand1;

    if (label == "both hands") {
      target = HonorPossessionTargetHand::kBoth;
    } else if (label == hand1_label) {
      target = HonorPossessionTargetHand::kHand1;
    } else if (label == hand2_label) {
      target = HonorPossessionTargetHand::kHand2;
    } else {
      return nullptr;
    }
    return std::make_unique<HonorPossessionInvariant>(target, card, label);
  } catch (...) {
    return nullptr;
  }
}

std::string HonorPossessionInvariant::ToString() const {
  std::string lbl = label_;
  if (lbl.empty()) {
    if (target_hand_ == HonorPossessionTargetHand::kHand1) lbl = "hand 1";
    else if (target_hand_ == HonorPossessionTargetHand::kHand2) lbl = "hand 2";
    else if (target_hand_ == HonorPossessionTargetHand::kBoth) lbl = "both hands";
  }
  if (target_hand_ == HonorPossessionTargetHand::kBoth || lbl == "both hands") {
    return lbl + " always contain the " + Card::PrintCard(card_);
  }
  return lbl + " always contains the " + Card::PrintCard(card_);
}

bool HonorPossessionInvariant::HoldsOnDeal(
    const PairedDeal& deal,
    const std::string& hand1_label,
    const std::string& hand2_label) const {
  if (target_hand_ == HonorPossessionTargetHand::kBoth || label_ == "both hands") {
    return (deal.hand1 & card_) != 0 && (deal.hand2 & card_) != 0;
  }
  if (target_hand_ == HonorPossessionTargetHand::kHand1 || label_ == hand1_label) {
    return (deal.hand1 & card_) != 0;
  }
  if (target_hand_ == HonorPossessionTargetHand::kHand2 || label_ == hand2_label) {
    return (deal.hand2 & card_) != 0;
  }
  return false;
}

// --- MaxSuitLengthInvariant Implementation ---

std::unique_ptr<MaxSuitLengthInvariant>
MaxSuitLengthInvariant::FromString(const std::string& str,
                                   const std::string& /* hand1_label */,
                                   const std::string& /* hand2_label */) {
  try {
    if (str.rfind("maximum length in ", 0) != 0) return nullptr;
    size_t pos_in_any = str.find(" in any hand is ");
    if (pos_in_any == std::string::npos) return nullptr;

    std::string s_str = str.substr(18, pos_in_any - 18);
    int n = std::stoi(str.substr(pos_in_any + 16));
    Suit s = Card::ParseSuit(s_str);
    return std::make_unique<MaxSuitLengthInvariant>(s, n);
  } catch (...) {
    return nullptr;
  }
}

std::string MaxSuitLengthInvariant::ToString() const {
  return "maximum length in " + SuitDebugString(suit_) + " in any hand is " + std::to_string(max_len_);
}

bool MaxSuitLengthInvariant::HoldsOnDeal(
    const PairedDeal& deal,
    const std::string& /* hand1_label */,
    const std::string& /* hand2_label */) const {
  int max_len = std::max(CardsInSuit(deal.hand1, suit_), CardsInSuit(deal.hand2, suit_));
  return max_len <= max_len_;
}

// --- ExactSuitLengthInvariant Implementation ---

std::unique_ptr<ExactSuitLengthInvariant>
ExactSuitLengthInvariant::FromString(const std::string& str,
                                     const std::string& hand1_label,
                                     const std::string& hand2_label) {
  try {
    size_t always_pos = str.find(" is always ");
    size_t len_pos = str.find(" length in ");
    if (always_pos == std::string::npos || len_pos == std::string::npos || len_pos >= always_pos) {
      return nullptr;
    }

    std::string label = str.substr(0, len_pos);
    std::string s_str = str.substr(len_pos + 11, always_pos - (len_pos + 11));
    int n = std::stoi(str.substr(always_pos + 11));
    Suit s = Card::ParseSuit(s_str);

    InvariantHandChoice choice = InvariantHandChoice::kHand1;
    if (label == hand1_label) {
      choice = InvariantHandChoice::kHand1;
    } else if (label == hand2_label) {
      choice = InvariantHandChoice::kHand2;
    } else {
      return nullptr;
    }
    return std::make_unique<ExactSuitLengthInvariant>(choice, s, n, label);
  } catch (...) {
    return nullptr;
  }
}

std::string ExactSuitLengthInvariant::ToString() const {
  std::string lbl = label_;
  if (lbl.empty()) {
    lbl = (hand_choice_ == InvariantHandChoice::kHand1) ? "hand 1" : "hand 2";
  }
  return lbl + " length in " + SuitDebugString(suit_) + " is always " + std::to_string(exact_len_);
}

bool ExactSuitLengthInvariant::HoldsOnDeal(
    const PairedDeal& deal,
    const std::string& hand1_label,
    const std::string& hand2_label) const {
  uint64_t target_hand = 0;
  if (hand_choice_ == InvariantHandChoice::kHand1 || label_ == hand1_label) {
    target_hand = deal.hand1;
  } else if (hand_choice_ == InvariantHandChoice::kHand2 || label_ == hand2_label) {
    target_hand = deal.hand2;
  } else {
    return false;
  }
  return CardsInSuit(target_hand, suit_) == exact_len_;
}

// --- SuitLengthRangeInvariant Implementation ---

std::unique_ptr<SuitLengthRangeInvariant>
SuitLengthRangeInvariant::FromString(const std::string& str,
                                     const std::string& hand1_label,
                                     const std::string& hand2_label) {
  try {
    size_t between_pos = str.find(" is between ");
    size_t len_pos = str.find(" length in ");
    if (between_pos == std::string::npos || len_pos == std::string::npos || len_pos >= between_pos) {
      return nullptr;
    }

    std::string label = str.substr(0, len_pos);
    std::string s_str = str.substr(len_pos + 11, between_pos - (len_pos + 11));
    size_t and_pos = str.find(" and ", between_pos + 12);
    if (and_pos == std::string::npos) return nullptr;

    int n1 = std::stoi(str.substr(between_pos + 12, and_pos - (between_pos + 12)));
    int n2 = std::stoi(str.substr(and_pos + 5));
    Suit s = Card::ParseSuit(s_str);

    InvariantHandChoice choice = InvariantHandChoice::kHand1;
    if (label == hand1_label) {
      choice = InvariantHandChoice::kHand1;
    } else if (label == hand2_label) {
      choice = InvariantHandChoice::kHand2;
    } else {
      return nullptr;
    }
    return std::make_unique<SuitLengthRangeInvariant>(choice, s, n1, n2, label);
  } catch (...) {
    return nullptr;
  }
}

std::string SuitLengthRangeInvariant::ToString() const {
  std::string lbl = label_;
  if (lbl.empty()) {
    lbl = (hand_choice_ == InvariantHandChoice::kHand1) ? "hand 1" : "hand 2";
  }
  return lbl + " length in " + SuitDebugString(suit_) + " is between " +
         std::to_string(min_len_) + " and " + std::to_string(max_len_);
}

bool SuitLengthRangeInvariant::HoldsOnDeal(
    const PairedDeal& deal,
    const std::string& hand1_label,
    const std::string& hand2_label) const {
  uint64_t target_hand = 0;
  if (hand_choice_ == InvariantHandChoice::kHand1 || label_ == hand1_label) {
    target_hand = deal.hand1;
  } else if (hand_choice_ == InvariantHandChoice::kHand2 || label_ == hand2_label) {
    target_hand = deal.hand2;
  } else {
    return false;
  }
  int len = CardsInSuit(target_hand, suit_);
  return len >= min_len_ && len <= max_len_;
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

std::vector<std::unique_ptr<Invariant>> JointSuitRuleExtractor::ExtractInvariants() const {
  std::vector<std::unique_ptr<Invariant>> invariants;
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
      invariants.push_back(std::make_unique<ExactSuitLengthInvariant>(
          InvariantHandChoice::kHand1, suit, bounds.min_hand1, hand1_label_));
    } else {
      invariants.push_back(std::make_unique<SuitLengthRangeInvariant>(
          InvariantHandChoice::kHand1, suit, bounds.min_hand1, bounds.max_hand1, hand1_label_));
    }

    if (bounds.min_hand2 == bounds.max_hand2) {
      invariants.push_back(std::make_unique<ExactSuitLengthInvariant>(
          InvariantHandChoice::kHand2, suit, bounds.min_hand2, hand2_label_));
    } else {
      invariants.push_back(std::make_unique<SuitLengthRangeInvariant>(
          InvariantHandChoice::kHand2, suit, bounds.min_hand2, bounds.max_hand2, hand2_label_));
    }

    if (bounds.min_hand1 != bounds.max_hand1 || bounds.min_hand2 != bounds.max_hand2) {
      int max_either = std::max(bounds.max_hand1, bounds.max_hand2);
      invariants.push_back(std::make_unique<MaxSuitLengthInvariant>(suit, max_either));
    }
  }
  return invariants;
}

std::vector<std::string> JointSuitRuleExtractor::ExtractRules() const {
  std::vector<std::string> rules;
  for (const auto& inv : ExtractInvariants()) {
    rules.push_back(inv->ToString());
  }
  return rules;
}

void HonorPossessionClassifier::ProcessDeal(const PairedDeal& deal) {
  total_deals_++;
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

std::vector<std::unique_ptr<Invariant>> HonorPossessionClassifier::ExtractInvariants() const {
  std::vector<std::unique_ptr<Invariant>> invariants;
  if (total_deals_ == 0) return invariants;

  for (int s = 0; s < 4; ++s) {
    Suit suit = static_cast<Suit>(s);
    auto suit_it = honor_stats_.find(suit);
    if (suit_it == honor_stats_.end()) continue;

    for (int r = 12; r >= 8; --r) {
      auto rank_it = suit_it->second.find(r);
      if (rank_it == suit_it->second.end()) continue;

      const auto& stats = rank_it->second;
      CardType card = Card::GetSuitedCard(suit, r);

      if (stats.hand1_count + stats.both_count == total_deals_ && stats.both_count < total_deals_) {
        invariants.push_back(std::make_unique<HonorPossessionInvariant>(
            HonorPossessionTargetHand::kHand1, card, hand1_label_));
      } else if (stats.hand2_count + stats.both_count == total_deals_ && stats.both_count < total_deals_) {
        invariants.push_back(std::make_unique<HonorPossessionInvariant>(
            HonorPossessionTargetHand::kHand2, card, hand2_label_));
      } else if (stats.both_count == total_deals_) {
        invariants.push_back(std::make_unique<HonorPossessionInvariant>(
            HonorPossessionTargetHand::kBoth, card, "both hands"));
      }
    }
  }
  return invariants;
}

std::vector<std::string> HonorPossessionClassifier::ExtractRules() const {
  std::vector<std::string> rules;
  for (const auto& inv : ExtractInvariants()) {
    rules.push_back(inv->ToString());
  }
  return rules;
}

void CrossSuitConditionalRuleExtractor::ProcessDeal(const PairedDeal& deal) {
  total_deals_++;
  deals_.push_back(deal);
}

std::vector<std::unique_ptr<Invariant>> CrossSuitConditionalRuleExtractor::ExtractInvariants() const {
  std::vector<std::unique_ptr<Invariant>> invariants;
  if (total_deals_ == 0 || deals_.empty()) return invariants;

  uint64_t unknown_cards = deals_[0].hand1 | deals_[0].hand2;
  int cards_per_hand = PopCount64(deals_[0].hand1);

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
    int hand1_deals = 0;
    int hand2_deals = 0;
    for (const auto& deal : deals_) {
      if ((deal.hand1 & card) != 0) {
        hand1_deals++;
      } else {
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

      if (min_len_X > 0) {
        if (min_len_notX < min_len_X) {
          ConditionalTargetType ttype = (min_len_X == max_len_X)
                                            ? ConditionalTargetType::kExactSuitLength
                                            : ConditionalTargetType::kMinSuitLength;
          auto inv = std::make_unique<CrossSuitConditionalInvariant>(card, ttype, suit, min_len_X);
          std::string rule = inv->ToString();
          if (emitted_rules.insert(rule).second) {
            invariants.push_back(std::move(inv));
          }
        }
      }

      if (min_len_X < max_len_X && max_len_X < cards_per_hand) {
        if (max_len_notX > max_len_X) {
          auto inv = std::make_unique<CrossSuitConditionalInvariant>(
              card, ConditionalTargetType::kMaxSuitLength, suit, max_len_X);
          std::string rule = inv->ToString();
          if (emitted_rules.insert(rule).second) {
            invariants.push_back(std::move(inv));
          }
        }
      }
    }

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
        auto inv = std::make_unique<CrossSuitConditionalInvariant>(card, card2);
        std::string rule = inv->ToString();
        if (emitted_rules.insert(rule).second) {
          invariants.push_back(std::move(inv));
        }
      }
    }
  }

  return invariants;
}

std::vector<std::string> CrossSuitConditionalRuleExtractor::ExtractRules() const {
  std::vector<std::string> rules;
  for (const auto& inv : ExtractInvariants()) {
    rules.push_back(inv->ToString());
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
  auto inv = Invariant::FromString(rule, hand1_label, hand2_label);
  if (!inv) return false;
  return inv->HoldsOnDeal(deal, hand1_label, hand2_label);
}

std::vector<std::unique_ptr<Invariant>> FilterUninterestingInvariants(
    std::vector<std::unique_ptr<Invariant>> candidate_invariants,
    uint64_t unknown_cards,
    int hand1_size,
    const std::string& hand1_label,
    const std::string& hand2_label,
    int num_bifurcations) {
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

  std::vector<std::unique_ptr<Invariant>> filtered;
  for (auto& inv : candidate_invariants) {
    if (!inv) continue;
    bool true_for_all_random = true;
    for (const auto& deal : bifurcations) {
      if (!inv->HoldsOnDeal(deal, hand1_label, hand2_label)) {
        true_for_all_random = false;
        break;
      }
    }
    if (!true_for_all_random) {
      filtered.push_back(std::move(inv));
    }
  }
  return filtered;
}

std::vector<std::string> FilterUninterestingRules(
    const std::vector<std::string>& candidate_rules,
    uint64_t unknown_cards,
    int hand1_size,
    const std::string& hand1_label,
    const std::string& hand2_label,
    int num_bifurcations) {
  std::vector<std::unique_ptr<Invariant>> candidates;
  for (const auto& rule : candidate_rules) {
    if (auto inv = Invariant::FromString(rule, hand1_label, hand2_label)) {
      candidates.push_back(std::move(inv));
    }
  }
  auto filtered = FilterUninterestingInvariants(
      std::move(candidates), unknown_cards, hand1_size, hand1_label, hand2_label, num_bifurcations);
  std::vector<std::string> result;
  for (const auto& inv : filtered) {
    if (inv) {
      result.push_back(inv->ToString());
    }
  }
  return result;
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

  auto suit_invariants = suit_extractor.ExtractInvariants();
  auto conditional_invariants = conditional_extractor.ExtractInvariants();
  auto honor_invariants = honor_classifier.ExtractInvariants();

  std::vector<std::unique_ptr<Invariant>> candidate_invariants;
  for (auto& inv : suit_invariants) candidate_invariants.push_back(std::move(inv));
  for (auto& inv : conditional_invariants) candidate_invariants.push_back(std::move(inv));
  for (auto& inv : honor_invariants) candidate_invariants.push_back(std::move(inv));

  auto filtered_invariants = FilterUninterestingInvariants(
      std::move(candidate_invariants), report.unknown_cards, report.cards_per_hand,
      hand1_label, hand2_label, 200);

  report.invariant_rules.clear();
  for (const auto& inv : filtered_invariants) {
    if (inv) {
      report.invariant_rules.push_back(inv->ToString());
    }
  }

  return report;
}
