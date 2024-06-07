#pragma once

#include <cassert>
#include <cstdint>
#include <string>
#include <utility>
#include "glog/logging.h"

enum Suit {
  SPADES = 0,
  HEARTS = 1,
  DIAMONDS = 2,
  CLUBS = 3,
  NOTRUMP = 4
};

enum PlayerDirection {
  NORTH = 0,
  EAST = 1,
  SOUTH = 2,
  WEST = 3
};

std::string SuitDebugString(Suit s);

using CardType = uint64_t;

class Card {
 public:
  Card();
  static uint16_t GetUnsuitedCard(int x);
  static uint64_t GetSuitedCard(Suit suit, int x) {
    return saved_suited_cards[suit * 16 + x];
  }
  static uint64_t ParseStringCard(std::string c);
  static Suit ParseSuit(char c);

  static std::string PrintDDSCard(Suit s, int dds_val);
  static std::string PrintCard(CardType c);
  static std::pair<Suit, int> DecodeCard(CardType c);
  static std::string PrintCardValue(int c);

  static uint64_t saved_suited_cards[64];
};

std::string PrintCards(const CardType *cards, int num, int leader);
