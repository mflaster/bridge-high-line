#include "board_state.h"

#include <catch2/catch.hpp>
#include "hand.h"

TEST_CASE("PBN") {
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S AQ"));
  hands.push_back(ParseHand("S KJ"));
  hands.push_back(ParseHand("S 87"));
  hands.push_back(ParseHand("S 54"));
  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  REQUIRE(bs.DebugString() == "S AQ;  // S KJ;  // S 87;  // S 54; ");
  REQUIRE(bs.GetPBNCards() == "N:AQ... KJ... 87... 54...");

  bs.PlayCard(0, Card::ParseStringCard("SA"));
  REQUIRE(bs.GetPBNCards() == "N:Q... KJ... 87... 54...");

}

TEST_CASE("SuitDistribution") {
  int dist[4];
  GetSuitDistribution(ParseHand("S AQ"), dist);
  REQUIRE(dist[SPADES] == 2);
  REQUIRE(dist[HEARTS] == 0);
  REQUIRE(dist[DIAMONDS] == 0);
  REQUIRE(dist[CLUBS] == 0);

  GetSuitDistribution(ParseHand("S A; H 543; D 2; C 87"), dist);
  REQUIRE(dist[SPADES] == 1);
  REQUIRE(dist[HEARTS] == 3);
  REQUIRE(dist[DIAMONDS] == 1);
  REQUIRE(dist[CLUBS] == 2);
}
