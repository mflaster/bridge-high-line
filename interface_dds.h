#pragma once

#include <memory>
#include <unordered_map>

#include "dll.h"
#include "board_state.h"
#include "card.h"
#include "hand.h"
#include "solver.h"

void PopulatePBNDeal(const BoardState &bs, dealPBN *d1);

int DDSFindOptimalLines(const BoardState &bs, int min_tricks,
                        OptimalLines *lines, SolverCache *cache);

std::vector<std::string> FindBestCardsToPlay(
    const BoardState &bs, int min_tricks);

int FindOptimalTricks(const BoardState &bs);

std::vector<std::pair<int, OptimalLines>>
    DDSParallelSolve(const std::vector<BoardState> &boards_to_run);

class OptimalPlayTree {
 public:
  OptimalPlayTree() = default;

  OptimalPlayTree(int mm, const BoardState &bs, int tr, int mt)
      : my_bs(bs),
        tricks_remaining(tr),
        min_tricks(mt),
        mini_max(mm) {}

  static std::shared_ptr<OptimalPlayTree> FindOptimalPlayTree(
      const BoardState &bs, int min_tricks, SolverCache *cache = nullptr,
      int deepen_levels = 0);


  void AddLine(const OptimalLine &line) {
    all_plays[line] = nullptr;
  }

  void Deepen(int num_levels, SolverCache *cache);
  std::shared_ptr<OptimalPlayTree> Deepen(
      const OptimalLine &trick, SolverCache *cache);

  size_t TotalCount() {
    size_t ans = 0;
    for (const auto &[_line, others] : all_plays) {
      ans++;
      if (others.get()) {
        ans += others->TotalCount();
      }
    }
    return ans;
  }

  std::string DebugString(int indent = 1) const {
    std::string ans;
    for (const auto &[line, others] : all_plays) {
      ans += "\n";
      for (int i = 0; i < 2 * indent; i++) {
        ans += ' ';
      }
      ans += line;
      if (others.get()) {
        ans += others->DebugString(indent + 1);
      }
    }
    return ans;
  }

  std::shared_ptr<OptimalPlayTree> PlayTrick(const OptimalLine &trick,
                                             SolverCache *cache);

  int CardsLeft() const { return tricks_remaining; }
  int MiniMax() const { return mini_max; }
  BoardState *MyBoard() { return &my_bs; }
  OptimalLines GetLines();

 private:
  std::unordered_map<OptimalLine, std::shared_ptr<OptimalPlayTree>> all_plays;
  BoardState my_bs;
  int tricks_remaining = 0;
  int min_tricks = 0;
  int mini_max = 0;
};
