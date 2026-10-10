#include "line_finder.h"

#include <ctype.h>
#include <omp.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <mutex>
#include "cross_hand_summarizer.h"
#include "deal.h"
#include "glog/logging.h"
#include "interface_dds.h"

void FuzzyBoardState::SetHands(const std::vector<uint64_t> &hands,
                               uint64_t unknown_cards) {
  known_hands_ = hands;
  unknown_cards_ = unknown_cards;
}

BoardState FuzzyBoardState::Randomize(bool deal_missing_cards) {
  std::vector<uint64_t> hands = known_hands_;
  if (unknown_cards_ == 0) {
    BoardState bs;
    bs.SetHands(hands);
    return bs;
  }
  if (deal_missing_cards) {
    hands = DealMissingCards(known_hands_, all_hand_reqs_);
  } else {
    auto [hand1, hand3] =
        DealTwoRequiredHands(unknown_cards_, hand_reqs1_, hand_reqs3_);

    hands[1] = hands[1] | hand1;
    hands[3] = hands[3] | hand3;
  }

  BoardState bs;
  bs.SetHands(hands);
  VLOG(1) << "Have hands: "
          << HandDebugString(hands[0], true) << " // "
          << HandDebugString(hands[1], true) << " // "
          << HandDebugString(hands[2], true) << " // "
          << HandDebugString(hands[3], true);
  return bs;
}

void PlayTrick(const std::string &trick, BoardState &bs) {
  return bs.PlayTrick(trick);
}

std::vector<BoardState> LineFinder::SetupBoards(
    Suit trump, int leader, std::set<int> &board_offsets,
    const std::vector<CardType> &cur_trick, bool deal_missing_cards) {
  std::vector<BoardState> boards;
  for (int i = 0; i < num_iters_; i++) {
    BoardState b = fuzzy_state_.Randomize(deal_missing_cards);
    b.set_check_card_play(false);
    b.SetTrump(trump);
    b.SetLead(leader);
    for (CardType c : cur_trick) {
      b.PlayCard(b.CurPlayer(), c);
    }
    boards.emplace_back(b);
    board_offsets.insert(i);
    LOG(INFO) << "Complete board " << i << ": " << b.DebugString();
  }
  return boards;
}


std::set<std::string> LineFinder::FindBestLead(
    int min_tricks,
    const std::vector<uint64_t> &hands, uint64_t unknown_cards,
    int leader, Suit trump) {
  CHECK_EQ(4, hands.size());
  fuzzy_state_.SetHands(hands, unknown_cards);
  std::set<int> board_offsets;
  std::vector<CardType> cur_trick;
  std::vector<BoardState> boards =
      SetupBoards(trump, leader, board_offsets, cur_trick,
                  /*deal_missing_cards=*/true);
  std::map<std::string, std::set<int>> lead_to_boards;
  std::map<int, int> board_tricks;
  size_t max_outcome = 0;
  for (uint i = 0; i < boards.size(); i++) {
    std::vector<std::string> choices =
        FindBestCardsToPlay(boards[i], min_tricks);
    if (choices.empty()) {
      LOG(INFO) << "Could not solve board " << i;
      continue;
    }
    for (const std::string &lead : choices) {
      auto &s = lead_to_boards[lead];
      s.insert(i);
      max_outcome = std::max(s.size(), max_outcome);
    }
  }
  std::set<std::string> ans;
  std::multimap<int, std::string> sorted_leads;
  for (const auto &[lead, offsets] : lead_to_boards) {
    sorted_leads.insert(std::make_pair(boards.size() - offsets.size(), lead));
  }
  for (const auto &[_count, lead] : sorted_leads) {
    std::set<int> offsets = lead_to_boards[lead];
    bool is_best = offsets.size() == max_outcome;
    std::string off_str = StrJoin(offsets, ",");
    if (off_str.size() > 80) {
      off_str.resize(80);
      off_str += "...";
    }
    LOG(INFO) << (is_best ? "*" : " ")
              << "Lead " << lead << " (" << offsets.size()
              << ") " << off_str;
    if (is_best) {
      ans.insert(lead);
    }
  }
  return ans;
}

std::multiset<int> LineFinder::FindTricksWon(
    const std::vector<uint64_t> &hands, uint64_t unknown_cards,
    int leader, Suit trump, bool deal_missing_cards) {
  CHECK_EQ(4, hands.size());
  fuzzy_state_.SetHands(hands, unknown_cards);
  std::set<int> board_offsets;
  std::vector<CardType> cur_trick;
  std::vector<BoardState> boards =
      SetupBoards(trump, leader, board_offsets, cur_trick,
                  deal_missing_cards);
  std::multiset<int> tricks_won;
  for (uint i = 0; i < boards.size(); i++) {
    int cards_left =
        CardsInHand(boards[i].GetHand(0), boards[i].played_cards()).size();
    int mini_max = FindOptimalTricks(boards[i]);
    tricks_won.insert(leader % 2 == 0 ? mini_max : cards_left - mini_max);
  }
  return tricks_won;
}

/* static */
std::string LineFinder::TricksWonDebugString(const std::multiset<int> &tricks) {
  int sum = 0;
  std::map<int, int> counts;
  for (int x : tricks) {
    sum += (x);
    counts[(x)]++;
  }
  double avg = sum / static_cast<double>(tricks.size());
  std::string ans = "Average: " + std::to_string(avg) + "\n";
  for (const auto &[t, n] : counts) {
    ans += "  " + std::to_string(t) + " tricks: " + std::to_string(n) + "\n";
  }
  return ans;
}

/* static */
int LineFinder::CountAtLeast(const std::multiset<int> &data, int thresh) {
  int ans = 0;
  for (int x : data) {
    if (x >= thresh) {
      ans++;
    }
  }
  return ans;
}

namespace {
std::mutex g_status_mutex;
std::string g_solve_status = "Idle";
// Progress tree of the search currently running, if any; the status reported
// while it is set is derived from it rather than from g_solve_status.
std::shared_ptr<SearchProgress> g_active_progress;
std::atomic<int> g_best_covered{0};
std::atomic<int> g_total_boards{0};
}  // namespace

static std::atomic<uint64_t> get_line_from_trick_calls{0};
static std::atomic<uint64_t> g_play_trick_calls{0};

std::string LineFinder::GetSolveStatus() {
  std::shared_ptr<SearchProgress> progress;
  {
    std::lock_guard<std::mutex> lock(g_status_mutex);
    if (!g_active_progress) {
      return g_solve_status;
    }
    progress = g_active_progress;
  }
  int pct = static_cast<int>(std::floor(progress->Fraction() * 100.0));
  return "Searching play trees: ~" + std::to_string(pct) +
         "% done (best coverage: " + std::to_string(g_best_covered.load()) +
         "/" + std::to_string(g_total_boards.load()) + " deals, " +
         std::to_string(get_line_from_trick_calls.load()) + " branches)...";
}

void LineFinder::SetSolveStatus(const std::string &status) {
  std::lock_guard<std::mutex> lock(g_status_mutex);
  g_solve_status = status;
}

double LineFinder::GetSolveProgress() {
  std::shared_ptr<SearchProgress> progress;
  {
    std::lock_guard<std::mutex> lock(g_status_mutex);
    progress = g_active_progress;
  }
  return progress ? progress->Fraction() : -1.0;
}

namespace {

void SetActiveProgress(std::shared_ptr<SearchProgress> progress) {
  std::lock_guard<std::mutex> lock(g_status_mutex);
  g_active_progress = std::move(progress);
}

}  // namespace

std::vector<ConditionalLine> LineFinder::Solve(
    int min_tricks,
    const std::vector<uint64_t> &hands, uint64_t unknown_cards,
    int leader, Suit trump, const std::vector<CardType> &cur_trick,
    int num_threads) {
  auto start_solve = std::chrono::high_resolution_clock::now();
  CHECK_EQ(4, hands.size());
  int max_dds_threads = EnsureDDSInitialized();
  int effective_threads = std::clamp(num_threads, 1, max_dds_threads);
  get_line_from_trick_calls = 0;
  g_play_trick_calls = 0;
  g_best_covered = 0;
  g_total_boards = num_iters_;
  SetSolveStatus("Generating " + std::to_string(num_iters_) +
                 " random deals...");
  LOG(INFO) << "Solving hands: \n\n"
            << "          N: " << HandDebugString(hands[0], true) << "\n\n"
            << " W: " << HandDebugString(hands[1], true)
            << " E: " << HandDebugString(hands[3], true)
            << "[" << HandDebugString(unknown_cards, true) << "]\n\n"
            << "          S: " << HandDebugString(hands[2], true)
            << "\n\nGoal: " << min_tricks << " tricks\n"
            << "Hands simulated: " << num_iters_
            << " (threads: " << effective_threads << ")\n\n";
  fuzzy_state_.SetHands(hands, unknown_cards);
  std::set<int> board_offsets;
  std::vector<BoardState> boards =
      SetupBoards(trump, leader, board_offsets, cur_trick);
  boards_ = boards;

  solve_memo_ = std::make_unique<SolveMemo>();
  SolverCache cache;
  auto start_init_trees = std::chrono::high_resolution_clock::now();
  std::map<int, std::shared_ptr<OptimalPlayTree>> board_trees;
  std::vector<std::shared_ptr<OptimalPlayTree>> init_trees(boards.size());
  std::atomic<int> init_done{0};
  #pragma omp parallel for num_threads(effective_threads) schedule(dynamic) \
      if (effective_threads > 1)
  for (size_t i = 0; i < boards.size(); i++) {
    init_trees[i] = OptimalPlayTree::FindOptimalPlayTree(
        boards[i], min_tricks, &cache);
    int done = ++init_done;
    SetSolveStatus("Solving initial double-dummy trees (" +
                   std::to_string(done) + "/" +
                   std::to_string(boards.size()) + " deals)...");
    VLOG(1) << "Finished computing tree for board " << i
            << " with mini_max " << init_trees[i]->MiniMax();
  }
  for (size_t i = 0; i < boards.size(); i++) {
    board_trees[static_cast<int>(i)] = init_trees[i];
  }
  auto end_init_trees = std::chrono::high_resolution_clock::now();
  double init_trees_ms = std::chrono::duration<double, std::milli>(
      end_init_trees - start_init_trees).count();

  SetSolveStatus("Searching play trees across " +
                 std::to_string(boards.size()) + " deals...");
  auto progress = std::make_shared<SearchProgress>();
  SetActiveProgress(progress);
  std::vector<ConditionalLine> ans;
  if (effective_threads > 1) {
    #pragma omp parallel num_threads(effective_threads)
    {
      #pragma omp single
      {
        ans = SolveHelper(min_tricks, cache, board_trees,
                          board_offsets, leader, 0, progress->root());
      }
    }
  } else {
    ans = SolveHelper(min_tricks, cache, board_trees,
                      board_offsets, leader, 0, progress->root());
  }
  SetActiveProgress(nullptr);
  auto end_solve = std::chrono::high_resolution_clock::now();
  double search_trees_ms = std::chrono::duration<double, std::milli>(
      end_solve - end_init_trees).count();
  solve_ms_ = std::chrono::duration<double, std::milli>(
      end_solve - start_solve).count();
  LOG(INFO) << "TIMING: init_trees_ms=" << init_trees_ms
            << " search_trees_ms=" << search_trees_ms
            << " (play_trick_calls=" << g_play_trick_calls.load()
            << " cache_hits=" << cache.tree_hits.load()
            << " cache_misses=" << cache.tree_misses.load() << ")"
            << " total_solve_ms=" << solve_ms_
            << " get_line_from_trick_calls="
            << get_line_from_trick_calls.load();

  // Tied lines can be the same line of play described with different
  // branching (say, splitting on a defender's card rather than on who won the
  // trick). Keep only the first of each set that prescribes identical declarer
  // plays on every covered board.
  {
    std::vector<ConditionalLine> distinct;
    std::vector<std::map<int, std::string>> seen;
    for (ConditionalLine &line : ans) {
      std::map<int, std::string> sig = line.DeclarerPlaySignature();
      if (std::find(seen.begin(), seen.end(), sig) == seen.end()) {
        seen.push_back(std::move(sig));
        distinct.push_back(std::move(line));
      }
    }
    ans = std::move(distinct);
  }

  auto start_invariant = std::chrono::high_resolution_clock::now();
  CrossHandDealSetSummarizer summarizer;
  for (size_t line_idx = 0; line_idx < ans.size(); ++line_idx) {
    auto &line = ans[line_idx];
    line.set_total_boards(num_iters_);
    line.set_min_tricks(min_tricks);
    if (!boards.empty()) {
      line.set_sample_board(boards[0]);
    }
    std::set<int> covered = line.CoveredBoardsSet();
    std::vector<PairedDeal> covered_deals;
    covered_deals.reserve(covered.size());
    for (int b_idx : covered) {
      if (b_idx >= 0 && b_idx < static_cast<int>(boards.size())) {
        covered_deals.emplace_back(boards[b_idx].GetHand(1),
                                   boards[b_idx].GetHand(3));
      }
    }

    std::vector<PairedDeal> failure_deals;
    for (int b_idx = 0; b_idx < static_cast<int>(boards.size()); ++b_idx) {
      if (covered.find(b_idx) == covered.end()) {
        failure_deals.emplace_back(boards[b_idx].GetHand(1),
                                   boards[b_idx].GetHand(3));
      }
    }

    if (!covered_deals.empty()) {
      SetSolveStatus("Summarizing deal invariants (line " +
                     std::to_string(line_idx + 1) + "/" +
                     std::to_string(ans.size()) + ")...");
      try {
        line.set_summary(summarizer.Summarize(
            covered_deals, "hand 1", "hand 3", &line, &failure_deals));
      } catch (const std::exception &e) {
        LOG(WARNING) << "Failed to summarize line: " << e.what();
      }
    }

    if (!failure_deals.empty()) {
      SetSolveStatus("Summarizing failure invariants (line " +
                     std::to_string(line_idx + 1) + "/" +
                     std::to_string(ans.size()) + ")...");
      try {
        line.set_failure_summary(summarizer.Summarize(
            failure_deals, "hand 1", "hand 3", &line, &covered_deals,
            /*is_failure_mode=*/true));
      } catch (const std::exception &e) {
        LOG(WARNING) << "Failed to summarize failure line: " << e.what();
      }
    }
  }
  auto end_invariant = std::chrono::high_resolution_clock::now();
  invariant_ms_ = std::chrono::duration<double, std::milli>(
      end_invariant - start_invariant).count();
  LOG(INFO) << "TIMING: invariant_ms=" << invariant_ms_
            << " ans.size()=" << ans.size();

  SetSolveStatus("Building decision tree...");
  LOG(INFO) << "Solve finished with get_line_from_trick_calls "
            << get_line_from_trick_calls.load();
  return ans;
}

std::string PrintBoardToTrick(const std::map<int, std::string> &arg) {
  std::string ans;
  for (const auto &[b, trick] : arg) {
    ans += std::to_string(b) + " -> " + trick;// + "; ";
  }
  return ans;
}

ConditionalLine LineFinder::GetLineFromTrick(
    const GeneralizedTrick &gen_trick,
    int min_tricks,
    SolverCache &cache,
    std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
    const std::set<int> &board_offsets,
    int leader, int lvl, SearchProgress::Node *progress) {
  SearchProgress::Completer progress_completer(progress);
  ++get_line_from_trick_calls;
  VLOG(2) << "Pursuing line satisfying boards " << gen_trick.boards.size();
  std::map<int, std::set<int>> leader_to_boards;
  VLOG(2) << "demi2.board_to_trick "
          << PrintBoardToTrick(gen_trick.demi2.board_to_trick);
  std::map<int, std::shared_ptr<OptimalPlayTree>> after_play_trees;

  if (omp_in_parallel()) {
    std::vector<std::pair<std::shared_ptr<OptimalPlayTree>, std::string>>
        to_deepen;
    for (int b : gen_trick.boards) {
      const std::shared_ptr<OptimalPlayTree> &board_tree = board_trees.at(b);
      CHECK(board_tree);
      std::string t_str = gen_trick.GetTrickForBoard(b);
      if (!board_tree->IsDeepened(t_str)) {
        to_deepen.emplace_back(board_tree, std::move(t_str));
      }
    }
    if (to_deepen.size() > 1) {
      for (size_t idx = 0; idx < to_deepen.size(); ++idx) {
        #pragma omp task firstprivate(idx) shared(to_deepen, cache)
        {
          to_deepen[idx].first->PlayTrick(to_deepen[idx].second, &cache);
        }
      }
      #pragma omp taskwait
    }
  }

  for (int b : gen_trick.boards) {
    const std::shared_ptr<OptimalPlayTree> &board_tree = board_trees.at(b);
    CHECK(board_tree);
    ++g_play_trick_calls;
    std::shared_ptr<OptimalPlayTree> after_play =
        board_tree->PlayTrick(gen_trick.GetTrickForBoard(b), &cache);
    CHECK_EQ(0, after_play->MyBoard()->CardsPlayedThisTrick());
    after_play_trees[b] = after_play;
    int cur_player = after_play->MyBoard()->CurPlayer();
    leader_to_boards[cur_player].insert(b);
    VLOG(2) << "board " << b << " after " << gen_trick.GetTrickForBoard(b)
            << " winner " << cur_player << ", tricks won: "
            << after_play->MyBoard()->tricks_won_even();
  }
  ConditionalLine a_line;
  a_line.SetAction(gen_trick);
  // One sub-search per trick winner, weighted by the deals it handles.
  std::vector<std::pair<int, std::set<int>>> entries(
      leader_to_boards.begin(), leader_to_boards.end());
  std::vector<double> branch_weights;
  branch_weights.reserve(entries.size());
  for (const auto &[_next_leader, some_board_offsets] : entries) {
    branch_weights.push_back(some_board_offsets.size());
  }
  std::vector<SearchProgress::Node *> branch_progress =
      SearchProgress::AddChildren(progress, branch_weights);
  std::vector<std::vector<ConditionalLine>> branch_results(entries.size());
  if (omp_in_parallel() && entries.size() > 1 && lvl <= 7) {
    for (size_t idx = 0; idx < entries.size(); ++idx) {
      #pragma omp task firstprivate(idx) \
          shared(entries, branch_results, branch_progress, cache, \
                 after_play_trees)
      {
        int next_leader = entries[idx].first;
        int trick_won = (next_leader % 2 == 0) ? 1 : 0;
        branch_results[idx] = SolveHelper(
            min_tricks - trick_won, cache, after_play_trees,
            entries[idx].second, next_leader, lvl + 1, branch_progress[idx]);
      }
    }
    #pragma omp taskwait
  } else {
    for (size_t idx = 0; idx < entries.size(); ++idx) {
      int next_leader = entries[idx].first;
      int trick_won = (next_leader % 2 == 0) ? 1 : 0;
      VLOG(2) << "Recurse for leader " << next_leader;
      branch_results[idx] = SolveHelper(
          min_tricks - trick_won, cache, after_play_trees,
          entries[idx].second, next_leader, lvl + 1, branch_progress[idx]);
      VLOG(2) << "SolveHelper returns " << branch_results[idx].size()
              << " lines: " << min_tricks;
    }
  }
  for (size_t idx = 0; idx < entries.size(); ++idx) {
    int next_leader = entries[idx].first;
    const auto &some_board_offsets = entries[idx].second;
    const auto &x = branch_results[idx];
    if (x.size() >= 1 && x[0].CoveredFinalBoards() > 0) {
      std::set<std::string> winning_cards;
      for (int b : some_board_offsets) {
        std::string trick_str = gen_trick.GetTrickForBoard(b);
        std::vector<std::string> cards = SplitWords(trick_str);
        if (cards.size() == 4) {
          int card_idx = (next_leader - gen_trick.leader + 4) % 4;
          winning_cards.insert(cards[card_idx]);
        }
      }
      a_line.AddCondition(
          new WinnerPlayCondition(next_leader, winning_cards), x[0]);
    }
  }
  VLOG(2) << "Returning a_line with conditions " << a_line.num_conditions();
  return a_line;
}

namespace {

// Opponent patterns that carry no information: "__" (any card) and "R_" (any
// discard).
bool IsUninformativeOppPattern(const std::string &p) {
  return p == "__" || p == "R_";
}

// True if, on every board, neither opponent follows suit or ruffs and partner
// does not ruff, so the trick splits no deals apart: cashing a trump when the
// opponents are void is the typical case.
bool IsUninformativeTrick(const GeneralizedTrick &t, Suit trump_suit) {
  bool partner_ruffs =
      trump_suit != NOTRUMP &&
      Card::ParseSuit(t.demi1.my_card[0]) != trump_suit &&
      Card::ParseSuit(t.demi2.my_card[0]) == trump_suit;
  return !partner_ruffs && IsUninformativeOppPattern(t.demi1.opp_card) &&
         IsUninformativeOppPattern(t.demi2.opp_card);
}

// Relative cost estimate of evaluating a trick, for progress reporting. The
// sub-search grows with the deals it has to handle; a trick that leaves every
// deal in play (no winner or defender-card split) faces nearly the same
// problem as its parent one card shorter, so it is weighted up further.
double TrickProgressWeight(const GeneralizedTrick &t, Suit trump_suit) {
  constexpr double kUninformativeTrickFactor = 4.0;
  double weight = t.boards.size();
  if (t.leader % 2 == 0 && IsUninformativeTrick(t, trump_suit)) {
    weight *= kUninformativeTrickFactor;
  }
  return weight;
}

}  // namespace

std::vector<ConditionalLine> LineFinder::GetLinesFromGroups(
    const std::vector<std::vector<GeneralizedTrick>> &groupings,
    int min_tricks,
    SolverCache &cache,
    std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
    const std::set<int> &board_offsets,
    int leader, int lvl, SearchProgress::Node *progress) {
  VLOG(2) << "GetLinesFromGroups " << groupings.size();
  int group_num = 0;
  for (const auto &grp : groupings) {
    VLOG(2) << "See group " << group_num++ << ":";
    for (const auto &trick : grp) {
      VLOG(2) << "   " << trick.ToString();
    }
  }

  std::vector<int> group_possible_boards(groupings.size(), 0);
  // keys[i][k] identifies groupings[i][k] in saved_work; weights[i][k] is
  // its estimated share of the search for progress reporting.
  std::vector<std::vector<std::string>> keys(groupings.size());
  std::vector<std::vector<double>> weights(groupings.size());
  Suit trump_suit = board_offsets.empty()
                        ? NOTRUMP
                        : board_trees.at(*board_offsets.begin())
                              ->MyBoard()->GetTrump();
  for (size_t i = 0; i < groupings.size(); ++i) {
    keys[i].reserve(groupings[i].size());
    weights[i].reserve(groupings[i].size());
    for (const auto &trick : groupings[i]) {
      group_possible_boards[i] += trick.boards.size();
      keys[i].push_back(trick.ToString(true));
      weights[i].push_back(TrickProgressWeight(trick, trump_suit));
    }
  }

  std::multimap<int, ConditionalLine> best_lines;
  std::unordered_map<std::string, ConditionalLine> saved_work;
  // Below the top level only one best line is needed, so groups that can at
  // best tie the current coverage are skipped. At the top level ties are
  // evaluated so that equivalent alternative lines are all returned.
  bool skip_ties = (lvl > 0);
  int most_boards_covered = 0;

  auto is_pruned = [&](size_t j) {
    return group_possible_boards[j] < most_boards_covered ||
           (skip_ties && group_possible_boards[j] == most_boards_covered);
  };
  // Estimated size of each batch the groups after `i` may still need: for
  // every group that can beat the best coverage so far, the weight of its
  // tricks that are neither cached nor part of the batch being launched.
  // Groups that can cover the most deals come first, since they outlast the
  // rising bar longest; the estimate is an upper bound either way.
  auto remaining_batches = [&](size_t i,
                               const std::set<std::string> *in_batch) {
    std::vector<std::pair<int, double>> possible_and_size;
    std::set<std::string> counted;
    for (size_t j = i + 1; j < groupings.size(); ++j) {
      if (is_pruned(j)) continue;
      double size = 0.0;
      for (size_t k = 0; k < keys[j].size(); ++k) {
        const std::string &key = keys[j][k];
        if (saved_work.count(key) == 0 &&
            (in_batch == nullptr || in_batch->count(key) == 0) &&
            counted.insert(key).second) {
          size += weights[j][k];
        }
      }
      if (size > 0.0) {
        possible_and_size.emplace_back(group_possible_boards[j], size);
      }
    }
    std::stable_sort(possible_and_size.begin(), possible_and_size.end(),
                     [](const auto &a, const auto &b) {
                       return a.first > b.first;
                     });
    std::vector<double> sizes;
    sizes.reserve(possible_and_size.size());
    for (const auto &[_possible, size] : possible_and_size) {
      sizes.push_back(size);
    }
    return sizes;
  };
  // Evaluates `batch` (tricks not yet in saved_work) as one progress batch,
  // in parallel when inside the OpenMP region.
  auto run_batch = [&](
      size_t i,
      std::vector<std::pair<std::string, const GeneralizedTrick *>> &batch,
      const std::set<std::string> &batch_seen) {
    std::stable_sort(
        batch.begin(), batch.end(),
        [](const auto &a, const auto &b) {
          return a.second->boards.size() > b.second->boards.size();
        });
    std::vector<double> batch_weights;
    batch_weights.reserve(batch.size());
    for (const auto &[_key, trick] : batch) {
      batch_weights.push_back(TrickProgressWeight(*trick, trump_suit));
    }
    std::vector<SearchProgress::Node *> trick_progress =
        SearchProgress::BeginBatch(progress, batch_weights,
                                   remaining_batches(i, &batch_seen));
    std::vector<ConditionalLine> batch_results(batch.size());
    auto evaluate = [&](size_t b_idx) {
      batch_results[b_idx] = GetLineFromTrick(
          *batch[b_idx].second, min_tricks, cache, board_trees,
          board_offsets, leader, lvl, trick_progress[b_idx]);
      if (lvl == 0) {
        // A single first trick is itself a complete line, so the deals it
        // makes are a floor on the final coverage worth reporting right
        // away: the top-level batch can run for most of the solve.
        int covered = batch_results[b_idx].CoveredFinalBoards();
        int seen = g_best_covered.load();
        while (covered > seen &&
               !g_best_covered.compare_exchange_weak(seen, covered)) {
        }
      }
    };
    if (omp_in_parallel() && batch.size() > 1) {
      for (size_t b_idx = 0; b_idx < batch.size(); ++b_idx) {
        #pragma omp task firstprivate(b_idx) shared(evaluate)
        {
          evaluate(b_idx);
        }
      }
      #pragma omp taskwait
    } else {
      for (size_t b_idx = 0; b_idx < batch.size(); ++b_idx) {
        evaluate(b_idx);
      }
    }
    for (size_t b_idx = 0; b_idx < batch.size(); ++b_idx) {
      saved_work[batch[b_idx].first] = std::move(batch_results[b_idx]);
    }
    SearchProgress::EndBatch(progress, remaining_batches(i, nullptr));
  };

  for (uint i = 0; i < groupings.size(); i++) {
    const std::vector<GeneralizedTrick> &group = groupings[i];
    VLOG(2) << "Trying group " << i << " with tricks " << group.size();
    int most_possible_boards = group_possible_boards[i];
    VLOG(2) << "lvl " << lvl << " most_possible_boards " << most_possible_boards
            << " vs " << most_boards_covered;
    if (is_pruned(i)) {
      VLOG(2)<< "Skipping, it can only cover " << most_possible_boards
             << " vs " << most_boards_covered;
      continue;
    }

    if (lvl == 0) {
      g_best_covered = most_boards_covered;
    }

    std::vector<std::pair<std::string, const GeneralizedTrick *>> batch;
    std::set<std::string> batch_seen;
    for (size_t k = 0; k < group.size(); ++k) {
      const std::string &key = keys[i][k];
      if (saved_work.count(key) == 0 && batch_seen.count(key) == 0) {
        batch_seen.insert(key);
        batch.emplace_back(key, &group[k]);
      }
    }
    bool has_uncomputed = !batch.empty();

    if (omp_in_parallel() && has_uncomputed) {
      int max_possible_so_far = group_possible_boards[i];
      auto add_group = [&](size_t j) {
        for (size_t k = 0; k < groupings[j].size(); ++k) {
          const std::string &key = keys[j][k];
          if (saved_work.count(key) == 0 && batch_seen.count(key) == 0) {
            batch_seen.insert(key);
            batch.emplace_back(key, &groupings[j][k]);
          }
        }
      };
      if (lvl <= 7) {
        if ((lvl == 0 && i == 0 && board_offsets.size() >= 30) ||
            (lvl == 1 && i == 0 && board_offsets.size() >= 60)) {
          int max_possible = max_possible_so_far;
          for (int p : group_possible_boards) {
            max_possible = std::max(max_possible, p);
          }
          int thresh = max_possible - (max_possible / 20);
          for (size_t j = i + 1; j < groupings.size(); ++j) {
            if (group_possible_boards[j] >= thresh) {
              add_group(j);
            }
          }
        } else if (lvl <= 1 && i >= 1 &&
                   group_possible_boards[0] - most_boards_covered >= 5) {
          for (size_t j = i + 1; j < groupings.size(); ++j) {
            if (group_possible_boards[j] >= max_possible_so_far) {
              max_possible_so_far = group_possible_boards[j];
              add_group(j);
            }
          }
        } else {
          for (size_t j = i + 1; j < groupings.size(); ++j) {
            if (group_possible_boards[j] > max_possible_so_far) {
              max_possible_so_far = group_possible_boards[j];
              add_group(j);
            }
          }
        }
      }
    }
    if (has_uncomputed) {
      run_batch(i, batch, batch_seen);
    }

    // Make a line for this group
    ConditionalLine a_line;
    for (size_t k = 0; k < group.size(); ++k) {
      const std::string &trick_str = keys[i][k];
      a_line.AddCondition(new TrickPlayCondition(trick_str),
                          saved_work.at(trick_str));
    }
    int covered_boards = a_line.CoveredFinalBoards();
    if (covered_boards >= most_boards_covered) {
      bool improved = covered_boards > most_boards_covered;
      most_boards_covered = covered_boards;
      if (lvl == 0) {
        g_best_covered = most_boards_covered;
      }
      best_lines.insert(std::make_pair(covered_boards, a_line));
      VLOG(2) << "Found new best line at lvl "
              << lvl << " with " << covered_boards
              << " to get " << best_lines.size()
              << " of " << a_line.Describe();
      if (improved && progress != nullptr) {
        // A higher bar prunes more of the groups still to come.
        SearchProgress::SetRemaining(progress, remaining_batches(i, nullptr));
      }
    } else {
      VLOG(2) << "Only covered_boards " << covered_boards
              << " vs " << most_boards_covered;
    }
  }
  SearchProgress::SetRemaining(progress, {});
  std::vector<ConditionalLine> ans;
  for (const auto &[num_boards, a_line] : best_lines) {
    if (num_boards == most_boards_covered) {
      ans.push_back(a_line);
    }
  }
  VLOG(2) << "Returning lines " << ans.size() << " at lvl " << lvl;
  return ans;
}

using std::string;

bool WonEnoughTricks(int mini_max, int min_tricks, int leader,
                     int cards_left) {
  if (leader % 2 == 0) {
    return mini_max >= min_tricks;
  } else {
    return (cards_left - mini_max >= min_tricks);
  }
}

namespace {
// Returns declarer's "null move" leads at this node: cards whose lead, on
// every board, neither opponent ever follows suit or ruffs (so every opponent
// pattern is "__" or "R_") and which partner does not ruff. Cashing a trump
// when the opponents are void is the typical case. Such a trick gains no
// information about the layout and merely postpones the real decision.
std::set<std::string> FindNullMoveLeads(
    const std::vector<std::vector<GeneralizedTrick>> &groupings,
    Suit trump_suit) {
  std::set<std::string> uninformative;
  std::set<std::string> informative;
  for (const auto &group : groupings) {
    for (const GeneralizedTrick &t : group) {
      if (t.leader % 2 != 0) {
        continue;
      }
      if (IsUninformativeTrick(t, trump_suit)) {
        uninformative.insert(t.demi1.my_card);
      } else {
        informative.insert(t.demi1.my_card);
      }
    }
  }
  std::set<std::string> ans;
  for (const std::string &lead : uninformative) {
    if (informative.count(lead) == 0) {
      ans.insert(lead);
    }
  }
  return ans;
}
}  // namespace

void LineFinder::TryBranchOnGoodCards(
    int min_tricks,
    SolverCache &cache_in,
    std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
    const std::set<int> &solvable_boards,
    int cards_left, int leader, int lvl,
    std::vector<ConditionalLine> &ans,
    SearchProgress::Node *progress) {
  int best_covered = ans.empty() ? 0 : ans[0].CoveredFinalBoards();
  if (lvl <= 0 || leader % 2 != 0 ||
      best_covered >= static_cast<int>(solvable_boards.size()) ||
      solvable_boards.size() < 2 ||
      get_line_from_trick_calls.load(std::memory_order_relaxed) >= 5000) {
    return;
  }
  // The split searches count like one more batch of the trick groups the
  // node has already run.
  SearchProgress::Node *split_progress =
      SearchProgress::AddChildren(progress, {SearchProgress::kBatchDecay})[0];
  SearchProgress::Completer split_completer(split_progress);

  const BoardState *first_bs =
      board_trees.at(*solvable_boards.begin())->MyBoard();
  uint64_t mask = ~first_bs->played_cards();
  uint64_t decl_cards =
      (first_bs->GetHand(0) | first_bs->GetHand(2)) & mask;
  for (int suit = 0; suit < 4; ++suit) {
    uint16_t suit_bits = (decl_cards >> (suit * 16)) & 0xffff;
    if (suit_bits == 0) continue;
    // Check highest remaining card held by N/S in this suit.
    int top_val = 31 - __builtin_clz(suit_bits);
    CardType top_card =
        Card::GetSuitedCard(static_cast<Suit>(suit), top_val);
    IsCardGoodCondition cond_true(top_card, true);
    std::set<int> good_boards;
    std::set<int> not_good_boards;
    for (int b : solvable_boards) {
      if (cond_true.Evaluate(*board_trees.at(b)->MyBoard())) {
        good_boards.insert(b);
      } else {
        not_good_boards.insert(b);
      }
    }
    if (good_boards.empty() || not_good_boards.empty()) {
      continue;
    }
    std::vector<SearchProgress::Node *> half_progress =
        SearchProgress::AddChildren(
            split_progress, {static_cast<double>(good_boards.size()),
                             static_cast<double>(not_good_boards.size())});
    std::vector<ConditionalLine> good_lines =
        SolveHelper(min_tricks, cache_in, board_trees, good_boards,
                    leader, lvl + 1, half_progress[0]);
    int good_cov = good_lines.empty() ? 0 : good_lines[0].CoveredFinalBoards();
    if (good_lines.empty() ||
        good_cov + static_cast<int>(not_good_boards.size()) <= best_covered) {
      // SolveHelper completes the nodes it runs; this one never runs.
      SearchProgress::Complete(half_progress[1]);
      continue;
    }
    std::vector<ConditionalLine> not_good_lines =
        SolveHelper(min_tricks, cache_in, board_trees, not_good_boards,
                    leader, lvl + 1, half_progress[1]);
    if (not_good_lines.empty()) continue;
    int total_cov = good_cov + not_good_lines[0].CoveredFinalBoards();
    if (total_cov > best_covered) {
      ConditionalLine split_line;
      split_line.AddCondition(new IsCardGoodCondition(top_card, true),
                              good_lines[0]);
      split_line.AddCondition(new IsCardGoodCondition(top_card, false),
                              not_good_lines[0]);
      best_covered = total_cov;
      ans = {split_line};
    }
  }
}

std::vector<ConditionalLine> LineFinder::SolveHelper(
    int min_tricks,
    SolverCache &cache_in,
    std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
    const std::set<int> &board_offsets,
    int leader, int lvl, SearchProgress::Node *progress) {
  if (lvl <= 0 || board_offsets.empty() || !solve_memo_) {
    return SolveHelperUncached(min_tricks, cache_in, board_trees,
                               board_offsets, leader, lvl, progress);
  }
  std::string key;
  key.reserve(2 + board_offsets.size() * 12);
  key.push_back(static_cast<char>(leader));
  key.push_back(static_cast<char>(std::max(min_tricks, 0)));
  for (int b : board_offsets) {
    uint32_t bid = static_cast<uint32_t>(b);
    uint64_t played = board_trees.at(b)->MyBoard()->played_cards();
    key.append(reinterpret_cast<const char *>(&bid), sizeof(bid));
    key.append(reinterpret_cast<const char *>(&played), sizeof(played));
  }
  size_t shard = std::hash<std::string>{}(key) & (kMemoShards - 1);
  {
    std::lock_guard<std::mutex> lock(solve_memo_->mu[shard]);
    auto it = solve_memo_->shards[shard].find(key);
    if (it != solve_memo_->shards[shard].end()) {
      SearchProgress::Complete(progress);
      return it->second;
    }
  }
  std::vector<ConditionalLine> res = SolveHelperUncached(
      min_tricks, cache_in, board_trees, board_offsets, leader, lvl, progress);
  {
    std::lock_guard<std::mutex> lock(solve_memo_->mu[shard]);
    solve_memo_->shards[shard].emplace(std::move(key), res);
  }
  return res;
}

std::vector<ConditionalLine> LineFinder::SolveHelperUncached(
    int min_tricks,
    SolverCache &cache_in,
    // const std::vector<BoardState> &boards,
    std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
    const std::set<int> &board_offsets,
    int leader, int lvl, SearchProgress::Node *progress) {
  SearchProgress::Completer progress_completer(progress);
  CHECK_LT(lvl, 30);
  if (lvl < 3) {
    VLOG(1) << "SolveHelper lvl " << lvl << " to win " << min_tricks
            << " on boards " << board_trees.size()
            << " (" << StrJoin(board_offsets, ", ") << ")";
  }
  if (board_offsets.empty()) {
    return std::vector<ConditionalLine>();
  }
  Suit trump_suit =
      board_trees.at(*board_offsets.begin())->MyBoard()->GetTrump();
  GeneralizedTrickFinder gen_trick_finder(trump_suit);
  std::set<int> solvable_boards;
  int cards_left = 0;
  for (int b : board_offsets) {
    const std::shared_ptr<OptimalPlayTree> &play_tree = board_trees.at(b);
    CHECK(play_tree);
    cards_left = play_tree->CardsLeft();
    if (cards_left == 0) {
      VLOG(2) << "Out of cards with min_tricks " << min_tricks
              << play_tree->DebugString();
      if (min_tricks > 0) {
        // Failure
        return std::vector<ConditionalLine>();
      } else {
        // Success
        ConditionalLine line(board_offsets);
        return std::vector<ConditionalLine>({line});
      }
    }

    int mini_max = play_tree->MiniMax();
    if (!WonEnoughTricks(mini_max, min_tricks, leader, cards_left)) {
      VLOG(2) << "No solution for board " << b << " (" << mini_max
              << " vs " << min_tricks << ")";
      continue;
    }
    solvable_boards.insert(b);
    const OptimalLines &lines = play_tree->GetLines();
    for (const OptimalLine &trick : lines) {
      VLOG(3) << "    " << trick;
      gen_trick_finder.RecordOptimalTrick(leader, trick, b);
    }
  }
  uint kMaxTricks = 500;
  GeneralizedTricksVector gen_tricks =
      gen_trick_finder.GetGeneralizedTricks(kMaxTricks);
  if (gen_tricks.size() == kMaxTricks) {
    LOG(INFO) << "Hit kMaxTricks " << kMaxTricks;
  }
  if (gen_tricks.empty()) {
    if (lvl < 3) {
      VLOG(1) << "Giving up";
    }
    return std::vector<ConditionalLine>();
  }
  std::vector<std::vector<GeneralizedTrick>> groupings =
      GroupTricks(gen_tricks);

  // Null-move leads are double-dummy optimal on nearly every board, so they
  // always carry the largest upper bound and would otherwise be searched
  // first and win every coverage tie. Searching them last keeps them as
  // candidates (a squeeze or entry play may genuinely need them) but lets an
  // informative trick with equal coverage be preferred.
  std::set<std::string> null_leads = FindNullMoveLeads(groupings, trump_suit);
  if (!null_leads.empty()) {
    std::stable_partition(
        groupings.begin(), groupings.end(),
        [&null_leads](const std::vector<GeneralizedTrick> &g) {
          return g.empty() || g[0].leader % 2 != 0 ||
                 null_leads.count(g[0].demi1.my_card) == 0;
        });
  }

  VLOG(2) << "See groupings " << groupings.size();

  std::vector<ConditionalLine> ans =
      GetLinesFromGroups(groupings, min_tricks, cache_in, board_trees,
                         board_offsets, leader, lvl, progress);
  TryBranchOnGoodCards(min_tricks, cache_in, board_trees, solvable_boards,
                       cards_left, leader, lvl, ans, progress);
  return ans;
}
