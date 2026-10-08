#include "line_finder.h"

#include <ctype.h>
#include <omp.h>
#include <algorithm>
#include <atomic>
#include <chrono>
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
std::atomic<int> g_top_group_idx{0};
std::atomic<int> g_top_group_total{0};
std::atomic<int> g_best_covered{0};
std::atomic<int> g_total_boards{0};
}  // namespace

std::string LineFinder::GetSolveStatus() {
  std::lock_guard<std::mutex> lock(g_status_mutex);
  return g_solve_status;
}

void LineFinder::SetSolveStatus(const std::string &status) {
  std::lock_guard<std::mutex> lock(g_status_mutex);
  g_solve_status = status;
}

static std::atomic<uint64_t> get_line_from_trick_calls{0};
static std::atomic<uint64_t> g_play_trick_calls{0};

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
  g_top_group_idx = 0;
  g_top_group_total = 0;
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
  std::vector<ConditionalLine> ans;
  if (effective_threads > 1) {
    #pragma omp parallel num_threads(effective_threads)
    {
      #pragma omp single
      {
        ans = SolveHelper(min_tricks, cache, board_trees,
                          board_offsets, leader, 0);
      }
    }
  } else {
    ans = SolveHelper(min_tricks, cache, board_trees,
                      board_offsets, leader, 0);
  }
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
    if (!covered.empty()) {
      std::vector<PairedDeal> covered_deals;
      covered_deals.reserve(covered.size());
      for (int b_idx : covered) {
        if (b_idx >= 0 && b_idx < static_cast<int>(boards.size())) {
          covered_deals.emplace_back(boards[b_idx].GetHand(1),
                                     boards[b_idx].GetHand(3));
        }
      }
      if (!covered_deals.empty()) {
        SetSolveStatus("Summarizing deal invariants (line " +
                       std::to_string(line_idx + 1) + "/" +
                       std::to_string(ans.size()) + ")...");
        try {
          line.set_summary(
              summarizer.Summarize(covered_deals, "hand 1", "hand 3", &line));
        } catch (const std::exception &e) {
          LOG(WARNING) << "Failed to summarize line: " << e.what();
        }
      }
    }

    // Also summarize non-covered (failure) boards if any exist
    std::vector<PairedDeal> failure_deals;
    for (int b_idx = 0; b_idx < static_cast<int>(boards.size()); ++b_idx) {
      if (covered.find(b_idx) == covered.end()) {
        failure_deals.emplace_back(boards[b_idx].GetHand(1),
                                   boards[b_idx].GetHand(3));
      }
    }
    if (!failure_deals.empty()) {
      SetSolveStatus("Summarizing failure invariants (line " +
                     std::to_string(line_idx + 1) + "/" +
                     std::to_string(ans.size()) + ")...");
      try {
        line.set_failure_summary(
            summarizer.Summarize(failure_deals, "hand 1", "hand 3"));
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

  SetSolveStatus("Solve complete.");
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
    int leader, int lvl) {
  uint64_t calls = ++get_line_from_trick_calls;
  if ((calls & 31) == 0 && g_top_group_total.load() > 0) {
    SetSolveStatus("Searching play trees: group " +
                   std::to_string(g_top_group_idx.load()) + "/" +
                   std::to_string(g_top_group_total.load()) +
                   " (best coverage: " +
                   std::to_string(g_best_covered.load()) + "/" +
                   std::to_string(g_total_boards.load()) + " deals, " +
                   std::to_string(calls) +
                   " branches)...");
  }
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
  if (omp_in_parallel() && leader_to_boards.size() > 1 && lvl <= 7) {
    std::vector<std::pair<int, std::set<int>>> entries(
        leader_to_boards.begin(), leader_to_boards.end());
    std::vector<std::vector<ConditionalLine>> branch_results(entries.size());
    for (size_t idx = 0; idx < entries.size(); ++idx) {
      #pragma omp task firstprivate(idx) \
          shared(entries, branch_results, cache, after_play_trees)
      {
        int next_leader = entries[idx].first;
        int trick_won = (next_leader % 2 == 0) ? 1 : 0;
        branch_results[idx] = SolveHelper(
            min_tricks - trick_won, cache, after_play_trees,
            entries[idx].second, next_leader, lvl + 1);
      }
    }
    #pragma omp taskwait
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
  } else {
    for (const auto &[next_leader, some_board_offsets] : leader_to_boards) {
      int trick_won = (next_leader % 2 == 0) ? 1 : 0;
      VLOG(2) << "Recurse for leader " << next_leader;
      std::vector<ConditionalLine> x =
          SolveHelper(min_tricks - trick_won, cache,
                      after_play_trees, some_board_offsets, next_leader,
                      lvl + 1);
      VLOG(2) << "SolveHelper returns " << x.size() << " lines: " << min_tricks;
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
  }
  VLOG(2) << "Returning a_line with conditions " << a_line.num_conditions();
  return a_line;
}

std::vector<ConditionalLine> LineFinder::GetLinesFromGroups(
    const std::vector<std::vector<GeneralizedTrick>> &groupings,
    int min_tricks,
    SolverCache &cache,
    std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
    const std::set<int> &board_offsets,
    int leader, int lvl) {
  VLOG(2) << "GetLinesFromGroups " << groupings.size();
  int group_num = 0;
  for (const auto &grp : groupings) {
    VLOG(2) << "See group " << group_num++ << ":";
    for (const auto &trick : grp) {
      VLOG(2) << "   " << trick.ToString();
    }
  }

  std::vector<int> group_possible_boards(groupings.size(), 0);
  for (size_t i = 0; i < groupings.size(); ++i) {
    for (const auto &trick : groupings[i]) {
      group_possible_boards[i] += trick.boards.size();
    }
  }

  std::multimap<int, ConditionalLine> best_lines;
  std::unordered_map<std::string, ConditionalLine> saved_work;
  int ONLY_ONE_BEST_LINE = 1;
  int most_boards_covered = 0;
  for (uint i = 0; i < groupings.size(); i++) {
    const std::vector<GeneralizedTrick> &group = groupings[i];
    VLOG(2) << "Trying group " << i << " with tricks " << group.size();
    int most_possible_boards = group_possible_boards[i];
    VLOG(2) << "lvl " << lvl << " most_possible_boards " << most_possible_boards
            << " vs " << most_boards_covered;
    if (most_possible_boards < most_boards_covered ||
        (ONLY_ONE_BEST_LINE && most_possible_boards == most_boards_covered)) {
      VLOG(2)<< "Skipping, it can only cover " << most_possible_boards
             << " vs " << most_boards_covered;
      continue;
    }

    if (lvl == 0) {
      g_top_group_idx = i + 1;
      g_top_group_total = groupings.size();
      g_best_covered = most_boards_covered;
      SetSolveStatus("Searching play trees: group " +
                     std::to_string(g_top_group_idx.load()) + "/" +
                     std::to_string(g_top_group_total.load()) +
                     " (best coverage: " +
                     std::to_string(g_best_covered.load()) + "/" +
                     std::to_string(g_total_boards.load()) + " deals, " +
                     std::to_string(get_line_from_trick_calls.load()) +
                     " branches)...");
    }

    // Check if any trick in this group is not yet in saved_work.
    bool has_uncomputed = false;
    for (const GeneralizedTrick &trick : group) {
      if (saved_work.count(trick.ToString(true)) == 0) {
        has_uncomputed = true;
        break;
      }
    }

    if (omp_in_parallel() && has_uncomputed) {
      std::vector<std::pair<std::string, const GeneralizedTrick *>> batch;
      std::set<std::string> batch_seen;
      int max_possible_so_far = group_possible_boards[i];
      for (const GeneralizedTrick &t : groupings[i]) {
        std::string t_str = t.ToString(true);
        if (saved_work.count(t_str) == 0 && batch_seen.count(t_str) == 0) {
          batch_seen.insert(t_str);
          batch.emplace_back(std::move(t_str), &t);
        }
      }
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
              for (const GeneralizedTrick &t : groupings[j]) {
                std::string t_str = t.ToString(true);
                if (saved_work.count(t_str) == 0 &&
                    batch_seen.count(t_str) == 0) {
                  batch_seen.insert(t_str);
                  batch.emplace_back(std::move(t_str), &t);
                }
              }
            }
          }
        } else if (lvl <= 1 && i >= 1 &&
                   group_possible_boards[0] - most_boards_covered >= 5) {
          for (size_t j = i + 1; j < groupings.size(); ++j) {
            if (group_possible_boards[j] >= max_possible_so_far) {
              max_possible_so_far = group_possible_boards[j];
              for (const GeneralizedTrick &t : groupings[j]) {
                std::string t_str = t.ToString(true);
                if (saved_work.count(t_str) == 0 &&
                    batch_seen.count(t_str) == 0) {
                  batch_seen.insert(t_str);
                  batch.emplace_back(std::move(t_str), &t);
                }
              }
            }
          }
        } else {
          for (size_t j = i + 1; j < groupings.size(); ++j) {
            if (group_possible_boards[j] > max_possible_so_far) {
              max_possible_so_far = group_possible_boards[j];
              for (const GeneralizedTrick &t : groupings[j]) {
                std::string t_str = t.ToString(true);
                if (saved_work.count(t_str) == 0 &&
                    batch_seen.count(t_str) == 0) {
                  batch_seen.insert(t_str);
                  batch.emplace_back(std::move(t_str), &t);
                }
              }
            }
          }
        }
      }

      if (batch.size() == 1) {
        saved_work[batch[0].first] =
            GetLineFromTrick(*batch[0].second, min_tricks, cache, board_trees,
                             board_offsets, leader, lvl);
      } else if (batch.size() > 1) {
        std::stable_sort(
            batch.begin(), batch.end(),
            [](const auto &a, const auto &b) {
              return a.second->boards.size() > b.second->boards.size();
            });
        std::vector<ConditionalLine> batch_results(batch.size());
        for (size_t b_idx = 0; b_idx < batch.size(); ++b_idx) {
          #pragma omp task firstprivate(b_idx) \
              shared(batch, batch_results, cache, board_trees, board_offsets)
          {
            batch_results[b_idx] = GetLineFromTrick(
                *batch[b_idx].second, min_tricks, cache, board_trees,
                board_offsets, leader, lvl);
          }
        }
        #pragma omp taskwait
        for (size_t b_idx = 0; b_idx < batch.size(); ++b_idx) {
          saved_work[batch[b_idx].first] = std::move(batch_results[b_idx]);
        }
      }
    }

    // Make a line for this group
    ConditionalLine a_line;
    for (const GeneralizedTrick &trick : group) {
      std::string trick_str = trick.ToString(true);
      ConditionalLine trick_line;
      auto sw_it = saved_work.find(trick_str);
      if (sw_it != saved_work.end()) {
        trick_line = sw_it->second;
      } else {
        trick_line = GetLineFromTrick(trick, min_tricks, cache, board_trees,
                                      board_offsets, leader, lvl);
        saved_work[trick_str] = trick_line;
      }
      a_line.AddCondition(new TrickPlayCondition(trick_str),
                          std::move(trick_line));
    }
    int covered_boards = a_line.CoveredFinalBoards();
    if (covered_boards >= most_boards_covered) {
      most_boards_covered = covered_boards;
      if (lvl == 0) {
        g_best_covered = most_boards_covered;
      }
      best_lines.insert(std::make_pair(covered_boards, a_line));
      VLOG(2) << "Found new best line at lvl "
              << lvl << " with " << covered_boards
              << " to get " << best_lines.size()
              << " of " << a_line.Describe();
    } else {
      VLOG(2) << "Only covered_boards " << covered_boards
              << " vs " << most_boards_covered;
    }
  }
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

std::vector<ConditionalLine> LineFinder::SolveHelper(
    int min_tricks,
    SolverCache &cache_in,
    // const std::vector<BoardState> &boards,
    std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
    const std::set<int> &board_offsets,
    int leader, int lvl) {
  CHECK_LT(lvl, 15);
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
  for (int b : board_offsets) {
    const std::shared_ptr<OptimalPlayTree> &play_tree = board_trees.at(b);
    CHECK(play_tree);
    int cards_left = play_tree->CardsLeft();
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

  VLOG(2) << "See groupings " << groupings.size();

  return GetLinesFromGroups(groupings, min_tricks, cache_in, board_trees,
                            board_offsets, leader, lvl);
}
