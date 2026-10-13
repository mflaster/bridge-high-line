#include "golden_serializer.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "string_util.h"

std::string PlayLineGoldenSerializer::SerializeConditionalLineHelper(
    const ConditionalLine &line, int lvl) {
  return line.Describe(lvl, /*sort_branches=*/true);
}

std::string PlayLineGoldenSerializer::Serialize(const ConditionalLine &line) {
  std::string res = SerializeConditionalLineHelper(line, 0);
  return Serialize(res);
}

std::string PlayLineGoldenSerializer::Serialize(
    const std::vector<ConditionalLine> &lines) {
  if (lines.empty()) {
    return "No lines found\n";
  }
  if (lines.size() == 1) {
    return Serialize(lines[0]);
  }
  std::string result;
  for (size_t i = 0; i < lines.size(); ++i) {
    if (i > 0) result += "\n";
    result += "Line " + std::to_string(i + 1) + ":\n";
    result += Serialize(lines[i]);
  }
  return result;
}

std::string PlayLineGoldenSerializer::Serialize(
    const std::multiset<int> &tricks_won) {
  if (tricks_won.empty()) {
    return "Average: 0.000000\n";
  }
  int sum = 0;
  std::map<int, int> counts;
  for (int x : tricks_won) {
    sum += x;
    counts[x]++;
  }
  double avg = sum / static_cast<double>(tricks_won.size());
  std::ostringstream oss;
  oss << "Average: " << std::fixed << std::setprecision(6) << avg << "\n";
  for (const auto &[t, n] : counts) {
    oss << "  " << t << " tricks: " << n << "\n";
  }
  return oss.str();
}

std::string PlayLineGoldenSerializer::Serialize(const std::string &str) {
  std::string res;
  res.reserve(str.size() + 1);
  for (char c : str) {
    if (c != '\r') {
      res += c;
    }
  }
  if (res.empty() || res.back() != '\n') {
    res += '\n';
  }
  return res;
}

std::string PlayLineGoldenSerializer::Serialize(const OptimalLines &lines) {
  std::vector<std::string> sorted_lines(lines.begin(), lines.end());
  std::sort(sorted_lines.begin(), sorted_lines.end());
  std::string res;
  for (const auto &l : sorted_lines) {
    res += l + "\n";
  }
  return res;
}

std::string PlayLineGoldenSerializer::FormatHands(uint64_t hand, uint64_t unknown_cards) {
  if (hand == 0 && unknown_cards == 0) return "";
  std::string res;
  if (hand != 0) {
    res += "# Hand: " + TrimRight(HandDebugString(hand)) + "\n";
  }
  if (unknown_cards != 0) {
    res += "# Unknown cards: " + TrimRight(HandDebugString(unknown_cards)) + "\n";
  }
  return res;
}

std::string PlayLineGoldenSerializer::FormatHands(uint64_t hand, Suit trump) {
  return FormatHands(hand, 0, trump);
}

std::string PlayLineGoldenSerializer::FormatHands(uint64_t hand, uint64_t unknown_cards, Suit trump) {
  std::string res = FormatHands(hand, unknown_cards);
  res += "# Trump: " + SuitDebugString(trump) + "\n";
  return res;
}

std::string PlayLineGoldenSerializer::FormatHands(const std::vector<uint64_t> &hands, uint64_t unknown_cards) {
  if (hands.empty() && unknown_cards == 0) return "";
  std::string res;
  for (size_t i = 0; i < hands.size(); ++i) {
    res += "# Hand " + std::to_string(i) + ": " + TrimRight(HandDebugString(hands[i])) + "\n";
  }
  if (unknown_cards != 0) {
    res += "# Unknown cards: " + TrimRight(HandDebugString(unknown_cards)) + "\n";
  }
  return res;
}

std::string PlayLineGoldenSerializer::FormatHands(const std::vector<uint64_t> &hands, Suit trump) {
  return FormatHands(hands, 0, trump);
}

std::string PlayLineGoldenSerializer::FormatHands(const std::vector<uint64_t> &hands, uint64_t unknown_cards, Suit trump) {
  std::string res = FormatHands(hands, unknown_cards);
  res += "# Trump: " + SuitDebugString(trump) + "\n";
  return res;
}

std::string PlayLineGoldenSerializer::FormatHands(const std::pair<uint64_t, uint64_t> &hands, uint64_t unknown_cards) {
  std::string res;
  res += "# Hand 0: " + TrimRight(HandDebugString(hands.first)) + "\n";
  res += "# Hand 1: " + TrimRight(HandDebugString(hands.second)) + "\n";
  if (unknown_cards != 0) {
    res += "# Unknown cards: " + TrimRight(HandDebugString(unknown_cards)) + "\n";
  }
  return res;
}

std::string PlayLineGoldenSerializer::FormatHands(const std::pair<uint64_t, uint64_t> &hands, Suit trump) {
  return FormatHands(hands, 0, trump);
}

std::string PlayLineGoldenSerializer::FormatHands(const std::pair<uint64_t, uint64_t> &hands, uint64_t unknown_cards, Suit trump) {
  std::string res = FormatHands(hands, unknown_cards);
  res += "# Trump: " + SuitDebugString(trump) + "\n";
  return res;
}

std::string PlayLineGoldenSerializer::FormatHands(const std::string &hands_str) {
  if (hands_str.empty()) return "";
  std::string res;
  std::istringstream iss(hands_str);
  std::string line;
  while (std::getline(iss, line)) {
    std::string trimmed = TrimRight(line);
    if (!trimmed.empty()) {
      if (trimmed[0] == '#') {
        res += trimmed + "\n";
      } else {
        res += "# Hand: " + trimmed + "\n";
      }
    }
  }
  return res;
}

std::string PlayLineGoldenSerializer::FormatHands(const char *hands_str) {
  return hands_str ? FormatHands(std::string(hands_str)) : "";
}

std::string PlayLineGoldenSerializer::FormatHands(const BoardState &bs) {
  std::vector<uint64_t> hands = {bs.GetHand(0), bs.GetHand(1), bs.GetHand(2), bs.GetHand(3)};
  return FormatHands(hands, 0, bs.GetTrump());
}

std::string PlayLineGoldenSerializer::FormatCommentHeader(
    const std::string &description, const std::string &formatted_hands) {
  std::string header;
  if (!description.empty()) {
    std::istringstream iss(description);
    std::string line;
    while (std::getline(iss, line)) {
      std::string trimmed = TrimRight(line);
      if (!trimmed.empty()) {
        if (trimmed[0] == '#') {
          header += trimmed + "\n";
        } else {
          header += "# Setup: " + trimmed + "\n";
        }
      }
    }
  }
  if (!formatted_hands.empty()) {
    header += formatted_hands;
  }
  return header;
}

GoldenMatcher::GoldenMatcher(std::string golden_filename)
    : golden_filename_(std::move(golden_filename)) {}

static std::string GetGoldenFilePath(const std::string &filename) {
  if (filename.find("testdata/goldens/") == 0) {
    return filename;
  }
  return "testdata/goldens/" + filename;
}

static std::string ComputeLineDiff(const std::string &expected,
                                    const std::string &actual,
                                    const std::string &filename) {
  auto split_lines = [](const std::string &s) {
    std::vector<std::string> lines;
    std::string line;
    for (char c : s) {
      if (c == '\n') {
        lines.push_back(line);
        line.clear();
      } else if (c != '\r') {
        line += c;
      }
    }
    if (!line.empty()) {
      lines.push_back(line);
    }
    return lines;
  };

  std::vector<std::string> exp_lines = split_lines(expected);
  std::vector<std::string> act_lines = split_lines(actual);

  int n = exp_lines.size();
  int m = act_lines.size();
  std::vector<std::vector<int>> lcs(n + 1, std::vector<int>(m + 1, 0));

  for (int i = 1; i <= n; ++i) {
    for (int j = 1; j <= m; ++j) {
      if (exp_lines[i - 1] == act_lines[j - 1]) {
        lcs[i][j] = lcs[i - 1][j - 1] + 1;
      } else {
        lcs[i][j] = std::max(lcs[i - 1][j], lcs[i][j - 1]);
      }
    }
  }

  struct DiffLine {
    char type;
    std::string text;
  };
  std::vector<DiffLine> diff_lines;

  int i = n, j = m;
  while (i > 0 || j > 0) {
    if (i > 0 && j > 0 && exp_lines[i - 1] == act_lines[j - 1]) {
      diff_lines.push_back({' ', exp_lines[i - 1]});
      --i;
      --j;
    } else if (j > 0 && (i == 0 || lcs[i][j - 1] >= lcs[i - 1][j])) {
      diff_lines.push_back({'+', act_lines[j - 1]});
      --j;
    } else {
      diff_lines.push_back({'-', exp_lines[i - 1]});
      --i;
    }
  }
  std::reverse(diff_lines.begin(), diff_lines.end());

  std::string diff_output;
  diff_output += "--- Expected (" + filename + ")\n";
  diff_output += "+++ Actual\n";
  for (const auto &dl : diff_lines) {
    diff_output += dl.type;
    diff_output += " ";
    diff_output += dl.text;
    diff_output += "\n";
  }

  return diff_output;
}

bool GoldenMatcher::match(const std::string &actual) const {
  std::string filepath = GetGoldenFilePath(golden_filename_);
  const char *update_env = std::getenv("UPDATE_GOLDENS");
  bool should_update = (update_env != nullptr && std::string(update_env) == "1");

  if (should_update) {
    std::filesystem::path p(filepath);
    if (p.has_parent_path()) {
      std::filesystem::create_directories(p.parent_path());
    }
    std::ofstream out(filepath, std::ios::binary);
    if (!out) {
      failure_message_ = "Failed to open golden file for writing: " + filepath;
      return false;
    }
    out.write(actual.data(), actual.size());
    out.close();
    return true;
  }

  std::ifstream in(filepath, std::ios::binary);
  if (!in) {
    failure_message_ = "Golden expectation file missing: " + filepath +
                       "\nRun tests with UPDATE_GOLDENS=1 to create it.";
    return false;
  }
  std::string expected((std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>());
  in.close();

  if (expected == actual) {
    return true;
  }

  failure_message_ = "Golden result mismatch for " + filepath + ":\n" +
                     ComputeLineDiff(expected, actual, filepath);
  return false;
}

std::string GoldenMatcher::describe() const {
  return failure_message_.empty() ? ("Matches golden file " + golden_filename_)
                                  : failure_message_;
}
