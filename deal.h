#pragma once

#include "board_state.h"
#include "hand.h"
#include "hand_requirements.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

BoardState LoadDealFromKeyValueFile(const std::string &filename);
BoardState LoadDealFromKeyValueString(const std::string &content);

struct DealFixture {
  BoardState board_state;
  int target_tricks = 0;
  Suit trump = NOTRUMP;
  PlayerDirection leader = NORTH;
  std::vector<uint64_t> hands;

  operator BoardState() const { return board_state; }
};

DealFixture LoadDealFromJSON(const std::string &json_str, const std::string &source_name = "JSON input");
DealFixture LoadDealFromJSONFile(const std::string &file_path);

std::vector<uint64_t> DealHands();

std::pair<uint64_t, uint64_t> DealTwoHands(uint64_t cards);

std::pair<uint64_t, uint64_t> DealTwoRequiredHands(
    uint64_t cards,
    const std::vector<std::shared_ptr<HandRequirements>> &reqs1,
    const std::vector<std::shared_ptr<HandRequirements>> &reqs2);

std::vector<uint64_t> DealMissingCards(
    const std::vector<uint64_t> &hands,
    const std::vector<std::vector<std::shared_ptr<HandRequirements>>> &reqs);

uint64_t DealOneRequiredHand(
    uint64_t cards,
    const std::vector<std::shared_ptr<HandRequirements>> &reqs);
