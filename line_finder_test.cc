#include <iostream>
#include <catch2/catch_all.hpp>

#include "golden_serializer.h"
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
  std::cout << "Got top_plays:\n" << GeneralizedDemiPlay::PrintGeneralizedPlays(
      top_plays) << "\n";
  REQUIRE(top_plays.size() == 3);
  REQUIRE_GOLDEN_LINE_RESULT(GeneralizedDemiPlay::PrintGeneralizedPlays(top_plays),
                             "generalized_demi_play_1.golden",
                             "GeneralizedDemiPlay1: top plays in hearts");
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
  std::cout << "Got top_plays:\n" << GeneralizedDemiPlay::PrintGeneralizedPlays(
      top_plays) << "\n";
  // 14 if you consider low cards, 9 cards otherwise
  REQUIRE((top_plays.size() == 9 || top_plays.size() == 10));
  REQUIRE_GOLDEN_LINE_RESULT(GeneralizedDemiPlay::PrintGeneralizedPlays(top_plays),
                             "generalized_demi_play_2.golden",
                             "GeneralizedDemiPlay2: top plays in hearts with opponent cards");
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
    std::cout << "Leader " << leader << ": got top tricks\n"
              << GeneralizedTrickFinder::PrintGeneralizedTricks(gen_tricks) << "\n";
    REQUIRE(gen_tricks.size() == 20);
    REQUIRE_GOLDEN_LINE_RESULT(GeneralizedTrickFinder::PrintGeneralizedTricks(gen_tricks),
                               "generalized_trick_finder_leader_" + std::to_string(leader) + ".golden",
                               "GeneralizedTrickFinders: leader " + std::to_string(leader) + " top tricks");
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
  std::cout << "PrintBestLine on " << lines.size() << "\n";
  uint shortest = 0;
  std::string text;
  for (const ConditionalLine &line : lines) {
    std::string x = line.Describe();
    if (text.empty() || x.size() < shortest) {
      shortest = x.size();
      text = x;
    }
  }
  std::cout << "Done with PrintBestLine" << "\n";
  return text;
}


TEST_CASE("BestLead1") {
  std::cout << "BestLead1" << "\n";
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
  std::cout << "Found best leads " << StrJoin(leads, ", ") << "\n";
  REQUIRE_GOLDEN_LINE_RESULT(StrJoin(leads, ", "), "best_lead_1.golden", hands, unknown_cards, SPADES, "BestLead1: best lead for spades contract, Hand 1 on lead");
  // REQUIRE(leads.size() == 1);
}

TEST_CASE("BestContract1", "[.slow]") {
  std::cout << "BestContract1" << "\n";
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
  std::cout << "Spades:\n" << lf.TricksWonDebugString(spade_tricks) << "\n";
  std::cout << "Notrump:\n" << lf.TricksWonDebugString(notrump_tricks) << "\n";
  REQUIRE_GOLDEN_LINE_RESULT(spade_tricks, "best_contract_1_spades.golden", hands, unknown_cards, SPADES, "BestContract1: spade contract trick distribution, Hand 1 on lead");
  REQUIRE_GOLDEN_LINE_RESULT(notrump_tricks, "best_contract_1_notrump.golden", hands, unknown_cards, NOTRUMP, "BestContract1: NT contract trick distribution, Hand 1 on lead");
}

TEST_CASE("BestContract2", "[.slow]") {
  std::cout << "BestContract2" << "\n";
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
      std::cout << "Considering responder " << HandDebugString(responder) << "\n";
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
    std::cout << "Spades:\n" << lf.TricksWonDebugString(spade_tricks) << "\n";
    std::cout << "Notrump:\n" << lf.TricksWonDebugString(notrump_tricks) << "\n";
    int spade_game = lf.CountAtLeast(spade_tricks, 10);
    int nt_game = lf.CountAtLeast(notrump_tricks, 9);
    if (abs(spade_game - nt_game) <= 5) {
      std::cout << "Better for neither "
                << i << ", " << spade_game << " vs " << nt_game
                << ": " << HandDebugString(responder) << "\n";
    } else if (spade_game >= nt_game) {
      better_for_spades.insert(responder);
      std::cout << "Better for spades "
                << i << ", " << spade_game << " vs " << nt_game
                << ": " << HandDebugString(responder) << "\n";
    } else {
      std::cout << "Better for NT "
                << i << ", " << nt_game << " vs " << spade_game
                << ": " << HandDebugString(responder) << "\n";
    }
  }
  std::cout << "Total hands better for spades: " << better_for_spades.size() << "\n";
}

TEST_CASE("BestContract3", "[.slow]") {
  std::cout << "BestContract3" << "\n";
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
  std::cout << "Hearts:\n" << lf.TricksWonDebugString(heart_tricks) << "\n";
  REQUIRE_GOLDEN_LINE_RESULT(heart_tricks, "best_contract_3_hearts.golden", hands, unknown_cards, HEARTS, "BestContract3: heart contract trick distribution, Hand 0 on lead");
}


struct LineFinderTestCase {
  std::string name;
  Suit trump = NOTRUMP;
  int leader = 0;
  int min_tricks = -1;
  int num_iters = 20;
  int seed = 0;
  int pre_solve_deals = 0;
  std::string hand0;
  std::string hand1;
  std::string hand2;
  std::string hand3;
  std::string unknown_cards;
  std::string description;
  std::vector<std::shared_ptr<HandRequirements>> reqs1;
  std::vector<std::shared_ptr<HandRequirements>> reqs3;
};

void RunLineFinderTestCase(const LineFinderTestCase &tc) {
  std::cout << "Running LineFinder test case: " << tc.name << "\n";
  srand48(tc.seed);

  std::vector<uint64_t> hands(4, 0);
  if (!tc.hand0.empty()) hands[0] = ParseHand(tc.hand0);
  if (!tc.hand1.empty()) hands[1] = ParseHand(tc.hand1);
  if (!tc.hand2.empty()) hands[2] = ParseHand(tc.hand2);
  if (!tc.hand3.empty()) hands[3] = ParseHand(tc.hand3);

  uint64_t unknown = tc.unknown_cards.empty() ? 0 : ParseHand(tc.unknown_cards);

  for (int i = 0; i < tc.pre_solve_deals; i++) {
    DealTwoHands(unknown);
  }

  int total_cards = CountCardsInHand(hands[0]) + CountCardsInHand(hands[1]) +
                    CountCardsInHand(hands[2]) + CountCardsInHand(hands[3]) +
                    CountCardsInHand(unknown);
  int min_tricks = tc.min_tricks;
  if (min_tricks < 0) {
    min_tricks = total_cards > 0 ? total_cards / 4 : 0;
  }

  LineFinder lf(tc.num_iters);
  if (!tc.reqs1.empty() || !tc.reqs3.empty()) {
    lf.SetRequirements(tc.reqs1, tc.reqs3);
  }

  std::vector<CardType> cur_trick;
  std::vector<ConditionalLine> lines =
      lf.Solve(min_tricks, hands, unknown, tc.leader, tc.trump, cur_trick);

  std::cout << "Found lines: " << lines.size() << "\n";
  REQUIRE(!lines.empty());

  std::string desc = tc.description;
  if (desc.empty()) {
    desc = tc.name + ": " + SuitDebugString(tc.trump) +
           " contract, Hand " + std::to_string(tc.leader) + " on lead";
  }

  REQUIRE_GOLDEN_LINE_RESULT(lines, tc.name + ".golden", hands, unknown, tc.trump, desc);
}

TEST_CASE("LineFinder2") {
  RunLineFinderTestCase({
    .name = "line_finder_2",
    .trump = SPADES,
    .leader = 0,
    .min_tricks = 2,
    .num_iters = 10,
    .hand0 = "S32",
    .hand2 = "SAQ",
    .unknown_cards = "SK987",
    .description = "LineFinder2: lines for spade contract with 2 known hands, Hand 0 on lead",
  });
}

TEST_CASE("LineFinder3") {
  RunLineFinderTestCase({
    .name = "line_finder_3",
    .trump = SPADES,
    .leader = 0,
    .min_tricks = 3,
    .num_iters = 20,
    .seed = 5,
    .hand0 = "SK32",
    .hand2 = "SAJ4",
    .unknown_cards = "SQT9876 H -",
    .description = "LineFinder3: lines for spade contract with 2 known hands, Hand 0 on lead",
  });
}

TEST_CASE("LineFinder4") {
  RunLineFinderTestCase({
    .name = "line_finder_4",
    .trump = SPADES,
    .leader = 0,
    .min_tricks = 4,
    .num_iters = 10,
    .hand0 = "SAK HAK",
    .hand2 = "SQJ HQJ",
    .unknown_cards = "ST9 HT DAKQJT",
    .description = "LineFinder4: lines for spade contract, Hand 0 on lead",
  });
}

TEST_CASE("LineFinder5", "[.slow]") {
  RunLineFinderTestCase({
    .name = "line_finder_5",
    .trump = SPADES,
    .leader = 0,
    .min_tricks = 5,
    .num_iters = 20,
    .hand0 = "SA HQJT9",
    .hand2 = "SK HA876",
    .unknown_cards = "HK5432 DAKQJT",
    .description = "LineFinder5: lines for spade contract, Hand 0 on lead",
  });
}

TEST_CASE("LineFinder5b", "[.slow]") {
  RunLineFinderTestCase({
    .name = "line_finder_5b",
    .trump = SPADES,
    .leader = 0,
    .min_tricks = 5,
    .num_iters = 20,
    .pre_solve_deals = 500,
    .hand0 = "SA HKJT9",
    .hand2 = "SK HA876",
    .unknown_cards = "HQ5432 DAKQJT",
    .description = "LineFinder5b: lines for spade contract, Hand 0 on lead",
  });
}

TEST_CASE("LineFinder5c", "[.slow]") {
  RunLineFinderTestCase({
    .name = "line_finder_5c",
    .trump = SPADES,
    .leader = 0,
    .min_tricks = 5,
    .num_iters = 20,
    .seed = 1,
    .pre_solve_deals = 25,
    .hand0 = "SKT2 D32",
    .hand2 = "SAJ987",
    .unknown_cards = "S Q654 D 987654",
    .description = "LineFinder5c: lines for spade contract, Hand 0 on lead",
  });
}

TEST_CASE("LineFinder5d", "[.slow]") {
  RunLineFinderTestCase({
    .name = "line_finder_5d",
    .trump = DIAMONDS,
    .leader = 0,
    .min_tricks = 5,
    .num_iters = 100,
    .seed = 5,
    .pre_solve_deals = 8,
    .hand0 = "D AKT9; C A",
    .hand2 = "D Q876; C K",
    .unknown_cards = "DJ543 SAKQ HAKQ",
    .description = "LineFinder5d: lines for diamond contract, Hand 0 on lead",
  });
}

TEST_CASE("LineFinder5e", "[.slow]") {
  RunLineFinderTestCase({
    .name = "line_finder_5e",
    .trump = SPADES,
    .leader = 0,
    .min_tricks = 5,
    .num_iters = 100,
    .seed = 1,
    .pre_solve_deals = 1,
    .hand0 = "SKT2 D32",
    .hand2 = "SAJ987",
    .unknown_cards = "S Q6543 D 98765",
    .description = "LineFinder5e: lines for spade contract, Hand 0 on lead",
  });
}

TEST_CASE("LineFinder7", "[.slow]") {
  RunLineFinderTestCase({
    .name = "line_finder_7",
    .trump = SPADES,
    .leader = 0,
    .min_tricks = 5,
    .num_iters = 20,
    .hand0 = "H Q7; D KQT9; C A;",
    .hand2 = "H KJ98; D 2; C T9;",
    .unknown_cards = "H T5; D A754; C Q; H A4; D J863; C 5;",
    .description = "LineFinder7: lines for spade contract, Hand 0 on lead",
  });
}

TEST_CASE("LineFinder9", "[.slow]") {
  RunLineFinderTestCase({
    .name = "line_finder_9",
    .trump = NOTRUMP,
    .leader = 0,
    .min_tricks = 6,
    .num_iters = 19,
    .hand0 = "S KQJ; H KQJ; D KQJ",
    .hand2 = "C AKQJT9876",
    .unknown_cards = "S AT8; H AT8; D 876; S 976; H 976; D AT9;",
    .description = "LineFinder9: lines for NT contract, Hand 0 on lead",
  });
}

TEST_CASE("LineFinder11", "[.slow]") {
  RunLineFinderTestCase({
    .name = "line_finder_11",
    .trump = HEARTS,
    .leader = 2,
    .min_tricks = 8,
    .num_iters = 20,
    .hand0 = "S 764 HA32 D K64 C82",
    .hand2 = "H Q74 D 53 C AQT754",
    .unknown_cards = "D AQT9872; C J6; S QJT852; H T98; DJ; C K93",
    .description = "LineFinder11: lines for heart contract with Spade requirements, Hand 2 on lead",
    .reqs3 = {HandRequirements::FromString("SuitAtLeast(Spades, 6)")},
  });
}

TEST_CASE("LineFinder12", "[.slow]") {
  RunLineFinderTestCase({
    .name = "line_finder_12",
    .trump = NOTRUMP,
    .leader = 2,
    .min_tricks = 12,
    .num_iters = 20,
    .hand0 = "SAJ4 HAKQJ74 DA5 CJ",
    .hand2 = "SK65 H6 DKQT73 C832",
    .unknown_cards = "SQT98732 HT98532 DJ98642 CQT976",
    .description = "LineFinder12: lines for NT contract with CQ requirement, Hand 2 on lead",
    .reqs1 = {HandRequirements::FromString("HandHasCard(CQ)")},
  });
}

TEST_CASE("LineFinder13", "[.slow]") {
  std::cout << "LineFinder13" << "\n";
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
    std::cout << "Trying hand " << i << "\n";
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
    std::cout << "Found lines " << lines.size() << "\n";
    std::cout << PrintBestLine(lines) << "\n";
    if (lines.size() > 0 && lines[0].CoveredBoards() > num_boards * 0.5) {
      std::cout << "**Success on " << i << " with frac "
                << lines[0].CoveredBoards() / num_boards << "\n";
      more_than_half++;
    }
    // int j = 0;
        //for (const auto &line : lines) {
      // std::cout << "line " << ++j << ":\n" << line.Describe() << "\n";
        // }
  }
  std::cout << "Found > 50% lines for " << more_than_half << " / "
            << num_hands_to_try << " = " << (more_than_half / num_hands_to_try) << "\n";
}

TEST_CASE("LineFinder_Squeeze1") {
  RunLineFinderTestCase({
    .name = "squeeze_1",
    .trump = NOTRUMP,
    .leader = 0,
    .min_tricks = 6,
    .num_iters = 60,
    .hand0 = "S A43 D AKQ",
    .hand2 = "S K2 H K D 432",
    .unknown_cards = "S QJT987 H AQJT98",
  });
}

TEST_CASE("LineFinder_TrumpSafety") {
  RunLineFinderTestCase({
    .name = "trump_safety",
    .trump = SPADES,
    .leader = 0,
    .num_iters = 20,
    .hand0 = "S AKT9",
    .hand2 = "S Q876",
    .unknown_cards = "S J543 D 5432",
    .reqs1 = {HandRequirements::FromString("Anti(SuitLength(Spades,1,3))")},
    .reqs3 = {HandRequirements::FromString("Anti(SuitLength(Spades,1,3))")},
  });
}

TEST_CASE("LineFinder_OnsideAce") {
  RunLineFinderTestCase({
    .name = "OnsideAce",
    .trump = CLUBS,
    .leader = 2,
    .min_tricks = 6,
    .num_iters = 52,
    .hand0 = "S K73; H 2; D 93; C 9;",
    .hand2 = "S -; H -; D K4; C QJT64;",
    .unknown_cards = "S AT854; H 87; D AQJT87; C 7;",
  });
}

TEST_CASE("LineFinder_4CardEndplay") {
  RunLineFinderTestCase({
    .name = "line_finder_4_card_endplay",
    .trump = HEARTS,
    .leader = 2,
    .min_tricks = 3,
    .num_iters = 30,
    .hand0 = "S AQT; H 2; D -; C -;",
    .hand2 = "S 876; H A; D -; C -;",
    .unknown_cards = "S KJ9543 D 34",
    .description = "LineFinder 4-Card Endplay Deal: lines for heart contract with 4-card endplay",
  });
}

TEST_CASE("LineFinder_OrInvariant") {
  RunLineFinderTestCase({
    .name = "or_invariant",
    .trump = DIAMONDS,
    .leader = 0,
    .min_tricks = 6,
    .num_iters = 10,
    .hand0 = "S QJT H QJT D A",
    .hand2 = "S A32 H A32 D 4",
    .unknown_cards = "S K987654 H K987654",
  });
}

TEST_CASE("LineFinder_AndInvariant") {
  RunLineFinderTestCase({
    .name = "and_invariant",
    .trump = DIAMONDS,
    .leader = 0,
    .min_tricks = 5,
    .num_iters = 10,
    .hand0 = "S 32 H 32 D A",
    .hand2 = "S AQ H AQ D 4",
    .unknown_cards = "S K7654 H K7654",
  });
}

TEST_CASE("LineFinder_MidTrickSolveAndPreassignedDefenderCard") {
  srand48(0);
  // 2-card Spades finesse deal: N has SAQ, S has S87, E/W share SK543.
  // Case 1: South leads S8 (1 card in cur_trick).
  {
    std::vector<uint64_t> hands = {
      ParseHand("S AQ"),
      0,
      ParseHand("S 87"),
      0,
    };
    uint64_t unknown_cards = ParseHand("S K543");
    std::vector<CardType> cur_trick = {Card::ParseStringCard("S8")};
    LineFinder lf(20);
    std::vector<ConditionalLine> lines =
        lf.Solve(2, hands, unknown_cards, /*leader=*/2, SPADES, cur_trick);
    REQUIRE(!lines.empty());
    REQUIRE(lines[0].CoveredBoards() > 0);
    REQUIRE(lines[0].Describe().find("S8") != std::string::npos);
  }

  // Case 2: South leads S8, West plays S3 from Unknown Cards (assigned to W).
  // Now West has 1 known card (S3) and 1 unknown card, while East has 2 unknown
  // cards from {SK, S5, S4}.
  {
    std::vector<uint64_t> hands = {
      ParseHand("S AQ"),
      0,
      ParseHand("S 87"),
      ParseHand("S 3"),
    };
    uint64_t unknown_cards = ParseHand("S K54");
    std::vector<CardType> cur_trick = {
      Card::ParseStringCard("S8"),
      Card::ParseStringCard("S3"),
    };
    LineFinder lf(20);
    std::vector<ConditionalLine> lines =
        lf.Solve(2, hands, unknown_cards, /*leader=*/2, SPADES, cur_trick);
    REQUIRE(!lines.empty());
    REQUIRE(lines[0].CoveredBoards() > 0);
    REQUIRE(lines[0].Describe().find("S8") != std::string::npos);
  }

  // Case 3: 3 cards in cur_trick (S8, S3, SQ played; East to play 4th card).
  {
    std::vector<uint64_t> hands = {
      ParseHand("S AQ"),
      0,
      ParseHand("S 87"),
      ParseHand("S 3"),
    };
    uint64_t unknown_cards = ParseHand("S K54");
    std::vector<CardType> cur_trick = {
      Card::ParseStringCard("S8"),
      Card::ParseStringCard("S3"),
      Card::ParseStringCard("SQ"),
    };
    LineFinder lf(20);
    std::vector<ConditionalLine> lines =
        lf.Solve(2, hands, unknown_cards, /*leader=*/2, SPADES, cur_trick);
    REQUIRE(!lines.empty());
    REQUIRE(lines[0].CoveredBoards() > 0);
    REQUIRE(lines[0].Describe().find("SQ") != std::string::npos);
  }
}
