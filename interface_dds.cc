#include "interface_dds.h"

#include <algorithm>

#include "glog/logging.h"
#include "omp_compat.h"

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
      z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
      z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
      z = z ^ (z >> 31);
      return base_r + static_cast<int>(z % static_cast<uint64_t>(count));
    }
    return base_r + static_cast<int>(lrand48() % count);
  }
  return base_r;
}

// Chooses which DDS equivalence group index `i` (0 .. futp.cards - 1) and
// which specific DDS rank (2..14) within that group a defender should play.
std::pair<int, int> ChooseDefenderPlay(const BoardState &bs, int level,
                                       const futureTricks &futp) {
  auto finalize = [&](int idx, int base_r) -> std::pair<int, int> {
    return {idx, RandomizeTouchingHonorRank(bs, futp, idx, base_r)};
  };

  Suit suit_led = bs.SuitLed();
  // 1. Not following suit (discarding or ruffing when void in suit_led).
  if (suit_led != NOTRUMP && futp.suit[0] != suit_led) {
    int idx = bs.FindBestDiscard(
        futp.cards, const_cast<int *>(futp.suit), const_cast<int *>(futp.rank));
    return finalize(idx, LowestRankInGroup(futp, idx));
  }

  // 2. Defender on lead (1st hand, level == 0).
  if (level == 0 || suit_led == NOTRUMP) {
    uint64_t mask = ~bs.played_cards();
    uint64_t def_cards = bs.GetHand(bs.CurPlayer()) & mask;
    uint64_t ns_cards = (bs.GetHand(0) | bs.GetHand(2)) & mask;
    int best_idx = 0;
    int best_rank = LowestRankInGroup(futp, 0);
    int best_key = 10000;
    for (int i = 0; i < futp.cards; ++i) {
      int s = futp.suit[i];
      int low_r = LowestRankInGroup(futp, i);
      int top_r = futp.rank[i];
      bool touching_honors =
          top_r >= 10 && ((futp.equals[i] >> (top_r - 1)) & 1) != 0;
      int seq_low = top_r;
      while (touching_honors && seq_low - 1 >= 2 &&
             ((futp.equals[i] >> (seq_low - 1)) & 1) != 0) {
        --seq_low;
      }
      int play_r = touching_honors ? seq_low : low_r;

      uint16_t def_suit = (def_cards >> (s * 16)) & 0xffff;
      uint16_t ew_suit =
          (((bs.GetHand(1) | bs.GetHand(3)) & mask) >> (s * 16)) & 0xffff;
      uint16_t ns_suit = (ns_cards >> (s * 16)) & 0xffff;

      // Check if top_r and top_r - 1 are touching honors in the group OR in
      // the defender's hand (when partner holds the card between two DDS
      // equivalence groups or when only one is in futp.equals).
      bool hand_touching_honors =
          top_r >= 10 &&
          (((futp.equals[i] >> (top_r - 1)) & 1) != 0 ||
           (top_r >= 3 && ((def_suit >> (top_r - 3)) & 1) != 0));
      bool ns_has_higher_than_top =
          top_r < 14 && (ns_suit >> (top_r - 1)) != 0;
      // Check whether N/S have a guarded honor/tenace in suit `s` that could
      // benefit from a defender lead: either N/S hold >=2 cards with an honor
      // and E/W still hold an honor >= J that could be finessed/trapped (e.g.
      // N/S have AKJ missing Q, or K9 missing AQJ), as opposed to N/S holding
      // only the boss Ace with E/W holding QJ (e.g. DA5 after DK won).
      int ns_top_bit = ns_suit != 0 ? 31 - __builtin_clz(ns_suit) : -1;
      int ew_top_bit = ew_suit != 0 ? 31 - __builtin_clz(ew_suit) : -1;
      uint16_t ns_honors = ns_suit >> 9;
      bool ns_has_tenace =
          ns_honors != 0 && __builtin_popcount(ns_suit) >= 2 &&
          (ew_top_bit > ns_top_bit || __builtin_popcount(ns_honors) >= 2);
      int def_highest_bit =
          def_suit != 0 ? 31 - __builtin_clz(def_suit) : -1;
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
      int key = tier + (hand_touching_honors ? (15 - top_r) : low_r);
      if (i == 0 || key < best_key) {
        best_key = key;
        best_idx = i;
        best_rank = play_r;
      }
    }
    return finalize(best_idx, best_rank);
  }








  // 3. Following suit in 2nd, 3rd, or 4th hand.
  auto [win_offset, win_pair] = bs.TrickWinner();
  int cur_winner = (bs.cur_trick_leader() + win_offset) % 4;
  Suit cur_win_suit = win_pair.first;
  int cur_win_rank = win_pair.second + 2;  // Convert 0..12 to DDS rank 2..14.
  int partner = bs.CurPlayer() ^ 2;
  bool trick_ruffed = (bs.GetTrump() != NOTRUMP &&
                       suit_led != bs.GetTrump() &&
                       cur_win_suit == bs.GetTrump());

  // Helper to pick the overall lowest card across all optimal groups.
  auto pick_lowest = [&]() -> std::pair<int, int> {
    int best_idx = 0;
    int lowest = LowestRankInGroup(futp, 0);
    for (int i = 1; i < futp.cards; ++i) {
      int r = LowestRankInGroup(futp, i);
      if (r < lowest) {
        lowest = r;
        best_idx = i;
      }
    }
    return finalize(best_idx, lowest);
  };

  if (level == 1 || trick_ruffed) {
    // 2nd hand low (or trick already ruffed so following suit cannot win).
    return pick_lowest();
  }

  if (level == 2) {
    // 3rd hand high (playing from the highest winning group), unless partner
    // led an honor (>= J) that is currently winning and we have a lower
    // optimal play that avoids overtaking partner.
    if (cur_winner == partner && cur_win_rank >= 11) {
      auto [low_idx, low_r] = pick_lowest();
      if (low_r < cur_win_rank) {
        return {low_idx, low_r};
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
      int base_r = std::max(LowestRankInGroup(futp, best_idx),
                            CheapestWinningRankInGroup(futp, best_idx,
                                                       cur_win_rank));
      return finalize(best_idx, base_r);
    }
    // Cannot beat the current winning card; play 3rd hand low.
    return pick_lowest();
  }

  // level == 3 (4th hand):
  // If partner is already winning the trick, do not overtake unless every
  // optimal play in `futp` overtakes (in which case pick_lowest() chooses the
  // cheapest overtaking card).
  if (cur_winner == partner) {
    return pick_lowest();
  }

  // Declarer/dummy is currently winning: win as cheaply as possible if an
  // optimal winning play exists; otherwise duck with the lowest card.
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
    return finalize(best_win_idx, cheapest_win_rank);
  }
  return pick_lowest();
}

}  // namespace

std::string FindBestDefenderCard(const BoardState &bs) {
  DoInit();
  std::vector<CardType> valid = bs.ValidCardsToPlay();
  if (valid.size() == 1) {
    return Card::PrintCard(valid[0]);
  }
  dealPBN dl;
  PopulatePBNDeal(bs, &dl);
  futureTricks futp;
  int target = -1;
  int solutions = 2;
  int mode = 1;
  int threadindex = omp_get_thread_num();
  SolveBoardPBN(dl, target, solutions, mode, &futp, threadindex);
  if (futp.cards <= 0) {
    return valid.empty() ? "" : Card::PrintCard(valid[0]);
  }
  int level = bs.CardsPlayedThisTrick();
  auto [card_to_play, defender_rank_to_play] =
      ChooseDefenderPlay(bs, level, futp);
  return Card::PrintDDSCard(static_cast<Suit>(futp.suit[card_to_play]),
                            defender_rank_to_play);
}

int DDSSolveHelper(int min_tricks, const BoardState &bs, dealPBN &dl,
                   int level, OptimalLines *lines,
                   SolverCache *cache,
                   futureTricks *solved_tricks = nullptr) {
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
  futureTricks futp_storage;
  if (!solved_tricks) {
    int target = use_min_tricks_target ? min_tricks : -1;
    int solutions = 2; // Return all optimum cards and their scores.
    // int mode = 1;  // automatic reuse of state / always search
    int mode = 2;  // force reuse of state
    int threadindex = omp_get_thread_num();
    SolveBoardPBN(dl, target, solutions, mode, &futp_storage, threadindex);
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
        ChooseDefenderPlay(bs, level, futp);
    CHECK_GE(card_to_play, 0) << futp.cards;
  }
  int lowest_discard_rank[4] = {100, 100, 100, 100};
  if (pair_is_me && bs.SuitLed() != NOTRUMP) {
    for (int i = 0; i < futp.cards; i++) {
      int s = futp.suit[i];
      if (s != bs.SuitLed() && s != bs.GetTrump()) {
        lowest_discard_rank[s] = std::min(lowest_discard_rank[s], futp.rank[i]);
      }
    }
  }
  OptimalLines my_lines;
  for (int i = 0; i < futp.cards; i++) {
    if (!pair_is_me && i != card_to_play) {
      continue;
    }
    if (pair_is_me && bs.SuitLed() != NOTRUMP) {
      int s = futp.suit[i];
      if (s != bs.SuitLed() && s != bs.GetTrump() &&
          futp.rank[i] > lowest_discard_rank[s]) {
        continue;
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
      DDSSolveHelper(min_tricks, copy, dl, level + 1, &rest, cache);
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
    key = BoardCacheKey{
        bs.GetHand(0) & mask,
        h1,
        bs.GetHand(2) & mask,
        h3,
        static_cast<uint8_t>(bs.CurPlayer()),
        static_cast<uint8_t>(bs.GetTrump()),
        static_cast<uint8_t>(std::max(0, min_tricks)),
        has_touching_honors ? bs.board_id() : 0u};
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
