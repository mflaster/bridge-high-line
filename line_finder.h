#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>

#include "conditional_line.h"
#include "interface_dds.h"
#include "search_progress.h"
#include "solver.h"
#include "hand_requirements.h"

class FuzzyBoardState {
 public:
  FuzzyBoardState() = default;
  FuzzyBoardState(const std::vector<uint64_t> &hands, uint64_t unknown_cards) {
    SetHands(hands, unknown_cards);
  }
  void SetHands(const std::vector<uint64_t> &hands, uint64_t unknown_cards);
  void SetRequirements1(
      const std::vector<std::shared_ptr<HandRequirements>> &reqs) {
    hand_reqs1_ = reqs;
    if (all_hand_reqs_.size() < 4) all_hand_reqs_.resize(4);
    all_hand_reqs_[1] = reqs;
  }
  void SetRequirements3(
      const std::vector<std::shared_ptr<HandRequirements>> &reqs) {
    hand_reqs3_ = reqs;
    if (all_hand_reqs_.size() < 4) all_hand_reqs_.resize(4);
    all_hand_reqs_[3] = reqs;
  }
  void SetRequirements(
      const std::vector<std::vector<std::shared_ptr<HandRequirements>>> &r) {
    all_hand_reqs_ = r;
    if (r.size() > 1) hand_reqs1_ = r[1];
    if (r.size() > 3) hand_reqs3_ = r[3];
  }
  const std::vector<std::shared_ptr<HandRequirements>> &hand_reqs1() const {
    return hand_reqs1_;
  }
  const std::vector<std::shared_ptr<HandRequirements>> &hand_reqs3() const {
    return hand_reqs3_;
  }

  BoardState Randomize(bool deal_missing_cards = false);

 private:
  std::vector<uint64_t> known_hands_;
  uint64_t unknown_cards_ = 0;
  std::vector<std::shared_ptr<HandRequirements>> hand_reqs1_;
  std::vector<std::shared_ptr<HandRequirements>> hand_reqs3_;
  std::vector<std::vector<std::shared_ptr<HandRequirements>>> all_hand_reqs_;
};

class LineFinder {
 public:
  LineFinder(int num_iters)
      : num_iters_(num_iters),
        solver_cache_(std::make_shared<SolverCache>()),
        solve_memo_(std::make_unique<SolveMemo>()) {}

  void SetRequirements(
      const std::vector<std::shared_ptr<HandRequirements>> &reqs1,
      const std::vector<std::shared_ptr<HandRequirements>> &reqs3) {
    fuzzy_state_.SetRequirements1(reqs1);
    fuzzy_state_.SetRequirements3(reqs3);
    ResetCachedDeals();
  }
  void SetRequirements(
      const std::vector<std::vector<std::shared_ptr<HandRequirements>>> &r) {
    fuzzy_state_.SetRequirements(r);
    ResetCachedDeals();
  }

  void ResetCachedDeals() {
    base_boards_.clear();
    base_init_trees_.clear();
    base_hands_.clear();
    base_unknown_cards_ = 0;
    base_leader_ = -1;
    base_min_tricks_ = -1;
    solve_memo_ = std::make_unique<SolveMemo>();
  }
  const std::vector<std::shared_ptr<HandRequirements>> &hand_reqs1() const {
    return fuzzy_state_.hand_reqs1();
  }
  const std::vector<std::shared_ptr<HandRequirements>> &hand_reqs3() const {
    return fuzzy_state_.hand_reqs3();
  }

  void SetPredealtBoards(
      const std::vector<BoardState> &predealt_boards,
      const std::vector<std::vector<std::string>> &replay_tricks = {}) {
    predealt_boards_ = predealt_boards;
    replay_tricks_ = replay_tricks;
  }

  void SetSharedCache(std::shared_ptr<SolverCache> cache) {
    if (cache) solver_cache_ = std::move(cache);
  }

  // Solves for optimal play lines to achieve min_tricks across simulated boards.
  //
  // Returns:
  //   std::vector<ConditionalLine>: A vector containing all top-level ConditionalLine decision trees
  //   that tie for the maximum possible board coverage count (most_boards_covered).
  //   - If multiple lines are returned, they are all equivalent in terms of total board success count,
  //     but represent distinct, alternative optimal play strategies (e.g. leading different suits/cards).
  //   - The top ConditionalLine returned has its do_first_ set to empty and its conditions_ list populated
  //     with TrickPlayCondition branches ("after trick...") pointing to child subtrees.
  std::vector<ConditionalLine> Solve(
      int min_tricks,
      const std::vector<uint64_t> &hands, uint64_t unknown_cards,
      int leader, Suit trump, const std::vector<CardType> &cur_trick = {},
      int num_threads = 4);

  std::set<std::string> FindBestLead(
      int min_tricks,
      const std::vector<uint64_t> &hands, uint64_t unknown_cards,
      int leader, Suit trump);

  std::multiset<int> FindTricksWon(
      const std::vector<uint64_t> &hands, uint64_t unknown_cards,
      int leader, Suit trump, bool deal_missing_cards = true);

  static std::string TricksWonDebugString(const std::multiset<int> &tricks);
  static int CountAtLeast(const std::multiset<int> &data, int thresh);

  static std::string GetSolveStatus();
  static void SetSolveStatus(const std::string &status);
  // Estimated fraction of the play-tree search finished, in [0, 1], or -1
  // when no search is running.
  static double GetSolveProgress();

  double solve_ms() const { return solve_ms_; }
  double invariant_ms() const { return invariant_ms_; }
  const std::vector<BoardState> &boards() const { return boards_; }
  const std::vector<BoardState> &initial_boards() const {
    return initial_boards_;
  }
  const std::shared_ptr<SolverCache> &shared_cache() const {
    return solver_cache_;
  }

 protected:
  std::vector<BoardState> SetupBoards(
      Suit trump, int leader, std::set<int> &board_offsets,
      const std::vector<CardType> &cur_trick,
      bool deal_missing_cards = false);

  // Constructs an intermediate action ConditionalLine node for a specific GeneralizedTrick.
  // Sets a_line.do_first_ = gen_trick, plays the trick on each board tree, and adds WinnerPlayCondition
  // branches ("when 0 wins", "if 1 wins") for each trick winner pointing to recursive SolveHelper subtrees.
  // `progress`, if set, is this trick's node in the search progress tree.
  ConditionalLine GetLineFromTrick(
      const GeneralizedTrick &gen_trick,
      int min_tricks,
      SolverCache &cache,
      std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
      const std::set<int> &board_offsets,
      int leader, int lvl, SearchProgress::Node *progress = nullptr,
      int min_covered = 0);

  // Evaluates groups of generalized tricks and constructs top-level or branch ConditionalLine nodes.
  // Filters and returns all ConditionalLine options that achieve the maximum board coverage count.
  // `progress`, if set, is the node whose batches this call reports.
  std::vector<ConditionalLine> GetLinesFromGroups(
      const std::vector<std::vector<GeneralizedTrick>> &groupings,
      int min_tricks,
      SolverCache &cache,
      std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
      const std::set<int> &board_offsets,
      int leader, int lvl, SearchProgress::Node *progress = nullptr,
      int min_covered = 0);

  // Attempts to improve board coverage when single-trick groupings do not
  // cover all solvable boards by branching on IsCardGoodCondition for the
  // highest remaining N/S card in each suit.
  void TryBranchOnGoodCards(
      int min_tricks,
      SolverCache &cache,
      std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
      const std::set<int> &solvable_boards,
      int cards_left, int leader, int lvl,
      std::vector<ConditionalLine> &ans,
      SearchProgress::Node *progress = nullptr,
      int min_covered = 0);

  // Recursive workhorse for finding optimal play lines.
  // - Base case (cards_left == 0): returns empty vector if min_tricks > 0 (failure), or a single leaf
  //   ConditionalLine initialized with board_offsets if min_tricks <= 0 (success).
  // - Recursive case: extracts optimal tricks across boards, generalizes them into trick groups,
  //   and invokes GetLinesFromGroups.
  // `progress`, if set, is this sub-search's node in the search progress
  // tree; it is completed before returning.
  std::vector<ConditionalLine> SolveHelper(
      int min_tricks,
      SolverCache &cache,
      std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
      const std::set<int> &board_offsets,
      int leader, int lvl, SearchProgress::Node *progress = nullptr,
      int min_covered = 0);

  std::vector<ConditionalLine> SolveHelperUncached(
      int min_tricks,
      SolverCache &cache,
      std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
      const std::set<int> &board_offsets,
      int leader, int lvl, SearchProgress::Node *progress = nullptr,
      int min_covered = 0);

 private:
  static constexpr size_t kMemoShards = 64;
  static constexpr size_t kMaxMemoPerShard = 4096;
  struct MemoEntry {
    std::vector<ConditionalLine> lines;
    int fail_low_bound = 0;
    bool exact = false;
  };
  struct SolveMemo {
    std::mutex mu[kMemoShards];
    std::unordered_map<std::string, MemoEntry> shards[kMemoShards];
    std::vector<std::string> ring[kMemoShards];
    size_t cursor[kMemoShards] = {};

    bool Lookup(const std::string &key, int min_covered,
                std::vector<ConditionalLine> *out);
    void Insert(std::string key, int min_covered,
                const std::vector<ConditionalLine> &res);
  };

  int num_iters_;
  FuzzyBoardState fuzzy_state_;
  std::vector<BoardState> boards_;
  std::vector<BoardState> initial_boards_;
  std::vector<BoardState> predealt_boards_;
  std::vector<std::vector<std::string>> replay_tricks_;
  std::vector<BoardState> base_boards_;
  std::vector<std::shared_ptr<OptimalPlayTree>> base_init_trees_;
  std::vector<uint64_t> base_hands_;
  uint64_t base_unknown_cards_ = 0;
  Suit base_trump_ = NOTRUMP;
  int base_leader_ = -1;
  int base_min_tricks_ = -1;
  std::shared_ptr<SolverCache> solver_cache_;
  std::unique_ptr<SolveMemo> solve_memo_;
  double solve_ms_ = 0.0;
  double invariant_ms_ = 0.0;
};
