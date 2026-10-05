#include "hand_requirements.h"

#include <cctype>
#include <sstream>
#include <vector>
#include "card.h"

namespace {

std::string Trim(const std::string &s) {
  size_t start = s.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) return "";
  size_t end = s.find_last_not_of(" \t\r\n");
  return s.substr(start, end - start + 1);
}

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

Suit ParseSuitString(const std::string &s) {
  return Card::ParseSuit(s);
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

}  // namespace

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
  return "SuitAtLeast(" + SuitToString(suit_) + ", " + std::to_string(count_) + ")";
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
  return "SuitLength(" + SuitToString(suit_) + ", " + std::to_string(low_) + ", " +
         std::to_string(high_) + ")";
}

bool HighCardRange::MeetsRequirement(uint64_t hand) const {
  int hcp = GetHighCardPoints(hand);
  return hcp >= low_ && hcp <= high_;
}

std::string HighCardRange::ToString() const {
  return "HighCardRange(" + std::to_string(low_) + ", " + std::to_string(high_) + ")";
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

  std::string class_lower;
  for (char c : class_name) class_lower += tolower(c);

  if (class_lower == "suitatleast") {
    auto args = SplitTopLevelArgs(args_str);
    if (args.size() >= 2) {
      Suit s = ParseSuitString(args[0]);
      int count = std::stoi(args[1]);
      return std::make_shared<SuitAtLeast>(s, count);
    }
  } else if (class_lower == "suitlength") {
    auto args = SplitTopLevelArgs(args_str);
    if (args.size() >= 3) {
      Suit s = ParseSuitString(args[0]);
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
  }

  LOG(WARNING) << "Failed to parse HandRequirements from string: " << str;
  return nullptr;
}

