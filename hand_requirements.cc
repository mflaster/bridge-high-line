#include "hand_requirements.h"

bool SuitAtLeast::MeetsRequirement(uint64_t hand) const {
  hand = hand >> (suit_ * 16);
  hand = hand & 0xffff;
  int num_cards = 0;
  for (int i = 0; i < 13; i++) {
    if (hand & 0x1) {
      if (++num_cards >= count_) {
        return true;
      }
    }
    hand = hand >> 1;
  }
  return false;
}

bool HandHasCard::MeetsRequirement(uint64_t hand) const {
  return hand & card_;
}

bool SuitLength::MeetsRequirement(uint64_t hand) const {
  int dist[4];
  GetSuitDistribution(hand, dist);
  return dist[suit_] >= low_ && dist[suit_] <= high_;
}

bool HighCardRange::MeetsRequirement(uint64_t hand) const {
  int hcp = GetHighCardPoints(hand);
  return hcp >= low_ && hcp <= high_;
}

bool BalancedHand::MeetsRequirement(uint64_t hand) const {
  int dist[4];
  GetSuitDistribution(hand, dist);
  for (int i = 0; i < 4; i++) {
    if (dist[i] < 2 || dist[i] > 5) {
      return false;
    }
  }
  return true;
}

bool AntiRequirement::MeetsRequirement(uint64_t hand) const {
  return !other_->MeetsRequirement(hand);
}

bool AtLeastAces::MeetsRequirement(uint64_t hand) const {
  int aces = 0;
  for (int s = 0; s < 4; s++) {
    uint64_t c = Card::GetSuitedCard((Suit)s, 12);
    if (hand & c) {
      aces++;
    }
  }
  return aces >= n_;
}
