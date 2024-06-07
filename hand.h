#pragma once

#include "card.h"

#include <string>
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

template <typename T>
std::string StrJoin(const T &v, std::string delim) {
  std::string ans;
  for (auto x : v) {
    if (!ans.empty()) {
      ans += delim;
    };
    ans += x;
  }
  return ans;
}
