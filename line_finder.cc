#include "line_finder.h"

#include <ctype.h>
#include "deal.h"
#include "interface_dds.h"

void FuzzyBoardState::SetHands(const std::vector<uint64_t> &hands,
                               uint64_t unknown_cards) {
  known_hands_ = hands;
  unknown_cards_ = unknown_cards;
}

BoardState FuzzyBoardState::Randomize(bool deal_missing_cards) {
  std::vector<uint64_t> hands = known_hands_;
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
    int leader, Suit trump) {
  CHECK_EQ(4, hands.size());
  fuzzy_state_.SetHands(hands, unknown_cards);
  std::set<int> board_offsets;
  std::vector<CardType> cur_trick;
  std::vector<BoardState> boards =
      SetupBoards(trump, leader, board_offsets, cur_trick,
                  /*deal_missing_cards=*/true);
  std::multiset<int> tricks_won;
  for (uint i = 0; i < boards.size(); i++) {
    int mini_max = FindOptimalTricks(boards[i]);
    tricks_won.insert(13 - mini_max);
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

static uint64_t get_line_from_trick_calls = 0;

std::vector<ConditionalLine> LineFinder::Solve(
    int min_tricks,
    const std::vector<uint64_t> &hands, uint64_t unknown_cards,
    int leader, Suit trump, const std::vector<CardType> &cur_trick) {
  CHECK_EQ(4, hands.size());
  LOG(INFO) << "Solving hands: \n\n"
            << "          N: " << HandDebugString(hands[0], true) << "\n\n"
            << " W: " << HandDebugString(hands[1], true)
            << " E: " << HandDebugString(hands[3], true)
            << "[" << HandDebugString(unknown_cards, true) << "]\n\n"
            << "          S: " << HandDebugString(hands[2], true)
            << "\n\nGoal: " << min_tricks << " tricks\n"
            << "Hands simulated: " << num_iters_ << "\n\n";
  fuzzy_state_.SetHands(hands, unknown_cards);
  std::set<int> board_offsets;
  std::vector<BoardState> boards =
      SetupBoards(trump, leader, board_offsets, cur_trick);

  SolverCache cache;
  std::map<int, std::shared_ptr<OptimalPlayTree>> board_trees;
  for (uint i = 0; i < boards.size(); i++) {
    board_trees[i] = OptimalPlayTree::FindOptimalPlayTree(
        boards[i], min_tricks, &cache);
    VLOG(1) << "Finished computing tree for board " << i
            << " with mini_max " << board_trees[i]->MiniMax();
  }

  auto ans = SolveHelper(min_tricks, cache, board_trees,
                         board_offsets, leader, 0);
  LOG(INFO) << "Solve finished with get_line_from_trick_calls "
            << get_line_from_trick_calls;
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
  get_line_from_trick_calls++;
  VLOG(2) << "Pursuing line satisfying boards " << gen_trick.boards.size();
  std::map<int, std::set<int>> leader_to_boards;
  VLOG(2) << "demi2.board_to_trick "
          << PrintBoardToTrick(gen_trick.demi2.board_to_trick);
  std::map<int, std::shared_ptr<OptimalPlayTree>> after_play_trees;
  for (int b : gen_trick.boards) {
    std::shared_ptr<OptimalPlayTree> board_tree = board_trees[b];
    CHECK(board_tree);
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
  for (const auto &[leader, some_board_offsets] : leader_to_boards) {
    int trick_won = (leader % 2 == 0) ? 1 : 0;
    VLOG(2) << "Recurse for leader " << leader;
    std::vector<ConditionalLine> x =
        SolveHelper(min_tricks - trick_won, cache,
                    after_play_trees, some_board_offsets, leader, lvl + 1);
    VLOG(2) << "SolveHelper returns " << x.size() << " lines: " << min_tricks;
    // CHECK_LE(x.size(), 1);
    // For now, let's just take the first one returned.
    if (x.size() >= 1 && x[0].CoveredFinalBoards() > 0) {
      a_line.AddCondition(new WinnerPlayCondition(leader), x[0]);
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

  std::multimap<int, ConditionalLine> best_lines;
  std::unordered_map<std::string, ConditionalLine> saved_work;
  int ONLY_ONE_BEST_LINE = 1;
  int most_boards_covered = 0;
  for (uint i = 0; i < groupings.size(); i++) {
    const std::vector<GeneralizedTrick> &group = groupings[i];
    VLOG(2) << "Trying group " << i << " with tricks " << group.size();
    int most_possible_boards = 0;
    for (const auto &trick : group) {
      most_possible_boards += trick.boards.size();
    }
    VLOG(2) << "lvl " << lvl << " most_possible_boards " << most_possible_boards
            << " vs " << most_boards_covered;
    if (most_possible_boards < most_boards_covered ||
        (ONLY_ONE_BEST_LINE && most_possible_boards == most_boards_covered)) {
      VLOG(2)<< "Skipping, it can only cover " << most_possible_boards
             << " vs " << most_boards_covered;
      continue;
    }

    // Make a line for this group
    ConditionalLine a_line;
    for (const GeneralizedTrick &trick : group) {
      std::string trick_str = trick.ToString(true);
      ConditionalLine trick_line;
      if (saved_work.count(trick_str)) {
        trick_line = saved_work[trick_str];
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

bool WonEnoughTricks(int mini_max, int min_tricks, int leader) {
  if (leader % 2 == 0) {
    return mini_max >= min_tricks;
  } else {
    return (13 - mini_max >= min_tricks);
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
  std::map<int, OptimalLines> board_lines;
  std::set<int> impossible_boards;
  Suit trump_suit = NOTRUMP;
  for (int b : board_offsets) {
    std::shared_ptr<OptimalPlayTree> &play_tree = board_trees[b];
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

    trump_suit = play_tree->MyBoard()->GetTrump();
    int mini_max = play_tree->MiniMax();
    // if (mini_max < min_tricks) {
    if (!WonEnoughTricks(mini_max, min_tricks, leader)) {
      impossible_boards.insert(b);
      VLOG(2) << "No solution for board " << b << " (" << mini_max
              << " vs " << min_tricks << ")";
      continue;
    }
    OptimalLines lines = play_tree->GetLines();
#if 0
    if (lvl < 3) {
      VLOG(1) << "MostTricksCanBeWon for " << b << " is " << mini_max
              << " with choices " << lines.size()
              << ": " << boards[b].DebugString()
              << ": " << (lines.empty() ? std::string("") : *lines.begin())
              << ", " << (lines.size() > 1 ? *(++lines.begin()) : string());
    }
#endif
    for (const auto &line : lines) {
      VLOG(3) << "    " << line;
    }
    // TODO: Don't really have to copy to board_lines.
    board_lines[b] = lines;
  }

  GeneralizedTrickFinder gen_trick_finder(trump_suit);
  std::set<int> solvable_boards;
  for (int b : board_offsets) {
    if (impossible_boards.count(b) > 0) { continue; }
    solvable_boards.insert(b);
    CHECK(board_lines.count(b) > 0) << b;
    for (const OptimalLine &trick : board_lines[b]) {
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
