#pragma once

#include <cassert>
#include <cstdint>
#include <string>
#include <utility>

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
  static Suit ParseSuit(const std::string &s);

  static std::string PrintDDSCard(Suit s, int dds_val);
  static std::string PrintCard(CardType c);
  static std::pair<Suit, int> DecodeCard(CardType c);
  static std::string PrintCardValue(int c);

  static uint64_t saved_suited_cards[64];
};

std::string PrintCards(const CardType *cards, int num, int leader);

inline int PopCount64(uint64_t v) {
#if defined(_MSC_VER) && !defined(__clang__)
  return static_cast<int>(__popcnt64(v));
#else
  return __builtin_popcountll(v);
#endif
}

// 64-bit rank and honor bitmask constants across all 4 suits
constexpr uint64_t kTenMask64   = 0x0100010001000100ULL;
constexpr uint64_t kJackMask64  = 0x0200020002000200ULL;
constexpr uint64_t kQueenMask64 = 0x0400040004000400ULL;
constexpr uint64_t kKingMask64  = 0x0800080008000800ULL;
constexpr uint64_t kAceMask64   = 0x1000100010001000ULL;
constexpr uint64_t kHonorMask64 = 0x1F001F001F001F00ULL;
constexpr uint64_t kSpotMask64  = 0x00FF00FF00FF00FFULL;

inline uint64_t ExtractHonors(uint64_t hand) {
  return hand & kHonorMask64;
}

inline int CountHonorsInHand(uint64_t hand) {
  return PopCount64(ExtractHonors(hand));
}

inline int CardsInSuit(uint64_t hand, Suit s) {
  if (static_cast<int>(s) < 0 || static_cast<int>(s) >= 4) {
    return 0;
  }
  return PopCount64((hand >> (static_cast<int>(s) * 16)) & 0xffffULL);
}
