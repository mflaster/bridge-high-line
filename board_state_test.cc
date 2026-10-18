#include "board_state.h"

#include <gtest/gtest.h>
#include "hand.h"

TEST(BoardStateTest, PBN) {
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S AQ"));
  hands.push_back(ParseHand("S KJ"));
  hands.push_back(ParseHand("S 87"));
  hands.push_back(ParseHand("S 54"));
  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  ASSERT_TRUE(bs.DebugString() == "S AQ;  // S KJ;  // S 87;  // S 54; ");
  ASSERT_TRUE(bs.GetPBNCards() == "N:AQ... KJ... 87... 54...");

  bs.PlayCard(0, Card::ParseStringCard("SA"));
  ASSERT_TRUE(bs.GetPBNCards() == "N:Q... KJ... 87... 54...");

}

TEST(BoardStateTest, SuitDistribution) {
  int dist[4];
  GetSuitDistribution(ParseHand("S AQ"), dist);
  ASSERT_TRUE(dist[SPADES] == 2);
  ASSERT_TRUE(dist[HEARTS] == 0);
  ASSERT_TRUE(dist[DIAMONDS] == 0);
  ASSERT_TRUE(dist[CLUBS] == 0);

  GetSuitDistribution(ParseHand("S A; H 543; D 2; C 87"), dist);
  ASSERT_TRUE(dist[SPADES] == 1);
  ASSERT_TRUE(dist[HEARTS] == 3);
  ASSERT_TRUE(dist[DIAMONDS] == 1);
  ASSERT_TRUE(dist[CLUBS] == 2);
}

TEST(BoardStateTest, FindBestDiscard_GuardHonorAgainstDrop) {
  // 9-card Hearts scenario after HT is played on trick 1 (8 cards left):
  // North has C 6, South has C AKJ (2 clubs higher than CQ: CA, CK).
  // East holds S A64, D KQ, C Q84 (3 clubs, which is just enough to guard CQ
  // against CA and CK being cashed from the top).
  // On trick 2 (H7 led), East's candidate discards are S4, SA, DK, DJ, C4, CQ.
  // East should NOT unguard C Q84 down to C Q8.
  std::vector<uint64_t> hands = {
      ParseHand("S 532; H 7; D T32; C 6;"),
      ParseHand("S A64; H -; D KQ; C Q84;"),
      ParseHand("S K9; H -; D A85; C AKJ;"),
      ParseHand("S JT; H -; D J97; C T93;"),
  };
  BoardState bs;
  bs.SetHands(hands);
  bs.SetTrump(HEARTS);
  bs.SetLead(0);
  bs.PlayCard(0, Card::ParseStringCard("H7"));
  ASSERT_TRUE(bs.CurPlayer() == 1);

  int suits[3] = {SPADES, DIAMONDS, CLUBS};
  int ranks[3] = {4, 12, 4};  // S4, DQ, C4
  int chosen = bs.FindBestDiscard(3, suits, ranks);
  // S4 should be discarded rather than C4 (which would unguard CQ84 to CQ8
  // under CA, CK).
  ASSERT_TRUE(suits[chosen] == SPADES);
}

