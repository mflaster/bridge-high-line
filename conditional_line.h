#pragma once

#include <array>
#include <cassert>
#include <memory>
#include <map>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>
#include "board_state.h"
#include "card.h"
#include "cross_hand_summarizer.h"
#include "hand.h"

struct GeneralizedDemiStruct {
  std::string opp_card;
  std::string my_card;
  std::set<int> boards;
  std::vector<int> boards_vec;
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
      int leader, Suit suit_led, unsigned int top = 10,
      bool populate_boards_set = true);

  static std::string PrintGeneralizedPlays(const GeneralizedPlaysVector &plays);

 private:
  struct RawPlayBucket {
    Suit suit_led;
    std::string opp_card;
    std::string my_card;
    std::vector<int> boards;
  };
  std::vector<RawPlayBucket> buckets_;
  bool has_by_suit_ = false;
  Suit trump_suit_;
};

struct GeneralizedTrick {
  struct BoardTrickEntry {
    int board = 0;
    uint8_t winner = 0;
    char win_card[3] = {0, 0, 0};
    uint64_t trick_mask = 0;
    std::string trick_str;
  };

  GeneralizedDemiStruct demi1;
  GeneralizedDemiStruct demi2;
  std::set<int> boards;
  std::vector<BoardTrickEntry> board_entries;
  int leader = 0;
  size_t opp1_step1_boards = 0;
  size_t opp2_step1_boards = 0;
  size_t opp1_unmakeable_boards = 0;
  size_t opp2_unmakeable_boards = 0;
  std::array<size_t, 4> winner_unmakeable_boards = {};
  GeneralizedTrick(const GeneralizedDemiStruct &d1,
                   const GeneralizedDemiStruct &d2,
                   const std::set<int> &b,
                   int ldr)
      : demi1(d1), demi2(d2), boards(b), leader(ldr) {}

  GeneralizedTrick() {}

  size_t NumBoards() const {
    return !demi2.boards_vec.empty() ? demi2.boards_vec.size() : boards.size();
  }
  bool HasBoardTricks() const {
    return !board_entries.empty() || !demi2.board_to_trick.empty();
  }
  bool Empty() const { return boards.empty() && demi2.boards_vec.empty(); }
  void PopulateLegacyMaps();
  std::string ToString(bool compact = false) const;

  std::string GetTrickForBoard(int b) const;
};

using GeneralizedTricksVector = std::vector<GeneralizedTrick>;

class GeneralizedTrickFinder {
 public:
  GeneralizedTrickFinder(Suit trump_suit)
      : trump_suit_(trump_suit) {
  }

  void RecordOptimalTrick(int leader, const std::string &trick, int board);
  GeneralizedTricksVector GetGeneralizedTricks(
      unsigned int top = 10, bool populate_boards_set = true);

  static std::string PrintGeneralizedTricks(
      const GeneralizedTricksVector &plays);

 private:
  struct RecordedTrick {
    std::array<std::string, 4> cards;
    int board = 0;
    uint8_t winner = 0;
    uint8_t win_card_idx = 0;
    Suit suit_led = NOTRUMP;
    uint64_t trick_mask = 0;
  };
  GeneralizedTrick BuildGeneralizedTrick(
      const GeneralizedDemiStruct &d1,
      const GeneralizedDemiStruct &d2,
      const std::vector<unsigned int> *d1_trick_indices,
      size_t opp1_step1_boards,
      bool populate_boards_set) const;

  std::vector<RecordedTrick> tricks_;
  std::vector<std::string> raw_tricks_;
  int leader_ = -1;
  Suit trump_suit_ = NOTRUMP;
  bool monotonic_boards_ = true;
};

// Abstract base class representing a conditional branching rule in a ConditionalLine.
// Evaluates conditions such as trick outcomes (WinnerPlayCondition) or candidate initial play choices (TrickPlayCondition).
class PlayCondition {
 public:
  PlayCondition() = default;
  virtual ~PlayCondition() = default;

  // Formats human-readable description of condition (e.g. "when 0 wins", "after S2 (S9) SA (S3)").
  virtual std::string Describe(bool only_choice) const = 0;
};

// ConditionalLine represents a node in a decision tree that encodes an optimal (or candidate optimal)
// line of play across a set of simulated bridge deals (boards).
//
// Decision Tree Node Taxonomy & Structure:
//
// 1. Top-level Root Node (returned by LineFinder::Solve):
//    - do_first_: Empty (do_first_.Empty() == true).
//    - conditions_: Contains TrickPlayCondition entries pairing a candidate initial trick line
//      description (e.g. "after S2 (S9) SA (S3)") with child ConditionalLine nodes.
//    - final_covered_boards_: Empty.
//
// 2. Intermediate Action Node (created in GetLineFromTrick for each trick):
//    - do_first_: Non-empty GeneralizedTrick representing the action (trick) to execute next.
//    - conditions_: Contains WinnerPlayCondition entries (e.g. "when 0 wins" / "if 1 wins")
//      branching on which player/side won the trick, pointing to the continuation subtrees for subsequent tricks.
//    - final_covered_boards_: Empty.
//
// 3. Leaf Node (Terminal Success Node):
//    - do_first_: Empty.
//    - conditions_: Empty (conditions_.empty() == true).
//    - final_covered_boards_: Populated with the set of board IDs (simulated deals) that successfully
//      achieved the required minimum tricks (min_tricks) once all cards/tricks were played.
//
// Multiple Returned Lines in LineFinder::Solve():
//    LineFinder::Solve() returns a std::vector<ConditionalLine> containing all lines that tie for the
//    maximum possible number of covered boards (most_boards_covered). All returned lines are equivalent
//    in terms of total board success count, but each represents a distinct alternative strategic play choice
//    (e.g., leading a different suit or card or following a different branch grouping).
//
// Tree Depth vs. Remaining Tricks / Cards Held:
//    For a deal with N cards held per hand (N tricks to be played):
//    - There are exactly N intermediate action nodes (nodes with do_first_ set) along any path from root to leaf.
//    - Total node path length from root to leaf is N + 2 nodes:
//        [Root Node] -> [Trick 1 Action Node] -> [Trick 2 Action Node] -> ... -> [Trick N Action Node] -> [Leaf Node]
//    - Measuring depth where a leaf node (conditions_.empty()) has depth = 1:
//        - Leaf Node: depth = 1
//        - Action Node for Trick k (from last trick): depth = N - k + 2  (Trick N has depth 2, Trick 1 has depth N + 1)
//        - Root Node: depth = N + 2
//    - Thus, the number of trick action nodes (do_first_ present) along any root-to-leaf path is EXACTLY
//      equal to N, the number of tricks / cards held to be played.
class ConditionalLine {  // ConditionalLineBranch
 public:
  // Default constructor creates an empty node.
  ConditionalLine() {}

  // Explicit constructor for terminal leaf node initialized with succeeded board IDs.
  explicit ConditionalLine(const std::set<int> &boards)
      : final_covered_boards_(boards),
        covered_count_(static_cast<int>(boards.size())) {}

  // Adds a conditional branch linking a PlayCondition (e.g. WinnerPlayCondition or TrickPlayCondition)
  // to a child ConditionalLine continuation node.
  void AddCondition(PlayCondition *c,
                    const ConditionalLine &continue_with) {
    AddCondition(c, std::make_shared<ConditionalLine>(continue_with));
  }

  void AddCondition(PlayCondition *c,
                    std::shared_ptr<ConditionalLine> continue_with) {
    assert(final_covered_boards_.empty());
    if (continue_with) {
      covered_count_ += continue_with->CoveredFinalBoards();
    }
    conditions_.emplace_back(std::shared_ptr<PlayCondition>(c),
                             std::move(continue_with));
  }

  // Sets the next trick action (GeneralizedTrick) to be played at this node in
  // the decision tree.
  void SetAction(const GeneralizedTrick &gt) {
    do_first_ = gt;
    do_first_.PopulateLegacyMaps();
  }

  // Recursively formats human-readable text representation of the decision tree.
  std::string Describe(int lvl = 0, bool sort_branches = false) const;

  // Returns the number of conditional branches out of this node.
  int num_conditions() const { return conditions_.size(); }

  // Returns total number of covered boards at this node.
  int CoveredBoards() const { return CoveredFinalBoards(); }

  // Returns set of all board IDs that succeed under this line or any of its subtrees.
  std::set<int> CoveredBoardsSet() const { return CoveredFinalBoardsSet(); }

  // Returns total number of simulated deals/boards evaluated.
  int total_boards() const { return total_boards_; }
  void set_total_boards(int n) { total_boards_ = n; }

  // Sets and accesses cross-hand summary report for covered (succeeded) deals.
  void set_summary(const CrossHandSummaryReport &summary) { summary_ = summary; }
  const CrossHandSummaryReport &summary() const { return summary_; }
  bool has_summary() const { return summary_.total_deals > 0; }

  // Sets and accesses cross-hand summary report for failed deals.
  void set_failure_summary(const CrossHandSummaryReport &summary) { failure_summary_ = summary; }
  const CrossHandSummaryReport &failure_summary() const { return failure_summary_; }
  bool has_failure_summary() const { return failure_summary_.total_deals > 0; }

  int min_tricks() const { return min_tricks_; }
  void set_min_tricks(int n) { min_tricks_ = n; }

  const BoardState &sample_board() const { return sample_board_; }
  void set_sample_board(const BoardState &bs) {
    sample_board_ = bs;
    has_sample_board_ = true;
  }
  bool has_sample_board() const { return has_sample_board_; }

  // Returns number of tricks won when executing this line on a BoardState or
  // PairedDeal.
  int ResultsOnHand(const BoardState &bs) const;
  int ResultsOnHand(const PairedDeal &deal) const;

  // Replays this line on `bs` where defenders pick among DDS-optimal plays to
  // minimize declarer's branch success probability in this line.
  struct DefenderStateKey {
    uint64_t played_cards = 0;
    uint64_t cur_trick_bits = 0;
    bool operator==(const DefenderStateKey &o) const {
      return played_cards == o.played_cards &&
             cur_trick_bits == o.cur_trick_bits;
    }
  };
  struct DefenderStateKeyHash {
    size_t operator()(const DefenderStateKey &k) const {
      size_t h1 = std::hash<uint64_t>{}(k.played_cards);
      size_t h2 = std::hash<uint64_t>{}(k.cur_trick_bits);
      return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
    }
  };
  using DefenderCardsCache =
      std::unordered_map<DefenderStateKey, std::vector<std::string>,
                         DefenderStateKeyHash>;
  struct RescoreBoardTrace {
    const ConditionalLine *leaf = nullptr;
    std::vector<std::pair<const ConditionalLine *, std::string>> node_tricks;
  };
  int ResultsOnHandAdversarial(
      const BoardState &bs, DefenderCardsCache *def_cache = nullptr,
      RescoreBoardTrace *out_trace = nullptr) const;

  using Step1DefenderCardCache =
      std::unordered_map<DefenderStateKey, std::string, DefenderStateKeyHash>;
  struct UnmakeableBoardTrace {
    std::vector<const ConditionalLine *> opp1_nodes;
    std::vector<const ConditionalLine *> opp2_nodes;
    std::vector<std::pair<const ConditionalLine *, int>> winner_nodes;
  };
  void TraceUnmakeableBoard(
      const BoardState &bs, int board_num, bool in_step1_set,
      Step1DefenderCardCache *def_cache,
      UnmakeableBoardTrace *out_trace) const;
  void ApplyUnmakeableBoardCounts(
      const std::unordered_map<const ConditionalLine *, size_t> &opp1_counts,
      const std::unordered_map<const ConditionalLine *, size_t> &opp2_counts,
      const std::unordered_map<const ConditionalLine *, std::array<size_t, 4>>
          &winner_counts);

  // Clones all child ConditionalLine pointers recursively so this tree does
  // not share subtree nodes with other candidate lines.
  void DeepCloneChildren();

  // Updates leaf final_covered_boards_, action board_to_trick maps, and
  // bottom-up covered_count_ from Step-2 rescored results while preserving
  // step1_covered_count_.
  void ApplyRescoredBoards(
      const std::unordered_map<const ConditionalLine *, std::set<int>>
          &leaf_boards,
      const std::unordered_map<const ConditionalLine *,
                               std::map<int, std::string>>
          &node_board_tricks = {});
  bool has_rescored_boards() const { return has_rescored_boards_; }
  int step1_covered_boards() const {
    return has_rescored_boards_ ? step1_covered_count_ : covered_count_;
  }

  struct PlayedTrickStep {
    int leader = 0;
    std::string trick_str;
    std::string my1_card;
    std::string my2_card;
    int winner = 0;
    std::string winning_card;
    int ns_tricks_so_far = 0;
    bool diverged_after = false;
    // The action node of this line whose trick was followed for this step.
    const ConditionalLine *node = nullptr;
    // When a defender's card matched none of the line's patterns for this
    // trick, the seat and card that broke the pattern.
    int unmatched_seat = -1;
    std::string unmatched_card;
  };

  struct BoardPlayTrace {
    int board_id = 0;
    std::vector<PlayedTrickStep> steps;
    int final_ns_tricks = 0;
    const ConditionalLine *leaf = nullptr;
  };

  // Canonical single-pass simulator: replays this ConditionalLine on a
  // BoardState up to the trick where it diverges or terminates, populating the
  // step-by-step trace, terminal leaf (if reached), and final_ns_tricks.
  BoardPlayTrace SimulateBoard(const BoardState &bs, int board_id = 0,
                               bool adversarial = false,
                               DefenderCardsCache *def_cache = nullptr) const;

  // Traces play of this ConditionalLine on a BoardState up to the trick where
  // it diverges or terminates, computing final_ns_tricks.
  BoardPlayTrace TraceOnBoard(const BoardState &bs, int board_id = 0) const;

  void set_board_traces(std::map<int, BoardPlayTrace> traces) {
    board_traces_ = std::move(traces);
  }
  const std::map<int, BoardPlayTrace> &board_traces() const {
    return board_traces_;
  }

  // Returns total distinct final boards covered across all leaf subtrees.
  int CoveredFinalBoards() const { return covered_count_; }

  // Recursively gathers set of distinct final boards covered across all leaf subtrees, avoiding cycles.
  std::set<int> CoveredFinalBoardsSet(std::set<const ConditionalLine*> visited = {}) const;

  // Returns, for each covered board, the cards declarer's side plays along
  // this line, trick by trick. Two lines with equal signatures prescribe the
  // same plays on every covered board and differ only in how the tree happens
  // to be branched (e.g. on a defender's card versus on the trick winner).
  std::map<int, std::string> DeclarerPlaySignature() const;

  // Returns the GeneralizedTrick action to execute first at this node (the next trick).
  const GeneralizedTrick &do_first() const { return do_first_; }

  // Returns reference to vector of (PlayCondition, child ConditionalLine) conditional branches.
  const std::vector<std::pair<std::shared_ptr<PlayCondition>,
                             std::shared_ptr<ConditionalLine>>> &
  conditions() const {
    return conditions_;
  }

  // Returns set of board IDs directly covered at this terminal leaf node.
  const std::set<int> &final_covered_boards() const {
    return final_covered_boards_;
  }

 private:
  // The next trick action (4-card play sequence) executed at this step in the line of play.
  GeneralizedTrick do_first_;

  // List of conditional branching pairs: (condition, child subtree).
  // At top level: paired with TrickPlayCondition ("after trick...").
  // At intermediate levels: paired with WinnerPlayCondition ("when X wins").
  // Empty at leaf nodes.
  std::vector<std::pair<std::shared_ptr<PlayCondition>,
                       std::shared_ptr<ConditionalLine>>>
      conditions_;

  // Non-empty only on terminal leaf nodes: set of board IDs where min_tricks target was reached.
  std::set<int> final_covered_boards_;

  // Cached count of covered boards across this node and its subtrees.
  int covered_count_ = 0;
  int step1_covered_count_ = 0;
  bool has_rescored_boards_ = false;

  // Total number of simulated deal iterations in the run.
  int total_boards_ = 0;

  // Summary reports generated by CrossHandDealSetSummarizer for covered/failed deals.
  CrossHandSummaryReport summary_;
  CrossHandSummaryReport failure_summary_;

  BoardState sample_board_;
  bool has_sample_board_ = false;
  int min_tricks_ = 0;
  std::map<int, BoardPlayTrace> board_traces_;
};

// WinnerPlayCondition represents a trick-outcome condition based on which player
// (by seat index: 0 = North, 1 = West, 2 = South, 3 = East) won the preceding trick.
//
// Describe() formats as "when X wins [the cards]" if this is the sole outcome in the parent line,
// or "if X wins [the cards]" if there are multiple branch outcomes depending on trick winner.
class WinnerPlayCondition : public PlayCondition {
 public:
  explicit WinnerPlayCondition(int who) : who_(who) {}
  WinnerPlayCondition(int who, const std::set<std::string> &winning_cards)
      : who_(who), winning_cards_(winning_cards) {}
  WinnerPlayCondition(int who, const std::vector<std::string> &winning_cards)
      : who_(who), winning_cards_(winning_cards.begin(), winning_cards.end()) {}

  std::string Describe(bool only_choice) const override;

  int who() const { return who_; }
  const std::set<std::string> &winning_cards() const { return winning_cards_; }

 private:
  int who_;  // Seat index of the winning player (0 = North, 1 = West, 2 = South, 3 = East)
  std::set<std::string> winning_cards_;
};

class TrickPlayCondition : public PlayCondition {
 public:
  TrickPlayCondition(const std::string &trick) : trick_(trick) {}

  std::string Describe(bool only_choice) const override;

 private:
  std::string trick_;
};

class IsCardGoodCondition : public PlayCondition {
 public:
  IsCardGoodCondition(CardType card, bool is_good)
      : card_(card), is_good_(is_good) {}

  std::string Describe(bool only_choice) const override;

  CardType card() const { return card_; }
  bool is_good() const { return is_good_; }
  bool Evaluate(const BoardState &bs) const;

 private:
  CardType card_;
  bool is_good_;
};

std::vector<std::vector<GeneralizedTrick>> GroupTricks(
    const std::vector<GeneralizedTrick> &input);
std::vector<std::vector<GeneralizedTrick>> GroupTricks(
    std::vector<GeneralizedTrick> &&input);
