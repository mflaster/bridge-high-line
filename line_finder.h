#pragma once

#include <map>
#include <memory>
#include <set>

#include "conditional_line.h"
#include "interface_dds.h"
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
  }
  void SetRequirements3(
      const std::vector<std::shared_ptr<HandRequirements>> &reqs) {
    hand_reqs3_ = reqs;
  }
  void SetRequirements(
      const std::vector<std::vector<std::shared_ptr<HandRequirements>>> &r) {
    all_hand_reqs_ = r;
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
  LineFinder(int num_iters) : num_iters_(num_iters) {}

  void SetRequirements(
      const std::vector<std::shared_ptr<HandRequirements>> &reqs1,
      const std::vector<std::shared_ptr<HandRequirements>> &reqs3) {
    fuzzy_state_.SetRequirements1(reqs1);
    fuzzy_state_.SetRequirements3(reqs3);
  }
  void SetRequirements(
      const std::vector<std::vector<std::shared_ptr<HandRequirements>>> &r) {
    fuzzy_state_.SetRequirements(r);
  }

  std::vector<ConditionalLine> Solve(
      int min_tricks,
      const std::vector<uint64_t> &hands, uint64_t unknown_cards,
      int leader, Suit trump, const std::vector<CardType> &cur_trick = {});

  std::set<std::string> FindBestLead(
      int min_tricks,
      const std::vector<uint64_t> &hands, uint64_t unknown_cards,
      int leader, Suit trump);

  std::multiset<int> FindTricksWon(
      const std::vector<uint64_t> &hands, uint64_t unknown_cards,
      int leader, Suit trump);

  static std::string TricksWonDebugString(const std::multiset<int> &tricks);
  static int CountAtLeast(const std::multiset<int> &data, int thresh);

 protected:
  std::vector<BoardState> SetupBoards(
      Suit trump, int leader, std::set<int> &board_offsets,
      const std::vector<CardType> &cur_trick,
      bool deal_missing_cards = false);


  ConditionalLine GetLineFromTrick(
      const GeneralizedTrick &gen_trick,
      int min_tricks,
      SolverCache &cache,
      std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
      const std::set<int> &board_offsets,
      int leader, int lvl);

  std::vector<ConditionalLine> GetLinesFromGroups(
      const std::vector<std::vector<GeneralizedTrick>> &groupings,
      int min_tricks,
      SolverCache &cache,
      std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
      const std::set<int> &board_offsets,
      int leader, int lvl);

  std::vector<ConditionalLine> SolveHelper(
      int min_tricks,
      SolverCache &cache,
      std::map<int, std::shared_ptr<OptimalPlayTree>> &board_trees,
      const std::set<int> &board_offsets,
      int leader, int lvl);

 private:
  int num_iters_;
  FuzzyBoardState fuzzy_state_;
};
