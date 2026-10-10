#include "deal.h"

#include "card.h"
#include "hand.h"
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
    int hand1_count) {
  for (int i = 0; i < 1000 * 1000; i++) {
    auto [hand1, hand2] = DealTwoHands(cards, hand1_count);
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
  LOG(FATAL) << "Couldn't meet requirements";
}

static std::string TrimString(const std::string &s) {
  size_t start = 0;
  while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
    start++;
  }
  if (start == s.size()) {
    return "";
  }
  size_t end = s.size();
  while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
    end--;
  }
  return s.substr(start, end - start);
}

static std::string ToLowerString(const std::string &s) {
  std::string res = s;
  for (char &c : res) {
    c = std::tolower(static_cast<unsigned char>(c));
  }
  return res;
}

static Suit ParseSuitName(const std::string &val_str) {
  return Card::ParseSuit(val_str);
}

static PlayerDirection ParseDirectionName(const std::string &val_str) {
  std::string s = ToLowerString(TrimString(val_str));
  if (s == "north" || s == "n") return NORTH;
  if (s == "east" || s == "e") return EAST;
  if (s == "south" || s == "s") return SOUTH;
  if (s == "west" || s == "w") return WEST;
  LOG(ERROR) << "Unrecognized player direction: " << val_str;
  throw std::runtime_error("Unrecognized player direction: " + val_str);
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
    std::string trimmed = TrimString(line_content);
    if (trimmed.empty()) {
      continue;
    }

    size_t eq_pos = trimmed.find('=');
    if (eq_pos == std::string::npos) {
      LOG(ERROR) << "Line " << line_num << " is missing '=' delimiter: " << line;
      throw std::runtime_error("Missing '=' delimiter in deal line: " + line);
    }

    std::string key = ToLowerString(TrimString(trimmed.substr(0, eq_pos)));
    std::string val = TrimString(trimmed.substr(eq_pos + 1));

    if (key == "north" || key == "n") {
      if (val.empty()) {
        LOG(ERROR) << "Empty hand string provided for north";
        throw std::runtime_error("Empty hand string provided for north");
      }
      std::vector<std::string> words = SplitWords(val);
      std::string hand_str = StrJoin(words, " ");
      uint64_t parsed = ParseHand(hand_str);
      if (parsed == 0) {
        LOG(ERROR) << "Unparseable hand string for north: " << val;
        throw std::runtime_error("Unparseable hand string for north: " + val);
      }
      hands[NORTH] = parsed;
      hand_set[NORTH] = true;
    } else if (key == "east" || key == "e") {
      if (val.empty()) {
        LOG(ERROR) << "Empty hand string provided for east";
        throw std::runtime_error("Empty hand string provided for east");
      }
      std::vector<std::string> words = SplitWords(val);
      std::string hand_str = StrJoin(words, " ");
      uint64_t parsed = ParseHand(hand_str);
      if (parsed == 0) {
        LOG(ERROR) << "Unparseable hand string for east: " << val;
        throw std::runtime_error("Unparseable hand string for east: " + val);
      }
      hands[EAST] = parsed;
      hand_set[EAST] = true;
    } else if (key == "south" || key == "s") {
      if (val.empty()) {
        LOG(ERROR) << "Empty hand string provided for south";
        throw std::runtime_error("Empty hand string provided for south");
      }
      std::vector<std::string> words = SplitWords(val);
      std::string hand_str = StrJoin(words, " ");
      uint64_t parsed = ParseHand(hand_str);
      if (parsed == 0) {
        LOG(ERROR) << "Unparseable hand string for south: " << val;
        throw std::runtime_error("Unparseable hand string for south: " + val);
      }
      hands[SOUTH] = parsed;
      hand_set[SOUTH] = true;
    } else if (key == "west" || key == "w") {
      if (val.empty()) {
        LOG(ERROR) << "Empty hand string provided for west";
        throw std::runtime_error("Empty hand string provided for west");
      }
      std::vector<std::string> words = SplitWords(val);
      std::string hand_str = StrJoin(words, " ");
      uint64_t parsed = ParseHand(hand_str);
      if (parsed == 0) {
        LOG(ERROR) << "Unparseable hand string for west: " << val;
        throw std::runtime_error("Unparseable hand string for west: " + val);
      }
      hands[WEST] = parsed;
      hand_set[WEST] = true;
    } else if (key == "trump" || key == "trumps") {
      trump = ParseSuitName(val);
    } else if (key == "leader" || key == "lead") {
      leader = ParseDirectionName(val);
    } else {
      LOG(ERROR) << "Unrecognized key '" << key << "' on line " << line_num;
      throw std::runtime_error("Unrecognized key in deal file: " + key);
    }
  }

  for (int i = 0; i < 4; i++) {
    if (!hand_set[i]) {
      std::string dir_name;
      switch (i) {
        case NORTH: dir_name = "north"; break;
        case EAST:  dir_name = "east"; break;
        case SOUTH: dir_name = "south"; break;
        case WEST:  dir_name = "west"; break;
      }
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
