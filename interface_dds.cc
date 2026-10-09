#include "interface_dds.h"

#include <algorithm>
#include <omp.h>

#include "glog/logging.h"

namespace {
class InitThreads {
 public:
  InitThreads() {
#if defined(__linux) || defined(__APPLE__)
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
  CHECK_GT(futp.cards, 0);
  int result = futp.score[0];
  return result;
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
  bool use_min_tricks_target = false;
  if (pair_is_me && min_tricks >= 1 && bs.SuitLed() != NOTRUMP && level < 3) {
    uint16_t suit_cards = (cur_hand >> (bs.SuitLed() * 16)) & 0xffff;
    // Only use target = min_tricks when following suit with >= 2 honors
    // (T, J, Q, K, A = bits 8..12 = 0x1f00) so finesses like CJ from AKJ
    // are included without adding suboptimal leads, ducks, or discards.
    if (__builtin_popcount(suit_cards & 0x1f00) >= 2) {
      use_min_tricks_target = true;
    }
  }
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
    if (pair_is_me && level == 0 && result < min_tricks) {
      return result;
    }
  }
  int lowest_rank = 100;
  int card_to_play = -1;
  if (defense) {
    if (bs.SuitLed() != NOTRUMP && futp.suit[0] != bs.SuitLed()) {
      // They're discarding.
      card_to_play = bs.FindBestDiscard(futp.cards, futp.suit, futp.rank);
    } else {
      // Take lowest rank.
      VLOG(2) << "Looking for lowest rank vs " << lowest_rank;
      for (int i = 0; i < futp.cards; i++) {
        VLOG(3) << "Rank of card " << i << " is " << futp.rank[i];
        if (futp.rank[i] < lowest_rank) {
          lowest_rank = futp.rank[i];
          card_to_play = i;
        }
      }
    }
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
    std::string c = Card::PrintDDSCard((Suit)futp.suit[i], futp.rank[i]);
    VLOG(3) << "At level " << level << " trying card " << c
            << " " << i << " / " << futp.cards
            << " with suit led " << SuitDebugString(bs.SuitLed());
    OptimalLines rest;
    if (level < 3) {
      BoardState copy(bs);
      CardType c_val = Card::ParseStringCard(c);
      copy.PlayCard(copy.CurPlayer(), c_val);

      dl.currentTrickSuit[level] = futp.suit[i];
      dl.currentTrickRank[level] = futp.rank[i];
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

  return DDSSolveHelper(min_tricks, bs, dl, bs.CardsPlayedThisTrick(), lines,
                        cache);
}

std::vector<std::pair<int, OptimalLines>>
    DDSParallelSolve(const std::vector<BoardState> &boards_to_run) {
#if 0
  int target = -1;  // Find optimal solution for side to play
  int solutions = 2; // Return all optimum cards and their scores.
  // int mode = 1;  // automatic reuse of state / always search
  int mode = 2;  // force reuse of state

  boardsPBN all_boards;
  all_boards.noOfBoards = 0;
  for (const BoardState &bs : boards_to_run) {
    int bn = all_boards.noOfBoards++;
    dealPBN &dl = all_boards.deals[bn];
    PopulatePBNDeal(bs, &dl);
    all_boards.target[bn] = target;
    all_boards.solutions[bn] = solutions;
    all_boards.mode[bn] = mode;
  }
  solvedBoards solved_boards;
  SolveAllChunksPBN(&all_boards, &solved_boards, /*chunkSize=*/1);
  CHECK_EQ(solved_boards.noOfBoards, all_boards.noOfBoards);
#endif
  std::vector<std::pair<int, OptimalLines>> ans;
  for (uint i = 0; i < boards_to_run.size(); i++) {
    OptimalLines lines;

  dealPBN dl;
  PopulatePBNDeal(boards_to_run[i], &dl);

  int mini_max = DDSSolveHelper(0, boards_to_run[i], dl,// all_boards.deals[i],
                                0, &lines,
                                nullptr); // &solved_boards.solvedBoard[i]);
    ans.push_back(std::make_pair(mini_max, lines));
  }
  return ans;
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
    key = BoardCacheKey{
        bs.GetHand(0) & mask,
        bs.GetHand(1) & mask,
        bs.GetHand(2) & mask,
        bs.GetHand(3) & mask,
        static_cast<uint8_t>(bs.CurPlayer()),
        static_cast<uint8_t>(bs.GetTrump()),
        static_cast<uint8_t>(std::max(0, min_tricks))};
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
    auto [it, inserted] = cache->tree_cache[shard].emplace(key, ans);
    if (!inserted) {
      cache->tree_hits.fetch_add(1, std::memory_order_relaxed);
      ans = it->second;
    } else {
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
  }
  BoardState copy(my_bs);
  copy.PlayTrick(line);
  int ns_wins_trick = copy.CurPlayer() % 2 == 0 ? 1 : 0;
  std::shared_ptr<OptimalPlayTree> rest =
      FindOptimalPlayTreeHelper(copy,
                                std::max(0, min_tricks - ns_wins_trick),
                                tricks_remaining - 1, 0, cache);
  std::lock_guard<std::mutex> lock(mu_);
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
  int tricks_remaining = CardsInHand(bs.GetHand(0), bs.played_cards()).size();
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
