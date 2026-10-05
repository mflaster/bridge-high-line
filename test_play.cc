#include <iostream>
#include <memory>

#include "deal.h"
#include "hand_requirements.h"
#include "solver.h"


static int num_iters = 25;

void hand1line1() {
  std::cout << "----------------------------------------" << "\n";
  std::cout << "hand1line1" << "\n";
  std::cout << "----------------------------------------" << "\n";
  uint64_t cards = ParseHand("D AQT9872; C J6; S QJT852; H T; DJ; C K9");
  CHECK_EQ(CardsInHand(cards, 0).size(), 19);
  std::vector<std::unique_ptr<HandRequirements>> reqs1, reqs2;
  reqs2.emplace_back(new SuitAtLeast(SPADES, 6));
  double total_optimal = 0;
  double makes_7_tricks = 0;
  for (int i = 0; i < num_iters; i++) {
    std::cout << "----------------------------------------" << "\n";
    auto [hand1, hand2] = DealTwoRequiredHands(cards, reqs1, reqs2);
    CHECK_EQ(CardsInHand(hand1, 0).size(), 9);
    CHECK_EQ(CardsInHand(hand2, 0).size(), 10);
    hand1 = hand1 | Card::ParseStringCard("C3");
    CHECK_EQ(CardsInHand(hand1, 0).size(), 10);

    std::cout << "East: " << HandDebugString(hand1) << "\n";
    std::cout << "West: " << HandDebugString(hand2) << "\n";
    std::vector<uint64_t> hands;
    hands.push_back(ParseHand("S 764; H 32; D K64; C 82"));
    hands.push_back(hand1);
    hands.push_back(ParseHand("S -; H Q7; D 53; C AQT754"));
    hands.push_back(hand2);

    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(0);
    bs.SetTrump(HEARTS);
    bs.PlayCard(0, Card::ParseStringCard("C2"));
    bs.PlayCard(1, Card::ParseStringCard("C3"));
    bs.PlayCard(2, Card::ParseStringCard("CQ"));
    int optimal = MostTricksCanBeWon(bs, 0, nullptr);
    std::cout << "*** optimal: " << optimal << " tricks after searching "
              << num_solver_iters_str() << "\n";
    total_optimal += optimal;
    if (optimal >= 7) {
      makes_7_tricks++;
    }
  }
  std::cout << "Avg tricks won: " << total_optimal / num_iters << "\n";
  std::cout << "Makes contract " << makes_7_tricks << " / " << num_iters << "\n";
}

void hand1line2() {
  std::cout << "----------------------------------------" << "\n";
  std::cout << "hand1line2" << "\n";
  std::cout << "----------------------------------------" << "\n";
  uint64_t cards = ParseHand("D AQT9872; C J6; S QJT85; DJ; C K9");
  CHECK_EQ(CardsInHand(cards, 0).size(), 17);
  std::vector<std::unique_ptr<HandRequirements>> reqs1, reqs2;
  reqs2.emplace_back(new SuitAtLeast(SPADES, 5));
  double total_optimal = 0;
  double makes_6_tricks = 0;
  for (int i = 0; i < num_iters; i++) {
    std::cout << "----------------------------------------" << "\n";
    auto [hand1, hand2] = DealTwoRequiredHands(cards, reqs1, reqs2);
    hand1 = hand1 | Card::ParseStringCard("C3");

    std::cout << "East: " << HandDebugString(hand1) << "\n";
    std::cout << "West: " << HandDebugString(hand2) << "\n";
    std::vector<uint64_t> hands;
    hands.push_back(ParseHand("S 764; H 3; D K64; C 82"));
    hands.push_back(hand1);
    hands.push_back(ParseHand("S -; H 7; D 53; C AQT754"));
    hands.push_back(hand2);

    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(0);
    bs.SetTrump(HEARTS);
    bs.PlayCard(0, Card::ParseStringCard("C2"));
    bs.PlayCard(1, Card::ParseStringCard("C3"));
    bs.PlayCard(2, Card::ParseStringCard("CQ"));
    int optimal = MostTricksCanBeWon(bs, 0, nullptr);
    std::cout << "*** optimal: " << optimal << " tricks after searching "
              << num_solver_iters_str() << "\n";
    total_optimal += optimal;
    if (optimal >= 6) {
      makes_6_tricks++;
    }
  }
  std::cout << "Avg tricks won: " << total_optimal / num_iters << "\n";
  std::cout << "Makes contract " << makes_6_tricks << " / " << num_iters << "\n";
}

int main() {
  srand48(0);

  hand1line2();
  hand1line1();
}
