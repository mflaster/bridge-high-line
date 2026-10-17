#pragma once

#include "card.h"

#include <string>
#include <type_traits>
#include <vector>


std::string HandDebugString(uint64_t hand, bool compact = false);
std::vector<CardType> CardsInHand(uint64_t hand, uint64_t played_cards);
std::vector<CardType> CardsInHandInSuit(uint64_t hand, Suit s,
                                        uint64_t played_cards);

uint64_t ParseHand(std::string hand_str);
uint64_t MakeHandFromCards(const std::vector<CardType> &cards);

std::vector<std::string> SplitWords(const std::string &input);

void GetSuitDistribution(uint64_t hand, int *dist);
int CountCardsInHand(uint64_t hand);
int GetHighCardPoints(uint64_t hand);

struct HonorPossessionMatrix {
  uint64_t hand1_honors = 0;
  uint64_t hand2_honors = 0;
  uint64_t shared_honors = 0;
  uint64_t neither_honors = 0;
  uint64_t joint_honors = 0;

  int hand1_honor_count = 0;
  int hand2_honor_count = 0;
  int joint_honor_count = 0;

  bool Hand1HasHonor(Suit suit, int rank) const {
    return (hand1_honors & Card::GetSuitedCard(suit, rank)) != 0;
  }

  bool Hand2HasHonor(Suit suit, int rank) const {
    return (hand2_honors & Card::GetSuitedCard(suit, rank)) != 0;
  }

  bool JointHasHonor(Suit suit, int rank) const {
    return (joint_honors & Card::GetSuitedCard(suit, rank)) != 0;
  }

  uint16_t Hand1SuitHonors(Suit suit) const {
    return static_cast<uint16_t>((hand1_honors >> (static_cast<int>(suit) * 16)) & 0xFFFF);
  }

  uint16_t Hand2SuitHonors(Suit suit) const {
    return static_cast<uint16_t>((hand2_honors >> (static_cast<int>(suit) * 16)) & 0xFFFF);
  }
};

HonorPossessionMatrix ClassifyHonorPossession(uint64_t hand1, uint64_t hand2);

template <typename T>
std::string StrJoin(const T &v, const std::string &delim = ", ") {
  std::string ans;
  for (const auto &x : v) {
    if (!ans.empty()) {
      ans += delim;
    }
    if constexpr (std::is_convertible_v<decltype(x), std::string>) {
      ans += x;
    } else {
      ans += std::to_string(x);
    }
  }
  return ans;
}
