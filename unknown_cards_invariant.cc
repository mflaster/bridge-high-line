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

std::shared_ptr<UnknownCardsInvariant> UnknownCardsInvariant::FromString(
    const std::string& str,
    const std::string& hand1_label,
    const std::string& hand2_label) {
  if (auto res = OrInvariant::FromString(str, hand1_label, hand2_label)) {
    return res;
  }
  if (auto res = AndInvariant::FromString(str, hand1_label, hand2_label)) {
    return res;
  }
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

// --- OrInvariant Implementation ---

std::shared_ptr<OrInvariant> OrInvariant::FromString(
    const std::string& str,
    const std::string& hand1_label,
    const std::string& hand2_label) {
  try {
    if (str.rfind("OrInvariant(", 0) != 0 || str.back() != ')') {
      return nullptr;
    }
    std::string content = str.substr(12, str.size() - 13);
    size_t comma_pos = std::string::npos;
    int depth = 0;
    for (size_t i = 0; i < content.size(); ++i) {
      if (content[i] == '(') {
        depth++;
      } else if (content[i] == ')') {
        depth--;
      } else if (depth == 0 && content.substr(i, 2) == ", ") {
        comma_pos = i;
        break;
      }
    }
    if (comma_pos == std::string::npos) {
      return nullptr;
    }
    std::string left_str = content.substr(0, comma_pos);
    std::string right_str = content.substr(comma_pos + 2);

    auto left = UnknownCardsInvariant::FromString(left_str, hand1_label, hand2_label);
    auto right = UnknownCardsInvariant::FromString(right_str, hand1_label, hand2_label);
    if (!left || !right) {
      return nullptr;
    }
    return std::make_shared<OrInvariant>(left, right);
  } catch (...) {
    return nullptr;
  }
}

std::string OrInvariant::ToString() const {
  std::string l_str = left_ ? left_->ToString() : "";
  std::string r_str = right_ ? right_->ToString() : "";
  return "OrInvariant(" + l_str + ", " + r_str + ")";
}

bool OrInvariant::HoldsOnDeal(
    const PairedDeal& deal,
    const std::string& hand1_label,
    const std::string& hand2_label) const {
  bool l_holds = left_ && left_->HoldsOnDeal(deal, hand1_label, hand2_label);
  bool r_holds = right_ && right_->HoldsOnDeal(deal, hand1_label, hand2_label);
  return l_holds || r_holds;
}

std::shared_ptr<UnknownCardsInvariant> OrInvariant::Invert(
    uint64_t unknown_cards,
    int hand1_size,
    const std::string& hand1_label,
    const std::string& hand2_label) const {
  if (!left_ || !right_) return nullptr;
  auto inv_left =
      left_->Invert(unknown_cards, hand1_size, hand1_label, hand2_label);
  auto inv_right =
      right_->Invert(unknown_cards, hand1_size, hand1_label, hand2_label);
  if (!inv_left || !inv_right) return nullptr;
  if (inv_left->ToString() > inv_right->ToString()) {
    std::swap(inv_left, inv_right);
  }
  return std::make_shared<AndInvariant>(inv_left, inv_right);
}

// --- AndInvariant Implementation ---

std::shared_ptr<AndInvariant> AndInvariant::FromString(
    const std::string& str,
    const std::string& hand1_label,
    const std::string& hand2_label) {
  try {
    if (str.rfind("AndInvariant(", 0) != 0 || str.back() != ')') {
      return nullptr;
    }
    std::string content = str.substr(13, str.size() - 14);
    size_t comma_pos = std::string::npos;
    int depth = 0;
    for (size_t i = 0; i < content.size(); ++i) {
      if (content[i] == '(') {
        depth++;
      } else if (content[i] == ')') {
        depth--;
      } else if (depth == 0 && content.substr(i, 2) == ", ") {
        comma_pos = i;
        break;
      }
    }
    if (comma_pos == std::string::npos) {
      return nullptr;
    }
    std::string left_str = content.substr(0, comma_pos);
    std::string right_str = content.substr(comma_pos + 2);

    auto left =
        UnknownCardsInvariant::FromString(left_str, hand1_label, hand2_label);
    auto right =
        UnknownCardsInvariant::FromString(right_str, hand1_label, hand2_label);
    if (!left || !right) {
      return nullptr;
    }
    return std::make_shared<AndInvariant>(left, right);
  } catch (...) {
    return nullptr;
  }
}

std::string AndInvariant::ToString() const {
  std::string l_str = left_ ? left_->ToString() : "";
  std::string r_str = right_ ? right_->ToString() : "";
  return "AndInvariant(" + l_str + ", " + r_str + ")";
}

bool AndInvariant::HoldsOnDeal(
    const PairedDeal& deal,
    const std::string& hand1_label,
    const std::string& hand2_label) const {
  bool l_holds = left_ && left_->HoldsOnDeal(deal, hand1_label, hand2_label);
  bool r_holds = right_ && right_->HoldsOnDeal(deal, hand1_label, hand2_label);
  return l_holds && r_holds;
}

std::shared_ptr<UnknownCardsInvariant> AndInvariant::Invert(
    uint64_t unknown_cards,
    int hand1_size,
    const std::string& hand1_label,
    const std::string& hand2_label) const {
  if (!left_ || !right_) return nullptr;
  auto inv_left =
      left_->Invert(unknown_cards, hand1_size, hand1_label, hand2_label);
  auto inv_right =
      right_->Invert(unknown_cards, hand1_size, hand1_label, hand2_label);
  if (!inv_left || !inv_right) return nullptr;
  if (inv_left->ToString() > inv_right->ToString()) {
    std::swap(inv_left, inv_right);
  }
  return std::make_shared<OrInvariant>(inv_left, inv_right);
}

// --- CrossSuitConditionalInvariant Implementation ---

std::shared_ptr<CrossSuitConditionalInvariant>
CrossSuitConditionalInvariant::FromString(
    const std::string& str,
    const std::string& /* hand1_label */,
    const std::string& /* hand2_label */) {
  try {
    if (str.rfind("the hand with the ", 0) != 0) return nullptr;
    size_t pos1 = 18;  // length of "the hand with the "
    size_t pos_also = str.find(" also has ", pos1);
    if (pos_also == std::string::npos) return nullptr;

    std::string card1_str = str.substr(pos1, pos_also - pos1);
    if (card1_str.size() != 2) return nullptr;

    CardType card1 = Card::ParseStringCard(card1_str);
    std::string target_str = str.substr(pos_also + 10);

    if (target_str.rfind("the ", 0) == 0) {
      std::string card2_str = target_str.substr(4);
      if (card2_str.size() != 2) return nullptr;
      CardType card2 = Card::ParseStringCard(card2_str);
      return std::make_shared<CrossSuitCardConditionalInvariant>(card1, card2);
    } else if (target_str.rfind("exactly ", 0) == 0) {
      size_t space_pos = target_str.find(' ', 8);
      if (space_pos == std::string::npos) return nullptr;
      int n = std::stoi(target_str.substr(8, space_pos - 8));
      std::string s_str = target_str.substr(space_pos + 1);
      Suit s = Card::ParseSuit(s_str);
      return std::make_shared<CrossSuitExactSuitLengthInvariant>(card1, s, n);
    } else if (target_str.rfind("at least ", 0) == 0) {
      size_t space_pos = target_str.find(' ', 9);
      if (space_pos == std::string::npos) return nullptr;
      int n = std::stoi(target_str.substr(9, space_pos - 9));
      std::string s_str = target_str.substr(space_pos + 1);
      Suit s = Card::ParseSuit(s_str);
      return std::make_shared<CrossSuitMinSuitLengthInvariant>(card1, s, n);
    } else if (target_str.rfind("at most ", 0) == 0) {
      size_t space_pos = target_str.find(' ', 8);
      if (space_pos == std::string::npos) return nullptr;
      int n = std::stoi(target_str.substr(8, space_pos - 8));
      std::string s_str = target_str.substr(space_pos + 1);
      Suit s = Card::ParseSuit(s_str);
      return std::make_shared<CrossSuitMaxSuitLengthInvariant>(card1, s, n);
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

std::shared_ptr<UnknownCardsInvariant>
CrossSuitCardConditionalInvariant::Invert(
    uint64_t unknown_cards,
    int /*hand1_size*/,
    const std::string& hand1_label,
    const std::string& hand2_label) const {
  if (condition_card() == target_card_) return nullptr;
  if (unknown_cards != 0 &&
      ((unknown_cards & condition_card()) == 0 ||
       (unknown_cards & target_card_) == 0)) {
    return nullptr;
  }
  std::string l1 = (hand1_label == "hand 1") ? "E" : hand1_label;
  std::string l2 =
      (hand2_label == "hand 2" || hand2_label == "hand 3") ? "W" : hand2_label;
  std::shared_ptr<UnknownCardsInvariant> a1 =
      std::make_shared<HonorPossessionInvariant>(
          HonorPossessionTargetHand::kHand1, condition_card(), l1);
  std::shared_ptr<UnknownCardsInvariant> a2 =
      std::make_shared<HonorPossessionInvariant>(
          HonorPossessionTargetHand::kHand2, target_card_, l2);
  if (a1->ToString() > a2->ToString()) std::swap(a1, a2);

  std::shared_ptr<UnknownCardsInvariant> b1 =
      std::make_shared<HonorPossessionInvariant>(
          HonorPossessionTargetHand::kHand2, condition_card(), l2);
  std::shared_ptr<UnknownCardsInvariant> b2 =
      std::make_shared<HonorPossessionInvariant>(
          HonorPossessionTargetHand::kHand1, target_card_, l1);
  if (b1->ToString() > b2->ToString()) std::swap(b1, b2);

  std::shared_ptr<UnknownCardsInvariant> left =
      std::make_shared<AndInvariant>(a1, a2);
  std::shared_ptr<UnknownCardsInvariant> right =
      std::make_shared<AndInvariant>(b1, b2);
  if (left->ToString() > right->ToString()) std::swap(left, right);
  return std::make_shared<OrInvariant>(left, right);
}

// --- CrossSuitExactSuitLengthInvariant Implementation ---

std::string CrossSuitExactSuitLengthInvariant::ToString() const {
  return "the hand with the " + Card::PrintCard(condition_card()) +
         " also has exactly " + std::to_string(length_) + " " +
         SuitDebugString(target_suit_);
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

std::shared_ptr<UnknownCardsInvariant>
CrossSuitExactSuitLengthInvariant::Invert(
    uint64_t unknown_cards,
    int /*hand1_size*/,
    const std::string& /*hand1_label*/,
    const std::string& /*hand2_label*/) const {
  if (unknown_cards != 0 && (unknown_cards & condition_card()) == 0) {
    return nullptr;
  }
  int total_suit = CardsInSuit(unknown_cards, target_suit_);
  int min_possible =
      (Card::DecodeCard(condition_card()).first == target_suit_) ? 1 : 0;
  int max_possible = (unknown_cards != 0) ? total_suit : (length_ + 1);
  if (length_ < min_possible || length_ > max_possible ||
      min_possible == max_possible) {
    return nullptr;
  }
  if (length_ == min_possible) {
    return std::make_shared<CrossSuitMinSuitLengthInvariant>(
        condition_card(), target_suit_, length_ + 1);
  }
  if (length_ == max_possible) {
    return std::make_shared<CrossSuitMaxSuitLengthInvariant>(
        condition_card(), target_suit_, length_ - 1);
  }
  auto low = std::make_shared<CrossSuitMaxSuitLengthInvariant>(
      condition_card(), target_suit_, length_ - 1);
  auto high = std::make_shared<CrossSuitMinSuitLengthInvariant>(
      condition_card(), target_suit_, length_ + 1);
  return std::make_shared<OrInvariant>(low, high);
}

// --- CrossSuitMinSuitLengthInvariant Implementation ---

std::string CrossSuitMinSuitLengthInvariant::ToString() const {
  return "the hand with the " + Card::PrintCard(condition_card()) +
         " also has at least " + std::to_string(min_len_) + " " +
         SuitDebugString(target_suit_);
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

std::shared_ptr<UnknownCardsInvariant> CrossSuitMinSuitLengthInvariant::Invert(
    uint64_t unknown_cards,
    int /*hand1_size*/,
    const std::string& /*hand1_label*/,
    const std::string& /*hand2_label*/) const {
  if (unknown_cards != 0 && (unknown_cards & condition_card()) == 0) {
    return nullptr;
  }
  int min_possible =
      (Card::DecodeCard(condition_card()).first == target_suit_) ? 1 : 0;
  if (min_len_ <= min_possible) {
    return nullptr;
  }
  return std::make_shared<CrossSuitMaxSuitLengthInvariant>(
      condition_card(), target_suit_, min_len_ - 1);
}

// --- CrossSuitMaxSuitLengthInvariant Implementation ---

std::string CrossSuitMaxSuitLengthInvariant::ToString() const {
  return "the hand with the " + Card::PrintCard(condition_card()) +
         " also has at most " + std::to_string(max_len_) + " " +
         SuitDebugString(target_suit_);
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

std::shared_ptr<UnknownCardsInvariant> CrossSuitMaxSuitLengthInvariant::Invert(
    uint64_t unknown_cards,
    int /*hand1_size*/,
    const std::string& /*hand1_label*/,
    const std::string& /*hand2_label*/) const {
  if (unknown_cards != 0 && (unknown_cards & condition_card()) == 0) {
    return nullptr;
  }
  int total_suit = CardsInSuit(unknown_cards, target_suit_);
  if (unknown_cards != 0 && max_len_ >= total_suit) {
    return nullptr;
  }
  return std::make_shared<CrossSuitMinSuitLengthInvariant>(
      condition_card(), target_suit_, max_len_ + 1);
}

// --- HonorPossessionInvariant Implementation ---

std::shared_ptr<HonorPossessionInvariant>
HonorPossessionInvariant::FromString(const std::string& str,
                                     const std::string& hand1_label,
                                     const std::string& hand2_label) {
  try {
    size_t pos = str.find(" always contains the ");
    size_t sub_len = 21;  // length of " always contains the "
    if (pos == std::string::npos) {
      pos = str.find(" always contain the ");
      sub_len = 20;  // length of " always contain the "
    }
    if (pos == std::string::npos) {
      pos = str.find(" has the ");
      sub_len = 9;  // length of " has the "
    }
    if (pos == std::string::npos) return nullptr;

    std::string label = str.substr(0, pos);
    std::string card_str = str.substr(pos + sub_len);
    if (card_str.size() != 2) return nullptr;

    CardType card = Card::ParseStringCard(card_str);
    HonorPossessionTargetHand target = HonorPossessionTargetHand::kHand1;

    if (label == hand1_label) {
      target = HonorPossessionTargetHand::kHand1;
    } else if (label == hand2_label) {
      target = HonorPossessionTargetHand::kHand2;
    } else if (label == "both hands") {
      target = HonorPossessionTargetHand::kBoth;
    } else if (label == "E" || label == "N" || label == "hand 1") {
      target = HonorPossessionTargetHand::kHand1;
    } else if (label == "W" || label == "S" || label == "hand 2" ||
               label == "hand 3") {
      target = HonorPossessionTargetHand::kHand2;
    } else {
      return nullptr;
    }
    return std::make_shared<HonorPossessionInvariant>(target, card, label);
  } catch (...) {
    return nullptr;
  }
}

std::string HonorPossessionInvariant::ToString() const {
  std::string lbl = label_;
  if (lbl.empty()) {
    if (target_hand_ == HonorPossessionTargetHand::kHand1) {
      lbl = "hand 1";
    } else if (target_hand_ == HonorPossessionTargetHand::kHand2) {
      lbl = "hand 2";
    } else if (target_hand_ == HonorPossessionTargetHand::kBoth) {
      lbl = "both hands";
    }
  }
  if (lbl == "E" || lbl == "W" || lbl == "N" || lbl == "S") {
    return lbl + " has the " + Card::PrintCard(card_);
  }
  if (target_hand_ == HonorPossessionTargetHand::kBoth || lbl == "both hands") {
    return lbl + " always contain the " + Card::PrintCard(card_);
  }
  return lbl + " always contains the " + Card::PrintCard(card_);
}

bool HonorPossessionInvariant::HoldsOnDeal(
    const PairedDeal& deal,
    const std::string& /*hand1_label*/,
    const std::string& /*hand2_label*/) const {
  if (target_hand_ == HonorPossessionTargetHand::kBoth ||
      label_ == "both hands") {
    return (deal.hand1 & card_) != 0 && (deal.hand2 & card_) != 0;
  }
  if (target_hand_ == HonorPossessionTargetHand::kHand1) {
    return (deal.hand1 & card_) != 0;
  }
  if (target_hand_ == HonorPossessionTargetHand::kHand2) {
    return (deal.hand2 & card_) != 0;
  }
  return false;
}

std::shared_ptr<UnknownCardsInvariant> HonorPossessionInvariant::Invert(
    uint64_t unknown_cards,
    int /*hand1_size*/,
    const std::string& hand1_label,
    const std::string& hand2_label) const {
  if (target_hand_ == HonorPossessionTargetHand::kBoth ||
      label_ == "both hands") {
    return nullptr;
  }
  if (unknown_cards != 0 && (unknown_cards & card_) == 0) {
    return nullptr;
  }
  HonorPossessionTargetHand inv_target =
      (target_hand_ == HonorPossessionTargetHand::kHand1)
          ? HonorPossessionTargetHand::kHand2
          : HonorPossessionTargetHand::kHand1;
  std::string inv_label;
  if (label_ == "E") {
    inv_label = "W";
  } else if (label_ == "W") {
    inv_label = "E";
  } else if (label_ == "N") {
    inv_label = "S";
  } else if (label_ == "S") {
    inv_label = "N";
  } else {
    inv_label = (inv_target == HonorPossessionTargetHand::kHand1)
                    ? hand1_label
                    : hand2_label;
  }
  return std::make_shared<HonorPossessionInvariant>(
      inv_target, card_, inv_label);
}

static void ComputeHandSuitBounds(uint64_t unknown_cards,
                                  int hand1_size,
                                  InvariantHandChoice choice,
                                  Suit suit,
                                  int* min_possible,
                                  int* max_possible) {
  int total_suit = CardsInSuit(unknown_cards, suit);
  int total_unknown = CountCardsInHand(unknown_cards);
  int h1_size = (hand1_size > 0) ? hand1_size : (total_unknown / 2);
  if (total_unknown > 0 && h1_size > 0 && h1_size < total_unknown) {
    int this_size = (choice == InvariantHandChoice::kHand1)
                        ? h1_size
                        : (total_unknown - h1_size);
    int other_size = total_unknown - this_size;
    *min_possible = std::max(0, total_suit - other_size);
    *max_possible = std::min(total_suit, this_size);
  } else {
    *min_possible = 0;
    *max_possible = total_suit;
  }
}

static std::shared_ptr<UnknownCardsInvariant> MakeSuitIntervalInvariant(
    InvariantHandChoice choice,
    Suit suit,
    int lo,
    int hi,
    const std::string& label) {
  if (lo > hi) return nullptr;
  if (lo == hi) {
    return std::make_shared<ExactSuitLengthInvariant>(choice, suit, lo, label);
  }
  return std::make_shared<SuitLengthRangeInvariant>(
      choice, suit, lo, hi, label);
}

static std::shared_ptr<UnknownCardsInvariant> InvertSuitInterval(
    InvariantHandChoice choice,
    Suit suit,
    int lo,
    int hi,
    const std::string& label,
    uint64_t unknown_cards,
    int hand1_size,
    const std::string& hand1_label,
    const std::string& hand2_label) {
  if (unknown_cards == 0) return nullptr;
  int min_possible = 0;
  int max_possible = 0;
  ComputeHandSuitBounds(
      unknown_cards, hand1_size, choice, suit, &min_possible, &max_possible);
  int clamped_lo = std::max(lo, min_possible);
  int clamped_hi = std::min(hi, max_possible);
  if (clamped_lo > clamped_hi) return nullptr;
  if (clamped_lo == min_possible && clamped_hi == max_possible) {
    return nullptr;
  }
  std::string lbl = label;
  if (lbl.empty()) {
    lbl = (choice == InvariantHandChoice::kHand1) ? hand1_label : hand2_label;
  }
  if (clamped_lo == min_possible) {
    return MakeSuitIntervalInvariant(
        choice, suit, clamped_hi + 1, max_possible, lbl);
  }
  if (clamped_hi == max_possible) {
    return MakeSuitIntervalInvariant(
        choice, suit, min_possible, clamped_lo - 1, lbl);
  }
  auto low = MakeSuitIntervalInvariant(
      choice, suit, min_possible, clamped_lo - 1, lbl);
  auto high = MakeSuitIntervalInvariant(
      choice, suit, clamped_hi + 1, max_possible, lbl);
  return std::make_shared<OrInvariant>(low, high);
}

// --- MaxSuitLengthInvariant Implementation ---

std::shared_ptr<MaxSuitLengthInvariant>
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
    return std::make_shared<MaxSuitLengthInvariant>(s, n);
  } catch (...) {
    return nullptr;
  }
}

std::string MaxSuitLengthInvariant::ToString() const {
  return "maximum length in " + SuitDebugString(suit_) + " in any hand is " +
         std::to_string(max_len_);
}

bool MaxSuitLengthInvariant::HoldsOnDeal(
    const PairedDeal& deal,
    const std::string& /* hand1_label */,
    const std::string& /* hand2_label */) const {
  int max_len =
      std::max(CardsInSuit(deal.hand1, suit_), CardsInSuit(deal.hand2, suit_));
  return max_len <= max_len_;
}

std::shared_ptr<UnknownCardsInvariant> MaxSuitLengthInvariant::Invert(
    uint64_t unknown_cards,
    int hand1_size,
    const std::string& hand1_label,
    const std::string& hand2_label) const {
  if (unknown_cards == 0) return nullptr;
  int min1 = 0, max1 = 0, min2 = 0, max2 = 0;
  ComputeHandSuitBounds(unknown_cards, hand1_size, InvariantHandChoice::kHand1,
                        suit_, &min1, &max1);
  ComputeHandSuitBounds(unknown_cards, hand1_size, InvariantHandChoice::kHand2,
                        suit_, &min2, &max2);
  auto inv1 = MakeSuitIntervalInvariant(
      InvariantHandChoice::kHand1, suit_, max_len_ + 1, max1, hand1_label);
  auto inv2 = MakeSuitIntervalInvariant(
      InvariantHandChoice::kHand2, suit_, max_len_ + 1, max2, hand2_label);
  if (inv1 && inv2) {
    if (inv1->ToString() > inv2->ToString()) std::swap(inv1, inv2);
    return std::make_shared<OrInvariant>(inv1, inv2);
  }
  return inv1 ? inv1 : inv2;
}

// --- ExactSuitLengthInvariant Implementation ---

std::shared_ptr<ExactSuitLengthInvariant>
ExactSuitLengthInvariant::FromString(const std::string& str,
                                     const std::string& hand1_label,
                                     const std::string& hand2_label) {
  try {
    size_t always_pos = str.find(" is always ");
    size_t len_pos = str.find(" length in ");
    if (always_pos == std::string::npos || len_pos == std::string::npos ||
        len_pos >= always_pos) {
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
    return std::make_shared<ExactSuitLengthInvariant>(choice, s, n, label);
  } catch (...) {
    return nullptr;
  }
}

std::string ExactSuitLengthInvariant::ToString() const {
  std::string lbl = label_;
  if (lbl.empty()) {
    lbl = (hand_choice_ == InvariantHandChoice::kHand1) ? "hand 1" : "hand 2";
  }
  return lbl + " length in " + SuitDebugString(suit_) + " is always " +
         std::to_string(exact_len_);
}

bool ExactSuitLengthInvariant::HoldsOnDeal(
    const PairedDeal& deal,
    const std::string& hand1_label,
    const std::string& hand2_label) const {
  uint64_t target_hand = 0;
  if (hand_choice_ == InvariantHandChoice::kHand1 || label_ == hand1_label) {
    target_hand = deal.hand1;
  } else if (hand_choice_ == InvariantHandChoice::kHand2 ||
             label_ == hand2_label) {
    target_hand = deal.hand2;
  } else {
    return false;
  }
  return CardsInSuit(target_hand, suit_) == exact_len_;
}

std::shared_ptr<UnknownCardsInvariant> ExactSuitLengthInvariant::Invert(
    uint64_t unknown_cards,
    int hand1_size,
    const std::string& hand1_label,
    const std::string& hand2_label) const {
  return InvertSuitInterval(hand_choice_, suit_, exact_len_, exact_len_, label_,
                            unknown_cards, hand1_size, hand1_label,
                            hand2_label);
}

// --- SuitLengthRangeInvariant Implementation ---

std::shared_ptr<SuitLengthRangeInvariant>
SuitLengthRangeInvariant::FromString(const std::string& str,
                                     const std::string& hand1_label,
                                     const std::string& hand2_label) {
  try {
    size_t between_pos = str.find(" is between ");
    size_t len_pos = str.find(" length in ");
    if (between_pos == std::string::npos || len_pos == std::string::npos ||
        len_pos >= between_pos) {
      return nullptr;
    }

    std::string label = str.substr(0, len_pos);
    std::string s_str = str.substr(len_pos + 11, between_pos - (len_pos + 11));
    size_t and_pos = str.find(" and ", between_pos + 12);
    if (and_pos == std::string::npos) return nullptr;

    int n1 =
        std::stoi(str.substr(between_pos + 12, and_pos - (between_pos + 12)));
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
    return std::make_shared<SuitLengthRangeInvariant>(choice, s, n1, n2, label);
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
  } else if (hand_choice_ == InvariantHandChoice::kHand2 ||
             label_ == hand2_label) {
    target_hand = deal.hand2;
  } else {
    return false;
  }
  int len = CardsInSuit(target_hand, suit_);
  return len >= min_len_ && len <= max_len_;
}

std::shared_ptr<UnknownCardsInvariant> SuitLengthRangeInvariant::Invert(
    uint64_t unknown_cards,
    int hand1_size,
    const std::string& hand1_label,
    const std::string& hand2_label) const {
  return InvertSuitInterval(hand_choice_, suit_, min_len_, max_len_, label_,
                            unknown_cards, hand1_size, hand1_label,
                            hand2_label);
}
