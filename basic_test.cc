#include <catch2/catch_all.hpp>

#include "golden_serializer.h"
#include "board_state.h"
#include "deal.h"
#include "card.h"
#include "hand.h"
#include "solver.h"

TEST_CASE("Cards") {
  REQUIRE(Card::GetSuitedCard(SPADES, 0) == 1);
  REQUIRE(Card::GetSuitedCard(SPADES, 1) == 2);
  REQUIRE(Card::GetSuitedCard(SPADES, 2) == 4);
  REQUIRE(Card::GetSuitedCard(HEARTS, 0) == (1 << 16));
}

TEST_CASE("PrintCards") {
  REQUIRE(Card::PrintCard(Card::GetSuitedCard(SPADES, 0)) == "S2");
  REQUIRE(Card::PrintCard(Card::GetSuitedCard(DIAMONDS, 12)) == "DA");
}

TEST_CASE("DealCards") {
  srand48(0);
  std::vector<uint64_t> hands = DealHands();
  REQUIRE(hands.size() == 4);
  REQUIRE(HandDebugString(hands[0]) == "S 42; H T82; D KQT93; C Q92; ");
  REQUIRE(HandDebugString(hands[1]) == "S KQ9; H Q3; D A754; C A765; ");
  REQUIRE(HandDebugString(hands[2]) == "S AJ863; H KJ976; D -; C KJ3; ");
  REQUIRE(HandDebugString(hands[3]) == "S T75; H A54; D J862; C T84; ");
}

TEST_CASE("ParseHand") {
  // std::string hand_str = "S AJ863; H KJ976; D -; C KJ3; ";
  std::string hand_str = "S T75; H A54; D J862; C T84; ";
  uint64_t hand_int = ParseHand(hand_str);
  REQUIRE(HandDebugString(hand_int) == hand_str);

  hand_str = "S AJ863; H KJ976; D -; C KJ3; ";
  hand_int = ParseHand(hand_str);
  REQUIRE(HandDebugString(hand_int) == hand_str);

  hand_str = "S AJ863";
  hand_int = ParseHand(hand_str);
  REQUIRE(HandDebugString(hand_int) == hand_str + "; H -; D -; C -; ");
}

TEST_CASE("BoardState") {
  srand48(0);
  std::vector<uint64_t> hands = DealHands();
  BoardState state;
  state.SetHands(hands);

  REQUIRE(HandDebugString(MakeHandFromCards(state.ValidCardsToPlay())) ==
          "S 42; H T82; D KQT93; C Q92; ");
  state.PlayCard(0, Card::ParseStringCard("S4"));

  REQUIRE(HandDebugString(MakeHandFromCards(state.ValidCardsToPlay())) ==
          "S KQ9; H -; D -; C -; ");
  state.PlayCard(1, Card::ParseStringCard("SK"));
  state.PlayCard(2, Card::ParseStringCard("S3"));
  state.PlayCard(3, Card::ParseStringCard("S5"));
  REQUIRE(state.CurPlayer() == 1);
  REQUIRE(state.tricks_won_odd() == 1);
  REQUIRE(state.tricks_won_even() == 0);

  REQUIRE(HandDebugString(state.played_cards()) == "S K543; H -; D -; C -; ");

  state.SetTrump(SPADES);
  state.PlayCard(1, Card::ParseStringCard("DA"));

  REQUIRE(HandDebugString(MakeHandFromCards(state.ValidCardsToPlay())) ==
          "S AJ86; H KJ976; D -; C KJ3; ");
  state.PlayCard(2, Card::ParseStringCard("S6"));

  REQUIRE(HandDebugString(MakeHandFromCards(state.ValidCardsToPlay())) ==
          "S -; H -; D J862; C -; ");
  state.PlayCard(3, Card::ParseStringCard("D2"));
  state.PlayCard(0, Card::ParseStringCard("D3"));
  REQUIRE(state.CurPlayer() == 2);
  REQUIRE(state.tricks_won_odd() == 1);
  REQUIRE(state.tricks_won_even() == 1);
  REQUIRE(HandDebugString(state.played_cards()) ==
          "S K6543; H -; D A32; C -; ");

  state.PlayCard(2, Card::ParseStringCard("S8"));
  state.PlayCard(3, Card::ParseStringCard("ST"));

  REQUIRE(HandDebugString(MakeHandFromCards(state.ValidCardsToPlay())) ==
          "S 2; H -; D -; C -; ");
  state.PlayCard(0, Card::ParseStringCard("S2"));
  state.PlayCard(1, Card::ParseStringCard("S9"));

  REQUIRE(state.CurPlayer() == 3);
  REQUIRE(state.tricks_won_odd() == 2);
  REQUIRE(state.tricks_won_even() == 1);

  state.PlayCard(3, Card::ParseStringCard("D8"));

  REQUIRE(HandDebugString(MakeHandFromCards(state.ValidCardsToPlay())) ==
          "S -; H -; D KQT9; C -; ");
  state.PlayCard(0, Card::ParseStringCard("D9"));

  REQUIRE(HandDebugString(MakeHandFromCards(state.ValidCardsToPlay())) ==
          "S -; H -; D 754; C -; ");
  state.PlayCard(1, Card::ParseStringCard("D7"));

  REQUIRE(HandDebugString(MakeHandFromCards(state.ValidCardsToPlay())) ==
          "S AJ; H KJ976; D -; C KJ3; ");
  state.PlayCard(2, Card::ParseStringCard("CK"));

  REQUIRE(state.CurPlayer() == 0);
  REQUIRE(state.tricks_won_even() == 2);
}

TEST_CASE("HandsKey") {
  srand48(0);
  for (int suit = 0; suit < 4; suit++) {
    std::string suit_str = SuitDebugString((Suit)suit);
    LOG(INFO) << "Testing " << suit_str;
    std::pair<uint64_t, uint64_t> key1, key2, key3;
    {
      std::vector<uint64_t> hands;
      hands.push_back(ParseHand(suit_str + " AQ"));
      hands.push_back(ParseHand(suit_str + " KJ"));
      hands.push_back(ParseHand(suit_str + " 87"));
      hands.push_back(ParseHand(suit_str + " 54"));
      BoardState bs;
      bs.SetHands(hands);
      bs.SetLead(0);

      LOG(INFO) << "key1";
      key1 = bs.RemainingHandsKey();
    }
    {
      std::vector<uint64_t> hands;
      hands.push_back(ParseHand(suit_str + " AJ"));
      hands.push_back(ParseHand(suit_str + " K9"));
      hands.push_back(ParseHand(suit_str + " 86"));
      hands.push_back(ParseHand(suit_str + " 52"));
      BoardState bs;
      bs.SetHands(hands);
      bs.SetLead(0);

      LOG(INFO) << "key2";
      key2 = bs.RemainingHandsKey();
    }
    REQUIRE(key1 == key2);
    {
      std::vector<uint64_t> hands;
      hands.push_back(ParseHand(suit_str + " AJ"));
      hands.push_back(ParseHand(suit_str + " K9"));
      hands.push_back(ParseHand(suit_str + " 83"));
      hands.push_back(ParseHand(suit_str + " 52"));
      BoardState bs;
      bs.SetHands(hands);
      bs.SetLead(0);

      LOG(INFO) << "key3";
      key3 = bs.RemainingHandsKey();
    }
    REQUIRE(key1 != key3);
  }
}

TEST_CASE("HandsKey2") {
  srand48(0);
  std::pair<uint64_t, uint64_t> key1, key2, key3;
  {
    std::vector<uint64_t> hands;
    // 8 cards
    hands.push_back(ParseHand("S AJ; H KQ; D 832; C 5"));
    hands.push_back(ParseHand("S K8; H T8543; D 5"));
    hands.push_back(ParseHand("S 72; H -; D JT97 C 98"));
    hands.push_back(ParseHand("S 6543; H -; D 65;  C T2"));
    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(0);

    LOG(INFO) << "key1";
    key1 = bs.RemainingHandsKey();
  }
  {
    std::vector<uint64_t> hands;
    hands.push_back(ParseHand("S AJ; H AT; D 832; C T"));
    hands.push_back(ParseHand("S K8; H 98543; D 5"));
    hands.push_back(ParseHand("S 72; H -; D JT97 C KJ"));
    hands.push_back(ParseHand("S 6543; H -; D 65;  C A3"));
    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(0);

    LOG(INFO) << "key2";
    key2 = bs.RemainingHandsKey();
  }
  REQUIRE(key1 == key2);
  {
    std::vector<uint64_t> hands;
    hands.push_back(ParseHand("S AJ; H KQ; D 832; C 5"));
    hands.push_back(ParseHand("S K8; H T8543; D 5"));
    hands.push_back(ParseHand("S 73; H -; D JT94 C 98"));
    hands.push_back(ParseHand("S 6542; H -; D 65;  C T2"));
    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(0);

    LOG(INFO) << "key3";
    key3 = bs.RemainingHandsKey();
  }
  REQUIRE(key1.first != key3.first);
  REQUIRE(key1.second != key3.second);
}

TEST_CASE("HandsKey3") {
  srand48(0);
  std::pair<uint64_t, uint64_t> key1, key2;
  {
    std::vector<uint64_t> hands;
    // 3 cards
    hands.push_back(ParseHand("D AT; C A; "));
    hands.push_back(ParseHand(" H A; D 54;  "));
    hands.push_back(ParseHand(" D 87; C K;  "));
    hands.push_back(ParseHand("S A; H KQ; "));
    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(0);

    LOG(INFO) << "key1";
    key1 = bs.RemainingHandsKey();
  }
  {
    std::vector<uint64_t> hands;
    // 3 cards
    hands.push_back(ParseHand("D AT; C A; "));
    hands.push_back(ParseHand(" H A; D 54;  "));
    hands.push_back(ParseHand(" D 87; C K;  "));
    hands.push_back(ParseHand("S AK; H K; "));
    BoardState bs;
    bs.SetHands(hands);
    bs.SetLead(0);

    LOG(INFO) << "key1";
    key2 = bs.RemainingHandsKey();
  }
  REQUIRE(key1 != key2);
}

void PrintOutResults(const OptimalLines &lines) {
  LOG(INFO) << "Total optimal lines: " << lines.size();
  int ctr = 0;
  for (const auto &line : lines) {
    LOG(INFO) << "Optimal line: " << PrintLine(line);
    if (ctr++ > 20) { break; }
  }
}

TEST_CASE("Solver2") {
  LOG(INFO) << "Solver2";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S AQ"));
  hands.push_back(ParseHand("S KJ"));
  hands.push_back(ParseHand("S 87"));
  hands.push_back(ParseHand("S 54"));
  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  REQUIRE(bs.Finished() == false);
  std::string seq;
  OptimalLines lines;
  int ans = MostTricksCanBeWon(bs, 0, &seq, &lines);
  PrintOutResults(lines);
  LOG(INFO) << "seq " << seq;
  REQUIRE(ans == 1);
  // REQUIRE(seq == "/ SQ SK S7 S4 / SJ S8 S5 SA ");
  REQUIRE(seq == "/ SA SJ S7 S4 / SQ SK S8 S5 ");
  REQUIRE(lines.size() == 2);
  REQUIRE_GOLDEN_LINE_RESULT(seq, "solver_2_seq.golden", hands, "Solver2: optimal play sequence for 2 spade tricks, Hand 0 on lead");
  REQUIRE_GOLDEN_LINE_RESULT(lines, "solver_2_lines.golden", hands, "Solver2: optimal lines for 2 spade tricks, Hand 0 on lead");

  bs.SetLead(1);
  seq.clear();
  REQUIRE(MostTricksCanBeWon(bs, 0, &seq) == 2);
  // REQUIRE(seq == "/ SJ S7 S4 SQ / SA SK S8 S5 ");

  bs.SetLead(2);
  seq.clear();
  REQUIRE(MostTricksCanBeWon(bs, 0, &seq) == 1);
  // REQUIRE(seq == "/ S7 S4 SQ SK / SJ S8 S5 SA ");
}


TEST_CASE("Solver2b") {
  LOG(INFO) << "Solver2b";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("SA HA"));
  hands.push_back(ParseHand("SK HK"));
  hands.push_back(ParseHand("H 8; D 7"));
  hands.push_back(ParseHand("C 54"));
  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  std::string seq;
  REQUIRE(MostTricksCanBeWon(bs, 0, &seq) == 2);
  // 9 or 19 depending on whether ALLOW_CLAIMS
  REQUIRE(num_solver_iters() == 23);
}

TEST_CASE("Solver2c") {
  LOG(INFO) << "Solver2";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S 87"));
  hands.push_back(ParseHand("S KJ"));
  hands.push_back(ParseHand("S AQ"));
  hands.push_back(ParseHand("S 54"));
  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  REQUIRE(bs.Finished() == false);
  std::string seq;
  OptimalLines lines;
  int ans = MostTricksCanBeWon(bs, 0, &seq, &lines);
  PrintOutResults(lines);
  LOG(INFO) << "seq " << seq;
  REQUIRE(ans == 2);
  // REQUIRE(seq == "/ SQ SK S7 S4 / SJ S8 S5 SA ");
  REQUIRE(lines.size() == 1);
  REQUIRE_GOLDEN_LINE_RESULT(seq, "solver_2c_seq.golden", hands, "Solver2c: optimal play sequence for spades trick layout, Hand 0 on lead");
  REQUIRE_GOLDEN_LINE_RESULT(lines, "solver_2c_lines.golden", hands, "Solver2c: optimal lines for spades trick layout, Hand 0 on lead");
}

TEST_CASE("Solver3a") {
  LOG(INFO) << "Solver3a";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S AQ9"));
  hands.push_back(ParseHand("S KJT"));
  hands.push_back(ParseHand("S 876"));
  hands.push_back(ParseHand("S 543"));
  BoardState bs;
  bs.SetHands(hands);
  std::string seq;
  REQUIRE(MostTricksCanBeWon(bs, 0, &seq) == 2);
  // REQUIRE(seq == "/ S9 ST S6 S3 / SJ S7 S4 SQ / SA SK S8 S5 ");
  REQUIRE(seq == "/ S9 ST S6 S3 / SK S7 S4 SA / SQ SJ S8 S5 ");
  REQUIRE(num_solver_iters() == 79);
  REQUIRE_GOLDEN_LINE_RESULT(seq, "solver_3a_seq.golden", hands, "Solver3a: optimal play sequence with 3-card spade hands, Hand 0 on lead");

  seq.clear();
  REQUIRE(MostTricksCanBeWon(bs, 1, &seq) == 1);
  // REQUIRE(seq == "/ S9 ST S6 S3 / SJ S7 S4 SQ / SA SK S8 S5 ");
  REQUIRE(seq == "/ S9 ST S6 S3 / SK S7 S4 SA / SQ SJ S8 S5 ");
}

TEST_CASE("Solver3") {
  LOG(INFO) << "Solver3";
  srand48(0);
  std::vector<uint64_t> hands;
  // hands.push_back(ParseHand("S 42; H T82; D KQT93; C Q92; "));
  // hands.push_back(ParseHand("S KQ9; H Q3; D A754; C A765; "));
  // hands.push_back(ParseHand("S AJ863; H KJ976; D -; C KJ3; "));
  // hands.push_back(ParseHand("S T75; H A54; D J862; C T84; "));

  hands.push_back(ParseHand("H -; D KQT9; C -; "));
  hands.push_back(ParseHand("H -; D A754; C -; "));
  hands.push_back(ParseHand("H KJ9; D 2; C -; "));
  hands.push_back(ParseHand("H -; D J863; C -; "));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  std::string seq;
  REQUIRE(MostTricksCanBeWon(bs, 0, &seq) == 2);
  // REQUIRE(seq == "/ SQ SK S7 S4 / SJ S8 S5 SA ");
  REQUIRE(num_solver_iters() == 789);
  REQUIRE_GOLDEN_LINE_RESULT(seq, "solver_3_seq.golden", hands, "Solver3: optimal play sequence for 4-card partial deal, Hand 0 on lead");
}

TEST_CASE("Solver3b") {
  LOG(INFO) << "Solver3b";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("HA DA CA"));
  hands.push_back(ParseHand("H KQJ"));
  hands.push_back(ParseHand("D KQJ"));
  hands.push_back(ParseHand("C KQJ"));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  std::string seq;
  REQUIRE(MostTricksCanBeWon(bs, 0, &seq) == 3);
  // 13 or 49 depending on ALLOW_CLAIMS
  REQUIRE(num_solver_iters() == 49);
}

TEST_CASE("Solver3c") {
  LOG(INFO) << "Solver3c";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("H -; D KJT97; C -; "));
  hands.push_back(ParseHand("H -; D 65432; C -; "));
  hands.push_back(ParseHand("H AJ974; "));
  hands.push_back(ParseHand("S AJ974; "));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(1);
  REQUIRE(MostTricksCanBeWon(bs, 1, nullptr) == 0);
  REQUIRE(num_solver_iters() == 21);

  bs.SetTrump(SPADES);
  REQUIRE(MostTricksCanBeWon(bs, 0, nullptr) == 0);
}

TEST_CASE("Solver3d") {
  LOG(INFO) << "Solver3d";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S 87 HA"));
  hands.push_back(ParseHand("S KJ HK"));
  hands.push_back(ParseHand("S AQ HQ"));
  hands.push_back(ParseHand("S 54 HJ"));
  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  REQUIRE(bs.Finished() == false);
  std::string seq;
  OptimalLines lines;
  int ans = MostTricksCanBeWon(bs, 0, &seq, &lines);
  PrintOutResults(lines);
  LOG(INFO) << "seq " << seq;
  REQUIRE(ans == 3);
  // REQUIRE(seq == "/ SQ SK S7 S4 / SJ S8 S5 SA ");
  REQUIRE(lines.size() == 3);
  REQUIRE_GOLDEN_LINE_RESULT(seq, "solver_3d_seq.golden", hands, "Solver3d: optimal play sequence with spades and hearts, Hand 0 on lead");
  REQUIRE_GOLDEN_LINE_RESULT(lines, "solver_3d_lines.golden", hands, "Solver3d: optimal lines with spades and hearts, Hand 0 on lead");
}

TEST_CASE("Solver4") {
  LOG(INFO) << "Solver4";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("H Q; D KQT9; C -; "));
  hands.push_back(ParseHand("H T; D A754; C -; "));
  hands.push_back(ParseHand("H KJ98; D 2; C -; "));
  hands.push_back(ParseHand("H A; D J863; C -; "));

  BoardState bs;
  bs.SetHands(hands);
  bs.MarkMissingCardsAsPlayed();
  bs.SetLead(0);
  // Initially required 29,166,855 iterations
  // Requires 15,123,631 iterations with tricks_remaining optimization
  // Requires 15,752,218 trying low cards first
  // 2nd hand low: 14,986,505
  // 4th hand win if you can: 14,799,576
  std::string seq;
  REQUIRE(MostTricksCanBeWon(bs, 1, &seq) == 3);
  LOG(INFO) << "Total iterations: " << num_solver_iters();
  REQUIRE(num_solver_iters() == 5581);
}

TEST_CASE("Solver4b") {
  LOG(INFO) << "Solver4b";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("H Q; D KQJ; C 9; "));
  hands.push_back(ParseHand("H T; D A75; C Q; "));
  hands.push_back(ParseHand("H KJ98; D 2; C -; "));
  hands.push_back(ParseHand("H A; D T86; C J; "));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(3);
  // 128,119,931 iterations
  // single threaded: 154,651,390 iterations
  REQUIRE(MostTricksCanBeWon(bs, 0, nullptr) == 2);
  REQUIRE(num_solver_iters() == 1873);
  // REQUIRE(seq == "/ SQ SK S7 S4 / SJ S8 S5 SA ");
  LOG(INFO) << "Total iterations: " << num_solver_iters();
}

TEST_CASE("Solver4c") {
  LOG(INFO) << "Solver4c";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S KQ; H KQ"));
  hands.push_back(ParseHand("S AJ; H AJ; "));
  hands.push_back(ParseHand("C AKQJ"));
  hands.push_back(ParseHand("S T9; H T9;"));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  REQUIRE(MostTricksCanBeWon(bs, 0, nullptr) == 2);
  // REQUIRE(seq == "/ SQ SK S7 S4 / SJ S8 S5 SA ");
  LOG(INFO) << "Total iterations: " << num_solver_iters();
  REQUIRE(num_solver_iters() == 171);
}

TEST_CASE("Solver4d") {
  LOG(INFO) << "Solver4d";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S 875 HA"));
  hands.push_back(ParseHand("S KJ9 H2"));
  hands.push_back(ParseHand("S AQT H3"));
  hands.push_back(ParseHand("S 432 H4"));
  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  REQUIRE(bs.Finished() == false);
  std::string seq;
  OptimalLines lines;
  int ans = MostTricksCanBeWon(bs, 0, &seq, &lines);
  PrintOutResults(lines);
  LOG(INFO) << "seq " << seq;
  REQUIRE(ans == 4);
  REQUIRE(lines.size() == 1);
  REQUIRE_GOLDEN_LINE_RESULT(seq, "solver_4d_seq.golden", hands, "Solver4d: optimal play sequence for 4-trick deal, Hand 0 on lead");
  REQUIRE_GOLDEN_LINE_RESULT(lines, "solver_4d_lines.golden", hands, "Solver4d: optimal lines for 4-trick deal, Hand 0 on lead");
}

TEST_CASE("Solver4e") {
  LOG(INFO) << "Solver4e";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S AK; H AK"));
  hands.push_back(ParseHand("S 9; H T; D AJ"));
  hands.push_back(ParseHand("S QJ; H QJ;"));
  hands.push_back(ParseHand("S T; H -; D KQT; C -;"));
  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  REQUIRE(bs.Finished() == false);
  std::string seq;
  OptimalLines lines;
  int ans = MostTricksCanBeWon(bs, 0, &seq, &lines);
  PrintOutResults(lines);
  LOG(INFO) << "seq " << seq;
  REQUIRE(ans == 4);
  REQUIRE(lines.size() == 6);
  REQUIRE_GOLDEN_LINE_RESULT(seq, "solver_4e_seq.golden", hands, "Solver4e: optimal play sequence for 4-card deal, Hand 0 on lead");
  REQUIRE_GOLDEN_LINE_RESULT(lines, "solver_4e_lines.golden", hands, "Solver4e: optimal lines for 4-card deal, Hand 0 on lead");
}

TEST_CASE("Solver4f") {
  LOG(INFO) << "Solver4f";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("D AT9; C A;  "));
  hands.push_back(ParseHand(" H A; D J54; "));
  hands.push_back(ParseHand("D K87; C K;  "));
  hands.push_back(ParseHand(" S AK; H KQ; "));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(2);
  bs.SetTrump(DIAMONDS);
  OptimalLines lines;
  int ans = MostTricksCanBeWon(bs, 0, nullptr, &lines);
  LOG(INFO) << "Total iterations: " << num_solver_iters();
  PrintOutResults(lines);
  REQUIRE(ans == 4);
  REQUIRE_GOLDEN_LINE_RESULT(lines, "solver_4f_lines.golden", hands, DIAMONDS, "Solver4f: optimal lines with diamond trump contract, Hand 2 on lead");
}

TEST_CASE("Solver5") {
  LOG(INFO) << "Solver5";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("H Q; D KQT9; C A; "));
  hands.push_back(ParseHand("H T; D A754; C Q; "));
  hands.push_back(ParseHand("H KJ98; D 2; C T; "));
  hands.push_back(ParseHand("H A; D J863; C 5; "));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  REQUIRE(MostTricksCanBeWon(bs, 1, nullptr) == 3);
  REQUIRE(num_solver_iters() == 17285);
  // REQUIRE(seq == "/ SQ SK S7 S4 / SJ S8 S5 SA ");
  LOG(INFO) << "Total iterations: " << num_solver_iters();
}

TEST_CASE("Solver5b") {
  LOG(INFO) << "Solver5b";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S KT2; D 32;"));
  hands.push_back(ParseHand("S 6; D KJT9;"));
  hands.push_back(ParseHand("S AJ987; "));
  hands.push_back(ParseHand("S Q54; D AQ; "));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  bs.SetTrump(SPADES);
  OptimalLines lines;
  REQUIRE(MostTricksCanBeWon(bs, 0, nullptr, &lines) == 5);
  REQUIRE(lines.size() == 62);
  LOG(INFO) << "Total iterations: " << num_solver_iters();
  REQUIRE_GOLDEN_LINE_RESULT(lines, "solver_5b_lines.golden", hands, SPADES, "Solver5b: optimal lines with spade contract, Hand 0 on lead");
}

TEST_CASE("Solver5c") {
  LOG(INFO) << "Solver5c";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("      D AKT9; C A;  "));
  hands.push_back(ParseHand(" H A; D J543; "));
  hands.push_back(ParseHand("      D Q876; C K;  "));
  hands.push_back(ParseHand(" S AKQ; H KQ; "));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  bs.SetTrump(DIAMONDS);
  OptimalLines lines;
  std::string seq;
  int ans = MostTricksCanBeWon(bs, 0, &seq, &lines);
  LOG(INFO) << "seq " << seq;
  PrintOutResults(lines);
  REQUIRE(ans == 5);
  REQUIRE(lines.size() == 10);
  LOG(INFO) << "Total iterations: " << num_solver_iters();
  REQUIRE_GOLDEN_LINE_RESULT(seq, "solver_5c_seq.golden", hands, DIAMONDS, "Solver5c: optimal play sequence with diamond trump, Hand 0 on lead");
  REQUIRE_GOLDEN_LINE_RESULT(lines, "solver_5c_lines.golden", hands, DIAMONDS, "Solver5c: optimal lines with diamond trump, Hand 0 on lead");
}

TEST_CASE("Solver6") {
  LOG(INFO) << "Solver6";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S KQJ; H KQJ"));
  hands.push_back(ParseHand("S AT8; H AT8; "));
  hands.push_back(ParseHand("C AKQJT9"));
  hands.push_back(ParseHand("S 976; H 976;"));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  REQUIRE(MostTricksCanBeWon(bs, 0, nullptr) == 4);
  REQUIRE(num_solver_iters() == 1695);
  // REQUIRE(seq == "/ SQ SK S7 S4 / SJ S8 S5 SA ");
  LOG(INFO) << "Total iterations: " << num_solver_iters();
}

TEST_CASE("Solver7") {
  LOG(INFO) << "Solver7";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("H Q7; D KQT9; C A; "));
  hands.push_back(ParseHand("H T5; D A754; C Q; "));
  hands.push_back(ParseHand("H KJ98; D 2; C T9; "));
  hands.push_back(ParseHand("H A4; D J863; C 5; "));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  OptimalLines lines;
  REQUIRE(MostTricksCanBeWon(bs, 1, nullptr, &lines) == 2);
  PrintOutResults(lines);
  LOG(INFO) << "Total iterations: " << num_solver_iters();
  REQUIRE(num_solver_iters_str() == "262K");
  REQUIRE_GOLDEN_LINE_RESULT(lines, "solver_7_lines.golden", hands, "Solver7: optimal lines for 7-trick layout, Hand 0 on lead");
}

TEST_CASE("Solver8", "[.slow]") {
  LOG(INFO) << "Solver8";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("H Q7; D KQT9; C A2; "));
  hands.push_back(ParseHand("H T5; D A754; C Q3; "));
  hands.push_back(ParseHand("H KJ98; D 2; C T94; "));
  hands.push_back(ParseHand("H A4; D J863; C 56; "));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  // 7.3B iterations
  OptimalLines lines;
  REQUIRE(MostTricksCanBeWon(bs, 1, nullptr, &lines) == 3);  // ???
  PrintOutResults(lines);
  LOG(INFO) << "Total iterations: " << num_solver_iters();
  REQUIRE(num_solver_iters_str() == "1M");
  REQUIRE_GOLDEN_LINE_RESULT(lines, "solver_8_lines.golden", hands, "Solver8: optimal lines for solver8 layout, Hand 0 on lead");
}

TEST_CASE("Solver8b", "[.slow]") {
  LOG(INFO) << "Solver8b";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("H Q7; D KQT9; C A2; "));
  hands.push_back(ParseHand("H A4; D J863; C 56; "));
  hands.push_back(ParseHand("H KJ98; D 2; C T94; "));
  hands.push_back(ParseHand("H T5; D A754; C Q3; "));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  // 7.3B iterations
  OptimalLines lines;
  REQUIRE(MostTricksCanBeWon(bs, 1, nullptr, &lines) == 3);  // ???
  PrintOutResults(lines);
  LOG(INFO) << "Total iterations: " << num_solver_iters();
  REQUIRE(num_solver_iters_str() == "1M");
  REQUIRE_GOLDEN_LINE_RESULT(lines, "solver_8b_lines.golden", hands, "Solver8b: optimal lines for solver8b layout, Hand 0 on lead");
}

TEST_CASE("Solver8c", "[.slow]") {
  LOG(INFO) << "Solver8c";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("H Q7; D KQT9; C A2; "));
  hands.push_back(ParseHand("H A4; D J863; C 56; "));
  hands.push_back(ParseHand("H KJ98; D 2; C T94; "));
  hands.push_back(ParseHand("H T52; D A754; C Q; "));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  // 7.3B iterations
  OptimalLines lines;
  int ans = MostTricksCanBeWon(bs, 1, nullptr, &lines);
  PrintOutResults(lines);
  LOG(INFO) << "Total iterations: " << num_solver_iters();
  REQUIRE(num_solver_iters_str() == "1M");
  REQUIRE(ans == 2);
  REQUIRE_GOLDEN_LINE_RESULT(lines, "solver_8c_lines.golden", hands, "Solver8c: optimal lines for solver8c layout, Hand 0 on lead");
}

TEST_CASE("Solver9") {
  LOG(INFO) << "Solver9";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S KQJ; H KQJ; D KQJ"));
  hands.push_back(ParseHand("S AT8; H AT8; D 876; "));
  hands.push_back(ParseHand("C AKQJT9876"));
  hands.push_back(ParseHand("S 976; H 976; D AT9; "));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  OptimalLines lines;
  REQUIRE(MostTricksCanBeWon(bs, 0, nullptr, &lines) == 6);
  PrintOutResults(lines);
  LOG(INFO) << "Total iterations: " << num_solver_iters();
  REQUIRE_GOLDEN_LINE_RESULT(lines, "solver_9_lines.golden", hands, "Solver9: optimal lines for 9-trick layout, Hand 0 on lead");
}

TEST_CASE("Solver9b", "[.slow]") {
  LOG(INFO) << "Solver9b";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S -; H T8; D KQT9; C Q94; "));
  hands.push_back(ParseHand("S -; H Q; D A754; C A765; "));
  hands.push_back(ParseHand("S AJ; H KJ976; D -; C KJ; "));
  hands.push_back(ParseHand("S T; H A54; D J86; C T8; "));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  OptimalLines lines;
  REQUIRE(MostTricksCanBeWon(bs, 0, nullptr, &lines) == 6);  // ??
  PrintOutResults(lines);
  LOG(INFO) << "Total iterations: " << num_solver_iters();
}

TEST_CASE("Solver10", "[.slow]") {
  LOG(INFO) << "Solver10";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S 764; H 32; D K64; C 82"));
  hands.push_back(ParseHand("S -; H -; D AQT9872; C J63"));
  hands.push_back(ParseHand("S -; H Q7; D 53; C AQT754"));
  hands.push_back(ParseHand("S QJT852; H T; DJ; C K9"));

  // MostTricksCanBeWon solved after 1,272,988,688 with cache having 36,731,221
  // 262.144u 0.832s 4:23.15 99.9%   0+0k 0+0io 0pf+0w

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  bs.SetTrump(HEARTS);
  bs.PlayCard(0, Card::ParseStringCard("C2"));
  bs.PlayCard(1, Card::ParseStringCard("C3"));
  bs.PlayCard(2, Card::ParseStringCard("CQ"));
  std::string seq;
  OptimalLines lines;
  REQUIRE(MostTricksCanBeWon(bs, 0, &seq, &lines) == 6);  // ??
  PrintOutResults(lines);
  LOG(INFO) << "Total iterations: " << num_solver_iters();
  REQUIRE(num_solver_iters_str() == "11M");
  LOG(INFO) << "Line: " << seq;
  REQUIRE_GOLDEN_LINE_RESULT(seq, "solver_10_seq.golden", hands, HEARTS, "Solver10: optimal play sequence for hearts contract, Hand 0 on lead");
  REQUIRE_GOLDEN_LINE_RESULT(lines, "solver_10_lines.golden", hands, HEARTS, "Solver10: optimal lines for hearts contract, Hand 0 on lead");
  // MostTricksCanBeWon solved after 7111325 with cache having 206653
  // 0.952u 0.004s 0:00.95 100.0%    0+0k 0+0io 0pf+0w
}

#if 0
TEST_CASE("Solver11") {
  LOG(INFO) << "Solver11";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S 94; H T8; D KQT9; C Q94; "));
  hands.push_back(ParseHand("S KQ; H Q; D A754; C A765; "));
  hands.push_back(ParseHand("S AJ86; H KJ976; D -; C KJ; "));
  hands.push_back(ParseHand("S T75; H A54; D J86; C T8; "));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  std::string seq;
  REQUIRE(MostTricksCanBeWon(bs, 0, &seq) == 7);  // ??
  LOG(INFO) << "Total iterations: " << num_solver_iters();
  // MostTricksCanBeWon solved after 8,709,766,653 with cache having 192,416,041
  // 1473.068u 5.119s 24:39.18 99.9% 0+0k 0+0io 0pf+0w

  // After better memoization:
  // MostTricksCanBeWon solved after 216,685,432 with cache having 4,028,049
  // 35.411u 0.072s 0:35.49 99.9%    0+0k 0+0io 0pf+0w
}

TEST_CASE("Solver12") {
  LOG(INFO) << "Solver12";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S 4; H T83; D KQT93; C Q93; "));
  hands.push_back(ParseHand("S KQ9; H Q; D A754; C A765; "));
  hands.push_back(ParseHand("S AJ863; H KJ976; D -; C KJ; "));
  hands.push_back(ParseHand("S T75; H A54; D J86; C T84; "));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  int dd = MostTricksCanBeWon(bs, 0, nullptr);
  LOG(INFO) << "MostTricksCanBeWon " << dd;
  REQUIRE(dd == 7);
  LOG(INFO) << "Total iterations: " << num_solver_iters_str();
  // MostTricksCanBeWon solved after 3,454,569,914 with cache having 45,501,969
  // 762.230u 1.336s 12:43.95 99.9%  0+0k 0+0io 0pf+0w

  // After intervening-played-cards opt:
  // MostTricksCanBeWon solved after 3,430,497,387 with cache having 45,521,646
  // 773.691u 1.416s 12:55.52 99.9%  0+0k 0+0io 0pf+0w
}

TEST_CASE("Solver13") {
  // Hand 1, 3/8/23
  LOG(INFO) << "Solver13";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S KQ96; HA; DJK93; CT982"));
  hands.push_back(ParseHand("S T542; HKJ5; DA652; CK7"));
  hands.push_back(ParseHand("S3; HQ8743; DQ74; CAQJ3"));
  hands.push_back(ParseHand("S AJ87; HT962; DT8; C654"));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(1);
  bs.SetTrump(CLUBS);
  int dd = MostTricksCanBeWon(bs, 0, nullptr);
  LOG(INFO) << "dd is " << dd;
  LOG(INFO) << "Total iterations: " << num_solver_iters_str();
  REQUIRE(dd == 10);
  // MostTricksCanBeWon solved after 4,353,698,625 with cache having 65,178,868
  // 912.467u 1.884s 15:14.83 99.9%  0+0k 0+0io 0pf+0w

  // After intervening-played-cards opt:
  // MostTricksCanBeWon solved after 4,320,251,791 with cache having 65,178,868
  // 1046.598u 2.031s 17:29.39 99.9% 0+0k 0+0io 0pf+0w

  // Total iterations: 5B
  // 1173.141u 1.887s 19:35.69 99.9% 0+0k 0+0io 0pf+0w

}

TEST_CASE("Solver13b") {
  LOG(INFO) << "Solver13b";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S 42; H T82; D KQT93; C Q92; "));
  hands.push_back(ParseHand("S KQ9; H Q3; D A754; C A765; "));
  hands.push_back(ParseHand("S AJ863; H KJ976; D -; C KJ3; "));
  hands.push_back(ParseHand("S T75; H A54; D J862; C T84; "));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(0);
  bs.SetTrump(NOTRUMP);
  int dd = MostTricksCanBeWon(bs, 0, nullptr);
  LOG(INFO) << "dd is " << dd;
  REQUIRE(dd >= 5);
  LOG(INFO) << "Total iterations: " << num_solver_iters_str();
  // MostTricksCanBeWon solved 27,785,173,522 with cache having 330,146,725
  // 1hr 40.

  // Total iterations: 32B
  // 7074.347u 10.050s 1:58:07.25 99.9%      0+0k 3640+0io 21pf+0w
}

TEST_CASE("Solver13c") {
  LOG(INFO) << "Solver13c";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S 42; H T82; D KQT93; C Q92; "));
  hands.push_back(ParseHand("S KQ9; H Q3; D A754; C A765; "));
  hands.push_back(ParseHand("S AJ863; H KJ976; D -; C KJ3; "));
  hands.push_back(ParseHand("S T75; H A54; D J862; C T84; "));

  BoardState bs;
  bs.SetHands(hands);
  bs.SetLead(1);
  bs.SetTrump(HEARTS);
  int dd = MostTricksCanBeWon(bs, 0, nullptr);
  LOG(INFO) << "dd is " << dd;
  REQUIRE(dd >= 5);
  LOG(INFO) << "Total iterations: " << num_solver_iters_str();
  // MostTricksCanBeWon solved 27,785,173,522 with cache having 330,146,725
  // 1hr 40.

  // Total iterations: 32B
  // 7074.347u 10.050s 1:58:07.25 99.9%      0+0k 3640+0io 21pf+0w
}
#endif
