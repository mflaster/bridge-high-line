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
#include <nlohmann/json.hpp>

namespace {

static std::string ToUpper(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  return s;
}

static Suit ParseSuitString(const std::string &raw_s) {
  std::string s = ToUpper(raw_s);
  if (s == "SPADES" || s == "SPADE" || s == "S") return SPADES;
  if (s == "HEARTS" || s == "HEART" || s == "H") return HEARTS;
  if (s == "DIAMONDS" || s == "DIAMOND" || s == "D") return DIAMONDS;
  if (s == "CLUBS" || s == "CLUB" || s == "C") return CLUBS;
  if (s == "NOTRUMP" || s == "NO_TRUMP" || s == "NO TRUMP" || s == "NT" || s == "N") return NOTRUMP;
  throw std::invalid_argument("Invalid trump suit string: " + raw_s);
}

static PlayerDirection ParseLeaderString(const std::string &raw_s) {
  std::string s = ToUpper(raw_s);
  if (s == "NORTH" || s == "N") return NORTH;
  if (s == "EAST" || s == "E") return EAST;
  if (s == "SOUTH" || s == "S") return SOUTH;
  if (s == "WEST" || s == "W") return WEST;
  throw std::invalid_argument("Invalid leader direction string: " + raw_s);
}

static uint64_t ParseAndValidateHandString(const std::string &hand_str, const std::string &hand_label) {
  uint64_t hand_mask = 0;
  Suit cur_suit = NOTRUMP;
  for (size_t i = 0; i < hand_str.size(); ++i) {
    char ch = hand_str[i];
    if (ch == ' ' || ch == ';' || ch == '-') continue;
    if (ch == 'S' || ch == 's') { cur_suit = SPADES; continue; }
    if (ch == 'H' || ch == 'h') { cur_suit = HEARTS; continue; }
    if (ch == 'D' || ch == 'd') { cur_suit = DIAMONDS; continue; }
    if (ch == 'C' || ch == 'c') { cur_suit = CLUBS; continue; }

    int card_val = -1;
    if (ch == 'A' || ch == 'a') card_val = 12;
    else if (ch == 'K' || ch == 'k') card_val = 11;
    else if (ch == 'Q' || ch == 'q') card_val = 10;
    else if (ch == 'J' || ch == 'j') card_val = 9;
    else if (ch == 'T' || ch == 't') card_val = 8;
    else if (ch >= '2' && ch <= '9') card_val = ch - '2';
    else {
      throw std::invalid_argument("Invalid card character '" + std::string(1, ch) + "' in hand " + hand_label);
    }

    if (cur_suit == NOTRUMP) {
      throw std::invalid_argument("Card rank '" + std::string(1, ch) + "' specified before suit in hand " + hand_label);
    }

    uint64_t card_bit = Card::GetSuitedCard(cur_suit, card_val);
    if ((hand_mask & card_bit) != 0) {
      throw std::invalid_argument("Duplicate card defined in hand " + hand_label + ": " + Card::PrintCard(card_bit));
    }
    hand_mask |= card_bit;
  }
  return hand_mask;
}

static void ValidateDealHands(const std::vector<uint64_t> &hands) {
  if (hands.size() != 4) {
    throw std::invalid_argument("Deal must define exactly 4 hands");
  }
  uint64_t seen = 0;
  std::vector<int> card_counts(4, 0);
  for (int i = 0; i < 4; ++i) {
    uint64_t h = hands[i];
    card_counts[i] = CountCardsInHand(h);
    uint64_t duplicate = seen & h;
    if (duplicate != 0) {
      std::string card_name = "unknown";
      for (int s = 0; s < 4; ++s) {
        for (int v = 0; v < 13; ++v) {
          uint64_t c = Card::GetSuitedCard((Suit)s, v);
          if (duplicate & c) {
            card_name = Card::PrintCard(c);
            break;
          }
        }
      }
      throw std::invalid_argument("Duplicate card detected across hands: " + card_name);
    }
    seen |= h;
  }
  for (int i = 1; i < 4; ++i) {
    if (card_counts[i] != card_counts[0]) {
      throw std::invalid_argument("Hand card counts are unequal: North=" + std::to_string(card_counts[0]) +
                                  " vs hand " + std::to_string(i) + "=" + std::to_string(card_counts[i]));
    }
  }
  int total_cards = CountCardsInHand(seen);
  if (total_cards == 0) {
    throw std::invalid_argument("Deal contains no cards");
  }
}

}  // namespace

DealFixture LoadDealFromJSON(const std::string &json_str, const std::string &source_name) {
  nlohmann::json j;
  try {
    j = nlohmann::json::parse(json_str);
  } catch (const nlohmann::json::parse_error &e) {
    throw std::runtime_error("Malformed JSON in " + source_name + ": " + e.what());
  }

  if (!j.is_object()) {
    throw std::invalid_argument("JSON root must be an object in " + source_name);
  }

  if (!j.contains("hands")) {
    throw std::invalid_argument("Missing required 'hands' field in " + source_name);
  }

  std::vector<uint64_t> hands(4, 0);
  const auto &j_hands = j["hands"];

  if (j_hands.is_object()) {
    static const std::vector<std::pair<std::vector<std::string>, int>> dir_keys = {
      {{"NORTH", "North", "north", "N", "n"}, NORTH},
      {{"EAST", "East", "east", "E", "e"}, EAST},
      {{"SOUTH", "South", "south", "S", "s"}, SOUTH},
      {{"WEST", "West", "west", "W", "w"}, WEST}
    };
    for (const auto &[aliases, dir_idx] : dir_keys) {
      bool found = false;
      for (const auto &alias : aliases) {
        if (j_hands.contains(alias)) {
          if (!j_hands[alias].is_string()) {
            throw std::invalid_argument("Hand string for " + alias + " must be a string in " + source_name);
          }
          hands[dir_idx] = ParseAndValidateHandString(j_hands[alias].get<std::string>(), alias);
          found = true;
          break;
        }
      }
      if (!found) {
        throw std::invalid_argument("Missing hand specification for direction index " + std::to_string(dir_idx) + " in " + source_name);
      }
    }
  } else if (j_hands.is_array()) {
    if (j_hands.size() != 4) {
      throw std::invalid_argument("'hands' array must contain exactly 4 hand strings in " + source_name);
    }
    for (int i = 0; i < 4; ++i) {
      if (!j_hands[i].is_string()) {
        throw std::invalid_argument("Hand at index " + std::to_string(i) + " must be a string in " + source_name);
      }
      hands[i] = ParseAndValidateHandString(j_hands[i].get<std::string>(), "index " + std::to_string(i));
    }
  } else {
    throw std::invalid_argument("'hands' field must be an object or array in " + source_name);
  }

  ValidateDealHands(hands);

  Suit trump = NOTRUMP;
  std::string trump_key = "";
  if (j.contains("trump")) trump_key = "trump";
  else if (j.contains("trump_suit")) trump_key = "trump_suit";
  else {
    throw std::invalid_argument("Missing required 'trump' field in " + source_name);
  }

  if (j[trump_key].is_string()) {
    trump = ParseSuitString(j[trump_key].get<std::string>());
  } else if (j[trump_key].is_number_integer()) {
    int t_val = j[trump_key].get<int>();
    if (t_val < 0 || t_val > 4) {
      throw std::invalid_argument("Invalid trump integer value " + std::to_string(t_val) + " in " + source_name);
    }
    trump = static_cast<Suit>(t_val);
  } else {
    throw std::invalid_argument("'trump' field must be a string or integer in " + source_name);
  }

  PlayerDirection leader = NORTH;
  std::string leader_key = "";
  if (j.contains("leader")) leader_key = "leader";
  else if (j.contains("initial_leader")) leader_key = "initial_leader";
  else if (j.contains("first")) leader_key = "first";
  else {
    throw std::invalid_argument("Missing required 'leader' field in " + source_name);
  }

  if (j[leader_key].is_string()) {
    leader = ParseLeaderString(j[leader_key].get<std::string>());
  } else if (j[leader_key].is_number_integer()) {
    int l_val = j[leader_key].get<int>();
    if (l_val < 0 || l_val > 3) {
      throw std::invalid_argument("Invalid leader integer value " + std::to_string(l_val) + " in " + source_name);
    }
    leader = static_cast<PlayerDirection>(l_val);
  } else {
    throw std::invalid_argument("'leader' field must be a string or integer in " + source_name);
  }

  int target_tricks = 0;
  std::string target_key = "";
  if (j.contains("target_tricks")) target_key = "target_tricks";
  else if (j.contains("target")) target_key = "target";
  else if (j.contains("contract_tricks")) target_key = "contract_tricks";
  else if (j.contains("tricks")) target_key = "tricks";
  else {
    throw std::invalid_argument("Missing required 'target_tricks' field in " + source_name);
  }

  if (j[target_key].is_number_integer()) {
    target_tricks = j[target_key].get<int>();
  } else if (j[target_key].is_string()) {
    target_tricks = std::stoi(j[target_key].get<std::string>());
  } else {
    throw std::invalid_argument("'target_tricks' field must be an integer or numeric string in " + source_name);
  }

  DealFixture fixture;
  fixture.board_state.SetHands(hands);
  fixture.board_state.SetLead(static_cast<int>(leader));
  fixture.board_state.SetTrump(trump);
  fixture.target_tricks = target_tricks;
  fixture.trump = trump;
  fixture.leader = leader;
  fixture.hands = hands;

  return fixture;
}

DealFixture LoadDealFromJSONFile(const std::string &file_path) {
  std::ifstream ifs(file_path);
  if (!ifs.is_open()) {
    throw std::runtime_error("Could not open JSON fixture file: " + file_path);
  }
  std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  return LoadDealFromJSON(content, file_path);
}



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

std::pair<uint64_t, uint64_t> DealTwoHands(uint64_t cards) {
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
  std::string s = ToLowerString(TrimString(val_str));
  if (s == "spades" || s == "spade" || s == "s") return SPADES;
  if (s == "hearts" || s == "heart" || s == "h") return HEARTS;
  if (s == "diamonds" || s == "diamond" || s == "d") return DIAMONDS;
  if (s == "clubs" || s == "club" || s == "c") return CLUBS;
  if (s == "notrump" || s == "no_trump" || s == "notrumps" || s == "nt" || s == "n") return NOTRUMP;
  LOG(ERROR) << "Unrecognized trump suit name: " << val_str;
  throw std::runtime_error("Unrecognized trump suit name: " + val_str);
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
