#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

#include "card.h"
#include "conditional_line.h"
#include "deal.h"
#include "hand.h"
#include "line_finder.h"
#include "glog/logging.h"

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);

  std::mt19937 rng(12345);

  std::vector<CardType> full_deck;
  for (int suit = 0; suit < 4; suit++) {
    for (int val = 0; val < 13; val++) {
      full_deck.push_back(Card::GetSuitedCard(static_cast<Suit>(suit), val));
    }
  }

  std::cout << "Starting LineFinder Performance Benchmark (ending_size 3..11)" << std::endl;

  for (int ending_size = 3; ending_size <= 11; ending_size++) {
    std::vector<CardType> deck = full_deck;
    std::shuffle(deck.begin(), deck.end(), rng);

    std::vector<uint64_t> hands(4, 0);
    uint64_t unknown_cards = 0;

    for (int i = 0; i < ending_size; i++) {
      hands[0] |= deck[i];
    }
    for (int i = 0; i < ending_size; i++) {
      hands[2] |= deck[ending_size + i];
    }
    for (int i = 0; i < 2 * ending_size; i++) {
      unknown_cards |= deck[2 * ending_size + i];
    }

    int min_tricks = ending_size / 2;
    int leader = 0;
    Suit trump = SPADES;

    std::cout << "\n=== LineFinder Benchmark: ending_size = " << ending_size << " ===" << std::endl;
    std::cout << "hand[0]: " << HandDebugString(hands[0]) << std::endl;
    std::cout << "hand[2]: " << HandDebugString(hands[2]) << std::endl;
    std::cout << "unknown_cards: " << HandDebugString(unknown_cards) << std::endl;

    LineFinder lf(20);
    srand48(12345);

    auto start = std::chrono::high_resolution_clock::now();
    std::vector<ConditionalLine> lines =
        lf.Solve(min_tricks, hands, unknown_cards, leader, trump);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "Found " << lines.size() << " lines for ending_size = " << ending_size << std::endl;
    for (size_t i = 0; i < lines.size(); i++) {
      std::cout << "Line " << i + 1 << ": " << lines[i].Describe() << std::endl;
    }
    std::cout << "LineFinder.Solve for ending_size = " << ending_size
              << " took " << elapsed.count() << " seconds." << std::endl;

    if (elapsed.count() > 30.0) {
      std::cout << "Solve took " << elapsed.count()
                << "s (> 30s), breaking perf benchmark loop." << std::endl;
      break;
    }
  }

  std::cout << "\nBenchmark complete." << std::endl;
  return 0;
}
