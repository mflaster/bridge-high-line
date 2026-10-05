#include "unknown_cards_invariant.h"

#include <algorithm>
#include <memory>
#include <string>

static int CardsInSuit(uint64_t hand, Suit s) {
  uint16_t suit_bits = (hand >> (static_cast<int>(s) * 16)) & 0xffff;
  int count = 0;
  while (suit_bits) {
    count += (suit_bits & 1);
    suit_bits >>= 1;
  }
  return count;
}

// --- UnknownCardsInvariant Implementation ---

std::unique_ptr<UnknownCardsInvariant> UnknownCardsInvariant::FromString(
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
      return std::make_unique<CrossSuitCardConditionalInvariant>(card1, card2);
    } else if (target_str.rfind("exactly ", 0) == 0) {
      size_t space_pos = target_str.find(' ', 8);
      if (space_pos == std::string::npos) return nullptr;
      int n = std::stoi(target_str.substr(8, space_pos - 8));
      std::string s_str = target_str.substr(space_pos + 1);
      Suit s = Card::ParseSuit(s_str);
      return std::make_unique<CrossSuitExactSuitLengthInvariant>(card1, s, n);
    } else if (target_str.rfind("at least ", 0) == 0) {
      size_t space_pos = target_str.find(' ', 9);
      if (space_pos == std::string::npos) return nullptr;
      int n = std::stoi(target_str.substr(9, space_pos - 9));
      std::string s_str = target_str.substr(space_pos + 1);
      Suit s = Card::ParseSuit(s_str);
      return std::make_unique<CrossSuitMinSuitLengthInvariant>(card1, s, n);
    } else if (target_str.rfind("at most ", 0) == 0) {
      size_t space_pos = target_str.find(' ', 8);
      if (space_pos == std::string::npos) return nullptr;
      int n = std::stoi(target_str.substr(8, space_pos - 8));
      std::string s_str = target_str.substr(space_pos + 1);
      Suit s = Card::ParseSuit(s_str);
      return std::make_unique<CrossSuitMaxSuitLengthInvariant>(card1, s, n);
    }
  } catch (...) {
    return nullptr;
  }
  return nullptr;
}

// --- CrossSuitCardConditionalInvariant Implementation ---

std::string CrossSuitCardConditionalInvariant::ToString() const {
  return "the hand with the " + Card::PrintCard(condition_card()) +
         " also has the " + Card::PrintCard(target_card_);
}

bool CrossSuitCardConditionalInvariant::HoldsOnDeal(
    const PairedDeal& deal,
    const std::string& /* hand1_label */,
    const std::string& /* hand2_label */) const {
  if ((deal.hand1 & condition_card()) != 0) {
    return (deal.hand1 & target_card_) != 0;
  }
  if ((deal.hand2 & condition_card()) != 0) {
    return (deal.hand2 & target_card_) != 0;
  }
  return false;
}

// --- CrossSuitExactSuitLengthInvariant Implementation ---

std::string CrossSuitExactSuitLengthInvariant::ToString() const {
  return "the hand with the " + Card::PrintCard(condition_card()) +
         " also has exactly " + std::to_string(length_) + " " + SuitDebugString(target_suit_);
}

bool CrossSuitExactSuitLengthInvariant::HoldsOnDeal(
    const PairedDeal& deal,
    const std::string& /* hand1_label */,
    const std::string& /* hand2_label */) const {
  if ((deal.hand1 & condition_card()) != 0) {
    return CardsInSuit(deal.hand1, target_suit_) == length_;
  }
  if ((deal.hand2 & condition_card()) != 0) {
    return CardsInSuit(deal.hand2, target_suit_) == length_;
  }
  return false;
}

// --- CrossSuitMinSuitLengthInvariant Implementation ---

std::string CrossSuitMinSuitLengthInvariant::ToString() const {
  return "the hand with the " + Card::PrintCard(condition_card()) +
         " also has at least " + std::to_string(min_len_) + " " + SuitDebugString(target_suit_);
}

bool CrossSuitMinSuitLengthInvariant::HoldsOnDeal(
    const PairedDeal& deal,
    const std::string& /* hand1_label */,
    const std::string& /* hand2_label */) const {
  if ((deal.hand1 & condition_card()) != 0) {
    return CardsInSuit(deal.hand1, target_suit_) >= min_len_;
  }
  if ((deal.hand2 & condition_card()) != 0) {
    return CardsInSuit(deal.hand2, target_suit_) >= min_len_;
  }
  return false;
}

// --- CrossSuitMaxSuitLengthInvariant Implementation ---

std::string CrossSuitMaxSuitLengthInvariant::ToString() const {
  return "the hand with the " + Card::PrintCard(condition_card()) +
         " also has at most " + std::to_string(max_len_) + " " + SuitDebugString(target_suit_);
}

bool CrossSuitMaxSuitLengthInvariant::HoldsOnDeal(
    const PairedDeal& deal,
    const std::string& /* hand1_label */,
    const std::string& /* hand2_label */) const {
  if ((deal.hand1 & condition_card()) != 0) {
    return CardsInSuit(deal.hand1, target_suit_) <= max_len_;
  }
  if ((deal.hand2 & condition_card()) != 0) {
    return CardsInSuit(deal.hand2, target_suit_) <= max_len_;
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
