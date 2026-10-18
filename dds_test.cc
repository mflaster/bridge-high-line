#include <gtest/gtest.h>
#include <iostream>

#include "dll.h"
#include "card.h"

TEST(DdsTest, Basic) {
#if defined(__linux) || defined(__APPLE__)
  SetMaxThreads(0);
#endif

  dealPBN d1;
  int target = -1;  // Find optimal solution for side to play
  int solutions = 2; // Return all optimum cards and their scores.
  int mode = 1;  // automatic / always search
  futureTricks futp;
  int threadindex = 0;

  d1.trump = SPADES;
  d1.first = NORTH;
  d1.currentTrickSuit[0] = 0;
  d1.currentTrickSuit[1] = 0;
  d1.currentTrickSuit[2] = 0;
  d1.currentTrickRank[0] = 0;
  d1.currentTrickRank[1] = 0;
  d1.currentTrickRank[2] = 0;

  strcpy(d1.remainCards, "N:AQJ... KT9... 876... 543...");
  std::cout << "Calling SolveBoardPBN\n";
  SolveBoardPBN(d1, target, solutions, mode, &futp, threadindex);

  std::cout << "nodes searched " << futp.nodes << "\n";
  std::cout << "solutions: " << futp.cards << "\n";
  for (int i = 0; i < futp.cards; i++) {
    std::cout << "Card " << i << ": "
              << Card::PrintDDSCard((Suit)futp.suit[i], futp.rank[i])
              << " (equals " << futp.equals[i] << "), score: " << futp.score[i] << "\n";
  }
  ASSERT_TRUE(futp.cards == 2);
  ASSERT_TRUE(futp.score[0] == 2);
}

TEST(DdsTest, Basic2) {
#if defined(__linux) || defined(__APPLE__)
  SetMaxThreads(0);
#endif

  dealPBN d1;
  int target = -1;  // Find optimal solution for side to play
  int solutions = 2; // Return all optimum cards and their scores.
  int mode = 1;  // automatic / always search
  futureTricks futp;
  int threadindex = 0;

  futp.score[0] = -100;

  d1.trump = SPADES;
  d1.first = NORTH;
  d1.currentTrickSuit[0] = 0;
  d1.currentTrickSuit[1] = 0;
  d1.currentTrickSuit[2] = 0;
  d1.currentTrickRank[0] = 0;
  d1.currentTrickRank[1] = 0;
  d1.currentTrickRank[2] = 0;

  strcpy(d1.remainCards, "N:KQJ... A98... 654... 32.3..");
  std::cout << "Calling SolveBoardPBN\n";
  SolveBoardPBN(d1, target, solutions, mode, &futp, threadindex);

  std::cout << "nodes searched " << futp.nodes << "\n";
  std::cout << "solutions: " << futp.cards << "\n";
  for (int i = 0; i < futp.cards; i++) {
    std::cout << "Card " << i << ": "
              << Card::PrintDDSCard((Suit)futp.suit[i], futp.rank[i])
              << " (equals " << futp.equals[i] << "), score: " << futp.score[i] << "\n";
  }
  ASSERT_TRUE(futp.cards == 1);
  ASSERT_TRUE(futp.score[0] == 2);
}


TEST(DdsTest, Basic3) {
#if defined(__linux) || defined(__APPLE__)
  SetMaxThreads(0);
#endif

  dealPBN d1;
  int target = -1;  // Find optimal solution for side to play
  int solutions = 2; // Return all optimum cards and their scores.
  int mode = 1;  // automatic / always search
  futureTricks futp;
  int threadindex = 0;

  d1.trump = SPADES;
  d1.first = NORTH;
  d1.currentTrickSuit[0] = SPADES;
  d1.currentTrickRank[0] = 13;
  d1.currentTrickSuit[1] = 0;
  d1.currentTrickRank[1] = 0;
  d1.currentTrickSuit[2] = 0;
  d1.currentTrickRank[2] = 0;

  strcpy(d1.remainCards, "N:J... AQ... 87... 54...");
  std::cout << "Calling SolveBoardPBN\n";
  SolveBoardPBN(d1, target, solutions, mode, &futp, threadindex);

  std::cout << "nodes searched " << futp.nodes << "\n";
  std::cout << "solutions: " << futp.cards << "\n";
  for (int i = 0; i < futp.cards; i++) {
    std::cout << "Card " << i << ": "
              << Card::PrintDDSCard((Suit)futp.suit[i], futp.rank[i])
              << " (equals " << futp.equals[i] << "), score: " << futp.score[i] << "\n";
  }
  ASSERT_TRUE(futp.cards == 1);
  ASSERT_TRUE(futp.score[0] == 2);
}
