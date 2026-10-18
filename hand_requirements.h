#pragma once

#include <memory>
#include <string>
#include <vector>
#include "hand.h"

class HandRequirements {
 public:
  HandRequirements() {}
  virtual ~HandRequirements() {}

  virtual bool MeetsRequirement(uint64_t hand) const { return false; }
  virtual bool AppliesToRemainingHand() const { return true; }
  virtual std::string ToString() const { return ""; }

  static std::shared_ptr<HandRequirements> FromString(const std::string &str);
};

class SuitAtLeast : public HandRequirements {
 public:
  SuitAtLeast(Suit s, int count) : suit_(s), count_(count) {}
  ~SuitAtLeast() {}

  bool MeetsRequirement(uint64_t hand) const override;
  std::string ToString() const override;

 private:
  Suit suit_;
  int count_;
};

class SuitLength : public HandRequirements {
 public:
  SuitLength(Suit s, int low, int high) : suit_(s), low_(low), high_(high) {}
  ~SuitLength() {}

  bool MeetsRequirement(uint64_t hand) const override;
  std::string ToString() const override;

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
  bool AppliesToRemainingHand() const override { return false; }
  std::string ToString() const override;
};

class AntiRequirement : public HandRequirements {
 public:
  AntiRequirement(std::shared_ptr<HandRequirements> other)
      : other_(other) {}
  ~AntiRequirement() {}

  bool MeetsRequirement(uint64_t hand) const override;
  bool AppliesToRemainingHand() const override {
    return other_ ? other_->AppliesToRemainingHand() : true;
  }
  std::string ToString() const override;

 private:
  std::shared_ptr<HandRequirements> other_;
};

class HighCardRange : public HandRequirements {
 public:
  HighCardRange(int low, int high) : low_(low), high_(high) {}
  ~HighCardRange() {}

  bool MeetsRequirement(uint64_t hand) const override;
  bool AppliesToRemainingHand() const override { return false; }
  std::string ToString() const override;

 private:
  int low_;
  int high_;
};

class HandHasCard : public HandRequirements {
 public:
  HandHasCard(CardType c) : card_(c) {}
  ~HandHasCard() {}

  bool MeetsRequirement(uint64_t hand) const override;
  std::string ToString() const override;

 private:
  CardType card_;
};

class AtLeastAces : public HandRequirements {
 public:
  AtLeastAces(int n) : n_(n) {}
  ~AtLeastAces() {}

  bool MeetsRequirement(uint64_t hand) const override;
  bool AppliesToRemainingHand() const override { return false; }
  std::string ToString() const override;

 private:
  int n_;
};

class AnyOfRequirement : public HandRequirements {
 public:
  AnyOfRequirement(std::vector<std::shared_ptr<HandRequirements>> reqs)
      : reqs_(std::move(reqs)) {}
  ~AnyOfRequirement() override = default;

  bool MeetsRequirement(uint64_t hand) const override;
  std::string ToString() const override;

 private:
  std::vector<std::shared_ptr<HandRequirements>> reqs_;
};

namespace sayc {
struct SaycDagNode;
}  // namespace sayc

// Evaluates whether a 13-card hand would make `bid` after the space-separated
// first-round call sequence `after` by evaluating the compiled constraint DAG
// generated at build time from SAYCBridge
// (https://github.com/eseidel/saycbridge, Copyright (c) 2013 The SAYCBridge
// Authors, BSD-3-Clause).
class WouldBid : public HandRequirements {
 public:
  WouldBid(const std::string &bid, const std::string &after = "");
  ~WouldBid() override;

  bool MeetsRequirement(uint64_t hand) const override;
  bool AppliesToRemainingHand() const override { return false; }
  std::string ToString() const override;

  const std::string &bid() const { return bid_; }
  const std::string &after() const { return after_; }

  static std::string NormalizeCall(const std::string &raw_call);

 private:
  std::string bid_;
  std::string after_;
  std::vector<sayc::SaycDagNode> prog_;
};

