#include "interface_dds.h"

#include <algorithm>
#include <atomic>

#include "glog/logging.h"
#include "omp_compat.h"

namespace {
std::atomic<int> g_defender_noise_pct{10};
}  // namespace

void SetDefenderNoisePct(int pct) {
  g_defender_noise_pct.store(std::clamp(pct, 0, 100),
                             std::memory_order_relaxed);
}

int GetDefenderNoisePct() {
  return g_defender_noise_pct.load(std::memory_order_relaxed);
}

namespace {
class InitThreads {
 public:
  InitThreads() {
#if defined(__linux) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
    SetMaxThreads(0);
#endif
    DDSInfo info;
    GetDDSInfo(&info);
    max_threads_ = std::max(1, info.noOfThreads);
  }
  int max_threads() const { return max_threads_; }

 private:
  int max_threads_ = 1;
};

int DoInit() {
  static InitThreads thread_initter;
  return thread_initter.max_threads();
}
}  // namespace

int EnsureDDSInitialized() {
  return DoInit();
}

static bool CONSIDER_EQUALS = false;

void PopulatePBNDeal(const BoardState &bs, dealPBN *d1) {
  DoInit();
  VLOG(2) << "PopulatePBNDeal";
  d1->trump = bs.GetTrump();
  d1->first = bs.cur_trick_leader();
  for (int i = 0; i < 3; i++) {
    if (i < bs.CardsPlayedThisTrick()) {
      CardType c = bs.trick_cards_played()[i];
      auto [suit, val] = Card::DecodeCard(c);
      d1->currentTrickSuit[i] = suit;
      d1->currentTrickRank[i] = val + 2;
    } else {
      d1->currentTrickSuit[i] = 0;
      d1->currentTrickRank[i] = 0;
    }
  }

  VLOG(2) << "Copying cards " << bs.GetPBNCards();
  strcpy(d1->remainCards, bs.GetPBNCards().c_str());
}

namespace {
uint64_t GetBoardKey(const BoardState &bs) {
  std::string cards_played;
  for (int i = 0; i < bs.CardsPlayedThisTrick(); i++) {
    cards_played += Card::PrintCard(bs.trick_cards_played()[i]);
  }
  uint64_t board_key = std::hash<std::string>{}(
      bs.DebugString() + cards_played);
  return board_key;
}
}

std::vector<std::string> FindBestCardsToPlay(
    const BoardState &bs, int min_tricks) {
  dealPBN dl;
  PopulatePBNDeal(bs, &dl);

  int target = -1;  // Find optimal solution for side to play
  int solutions = 2; // Return all optimum cards and their scores.
  int mode = 1;  // automatic reuse of state / always search
  int threadindex = omp_get_thread_num();
  futureTricks futp;
  SolveBoardPBN(dl, target, solutions, mode, &futp, threadindex);
  std::vector<std::string> ans;
  CHECK_GT(futp.cards, 0);
  int result = futp.score[0];
  if (result < min_tricks) {
    return std::vector<std::string>();
  }

  for (int i = 0; i < futp.cards; i++) {
    std::string c = Card::PrintDDSCard((Suit)futp.suit[i], futp.rank[i]);
    ans.push_back(c);
  }
  return ans;
}

int FindOptimalTricks(const BoardState &bs) {
  dealPBN dl;
  PopulatePBNDeal(bs, &dl);

  int target = -1;  // Find optimal solution for side to play
  int solutions = 2; // Return all optimum cards and their scores.
  int mode = 1;  // automatic reuse of state / always search
  int threadindex = omp_get_thread_num();
  futureTricks futp;
  SolveBoardPBN(dl, target, solutions, mode, &futp, threadindex);
  std::vector<std::string> ans;
  if (futp.cards <= 0) {
    return 0;
  }
  int result = futp.score[0];
  return result;
}

namespace {

// Represents a chosen DDS equivalence group index `group_idx`
// (0 .. futp.cards - 1) and the specific DDS card rank `rank` (2..14) to play.
struct DefenderPlayChoice {
  int group_idx = 0;
  int rank = 0;
};

// Best lead candidate found in each suit (and overall) when a defender leads to
// a trick. Used both for the primary lead heuristic and for diversifying suit
// leads across multiple active suits when `noise_pct > 0`.
struct DefenderLeadEvaluation {
  DefenderPlayChoice best_overall;
  int overall_best_key = 10000;
  int best_key_per_suit[4] = {10000, 10000, 10000, 10000};
  int best_idx_per_suit[4] = {-1, -1, -1, -1};
  int best_rank_per_suit[4] = {0, 0, 0, 0};
};

// Deterministic 64-bit SplitMix64 finalizer for board-seeded hashes.
uint64_t Mix64(uint64_t z) {
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
  return z ^ (z >> 31);
}

// Deterministic hash for defender noise / suit-lead diversification on a
// specific sampled board (`bs.board_id() != 0`).
uint64_t DefenderNoiseHash(const BoardState &bs, int level, int num_cards) {
  uint64_t z = static_cast<uint64_t>(bs.board_id()) * 0xd1b54a32d192ed03ULL;
  z ^= bs.played_cards() + 0x9e3779b97f4a7c15ULL + (z << 6) + (z >> 2);
  z ^= static_cast<uint64_t>((bs.CurPlayer() << 8) | (level << 4) | num_cards);
  return Mix64(z);
}

// Returns the lowest DDS rank (2..14) in equivalence group `i`.
int LowestRankInGroup(const futureTricks &futp, int i) {
  unsigned int eq = static_cast<unsigned int>(futp.equals[i]) & ~3u;
  if (eq != 0) {
    return __builtin_ctz(eq);
  }
  return futp.rank[i];
}

// Returns the lowest DDS rank (2..14) in equivalence group `i` that is
// strictly greater than `rank_to_beat` (assuming futp.rank[i] > rank_to_beat).
int CheapestWinningRankInGroup(const futureTricks &futp, int i,
                               int rank_to_beat) {
  for (int r = std::max(2, rank_to_beat + 1); r < futp.rank[i]; ++r) {
    if ((futp.equals[i] >> r) & 1) {
      return r;
    }
  }
  return futp.rank[i];
}

// Returns the representative rank to lead from DDS equivalence group `idx`:
// the top of a touching honor sequence if the group's top card is part of a
// touching honor run (e.g. K from KQ, or Q from QJ10), or the lowest rank in
// the group otherwise (4th-best / low spot).
int LeadRankInGroup(const futureTricks &futp, int idx) {
  int low_r = LowestRankInGroup(futp, idx);
  int top_r = futp.rank[idx];
  bool touching_honors =
      top_r >= 10 && ((futp.equals[idx] >> (top_r - 1)) & 1) != 0;
  return touching_honors ? top_r : low_r;
}

// Picks the equivalence group whose lowest rank is minimal across all optimal
// groups in `futp`.
DefenderPlayChoice PickLowestGroupPlay(const futureTricks &futp) {
  int best_idx = 0;
  int lowest = LowestRankInGroup(futp, 0);
  for (int i = 1; i < futp.cards; ++i) {
    int r = LowestRankInGroup(futp, i);
    if (r < lowest) {
      lowest = r;
      best_idx = i;
    }
  }
  return {best_idx, lowest};
}

// If `base_r` is the bottom of a contiguous run of touching cards in DDS
// equivalence group `idx` and at least one card in that touching run is an
// honor (DDS rank >= 10, i.e., T, J, Q, K, A), picks uniformly at random from
// that touching run.
int RandomizeTouchingHonorRank(const BoardState &bs, const futureTricks &futp,
                               int idx, int base_r) {
  unsigned int mask =
      (static_cast<unsigned int>(futp.equals[idx]) & ~3u) |
      (1u << futp.rank[idx]);
  int top_touching = base_r;
  while (top_touching + 1 <= 14 && ((mask >> (top_touching + 1)) & 1) != 0) {
    ++top_touching;
  }
  if (top_touching > base_r && top_touching >= 10) {
    int count = top_touching - base_r + 1;
    if (bs.board_id() != 0) {
      uint64_t z =
          static_cast<uint64_t>(bs.board_id()) * 0x9e3779b97f4a7c15ULL;
      z ^= bs.played_cards() + 0x9e3779b97f4a7c15ULL + (z << 6) + (z >> 2);
      z ^= static_cast<uint64_t>((bs.CurPlayer() << 8) |
                                 (futp.suit[idx] << 4) |
                                 bs.CardsPlayedThisTrick());
      return base_r + static_cast<int>(Mix64(z) % static_cast<uint64_t>(count));
    }
    return base_r + static_cast<int>(lrand48() % count);
  }
  return base_r;
}

DefenderPlayChoice FinalizeDefenderChoice(const BoardState &bs,
                                          const futureTricks &futp,
                                          DefenderPlayChoice choice) {
  return {choice.group_idx,
          RandomizeTouchingHonorRank(bs, futp, choice.group_idx, choice.rank)};
}

// Returns the representative DDS rank (2..14) to play from equivalence group
// `idx` under the standard defender seat conventions.
int RankForDefenderGroup(const BoardState &bs, int level,
                         const futureTricks &futp, int idx) {
  Suit suit_led = bs.SuitLed();
  if (suit_led != NOTRUMP && futp.suit[0] != suit_led) {
    return LowestRankInGroup(futp, idx);
  }
  if (level == 0 || suit_led == NOTRUMP) {
    return LeadRankInGroup(futp, idx);
  }
  auto [win_offset, win_pair] = bs.TrickWinner();
  int cur_winner = (bs.cur_trick_leader() + win_offset) % 4;
  Suit cur_win_suit = win_pair.first;
  int cur_win_rank = win_pair.second + 2;
  int partner = bs.CurPlayer() ^ 2;
  bool trick_ruffed = (bs.GetTrump() != NOTRUMP &&
                       suit_led != bs.GetTrump() &&
                       cur_win_suit == bs.GetTrump());
  if (level >= 2 && !trick_ruffed && cur_winner != partner &&
      futp.rank[idx] > cur_win_rank) {
    int cw = CheapestWinningRankInGroup(futp, idx, cur_win_rank);
    if (level == 2) {
      return std::max(LowestRankInGroup(futp, idx), cw);
    }
    return cw;
  }
  return LowestRankInGroup(futp, idx);
}

// Scores a single DDS equivalence group `i` when a defender is on lead
// (`level == 0`). Lower keys are more natural bridge leads:
//   tier   0: touching honors when N/S do not hold a tenace
//   tier  50: passive spot lead when N/S do not hold a tenace
//   tier 150: touching honors into an N/S tenace
//   tier 250: spot card into an N/S tenace
//   tier 350: unsupported honor into an N/S tenace (top higher than N/S)
//   tier 400: unsupported honor under a higher N/S honor/tenace
int ScoreDefenderLeadGroup(const futureTricks &futp, int i, int low_r,
                           uint64_t def_cards, uint64_t ew_cards,
                           uint64_t ns_cards) {
  int s = futp.suit[i];
  int top_r = futp.rank[i];
  uint16_t def_suit = (def_cards >> (s * 16)) & 0xffff;
  uint16_t ew_suit = (ew_cards >> (s * 16)) & 0xffff;
  uint16_t ns_suit = (ns_cards >> (s * 16)) & 0xffff;

  // Check if top_r and top_r - 1 are touching honors in the group OR in the
  // defender's hand (when partner holds the card between two DDS equivalence
  // groups or when only one is in futp.equals).
  bool hand_touching_honors =
      top_r >= 10 &&
      (((futp.equals[i] >> (top_r - 1)) & 1) != 0 ||
       (top_r >= 3 && ((def_suit >> (top_r - 3)) & 1) != 0));
  bool ns_has_higher_than_top =
      top_r < 14 && (ns_suit >> (top_r - 1)) != 0;

  // Check whether N/S have a guarded honor/tenace in suit `s` that could
  // benefit from a defender lead: N/S hold >=2 cards with an honor and
  // E/W still hold a card higher than N/S's lowest honor (e.g. N/S have
  // AKJ missing Q, or K9 missing AQJ), as opposed to N/S holding only
  // solid top honors (e.g. CAK2 after CQ won, or DA5 after DK won).
  int ew_top_bit = ew_suit != 0 ? 31 - __builtin_clz(ew_suit) : -1;
  uint16_t ns_honors = ns_suit >> 9;
  int ns_low_honor_bit =
      ns_honors != 0 ? __builtin_ctz(ns_suit & ~0x1ff) : -1;
  bool ns_has_tenace =
      ns_honors != 0 && __builtin_popcount(ns_suit) >= 2 &&
      ew_top_bit > ns_low_honor_bit;

  int def_highest_bit = def_suit != 0 ? 31 - __builtin_clz(def_suit) : -1;
  int def_highest_r = def_highest_bit + 2;
  bool unsupported_honor_into_tenace =
      !hand_touching_honors && ns_has_tenace &&
      (low_r >= 11 || def_highest_r >= 11);
  bool unsupported_honor_under_ns =
      !hand_touching_honors && low_r >= 11 && ns_has_higher_than_top &&
      ns_has_tenace;

  int tier = 50;
  if (unsupported_honor_into_tenace || unsupported_honor_under_ns) {
    tier = ns_has_higher_than_top ? 400 : 350;
  } else if (ns_has_tenace) {
    tier = hand_touching_honors ? 150 : 250;
  } else if (hand_touching_honors) {
    tier = 0;
  }
  return tier + (hand_touching_honors ? (15 - top_r) : low_r);
}

// Evaluates all DDS-optimal lead candidates when a defender is on lead.
DefenderLeadEvaluation EvaluateDefenderLead(const BoardState &bs,
                                            const futureTricks &futp) {
  DefenderLeadEvaluation eval;
  uint64_t mask = ~bs.played_cards();
  uint64_t def_cards = bs.GetHand(bs.CurPlayer()) & mask;
  uint64_t ew_cards = (bs.GetHand(1) | bs.GetHand(3)) & mask;
  uint64_t ns_cards = (bs.GetHand(0) | bs.GetHand(2)) & mask;

  eval.best_overall = {0, LowestRankInGroup(futp, 0)};
  int best_key = 10000;
  for (int i = 0; i < futp.cards; ++i) {
    int s = futp.suit[i];
    int low_r = LowestRankInGroup(futp, i);
    int play_r = LeadRankInGroup(futp, i);
    int key = ScoreDefenderLeadGroup(futp, i, low_r, def_cards, ew_cards,
                                     ns_cards);
    if (s >= 0 && s < 4 && key < eval.best_key_per_suit[s]) {
      eval.best_key_per_suit[s] = key;
      eval.best_idx_per_suit[s] = i;
      eval.best_rank_per_suit[s] = play_r;
    }
    if (i == 0 || key < best_key) {
      best_key = key;
      eval.best_overall = {i, play_r};
    }
  }
  eval.overall_best_key = best_key;
  return eval;
}

// 3rd-hand following-suit heuristic: play high from the highest winning group
// unless partner's honor (>= J) is already winning and we can duck underneath.
DefenderPlayChoice ChooseThirdHandFollow(const BoardState &bs,
                                         const futureTricks &futp,
                                         int cur_winner, int partner,
                                         int cur_win_rank) {
  if (cur_winner == partner && cur_win_rank >= 11) {
    DefenderPlayChoice low = FinalizeDefenderChoice(
        bs, futp, PickLowestGroupPlay(futp));
    if (low.rank < cur_win_rank) {
      return low;
    }
  }
  int best_idx = -1;
  int highest_top = -1;
  for (int i = 0; i < futp.cards; ++i) {
    if (futp.rank[i] > cur_win_rank && futp.rank[i] > highest_top) {
      highest_top = futp.rank[i];
      best_idx = i;
    }
  }
  if (best_idx >= 0) {
    int base_r =
        std::max(LowestRankInGroup(futp, best_idx),
                 CheapestWinningRankInGroup(futp, best_idx, cur_win_rank));
    return FinalizeDefenderChoice(bs, futp, {best_idx, base_r});
  }
  // Cannot beat the current winning card; play 3rd hand low.
  return FinalizeDefenderChoice(bs, futp, PickLowestGroupPlay(futp));
}

// 4th-hand following-suit heuristic: do not overtake partner's winning card;
// otherwise win as cheaply as possible, or duck with the lowest card.
DefenderPlayChoice ChooseFourthHandFollow(const BoardState &bs,
                                          const futureTricks &futp,
                                          int cur_winner, int partner,
                                          int cur_win_rank) {
  if (cur_winner == partner) {
    return FinalizeDefenderChoice(bs, futp, PickLowestGroupPlay(futp));
  }
  int best_win_idx = -1;
  int cheapest_win_rank = 100;
  for (int i = 0; i < futp.cards; ++i) {
    if (futp.rank[i] > cur_win_rank) {
      int r = CheapestWinningRankInGroup(futp, i, cur_win_rank);
      if (r < cheapest_win_rank) {
        cheapest_win_rank = r;
        best_win_idx = i;
      }
    }
  }
  if (best_win_idx >= 0) {
    return FinalizeDefenderChoice(bs, futp, {best_win_idx, cheapest_win_rank});
  }
  return FinalizeDefenderChoice(bs, futp, PickLowestGroupPlay(futp));
}

// Following suit in 2nd, 3rd, or 4th hand (`level` 1..3).
DefenderPlayChoice ChooseFollowingSuitPlay(const BoardState &bs, int level,
                                           const futureTricks &futp) {
  Suit suit_led = bs.SuitLed();
  auto [win_offset, win_pair] = bs.TrickWinner();
  int cur_winner = (bs.cur_trick_leader() + win_offset) % 4;
  Suit cur_win_suit = win_pair.first;
  int cur_win_rank = win_pair.second + 2;  // Convert 0..12 to DDS rank 2..14.
  int partner = bs.CurPlayer() ^ 2;
  bool trick_ruffed = (bs.GetTrump() != NOTRUMP &&
                       suit_led != bs.GetTrump() &&
                       cur_win_suit == bs.GetTrump());

  if (level == 1 || trick_ruffed) {
    // 2nd hand low (or trick already ruffed so following suit cannot win).
    return FinalizeDefenderChoice(bs, futp, PickLowestGroupPlay(futp));
  }
  if (level == 2) {
    return ChooseThirdHandFollow(bs, futp, cur_winner, partner, cur_win_rank);
  }
  return ChooseFourthHandFollow(bs, futp, cur_winner, partner, cur_win_rank);
}

// Applies optional exploration noise (`noise_pct > 0`):
// 1. On lead (`level == 0`), if multiple suits have reasonable non-tenace-gift
//    leads (`key < 350` and within 150 of `overall_best_key`), distributes the
//    opening lead across those active suits so Step 1 explores all reasonable
//    suit shifts.
// 2. Otherwise, with probability `noise_pct`%, switches to a random alternative
//    DDS-optimal equivalence group.
DefenderPlayChoice ApplyDefenderNoise(const BoardState &bs, int level,
                                      const futureTricks &futp, int noise_pct,
                                      const DefenderLeadEvaluation *lead_eval,
                                      DefenderPlayChoice heur_choice) {
  if (noise_pct <= 0 || futp.cards <= 1) {
    return heur_choice;
  }
  if (level == 0 && lead_eval != nullptr) {
    std::vector<int> active_suits;
    for (int s = 0; s < 4; ++s) {
      if (lead_eval->best_idx_per_suit[s] >= 0 &&
          lead_eval->best_key_per_suit[s] < 350 &&
          lead_eval->best_key_per_suit[s] <=
              lead_eval->overall_best_key + 150) {
        active_suits.push_back(s);
      }
    }
    if (active_suits.size() > 1) {
      uint64_t n_suits = static_cast<uint64_t>(active_suits.size());
      uint64_t pick =
          (bs.board_id() != 0)
              ? (DefenderNoiseHash(bs, level, futp.cards) % n_suits)
              : (static_cast<uint64_t>(lrand48()) % n_suits);
      int s = active_suits[pick];
      return FinalizeDefenderChoice(
          bs, futp,
          {lead_eval->best_idx_per_suit[s], lead_eval->best_rank_per_suit[s]});
    }
  }

  uint64_t roll = 0;
  uint64_t alt_roll = 0;
  uint64_t num_alts = static_cast<uint64_t>(futp.cards - 1);
  if (bs.board_id() != 0) {
    uint64_t z = DefenderNoiseHash(bs, level, futp.cards);
    roll = z % 100ULL;
    alt_roll = (z / 100ULL) % num_alts;
  } else {
    roll = static_cast<uint64_t>(lrand48() % 100);
    alt_roll = static_cast<uint64_t>(lrand48()) % num_alts;
  }
  if (static_cast<int>(roll) < noise_pct) {
    int idx = static_cast<int>(alt_roll);
    if (idx >= heur_choice.group_idx) ++idx;
    return FinalizeDefenderChoice(
        bs, futp, {idx, RankForDefenderGroup(bs, level, futp, idx)});
  }
  return heur_choice;
}

// Chooses which DDS equivalence group index `i` (0 .. futp.cards - 1) and
// which specific DDS rank (2..14) within that group a defender should play.
std::pair<int, int> ChooseDefenderPlay(const BoardState &bs, int level,
                                       const futureTricks &futp,
                                       int noise_pct = 0) {
  Suit suit_led = bs.SuitLed();
  DefenderLeadEvaluation lead_eval;
  const DefenderLeadEvaluation *lead_eval_ptr = nullptr;
  DefenderPlayChoice heur_choice;

  if (suit_led != NOTRUMP && futp.suit[0] != suit_led) {
    // 1. Not following suit (discarding or ruffing when void in suit_led).
    int idx = bs.FindBestDiscard(futp.cards, const_cast<int *>(futp.suit),
                                 const_cast<int *>(futp.rank));
    heur_choice = FinalizeDefenderChoice(
        bs, futp, {idx, LowestRankInGroup(futp, idx)});
  } else if (level == 0 || suit_led == NOTRUMP) {
    // 2. Defender on lead (1st hand, level == 0).
    lead_eval = EvaluateDefenderLead(bs, futp);
    lead_eval_ptr = &lead_eval;
    heur_choice = FinalizeDefenderChoice(bs, futp, lead_eval.best_overall);
  } else {
    // 3. Following suit in 2nd, 3rd, or 4th hand.
    heur_choice = ChooseFollowingSuitPlay(bs, level, futp);
  }

  DefenderPlayChoice final_choice = ApplyDefenderNoise(
      bs, level, futp, noise_pct, lead_eval_ptr, heur_choice);
  return {final_choice.group_idx, final_choice.rank};
}

}  // namespace

std::string FindBestDefenderCard(const BoardState &bs, int noise_pct) {
  DoInit();
  std::vector<CardType> valid = bs.ValidCardsToPlay();
  if (valid.size() == 1) {
    return Card::PrintCard(valid[0]);
  }
  dealPBN dl;
  PopulatePBNDeal(bs, &dl);
  futureTricks futp{};
  int target = -1;
  int solutions = 2;
  int mode = 1;
  int threadindex = omp_get_thread_num();
  int rc = SolveBoardPBN(dl, target, solutions, mode, &futp, threadindex);
  if (rc != RETURN_NO_FAULT || futp.cards <= 0) {
    return valid.empty() ? "" : Card::PrintCard(valid[0]);
  }
  int level = bs.CardsPlayedThisTrick();
  auto [card_to_play, defender_rank_to_play] =
      ChooseDefenderPlay(bs, level, futp, noise_pct);
  return Card::PrintDDSCard(static_cast<Suit>(futp.suit[card_to_play]),
                            defender_rank_to_play);
}

std::vector<std::string> FindOptimalDefenderCards(const BoardState &bs) {
  DoInit();
  std::vector<CardType> valid = bs.ValidCardsToPlay();
  if (valid.empty()) {
    return {};
  }
  if (valid.size() == 1) {
    return {Card::PrintCard(valid[0])};
  }
  dealPBN dl;
  PopulatePBNDeal(bs, &dl);
  futureTricks futp{};
  int target = -1;
  int solutions = 2;
  int mode = 1;
  int threadindex = omp_get_thread_num();
  int rc = SolveBoardPBN(dl, target, solutions, mode, &futp, threadindex);
  if (rc != RETURN_NO_FAULT || futp.cards <= 0) {
    return {Card::PrintCard(valid[0])};
  }
  int level = bs.CardsPlayedThisTrick();
  auto [heur_idx, heur_rank] =
      ChooseDefenderPlay(bs, level, futp, /*noise_pct=*/0);
  std::vector<std::string> ans;
  ans.reserve(futp.cards);
  ans.push_back(Card::PrintDDSCard(static_cast<Suit>(futp.suit[heur_idx]),
                                   heur_rank));
  for (int i = 0; i < futp.cards; ++i) {
    if (i == heur_idx) continue;
    int base_r = RankForDefenderGroup(bs, level, futp, i);
    int r = RandomizeTouchingHonorRank(bs, futp, i, base_r);
    ans.push_back(Card::PrintDDSCard(static_cast<Suit>(futp.suit[i]), r));
  }
  return ans;
}

struct PartnerLeadInfo {
  int lowest_lead_rank = 0;
  int cheapest_master_lead_rank = 0;
  uint16_t opp_suit_mask = 0;
};

bool CanNeedUnblockForPartner(uint16_t my_sm, uint16_t p_sm, int p_after,
                              int cheapest_r, int r) {
  if (p_after <= 0 || p_sm == 0 || r <= cheapest_r) {
    return false;
  }
  uint16_t lower_mask =
      (r > 2) ? static_cast<uint16_t>((1u << (r - 2)) - 1) : 0;
  if ((p_sm & lower_mask) == 0) {
    return false;
  }
  uint16_t my_rem_after =
      my_sm & static_cast<uint16_t>(~(1u << (cheapest_r - 2)));
  int my_after = __builtin_popcount(my_rem_after);
  if (my_after == 0 || my_after >= p_after) {
    return false;
  }
  int my_lowest_after = __builtin_ctz(my_rem_after) + 2;
  int p_highest = 31 - __builtin_clz(p_sm) + 2;
  return my_lowest_after > p_highest;
}

uint16_t RanksBetweenMask(int r1, int r2) {
  if (r2 <= r1 + 1) {
    return 0;
  }
  uint16_t below_r2 = static_cast<uint16_t>((1u << (r2 - 2)) - 1);
  uint16_t at_or_below_r1 = static_cast<uint16_t>((1u << (r1 - 1)) - 1);
  return below_r2 & ~at_or_below_r1;
}

int DDSSolveHelper(int min_tricks, const BoardState &bs, dealPBN &dl,
                   int level, OptimalLines *lines,
                   SolverCache *cache,
                   futureTricks *solved_tricks = nullptr,
                   PartnerLeadInfo partner_lead = {}) {
  DoInit();
  VLOG(2) << "DDSSolveHelper level " << level;
  CHECK_LT(level, 4);
  bool use_cache = false; //  cache != nullptr && level == 0;
  if (use_cache) {
    uint64_t board_key = GetBoardKey(bs);
    std::unordered_map<uint64_t, OptimalLines> &dds_lines =
        cache->dds_lines[bs.CurPlayer()];
    auto it = dds_lines.find(board_key);
    if (it != dds_lines.end()) {
      lines->insert(it->second.begin(), it->second.end());
      return cache->dds_results[bs.CurPlayer()][board_key];
    }
  }
  bool pair_is_me = (bs.CurPlayer() % 2 == 0);
  bool defense = !pair_is_me;
  uint64_t cur_hand = bs.GetHand(bs.CurPlayer()) & ~bs.played_cards();
  int total_tricks = __builtin_popcountll(cur_hand);
  if (min_tricks > total_tricks) {
    return 0;
  }
  // For declarer, ask DDS for every card that still makes min_tricks rather
  // than only the cards that maximize tricks. The search's objective is to
  // make the contract on as many boards as possible, so a play that makes
  // exactly min_tricks here must stay a candidate even when a peek-dependent
  // play (dropping an offside doubleton queen, say) would win an overtrick on
  // this particular board. Once the contract is secure (min_tricks <= 0) the
  // maximal plays suffice and keep the tree small.
  bool use_min_tricks_target = pair_is_me && min_tricks >= 1;
  futureTricks futp_storage{};
  if (!solved_tricks) {
    int target = use_min_tricks_target ? min_tricks : -1;
    int solutions = 2; // Return all optimum cards and their scores.
    // int mode = 1;  // automatic reuse of state / always search
    int mode = 2;  // force reuse of state
    int threadindex = omp_get_thread_num();
    int rc = SolveBoardPBN(dl, target, solutions, mode, &futp_storage,
                           threadindex);
    if (rc != RETURN_NO_FAULT) {
      return -1;
    }
  }
  futureTricks &futp = solved_tricks ? *solved_tricks : futp_storage;
  if (futp.cards <= 0) {
    return -1;
  }

  int result = futp.score[0];
  if (min_tricks >= 1) {
    if (defense && total_tricks - result < min_tricks) {
      return result;
    }
    if (pair_is_me && result < min_tricks) {
      return result;
    }
  }
  int card_to_play = -1;
  int defender_rank_to_play = -1;
  if (defense) {
    std::tie(card_to_play, defender_rank_to_play) =
        ChooseDefenderPlay(bs, level, futp, GetDefenderNoisePct());
    CHECK_GE(card_to_play, 0) << futp.cards;
  }
  int lowest_discard_rank[4] = {100, 100, 100, 100};
  int lowest_losing_follow_rank = 100;
  int cheapest_third_hand_win_rank[4] = {100, 100, 100, 100};
  int cheapest_fourth_hand_win_rank[4] = {100, 100, 100, 100};
  int lowest_lead_rank[4] = {100, 100, 100, 100};
  int cheapest_master_lead_rank[4] = {100, 100, 100, 100};
  int cur_winner = -1;
  int cur_win_rank = 0;
  Suit cur_win_suit = NOTRUMP;
  bool trick_ruffed = false;
  bool has_discard_in_futp = false;
  uint16_t my_suit_mask[4] = {0, 0, 0, 0};
  uint16_t partner_suit_mask[4] = {0, 0, 0, 0};
  uint16_t east_suit_mask[4] = {0, 0, 0, 0};
  uint16_t west_suit_mask[4] = {0, 0, 0, 0};
  uint16_t opp_suit_mask[4] = {0, 0, 0, 0};
  int opp_highest_rank[4] = {0, 0, 0, 0};
  int partner_after_suit[4] = {0, 0, 0, 0};
  uint16_t rho_led_mask = 0;
  if (pair_is_me) {
    Suit suit_led = bs.SuitLed();
    Suit trump = bs.GetTrump();
    uint64_t played = bs.played_cards();
    for (int k = 0; k < level; ++k) {
      played |= bs.trick_cards_played()[k];
    }
    int me = bs.CurPlayer();
    int partner = me ^ 2;
    uint64_t my_rem = bs.GetHand(me) & ~played;
    uint64_t partner_rem = bs.GetHand(partner) & ~played;
    uint64_t east_rem = bs.GetHand(1) & ~played;
    uint64_t west_rem = bs.GetHand(3) & ~played;
    for (int s = 0; s < 4; ++s) {
      my_suit_mask[s] = static_cast<uint16_t>((my_rem >> (s * 16)) & 0xffff);
      partner_suit_mask[s] =
          static_cast<uint16_t>((partner_rem >> (s * 16)) & 0xffff);
      east_suit_mask[s] =
          static_cast<uint16_t>((east_rem >> (s * 16)) & 0xffff);
      west_suit_mask[s] =
          static_cast<uint16_t>((west_rem >> (s * 16)) & 0xffff);
      opp_suit_mask[s] = east_suit_mask[s] | west_suit_mask[s];
      opp_highest_rank[s] =
          (opp_suit_mask[s] != 0) ? (31 - __builtin_clz(opp_suit_mask[s]) + 2)
                                  : 0;
      int p_after = __builtin_popcount(partner_suit_mask[s]);
      if ((level == 0 || (level == 1 && s == suit_led)) && p_after > 0) {
        --p_after;
      }
      partner_after_suit[s] = p_after;
    }
    if (level == 0) {
      for (int i = 0; i < futp.cards; ++i) {
        int s = futp.suit[i];
        int r = futp.rank[i];
        lowest_lead_rank[s] = std::min(lowest_lead_rank[s], r);
        if (r > opp_highest_rank[s]) {
          cheapest_master_lead_rank[s] =
              std::min(cheapest_master_lead_rank[s], r);
        }
      }
    } else if (suit_led != NOTRUMP) {
      auto [win_offset, win_pair] = bs.TrickWinner();
      cur_winner = (bs.cur_trick_leader() + win_offset) % 4;
      cur_win_suit = win_pair.first;
      cur_win_rank = win_pair.second + 2;
      trick_ruffed =
          (trump != NOTRUMP && suit_led != trump && cur_win_suit == trump);
      int rho = (me + 1) % 4;
      rho_led_mask =
          (rho == 1) ? east_suit_mask[suit_led] : west_suit_mask[suit_led];
      for (int i = 0; i < futp.cards; i++) {
        int s = futp.suit[i];
        int r = futp.rank[i];
        if (s != suit_led && s != trump) {
          lowest_discard_rank[s] = std::min(lowest_discard_rank[s], r);
          has_discard_in_futp = true;
        } else if (s == suit_led && (trick_ruffed || r < cur_win_rank)) {
          lowest_losing_follow_rank = std::min(lowest_losing_follow_rank, r);
        }
        bool wins_so_far =
            (s == suit_led && !trick_ruffed && r > cur_win_rank) ||
            (s == trump && (cur_win_suit != trump || r > cur_win_rank));
        if (level == 2 && wins_so_far) {
          bool sure_third_win =
              (s == suit_led && r > opp_highest_rank[suit_led]) ||
              (s == trump && suit_led != trump &&
               (rho_led_mask != 0 || r > opp_highest_rank[trump]));
          if (sure_third_win) {
            cheapest_third_hand_win_rank[s] =
                std::min(cheapest_third_hand_win_rank[s], r);
          }
        }
        if (level == 3 && wins_so_far) {
          cheapest_fourth_hand_win_rank[s] =
              std::min(cheapest_fourth_hand_win_rank[s], r);
        }
      }
    }
  }
  OptimalLines my_lines;
  for (int i = 0; i < futp.cards; i++) {
    if (!pair_is_me && i != card_to_play) {
      continue;
    }
    if (pair_is_me && level == 0) {
      int s = futp.suit[i];
      int r = futp.rank[i];
      if (r < 10 && r > lowest_lead_rank[s] && lowest_lead_rank[s] < 10 &&
          partner_suit_mask[s] != 0 &&
          (__builtin_ctz(partner_suit_mask[s]) + 2) > r) {
        continue;
      }
      bool pruned_lead = false;
      for (int j = 0; j < futp.cards; ++j) {
        if (futp.suit[j] != s || futp.rank[j] >= r) continue;
        int r1 = futp.rank[j];
        if ((r1 >= 10) != (r >= 10)) continue;
        if ((opp_suit_mask[s] & RanksBetweenMask(r1, r)) != 0) continue;
        uint16_t below_r1 =
            (r1 > 2) ? static_cast<uint16_t>((1u << (r1 - 2)) - 1) : 0;
        if ((partner_suit_mask[s] & below_r1) == 0) continue;
        if (!CanNeedUnblockForPartner(my_suit_mask[s], partner_suit_mask[s],
                                      partner_after_suit[s], r1, r)) {
          pruned_lead = true;
          break;
        }
      }
      if (pruned_lead) {
        continue;
      }
    }
    if (pair_is_me && bs.SuitLed() != NOTRUMP) {
      Suit suit_led = bs.SuitLed();
      Suit trump = bs.GetTrump();
      int me = bs.CurPlayer();
      int partner = me ^ 2;
      int s = futp.suit[i];
      int r = futp.rank[i];
      int lead_rank =
          (level >= 1)
              ? (Card::DecodeCard(bs.trick_cards_played()[0]).second + 2)
              : 0;
      if (s != suit_led && s != trump && r > lowest_discard_rank[s]) {
        continue;
      }
      if (s == suit_led && (trick_ruffed || r < cur_win_rank) &&
          r > lowest_losing_follow_rank &&
          !(level == 2 && !trick_ruffed && r >= 10 && r > lead_rank)) {
        uint16_t higher_mask =
            static_cast<uint16_t>(~((1u << (r - 1)) - 1) & 0x1fff);
        int max_opp_higher =
            std::max(__builtin_popcount(east_suit_mask[s] & higher_mask),
                     __builtin_popcount(west_suit_mask[s] & higher_mask));
        if (max_opp_higher >= partner_after_suit[s] ||
            !CanNeedUnblockForPartner(my_suit_mask[s], partner_suit_mask[s],
                                      partner_after_suit[s],
                                      lowest_losing_follow_rank, r)) {
          continue;
        }
      }
      bool wins_so_far =
          (s == suit_led && !trick_ruffed && r > cur_win_rank) ||
          (s == trump && (cur_win_suit != trump || r > cur_win_rank));
      if (level == 2) {
        if (partner_lead.lowest_lead_rank > 0 &&
            lead_rank > partner_lead.lowest_lead_rank) {
          if (s == trump && suit_led != trump) {
            continue;
          }
          if (s != suit_led && s != trump &&
              (trick_ruffed || cur_win_rank > lead_rank)) {
            continue;
          }
          if (s == suit_led && wins_so_far && r > lead_rank) {
            uint16_t p_sm_before =
                partner_suit_mask[s] |
                static_cast<uint16_t>(1u << (lead_rank - 2));
            uint16_t my_rem_after =
                my_suit_mask[s] & static_cast<uint16_t>(~(1u << (r - 2)));
            if ((partner_lead.opp_suit_mask &
                 RanksBetweenMask(partner_lead.lowest_lead_rank, lead_rank)) ==
                    0 &&
                !CanNeedUnblockForPartner(p_sm_before, my_rem_after,
                                          __builtin_popcount(my_rem_after),
                                          partner_lead.lowest_lead_rank,
                                          lead_rank)) {
              continue;
            }
          }
        }
        if (s == suit_led && !trick_ruffed && cur_winner == partner &&
            r < lead_rank && partner_lead.cheapest_master_lead_rank > 0 &&
            lead_rank > partner_lead.cheapest_master_lead_rank &&
            (partner_lead.cheapest_master_lead_rank >= 10) ==
                (lead_rank >= 10)) {
          uint16_t p_sm_before =
              partner_suit_mask[s] |
              static_cast<uint16_t>(1u << (lead_rank - 2));
          uint16_t my_rem_after =
              my_suit_mask[s] & static_cast<uint16_t>(~(1u << (r - 2)));
          if (!CanNeedUnblockForPartner(
                  p_sm_before, my_rem_after, __builtin_popcount(my_rem_after),
                  partner_lead.cheapest_master_lead_rank, lead_rank)) {
            continue;
          }
        }
        bool sure_third_win =
            wins_so_far &&
            ((s == suit_led && r > opp_highest_rank[suit_led]) ||
             (s == trump && suit_led != trump &&
              (rho_led_mask != 0 || r > opp_highest_rank[trump])));
        if (sure_third_win && r > cheapest_third_hand_win_rank[s] &&
            !CanNeedUnblockForPartner(my_suit_mask[s], partner_suit_mask[s],
                                      partner_after_suit[s],
                                      cheapest_third_hand_win_rank[s], r)) {
          continue;
        }
      }
      if (level == 3) {
        if (cur_winner == partner && !trick_ruffed && cur_win_rank == 14 &&
            s == trump && suit_led != trump && has_discard_in_futp) {
          continue;
        }
        if (wins_so_far && r > cheapest_fourth_hand_win_rank[s] &&
            !CanNeedUnblockForPartner(my_suit_mask[s], partner_suit_mask[s],
                                      partner_after_suit[s],
                                      cheapest_fourth_hand_win_rank[s], r)) {
          continue;
        }
      }
      if (wins_so_far) {
        bool pruned_equiv = false;
        for (int j = 0; j < futp.cards; ++j) {
          if (futp.suit[j] != s || futp.rank[j] >= r) continue;
          int r1 = futp.rank[j];
          bool j_wins =
              (s == suit_led && !trick_ruffed && r1 > cur_win_rank) ||
              (s == trump && (cur_win_suit != trump || r1 > cur_win_rank));
          if (!j_wins) continue;
          if ((opp_suit_mask[s] & RanksBetweenMask(r1, r)) != 0) continue;
          if (!CanNeedUnblockForPartner(my_suit_mask[s], partner_suit_mask[s],
                                        partner_after_suit[s], r1, r)) {
            pruned_equiv = true;
            break;
          }
        }
        if (pruned_equiv) {
          continue;
        }
      }
    }
    int rank_to_play = defense ? defender_rank_to_play : futp.rank[i];
    std::string c = Card::PrintDDSCard((Suit)futp.suit[i], rank_to_play);
    VLOG(3) << "At level " << level << " trying card " << c
            << " " << i << " / " << futp.cards
            << " with suit led " << SuitDebugString(bs.SuitLed());
    OptimalLines rest;
    if (level < 3) {
      BoardState copy(bs);
      CardType c_val = Card::ParseStringCard(c);
      copy.PlayCard(copy.CurPlayer(), c_val);

      dl.currentTrickSuit[level] = futp.suit[i];
      dl.currentTrickRank[level] = rank_to_play;
      strcpy(dl.remainCards, copy.GetPBNCards().c_str());
      PartnerLeadInfo next_lead =
          (level == 0 && pair_is_me)
              ? PartnerLeadInfo{lowest_lead_rank[futp.suit[i]],
                                cheapest_master_lead_rank[futp.suit[i]],
                                opp_suit_mask[futp.suit[i]]}
              : partner_lead;
      DDSSolveHelper(min_tricks, copy, dl, level + 1, &rest, cache, nullptr,
                     next_lead);
    } else {
      rest.insert("");
    }
    for (const std::string &line : rest) {
      VLOG(3) << "At level " << level << " adding card " << c
              << " " << i << " / " << futp.cards
              << " to make " << (c + " " + line)
              << " (" << my_lines.size() << ")";
      int others = futp.equals[i];
      my_lines.insert(c + " " + line);
      if (defense) {
        continue;
      }
      int rank = 0;
      while ((defense || CONSIDER_EQUALS) && others > 0) {
        rank++;
        others = others >> 1;
        if (others & 1) {
          my_lines.insert(
              Card::PrintDDSCard((Suit)futp.suit[i], rank) + " " + line);
          VLOG(3) << "Found equiv card "
                  << Card::PrintDDSCard((Suit)futp.suit[i], rank);
        }
      }
    }
  }
  lines->insert(my_lines.begin(), my_lines.end());
  if (use_cache) {
    uint64_t board_key = GetBoardKey(bs);
    cache->dds_lines[bs.CurPlayer()][board_key] = my_lines;
    cache->dds_results[bs.CurPlayer()][board_key] = result;
  }
  return result;
}

int DDSFindOptimalLines(const BoardState &bs, int min_tricks,
                        OptimalLines *lines, SolverCache *cache) {
  DoInit();
  VLOG(2) << "DDSFindOptimalLines";
  CHECK(lines);
  lines->clear();

  dealPBN dl;
  PopulatePBNDeal(bs, &dl);

  int played = bs.CardsPlayedThisTrick();
  OptimalLines raw_lines;
  int res = DDSSolveHelper(min_tricks, bs, dl, played,
                           played > 0 ? &raw_lines : lines, cache);
  if (played > 0) {
    std::string prefix;
    for (int i = 0; i < played; ++i) {
      prefix += Card::PrintCard(bs.trick_cards_played()[i]) + " ";
    }
    for (const std::string &suffix : raw_lines) {
      lines->insert(prefix + suffix);
    }
  }
  return res;
}

std::shared_ptr<OptimalPlayTree> FindOptimalPlayTreeHelper(
    const BoardState &bs, int min_tricks, int tricks_remaining,
    int deepen_levels,
    SolverCache *cache) {
  if (tricks_remaining == 0) {
    std::shared_ptr<OptimalPlayTree> ans(
        new OptimalPlayTree(0, bs, tricks_remaining, min_tricks));
    return ans;
  }
  BoardCacheKey key{};
  size_t shard = 0;
  bool use_tree_cache = (cache != nullptr && bs.CardsPlayedThisTrick() == 0);
  if (use_tree_cache) {
    uint64_t mask = ~bs.played_cards();
    uint64_t h1 = bs.GetHand(1) & mask;
    uint64_t h3 = bs.GetHand(3) & mask;
    constexpr uint64_t kTouchingHonorMask = 0x0f800f800f800f80ULL;
    bool has_touching_honors =
        (((h1 & (h1 >> 1)) | (h3 & (h3 >> 1))) & kTouchingHonorMask) != 0;
    bool need_board_id = has_touching_honors || GetDefenderNoisePct() > 0;
    key = BoardCacheKey{
        bs.GetHand(0) & mask,
        h1,
        bs.GetHand(2) & mask,
        h3,
        static_cast<uint8_t>(bs.CurPlayer()),
        static_cast<uint8_t>(bs.GetTrump()),
        static_cast<uint8_t>(std::max(0, min_tricks)),
        need_board_id ? bs.board_id() : 0u};
    shard = BoardCacheKeyHash{}(key) & (SolverCache::kNumShards - 1);
    std::lock_guard<std::mutex> lock(cache->shard_mu[shard]);
    auto it = cache->tree_cache[shard].find(key);
    if (it != cache->tree_cache[shard].end()) {
      cache->tree_hits.fetch_add(1, std::memory_order_relaxed);
      std::shared_ptr<OptimalPlayTree> cached = it->second;
      if (deepen_levels > 0) {
        cached->Deepen(deepen_levels, cache);
      }
      return cached;
    }
  }
  OptimalLines lines;
  int mini_max = DDSFindOptimalLines(bs, min_tricks, &lines, cache);
  VLOG(2) << "FindOptimalPlayTreeHelper got " << mini_max;
  std::shared_ptr<OptimalPlayTree> ans(
      new OptimalPlayTree(mini_max, bs, tricks_remaining, min_tricks));
  for (const OptimalLine &line : lines) {
    ans->AddLine(line);
  }
  if (use_tree_cache) {
    std::lock_guard<std::mutex> lock(cache->shard_mu[shard]);
    auto existing = cache->tree_cache[shard].find(key);
    if (existing != cache->tree_cache[shard].end()) {
      cache->tree_hits.fetch_add(1, std::memory_order_relaxed);
      ans = existing->second;
    } else {
      if (cache->tree_ring[shard].size() <
          SolverCache::kMaxTreeCachePerShard) {
        cache->tree_ring[shard].push_back(key);
      } else {
        size_t &cur = cache->tree_cursor[shard];
        cache->tree_cache[shard].erase(cache->tree_ring[shard][cur]);
        cache->tree_ring[shard][cur] = key;
        cur = (cur + 1) % SolverCache::kMaxTreeCachePerShard;
      }
      cache->tree_cache[shard].emplace(key, ans);
      cache->tree_misses.fetch_add(1, std::memory_order_relaxed);
    }
  }
  ans->Deepen(deepen_levels, cache);
  return ans;
}

void OptimalPlayTree::Deepen(int num_levels, SolverCache *cache) {
  VLOG(2) << "Deepen num_levels " << num_levels
          << " tricks_remaining " << tricks_remaining;
  if (num_levels <= 0 || tricks_remaining <= 0) {
    return;
  }
  for (auto &p : all_plays) {
    if (p.second.get()) {
      p.second->Deepen(num_levels - 1, cache);
      continue;
    }
    const OptimalLine &line = p.first;
    BoardState copy(my_bs);
    copy.PlayTrick(line);
    int ns_wins_trick = copy.CurPlayer() % 2 == 0 ? 1 : 0;
    std::shared_ptr<OptimalPlayTree> rest =
        FindOptimalPlayTreeHelper(copy,
                                  std::max(0, min_tricks - ns_wins_trick),
                                  tricks_remaining - 1, num_levels - 1, cache);
    p.second = rest;
  }
}

std::shared_ptr<OptimalPlayTree> OptimalPlayTree::Deepen(
    const OptimalLine &line, SolverCache *cache) {
  VLOG(2) << "Deepen on " << line;
  {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = all_plays.find(line);
    CHECK(it != all_plays.end()) << line;
    if (it->second) {
      return it->second;
    }
    auto wit = cached_plays_.find(line);
    if (wit != cached_plays_.end()) {
      if (auto sp = wit->second.lock()) {
        return sp;
      }
    }
  }
  BoardState copy(my_bs);
  copy.PlayTrick(line);
  int ns_wins_trick = copy.CurPlayer() % 2 == 0 ? 1 : 0;
  std::shared_ptr<OptimalPlayTree> rest =
      FindOptimalPlayTreeHelper(copy,
                                std::max(0, min_tricks - ns_wins_trick),
                                tricks_remaining - 1, 0, cache);
  std::lock_guard<std::mutex> lock(mu_);
  if (cache != nullptr && tricks_remaining > 1) {
    cached_plays_[line] = rest;
    return rest;
  }
  auto it = all_plays.find(line);
  if (!it->second) {
    it->second = rest;
  }
  return it->second;
}

std::shared_ptr<OptimalPlayTree> OptimalPlayTree::PlayTrick(
    const OptimalLine &trick, SolverCache *cache) {
  return Deepen(trick, cache);
}

/* static */
std::shared_ptr<OptimalPlayTree> OptimalPlayTree::FindOptimalPlayTree(
    const BoardState &bs, int min_tricks,
    SolverCache *cache, int deepen_levels) {
  int tricks_remaining = 0;
  for (int i = 0; i < 4; ++i) {
    int cnt = CardsInHand(bs.GetHand(i), bs.played_cards()).size();
    for (int j = 0; j < bs.CardsPlayedThisTrick(); ++j) {
      if (bs.trick_cards_played()[j] & bs.GetHand(i)) {
        --cnt;
      }
    }
    tricks_remaining = std::max(tricks_remaining, cnt);
  }
  auto ans =
      FindOptimalPlayTreeHelper(bs, min_tricks, tricks_remaining, deepen_levels,
                                cache);
  if (cache) {
    VLOG(1) << "OptimalTreeSearch finished with cache having "
            << cache->dds_size();
  }
  VLOG(1) << "Found solution of " << ans->MiniMax();
  return ans;
}
