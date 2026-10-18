#include "hand_requirements.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>
#include "card.h"
#include "glog/logging.h"
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

bool ParseContractCall(const std::string &call, int *level_out,
                       Suit *strain_out) {
  if (call.size() < 2 || call[0] < '1' || call[0] > '7') {
    return false;
  }
  int lvl = call[0] - '0';
  std::string st = call.substr(1);
  Suit suit = NOTRUMP;
  if (st == "S") {
    suit = SPADES;
  } else if (st == "H") {
    suit = HEARTS;
  } else if (st == "D") {
    suit = DIAMONDS;
  } else if (st == "C") {
    suit = CLUBS;
  } else if (st == "NT" || st == "N") {
    suit = NOTRUMP;
  } else {
    return false;
  }
  if (level_out) *level_out = lvl;
  if (strain_out) *strain_out = suit;
  return true;
}

int SuitRankOrder(Suit s) {
  switch (s) {
    case CLUBS: return 0;
    case DIAMONDS: return 1;
    case HEARTS: return 2;
    case SPADES: return 3;
    case NOTRUMP: return 4;
    default: return -1;
  }
}

int TopFiveHonorsInSuit(uint64_t hand, Suit s) {
  uint16_t sb = static_cast<uint16_t>((hand >> (s * 16)) & 0x1FFFU);
  return PopCount64(sb & 0x1F00U);  // 10, J, Q, K, A
}

int TopThreeHonorsInSuit(uint64_t hand, Suit s) {
  uint16_t sb = static_cast<uint16_t>((hand >> (s * 16)) & 0x1FFFU);
  return PopCount64(sb & 0x1C00U);  // Q, K, A
}

bool HasSuitStopper(uint64_t hand, Suit s, int len) {
  if (s < 0 || s >= 4) return false;
  uint16_t sb = static_cast<uint16_t>((hand >> (s * 16)) & 0x1FFFU);
  if (sb & (1U << 12)) return true;                 // Ace
  if ((sb & (1U << 11)) && len >= 2) return true;   // Kx+
  if ((sb & (1U << 10)) && len >= 3) return true;   // Qxx+
  if ((sb & (1U << 9)) && len >= 4) return true;    // Jxxx+
  return false;
}

bool IsGoodPreemptSuit(uint64_t hand, Suit s) {
  return TopThreeHonorsInSuit(hand, s) >= 2 ||
         TopFiveHonorsInSuit(hand, s) >= 3;
}

struct HandEvalMetrics {
  int dist[4] = {0, 0, 0, 0};
  int hcp = 0;
  int length_pts = 0;
  int playing_pts = 0;
  int shortness_pts = 0;
  int longest = 0;
  int second_longest = 0;
  bool is_balanced = false;
  bool has_opening_values = false;
};

HandEvalMetrics EvaluateHandForBidding(uint64_t hand) {
  HandEvalMetrics m;
  GetSuitDistribution(hand, m.dist);
  m.hcp = GetHighCardPoints(hand);
  int sorted_lens[4] = {m.dist[0], m.dist[1], m.dist[2], m.dist[3]};
  std::sort(sorted_lens, sorted_lens + 4, std::greater<int>());
  m.longest = sorted_lens[0];
  m.second_longest = sorted_lens[1];

  int doubletons = 0;
  bool has_short = false;
  for (int s = 0; s < 4; ++s) {
    if (m.dist[s] > 4) {
      m.length_pts += (m.dist[s] - 4);
    }
    if (m.dist[s] == 0) {
      m.shortness_pts += 5;
      has_short = true;
    } else if (m.dist[s] == 1) {
      m.shortness_pts += 3;
      has_short = true;
    } else if (m.dist[s] == 2) {
      m.shortness_pts += 1;
      ++doubletons;
    }
  }
  m.playing_pts = m.hcp + m.length_pts;
  m.is_balanced = !has_short && doubletons <= 1 && m.longest <= 5;
  // SAYC opening threshold: 12+ HCP, 13+ playing points, or Rule of 20 with
  // at least 10 HCP.
  m.has_opening_values =
      (m.hcp >= 12) || (m.playing_pts >= 13) ||
      (m.hcp >= 10 && (m.hcp + m.longest + m.second_longest) >= 20);
  return m;
}

bool WouldOpenSpecificCall(uint64_t hand, const HandEvalMetrics &m,
                           const std::string &call, int seat_index) {
  if (call == "3NT") {
    return m.is_balanced && m.hcp >= 25 && m.hcp <= 27;
  }
  if (call == "2C") {
    if (m.is_balanced && m.hcp >= 25 && m.hcp <= 27) return false;
    if (m.is_balanced && m.hcp >= 20 && m.hcp <= 21) return false;
    return m.hcp >= 22 || m.playing_pts >= 23;
  }
  if (call == "2NT") {
    return m.is_balanced && m.hcp >= 20 && m.hcp <= 21;
  }
  if (call == "1NT") {
    return m.is_balanced && m.hcp >= 15 && m.hcp <= 17;
  }

  int lvl = 0;
  Suit suit = NOTRUMP;
  if (!ParseContractCall(call, &lvl, &suit) || suit == NOTRUMP) {
    return false;
  }

  if (lvl == 1) {
    if (!m.has_opening_values) return false;
    if (m.is_balanced && ((m.hcp >= 15 && m.hcp <= 17) ||
                          (m.hcp >= 20 && m.hcp <= 21) ||
                          (m.hcp >= 25 && m.hcp <= 27))) {
      return false;
    }
    if (m.hcp >= 22 || m.playing_pts >= 23) {
      return false;
    }
    // 5-card major priority in SAYC.
    if (m.dist[SPADES] >= 5 || m.dist[HEARTS] >= 5) {
      if (suit == SPADES) {
        return m.dist[SPADES] >= 5 && m.dist[SPADES] >= m.dist[HEARTS];
      }
      if (suit == HEARTS) {
        return m.dist[HEARTS] >= 5 && m.dist[HEARTS] > m.dist[SPADES];
      }
      return false;
    }
    // Better minor when no 5-card major (3-3 -> 1C, 4-4 -> 1D).
    if (suit == DIAMONDS) {
      return m.dist[DIAMONDS] > m.dist[CLUBS] ||
             (m.dist[DIAMONDS] == m.dist[CLUBS] && m.dist[DIAMONDS] >= 4);
    }
    if (suit == CLUBS) {
      return m.dist[CLUBS] > m.dist[DIAMONDS] ||
             (m.dist[CLUBS] == m.dist[DIAMONDS] && m.dist[CLUBS] <= 3);
    }
    return false;
  }

  // Preemptive openings (2D/2H/2S, 3C/3D/3H/3S, 4C/4D/4H/4S).
  if (m.has_opening_values) return false;
  if (m.hcp < 5 || m.hcp > 10) return false;
  if (seat_index >= 3 && lvl <= 3) {
    // Do not open weak preempts in 4th seat after three passes.
    return false;
  }
  if (!IsGoodPreemptSuit(hand, suit)) return false;

  if (lvl == 2 && (suit == DIAMONDS || suit == HEARTS || suit == SPADES)) {
    if (m.dist[suit] != 6) return false;
    if (suit != SPADES && m.dist[SPADES] >= 4) return false;
    if (suit != HEARTS && m.dist[HEARTS] >= 4) return false;
    return true;
  }
  if (lvl == 3) {
    if (suit == CLUBS) {
      return (m.dist[CLUBS] == 6 || m.dist[CLUBS] == 7) &&
             m.dist[SPADES] < 4 && m.dist[HEARTS] < 4;
    }
    return m.dist[suit] == 7;
  }
  if (lvl == 4) {
    return m.dist[suit] >= 8;
  }
  return false;
}

bool WouldOpenAnyCall(uint64_t hand, const HandEvalMetrics &m, int seat_index) {
  if (m.has_opening_values) return true;
  static const char *kPreempts[] = {"2D", "2H", "2S", "3C", "3D",
                                    "3H", "3S", "4C", "4D", "4H", "4S"};
  for (const char *c : kPreempts) {
    if (WouldOpenSpecificCall(hand, m, c, seat_index)) {
      return true;
    }
  }
  return false;
}

bool QualifiesFor1NTOvercall(uint64_t hand, const HandEvalMetrics &m,
                             int open_level, Suit open_suit, bool balancing) {
  if (open_level != 1 || open_suit == NOTRUMP) return false;
  int min_hcp = balancing ? 11 : 15;
  int max_hcp = balancing ? 14 : 18;
  return m.is_balanced && m.hcp >= min_hcp && m.hcp <= max_hcp &&
         HasSuitStopper(hand, open_suit, m.dist[open_suit]);
}

bool QualifiesForTakeoutDouble(uint64_t hand, const HandEvalMetrics &m,
                               int open_level, Suit open_suit,
                               bool balancing) {
  if (open_suit == NOTRUMP) {
    return m.hcp >= 16;
  }
  if (QualifiesFor1NTOvercall(hand, m, open_level, open_suit, balancing)) {
    return false;
  }
  // Power double (17+ HCP) doubles first with any shape.
  if (m.hcp >= 17) {
    return true;
  }

  // Must be short in opener's suit (at most 2 cards, or 3 with 13+ HCP).
  if (m.dist[open_suit] > 3) return false;
  if (m.dist[open_suit] == 3 && m.hcp < 13) return false;

  // Minimum HCP for a standard takeout double:
  // - Direct seat over 1-level opening: 12+ HCP, or 11 HCP with singleton/void
  //   in opener's suit.
  // - Balancing seat over 1-level opening: 10+ HCP, or 9 HCP with
  //   singleton/void in opener's suit.
  // - Over 2+ level opening: 13+ HCP, or 12 HCP with singleton/void in
  //   opener's suit.
  int min_hcp = (open_level == 1) ? (balancing ? 10 : 12) : 13;
  int min_short_hcp = min_hcp - 1;
  bool enough_hcp =
      (m.hcp >= min_hcp) || (m.dist[open_suit] <= 1 && m.hcp >= min_short_hcp);
  if (!enough_hcp) return false;

  // With < 17 HCP:
  // - Any 6+ card unbid suit overcalls its own suit instead of doubling.
  // - A 5+ card unbid major overcalls the major (or bids Michaels if 5-5)
  //   instead of making a takeout double.
  for (int s = 0; s < 4; ++s) {
    if (s == open_suit) continue;
    if (m.dist[s] >= 6) return false;
    if ((s == SPADES || s == HEARTS) && m.dist[s] >= 5) return false;
  }

  // Classic takeout support: at least 3 cards in every unbid suit.
  bool all_unbid_3plus = true;
  for (int s = 0; s < 4; ++s) {
    if (s == open_suit) continue;
    if (m.dist[s] < 3) {
      all_unbid_3plus = false;
      break;
    }
  }
  if (all_unbid_3plus) {
    // If holding 3 cards in opener's suit, require 4 cards in the unbid major
    // (or at least one 4-card major over a minor).
    if (m.dist[open_suit] == 3) {
      if (open_suit == SPADES) return m.dist[HEARTS] == 4;
      if (open_suit == HEARTS) return m.dist[SPADES] == 4;
      return m.dist[SPADES] == 4 || m.dist[HEARTS] == 4;
    }
    return true;
  }

  // Over a minor opening (1C/1D), allow 4-4 in the majors with <= 2 in
  // opener's minor and a doubleton (never singleton/void) in the unbid minor
  // (i.e. 4-4-3-2 or 4-4-2-3).
  if ((open_suit == CLUBS || open_suit == DIAMONDS) &&
      m.dist[open_suit] <= 2 && m.dist[SPADES] == 4 && m.dist[HEARTS] == 4) {
    Suit other_minor = (open_suit == CLUBS) ? DIAMONDS : CLUBS;
    if (m.dist[other_minor] >= 2) {
      return true;
    }
  }
  return false;
}

bool QualifiesForOvercallCall(uint64_t hand, const HandEvalMetrics &m,
                              int open_level, Suit open_suit,
                              const std::string &call, bool balancing) {
  if (call == "1NT") {
    return QualifiesFor1NTOvercall(hand, m, open_level, open_suit, balancing);
  }
  if (call == "X") {
    return QualifiesForTakeoutDouble(hand, m, open_level, open_suit, balancing);
  }

  int bid_level = 0;
  Suit bid_suit = NOTRUMP;
  if (!ParseContractCall(call, &bid_level, &bid_suit)) {
    return false;
  }

  // Unusual 2NT over 1-level suit opening: 5-5 in two lowest unbid suits.
  if (call == "2NT" && open_level == 1 && open_suit != NOTRUMP) {
    Suit low1 = (open_suit == CLUBS) ? DIAMONDS : CLUBS;
    Suit low2 = (open_suit == CLUBS || open_suit == DIAMONDS) ? HEARTS
                                                              : DIAMONDS;
    return m.dist[low1] >= 5 && m.dist[low2] >= 5 && m.playing_pts >= 8;
  }

  if (bid_suit == NOTRUMP) return false;

  // Michaels cuebid (2x in opener's 1-level suit): 5-5 two-suiter.
  if (open_level == 1 && bid_level == 2 && bid_suit == open_suit) {
    if (open_suit == CLUBS || open_suit == DIAMONDS) {
      return m.dist[SPADES] >= 5 && m.dist[HEARTS] >= 5 && m.playing_pts >= 8;
    }
    Suit other_maj = (open_suit == SPADES) ? HEARTS : SPADES;
    return m.dist[other_maj] >= 5 &&
           (m.dist[CLUBS] >= 5 || m.dist[DIAMONDS] >= 5) &&
           m.playing_pts >= 8;
  }

  if (bid_suit == open_suit) return false;

  int min_simple_level =
      (SuitRankOrder(bid_suit) > SuitRankOrder(open_suit)) ? open_level
                                                           : (open_level + 1);
  if (bid_level < min_simple_level) return false;

  // Simple 1-level suit overcall (e.g. 1H over 1C/1D, 1S over 1C/1D/1H).
  if (bid_level == 1 && min_simple_level == 1) {
    int min_pts = balancing ? 7 : 8;
    if (m.dist[bid_suit] < 5) return false;
    if (m.playing_pts < min_pts || m.hcp > 17) return false;
    if (m.dist[bid_suit] == 5 && m.hcp < 9 &&
        TopFiveHonorsInSuit(hand, bid_suit) < 2) {
      return false;
    }
    return true;
  }

  // Simple 2-level suit overcall (non-jump).
  if (bid_level == 2 && min_simple_level == 2) {
    if (m.dist[bid_suit] < 5 || m.hcp > 17) return false;
    if (m.dist[bid_suit] == 5) {
      return m.hcp >= 10 && TopFiveHonorsInSuit(hand, bid_suit) >= 2;
    }
    return m.hcp >= 9 || m.playing_pts >= 10;
  }

  // Weak jump overcall (jump by 1 or 2 levels).
  if (bid_level > min_simple_level) {
    if (m.has_opening_values || m.hcp < 5 || m.hcp > 10) return false;
    if (!IsGoodPreemptSuit(hand, bid_suit)) return false;
    if (bid_level == 2) return m.dist[bid_suit] == 6;
    if (bid_level == 3) return m.dist[bid_suit] >= 7;
  }

  return false;
}

bool WouldActOverOpener(uint64_t hand, const HandEvalMetrics &m,
                        int open_level, Suit open_suit, bool balancing) {
  if (QualifiesFor1NTOvercall(hand, m, open_level, open_suit, balancing)) {
    return true;
  }
  if (QualifiesForTakeoutDouble(hand, m, open_level, open_suit, balancing)) {
    return true;
  }
  static const char *kOvercalls[] = {"1D",  "1H", "1S", "2C", "2D", "2H",
                                     "2S", "2NT", "3C", "3D", "3H", "3S"};
  for (const char *c : kOvercalls) {
    if (QualifiesForOvercallCall(hand, m, open_level, open_suit, c,
                                 balancing)) {
      return true;
    }
  }
  return false;
}

bool QualifiesForResponderCall(uint64_t hand, const HandEvalMetrics &m,
                               int open_level, Suit open_suit,
                               const std::string &rho_call,
                               const std::string &call) {
  int fit_needed = (open_suit == SPADES || open_suit == HEARTS) ? 3 : 4;
  bool has_fit = (open_suit != NOTRUMP && m.dist[open_suit] >= fit_needed);
  int support_pts = m.hcp + (has_fit ? m.shortness_pts : m.length_pts);

  // Redouble over RHO's takeout double shows 10+ HCP.
  if (call == "XX") {
    return rho_call == "X" && m.hcp >= 10;
  }

  // Negative double over RHO's suit overcall.
  int rho_lvl = 0;
  Suit rho_suit = NOTRUMP;
  bool rho_overcalled =
      ParseContractCall(rho_call, &rho_lvl, &rho_suit) && rho_suit != NOTRUMP;
  if (call == "X" && rho_overcalled) {
    if (m.playing_pts < 6) return false;
    for (Suit maj : {SPADES, HEARTS}) {
      if (maj != open_suit && maj != rho_suit && m.dist[maj] >= 4) {
        return true;
      }
    }
    return false;
  }

  if (open_level == 1 && open_suit == NOTRUMP) {
    if (call == "P") {
      return m.hcp < 8 && m.dist[SPADES] < 5 && m.dist[HEARTS] < 5;
    }
    if (call == "2C") {
      return m.hcp >= 8 && (m.dist[SPADES] == 4 || m.dist[HEARTS] == 4);
    }
    if (call == "2D") return m.dist[HEARTS] >= 5;
    if (call == "2H") return m.dist[SPADES] >= 5;
    if (call == "2NT") {
      return m.hcp >= 8 && m.hcp <= 9 && m.dist[SPADES] < 4 &&
             m.dist[HEARTS] < 4;
    }
    if (call == "3NT") {
      return m.hcp >= 10 && m.hcp <= 15 && m.dist[SPADES] < 4 &&
             m.dist[HEARTS] < 4;
    }
    return false;
  }

  if (call == "P") {
    if (rho_call == "X") {
      return m.hcp < 10 && !has_fit && m.playing_pts < 6;
    }
    if (rho_overcalled) {
      return m.playing_pts < 8 && !has_fit;
    }
    return m.playing_pts < 6 && !has_fit;
  }

  int bid_lvl = 0;
  Suit bid_suit = NOTRUMP;
  if (!ParseContractCall(call, &bid_lvl, &bid_suit)) {
    return false;
  }

  if (open_level == 1 && (open_suit == SPADES || open_suit == HEARTS)) {
    if (bid_suit == open_suit) {
      if (bid_lvl == 2) return has_fit && support_pts >= 6 && support_pts <= 9;
      if (bid_lvl == 3) {
        return m.dist[open_suit] >= 4 && support_pts >= 10 &&
               support_pts <= 12;
      }
      if (bid_lvl == 4) {
        return m.dist[open_suit] >= 5 && m.hcp < 10 && support_pts >= 9;
      }
    }
    if (call == "2NT") return m.dist[open_suit] >= 4 && m.hcp >= 13;
    if (call == "1NT") {
      return !has_fit && m.playing_pts >= 6 && m.hcp <= 10;
    }
    if (bid_lvl == 1 && bid_suit == SPADES && open_suit == HEARTS) {
      return m.dist[SPADES] >= 4 && m.playing_pts >= 6;
    }
    if (bid_lvl == 2 && bid_suit != NOTRUMP && bid_suit != open_suit) {
      return m.dist[bid_suit] >= 4 && m.playing_pts >= 10;
    }
  }

  if (open_level == 1 && (open_suit == DIAMONDS || open_suit == CLUBS)) {
    if (bid_lvl == 1 && bid_suit != NOTRUMP &&
        SuitRankOrder(bid_suit) > SuitRankOrder(open_suit)) {
      return m.dist[bid_suit] >= 4 && m.playing_pts >= 6;
    }
    if (call == "1NT") {
      return m.hcp >= 6 && m.hcp <= 10 && m.dist[SPADES] < 4 &&
             m.dist[HEARTS] < 4;
    }
    if (bid_suit == open_suit) {
      if (bid_lvl == 2) return has_fit && support_pts >= 6 && support_pts <= 9;
      if (bid_lvl == 3) {
        return m.dist[open_suit] >= 5 && support_pts >= 10 &&
               support_pts <= 12;
      }
    }
    if (call == "2NT") return m.is_balanced && m.hcp >= 11 && m.hcp <= 12;
    if (call == "3NT") return m.is_balanced && m.hcp >= 13 && m.hcp <= 15;
  }

  return HasSuitStopper(hand, open_suit, m.dist[open_suit]) && m.hcp >= 10;
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
  std::string tok;
  for (size_t i = 0; i < clean_after.size(); ++i) {
    char c = clean_after[i];
    if (std::isspace(static_cast<unsigned char>(c)) || c == ',' || c == ';') {
      if (!tok.empty()) {
        prior_calls_.push_back(NormalizeCall(tok));
        tok.clear();
      }
    } else {
      tok += c;
    }
  }
  if (!tok.empty()) {
    prior_calls_.push_back(NormalizeCall(tok));
  }
  after_.clear();
  for (size_t i = 0; i < prior_calls_.size(); ++i) {
    if (i > 0) after_ += " ";
    after_ += prior_calls_[i];
  }
}

bool WouldBid::MeetsRequirement(uint64_t hand) const {
  HandEvalMetrics m = EvaluateHandForBidding(hand);

  int first_bid_idx = -1;
  for (size_t i = 0; i < prior_calls_.size(); ++i) {
    if (prior_calls_[i] != "P") {
      first_bid_idx = static_cast<int>(i);
      break;
    }
  }

  // Case 1: Auction is unopened up to this seat (0 to 3 passes).
  if (first_bid_idx < 0) {
    int seat_index = static_cast<int>(prior_calls_.size());
    if (bid_ == "P") {
      return !WouldOpenAnyCall(hand, m, seat_index);
    }
    return WouldOpenSpecificCall(hand, m, bid_, seat_index);
  }

  int open_level = 0;
  Suit open_suit = NOTRUMP;
  if (!ParseContractCall(prior_calls_[first_bid_idx], &open_level,
                         &open_suit)) {
    return true;
  }

  int calls_after_opener =
      static_cast<int>(prior_calls_.size()) - 1 - first_bid_idx;

  // Case 2A: Direct seat over opponent's opening bid (e.g. after="1S" or
  // after="P 1S" or after="P P 1S").
  if (calls_after_opener == 0) {
    if (bid_ == "P") {
      return !WouldActOverOpener(hand, m, open_level, open_suit,
                                 /*balancing=*/false);
    }
    return QualifiesForOvercallCall(hand, m, open_level, open_suit, bid_,
                                    /*balancing=*/false);
  }

  // Case 2B: Responder to partner's opening bid (e.g. after="1S P" or
  // after="1S X").
  if (calls_after_opener == 1) {
    const std::string &rho_call = prior_calls_[first_bid_idx + 1];
    return QualifiesForResponderCall(hand, m, open_level, open_suit, rho_call,
                                     bid_);
  }

  // Case 2C: 4th call in first round when LHO opened (calls_after_opener == 2,
  // e.g. "1S P P", "1S X P", "1S P 2S").
  if (calls_after_opener == 2) {
    const std::string &partner_call = prior_calls_[first_bid_idx + 1];
    const std::string &rho_call = prior_calls_[first_bid_idx + 2];
    if (partner_call == "P" && rho_call == "P") {
      // Balancing seat (1x - P - P - ?).
      if (bid_ == "P") {
        return !WouldActOverOpener(hand, m, open_level, open_suit,
                                   /*balancing=*/true);
      }
      return QualifiesForOvercallCall(hand, m, open_level, open_suit, bid_,
                                      /*balancing=*/true);
    }
    int rho_lvl = 0;
    Suit rho_suit = NOTRUMP;
    if (ParseContractCall(rho_call, &rho_lvl, &rho_suit)) {
      if (bid_ == "P") {
        return !WouldActOverOpener(hand, m, rho_lvl, rho_suit,
                                   /*balancing=*/false);
      }
      return QualifiesForOvercallCall(hand, m, rho_lvl, rho_suit, bid_,
                                      /*balancing=*/false);
    }
    if (partner_call == "X" && rho_call == "P") {
      // Advancing partner's takeout double (forced unless penalty pass).
      if (bid_ == "P") {
        return open_suit != NOTRUMP && m.dist[open_suit] >= 5 && m.hcp >= 8;
      }
      if (bid_ == "1NT") {
        return open_suit != NOTRUMP &&
               HasSuitStopper(hand, open_suit, m.dist[open_suit]) &&
               m.hcp >= 6 && m.hcp <= 10;
      }
      int adv_lvl = 0;
      Suit adv_suit = NOTRUMP;
      if (ParseContractCall(bid_, &adv_lvl, &adv_suit) &&
          adv_suit != NOTRUMP && adv_suit != open_suit) {
        return m.dist[adv_suit] >= 4;
      }
    }
  }

  return true;
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


