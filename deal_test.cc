#include <iostream>
#include <gtest/gtest.h>

#include "deal.h"

TEST(DealTest, DealTwo) {
  srand48(1);
  uint64_t cards = ParseHand("H5432 D6 C52");
  ASSERT_TRUE(CardsInHand(cards, 0).size() == 7);
  auto [hand1, hand2] = DealTwoHands(cards);
  ASSERT_TRUE(CardsInHand(hand1, 0).size() == 3);
  ASSERT_TRUE(CardsInHand(hand2, 0).size() == 4);
  ASSERT_TRUE(HandDebugString(hand1) == "S -; H 2; D -; C 52; ");
  ASSERT_TRUE(HandDebugString(hand2) == "S -; H 543; D 6; C -; ");
}

TEST(DealTest, DealTwoWithReqs) {
  srand48(1);

  uint64_t cards = ParseHand("H5432 D6 C52");
  ASSERT_TRUE(CardsInHand(cards, 0).size() == 7);

  {
    std::vector<std::shared_ptr<HandRequirements>> reqs1, reqs2;
    auto [hand1, hand2] = DealTwoRequiredHands(cards, reqs1, reqs2);
    ASSERT_TRUE(CardsInHand(hand1, 0).size() == 3);
    ASSERT_TRUE(CardsInHand(hand2, 0).size() == 4);
    ASSERT_TRUE(HandDebugString(hand1) == "S -; H 2; D -; C 52; ");
    ASSERT_TRUE(HandDebugString(hand2) == "S -; H 543; D 6; C -; ");
  }
  {
    std::vector<std::shared_ptr<HandRequirements>> reqs1, reqs2;
    reqs2.emplace_back(new SuitAtLeast(HEARTS, 4));
    auto [hand1, hand2] = DealTwoRequiredHands(cards, reqs1, reqs2);
    ASSERT_TRUE(CardsInHand(hand1, 0).size() == 3);
    ASSERT_TRUE(CardsInHand(hand2, 0).size() == 4);
    ASSERT_TRUE(HandDebugString(hand1) == "S -; H -; D 6; C 52; ");
    ASSERT_TRUE(HandDebugString(hand2) == "S -; H 5432; D -; C -; ");
  }
}

TEST(DealTest, DealMissing) {
  std::vector<std::vector<std::shared_ptr<HandRequirements>>> reqs;
  uint64_t hand = ParseHand("S97; HQJ3; DQ972; CJ876");
  std::vector<uint64_t> hands = DealMissingCards(
      std::vector<uint64_t>({0, hand, 0, 0}), reqs);
  for (uint64_t h : hands) {
    std::cout << "Dealt hand: " << HandDebugString(h) << "\n";
  }
}

TEST(DealTest, LoadDealFromKeyValueFileSample) {
  BoardState bs = LoadDealFromKeyValueFile("testdata/sample.deal");

  ASSERT_TRUE(bs.GetHand(NORTH) == ParseHand("S AKQ2 H JT9 D 876 C 543"));
  ASSERT_TRUE(bs.GetHand(EAST)  == ParseHand("S JT9 H 8762 D 543 C AKQ"));
  ASSERT_TRUE(bs.GetHand(SOUTH) == ParseHand("S 876 H AKQ D AKQ2 C 876"));
  ASSERT_TRUE(bs.GetHand(WEST)  == ParseHand("S 543 H 543 D JT9 C JT92"));

  ASSERT_TRUE(bs.GetTrump() == HEARTS);
  ASSERT_TRUE(bs.cur_trick_leader() == NORTH);
}

TEST(DealTest, LoadDealFromKeyValueStringCommentsAndFormatting) {
  std::string deal_str = R"(
# Header comment line
// Another comment line
north = S AKQ2 H J109 D 876 C 543 # inline comment
east  = S JT9 H 8762 D 543 C AKQ

south = S 876 H AKQ D AKQ2 C 876
west  = S 543 H 543 D J109 C JT92
trump = SPADES
leader = EAST
)";

  BoardState bs = LoadDealFromKeyValueString(deal_str);
  ASSERT_TRUE(bs.GetHand(NORTH) == ParseHand("S AKQ2 H JT9 D 876 C 543"));
  ASSERT_TRUE(bs.GetHand(EAST)  == ParseHand("S JT9 H 8762 D 543 C AKQ"));
  ASSERT_TRUE(bs.GetHand(SOUTH) == ParseHand("S 876 H AKQ D AKQ2 C 876"));
  ASSERT_TRUE(bs.GetHand(WEST)  == ParseHand("S 543 H 543 D JT9 C JT92"));

  ASSERT_TRUE(bs.GetTrump() == SPADES);
  ASSERT_TRUE(bs.cur_trick_leader() == EAST);
}

TEST(DealTest, LoadDealFromKeyValueFilePerformance) {
  // Warm up filesystem cache so cold disk I/O jitter on CI doesn't flake.
  (void)LoadDealFromKeyValueFile("testdata/sample.deal");
  auto start = std::chrono::high_resolution_clock::now();
  BoardState bs = LoadDealFromKeyValueFile("testdata/sample.deal");
  auto end = std::chrono::high_resolution_clock::now();

  double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
  std::cout << "LoadDealFromKeyValueFile execution time: " << elapsed_ms << " ms" << "\n";
  ASSERT_TRUE(elapsed_ms < 1.0);
  ASSERT_TRUE(bs.GetHand(NORTH) != 0);
}

TEST(DealTest, LoadDealFromKeyValueFileErrorHandling) {
  {  // Non-existent file
    EXPECT_THROW(LoadDealFromKeyValueFile("non_existent_file.deal"), std::runtime_error);
  }

  {  // Invalid key
    EXPECT_THROW(LoadDealFromKeyValueFile("testdata/invalid_key.deal"), std::runtime_error);
  }

  {  // Missing required hand
    EXPECT_THROW(LoadDealFromKeyValueFile("testdata/missing_hand.deal"), std::runtime_error);
  }

  {  // Missing equals delimiter
    std::string bad_str = "north S AKQ\neast S JT9\nsouth S 876\nwest S 5432\n";
    EXPECT_THROW(LoadDealFromKeyValueString(bad_str), std::runtime_error);
  }

  {  // Duplicate card across hands
    std::string dup_str = R"(
north = S AKQ H JT9 D 876 C 543
east  = S AKQ H 876 D 543 C AKQ
south = S 876 H AKQ D AKQ C 876
west  = S 5432 H 542 D JT9 C JT9
)";
    EXPECT_THROW(LoadDealFromKeyValueString(dup_str), std::runtime_error);
  }
}

TEST(DealTest, LoadDealFromKeyValueStringWhitespaceAndOrderings) {
  {  // Extra spaces and arbitrary direction order
    std::string deal_str = R"(
north =     S AKQ2 H J109 D 876 C 543
west = S 543 H 543 D J109 C JT92
east =                         S JT9 H 8762 D 543 C AKQ
south =     S 876 H AKQ D AKQ2 C 876
trump = NT
leader = North
)";
    BoardState bs = LoadDealFromKeyValueString(deal_str);
    ASSERT_TRUE(bs.GetHand(NORTH) == ParseHand("S AKQ2 H JT9 D 876 C 543"));
    ASSERT_TRUE(bs.GetHand(EAST)  == ParseHand("S JT9 H 8762 D 543 C AKQ"));
    ASSERT_TRUE(bs.GetHand(SOUTH) == ParseHand("S 876 H AKQ D AKQ2 C 876"));
    ASSERT_TRUE(bs.GetHand(WEST)  == ParseHand("S 543 H 543 D JT9 C JT92"));
    ASSERT_TRUE(bs.GetTrump() == NOTRUMP);
    ASSERT_TRUE(bs.cur_trick_leader() == NORTH);
  }

  {  // Reordered suits within hand string and lower case characters
    std::string deal_str = R"(
west  = c jt92 d j109 h 543 s 543
south = H akq D akq2 S 876 C 876
north = s akq2 h j109 d 876 c 543
east  = S jt9 H 8762 D 543 C akq
)";
    BoardState bs = LoadDealFromKeyValueString(deal_str);
    ASSERT_TRUE(bs.GetHand(NORTH) == ParseHand("S AKQ2 H JT9 D 876 C 543"));
    ASSERT_TRUE(bs.GetHand(EAST)  == ParseHand("S JT9 H 8762 D 543 C AKQ"));
    ASSERT_TRUE(bs.GetHand(SOUTH) == ParseHand("S 876 H AKQ D AKQ2 C 876"));
    ASSERT_TRUE(bs.GetHand(WEST)  == ParseHand("S 543 H 543 D JT9 C JT92"));
  }

  {  // Tabs and carriage returns
    std::string deal_str = "north\t=\t\tS AKQ2 H J109 D 876 C 543\r\neast = S JT9 H 8762 D 543 C AKQ\r\nsouth = S 876 H AKQ D AKQ2 C 876\r\nwest = S 543 H 543 D J109 C JT92\r\n";
    BoardState bs = LoadDealFromKeyValueString(deal_str);
    ASSERT_TRUE(bs.GetHand(NORTH) == ParseHand("S AKQ2 H JT9 D 876 C 543"));
    ASSERT_TRUE(bs.GetHand(EAST)  == ParseHand("S JT9 H 8762 D 543 C AKQ"));
    ASSERT_TRUE(bs.GetHand(SOUTH) == ParseHand("S 876 H AKQ D AKQ2 C 876"));
    ASSERT_TRUE(bs.GetHand(WEST)  == ParseHand("S 543 H 543 D JT9 C JT92"));
  }

  {  // Leader formats and invalid direction handling
    std::string deal_str = R"(
n = S AKQ2 H J109 D 876 C 543
e = S JT9 H 8762 D 543 C AKQ
s = S 876 H AKQ D AKQ2 C 876
w = S 543 H 543 D JT9 C JT92
trump = NT
leader = 1
)";
    BoardState bs = LoadDealFromKeyValueString(deal_str);
    ASSERT_TRUE(bs.cur_trick_leader() == EAST);

    std::string bad_leader_str = R"(
north = S AKQ2 H J109 D 876 C 543
east  = S JT9 H 8762 D 543 C AKQ
south = S 876 H AKQ D AKQ2 C 876
west  = S 543 H 543 D JT9 C JT92
leader = INVALID_DIRECTION
)";
    EXPECT_THROW(LoadDealFromKeyValueString(bad_leader_str),
                      std::runtime_error);
  }
}


