#include "deal.h"

#include "card.h"
#include "hand.h"
#include <set>
#include <random>



std::vector<uint64_t> DealHands() {
  static std::mt19937 mt_rand(0);
  std::multiset<std::pair<double, CardType>> deck;
  for (int suit = 0; suit < 4; suit++) {
    for (int val = 0; val < 13; val++) {
      deck.insert(std::make_pair(mt_rand(), // drand48(),
                                 Card::GetSuitedCard((Suit)suit, val)));
    }
  }
  uint64_t cur_hand = 0;
  int cur_cards = 0;
  std::vector<uint64_t> hands;
  for (const auto &[_r, card] : deck) {
    // std::cout << "Dealing " << Card::PrintCard(card) << std::endl;
    assert((cur_hand & card) == 0);
    cur_hand = (cur_hand | card);
    cur_cards++;
    if (cur_cards >= 13) {
      // std::cout << "Hand complete" << std::endl;
      hands.push_back(cur_hand);
      cur_hand = 0;
      cur_cards = 0;
    }
  }
  return hands;
}

std::pair<uint64_t, uint64_t> DealTwoHands(uint64_t cards) {
  static std::mt19937 mt_rand(0);
  std::multiset<std::pair<double, CardType>> deck;
  for (int suit = 0; suit < 4; suit++) {
    for (int val = 0; val < 13; val++) {
      CardType c = Card::GetSuitedCard((Suit)suit, val);
      if (c & cards) {
        deck.insert(std::make_pair(mt_rand(),  // drand48(),
                                   c));
      }
    }
  }
  int cards_in_deck = deck.size();
  CHECK_GT(cards_in_deck, 1);
  uint64_t hand1 = 0;
  uint64_t hand2 = 0;
  uint64_t *cur_hand = &hand1;
  int cur_cards = 0;
  for (const auto &[_r, card] : deck) {
    assert(((*cur_hand) & card) == 0);
    *cur_hand = ((*cur_hand) | card);
    cur_cards++;
    if (cur_cards >= cards_in_deck / 2) {
      cur_hand = &hand2;
      cur_cards = 0;
    }
  }
  return std::make_pair(hand1, hand2);
}

std::pair<uint64_t, uint64_t> DealTwoRequiredHands(
    uint64_t cards,
    const std::vector<std::shared_ptr<HandRequirements>> &reqs1,
    const std::vector<std::shared_ptr<HandRequirements>> &reqs2) {
  for (int i = 0; i < 1000 * 1000; i++) {
    auto [hand1, hand2] = DealTwoHands(cards);
    bool meets_reqs = true;
    for (const auto &req : reqs1) {
      if (!req->MeetsRequirement(hand1)) {
        meets_reqs = false;
        break;
      }
    }

    for (const auto &req : reqs2) {
      if (!req->MeetsRequirement(hand2)) {
        meets_reqs = false;
        break;
      }
    }
    if (meets_reqs) {
      return std::make_pair(hand1, hand2);
    }
  }
  LOG(FATAL) << "Couldn't meet requirements";
}


uint64_t DealOneHand(uint64_t cards) {
  static std::mt19937 mt_rand(0);
  std::multiset<std::pair<double, CardType>> deck;
  for (int suit = 0; suit < 4; suit++) {
    for (int val = 0; val < 13; val++) {
      CardType c = Card::GetSuitedCard((Suit)suit, val);
      if (c & cards) {
        deck.insert(std::make_pair(mt_rand(),  // drand48(),
                                   c));
      }
    }
  }
  int cards_in_deck = deck.size();
  CHECK_GT(cards_in_deck, 1);
  uint64_t hand1 = 0;
  uint64_t *cur_hand = &hand1;
  int cur_cards = 0;
  for (const auto &[_r, card] : deck) {
    assert(((*cur_hand) & card) == 0);
    *cur_hand = ((*cur_hand) | card);
    cur_cards++;
    if (cur_cards >= 13) {
      break;
    }
  }
  CHECK_EQ(cur_cards, 13);
  return hand1;
}

uint64_t DealOneRequiredHand(
    uint64_t cards,
    const std::vector<std::shared_ptr<HandRequirements>> &reqs) {
  for (int i = 0; i < 1000 * 1000; i++) {
    uint64_t hand = DealOneHand(cards);
    bool meets_reqs = true;
    for (const auto &req : reqs) {
      if (!req->MeetsRequirement(hand)) {
        meets_reqs = false;
        break;
      }
    }
    if (meets_reqs) {
      return hand;
    }
  }
  LOG(FATAL) << "Couldn't meet requirements";
}


bool MeetsAllRequirements(
    const std::vector<uint64_t> &hands,
    const std::vector<std::vector<std::shared_ptr<HandRequirements>>> &reqs) {
  CHECK_EQ(hands.size(), 4);
  CHECK_LE(reqs.size(), 4);
  for (uint i = 0; i < reqs.size(); i++) {
    for (const auto &hand_req : reqs[i]) {
      if (!hand_req->MeetsRequirement(hands[i])) {
        return false;
      }
    }
  }
  return true;
}


std::vector<uint64_t> DealMissingCards(
    const std::vector<uint64_t> &hands,
    const std::vector<std::vector<std::shared_ptr<HandRequirements>>> &reqs) {
  CHECK_EQ(hands.size(), 4);
  static std::mt19937 mt_rand(1);
  uint64_t present_cards = 0;
  std::vector<int> cards_in_hand;
  int max_cards = 0;
  for (uint64_t hand : hands) {
    present_cards = present_cards | (hand);
    int n = CountCardsInHand(hand);
    cards_in_hand.push_back(n);
    max_cards = std::max(max_cards, n);
  }
  if (max_cards == 0) {
    // Assume we have the whole deck to deal.
    max_cards = 13;
  }
  uint64_t missing_cards = ~present_cards;

  int first_hand_missing_cards = -1;
  for (int i = 0; i < 4; i++) {
    if (cards_in_hand[i] < max_cards) {
      first_hand_missing_cards = i;
      break;
    }
  }
  CHECK_GE(first_hand_missing_cards, 0);
  VLOG(2) << "max_cards " << max_cards;

  for (int i = 0; i < 1000 * 1000; i++) {
    std::multiset<std::pair<double, CardType>> deck;
    for (int suit = 0; suit < 4; suit++) {
      for (int val = 0; val < 13; val++) {
        CardType c = Card::GetSuitedCard((Suit)suit, val);
        if (c & missing_cards) {
          deck.insert(std::make_pair(mt_rand(), // drand48(),
                                     c));
        }
      }
    }
    VLOG(2) << "Deck has cards " << deck.size()
            << " first_hand_missing_cards " << first_hand_missing_cards;
    std::vector<uint64_t> final_hands = hands;
    std::vector<int> cards_in_hand_copy(cards_in_hand);
    int cur_hand_missing_cards = first_hand_missing_cards;
    for (const auto &[_r, card] : deck) {
      CHECK_LT(cur_hand_missing_cards, 4);
      auto &cur_hand = final_hands[cur_hand_missing_cards];
      CHECK_EQ(cur_hand & card, 0);
      cur_hand = cur_hand | card;
      if (++cards_in_hand_copy[cur_hand_missing_cards] == max_cards) {
        while (++cur_hand_missing_cards < 4) {
          if (cards_in_hand_copy[cur_hand_missing_cards] < max_cards) {
            break;
          }
        }
      }
    }
    CHECK_EQ(cur_hand_missing_cards, 4);

    if (!MeetsAllRequirements(final_hands, reqs)) {
      continue;
    }
    for (int i = 0; i < 4; i++) {
      CHECK_EQ(CountCardsInHand(final_hands[i]), max_cards)
          << HandDebugString(final_hands[i]);
      VLOG(1) << "Dealt hand: " << HandDebugString(final_hands[i]);
    }
    VLOG(1) << "Met requirements on deal " << i;
    return final_hands;
  }
  LOG(FATAL) << "Couldn't meet requirements";
}
