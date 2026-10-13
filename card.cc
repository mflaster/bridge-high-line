#include "card.h"

#include <iostream>

#include "assert.h"
#include "glog/logging.h"
#include "string_util.h"


uint64_t Card::saved_suited_cards[64];
static Card trigger_card_construction;

Card::Card() {
  if (!saved_suited_cards[0]) {
    for (int s1 = 0; s1 < 4; s1++) {
      for (int val = 0; val < 13; val++) {
        uint64_t unsuited = GetUnsuitedCard(val);
        saved_suited_cards[s1 * 16 + val] = unsuited << (s1 * 16);
      }
    }
  }
}

std::string SuitDebugString(Suit s) {
  switch(s) {
    case SPADES: return "S";
    case HEARTS: return "H";
    case DIAMONDS: return "D";
    case CLUBS: return "C";
    case NOTRUMP: return "NT";
    default: return "UNKNOWN";
  }
}

/* static */
Suit Card::ParseSuit(char c) {
  switch (c) {
    case 'S': case 's': return SPADES;
    case 'H': case 'h': return HEARTS;
    case 'D': case 'd': return DIAMONDS;
    case 'C': case 'c': return CLUBS;
    case 'N': case 'n': return NOTRUMP;
    default: LOG(FATAL) << "Unknown suit char: " << c;
  }
  return SPADES;
}

/* static */
Suit Card::ParseSuit(const std::string &s) {
  std::string lower = ToLower(Trim(s));
  if (lower.empty()) {
    LOG(FATAL) << "Empty suit string";
    return NOTRUMP;
  }

  if (lower == "spades" || lower == "s") return SPADES;
  if (lower == "hearts" || lower == "h") return HEARTS;
  if (lower == "diamonds" || lower == "d") return DIAMONDS;
  if (lower == "clubs" || lower == "c") return CLUBS;
  if (lower == "notrump" || lower == "no trump" || lower == "no_trump" || lower == "nt" || lower == "n") return NOTRUMP;

  LOG(FATAL) << "Unknown suit string: " << s;
  return NOTRUMP;
}

/* static */
uint64_t Card::ParseStringCard(std::string c) {
  assert(c.size() == 2);
  Suit s = ParseSuit(c[0]);
  int card_val = -1;
  switch(c[1]) {
    case 'A': card_val = 12; break;
    case 'K': card_val = 11; break;
    case 'Q': card_val = 10; break;
    case 'J': card_val = 9; break;
    case 'T': card_val = 8; break;
    default:
      card_val = c[1] - '2';
      break;
  }
  assert(card_val >= 0);
  assert(card_val < 13);
  return GetSuitedCard(s, card_val);
}

/* static */
uint16_t Card::GetUnsuitedCard(int x) {
  assert(x < 13);
  return (1 << x);
}

/* static */
std::pair<Suit, int> Card::DecodeCard(CardType c) {
  Suit suit = (Suit)0;
  CardType orig = c;
  while (c > 0) {
    uint16_t cur_suit = c & 0xffff;
    if (cur_suit > 0) {
      int val = 0;
      while (cur_suit > 0) {
        if (cur_suit & 0x1) {
          return std::make_pair(suit, val);
        } else {
          val++;
          cur_suit = (cur_suit >> 1);
        }
      }
    }
    c = (c >> 16);
    suit = (Suit)((int)suit + 1);
  }
  std::cout << "DecodeCard: Can't understand " << orig << std::endl;
  assert(false);
  return std::make_pair(NOTRUMP, 0);
}

/* static */
std::string Card::PrintCardValue(int c) {
  assert(c < 13);
  if (c < 8) {
    return std::to_string(c + 2);
  } else {
    switch(c) {
      case 8: return "T";
      case 9: return "J";
      case 10: return "Q";
      case 11: return "K";
      case 12: return "A";
      default: assert(false);
    }
  }
  return "failure";
}

/* static */
std::string Card::PrintDDSCard(Suit s, int dds_val) {
  CHECK_GE(dds_val, 2);
  return SuitDebugString(s) + PrintCardValue(dds_val - 2);
}


/* static */
std::string Card::PrintCard(CardType c) {
  auto [suit, val] = DecodeCard(c);
  return SuitDebugString(suit) + PrintCardValue(val);
}


std::string PrintCards(const CardType *cards, int num, int leader) {
  std::string ans;
  if (leader > 0) {
    for (int i = 0; i < leader * 2 + 1; i++) {
      ans += " ";
    }
  }
  for (int i = 0; i < num; i++) {
    ans += Card::PrintCard(cards[i]) + " ";
  }
  return ans;
}
