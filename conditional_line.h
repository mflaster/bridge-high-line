#pragma once

#include <memory>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include "board_state.h"
#include "card.h"
#include "hand.h"

struct GeneralizedDemiStruct {
  std::string opp_card;
  std::string my_card;
  std::set<int> boards;
  std::map<int, std::string> board_to_trick;

  GeneralizedDemiStruct(const std::string &opp,
                        const std::string &me,
                        const std::set<int> &b)
      : opp_card(opp), my_card(me), boards(b) {}

  GeneralizedDemiStruct() {}
};

using GeneralizedPlaysVector = std::vector<GeneralizedDemiStruct>;

class GeneralizedDemiPlay {
 public:
  GeneralizedDemiPlay(Suit trump_suit)
      : trump_suit_(trump_suit) {
  }
#if 0
  GeneralizedDemiPlay()
      : trump_suit_(NOTRUMP) {
  }
#endif

  void RecordOptimalPlay(const std::string &opp_card,
                         const std::string &my_card,
                         int board_num,
                         const std::string *trick = nullptr);

  GeneralizedPlaysVector GetGeneralizedPlays(
      int leader, Suit suit_led, unsigned int top = 10);

  static std::string PrintGeneralizedPlays(const GeneralizedPlaysVector &plays);

 private:
  std::map<std::string,
     std::map<std::string, std::set<int>>> opp_to_card_to_boards_;

  std::map<std::string,
      std::map<std::string, std::map<int, std::string>>> saved_tricks_;
  Suit trump_suit_;
};

struct GeneralizedTrick {
  GeneralizedDemiStruct demi1;
  GeneralizedDemiStruct demi2;
  std::set<int> boards;
  int leader = 0;
  GeneralizedTrick(const GeneralizedDemiStruct &d1,
                   const GeneralizedDemiStruct &d2,
                   const std::set<int> &b,
                   int ldr)
      : demi1(d1), demi2(d2), boards(b), leader(ldr) {}

  GeneralizedTrick() {}

  bool Empty() const { return boards.empty(); }
  std::string ToString(bool compact = false) const;

  std::string GetTrickForBoard(int b) const {
    auto it = demi2.board_to_trick.find(b);
    CHECK(it != demi2.board_to_trick.end());
    return it->second;
  }
};

using GeneralizedTricksVector = std::vector<GeneralizedTrick>;

class GeneralizedTrickFinder {
 public:
  GeneralizedTrickFinder(Suit trump_suit)
      : trump_suit_(trump_suit) {
  }

  void RecordOptimalTrick(int leader, const std::string &trick, int board);
  GeneralizedTricksVector GetGeneralizedTricks(unsigned int top = 10);

  static std::string PrintGeneralizedTricks(
      const GeneralizedTricksVector &plays);

 private:
  std::vector<std::pair<std::vector<std::string>, int>> tricks_;
  std::vector<std::string> raw_tricks_;
  int leader_ = -1;
  Suit trump_suit_ = NOTRUMP;
};

class PlayCondition {
 public:
  PlayCondition() = default;
  virtual ~PlayCondition() = default;

  virtual std::string Describe(bool only_choice) const = 0;
};

class ConditionalLine {  // ConditionalLineBranch
 public:
  ConditionalLine() {}
  explicit ConditionalLine(const std::set<int> &boards)
      : final_covered_boards_(boards) {}
  void AddCondition(PlayCondition *c,
                    const ConditionalLine &continue_with) {
    CHECK(final_covered_boards_.empty());
    conditions_.emplace_back(std::shared_ptr<PlayCondition>(c),
                             std::shared_ptr<ConditionalLine>(
                                 new ConditionalLine(continue_with)));
  }

  void SetAction(const GeneralizedTrick &gt) { do_first_ = gt; }

  std::string Describe(int lvl = 0) const;
  int num_conditions() const { return conditions_.size(); }

  int CoveredBoards() const { return CoveredFinalBoards(); }
  std::set<int> CoveredBoardsSet() const { return CoveredFinalBoardsSet(); }

  int total_boards() const { return total_boards_; }
  void set_total_boards(int n) { total_boards_ = n; }

  int CoveredFinalBoards() const;
  std::set<int> CoveredFinalBoardsSet() const;

  const GeneralizedTrick &do_first() const { return do_first_; }
  const std::vector<std::pair<std::shared_ptr<PlayCondition>,
                             std::shared_ptr<ConditionalLine>>> &
  conditions() const {
    return conditions_;
  }
  const std::set<int> &final_covered_boards() const {
    return final_covered_boards_;
  }

 private:
  GeneralizedTrick do_first_;

  std::vector<std::pair<std::shared_ptr<PlayCondition>,
                       std::shared_ptr<ConditionalLine>>>
      conditions_;

  std::set<int> final_covered_boards_;
  int total_boards_ = 0;
};

class WinnerPlayCondition : public PlayCondition {
 public:
  WinnerPlayCondition(int who) : who_(who) {}

  std::string Describe(bool only_choice) const override;

 private:
  int who_;
};

class TrickPlayCondition : public PlayCondition {
 public:
  TrickPlayCondition(const std::string &trick) : trick_(trick) {}

  std::string Describe(bool only_choice) const override;

 private:
  std::string trick_;
};

std::vector<std::vector<GeneralizedTrick>> GroupTricks(
    const std::vector<GeneralizedTrick> &input);
