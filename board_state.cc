#include "board_state.h"

#include <cassert>
#include <iostream>
#include "glog/logging.h"
#include "hand.h"

int BoardState::CurPlayer() const {
  return (cur_trick_leader_ + num_trick_cards_played_) % 4;
}

void BoardState::PlayCard(int player, CardType c) {
  VLOG(2) << "PlayCard " << Card::PrintCard(c);

  CHECK_EQ(player, CurPlayer());
  CHECK_LT(num_trick_cards_played_, 4);
  if (check_card_play_) {
    CHECK_GT((hands_[player] & c), 0) << HandDebugString(hands_[player])
                                      << " doesn't have " << Card::PrintCard(c);
    CHECK_EQ(played_cards_ & c, 0);
  }
  trick_cards_played_[num_trick_cards_played_++] = c;

  if (num_trick_cards_played_ == 4) {
    RecordPlayedCards();
    auto [winner, _card_pair] = TrickWinner();
    winner = (winner + cur_trick_leader_) % 4;
    if (winner % 2 == 0) {
      tricks_won_even_++;
    } else {
      tricks_won_odd_++;
    }
    SetLead(winner);
  }
}

void BoardState::RecordPlayedCards() {
  CHECK_EQ(num_trick_cards_played_, 4);
  for (int i = 0; i < 4; i++) {
    played_cards_ = (played_cards_ | trick_cards_played_[i]);
  }
}

void BoardState::SetLead(int winner) {
  prev_trick_leader_ = cur_trick_leader_;
  cur_trick_leader_ = winner;
  num_trick_cards_played_ = 0;
}

Suit BoardState::SuitLed() const {
  if (num_trick_cards_played_ == 0) {
    return NOTRUMP;
  } else {
    auto [suit, _val] = Card::DecodeCard(trick_cards_played_[0]);
    return suit;
  }
}

std::string BoardState::DebugString() const {
  std::string ans;
  for (int i = 0; i < 4; i++) {
    if (!ans.empty()) {
      ans += " // ";
    }
    ans += HandDebugString(hands_[i] & (~played_cards_), /*compact=*/true);
  }
  return ans;
}

std::pair<int, std::pair<Suit, int>> BoardState::TrickWinner() const {
  CHECK_GT(num_trick_cards_played_, 0);
  auto [suit_led, winning_val] = Card::DecodeCard(trick_cards_played_[0]);
  int winning_player = 0;
  for (int i = 1; i < num_trick_cards_played_; i++) {
    auto [suit_played, val_played] = Card::DecodeCard(trick_cards_played_[i]);
    if (suit_played == suit_led) {
      if (val_played > winning_val) {
        winning_val = val_played;
        winning_player = i;
      }
    } else if (suit_played == trump_suit_) {
      suit_led = suit_played;
      winning_player = i;
      winning_val = val_played;
    }
  }
  VLOG(2) << "Trick " << (tricks_won_odd_ + tricks_won_even_ + 1)
          << " won by " << ((cur_trick_leader_ + winning_player) % 4)
          << " with "
          << Card::PrintCard(Card::GetSuitedCard(suit_led, winning_val))
          << " for "
          << ((cur_trick_leader_ + winning_player) % 2 == 0
              ? tricks_won_even_ + 1 : tricks_won_odd_ + 1)
          << " after "
          << PrintCards(trick_cards_played_, num_trick_cards_played_,
                        cur_trick_leader_);

  return std::make_pair(winning_player,
                        std::make_pair(suit_led, winning_val));
}

std::vector<CardType> BoardState::ValidCardsToPlay() const {
  int cur_player = CurPlayer();
  VLOG(2) << "ValidCardsToPlay for " << cur_player;
  if (cur_player == cur_trick_leader_) {
    CHECK_EQ(num_trick_cards_played_, 0);
    return CardsInHand(hands_[cur_player], played_cards_);
  }
  CHECK_GT(num_trick_cards_played_, 0);
  Suit suit_led = SuitLed();
  std::vector<CardType> in_suit =
      CardsInHandInSuit(hands_[cur_player], suit_led, played_cards_);
  VLOG(2) << "in_suit has " << in_suit.size() << " for plyr " << cur_player
          << " suit " << suit_led
          << " with " << HandDebugString(hands_[cur_player]);
  if (!in_suit.empty()) {
    return in_suit;
  } else {
    return CardsInHand(hands_[cur_player], played_cards_);
  }
}

void BoardState::SetHands(const std::vector<uint64_t> &hands) {
  CHECK_EQ(hands.size(), 4);
  int num_cards = 0;
  for (unsigned int i = 0; i < hands.size(); i++) {
    if (i == 0) {
      num_cards = CountCardsInHand(hands[i]) << i;
    } else {
      CHECK_EQ(num_cards, CountCardsInHand(hands[i]))
          << i << ": " << HandDebugString(hands[i]);
    }
    hands_[i] = hands[i];
  }
  MarkMissingCardsAsPlayed();
}

void BoardState::MarkMissingCardsAsPlayed() {
  uint64_t unplayed_cards = 0;
  for (int i = 0; i < 4; i++) {
    unplayed_cards = (unplayed_cards | hands_[i]);
  }
  played_cards_ = ~unplayed_cards;
  VLOG(1) << "played_cards_ now " << HandDebugString(played_cards_);
}

std::pair<uint64_t, uint64_t> BoardState::RemainingHandsKey() const {
  CHECK_EQ(num_trick_cards_played_, 0);
  uint64_t suit_bytes[4];
  bzero(suit_bytes, sizeof(suit_bytes));

  for (int suit = 0; suit < 4; suit++) {
    // uint64_t *cur_bytes = (suit < 2 ? &ans.first : &ans.second);
    uint64_t *cur_bytes = &suit_bytes[suit];
    for (int val = 0; val < 13; val++) {
      uint64_t cur_card = Card::GetSuitedCard((Suit)suit, val);
      if (played_cards_ & cur_card) {
        continue;
      }
      int hand_with_card = -1;
      for (int hand = 0; hand < 4; hand++) {
        if (hands_[hand] & cur_card) {
          hand_with_card = hand;
          break;
        }
      }
      if (hand_with_card >= 0) {
        (*cur_bytes) = (*cur_bytes) | hand_with_card;
        (*cur_bytes) = (*cur_bytes) << 2;
        VLOG(5) << "Cur suit now: " << *cur_bytes
                << " after " << Card::PrintCard(cur_card);
      }
    }
  }
  std::pair<uint64_t, uint64_t> ans;
  ans.first = ((suit_bytes[0] << 32) | (suit_bytes[1]));
  ans.second = ((suit_bytes[2] << 32) | (suit_bytes[3]));
  VLOG(4) << "Got key " << ans.first << ", " << ans.second;
  return ans;
}

std::string StrJoin(const std::set<int> &board_offsets, const char *sep) {
  std::string ans;
  for (const auto &x : board_offsets) {
    if (!ans.empty()) {
      ans += sep;
    }
    ans += std::to_string(x);
  }
  return ans;
}

std::string BoardState::ConvertHandToPBN(uint64_t hand) const {
  std::string ans;
  for (int suit = 0; suit < 4; suit++) {
    std::vector<CardType> cards = CardsInHandInSuit(hand, (Suit)suit,
                                                    /*played_cards=*/0);
    for (auto ri = cards.rbegin(); ri != cards.rend(); ++ri) {
      CardType c = *ri;
      if (CardPlayedThisTrick(c)) {
        continue;
      }
      auto [found_suit, val] = Card::DecodeCard(c);
      CHECK_EQ(suit, found_suit);
      ans += Card::PrintCardValue(val);
    }
    if (suit < 3) {
      ans += ".";
    }
  }
  return ans;
}

std::string BoardState::GetPBNCards() const {
  std::string ans;
  for (int i = 0; i < 4; i++) {
    if (!ans.empty()) {
      ans += " ";
    }
    ans += ConvertHandToPBN(hands_[i] & (~played_cards_));
  }
  return "N:" + ans;
}

void BoardState::PlayTrick(const std::string &trick) {
  VLOG(2) << "PlayTrick: " << trick;
  std::vector<std::string> cards = SplitWords(trick);
  CHECK_GE(cards.size(), 4) << trick;
  for (int i = 0; i < 4; i++) {
    CardType c = Card::ParseStringCard(cards[i]);
    int64_t min_distance = 0;
    CardType best_card = 0;
    std::vector<CardType> valid_cards = ValidCardsToPlay();
    CHECK(!valid_cards.empty());
    for (uint i = 0; i < valid_cards.size(); i++) {
      int64_t d = c > valid_cards[i] ? c - valid_cards[i] :
                  valid_cards[i] - c;
      VLOG(3) << "Looking at card " << i << " / " << valid_cards.size()
              << ": " << Card::PrintCard(valid_cards[i])
              << " vs " << Card::PrintCard(c)
              << " with distance " << d << " (" << valid_cards[i] << " - "
              << c << ")";
      if (i == 0 || d < min_distance) {
        min_distance = d;
        best_card = valid_cards[i];
        VLOG(3) << "best_card now " << Card::PrintCard(best_card);
      }
    }
    if (c != best_card) {
      VLOG(1) << "Can't find " << cards[i]
              << ", instead playing " << Card::PrintCard(best_card);
    }
    PlayCard(CurPlayer(), best_card);
  }
}

int BoardState::FindBestDiscard(int n, int *suit, int *rank) const {
  uint64_t mask = ~played_cards_;
  uint64_t h0 = hands_[0] & mask;
  uint64_t h2 = hands_[2] & mask;
  uint64_t my_h = hands_[CurPlayer()] & mask;
  int hand0_dist[4];
  int hand2_dist[4];
  int myhand_dist[4];
  GetSuitDistribution(h0, hand0_dist);
  GetSuitDistribution(h2, hand2_dist);
  GetSuitDistribution(my_h, myhand_dist);

  int best_rank_in_suit[4] = {0, 0, 0, 0};
  for (int i = 0; i < n; i++) {
    best_rank_in_suit[suit[i]] = std::max(best_rank_in_suit[suit[i]], rank[i]);
  }

  int decl_best_rank[4] = {0, 0, 0, 0};
  uint64_t decl_cards = h0 | h2;
  for (int s = 0; s < 4; ++s) {
    uint16_t suit_bits = (decl_cards >> (s * 16)) & 0xffff;
    if (suit_bits != 0) {
      decl_best_rank[s] = (31 - __builtin_clz(suit_bits)) + 2;
    }
  }

  int64_t best_score = 0;
  int best_card = 0;
  for (int i = 0; i < n; i++) {
    int64_t score = 0;
    int suit_i = suit[i];
    int decl_max = std::max(hand0_dist[suit_i], hand2_dist[suit_i]);
    int suit_surplus = myhand_dist[suit_i] - decl_max;
    if (suit_surplus > 0) {
      score += 1000 * suit_surplus;
    } else if (suit_surplus == 0 && best_rank_in_suit[suit_i] >= 10 &&
               decl_best_rank[suit_i] > best_rank_in_suit[suit_i] &&
               hand0_dist[suit_i] + hand2_dist[suit_i] > myhand_dist[suit_i]) {
      score -= 100 * decl_max;
    }
    score -= best_rank_in_suit[suit_i] * 20;
    score -= rank[i];
    if (i == 0 || score > best_score) {
      best_score = score;
      best_card = i;
    }
  }
  return best_card;
}
