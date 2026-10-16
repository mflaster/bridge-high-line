#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "card.h"
#include "conditional_line.h"
#include "deal.h"
#include "hand.h"
#include "interface_dds.h"
#include "line_finder.h"
#include "glog/logging.h"

#include <functional>

namespace {

std::string SummarizeLineT1T2(const ConditionalLine& line) {
  std::ostringstream os;
  bool first_t1 = true;
  for (const auto& [t1_cond, t1_child] : line.conditions()) {
    if (!t1_child) continue;
    if (!first_t1) os << " | ";
    first_t1 = false;
    const auto& gt1 = t1_child->do_first();
    os << gt1.demi1.my_card << "(" << gt1.demi1.opp_card << ")->"
       << gt1.demi2.my_card;
    if (!t1_child->conditions().empty()) {
      os << " [";
      bool first_w = true;
      for (const auto& [w_cond, t2_grp] : t1_child->conditions()) {
        if (!t2_grp) continue;
        if (!first_w) os << "; ";
        first_w = false;
        os << w_cond->Describe(false) << ": ";
        bool first_t2 = true;
        for (const auto& [t2_cond, t2_child] : t2_grp->conditions()) {
          if (!t2_child) continue;
          if (!first_t2) os << ", ";
          first_t2 = false;
          const auto& gt2 = t2_child->do_first();
          if (gt2.leader % 2 == 1) {
            os << gt2.demi1.opp_card << ":" << gt2.demi1.my_card << "->"
               << gt2.demi2.my_card;
          } else {
            os << gt2.demi1.my_card << "->" << gt2.demi2.my_card;
          }
        }
      }
      os << "]";
    }
  }
  return os.str();
}

void GetHandSuitDistribution(uint64_t hand, int dist[4]) {
  for (int suit = 0; suit < 4; ++suit) {
    dist[suit] = 0;
    uint16_t cur_suit = (hand >> (suit * 16)) & 0xffff;
    for (int val = 0; val < 13; ++val) {
      if ((cur_suit >> val) & 1) {
        dist[suit]++;
      }
    }
  }
}

// Samples 11 random defender layouts and returns the median double-dummy
// tricks won by declarer's side, or -1 if all 11 layouts win the exact same
// number of tricks (zero variance).
int EstimateMedianMinTricks(const std::vector<uint64_t>& hands,
                            uint64_t unknown_cards, int leader, Suit trump,
                            long seed) {
  srand48(seed);
  LineFinder lf_sample(11);
  std::multiset<int> sample_tricks = lf_sample.FindTricksWon(
      hands, unknown_cards, leader, trump, /*deal_missing_cards=*/false);
  if (sample_tricks.empty() ||
      *sample_tricks.begin() == *sample_tricks.rbegin()) {
    return -1;
  }
  auto med_it = sample_tricks.begin();
  std::advance(med_it, sample_tricks.size() / 2);
  return std::max(1, *med_it);
}

void RunPerf9CardHeartsCase(
    const std::string& name, int num_deals,
    const std::function<void(const std::string&)>& log_summary,
    const std::function<void(const std::string&)>& log_details) {
  srand48(0);
  std::vector<uint64_t> hands(4, 0);
  hands[0] = ParseHand("S 532; H T7; D T32; C 6;");
  hands[2] = ParseHand("S K9; H -; D A854; C AKJ;");
  uint64_t unknown = ParseHand("S AJTQ64 D KQJ976 C T43Q98");

  LineFinder lf(num_deals);
  auto start = std::chrono::high_resolution_clock::now();
  std::vector<ConditionalLine> lines =
      lf.Solve(6, hands, unknown, 0, HEARTS, {});
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;

  bool succeeded = !lines.empty();
  std::string status = succeeded ? "SUCCESS" : "FAILED";
  int covered = lines.empty() ? 0 : lines[0].CoveredFinalBoards();
  double pct = (100.0 * covered) / num_deals;

  std::stringstream ss;
  ss << name << " num_deals=" << std::setw(3) << num_deals
     << " time=" << std::fixed << std::setprecision(1) << elapsed.count() << "s"
     << " (solve=" << std::fixed << std::setprecision(1) << lf.solve_ms()
     << "ms, invariant=" << std::fixed << std::setprecision(1)
     << lf.invariant_ms() << "ms)"
     << " result=" << status << " (covered=" << covered << "/" << num_deals
     << ", " << std::fixed << std::setprecision(1) << pct
     << "%, lines=" << lines.size() << ")";

  log_details("\n=== " + name + " ===");
  log_details(ss.str());
  log_summary(ss.str());
}

}  // namespace

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);

  bool append_9_card_hearts_only = false;
  int custom_9h_deals = 0;
  int custom_9h_threads = 16;
  bool custom_9h_warm = false;
  int custom_9h_noise = 10;
  int custom_9h_top_k = 20;
  int custom_9h_top_k_lvl1 = 4;
  bool custom_9h_rescore = true;
  int custom_9h_print_lines = 0;
  std::string custom_9h_lead;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--append-9-card-hearts") {
      append_9_card_hearts_only = true;
    } else if (arg.rfind("--9h-deals=", 0) == 0) {
      custom_9h_deals = std::stoi(arg.substr(11));
    } else if (arg.rfind("--9h-threads=", 0) == 0) {
      custom_9h_threads = std::stoi(arg.substr(13));
    } else if (arg.rfind("--9h-lead=", 0) == 0) {
      custom_9h_lead = arg.substr(10);
    } else if (arg == "--9h-warm") {
      custom_9h_warm = true;
    } else if (arg.rfind("--9h-noise=", 0) == 0) {
      custom_9h_noise = std::stoi(arg.substr(11));
    } else if (arg.rfind("--9h-top-k=", 0) == 0) {
      custom_9h_top_k = std::stoi(arg.substr(11));
    } else if (arg.rfind("--9h-top-k-lvl1=", 0) == 0) {
      custom_9h_top_k_lvl1 = std::stoi(arg.substr(16));
    } else if (arg == "--9h-rescore") {
      custom_9h_rescore = true;
    } else if (arg == "--9h-no-rescore") {
      custom_9h_rescore = false;
    } else if (arg.rfind("--9h-print-lines=", 0) == 0) {
      custom_9h_print_lines = std::stoi(arg.substr(17));
    }
  }

  if (custom_9h_deals > 0) {
    SetDefenderNoisePct(custom_9h_noise);
    srand48(0);
    std::vector<uint64_t> hands(4, 0);
    hands[0] = ParseHand("S 532; H T7; D T32; C 6;");
    hands[2] = ParseHand("S K9; H -; D A854; C AKJ;");
    uint64_t unknown = ParseHand("S AJTQ64 D KQJ976 C T43Q98");
    LineFinder lf(custom_9h_deals);
    lf.SetTopKLines(custom_9h_top_k);
    lf.SetTopKLvl1(custom_9h_top_k_lvl1);
    lf.SetAdversarialRescore(custom_9h_rescore);
    lf.SetKeepAllTopK(custom_9h_top_k > 1);
    auto run_solve = [&](const std::string& lead_str) {
      if (!custom_9h_warm) {
        srand48(0);
      }
      std::vector<CardType> cur_trick;
      if (!lead_str.empty()) {
        for (const std::string& tok : SplitWords(lead_str)) {
          cur_trick.push_back(Card::ParseStringCard(tok));
        }
      }
      auto start = std::chrono::high_resolution_clock::now();
      std::vector<ConditionalLine> lines =
          lf.Solve(6, hands, unknown, 0, HEARTS, cur_trick, custom_9h_threads);
      auto end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> elapsed = end - start;
      int covered = lines.empty() ? 0 : lines[0].CoveredFinalBoards();
      std::cout << "9H bench deals=" << custom_9h_deals
                << " threads=" << custom_9h_threads
                << " noise=" << custom_9h_noise << "%"
                << " top_k=" << custom_9h_top_k
                << " top_k_lvl1=" << custom_9h_top_k_lvl1
                << " rescore=" << (custom_9h_rescore ? 1 : 0)
                << " warm=" << (custom_9h_warm ? 1 : 0)
                << " lead=\"" << lead_str << "\""
                << " time=" << std::fixed << std::setprecision(3)
                << elapsed.count() << "s"
                << " (solve=" << lf.solve_ms() << "ms, rescore="
                << lf.rescore_ms() << "ms, invariant="
                << lf.invariant_ms() << "ms)"
                << " covered=" << covered << "/" << custom_9h_deals
                << " lines=" << lines.size() << std::endl;
      if (custom_9h_top_k > 1 || custom_9h_rescore) {
        std::cout << "Rank  Step1  Step2  Delta  Trick1 & Trick2 summary\n";
        for (size_t idx = 0; idx < lines.size(); ++idx) {
          int s1 = lines[idx].step1_covered_boards();
          int s2 = lines[idx].CoveredFinalBoards();
          int delta = s2 - s1;
          std::cout << std::setw(4) << (idx + 1) << "  "
                    << std::setw(5) << s1 << "  "
                    << std::setw(5) << s2 << "  "
                    << std::setw(5) << std::showpos << delta << std::noshowpos
                    << "  " << SummarizeLineT1T2(lines[idx]) << "\n";
        }
      }
      for (int idx = 0;
           idx < custom_9h_print_lines && idx < static_cast<int>(lines.size());
           ++idx) {
        std::cout << "\n--- Line #" << (idx + 1)
                  << " (Step1=" << lines[idx].step1_covered_boards()
                  << ", Step2=" << lines[idx].CoveredFinalBoards() << ") ---\n"
                  << lines[idx].Describe() << std::endl;
      }
    };
    if (custom_9h_warm) {
      run_solve("");
    }
    if (custom_9h_lead.empty()) {
      if (!custom_9h_warm) run_solve("");
    } else {
      std::stringstream ss(custom_9h_lead);
      std::string item;
      while (std::getline(ss, item, ',')) {
        run_solve(item);
      }
    }
    return 0;
  }

  std::ios_base::openmode mode =
      append_9_card_hearts_only ? std::ios::app : std::ios::out;
  std::ofstream details_log("perf/details.log", mode);
  std::ofstream summary_log("perf/summary.log", mode);

  auto log_details = [&](const std::string& msg) {
    std::cout << msg << std::endl;
    details_log << msg << std::endl;
  };

  auto log_summary = [&](const std::string& msg) {
    std::cout << msg << std::endl;
    summary_log << msg << std::endl;
  };

  if (!append_9_card_hearts_only) {
    std::mt19937 rng(12345);

    std::vector<CardType> full_deck;
    for (int suit = 0; suit < 4; suit++) {
      for (int val = 0; val < 13; val++) {
        full_deck.push_back(Card::GetSuitedCard(static_cast<Suit>(suit), val));
      }
    }

    log_details("Starting LineFinder Performance Benchmark");
    const int num_deals = 100;
    log_details(
        "Range: ending_size 6..13, 3 trials per ending_size, num_deals=" +
        std::to_string(num_deals));

    log_summary("=== LineFinder Performance Benchmark Summary ===");

    auto bench_start = std::chrono::high_resolution_clock::now();
    LineFinder lf(num_deals);

    for (int ending_size = 6; ending_size <= 13; ending_size++) {
      for (int trial = 1; trial <= 3; trial++) {
        int leader = 1;  // Leader is hand 1 (EAST)
        int min_fit = ending_size * 2 / 3;
        int min_tricks = 1;

        std::vector<uint64_t> hands(4, 0);
        uint64_t unknown_cards = 0;
        int dist0[4] = {0};
        int dist2[4] = {0};
        Suit trump = SPADES;
        int max_combined = -1;

        while (true) {
          std::vector<CardType> deck = full_deck;
          std::shuffle(deck.begin(), deck.end(), rng);

          hands.assign(4, 0);
          unknown_cards = 0;

          for (int i = 0; i < ending_size; i++) {
            hands[0] |= deck[i];
          }
          for (int i = 0; i < ending_size; i++) {
            hands[2] |= deck[ending_size + i];
          }
          for (int i = 0; i < 2 * ending_size; i++) {
            unknown_cards |= deck[2 * ending_size + i];
          }

          GetHandSuitDistribution(hands[0], dist0);
          GetHandSuitDistribution(hands[2], dist2);

          trump = SPADES;
          max_combined = -1;
          for (int s = 0; s < 4; s++) {
            int combined = dist0[s] + dist2[s];
            if (combined > max_combined) {
              max_combined = combined;
              trump = static_cast<Suit>(s);
            }
          }
          if (max_combined < min_fit) {
            continue;
          }

          long seed = 12345 + ending_size * 10 + trial;
          min_tricks = EstimateMedianMinTricks(hands, unknown_cards, leader,
                                               trump, seed);
          if (min_tricks > 0) {
            break;
          }
        }

        std::string suit_len_str = "(" + std::to_string(dist0[trump]) + "-" +
                                   std::to_string(dist2[trump]) + ")";

        bool should_log_details = (ending_size < 8 && trial == 1);
        if (should_log_details) {
          log_details("\n=== ending_size = " + std::to_string(ending_size) +
                      ", trial = " + std::to_string(trial) + " ===");
          log_details("hand[0] (Declarer): " + HandDebugString(hands[0]));
          log_details("hand[2] (Partner):  " + HandDebugString(hands[2]));
          log_details("unknown_cards:      " + HandDebugString(unknown_cards));
          log_details("Trump: " + SuitDebugString(trump) + " " + suit_len_str +
                      ", Leader: " + std::to_string(leader) +
                      ", Min Tricks: " + std::to_string(min_tricks));
        }

        srand48(12345 + ending_size * 10 + trial);

        auto start = std::chrono::high_resolution_clock::now();
        std::vector<ConditionalLine> lines =
            lf.Solve(min_tricks, hands, unknown_cards, leader, trump);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;

        bool succeeded = !lines.empty();
        std::string status = succeeded ? "SUCCESS" : "FAILED";

        if (should_log_details) {
          log_details("Found " + std::to_string(lines.size()) +
                      " lines for ending_size = " +
                      std::to_string(ending_size) + " trial " +
                      std::to_string(trial));
          for (size_t i = 0; i < lines.size(); i++) {
            log_details("Line " + std::to_string(i + 1) + ": " +
                        lines[i].Describe());
          }
          log_details("Solve took " + std::to_string(elapsed.count()) +
                      " seconds. Status: " + status);
        }

        int covered = lines.empty() ? 0 : lines[0].CoveredFinalBoards();
        double pct = (100.0 * covered) / num_deals;
        std::stringstream ss;
        ss << "ending_size=" << std::setw(2) << ending_size
           << " trial=" << trial << " time=" << std::fixed
           << std::setprecision(1) << elapsed.count() << "s"
           << " trump=" << std::setw(2) << SuitDebugString(trump)
           << " fit=" << std::setw(6) << suit_len_str
           << " min_tricks=" << std::setw(2) << min_tricks
           << " result=" << status << " (covered=" << covered << "/"
           << num_deals << ", " << std::fixed << std::setprecision(1) << pct
           << "%, lines=" << lines.size() << ")";

        log_summary(ss.str());
      }
    }

    auto bench_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> total_elapsed = bench_end - bench_start;
    std::stringstream total_ss;
    total_ss << "Benchmark complete. Total wall time: " << std::fixed
             << std::setprecision(1) << total_elapsed.count() << "s";

    log_details("\n" + total_ss.str());
    log_summary(total_ss.str());
  }

  RunPerf9CardHeartsCase("Perf9CardHearts100", 100, log_summary, log_details);
  RunPerf9CardHeartsCase("Perf9CardHearts150", 150, log_summary, log_details);

  return 0;
}
