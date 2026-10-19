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

TEST(DdsTest, SmallTransTableMode2AndDealSwitch) {
  SetResources(30, 1);

  dealPBN d1{};
  d1.trump = SPADES;
  d1.first = EAST;
  d1.currentTrickSuit[0] = HEARTS;
  d1.currentTrickRank[0] = 14;
  strcpy(d1.remainCards,
         "N:AJ8765.J85..KJ86 2.KQT.J762.T742 "
         "KT9.93.AQ853.AQ9 Q43.7642.KT94.53");

  futureTricks fut_mode2{};
  ASSERT_EQ(RETURN_NO_FAULT, SolveBoardPBN(d1, 11, 2, 2, &fut_mode2, 0));

  futureTricks fut_mode1{};
  ASSERT_EQ(RETURN_NO_FAULT, SolveBoardPBN(d1, 11, 2, 1, &fut_mode1, 0));
  EXPECT_EQ(fut_mode1.score[0], fut_mode2.score[0]);

  dealPBN d2 = d1;
  strcpy(d2.remainCards,
         "N:AJ8765.J85..KJ86 Q43.KQT.KT94.53 "
         "KT9.93.AQ853.AQ9 2.7642.J762.T742");
  futureTricks fut2_mode2{};
  ASSERT_EQ(RETURN_NO_FAULT, SolveBoardPBN(d2, 11, 2, 2, &fut2_mode2, 0));

  SetResources(30, 1);
  futureTricks fut2_fresh{};
  ASSERT_EQ(RETURN_NO_FAULT, SolveBoardPBN(d2, 11, 2, 1, &fut2_fresh, 0));
  EXPECT_EQ(fut2_fresh.score[0], fut2_mode2.score[0]);

#if defined(__linux) || defined(__APPLE__)
  SetMaxThreads(0);
#endif
}

