#include "hand.h"

#include <iostream>
#include <set>
#include <utility>
#include <stdlib.h>

std::string HandDebugString(uint64_t hand, bool compact) {
  std::string ans;
  while (hand > 0) {
    for (int suit = 0; suit < 4; suit++) {
      uint16_t cur_suit = hand & 0xffff;
      hand = (hand >> 16);
      std::string cur_suit_str;
      for (int val = 0; val < 13; val++) {
        if (cur_suit & 0x1) {
          cur_suit_str = Card::PrintCardValue(val) + cur_suit_str;
        }
        cur_suit = (cur_suit >> 1);
      }
      if (cur_suit_str.empty()) {
        if (!compact) {
          ans += SuitDebugString((Suit)suit) + " -; ";
        }
      } else {
        ans += SuitDebugString((Suit)suit) + " ";
        ans += cur_suit_str + "; ";
      }
    }
  }
  return ans;
}

std::vector<CardType> CardsInHand(uint64_t hand, uint64_t played_cards) {
  std::vector<CardType> ans;
  while (hand > 0) {
    for (int suit = 0; suit < 4; suit++) {
      uint16_t cur_suit = hand & 0xffff;
      hand = (hand >> 16);
      for (int val = 0; val < 13; val++) {
        if (cur_suit & 0x1) {
          uint64_t c = Card::GetSuitedCard((Suit)suit, val);
          if ((played_cards & c) == 0) {
            ans.push_back(c);
          }
        }
        cur_suit = (cur_suit >> 1);
      }
    }
  }
  return ans;
}

int GetHighCardPoints(uint64_t hand) {
  int total_hcp = 0;
  while (hand > 0) {
    for (int suit = 0; suit < 4; suit++) {
      uint16_t cur_suit = hand & 0xffff;
      hand = (hand >> 16);
      for (int val = 0; val < 13; val++) {
        if (val > 8 && cur_suit & 0x1) {
          total_hcp += val - 8;
        }
        cur_suit = (cur_suit >> 1);
      }
    }
  }
  return total_hcp;
}

void GetSuitDistribution(uint64_t hand, int *dist) {
  CHECK_GT(hand, 0);
  while (hand > 0) {
    for (int suit = 0; suit < 4; suit++) {
      dist[suit] = 0;
      uint16_t cur_suit = hand & 0xffff;
      hand = (hand >> 16);
      int cards_in_suit = 0;
      for (int val = 0; val < 13; val++) {
        if (cur_suit & 0x1) {
          cards_in_suit++;
        }
        cur_suit = (cur_suit >> 1);
      }
      dist[suit] = cards_in_suit;
    }
  }
}

int CountCardsInHand(uint64_t hand) {
  if (hand == 0) {
    return 0;
  }
  int dist[4];
  GetSuitDistribution(hand, dist);
  int sum = 0;
  for (int i = 0; i < 4; i++) {
    sum += dist[i];
  }
  return sum;
}

std::vector<CardType> CardsInHandInSuit(
    uint64_t hand, Suit s, uint64_t played_cards) {
  std::vector<CardType> ans;
  // uint16_t cur_suit = (hand & (0xffff << (int)s));
  // cur_suit = (cur_suit >> (s * 16));
  uint16_t cur_suit = ((hand >> (s * 16)) & 0xffff);
  // hand = (hand >> 16);
  for (int val = 0; val < 13; val++) {
    if (cur_suit & 0x1) {
      uint64_t c = Card::GetSuitedCard(s, val);
      if ((played_cards & c) == 0) {
        ans.push_back(c);
      }
    }
    cur_suit = (cur_suit >> 1);
  }
  return ans;
}

uint64_t ParseHand(std::string hand_str) {
  uint64_t ans = 0;
  Suit cur_suit = NOTRUMP;
  for (unsigned int i = 0; i < hand_str.size(); i++) {
    int card_val = -1;
    switch(hand_str[i]) {
      case ';':
      case ' ':
      case '-': break;
      case 'S': cur_suit = SPADES; break;
      case 'H': cur_suit = HEARTS; break;
      case 'D': cur_suit = DIAMONDS; break;
      case 'C': cur_suit = CLUBS; break;
      case 'A': card_val = 12; break;
      case 'K': card_val = 11; break;
      case 'Q': card_val = 10; break;
      case 'J': card_val = 9; break;
      case 'T': card_val = 8; break;
      default:
        card_val = hand_str[i] - '2';
        break;
    }
    if (card_val == -1) {
      continue;
    }
    assert(card_val >= 0);
    assert(card_val < 13);
    uint64_t cur_card = Card::GetSuitedCard(cur_suit, card_val);
    ans = (ans | cur_card);
  }
  return ans;
}

uint64_t MakeHandFromCards(const std::vector<CardType> &cards) {
  uint64_t ans = 0;
  for (const CardType &c : cards) {
    ans = (ans | c);
  }
  return ans;
}

std::vector<std::string> SplitWords(const std::string &input) {
  VLOG(2) << "SplitWords on " << input;
  std::vector<std::string> words;
  std::stringstream ss(input);
  std::string word;
  while (ss >> word) {
    words.push_back(word);
    VLOG(2) << "See word " << word;
  }
  return words;
}
