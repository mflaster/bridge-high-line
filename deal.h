#pragma once

#include "board_state.h"
#include "hand.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "hand_requirements.h"

BoardState LoadDealFromKeyValueFile(const std::string &filename);
BoardState LoadDealFromKeyValueString(const std::string &content);

std::vector<uint64_t> DealHands();

std::pair<uint64_t, uint64_t> DealTwoHands(uint64_t cards,
                                           int hand1_count = -1);

std::pair<uint64_t, uint64_t> DealTwoRequiredHands(
    uint64_t cards,
    const std::vector<std::shared_ptr<HandRequirements>> &reqs1,
    const std::vector<std::shared_ptr<HandRequirements>> &reqs2,
    int hand1_count = -1);

std::vector<uint64_t> DealMissingCards(
    const std::vector<uint64_t> &hands,
    const std::vector<std::vector<std::shared_ptr<HandRequirements>>> &reqs);

uint64_t DealOneRequiredHand(
    uint64_t cards,
    const std::vector<std::shared_ptr<HandRequirements>> &reqs);
