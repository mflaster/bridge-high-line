#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include "hand.h"
#include "card.h"
#include "board_state.h"

class OptimalPlayTree;

struct BoardCacheKey {
  uint64_t h0;
  uint64_t h1;
  uint64_t h2;
  uint64_t h3;
  uint8_t leader;
  uint8_t trump;
  uint8_t min_tricks;

  bool operator==(const BoardCacheKey &o) const {
    return h0 == o.h0 && h1 == o.h1 && h2 == o.h2 && h3 == o.h3 &&
           leader == o.leader && trump == o.trump &&
           min_tricks == o.min_tricks;
  }
};

struct BoardCacheKeyHash {
  size_t operator()(const BoardCacheKey &k) const {
    uint64_t h = k.h0;
    h ^= k.h1 + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
    h ^= k.h2 + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
    h ^= k.h3 + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
    h ^= (static_cast<uint64_t>(k.min_tricks) << 16) |
         (static_cast<uint64_t>(k.leader) << 8) | k.trump;
    return static_cast<size_t>(h);
  }
};

struct HashOptimalLine {
  template <class T1>
      size_t operator()(const T1& v) const
  {
    size_t sum = 1;
    for (const auto &x : v) {
      sum = (x + sum) / 2 + 1;
    }
    return sum;
  }
};

using OptimalLine = std::string;
using OptimalLines = std::unordered_set<OptimalLine>;

std::string PrintLine(const OptimalLine &x);
std::string PrintLines(const OptimalLines &x);

// A hash function used to hash a pair of any kind
struct hash_pair {
  template <class T1, class T2>
      size_t operator()(const std::pair<T1, T2>& p) const
  {
    auto hash1 = p.first;
    auto hash2 = p.second;

    return hash1 + hash2 / 2;
  }
};

using CacheKey = std::pair<uint64_t, uint64_t>;

struct SolverCache {
  static constexpr size_t kNumShards = 64;
  mutable std::mutex shard_mu[kNumShards];
  std::unordered_map<BoardCacheKey, std::shared_ptr<OptimalPlayTree>,
                     BoardCacheKeyHash> tree_cache[kNumShards];
  std::atomic<uint64_t> tree_hits{0};
  std::atomic<uint64_t> tree_misses{0};

  std::unordered_map<CacheKey, int, hash_pair> saved_results[4];
  std::unordered_map<CacheKey, std::string, hash_pair> saved_sequences[4];
  std::unordered_map<CacheKey, OptimalLines, hash_pair> saved_optimal_plays[4];

  std::unordered_map<uint64_t, int> dds_results[4];
  std::unordered_map<uint64_t, OptimalLines> dds_lines[4];
  uint64_t dds_size() const {
    uint64_t ans = 0;
    for (int i = 0; i < 4; i++) {
      ans += dds_results[i].size();
    }
    return ans;
  }
};


inline constexpr bool PLAY_SMART = false;
inline constexpr bool FOURTH_SMART = false;
inline constexpr bool ALLOW_CLAIMS = false;
inline constexpr bool SEARCH_EQUIVALENT = false;

int MostTricksCanBeWon(const BoardState &bs, int pair, std::string *sequence,
                       OptimalLines *optimal_lines = nullptr,
                       SolverCache *cache = nullptr);

uint64_t num_solver_iters();
std::string num_solver_iters_str();
