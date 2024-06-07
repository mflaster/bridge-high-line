#pragma once

#include <memory>
#include "hand.h"

class HandRequirements {
 public:
  HandRequirements() {}
  virtual ~HandRequirements() {}

  virtual bool MeetsRequirement(uint64_t hand) const { return false; }
};

class SuitAtLeast : public HandRequirements {
 public:
  SuitAtLeast(Suit s, int count) : suit_(s), count_(count) {}
  ~SuitAtLeast() {}

  bool MeetsRequirement(uint64_t hand) const override;

 private:
  Suit suit_;
  int count_;
};

class SuitLength : public HandRequirements {
 public:
  SuitLength(Suit s, int low, int high) : suit_(s), low_(low), high_(high) {}
  ~SuitLength() {}

  bool MeetsRequirement(uint64_t hand) const override;

 private:
  Suit suit_;
  int low_;
  int high_;
};

class BalancedHand : public HandRequirements {
 public:
  BalancedHand() = default;
  ~BalancedHand() {}

  bool MeetsRequirement(uint64_t hand) const override;
};

class AntiRequirement : public HandRequirements {
 public:
  AntiRequirement(std::shared_ptr<HandRequirements> other)
      : other_(other) {}
  ~AntiRequirement() {}

  bool MeetsRequirement(uint64_t hand) const override;

 private:
  std::shared_ptr<HandRequirements> other_;
};

class HighCardRange : public HandRequirements {
 public:
  HighCardRange(int low, int high) : low_(low), high_(high) {}
  ~HighCardRange() {}

  bool MeetsRequirement(uint64_t hand) const override;

 private:
  int low_;
  int high_;
};

class HandHasCard : public HandRequirements {
 public:
  HandHasCard(CardType c) : card_(c) {}
  ~HandHasCard() {}

  bool MeetsRequirement(uint64_t hand) const override;

 private:
  CardType card_;
};

class AtLeastAces : public HandRequirements {
 public:
  AtLeastAces(int n) : n_(n) {}
  ~AtLeastAces() {}

  bool MeetsRequirement(uint64_t hand) const override;

 private:
  int n_;
};
