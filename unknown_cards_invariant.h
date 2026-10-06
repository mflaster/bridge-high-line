#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "card.h"
#include "hand.h"

struct PairedDeal {
  uint64_t hand1 = 0;
  uint64_t hand2 = 0;

  PairedDeal() = default;
  PairedDeal(uint64_t h1, uint64_t h2) : hand1(h1), hand2(h2) {}
  PairedDeal(const std::string& h1_str, const std::string& h2_str)
      : hand1(ParseHand(h1_str)), hand2(ParseHand(h2_str)) {}
};

// --- UnknownCardsInvariant Class Hierarchy ---

// Abstract base class representing a structural or distribution invariant that
// holds across unknown card distributions in a set of paired deals.
class UnknownCardsInvariant {
 public:
  virtual ~UnknownCardsInvariant() = default;

  virtual std::string ToString() const = 0;

  virtual bool HoldsOnDeal(
      const PairedDeal& deal,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2") const = 0;

  static std::shared_ptr<UnknownCardsInvariant> FromString(
      const std::string& str,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2");
};

// Represents a logical disjunction over two UnknownCardsInvariants.
class OrInvariant : public UnknownCardsInvariant {
 public:
  OrInvariant(std::shared_ptr<UnknownCardsInvariant> left,
              std::shared_ptr<UnknownCardsInvariant> right)
      : left_(std::move(left)), right_(std::move(right)) {}

  std::string ToString() const override;

  bool HoldsOnDeal(
      const PairedDeal& deal,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2") const override;

  static std::shared_ptr<OrInvariant> FromString(
      const std::string& str,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2");

  std::shared_ptr<UnknownCardsInvariant> left() const { return left_; }
  std::shared_ptr<UnknownCardsInvariant> right() const { return right_; }

 private:
  std::shared_ptr<UnknownCardsInvariant> left_;
  std::shared_ptr<UnknownCardsInvariant> right_;
};

// Abstract intermediate base class for conditional invariants where possession of a
// specific card ("condition_card") in a hand implies a restriction on another suit or
// card in that same hand.
class CrossSuitConditionalInvariant : public UnknownCardsInvariant {
 public:
  explicit CrossSuitConditionalInvariant(CardType condition_card)
      : condition_card_(condition_card) {}

  static std::shared_ptr<CrossSuitConditionalInvariant> FromString(
      const std::string& str,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2");

  CardType condition_card() const { return condition_card_; }

 private:
  CardType condition_card_ = 0;
};

// Describes card co-possession rules within the same hand: holding a specific
// condition card implies holding another target card in the same hand.
// Example string format: "the hand with the KH also has AC"
class CrossSuitCardConditionalInvariant : public CrossSuitConditionalInvariant {
 public:
  CrossSuitCardConditionalInvariant(CardType condition_card, CardType target_card)
      : CrossSuitConditionalInvariant(condition_card), target_card_(target_card) {}

  std::string ToString() const override;

  bool HoldsOnDeal(
      const PairedDeal& deal,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2") const override;

  CardType target_card() const { return target_card_; }

 private:
  CardType target_card_ = 0;
};

// Describes cross-suit conditional exact length rules: holding a specific
// condition card implies that hand has an exact card count in a target suit.
// Example string format: "the hand with the KH length in C is always 3"
class CrossSuitExactSuitLengthInvariant : public CrossSuitConditionalInvariant {
 public:
  CrossSuitExactSuitLengthInvariant(CardType condition_card,
                                    Suit target_suit,
                                    int length)
      : CrossSuitConditionalInvariant(condition_card),
        target_suit_(target_suit),
        length_(length) {}

  std::string ToString() const override;

  bool HoldsOnDeal(
      const PairedDeal& deal,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2") const override;

  Suit target_suit() const { return target_suit_; }
  int length() const { return length_; }

 private:
  Suit target_suit_ = Suit::SPADES;
  int length_ = 0;
};

// Describes cross-suit conditional minimum length rules: holding a specific
// condition card implies that hand has at least N cards in a target suit.
// Example string format: "the hand with the KH length in C is at least 3"
class CrossSuitMinSuitLengthInvariant : public CrossSuitConditionalInvariant {
 public:
  CrossSuitMinSuitLengthInvariant(CardType condition_card,
                                  Suit target_suit,
                                  int min_len)
      : CrossSuitConditionalInvariant(condition_card),
        target_suit_(target_suit),
        min_len_(min_len) {}

  std::string ToString() const override;

  bool HoldsOnDeal(
      const PairedDeal& deal,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2") const override;

  Suit target_suit() const { return target_suit_; }
  int min_len() const { return min_len_; }

 private:
  Suit target_suit_ = Suit::SPADES;
  int min_len_ = 0;
};

// Describes cross-suit conditional maximum length rules: holding a specific
// condition card implies that hand has at most N cards in a target suit.
// Example string format: "the hand with the KH length in C is at most 3"
class CrossSuitMaxSuitLengthInvariant : public CrossSuitConditionalInvariant {
 public:
  CrossSuitMaxSuitLengthInvariant(CardType condition_card,
                                  Suit target_suit,
                                  int max_len)
      : CrossSuitConditionalInvariant(condition_card),
        target_suit_(target_suit),
        max_len_(max_len) {}

  std::string ToString() const override;

  bool HoldsOnDeal(
      const PairedDeal& deal,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2") const override;

  Suit target_suit() const { return target_suit_; }
  int max_len() const { return max_len_; }

 private:
  Suit target_suit_ = Suit::SPADES;
  int max_len_ = 0;
};

enum class HonorPossessionTargetHand {
  kHand1,
  kHand2,
  kBoth
};

// Describes honor card possession invariants across paired hands: a specific hand
// or both hands always contain a designated honor card (A, K, Q, J, 10).
// Example string formats:
//   "hand 1 always contains the AS"
//   "both hands always contain the AS"
class HonorPossessionInvariant : public UnknownCardsInvariant {
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

  static std::shared_ptr<HonorPossessionInvariant> FromString(
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

// Describes suit length upper bounds invariant across either hand in a paired deal.
// Example string format: "maximum length in S in any hand is 4"
class MaxSuitLengthInvariant : public UnknownCardsInvariant {
 public:
  MaxSuitLengthInvariant(Suit suit, int max_len)
      : suit_(suit), max_len_(max_len) {}

  std::string ToString() const override;

  bool HoldsOnDeal(
      const PairedDeal& deal,
      const std::string& hand1_label = "hand 1",
      const std::string& hand2_label = "hand 2") const override;

  static std::shared_ptr<MaxSuitLengthInvariant> FromString(
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

// Describes exact suit length invariant for a specific hand in a paired deal.
// Example string format: "hand 1 length in S is always 3"
class ExactSuitLengthInvariant : public UnknownCardsInvariant {
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

  static std::shared_ptr<ExactSuitLengthInvariant> FromString(
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

// Describes suit length range invariant (min..max) for a specific hand in a paired deal.
// Example string format: "hand 1 length in S is between 2 and 4"
class SuitLengthRangeInvariant : public UnknownCardsInvariant {
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

  static std::shared_ptr<SuitLengthRangeInvariant> FromString(
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
