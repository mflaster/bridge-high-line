#include "deal.h"

#include "card.h"
#include "hand.h"
#include "string_util.h"
#include "glog/logging.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>



std::vector<uint64_t> DealHands() {
  std::multiset<std::pair<double, CardType>> deck;
  for (int suit = 0; suit < 4; suit++) {
    for (int val = 0; val < 13; val++) {
      deck.insert(std::make_pair(drand48(),
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

std::pair<uint64_t, uint64_t> DealTwoHands(uint64_t cards, int hand1_count) {
  std::multiset<std::pair<double, CardType>> deck;
  for (int suit = 0; suit < 4; suit++) {
    for (int val = 0; val < 13; val++) {
      CardType c = Card::GetSuitedCard((Suit)suit, val);
      if (c & cards) {
        deck.insert(std::make_pair(drand48(),
                                   c));
      }
    }
  }
  int cards_in_deck = deck.size();
  CHECK_GE(cards_in_deck, 1);
  int target1 = (hand1_count >= 0) ? hand1_count : (cards_in_deck / 2);
  uint64_t hand1 = 0;
  uint64_t hand2 = 0;
  uint64_t *cur_hand = (target1 > 0) ? &hand1 : &hand2;
  int cur_cards = 0;
  for (const auto &[_r, card] : deck) {
    assert(((*cur_hand) & card) == 0);
    *cur_hand = ((*cur_hand) | card);
    cur_cards++;
    if (cur_hand == &hand1 && cur_cards >= target1) {
      cur_hand = &hand2;
      cur_cards = 0;
    }
  }
  return std::make_pair(hand1, hand2);
}

std::pair<uint64_t, uint64_t> DealTwoRequiredHands(
    uint64_t cards,
    const std::vector<std::shared_ptr<HandRequirements>> &reqs1,
    const std::vector<std::shared_ptr<HandRequirements>> &reqs2,
    int hand1_count, uint64_t known_hand1, uint64_t known_hand2) {
  for (int i = 0; i < 1000 * 1000; i++) {
    auto [hand1, hand2] = DealTwoHands(cards, hand1_count);
    uint64_t full_hand1 = known_hand1 | hand1;
    uint64_t full_hand2 = known_hand2 | hand2;
    bool meets_reqs = true;
    for (const auto &req : reqs1) {
      if (!req->MeetsRequirement(full_hand1)) {
        meets_reqs = false;
        break;
      }
    }

    for (const auto &req : reqs2) {
      if (!req->MeetsRequirement(full_hand2)) {
        meets_reqs = false;
        break;
      }
    }
    if (meets_reqs) {
      return std::make_pair(hand1, hand2);
    }
  }
  LOG(ERROR) << "Couldn't meet requirements";
  throw std::runtime_error("Couldn't meet requirements");
}

std::pair<uint64_t, uint64_t> FastDealTwoHands(uint64_t cards, int hand1_count,
                                               std::mt19937_64 &rng) {
  constexpr uint64_t kAllCardsMask = 0x1FFF1FFF1FFF1FFFULL;
  CardType deck[52];
  int n = 0;
  uint64_t rem = cards & kAllCardsMask;
  while (rem != 0) {
    uint64_t lsb = rem & (~rem + 1ULL);
    deck[n++] = lsb;
    rem ^= lsb;
  }
  CHECK_GE(n, 1);
  int target1 = std::clamp((hand1_count >= 0) ? hand1_count : (n / 2), 0, n);
  uint64_t hand1 = 0;
  for (int i = 0; i < target1; ++i) {
    std::uniform_int_distribution<int> dist(i, n - 1);
    int pick = dist(rng);
    hand1 |= deck[pick];
    deck[pick] = deck[i];
  }
  uint64_t hand2 = (cards & kAllCardsMask) & ~hand1;
  return {hand1, hand2};
}

std::pair<uint64_t, uint64_t> FastDealTwoRequiredHands(
    uint64_t cards,
    const std::vector<std::shared_ptr<HandRequirements>> &reqs1,
    const std::vector<std::shared_ptr<HandRequirements>> &reqs2,
    int hand1_count, uint64_t known_hand1, uint64_t known_hand2,
    std::mt19937_64 &rng) {
  for (int i = 0; i < 1000 * 1000; ++i) {
    auto [hand1, hand2] = FastDealTwoHands(cards, hand1_count, rng);
    uint64_t full_hand1 = known_hand1 | hand1;
    uint64_t full_hand2 = known_hand2 | hand2;
    bool meets_reqs = true;
    for (const auto &req : reqs1) {
      if (!req->MeetsRequirement(full_hand1)) {
        meets_reqs = false;
        break;
      }
    }
    if (meets_reqs) {
      for (const auto &req : reqs2) {
        if (!req->MeetsRequirement(full_hand2)) {
          meets_reqs = false;
          break;
        }
      }
    }
    if (meets_reqs) {
      return {hand1, hand2};
    }
  }
  LOG(ERROR) << "Couldn't meet requirements";
  throw std::runtime_error("Couldn't meet requirements");
}


uint64_t DealOneHand(uint64_t cards) {
  std::multiset<std::pair<double, CardType>> deck;
  for (int suit = 0; suit < 4; suit++) {
    for (int val = 0; val < 13; val++) {
      CardType c = Card::GetSuitedCard((Suit)suit, val);
      if (c & cards) {
        deck.insert(std::make_pair(drand48(),
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
  LOG(ERROR) << "Couldn't meet requirements";
  throw std::runtime_error("Couldn't meet requirements");
}


bool MeetsAllRequirements(
    const std::vector<uint64_t> &hands,
    const std::vector<std::vector<std::shared_ptr<HandRequirements>>> &reqs) {
  CHECK_EQ(hands.size(), 4);
  CHECK_LE(reqs.size(), 4);
  for (unsigned int i = 0; i < reqs.size(); i++) {
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
          deck.insert(std::make_pair(drand48(),
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
  LOG(ERROR) << "Couldn't meet requirements";
  throw std::runtime_error("Couldn't meet requirements");
}

BoardState LoadDealFromKeyValueString(const std::string &content) {
  std::istringstream stream(content);
  std::string line;
  std::vector<uint64_t> hands(4, 0);
  std::vector<bool> hand_set(4, false);
  Suit trump = NOTRUMP;
  int leader = NORTH;

  int line_num = 0;
  while (std::getline(stream, line)) {
    line_num++;
    std::string line_content = line;
    size_t hash_pos = line_content.find('#');
    if (hash_pos != std::string::npos) {
      line_content = line_content.substr(0, hash_pos);
    }
    size_t slash_pos = line_content.find("//");
    if (slash_pos != std::string::npos) {
      line_content = line_content.substr(0, slash_pos);
    }
    std::string trimmed = Trim(line_content);
    if (trimmed.empty()) {
      continue;
    }

    size_t eq_pos = trimmed.find('=');
    if (eq_pos == std::string::npos) {
      LOG(ERROR) << "Line " << line_num << " is missing '=' delimiter: " << line;
      throw std::runtime_error("Missing '=' delimiter in deal line: " + line);
    }

    std::string key = ToLower(Trim(trimmed.substr(0, eq_pos)));
    std::string val = Trim(trimmed.substr(eq_pos + 1));

    bool handled_hand = false;
    for (PlayerDirection dir : {NORTH, EAST, SOUTH, WEST}) {
      std::string dir_name = ToLower(Card::PlayerDirectionDebugString(dir));
      if (key == dir_name || (key.length() == 1 && key[0] == dir_name[0])) {
        if (val.empty()) {
          LOG(ERROR) << "Empty hand string provided for " << dir_name;
          throw std::runtime_error(
              "Empty hand string provided for " + dir_name);
        }
        std::vector<std::string> words = SplitWords(val);
        std::string hand_str = StrJoin(words, " ");
        uint64_t parsed = ParseHand(hand_str);
        if (parsed == 0) {
          LOG(ERROR) << "Unparseable hand string for " << dir_name
                     << ": " << val;
          throw std::runtime_error(
              "Unparseable hand string for " + dir_name + ": " + val);
        }
        hands[dir] = parsed;
        hand_set[dir] = true;
        handled_hand = true;
        break;
      }
    }

    if (!handled_hand) {
      if (key == "trump" || key == "trumps") {
        trump = Card::ParseSuit(val);
      } else if (key == "leader" || key == "lead") {
        leader = Card::ParsePlayerDirection(val);
      } else {
        LOG(ERROR) << "Unrecognized key '" << key << "' on line " << line_num;
        throw std::runtime_error("Unrecognized key in deal file: " + key);
      }
    }
  }

  for (int i = 0; i < 4; i++) {
    if (!hand_set[i]) {
      std::string dir_name = ToLower(Card::PlayerDirectionDebugString(i));
      LOG(ERROR) << "Required directional hand missing from deal: " << dir_name;
      throw std::runtime_error("Missing required directional hand: " + dir_name);
    }
  }

  uint64_t combined_cards = 0;
  for (int i = 0; i < 4; i++) {
    if ((combined_cards & hands[i]) != 0) {
      LOG(ERROR) << "Duplicate card detected in directional hand " << i;
      throw std::runtime_error("Duplicate card detected across hands in deal");
    }
    combined_cards |= hands[i];
  }

  BoardState bs;
  bs.SetHands(hands);
  bs.SetTrump(trump);
  bs.SetLead(leader);
  return bs;
}

BoardState LoadDealFromKeyValueFile(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    LOG(ERROR) << "Could not open deal file: " << filename;
    throw std::runtime_error("Could not open deal file: " + filename);
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return LoadDealFromKeyValueString(buffer.str());
}
