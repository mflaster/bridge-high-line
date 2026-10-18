#include "hand_requirements.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>
#include "card.h"
#include "glog/logging.h"
#include "sayc_generated.h"
#include "string_util.h"

namespace {

std::string SuitToString(Suit s) {
  switch (s) {
    case SPADES: return "Spades";
    case HEARTS: return "Hearts";
    case DIAMONDS: return "Diamonds";
    case CLUBS: return "Clubs";
    case NOTRUMP: return "NoTrump";
    default: return "Unknown";
  }
}

std::vector<std::string> SplitTopLevelArgs(const std::string &args_str) {
  std::vector<std::string> args;
  std::string current;
  int paren_depth = 0;
  for (char c : args_str) {
    if (c == '(') {
      paren_depth++;
      current += c;
    } else if (c == ')') {
      paren_depth--;
      current += c;
    } else if (c == ',' && paren_depth == 0) {
      args.push_back(Trim(current));
      current.clear();
    } else {
      current += c;
    }
  }
  std::string trimmed_last = Trim(current);
  if (!trimmed_last.empty() || !args.empty()) {
    args.push_back(trimmed_last);
  }
  return args;
}

std::string StripKeywordAndQuotes(const std::string &raw) {
  std::string s = Trim(raw);
  size_t eq_pos = s.find('=');
  if (eq_pos != std::string::npos) {
    std::string key = ToLower(Trim(s.substr(0, eq_pos)));
    if (key == "bid" || key == "after") {
      s = Trim(s.substr(eq_pos + 1));
    }
  }
  if (s.size() >= 2 &&
      ((s.front() == '"' && s.back() == '"') ||
       (s.front() == '\'' && s.back() == '\''))) {
    s = Trim(s.substr(1, s.size() - 2));
  }
  return s;
}

// Converts a normalized call (e.g. "1NT") to SAYCBridge's call string ("1N").
std::string ToSaycCallName(const std::string &norm_call) {
  if (norm_call.size() == 3 && norm_call[0] >= '1' && norm_call[0] <= '7' &&
      norm_call.substr(1) == "NT") {
    return std::string(1, norm_call[0]) + "N";
  }
  return norm_call;
}

// Normalizes a sequence of prior calls to the canonical SAYCBridge history key.
std::string ToSaycHistoryKey(const std::vector<std::string> &prior_calls) {
  size_t first_non_pass = 0;
  while (first_non_pass < prior_calls.size() &&
         prior_calls[first_non_pass] == "P") {
    ++first_non_pass;
  }
  if (first_non_pass == prior_calls.size()) {
    // Unopened auction: SAYCBridge uses "" for 1st/2nd/3rd seat (Rule of 20)
    // and "P P P" for 4th seat (Rule of 15).
    return prior_calls.size() == 3 ? "P P P" : "";
  }
  std::string key;
  for (size_t i = first_non_pass; i < prior_calls.size(); ++i) {
    if (!key.empty()) key += " ";
    key += ToSaycCallName(prior_calls[i]);
  }
  return key;
}

}  // namespace

/* static */
std::string WouldBid::NormalizeCall(const std::string &raw_call) {
  std::string s;
  for (size_t i = 0; i < raw_call.size(); ++i) {
    unsigned char ch = static_cast<unsigned char>(raw_call[i]);
    if (std::isspace(ch)) continue;
    // Handle UTF-8 suit symbols: ♠ (E2 99 A0), ♥ (E2 99 A5), ♦ (E2 99 A6),
    // ♣ (E2 99 A3).
    if (ch == 0xE2 && i + 2 < raw_call.size() &&
        static_cast<unsigned char>(raw_call[i + 1]) == 0x99) {
      unsigned char sym = static_cast<unsigned char>(raw_call[i + 2]);
      if (sym == 0xA0) s += 'S';
      else if (sym == 0xA5) s += 'H';
      else if (sym == 0xA6) s += 'D';
      else if (sym == 0xA3) s += 'C';
      i += 2;
      continue;
    }
    s += static_cast<char>(std::toupper(ch));
  }
  if (s.empty() || s == "P" || s == "PASS" || s == "-") {
    return "P";
  }
  if (s == "X" || s == "DBL" || s == "DOUBLE" || s == "D") {
    return "X";
  }
  if (s == "XX" || s == "RDBL" || s == "REDOUBLE" || s == "R") {
    return "XX";
  }
  if (s.size() >= 2 && s[0] >= '1' && s[0] <= '7') {
    std::string strain = s.substr(1);
    if (strain == "N" || strain == "NT") {
      return std::string(1, s[0]) + "NT";
    }
    if (strain == "C" || strain == "D" || strain == "H" || strain == "S") {
      return std::string(1, s[0]) + strain;
    }
  }
  return s;
}

WouldBid::WouldBid(const std::string &bid, const std::string &after) {
  bid_ = NormalizeCall(StripKeywordAndQuotes(bid));
  std::string clean_after = StripKeywordAndQuotes(after);
  std::vector<std::string> prior_calls;
  std::string tok;
  for (size_t i = 0; i < clean_after.size(); ++i) {
    char c = clean_after[i];
    if (std::isspace(static_cast<unsigned char>(c)) || c == ',' || c == ';') {
      if (!tok.empty()) {
        prior_calls.push_back(NormalizeCall(tok));
        tok.clear();
      }
    } else {
      tok += c;
    }
  }
  if (!tok.empty()) {
    prior_calls.push_back(NormalizeCall(tok));
  }
  after_.clear();
  for (size_t i = 0; i < prior_calls.size(); ++i) {
    if (i > 0) after_ += " ";
    after_ += prior_calls[i];
  }
  int root = sayc::LookupSaycBidNode(ToSaycHistoryKey(prior_calls),
                                     ToSaycCallName(bid_));
  sayc::BuildSaycProgram(root, &prog_);
}

WouldBid::~WouldBid() = default;

bool WouldBid::MeetsRequirement(uint64_t hand) const {
  // Populate the 24 raw hand inputs defined by SAYCBridge's
  // z3b/model.py expr_for_hand(hand):
  //   vars[0..3]:   suit lengths (clubs, diamonds, hearts, spades)
  //   vars[4..23]:  A, K, Q, J, T indicator bits for each suit in order
  //                 (clubs, diamonds, hearts, spades).
  constexpr Suit kSuitOrder[4] = {CLUBS, DIAMONDS, HEARTS, SPADES};
  int vars[24];
  for (int i = 0; i < 4; ++i) {
    uint16_t sb =
        static_cast<uint16_t>((hand >> (kSuitOrder[i] * 16)) & 0x1FFFU);
    vars[i] = PopCount64(sb);
    int base = 4 + i * 5;
    vars[base + 0] = (sb >> 12) & 1;  // Ace
    vars[base + 1] = (sb >> 11) & 1;  // King
    vars[base + 2] = (sb >> 10) & 1;  // Queen
    vars[base + 3] = (sb >> 9) & 1;   // Jack
    vars[base + 4] = (sb >> 8) & 1;   // Ten
  }
  return sayc::EvalSaycProgram(prog_, vars);
}

std::string WouldBid::ToString() const {
  if (after_.empty()) {
    return "WouldBid(" + bid_ + ")";
  }
  return "WouldBid(" + bid_ + ", " + after_ + ")";
}

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

std::string SuitAtLeast::ToString() const {
  return "SuitAtLeast(" + SuitToString(suit_) + ", " + std::to_string(count_) +
         ")";
}

bool HandHasCard::MeetsRequirement(uint64_t hand) const {
  return hand & card_;
}

std::string HandHasCard::ToString() const {
  return "HandHasCard(" + Card::PrintCard(card_) + ")";
}

bool SuitLength::MeetsRequirement(uint64_t hand) const {
  int dist[4];
  GetSuitDistribution(hand, dist);
  return dist[suit_] >= low_ && dist[suit_] <= high_;
}

std::string SuitLength::ToString() const {
  return "SuitLength(" + SuitToString(suit_) + ", " + std::to_string(low_) +
         ", " + std::to_string(high_) + ")";
}

bool HighCardRange::MeetsRequirement(uint64_t hand) const {
  int hcp = GetHighCardPoints(hand);
  return hcp >= low_ && hcp <= high_;
}

std::string HighCardRange::ToString() const {
  return "HighCardRange(" + std::to_string(low_) + ", " +
         std::to_string(high_) + ")";
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

std::string BalancedHand::ToString() const {
  return "BalancedHand()";
}

bool AntiRequirement::MeetsRequirement(uint64_t hand) const {
  return !other_->MeetsRequirement(hand);
}

std::string AntiRequirement::ToString() const {
  return "Anti(" + (other_ ? other_->ToString() : "") + ")";
}

bool AtLeastAces::MeetsRequirement(uint64_t hand) const {
  return PopCount64(hand & kAceMask64) >= n_;
}

std::string AtLeastAces::ToString() const {
  return "AtLeastAces(" + std::to_string(n_) + ")";
}

bool AnyOfRequirement::MeetsRequirement(uint64_t hand) const {
  for (const auto &req : reqs_) {
    if (req && req->MeetsRequirement(hand)) {
      return true;
    }
  }
  return false;
}

std::string AnyOfRequirement::ToString() const {
  std::string res = "AnyOf(";
  for (size_t i = 0; i < reqs_.size(); ++i) {
    if (i > 0) res += ", ";
    if (reqs_[i]) res += reqs_[i]->ToString();
  }
  res += ")";
  return res;
}

std::shared_ptr<HandRequirements> HandRequirements::FromString(
    const std::string &str) {
  std::string trimmed = Trim(str);
  if (trimmed.empty()) return nullptr;

  std::string class_name;
  std::string args_str;

  size_t open_paren = trimmed.find('(');
  if (open_paren != std::string::npos && trimmed.back() == ')') {
    class_name = Trim(trimmed.substr(0, open_paren));
    args_str = trimmed.substr(open_paren + 1, trimmed.size() - open_paren - 2);
  } else {
    class_name = trimmed;
    args_str = "";
  }

  std::string class_lower = ToLower(class_name);

  if (class_lower == "suitatleast") {
    auto args = SplitTopLevelArgs(args_str);
    if (args.size() >= 2) {
      Suit s = Card::ParseSuit(args[0]);
      int count = std::stoi(args[1]);
      return std::make_shared<SuitAtLeast>(s, count);
    }
  } else if (class_lower == "suitlength") {
    auto args = SplitTopLevelArgs(args_str);
    if (args.size() >= 3) {
      Suit s = Card::ParseSuit(args[0]);
      int low = std::stoi(args[1]);
      int high = std::stoi(args[2]);
      return std::make_shared<SuitLength>(s, low, high);
    }
  } else if (class_lower == "balancedhand") {
    return std::make_shared<BalancedHand>();
  } else if (class_lower == "anti" || class_lower == "antirequirement") {
    auto inner = HandRequirements::FromString(args_str);
    if (inner) {
      return std::make_shared<AntiRequirement>(inner);
    }
  } else if (class_lower == "highcardrange") {
    auto args = SplitTopLevelArgs(args_str);
    if (args.size() >= 2) {
      int low = std::stoi(args[0]);
      int high = std::stoi(args[1]);
      return std::make_shared<HighCardRange>(low, high);
    }
  } else if (class_lower == "handhascard") {
    std::string card_str;
    for (char c : Trim(args_str)) {
      if (!isspace(c)) card_str += c;
    }
    if (card_str.size() == 2) {
      CardType c = Card::ParseStringCard(card_str);
      return std::make_shared<HandHasCard>(c);
    }
  } else if (class_lower == "atleastaces") {
    std::string trimmed_arg = Trim(args_str);
    if (!trimmed_arg.empty()) {
      int n = std::stoi(trimmed_arg);
      return std::make_shared<AtLeastAces>(n);
    }
  } else if (class_lower == "anyof" || class_lower == "anyofrequirement") {
    auto args = SplitTopLevelArgs(args_str);
    std::vector<std::shared_ptr<HandRequirements>> reqs;
    for (const auto &arg : args) {
      auto req = HandRequirements::FromString(arg);
      if (req) {
        reqs.push_back(req);
      }
    }
    return std::make_shared<AnyOfRequirement>(reqs);
  } else if (class_lower == "wouldbid") {
    auto args = SplitTopLevelArgs(args_str);
    if (args.size() == 1) {
      return std::make_shared<WouldBid>(args[0], "");
    } else if (args.size() >= 2) {
      return std::make_shared<WouldBid>(args[0], args[1]);
    }
  }

  LOG(WARNING) << "Failed to parse HandRequirements from string: " << str;
  return nullptr;
}
