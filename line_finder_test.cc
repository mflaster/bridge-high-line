#include <catch2/catch_all.hpp>

#include "line_finder.h"
#include "deal.h"

TEST_CASE("GeneralizedDemiPlay1") {
  GeneralizedDemiPlay line(HEARTS);
  for (int i = 0; i < 5; i++) {
    line.RecordOptimalPlay("", "HA", i);
  }
  for (int i = 5; i < 8; i++) {
    line.RecordOptimalPlay("", "HK", i);
  }
  for (int i = 8; i < 10; i++) {
    line.RecordOptimalPlay("", "HQ", i);
  }

  GeneralizedPlaysVector top_plays = line.GetGeneralizedPlays(0, HEARTS);
  LOG(INFO) << "Got top_plays:\n" << GeneralizedDemiPlay::PrintGeneralizedPlays(
      top_plays);
  REQUIRE(top_plays.size() == 3);
}

TEST_CASE("GeneralizedDemiPlay2") {
  GeneralizedDemiPlay line(HEARTS);
  for (int i = 0; i < 5; i++) {
    std::string opp_card = "H2";
    opp_card[1] = '2' + i;
    line.RecordOptimalPlay(opp_card, "HA", i);
  }
  for (int i = 5; i < 8; i++) {
    const char *opps_cards[3] = {"H2", "HJ", "S3"};
    line.RecordOptimalPlay(opps_cards[i-5], "HK", i);
  }
  for (int i = 8; i < 10; i++) {
    std::string opp_card = "H9";
    if (i == 8) {
      opp_card[1] = 'T';
    }
    line.RecordOptimalPlay(opp_card, "HQ", i);
  }

  GeneralizedPlaysVector top_plays = line.GetGeneralizedPlays(0, HEARTS, 40);
  VLOG(1) << "Got top_plays:\n" << GeneralizedDemiPlay::PrintGeneralizedPlays(
      top_plays);
  // 14 if you consider low cards, 9 cards otherwise
  REQUIRE((top_plays.size() == 9 || top_plays.size() == 10));
}

TEST_CASE("GeneralizedTrickFinders") {
  // From solver8
  std::vector<std::string> input = {
    "H7 H5 H8 H4",
    "HQ H5 HK H4",
    "H7 HT HJ H4",
    "HQ H5 HK HA",
    "H7 HT HJ HA",
    "HQ HT HK H4",
    "H7 H5 H8 HA",
    "H7 H5 HJ HA",
    "HQ HT HK HA",
    "H7 H5 HJ H4",
    "CA C3 C4 C5",
    "CA C3 C9 C5",
  };

  // From solver8b
  std::vector<std::string> input2 = {
    "H7 H4 HJ H5",
    "H7 H4 HK H5",
    "HQ H4 H8 H5",
    "HQ H4 HJ HT",
    "CA C5 C9 C3",
    "CA C5 C4 C3",
    "HQ H4 HK HT",
    "DQ D8 D2 D4",
    "HQ H4 HK H5",
    "DQ D6 D2 DA",
    "DQ D6 D2 D7",
    "DQ D8 D2 DA",
    "DQ D6 D2 D4",
    "HQ H4 HJ H5",
    "D9 DJ D2 D7",
    "H7 H4 HK HT",
    "DQ D3 D2 DA",
    "DQ D8 D2 D7",
    "HQ H4 H8 HT",
    "DQ D3 D2 D7",
    "H7 H4 HJ HT",
    "DQ D3 D2 D4",
  };

  // From solver8c
  std::vector<std::string> input3 = {
    "H7 HA HJ HT",
    "H7 HA H8 H2",
    "H7 HA H8 HT",
    "H7 H4 HK HT",
    "H7 HA HJ H2",
    "H7 H4 HK H2",
    "H7 H4 HJ HT",
    "H7 H4 HJ H2",
    "H7 HA HK H5",
    "H7 HA HK HT",
    "HQ H4 H8 H2",
    "H7 H4 HK H5",
    "CA C5 C4 CQ",
    "HQ HA H8 H2",
    "HQ HA H8 HT",
    "HQ H4 HK HT",
    "HQ H4 HK H5",
    "H7 HA HK H2",
    "HQ H4 HJ H5",
    "HQ H4 HJ HT",
    "HQ H4 HJ H2",
    "H7 HA H8 H5",
  };
  for (int leader = 0; leader < 2; leader++) {
    GeneralizedTrickFinder gen_trick(HEARTS);
    for (const auto &trick : input) {
      gen_trick.RecordOptimalTrick(leader, trick, 0);
    }
    for (const auto &trick : input2) {
      gen_trick.RecordOptimalTrick(leader, trick, 1);
    }
    for (const auto &trick : input3) {
      gen_trick.RecordOptimalTrick(leader, trick, 2);
    }

    GeneralizedTricksVector gen_tricks = gen_trick.GetGeneralizedTricks(20);
    LOG(INFO) << "Leader " << leader << ": got top tricks\n"
              << GeneralizedTrickFinder::PrintGeneralizedTricks(gen_tricks);
    REQUIRE(gen_tricks.size() == 20);
    if (leader == 0) {
      bool found = false;
      for (size_t i = 0; i < std::min<size_t>(5, gen_tricks.size()); i++) {
        if (gen_tricks[i].ToString().find("CA (") != std::string::npos &&
            gen_tricks[i].ToString().find("C4 (") != std::string::npos) {
          found = true;
          break;
        }
      }
      REQUIRE(found);
    } else {
      bool found = false;
      for (size_t i = 0; i < std::min<size_t>(10, gen_tricks.size()); i++) {
        if (gen_tricks[i].ToString().find("H4") != std::string::npos &&
            gen_tricks[i].ToString().find("H5") != std::string::npos) {
          found = true;
          break;
        }
      }
      REQUIRE(found);
    }
  }
}

std::string PrintBestLine(const std::vector<ConditionalLine> &lines) {
  VLOG(1) << "PrintBestLine on " << lines.size();
  uint shortest = 0;
  std::string text;
  for (const ConditionalLine &line : lines) {
    std::string x = line.Describe();
    if (text.empty() || x.size() < shortest) {
      shortest = x.size();
      text = x;
    }
  }
  VLOG(1) << "Done with PrintBestLine";
  return text;
}


TEST_CASE("BestLead1") {
  LOG(INFO) << "BestLead1";
  srand48(0);
  std::vector<uint64_t> hands;
  uint64_t hand = ParseHand("S97; HQJ3; DQ972; CJ876");
  hands.push_back(0);
  hands.push_back(hand);
  hands.push_back(0);
  hands.push_back(0);
  uint64_t unknown_cards = ~hand;

  LineFinder lf(500);

  std::vector<std::vector<std::shared_ptr<HandRequirements>>> reqs;
  reqs.push_back(
      std::vector<std::shared_ptr<HandRequirements>>({
          std::shared_ptr<HandRequirements>(new SuitLength(SPADES, 4, 5)),
          std::shared_ptr<HandRequirements>(new BalancedHand()),
          std::shared_ptr<HandRequirements>(new HighCardRange(15, 17)),
        }));
  reqs.push_back(std::vector<std::shared_ptr<HandRequirements>>());
  reqs.push_back(std::vector<std::shared_ptr<HandRequirements>>({
          std::shared_ptr<HandRequirements>(new SuitLength(SPADES, 4, 5)),
          std::shared_ptr<HandRequirements>(new HighCardRange(8, 9))
      }));
  lf.SetRequirements(reqs);

  int leader = 1;
  int min_tricks = 0;
  std::set<std::string> leads =
      lf.FindBestLead(min_tricks, hands, unknown_cards, leader, SPADES);
  LOG(INFO) << "Found best leads " << StrJoin(leads, ", ");
  // REQUIRE(leads.size() == 1);
}

TEST_CASE("BestContract1", "[.slow]") {
  LOG(INFO) << "BestContract1";
  srand48(0);
  std::vector<uint64_t> hands;
  // uint64_t hand = ParseHand("SQJT98; H3; DKQ9; CQT42");
  uint64_t hand = ParseHand("SKQJT9; H3; DA92; CT432");
  hands.push_back(0);
  hands.push_back(0);
  hands.push_back(hand);
  hands.push_back(0);
  uint64_t unknown_cards = ~hand;

  LineFinder lf(500);

  std::vector<std::vector<std::shared_ptr<HandRequirements>>> reqs;
  reqs.push_back(
      std::vector<std::shared_ptr<HandRequirements>>({
          std::shared_ptr<HandRequirements>(new SuitLength(SPADES, 2, 2)),
          std::shared_ptr<HandRequirements>(new BalancedHand()),
          std::shared_ptr<HandRequirements>(new HighCardRange(15, 17)),
        }));
  reqs.push_back(std::vector<std::shared_ptr<HandRequirements>>());
  reqs.push_back(std::vector<std::shared_ptr<HandRequirements>>());
  lf.SetRequirements(reqs);

  int leader = 1;
  std::multiset<int> spade_tricks =
      lf.FindTricksWon(hands, unknown_cards, leader, SPADES);
  std::multiset<int> notrump_tricks =
      lf.FindTricksWon(hands, unknown_cards, leader, NOTRUMP);
  LOG(INFO) << "Spades:\n" << lf.TricksWonDebugString(spade_tricks);
  LOG(INFO) << "Notrump:\n" << lf.TricksWonDebugString(notrump_tricks);
}

TEST_CASE("BestContract2", "[.slow]") {
  LOG(INFO) << "BestContract2";
  srand48(0);
  std::set<uint64_t> better_for_spades;
  for (int i = 0; i < 50; i++) {
    std::vector<std::vector<std::shared_ptr<HandRequirements>>> reqs;
    reqs.push_back(
        std::vector<std::shared_ptr<HandRequirements>>({
            std::shared_ptr<HandRequirements>(new SuitLength(SPADES, 2, 2)),
            std::shared_ptr<HandRequirements>(new BalancedHand()),
            std::shared_ptr<HandRequirements>(new HighCardRange(15, 17)),
          }));
    reqs.push_back(std::vector<std::shared_ptr<HandRequirements>>());
    reqs.push_back(
        std::vector<std::shared_ptr<HandRequirements>>({
            std::shared_ptr<HandRequirements>(new SuitLength(SPADES, 5, 5)),
            std::shared_ptr<HandRequirements>(new AntiRequirement(
                std::shared_ptr<HandRequirements>(new BalancedHand()))),
            std::shared_ptr<HandRequirements>(new HighCardRange(8, 10)),
          }));

    // Pick a hand for responder.
    uint64_t responder = 0;
    {
      std::vector<uint64_t> hands;
      hands.push_back(0);
      hands.push_back(0);
      hands.push_back(0);
      hands.push_back(0);
      std::vector<uint64_t> dealt_hands = DealMissingCards(hands, reqs);
      responder = dealt_hands[2];
      LOG(INFO) << "Considering responder " << HandDebugString(responder);
    }

    std::vector<uint64_t> hands;
    hands.push_back(0);
    hands.push_back(0);
    hands.push_back(responder);
    hands.push_back(0);

    uint64_t unknown_cards = ~responder;
    LineFinder lf(500);

    reqs.push_back(std::vector<std::shared_ptr<HandRequirements>>());
    lf.SetRequirements(reqs);

    int leader = 1;
    std::multiset<int> spade_tricks =
        lf.FindTricksWon(hands, unknown_cards, leader, SPADES);
    std::multiset<int> notrump_tricks =
        lf.FindTricksWon(hands, unknown_cards, leader, NOTRUMP);
    LOG(INFO) << "Spades:\n" << lf.TricksWonDebugString(spade_tricks);
    LOG(INFO) << "Notrump:\n" << lf.TricksWonDebugString(notrump_tricks);
    int spade_game = lf.CountAtLeast(spade_tricks, 10);
    int nt_game = lf.CountAtLeast(notrump_tricks, 9);
    if (abs(spade_game - nt_game) <= 5) {
      LOG(INFO) << "Better for neither "
                << i << ", " << spade_game << " vs " << nt_game
                << ": " << HandDebugString(responder);
    } else if (spade_game >= nt_game) {
      better_for_spades.insert(responder);
      LOG(INFO) << "Better for spades "
                << i << ", " << spade_game << " vs " << nt_game
                << ": " << HandDebugString(responder);
    } else {
      LOG(INFO) << "Better for NT "
                << i << ", " << nt_game << " vs " << spade_game
                << ": " << HandDebugString(responder);
    }
  }
  LOG(INFO) << "Total hands better for spades: " << better_for_spades.size();
}

TEST_CASE("BestContract3", "[.slow]") {
  LOG(INFO) << "BestContract3";
  srand48(0);
  std::vector<uint64_t> hands;
  // uint64_t hand = ParseHand("SKQJT9; H3; DA92; CT432");
  uint64_t hand = ParseHand("SQ6; HKQT983; DK7; CAJ3");
  hands.push_back(0);
  hands.push_back(0);
  hands.push_back(hand);
  hands.push_back(0);
  uint64_t unknown_cards = ~hand;

  LineFinder lf(500);

  std::vector<std::vector<std::shared_ptr<HandRequirements>>> reqs;
  reqs.push_back(
      std::vector<std::shared_ptr<HandRequirements>>({
          std::shared_ptr<HandRequirements>(new AntiRequirement(
              std::shared_ptr<HandRequirements>(new AtLeastAces(2)))),
          std::shared_ptr<HandRequirements>(new BalancedHand()),
          std::shared_ptr<HandRequirements>(new HighCardRange(15, 17)),
        }));
  reqs.push_back(std::vector<std::shared_ptr<HandRequirements>>());
  reqs.push_back(std::vector<std::shared_ptr<HandRequirements>>());
  lf.SetRequirements(reqs);

  int leader = 1;
  std::multiset<int> heart_tricks =
      lf.FindTricksWon(hands, unknown_cards, leader, HEARTS);
  LOG(INFO) << "Hearts:\n" << lf.TricksWonDebugString(heart_tricks);
}


TEST_CASE("LineFinder2") {
  LOG(INFO) << "LineFinder2";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S32"));
  hands.push_back(0);
  hands.push_back(ParseHand("SAQ"));
  hands.push_back(0);
  uint64_t unknown_cards = ParseHand("SK987");

  LineFinder lf(10);
  int leader = 0;
  std::vector<CardType> cur_trick;
  std::vector<ConditionalLine> lines =
      lf.Solve(2, hands, unknown_cards, leader, SPADES, cur_trick);
  LOG(INFO) << "Found lines " << lines.size();
  LOG(INFO) << PrintBestLine(lines);
  REQUIRE(lines.size() >= 1);
  REQUIRE(lines[0].CoveredBoards() >= 3);
}

TEST_CASE("LineFinder3") {
  LOG(INFO) << "LineFinder3";
  srand48(5);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("SK32"));
  hands.push_back(0);
  hands.push_back(ParseHand("SAJ4"));
  hands.push_back(0);
  uint64_t unknown_cards = ParseHand("SQT9876 H - ");

  for (int i = 0; i < 0; i++) {
    DealTwoHands(unknown_cards);
  }

  LineFinder lf(20);
  int leader = 0;
  std::vector<CardType> cur_trick;
  std::vector<ConditionalLine> lines =
      lf.Solve(3, hands, unknown_cards, leader, SPADES, cur_trick);
  LOG(INFO) << "Found lines " << lines.size();
  LOG(INFO) << PrintBestLine(lines);
  REQUIRE(lines.size() >= 1);
  REQUIRE(lines[0].CoveredBoards() >= 8);
}

TEST_CASE("LineFinder4") {
  LOG(INFO) << "LineFinder4";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("SAK HAK"));
  hands.push_back(0);
  hands.push_back(ParseHand("SQJ HQJ"));
  hands.push_back(0);
  uint64_t unknown_cards = ParseHand("ST9 HT DAKQJT");

  LineFinder lf(4);
  int leader = 0;
  std::vector<CardType> cur_trick;
  std::vector<ConditionalLine> lines =
      lf.Solve(0, hands, unknown_cards, leader, SPADES, cur_trick);
  LOG(INFO) << "Found lines " << lines.size();
  LOG(INFO) << PrintBestLine(lines);
  REQUIRE(lines[0].CoveredBoards() == 4);
}

TEST_CASE("LineFinder5", "[.slow]") {
  LOG(INFO) << "LineFinder5";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("SA HQJT9"));
  hands.push_back(0);
  hands.push_back(ParseHand("SK HA876"));
  hands.push_back(0);
  uint64_t unknown_cards = ParseHand("HK5432 DAKQJT");

  LineFinder lf(20);
  int leader = 0;
  std::vector<CardType> cur_trick;
  std::vector<ConditionalLine> lines =
      lf.Solve(5, hands, unknown_cards, leader, SPADES, cur_trick);
  LOG(INFO) << "Found lines " << lines.size();
  LOG(INFO) << PrintBestLine(lines);
  REQUIRE(lines.size() >= 1);
  REQUIRE(lines[0].CoveredBoards() == 10);
}

TEST_CASE("LineFinder5b", "[.slow]") {
  LOG(INFO) << "LineFinder5b";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("SA HKJT9"));
  hands.push_back(0);
  hands.push_back(ParseHand("SK HA876"));
  hands.push_back(0);
  uint64_t unknown_cards = ParseHand("HQ5432 DAKQJT");

  for (int i = 0; i < 500; i++) {
    DealTwoHands(unknown_cards);
  }

  LineFinder lf(20);
  int leader = 0;
  std::vector<CardType> cur_trick;
  std::vector<ConditionalLine> lines =
      lf.Solve(5, hands, unknown_cards, leader, SPADES, cur_trick);
  LOG(INFO) << "Found lines " << lines.size();
  LOG(INFO) << PrintBestLine(lines);
  REQUIRE(lines[0].CoveredBoards() >= 10);
}

TEST_CASE("LineFinder5c", "[.slow]") {
  LOG(INFO) << "LineFinder5c";
  srand48(1);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("SKT2 D32"));
  hands.push_back(0);
  hands.push_back(ParseHand("SAJ987"));
  hands.push_back(0);
  uint64_t unknown_cards = ParseHand("S Q654 D 987654 ");

  for (int i = 0; i < 25; i++) {
    DealTwoHands(unknown_cards);
  }

  int num_hands = 20;
  LineFinder lf(num_hands);
  int leader = 0;
  std::vector<CardType> cur_trick;
  std::vector<ConditionalLine> lines =
      lf.Solve(5, hands, unknown_cards, leader, SPADES, cur_trick);
  LOG(INFO) << "Found lines " << lines.size();
  LOG(INFO) << PrintBestLine(lines);
  REQUIRE(lines.size() >= 1);
  REQUIRE(lines[0].CoveredBoards() == num_hands);
}

TEST_CASE("LineFinder5d", "[.slow]") {
  LOG(INFO) << "LineFinder5d";
  srand48(5);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("D AKT9; C A"));
  hands.push_back(0);
  hands.push_back(ParseHand("D Q876; C K"));
  hands.push_back(0);
  uint64_t unknown_cards = ParseHand("DJ543 SAKQ HAKQ");

  for (int i = 0; i < 8; i++) {
    DealTwoHands(unknown_cards);
  }

  int num_deals = 100;
  LineFinder lf(num_deals);
  int leader = 0;
  std::vector<CardType> cur_trick;
  std::vector<ConditionalLine> lines =
      lf.Solve(5, hands, unknown_cards, leader, DIAMONDS, cur_trick);
  LOG(INFO) << "Found lines " << lines.size();
  // LOG(INFO) << PrintBestLine(lines);
  for (const ConditionalLine &line : lines) {
    LOG(INFO) << "  " << line.Describe();
  }
  REQUIRE(lines.size() >= 1);
  REQUIRE(lines[0].CoveredBoards() == num_deals);
}

TEST_CASE("LineFinder5e", "[.slow]") {
  LOG(INFO) << "LineFinder5e";
  srand48(1);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("SKT2 D32"));
  hands.push_back(0);
  hands.push_back(ParseHand("SAJ987"));
  hands.push_back(0);
  uint64_t unknown_cards = ParseHand("S Q6543 D 98765 ");

  for (int i = 0; i < 1; i++) {
    DealTwoHands(unknown_cards);
  }

  int num_hands = 100;
  LineFinder lf(num_hands);
  int leader = 0;
  std::vector<CardType> cur_trick;
  std::vector<ConditionalLine> lines =
      lf.Solve(5, hands, unknown_cards, leader, SPADES, cur_trick);
  LOG(INFO) << "Found lines " << lines.size();
  LOG(INFO) << PrintBestLine(lines);
  REQUIRE(lines.size() >= 1);
  REQUIRE(lines[0].CoveredBoards() == 58);
}

TEST_CASE("LineFinder7", "[.slow]") {
  LOG(INFO) << "LineFinder7";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("H Q7; D KQT9; C A; "));
  hands.push_back(0);
  hands.push_back(ParseHand("H KJ98; D 2; C T9; "));
  hands.push_back(0);
  uint64_t unknown_cards = ParseHand("H T5; D A754; C Q; "
                                     "H A4; D J863; C 5; ");

  LineFinder lf(20);
  int leader = 0;
  std::vector<CardType> cur_trick;
  std::vector<ConditionalLine> lines =
      lf.Solve(5, hands, unknown_cards, leader, SPADES, cur_trick);
  LOG(INFO) << "Found lines " << lines.size();
  LOG(INFO) << PrintBestLine(lines);
  REQUIRE(lines[0].CoveredBoards() == 6);
}

TEST_CASE("LineFinder9", "[.slow]") {
  LOG(INFO) << "LineFinder9";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S KQJ; H KQJ; D KQJ"));
  hands.push_back(0);
  hands.push_back(ParseHand("C AKQJT9876"));
  hands.push_back(0);
  uint64_t unknown_cards = ParseHand("S AT8; H AT8; D 876; "
                                     "S 976; H 976; D AT9; ");

  LineFinder lf(19);
  int leader = 0;
  std::vector<CardType> cur_trick;
  std::vector<ConditionalLine> lines =
      lf.Solve(6, hands, unknown_cards, leader, NOTRUMP, cur_trick);
  LOG(INFO) << "Found lines " << lines.size();
  LOG(INFO) << PrintBestLine(lines);
  REQUIRE(lines[0].CoveredBoards() > 3);
}

TEST_CASE("LineFinder11", "[.slow]") {
  LOG(INFO) << "LineFinder11";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("S 764 HA32 D K64 C82"));
  hands.push_back(0);
  hands.push_back(ParseHand("     H Q74 D 53 C AQT754"));
  hands.push_back(0);
  uint64_t unknown_cards = ParseHand(
      "D AQT9872; C J6; S QJT852; H T98; DJ; C K93");

  LineFinder lf(20);
  std::vector<std::shared_ptr<HandRequirements>> reqs1;
  std::vector<std::shared_ptr<HandRequirements>> reqs3;
  reqs3.push_back(std::shared_ptr<HandRequirements>(
      new SuitAtLeast(SPADES, 6)));
  lf.SetRequirements(reqs1, reqs3);
  int leader = 2;
  std::vector<CardType> cur_trick;
  std::vector<ConditionalLine> lines =
      lf.Solve(8, hands, unknown_cards, leader, HEARTS, cur_trick);
  LOG(INFO) << "Found lines " << lines.size();
  LOG(INFO) << PrintBestLine(lines);
  int i = 0;
  for (const auto &line : lines) {
    LOG(INFO) << "line " << ++i << ":\n" << line.Describe();
  }
  // REQUIRE(lines[0].CoveredBoards() == 17);
}

TEST_CASE("LineFinder12", "[.slow]") {
  LOG(INFO) << "LineFinder12";
  srand48(0);
  std::vector<uint64_t> hands;
  hands.push_back(ParseHand("SAJ4 HAKQJ74 DA5 CJ"));
  hands.push_back(0);
  hands.push_back(ParseHand("SK65 H6 DKQT73 C832"));
  hands.push_back(0);
  uint64_t unknown_cards = ParseHand(
      "SQT98732 HT98532 DJ98642 CQT976");

  LineFinder lf(20);
  std::vector<std::shared_ptr<HandRequirements>> reqs1;
  std::vector<std::shared_ptr<HandRequirements>> reqs3;
  reqs1.push_back(std::shared_ptr<HandRequirements>(
      new HandHasCard(Card::ParseStringCard("CQ"))));
  lf.SetRequirements(reqs1, reqs3);
  int leader = 2;
  std::vector<CardType> cur_trick;
  std::vector<ConditionalLine> lines =
      lf.Solve(12, hands, unknown_cards, leader, NOTRUMP, cur_trick);
  LOG(INFO) << "Found lines " << lines.size();
  LOG(INFO) << PrintBestLine(lines);
  int i = 0;
  for (const auto &line : lines) {
    LOG(INFO) << "line " << ++i << ":\n" << line.Describe();
  }
  // REQUIRE(lines[0].CoveredBoards() == 17);
}

TEST_CASE("LineFinder13", "[.slow]") {
  LOG(INFO) << "LineFinder13";
  srand48(0);
  uint64_t known_hand = ParseHand("SQ6; HKQT983; DK7; CAJ3");

  auto req = std::vector<std::shared_ptr<HandRequirements>>({
      std::shared_ptr<HandRequirements>(new AtLeastAces(2)),
      std::shared_ptr<HandRequirements>(new BalancedHand()),
      std::shared_ptr<HandRequirements>(new HighCardRange(15, 17)),
    });

  uint64_t orig_unknown_cards = ~known_hand;
  int more_than_half = 0;
  double num_hands_to_try = 30;
  for (int i = 0; i < num_hands_to_try; i++) {
    LOG(INFO) << "Trying hand " << i;
    uint64_t decl_hand = DealOneRequiredHand(orig_unknown_cards, req);
    std::vector<uint64_t> hands;
    hands.push_back(decl_hand);
    hands.push_back(0);
    hands.push_back(known_hand);
    hands.push_back(0);

    double num_boards = 11;
    LineFinder lf(num_boards);
    int leader = 1;
    std::vector<CardType> cur_trick;
    uint64_t dealt_cards = known_hand | decl_hand;
    uint64_t unknown_cards = ~dealt_cards;
    std::vector<ConditionalLine> lines =
        lf.Solve(12, hands, unknown_cards, leader, HEARTS, cur_trick);
    LOG(INFO) << "Found lines " << lines.size();
    LOG(INFO) << PrintBestLine(lines);
    if (lines.size() > 0 && lines[0].CoveredBoards() > num_boards * 0.5) {
      LOG(INFO) << "**Success on " << i << " with frac "
                << lines[0].CoveredBoards() / num_boards;
      more_than_half++;
    }
    // int j = 0;
        //for (const auto &line : lines) {
      // LOG(INFO) << "line " << ++j << ":\n" << line.Describe();
        // }
  }
  LOG(INFO) << "Found > 50% lines for " << more_than_half << " / "
            << num_hands_to_try << " = " << (more_than_half / num_hands_to_try);
}
