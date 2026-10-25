#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <gtest/gtest.h>

#include "deal.h"
#include "golden_serializer.h"
#include "line_finder.h"
#include "omp_compat.h"

TEST(LineFinderTest, GeneralizedDemiPlay1) {
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
  ASSERT_TRUE(top_plays.size() == 3);
  REQUIRE_GOLDEN_LINE_RESULT(GeneralizedDemiPlay::PrintGeneralizedPlays(top_plays),
                             "generalized_demi_play_1.golden",
                             "GeneralizedDemiPlay1: top plays in hearts");
}

TEST(LineFinderTest, GeneralizedDemiPlay2) {
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
  ASSERT_TRUE((top_plays.size() == 9 || top_plays.size() == 10));
  REQUIRE_GOLDEN_LINE_RESULT(GeneralizedDemiPlay::PrintGeneralizedPlays(top_plays),
                             "generalized_demi_play_2.golden",
                             "GeneralizedDemiPlay2: top plays in hearts with opponent cards");
}

TEST(LineFinderTest, GeneralizedTrickFinders) {
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
    ASSERT_TRUE(gen_tricks.size() == 20);
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
      ASSERT_TRUE(found);
    } else {
      bool found = false;
      for (size_t i = 0; i < std::min<size_t>(10, gen_tricks.size()); i++) {
        if (gen_tricks[i].ToString().find("H4") != std::string::npos &&
            gen_tricks[i].ToString().find("H5") != std::string::npos) {
          found = true;
          break;
        }
      }
      ASSERT_TRUE(found);
    }
  }
}

std::string PrintBestLine(const std::vector<ConditionalLine> &lines) {
  std::cout << "PrintBestLine on " << lines.size() << "\n";
  unsigned int shortest = 0;
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


TEST(LineFinderTest, BestLead1) {
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
  // ASSERT_TRUE(leads.size() == 1);
}

TEST(LineFinderTest, DISABLED_BestContract1) {
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

TEST(LineFinderTest, DISABLED_BestContract2) {
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

TEST(LineFinderTest, DISABLED_BestContract3) {
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
  ASSERT_TRUE(!lines.empty());

  std::string desc = tc.description;
  if (desc.empty()) {
    desc = tc.name + ": " + SuitDebugString(tc.trump) +
           " contract, Hand " + std::to_string(tc.leader) + " on lead";
  }

  REQUIRE_GOLDEN_LINE_RESULT(lines, tc.name + ".golden", hands, unknown, tc.trump, desc);
}

TEST(LineFinderTest, LineFinder2) {
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

TEST(LineFinderTest, LineFinder3) {
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

TEST(LineFinderTest, LineFinder4) {
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

TEST(LineFinderTest, DISABLED_LineFinder5) {
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

TEST(LineFinderTest, DISABLED_LineFinder5b) {
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

TEST(LineFinderTest, DISABLED_LineFinder5c) {
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

TEST(LineFinderTest, DISABLED_LineFinder5d) {
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

TEST(LineFinderTest, DISABLED_LineFinder5e) {
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

TEST(LineFinderTest, DISABLED_LineFinder7) {
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

TEST(LineFinderTest, DISABLED_LineFinder9) {
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

TEST(LineFinderTest, DISABLED_LineFinder11) {
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

TEST(LineFinderTest, DISABLED_LineFinder12) {
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

TEST(LineFinderTest, DISABLED_LineFinder13) {
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

TEST(LineFinderTest, LineFinder_Squeeze1) {
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

TEST(LineFinderTest, LineFinder_TrumpSafety) {
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

TEST(LineFinderTest, LineFinder_OnsideAce) {
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

TEST(LineFinderTest, LineFinder_4CardEndplay) {
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

TEST(LineFinderTest, LineFinder_OrInvariant) {
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

TEST(LineFinderTest, LineFinder_AndInvariant) {
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

TEST(LineFinderTest, LineFinder_Simple9CardHearts) {
  RunLineFinderTestCase({
    .name = "simple_9_card_hearts",
    .trump = HEARTS,
    .leader = 0,
    .min_tricks = 4,
    .num_iters = 64,
    .hand0 = "S 5432; H A; C 3",
    .hand2 = "S K6; C AKJ2",
    .unknown_cards = "S AQJT9; C QT98765",
  });
}

TEST(LineFinderTest, LineFinder_Hand6_9Card) {
  RunLineFinderTestCase({
    .name = "hand_6_9_card",
    .trump = SPADES,
    .leader = 2,
    .min_tricks = 9,
    .num_iters = 200,
    .hand0 = "S AJ8765; C KJ8;",
    .hand2 = "S K9; D AQ853; C AQ;",
    .unknown_cards = "S Q432; H T7; D KJT97642; C T754;",
    .description =
        "Hand 6 9-card ending: branch on E discard on SK before club entry",
  });
}

TEST(LineFinderTest, LineFinder_MidTrickSolveAndPreassignedDefenderCard) {
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
    ASSERT_TRUE(!lines.empty());
    ASSERT_TRUE(lines[0].CoveredBoards() > 0);
    ASSERT_TRUE(lines[0].Describe().find("S8") != std::string::npos);
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
    ASSERT_TRUE(!lines.empty());
    ASSERT_TRUE(lines[0].CoveredBoards() > 0);
    ASSERT_TRUE(lines[0].Describe().find("S8") != std::string::npos);
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
    ASSERT_TRUE(!lines.empty());
    ASSERT_TRUE(lines[0].CoveredBoards() > 0);
    ASSERT_TRUE(lines[0].Describe().find("SQ") != std::string::npos);
  }
}

TEST(LineFinderTest, LineFinder_NineCardHeartsTrick2DK) {
  srand48(0);
  // 9-Card Hearts deal at Trick 2 after #1: D2 D7 D8 D9 (West won D9) and West
  // leads DK at Trick 2 (with East known to have followed to Diamonds on
  // Trick 1, or general unknown cards with DK assigned to West).
  // Verify that ducking DK does not artificially succeed on ~99% of boards
  // when East follows in Diamonds, and that continuing DJ at Trick 3 when East
  // is void in Diamonds yields a consistent success rate.
  std::vector<uint64_t> hands = {
      ParseHand("S 532; H T7; D T3; C 6;"),
      0,
      ParseHand("S K9; H -; D A54; C AKJ;"),
      ParseHand("D K"),
  };
  uint64_t unknown_cards = ParseHand("S AJTQ64 D QJ6 C T43Q98");
  std::vector<CardType> cur_trick = {Card::ParseStringCard("DK")};
  LineFinder lf(40);
  std::vector<ConditionalLine> lines =
      lf.Solve(6, hands, unknown_cards, /*leader=*/3, HEARTS, cur_trick);
  ASSERT_TRUE(!lines.empty());
  ASSERT_TRUE(lines[0].CoveredBoards() > 0);
  ASSERT_TRUE(lines[0].Describe().find("(DK)") != std::string::npos);
}

TEST(LineFinderTest, LineFinder_RestrictedChoice_NoDuplicateTrick1Splits) {
  srand48(0);
  // Restricted Choice spade combination (AT987 opposite K654, missing QJ32).
  // Previously, Solve returned 4 tied lines that all played SA and S6 on the
  // exact same set of boards at Trick 1, differing only in whether Trick 1 was
  // split on the defenders' spade cards (S_ vs Sx/SJ/SQ).
  std::vector<uint64_t> hands = {
      ParseHand("S AT987; H A;"),
      0,
      ParseHand("S K654; H QJ;"),
      0,
  };
  uint64_t unknown_cards = ParseHand("S QJ32 H KT987654");
  LineFinder lf(100);
  std::vector<ConditionalLine> lines =
      lf.Solve(6, hands, unknown_cards, /*leader=*/0, SPADES,
               {Card::ParseStringCard("SA")});
  ASSERT_TRUE(lines.size() == 1);
  ASSERT_TRUE(lines[0].conditions().size() == 1);

  // Reduced Mollo 6S ending (East leads DQ, West holds DA, North void in D):
  // Verify that Solve does not return near-duplicate lines that share the same
  // dominant Trick-1 declarer play (D2 + HA) while varying North's 4th-hand
  // play only on minor defender-card sub-branches like (DT) or (DJ).
  std::vector<uint64_t> mollo_hands = {
      ParseHand("S AKJ3; H AKQ; D -; C A;"),
      ParseHand("D Q;"),
      ParseHand("S Q2; H JT98; D K2; C -;"),
      ParseHand("D A;"),
  };
  uint64_t mollo_unknown = ParseHand("S 976 H 765 D JT987 C KQJ");
  LineFinder mollo_lf(60);
  std::vector<ConditionalLine> mollo_lines =
      mollo_lf.Solve(7, mollo_hands, mollo_unknown, /*leader=*/1, SPADES,
                     {Card::ParseStringCard("DQ")});
  ASSERT_TRUE(!mollo_lines.empty());
  for (const ConditionalLine &line : mollo_lines) {
    ASSERT_TRUE(line.conditions().size() == 1);
  }
}

TEST(LineFinderTest, LineFinder_ReuseCachedDealsForForcedLead) {
  srand48(0);
  std::vector<uint64_t> hands(4, 0);
  hands[0] = ParseHand("S 532; H T7; D T32; C 6;");
  hands[2] = ParseHand("S K9; H -; D A854; C AKJ;");
  uint64_t unknown = ParseHand("S AJTQ64 D KQJ976 C T43Q98");

  LineFinder lf(30);
  std::vector<ConditionalLine> base_lines =
      lf.Solve(6, hands, unknown, 0, HEARTS, {});
  ASSERT_TRUE(!base_lines.empty());
  int base_cov = base_lines[0].CoveredFinalBoards();
  ASSERT_TRUE(base_cov > 0);

  // Forced lead HT (distinct rank in suit) should reuse base_boards_ and
  // base_init_trees_ and match a fresh cold solve with the same seed.
  std::vector<ConditionalLine> warm_ht =
      lf.Solve(6, hands, unknown, 0, HEARTS, {Card::ParseStringCard("HT")});
  ASSERT_TRUE(!warm_ht.empty());

  srand48(0);
  LineFinder cold_lf_ht(30);
  std::vector<ConditionalLine> cold_ht =
      cold_lf_ht.Solve(6, hands, unknown, 0, HEARTS,
                       {Card::ParseStringCard("HT")});
  ASSERT_TRUE(!cold_ht.empty());
  ASSERT_TRUE(warm_ht[0].CoveredFinalBoards() == cold_ht[0].CoveredFinalBoards());

  // Forced lead S5 (higher of touching/near-touching spades) and S3 (lower in
  // same suit) should also match fresh cold solves on the same 30 deals.
  std::vector<ConditionalLine> warm_s5 =
      lf.Solve(6, hands, unknown, 0, HEARTS, {Card::ParseStringCard("S5")});
  ASSERT_TRUE(!warm_s5.empty());

  srand48(0);
  LineFinder cold_lf_s5(30);
  std::vector<ConditionalLine> cold_s5 =
      cold_lf_s5.Solve(6, hands, unknown, 0, HEARTS,
                       {Card::ParseStringCard("S5")});
  ASSERT_TRUE(!cold_s5.empty());
  ASSERT_TRUE(warm_s5[0].CoveredFinalBoards() == cold_s5[0].CoveredFinalBoards());
}

TEST(LineFinderTest, LineFinder_StopSolve) {
  srand48(0);
  std::vector<uint64_t> hands(4, 0);
  hands[0] = ParseHand("S 532; H T7; D T32; C 6;");
  hands[2] = ParseHand("S K9; H -; D A854; C AKJ;");
  uint64_t unknown = ParseHand("S AJTQ64 D KQJ976 C T43Q98");

  LineFinder::SetSolveStatus("Idle");
  std::thread stopper([]() {
    while (true) {
      std::string st = LineFinder::GetSolveStatus();
      if (st.find("Solving") != std::string::npos ||
          st.find("Searching") != std::string::npos) {
        LineFinder::RequestStop();
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  });

  LineFinder lf(100);
  std::vector<ConditionalLine> stopped_lines =
      lf.Solve(6, hands, unknown, 0, HEARTS, {}, /*num_threads=*/2);
  stopper.join();
  ASSERT_TRUE(LineFinder::IsStopRequested());
  ASSERT_TRUE(stopped_lines.empty());
  ASSERT_TRUE(LineFinder::GetSolveStatus() == "Solve stopped.");

  // A subsequent Solve() automatically clears the stop flag and succeeds.
  LineFinder lf2(15);
  std::vector<ConditionalLine> resumed_lines =
      lf2.Solve(6, hands, unknown, 0, HEARTS, {});
  ASSERT_TRUE(!LineFinder::IsStopRequested());
  ASSERT_TRUE(!resumed_lines.empty());
  ASSERT_TRUE(resumed_lines[0].CoveredFinalBoards() > 0);
}

TEST(LineFinderTest, OmpCompatPool_FourThreadsNestedTasks) {
  auto &pool = OmpCompatPool::Instance();
  std::atomic<int> sum{0};
  std::atomic<int> thread_mask{0};

  pool.RunRegion(4, [&]() {
    ASSERT_TRUE(OmpCompatPool::InParallel() == 1);
    ASSERT_TRUE(OmpCompatPool::MaxThreads() == 4);
    pool.RunTaskBatch(8, [&](size_t outer) {
      int tid_outer = OmpCompatPool::ThreadNum();
      ASSERT_TRUE(tid_outer >= 0);
      ASSERT_TRUE(tid_outer < 4);
      thread_mask.fetch_or(1 << tid_outer, std::memory_order_relaxed);
      pool.RunTaskBatch(4, [&](size_t inner) {
        int tid_inner = OmpCompatPool::ThreadNum();
        ASSERT_TRUE(tid_inner >= 0);
        ASSERT_TRUE(tid_inner < 4);
        thread_mask.fetch_or(1 << tid_inner, std::memory_order_relaxed);
        sum.fetch_add(static_cast<int>(outer * 10 + inner + 1),
                      std::memory_order_relaxed);
      });
    });
  });

  int expected = 0;
  for (size_t outer = 0; outer < 8; ++outer) {
    for (size_t inner = 0; inner < 4; ++inner) {
      expected += static_cast<int>(outer * 10 + inner + 1);
    }
  }
  ASSERT_TRUE(sum.load() == expected);
  ASSERT_TRUE(OmpCompatPool::InParallel() == 0);
  ASSERT_TRUE((thread_mask.load() & 1) != 0);
  ASSERT_TRUE((thread_mask.load() & ~0xF) == 0);
}

TEST(LineFinderTest, LineFinder_BranchOnPlayerDiscardedCondition) {
  srand48(0);
  // 4-card ending where South (2) leads C2 to North's CA on Trick 1:
  // East (1) sits in 4th seat on Trick 1, so Rule (1) prevents Trick 1 from
  // branching on East's 4th-seat card.
  // The 2 unknown cards are C4 and SK (1 goes to East, 1 goes to West):
  // - When East gets C4 and West gets SK, East follows suit on Trick 1
  //   ("if E did not discard on C"), revealing that West holds singleton SK,
  //   so on Trick 2 South must rise with SA to drop West's singleton SK.
  // - When East gets SK and West gets C4, East is void in Clubs and discards H4
  //   on Trick 1 ("if E discarded on C"), revealing that East holds S K54, so
  //   on Trick 2 South must finesse SQ/SJ through East.
  std::vector<uint64_t> hands = {
      ParseHand("C A; S 32; H 2;"),
      ParseHand("S 4; H 54;"),
      ParseHand("C 2; S AQJ;"),
      ParseHand("C 3; H AK;"),
  };
  uint64_t unknown_cards = ParseHand("C 4; S K;");
  LineFinder lf(30);
  std::vector<ConditionalLine> lines =
      lf.Solve(4, hands, unknown_cards, /*leader=*/2, NOTRUMP, {});
  ASSERT_FALSE(lines.empty());
  EXPECT_EQ(lines[0].CoveredFinalBoards(), 30);
  std::string desc = lines[0].Describe();
  std::cout << "BranchOnPlayerDiscardedCondition line:\n" << desc << "\n";
  // Trick 1 must not split on East's 4th-seat card, and Trick 2 must branch
  // on "if E discarded on C" / "if E did not discard on C".
  EXPECT_EQ(lines[0].conditions().size(), 1u);
  EXPECT_NE(desc.find("if E discarded on C"), std::string::npos);
  EXPECT_NE(desc.find("if E did not discard on C"), std::string::npos);

  // Verify that the condition node immediately follows the Club trick
  // (where C was led).
  const auto &t1_child = lines[0].conditions().begin()->second;
  ASSERT_NE(t1_child, nullptr);
  ASSERT_EQ(t1_child->conditions().size(), 1u);
  const auto &t1_winner_child = t1_child->conditions().begin()->second;
  ASSERT_NE(t1_winner_child, nullptr);
  EXPECT_TRUE(t1_winner_child->do_first().Empty());
  EXPECT_EQ(t1_winner_child->conditions().size(), 2u);
  for (const auto &kv : t1_winner_child->conditions()) {
    EXPECT_TRUE(kv.first->IsStateCondition());
    EXPECT_NE(kv.first->Describe(false).find("discard"), std::string::npos);
  }
}

TEST(LineFinderTest,
     LineFinder_AbandonsFalsecardedSplitPreservingDiscardCondition) {
  // 6-card ending in Spades (South leads SK at Trick 1, goal = 6 tricks):
  //   North (0): S AJ82; C K6;
  //   South (2): S KT9; C AQ2;
  // Predealt 14 boards:
  // - Boards 0..3 (4 boards): West has S Q765; C 54, East has D 543; C T87
  //   (East is void in Spades and discards on Trick 1 SK -> declarer must
  //   branch on "if E discarded on S" and finesse West's Spades).
  // - Boards 4..11 (8 boards): West has S 65; D 43; C T7, East has S Q7; D 5;
  //   C 854 (both follow to SK and CQ; East holds SQ so declarer must play SA
  //   on the second Spade after winning CK; West holds C T7 and can falsecard
  //   CT on CQ).
  // - Boards 12..13 (2 boards): West has S Q76; D 43; C T, East has S 5; D 5;
  //   C 8754 (both follow to SK; in Step 1 non-adversarial play, West is forced
  //   to drop singleton CT on CQ, tempting declarer to split on CQ (Cx) CK vs
  //   CQ (CT) C6 and finesse Spades when CT appears).
  // In Step 2 adversarial rescoring, West falsecards CT from C T7 on CQ on
  // Boards 4..11, collapsing the CT-splitting line from 14/14 to 6/14.
  // Declarer must abandon the CT split while preserving the
  // "if E discarded on S" split to achieve 12/14 tricks.
  std::vector<uint64_t> hands = {
      ParseHand("S AJ82; C K6;"),
      0,
      ParseHand("S KT9; C AQ2;"),
      0,
  };
  uint64_t unknown = ParseHand("S Q765; D 543; C T8754;");

  std::vector<BoardState> boards(14);
  auto set_board = [&](int idx, const std::string &east_str,
                       const std::string &west_str) {
    std::vector<uint64_t> h = {
        hands[0],
        ParseHand(east_str),
        hands[2],
        ParseHand(west_str),
    };
    boards[idx].SetHands(h);
    boards[idx].SetTrump(SPADES);
    boards[idx].SetLead(2);
    boards[idx].set_board_id(idx + 1);
  };
  for (int i = 0; i < 4; ++i) {
    set_board(i, "D 543; C T87;", "S Q765; C 54;");
  }
  for (int i = 4; i < 12; ++i) {
    set_board(i, "S Q7; D 5; C 854;", "S 65; D 43; C T7;");
  }
  for (int i = 12; i < 14; ++i) {
    set_board(i, "S 5; D 5; C 8754;", "S Q76; D 43; C T;");
  }

  LineFinder lf(14);
  lf.SetPredealtBoards(boards);
  lf.SetKeepAllTopK(true);
  std::vector<ConditionalLine> lines =
      lf.Solve(6, hands, unknown, /*leader=*/2, SPADES, {ParseHand("S K")});
  ASSERT_FALSE(lines.empty());
  std::string desc = lines[0].Describe();
  EXPECT_EQ(lines[0].CoveredFinalBoards(), 12) << desc;
  EXPECT_EQ(lines[0].ExtraBranches(), 1u) << desc;
  EXPECT_NE(desc.find("if E discarded on S"), std::string::npos) << desc;
  EXPECT_NE(desc.find("if E did not discard on S"), std::string::npos) << desc;
  EXPECT_EQ(desc.find("(Cx)"), std::string::npos) << desc;

  bool found_falsecarded_ct_split = false;
  for (const auto &line : lines) {
    if (line.step1_covered_boards() == 14) {
      found_falsecarded_ct_split = true;
      EXPECT_EQ(line.CoveredFinalBoards(), 6);
    }
  }
  EXPECT_TRUE(found_falsecarded_ct_split);
}

TEST(LineFinderTest, LineFinder_PrefersDrawingTrumpsWhenAllElseEqual) {
  // Case 1: Opponents hold trumps (Spades), and both drawing trumps first
  // (SK -> SA -> CA -> CK) and cashing clubs first (CA -> CK -> SK -> SA)
  // cover all 8 boards with 0 extra branches. The solver should prefer
  // drawing trumps first.
  {
    std::vector<uint64_t> hands = {
        ParseHand("S A2; C K2;"),
        0,
        ParseHand("S K3; C A3;"),
        0,
    };
    uint64_t unknown = ParseHand("S 7654; C QT98;");
    std::vector<BoardState> boards(8);
    for (int i = 0; i < 8; ++i) {
      std::vector<uint64_t> h = {
          hands[0],
          ParseHand((i % 2 == 0) ? "S 76; C 98;" : "S 75; C Q9;"),
          hands[2],
          ParseHand((i % 2 == 0) ? "S 54; C QT;" : "S 64; C T8;"),
      };
      boards[i].SetHands(h);
      boards[i].SetTrump(SPADES);
      boards[i].SetLead(2);
      boards[i].set_board_id(i + 1);
    }
    LineFinder lf(8);
    lf.SetPredealtBoards(boards);
    std::vector<ConditionalLine> lines =
        lf.Solve(4, hands, unknown, /*leader=*/2, SPADES);
    ASSERT_FALSE(lines.empty());
    EXPECT_EQ(lines[0].CoveredFinalBoards(), 8);
    ASSERT_EQ(lines[0].conditions().size(), 1u);
    std::string t1 = lines[0].conditions()[0].first->Describe(true);
    EXPECT_NE(t1.find("SK"), std::string::npos) << t1;
    const auto &win1 = lines[0].conditions()[0].second;
    ASSERT_NE(win1, nullptr);
    ASSERT_EQ(win1->conditions().size(), 1u);
    const auto &sub = win1->conditions()[0].second;
    ASSERT_NE(sub, nullptr);
    ASSERT_EQ(sub->conditions().size(), 1u);
    std::string t2 = sub->conditions()[0].first->Describe(true);
    EXPECT_NE(t2.find("SA"), std::string::npos) << t2;
  }

  // Case 2: Opponents are void in trumps (Spades). Cashing the trump SA
  // does not draw any opponent trumps (it is an uninformative null-move lead),
  // so the solver should lead the side suit (Clubs) to finesse CQ rather than
  // cashing SA first.
  {
    std::vector<uint64_t> hands = {
        ParseHand("S A; C AQ;"),
        0,
        ParseHand("S 2; C 32;"),
        0,
    };
    uint64_t unknown = ParseHand("D 432; C K98;");
    std::vector<BoardState> boards(4);
    for (int i = 0; i < 4; ++i) {
      std::vector<uint64_t> h = {
          hands[0],
          ParseHand("D 43; C 9;"),
          hands[2],
          ParseHand("D 2; C K8;"),
      };
      boards[i].SetHands(h);
      boards[i].SetTrump(SPADES);
      boards[i].SetLead(2);
      boards[i].set_board_id(i + 1);
    }
    LineFinder lf(4);
    lf.SetPredealtBoards(boards);
    std::vector<ConditionalLine> lines =
        lf.Solve(3, hands, unknown, /*leader=*/2, SPADES);
    ASSERT_FALSE(lines.empty());
    EXPECT_EQ(lines[0].CoveredFinalBoards(), 4);
    ASSERT_EQ(lines[0].conditions().size(), 1u);
    std::string t1 = lines[0].conditions()[0].first->Describe(true);
    EXPECT_NE(t1.find("C"), std::string::npos) << t1;
    EXPECT_EQ(t1.find("SA"), std::string::npos) << t1;
  }
}

TEST(LineFinderTest, LineFinder_BranchOnGoodCardNonTouchingEquivalentSpots) {
  // After South leads C5 to North's CA, North holds C64 (equivalent once C5 is
  // played, so DDS canonicalizes the cash to C4) plus D8 opposite South's DAQJ.
  // - Boards 0..9: Clubs 1-1 (CK, CQ both fall on trick 1) and DK offside in
  //   West -> must cash C6/C4 ("if C6 is good"), then C6/C4 and DA.
  // - Boards 10..19: Clubs 2-0 in East with DK7 onside -> C6 is not good, so
  //   fall back to leading D8 through East to finesse DJ ("if C6 is not good").
  std::vector<uint64_t> hands = {
      ParseHand("D 8; C A64;"),
      0,
      ParseHand("D AQJ; C 5;"),
      0,
  };
  uint64_t unknown = ParseHand("D KT9732; C KQ;");
  std::vector<BoardState> boards(20);
  for (int i = 0; i < 20; ++i) {
    bool clubs_good = (i < 10);
    std::vector<uint64_t> h = {
        hands[0],
        ParseHand(clubs_good ? "D T72; C K;" : "D K7; C KQ;"),
        hands[2],
        ParseHand(clubs_good ? "D K93; C Q;" : "D T932;"),
    };
    boards[i].SetHands(h);
    boards[i].SetTrump(SPADES);
    boards[i].SetLead(2);
    boards[i].set_board_id(i + 1);
  }
  LineFinder lf(20);
  lf.SetPredealtBoards(boards);
  std::vector<ConditionalLine> lines =
      lf.Solve(4, hands, unknown, /*leader=*/2, SPADES);
  ASSERT_FALSE(lines.empty());
  std::string desc = lines[0].Describe(0);
  EXPECT_EQ(lines[0].CoveredFinalBoards(), 20) << desc;
  EXPECT_NE(desc.find("if C6 is good"), std::string::npos) << desc;
  EXPECT_NE(desc.find("if C6 is not good"), std::string::npos) << desc;
}

TEST(LineFinderTest, LineFinder_MissingDefenderShiftDoesNotConfuseStep2) {
  // 3-card ending in Clubs trump, North (0) on lead, min_tricks = 2:
  //   N: H 2; D 93   |   S: D K4; C Q
  // - Board 0: Seat 1 holds singleton DA + H87 (Seat 3 holds DQJ8). In Step 1,
  //   only Board 0 enters `D3 (DA) D4 (D_) -> when 1 wins the DA`, where Seat 1
  //   has only Hearts left (`after (H_) CQ (__) H2`).
  // - Boards 1..4: Seat 1 holds DAJ8 or DAQ8 (no Hearts). In Step 1, Seat 1
  //   plays 2nd-hand low (`D3 (D<A) DK (D_)`). In Step 2, Seat 1 tests rising
  //   with DA and returning a diamond into `(H_) CQ (__) H2`, where declarer
  //   still makes 2 tricks double-dummy (`DK` + `CQ`). Step 2 classifies the
  //   missing diamond-return continuation as Tier 2 rather than a defeat and
  //   keeps Boards 1..4 on `D3 (D<A) DK (D_)`, preserving 5/5 coverage.
  std::vector<uint64_t> hands = {
      ParseHand("H 2; D 93;"),
      0,
      ParseHand("D K4; C Q;"),
      0,
  };
  uint64_t unknown = ParseHand("H 87; D AQJ8;");
  std::vector<BoardState> boards(5);
  auto set_board = [&](int idx, const char *h1, const char *h3) {
    std::vector<uint64_t> h = {hands[0], ParseHand(h1), hands[2],
                               ParseHand(h3)};
    boards[idx].SetHands(h);
    boards[idx].SetTrump(CLUBS);
    boards[idx].SetLead(0);
    boards[idx].set_board_id(idx + 1);
  };
  set_board(0, "H 87; D A;", "D QJ8;");
  set_board(1, "D AJ8;", "H 87; D Q;");
  set_board(2, "D AJ8;", "H 87; D Q;");
  set_board(3, "D AQ8;", "H 87; D J;");
  set_board(4, "D AQ8;", "H 87; D J;");

  LineFinder lf(5);
  lf.SetPredealtBoards(boards);
  std::vector<ConditionalLine> lines =
      lf.Solve(2, hands, unknown, /*leader=*/0, CLUBS);
  ASSERT_FALSE(lines.empty());
  std::string desc = lines[0].Describe(0);
  EXPECT_EQ(lines[0].CoveredFinalBoards(), 5) << desc;
}

TEST(LineFinderTest, LineFinder_MissingWinnerAfterOvertakeDoesNotConfuseStep2) {
  // 4-card endplay in Notrump, North (0) on lead, min_tricks = 2:
  //   N: S 6; D AQ2   |   S: S 5; D 543
  // - Board 0 (makeable): East holds `S K; D T97`, West holds `S 84; D KJ`
  //   (East always wins Trick 1 with `SK`).
  // - Boards 1..3 (makeable): East holds `S 8; D T97`, West holds `S K4; D KJ`.
  //   In Step 1, East's `S8` is already winning so West ducks `S4`, meaning
  //   Step 1 only ever populates `when 1 wins` on `S6 (S_) S5 (S_)`.
  // - Board 4 (unmakeable): East holds `S 4; D KJ9`, West holds `S K8; D T7`
  //   (max 1 trick for N/S). `TraceUnmakeableBoard` plays `S6 S4 S5 SK`
  //   (`winner == 3`), setting `winner_unmakeable_boards[3] = 1`.
  // - In Step 2 on Boards 1..3, West evaluates overtaking `S8` with `SK` at
  //   `pos == 3` (`winner == 3`, empirical probability 0/1 vs 4/4 for `1
  //   wins`). Because N/S still makes 2 tricks (`DA` + `DQ`) after `SK`, Step 2
  //   ranks the missing `3 wins` continuation as Tier 2 (worse than Tier 1
  //   `S4`) and preserves 4/4 coverage without synthesizing `if 3 wins`.
  std::vector<uint64_t> hands = {
      ParseHand("S 6; D AQ2;"),
      0,
      ParseHand("S 5; D 543;"),
      0,
  };
  uint64_t unknown = ParseHand("S K84; D KJT97;");
  std::vector<BoardState> boards(5);
  auto set_board = [&](int idx, const char *h1, const char *h3) {
    std::vector<uint64_t> h = {hands[0], ParseHand(h1), hands[2],
                               ParseHand(h3)};
    boards[idx].SetHands(h);
    boards[idx].SetTrump(NOTRUMP);
    boards[idx].SetLead(0);
    boards[idx].set_board_id(idx + 1);
  };
  set_board(0, "S K; D T97;", "S 84; D KJ;");
  for (int i = 1; i < 4; ++i) {
    set_board(i, "S 8; D T97;", "S K4; D KJ;");
  }
  set_board(4, "S 4; D KJ9;", "S K8; D T7;");

  LineFinder lf(5);
  lf.SetPredealtBoards(boards);
  std::vector<ConditionalLine> lines =
      lf.Solve(2, hands, unknown, /*leader=*/0, NOTRUMP);
  ASSERT_FALSE(lines.empty());
  std::string desc = lines[0].Describe(0);
  EXPECT_EQ(lines[0].CoveredFinalBoards(), 4) << desc;
  EXPECT_EQ(desc.find("if 3 wins the "), std::string::npos) << desc;
}
