#pragma once

#include <cassert>
#include <set>
#include <utility>
#include <vector>
#include "card.h"
#include "hand.h"

class BoardState {
 public:
  BoardState() {
    for (int i = 0; i < 4; i++) {
      hands_[i] = 0;
      trick_cards_played_[i] = 0;
    }
  }
  void PlayCard(int player, CardType c);
  int CurPlayer() const;
  int cur_trick_leader() const { return cur_trick_leader_; }

  std::pair<int, std::pair<Suit, int>> TrickWinner() const;

  bool Finished() const { return (hands_[0] & (~played_cards_)) == 0; }

  std::string DebugString() const;

  void SetLead(int winner);
  void SetHands(const std::vector<uint64_t> &hands);
  void SetTrump(Suit s) { trump_suit_ = s; }
  Suit GetTrump() const { return trump_suit_; }
  int tricks_won_odd() const { return tricks_won_odd_; }
  int tricks_won_even() const { return tricks_won_even_; }

  std::vector<CardType> ValidCardsToPlay() const;
  uint64_t played_cards() const { return played_cards_; }
  uint64_t GetHand(int p) const { return hands_[p]; }

  std::pair<uint64_t, uint64_t> RemainingHandsKey() const;

  void MarkMissingCardsAsPlayed();

  // Returns NOTRUMP if nothing has been played to current trick
  Suit SuitLed() const;

  int CardsPlayedThisTrick() const { return num_trick_cards_played_; }
  const CardType* trick_cards_played() const { return trick_cards_played_; }

  bool CardPlayedThisTrick(CardType c) const {
    for (int i = 0; i < num_trick_cards_played_; i++) {
      if (trick_cards_played_[i] == c) {
        return true;
      }
    }
    return false;
  }

  const CardType *PreviousTrick() const {
    assert(num_trick_cards_played_ == 0);
    return trick_cards_played_;
  }
  int prev_trick_leader() const { return prev_trick_leader_; }

  void set_check_card_play(bool x) { check_card_play_ = x; }

  uint32_t board_id() const { return board_id_; }
  void set_board_id(uint32_t id) { board_id_ = id; }

  bool HasShownOut(int player, Suit s) const {
    return (shown_out_mask_ & (1u << (player * 4 + s))) != 0;
  }
  uint16_t shown_out_mask() const { return shown_out_mask_; }
  void set_shown_out_mask(uint16_t mask) { shown_out_mask_ = mask; }

  void SetPlayerHand(int player, uint64_t hand) {
    assert(player >= 0 && player < 4);
    hands_[player] = hand;
  }

  std::string GetPBNCards() const;
  std::string ConvertHandToPBN(uint64_t hand) const;

  void PlayTrick(const std::string &trick);

  int FindBestDiscard(int n, int *suit, int *rank) const;

 private:
  void RecordPlayedCards();

  uint64_t played_cards_ = 0;
  uint64_t hands_[4];

  CardType trick_cards_played_[4];
  uint8_t num_trick_cards_played_ = 0;
  bool check_card_play_ = true;
  Suit trump_suit_ = NOTRUMP;

  uint8_t cur_trick_leader_ = 0;
  uint8_t prev_trick_leader_ = 0;
  uint8_t tricks_won_odd_ = 0;
  uint8_t tricks_won_even_ = 0;
  uint16_t shown_out_mask_ = 0;
  uint32_t board_id_ = 0;
};
