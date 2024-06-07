#include "solver.h"

#include <memory>
#include <thread>
#include <unordered_map>

bool PLAY_SMART = 0;
bool FOURTH_SMART = 0;
bool ALLOW_CLAIMS = false;
bool SEARCH_EQUIVALENT = false; // true;

static uint64_t num_solver_iters_ = 0;
uint64_t num_solver_iters() { return num_solver_iters_; }
std::string num_solver_iters_str() {
  if (num_solver_iters_ > 1000 * 1000 * 1000) {
    return std::to_string(num_solver_iters_ / (1000 * 1000 * 1000)) + "B";
  } else if (num_solver_iters_ > 1000 * 1000) {
    return std::to_string(num_solver_iters_ / (1000 * 1000)) + "M";
  } else  if (num_solver_iters_ > 1000) {
    return std::to_string(num_solver_iters_ / (1000)) + "K";
  } else {
    return std::to_string(num_solver_iters_);
  }
}

#define TRICKS_WON(bs) \
  ((pair % 2 == 0) ? bs.tricks_won_even() : bs.tricks_won_odd())

void SwapCards(CardType *cards_to_try, int i, int other) {
  if (i != other) {
    CardType tmp = cards_to_try[other];
    cards_to_try[other] = cards_to_try[i];
    cards_to_try[i] = tmp;
  }
}

void ScrambleCards(CardType *cards_to_try, int num_cards_to_try) {
  for (int i = 0; i < num_cards_to_try; i += 2) {
    int other = lrand48() % num_cards_to_try;
    SwapCards(cards_to_try, i, other);
  }
}

bool WinsTrick(Suit trump_suit, Suit played_suit, int played_val,
               Suit cur_winning_suit, int cur_winning_val) {
  if (played_suit == cur_winning_suit) {
    return played_val > cur_winning_val;
  } else if (played_suit == trump_suit) {
    return true;
  } else {
    return false;
  }
}

void GetCardsToTry(const BoardState &bs, Suit suit_led,
                   CardType *cards_to_try, int &num_cards_to_try,
                   int *is_equivalent) {
  uint64_t hand = bs.GetHand(bs.CurPlayer());
  const uint64_t played_cards = bs.played_cards();
  VLOG(4) << "Picking card to play for " << bs.CurPlayer()
          << " from " << HandDebugString((hand & (~bs.played_cards())));
  if (suit_led != NOTRUMP) {
    // See if they're out of the suit led.
    uint16_t cur_suit =
        (((hand & (~played_cards)) >> (suit_led * 16)) & 0xffff);
    if (cur_suit == 0) {
      // Pretend you're on lead, as you can play anything.
      suit_led = NOTRUMP;
    }
  }

  while (hand > 0) {
    for (int suit = 0; suit < 4; suit++) {
      uint16_t cur_suit = hand & 0xffff;
      hand = (hand >> 16);
      if (suit_led != NOTRUMP && suit_led != suit) {
        continue;
      }
      int unplayed_intervening_cards = 1;
      for (int val = 0; val < 13; val++) {
        uint64_t c = Card::GetSuitedCard((Suit)suit, val);
        if (played_cards & c) {
          // Do neither of the next things
        } else if (cur_suit & 0x1) {
          // Should we consider playing this card?
          if (unplayed_intervening_cards > 0) {
            if (is_equivalent) {
              is_equivalent[num_cards_to_try] = 0;
            }
            cards_to_try[num_cards_to_try++] = c;
            VLOG(2) << "Got card to try " << Card::PrintCard(c);
            unplayed_intervening_cards = 0;
          } else {  // unplayed_intervening_cards is 0
            if (is_equivalent) {
              is_equivalent[num_cards_to_try] = 1;
              cards_to_try[num_cards_to_try++] = c;
              VLOG(2) << "Got eq card to try " << Card::PrintCard(c);
            }
          }
        } else {
          unplayed_intervening_cards++;
        }
        cur_suit = (cur_suit >> 1);
      }
    }
  }

  VLOG(4) << "Found num_cards_to_try " << num_cards_to_try;
  if (VLOG_IS_ON(2)) {
    std::string ans;
    for (int i = 0; i < num_cards_to_try; i++) {
      CardType c = cards_to_try[i];
      ans += Card::PrintCard(c) + ", ";
    }
    VLOG(4) << "Cards: " << ans;
  }
}

static CacheKey GetCacheKey(const BoardState &bs) {
  // return bs.played_cards();
  return bs.RemainingHandsKey();
}

OptimalLine GetFinishedTrick(const BoardState &bs) {
  if (bs.CardsPlayedThisTrick() != 0) {
    return "";
  }
  // Need to look at previous trick.
  int winner = bs.cur_trick_leader();
  int prev_leader = bs.prev_trick_leader();
  std::string trick;
  for (int i = 0; i < 4; i++) {
    int player = (prev_leader + i) % 4;
    if (1 || player % 2 == 0) {
      trick += Card::PrintCard(bs.PreviousTrick()[i]);
    } else {
      trick += 'x';
    }
    if (player == winner) {
      // trick += '*';
    }
    trick += ' ';
  }
  return trick; //  + "/ ";
}

std::string GetNDots(int n) {
  std::string ans;
  for (int i = 0; i < n; i++) {
    ans += '.';
  }
  return ans;
}

bool ShouldRecordOptimalLine(int lvl) {
  return (lvl < 4);
}

void AddOptimalLine(int lvl, const BoardState &bs, CardType c,
                    const OptimalLines &optimal_storage,
                    OptimalLines &current_optimal_plays) {
  if (!ShouldRecordOptimalLine(lvl)) {
    return;
  }
  if (bs.CardsPlayedThisTrick() == 0) {
    if (!optimal_storage.empty()) {
      VLOG(2) << "Ignoring " << optimal_storage.size() << " existing lines";
    }
    OptimalLine new_line = Card::PrintCard(c);
    current_optimal_plays.insert(new_line);
    VLOG(2) << "AddOptimalLine starting trick with " << new_line
            << " to yield " << current_optimal_plays.size();
    return;
  }

  // We need to add this card to existing optimal lines.
  for (const auto &line : optimal_storage) {
    OptimalLine new_line = Card::PrintCard(c) + " " + line;
    VLOG(2) << "AddOptimalLine adds card to yield " << new_line;
    current_optimal_plays.insert(new_line);
  }

  VLOG(2) << "AddOptimalLine at lvl " << lvl
          << " count " << current_optimal_plays.size();
}

std::string PrintLine(const OptimalLine &x) {
  return x;
}
std::string PrintLines(const OptimalLines &lines) {
  std::string ans;
  for (const auto &line : lines) {
    if (!ans.empty()) {
      ans += " | ";
    }
    ans += line;
  }
  return ans;
}

std::string ReplaceCard(CardType old, CardType cur, const std::string &input) {
  std::string storage = input;
  auto pos = storage.find(Card::PrintCard(old));
  if (pos == std::string::npos) {
    return storage;
  }
  std::string cur_card = Card::PrintCard(cur);
  CHECK_EQ(cur_card.size(), 2) << cur_card;
  storage[pos++] = cur_card[0];
  storage[pos++] = cur_card[1];
  return storage;
}

std::string PrintCacheKey(CacheKey k) {
  return "<" + std::to_string(k.first) + ", " + std::to_string(k.second) + ">";
}

int MostTricksCanBeWonHelper(const BoardState &bs, int pair,
                             std::string *sequence, const Suit suit_led,
                             int lvl, const int tricks_remaining,
                             int lower_bound, int upper_bound,
                             SolverCache *cache,
                             OptimalLines *optimal_plays) {
  num_solver_iters_++;
  LOG_EVERY_N(INFO, 100 * 1000 * 1000)
      << "MostTricksCanBeWon " << num_solver_iters_str()
      << " lvl " << lvl;
  VLOG(3) << "MostTricksCanBeWon lvl " << lvl << " won " << TRICKS_WON(bs);

  if (tricks_remaining == 0) {
    CHECK(bs.Finished());
    int ans = TRICKS_WON(bs);
    if (optimal_plays) {
      optimal_plays->insert(OptimalLine());
    }
    VLOG(2) << "End of search lvl " << lvl << ", result: " << ans;
    return ans;
  }

  const bool use_cache =
      (tricks_remaining <= 11 && bs.CardsPlayedThisTrick() == 0);
  if (use_cache) {
    auto &this_cache = cache->saved_results[bs.cur_trick_leader()];
    auto k = GetCacheKey(bs);
    auto it = this_cache.find(k);
    if (it != this_cache.end()) {
      if (sequence) {
        *sequence = cache->saved_sequences[bs.cur_trick_leader()][k];
      }
      if (optimal_plays) {
        *optimal_plays = cache->saved_optimal_plays[bs.cur_trick_leader()][k];
        VLOG(2) << "Cache hit of (" << TRICKS_WON(bs) << " +) " << it->second
                << " at " << PrintCacheKey(k) << " lvl " << lvl
                << " with " << PrintLines(*optimal_plays)
                << " from " << bs.DebugString();
      }
      int ans = TRICKS_WON(bs) + it->second;
      if (ans > 0 && ShouldRecordOptimalLine(lvl) && optimal_plays &&
          optimal_plays->empty()) {
        // We need to recalculate to get the optimal plays
      } else {
        return ans;
      }
    }
  }

  CardType cards_to_try[64];
  int num_cards_to_try = 0;
  bool consider_equivalent_cards =
      SEARCH_EQUIVALENT && ShouldRecordOptimalLine(lvl);
  int is_equivalent[64];

  GetCardsToTry(bs, suit_led, cards_to_try, num_cards_to_try,
                consider_equivalent_cards ? is_equivalent : nullptr);

  CHECK_GT(num_cards_to_try, 0);

  int mini_max_outcome = -1;
  bool pair_is_me = (pair % 2) == (bs.CurPlayer() % 2);
  CardType best_card = 0;
  std::string best_seq;

  bool prefer_low_play = (bs.CardsPlayedThisTrick() == 1 ||
                          // NB: 4th hand the "lowest" is really best
                          bs.CardsPlayedThisTrick() == 3);
  if (consider_equivalent_cards) {
    prefer_low_play = true;
  }
  if (PLAY_SMART && bs.CardsPlayedThisTrick() == 0) {
    LOG(FATAL) << "no";
    ScrambleCards(cards_to_try, num_cards_to_try);
  }

  OptimalLines current_optimal_plays;
  OptimalLines optimal_storage;
  int last_result = -1;
  for (int i = 0; i < num_cards_to_try; i++) {
    CardType c = prefer_low_play ? cards_to_try[i] :
                 cards_to_try[num_cards_to_try - i - 1];
    VLOG(2) << GetNDots(lvl) << "Trying " << Card::PrintCard(c)
            << " for " << i << " / " << num_cards_to_try
            << " by " << bs.CurPlayer() << " (lvl " << lvl << ")";
    BoardState copy = bs;
    copy.PlayCard(copy.CurPlayer(), c);
    std::string seq_storage;
    if (sequence) {
      seq_storage = *sequence;
    }
    int tricks_finished = (copy.SuitLed() == NOTRUMP ? 1 : 0);
    int cur_lower_bound = lower_bound;
    int cur_upper_bound = upper_bound;
    if (tricks_finished) {
      if (pair_is_me) {
        // Worst case is winning no more tricks
        cur_lower_bound = std::max(TRICKS_WON(bs), cur_lower_bound);
      } else {
        cur_upper_bound = std::min(TRICKS_WON(bs) + tricks_remaining,
                                   cur_upper_bound);
      }
    }
    if (cur_lower_bound != lower_bound ||
        cur_upper_bound != upper_bound) {
      VLOG(2) << "New bounds are [" << cur_lower_bound << ", "
              << cur_upper_bound << "]";
    }
    int result;
    if (consider_equivalent_cards && is_equivalent[i]) {
      result = last_result;
      VLOG(2) << "Reusing last result " << last_result
              << " for " << Card::PrintCard(cards_to_try[i])
              << " instead of " << Card::PrintCard(cards_to_try[i - 1]);
      OptimalLines new_storage;
      for (const std::string &line : optimal_storage) {
        new_storage.insert(
            ReplaceCard(cards_to_try[i - 1], cards_to_try[i], line));
      }
      optimal_storage = new_storage;
    } else {
      optimal_storage.clear();
      result =
          MostTricksCanBeWonHelper(
              copy, pair, sequence ? &seq_storage : nullptr,
              tricks_finished ? NOTRUMP : copy.SuitLed(), lvl + 1,
              tricks_remaining - tricks_finished,
              cur_lower_bound, cur_upper_bound, cache,
              optimal_plays ? &optimal_storage : nullptr);
      last_result = result;
    }
    VLOG(2) << "lvl " << lvl << " after " << Card::PrintCard(c)
            << " result " << result
            << " vs " << mini_max_outcome
            << ": " << PrintLines(optimal_storage);
    if (pair_is_me && optimal_plays && result == mini_max_outcome) {
      AddOptimalLine(lvl, copy, c, optimal_storage, current_optimal_plays);
    } else if (mini_max_outcome < 0) {
      mini_max_outcome = result;
      VLOG(4) << "Play " << i << " / " << num_cards_to_try
              << " by " << bs.CurPlayer() << " is "
              << Card::PrintCard(c) << " with result " << result
              << " (+" << (result - TRICKS_WON(bs))
              << ") [" << lower_bound << ", " << upper_bound << "]";
      best_card = c;
      if (sequence) { best_seq = seq_storage; }
      current_optimal_plays.clear();
      AddOptimalLine(lvl, copy, c, optimal_storage, current_optimal_plays);
      if (ALLOW_CLAIMS && (i + 1 < num_cards_to_try) &&
          ((pair_is_me && result - TRICKS_WON(bs) == tricks_remaining) ||
           (!pair_is_me && result - TRICKS_WON(bs) == 0))) {
        VLOG(3) << "Found optimal after " << i << " / " << num_cards_to_try;
        break;
      }

    } else if (pair_is_me) {
      if (result > mini_max_outcome) {
        VLOG(4) << "Best maximizing play by " << bs.CurPlayer() << " is "
                << Card::PrintCard(c) << " with result " << result
                << " [" << lower_bound << ", " << upper_bound << "]";
        mini_max_outcome = result;
        best_card = c;
        if (sequence) { best_seq = seq_storage; }
        current_optimal_plays.clear();
        AddOptimalLine(lvl, copy, c, optimal_storage, current_optimal_plays);
        VLOG(4) << "result " << result << " vs "
                << TRICKS_WON(bs) << " with remaining " << tricks_remaining
                << " after " << i << " / " << num_cards_to_try;
        if (ALLOW_CLAIMS &&
            i + 1 < num_cards_to_try &&
            result - TRICKS_WON(bs) == tricks_remaining) {
          VLOG(3) << "Found optimal best after " << i << " / "
                  << num_cards_to_try;
          break;
        }
      }
    } else {  // !pair_is_me
      if (result < mini_max_outcome) {
        VLOG(4) << "Best minimizing play by " << bs.CurPlayer() << " is "
                << Card::PrintCard(c) << " with result " << result
                << " [" << lower_bound << ", " << upper_bound << "]";
        mini_max_outcome = result;
        best_card = c;
        if (sequence) { best_seq = seq_storage; }
        current_optimal_plays.clear();
        AddOptimalLine(lvl, copy, c, optimal_storage, current_optimal_plays);
        if (ALLOW_CLAIMS &&
            i + 1 < num_cards_to_try && result - TRICKS_WON(bs) == 0) {
          VLOG(3) << "Found optimal worst after " << i << " / "
                  << num_cards_to_try;
          break;
        }
      }
    }
  }

  VLOG(2) << "Best play at lvl " << lvl << " is " << Card::PrintCard(best_card)
          << " yields " << mini_max_outcome;

  if (sequence) {
    *sequence = Card::PrintCard(best_card) + " " + best_seq;
    if (bs.CurPlayer() == bs.cur_trick_leader()) {
      *sequence = "/ " + *sequence;
    }
  }

  if (optimal_plays) {
    *optimal_plays = current_optimal_plays;
    VLOG(2) << "optimal_plays has " << optimal_plays->size()
            << " at lvl " << lvl << ": " << PrintLines(*optimal_plays);
  }

  if (use_cache) {
    auto k = GetCacheKey(bs);
    cache->saved_results[bs.cur_trick_leader()][k] =
        mini_max_outcome - TRICKS_WON(bs);
    VLOG(2) << "Caching result of " << mini_max_outcome - TRICKS_WON(bs)
            << " at " << PrintCacheKey(k)
            << " from " << bs.DebugString();
    if (sequence) {
      cache->saved_sequences[bs.cur_trick_leader()][k] =
          *sequence;
    }
    if (optimal_plays) {
      cache->saved_optimal_plays[bs.cur_trick_leader()][k] =
          *optimal_plays;
    }
  }

  return mini_max_outcome;
}

int MostTricksCanBeWon(const BoardState &bs, int pair, std::string *sequence,
                       OptimalLines *optimal_lines, SolverCache *cache) {
  SolverCache cache_storage;
  if (!cache) {
    cache = &cache_storage;
  }
  num_solver_iters_ = 0;
  int tricks_remaining = CardsInHand(bs.GetHand(0), bs.played_cards()).size();
  VLOG(1) << "MostTricksCanBeWon with tricks_remaining " << tricks_remaining;
  int ans = MostTricksCanBeWonHelper(
      bs, pair, sequence, bs.SuitLed(), 0,
      tricks_remaining, 0, tricks_remaining, cache,
      optimal_lines);
  int cache_size = 0;
  for (int i = 0; i < 4; i++) {
    cache_size += cache->saved_results[i].size();
  }
  VLOG(1) << "MostTricksCanBeWon solved after " << num_solver_iters_
          << " with cache having " << cache_size;
  return ans;
}
