#include <gtest/gtest.h>
#include <cmath>

#include "search_progress.h"

namespace {
constexpr double kEps = 1e-9;
}  // namespace

TEST(SearchProgressTest, SearchProgressStartsAtZeroAndFinishesAtOne) {
  SearchProgress progress;
  ASSERT_TRUE(progress.Fraction() == 0.0);
  SearchProgress::Complete(progress.root());
  ASSERT_TRUE(progress.Fraction() == 1.0);
}

TEST(SearchProgressTest, SearchProgressNullNodesAreIgnored) {
  auto kids = SearchProgress::AddChildren(nullptr, {1.0, 2.0});
  ASSERT_TRUE(kids.size() == 2);
  ASSERT_TRUE(kids[0] == nullptr);
  ASSERT_TRUE(kids[1] == nullptr);
  auto batch = SearchProgress::BeginBatch(nullptr, {3.0}, {5.0});
  ASSERT_TRUE(batch.size() == 1);
  ASSERT_TRUE(batch[0] == nullptr);
  SearchProgress::EndBatch(nullptr, {});
  SearchProgress::SetRemaining(nullptr, {});
  SearchProgress::Complete(nullptr);
  { SearchProgress::Completer completer(nullptr); }
}

TEST(SearchProgressTest, SearchProgressSplitsABatchByTrickWeight) {
  SearchProgress progress;
  // Two candidate first tricks weighing 70 and 95, with no further groups
  // able to beat them.
  auto tricks = SearchProgress::BeginBatch(progress.root(), {70.0, 95.0}, {});
  ASSERT_TRUE(tricks.size() == 2);
  ASSERT_TRUE(progress.Fraction() == 0.0);

  SearchProgress::Complete(tricks[0]);
  EXPECT_NEAR(progress.Fraction(), 70.0 / 165.0, kEps);

  SearchProgress::Complete(tricks[1]);
  EXPECT_NEAR(progress.Fraction(), 1.0, kEps);
  SearchProgress::EndBatch(progress.root(), {});
  EXPECT_NEAR(progress.Fraction(), 1.0, kEps);
}

TEST(SearchProgressTest, SearchProgressDiscountsLaterBatchesBySizeAndPosition) {
  const double a = SearchProgress::kBatchDecay;
  SearchProgress progress;
  SearchProgress::Node *root = progress.root();

  // First batch of size 100 while groups of size 40 and 20 could still need
  // evaluating: expected work is 100 + 40a + 20a^2.
  auto first = SearchProgress::BeginBatch(root, {60.0, 40.0}, {40.0, 20.0});
  double total = 100.0 + 40.0 * a + 20.0 * a * a;
  SearchProgress::Complete(first[0]);
  EXPECT_NEAR(progress.Fraction(), 60.0 / total, kEps);
  SearchProgress::Complete(first[1]);
  SearchProgress::EndBatch(root, {40.0, 20.0});
  EXPECT_NEAR(progress.Fraction(), 100.0 / total, kEps);

  // Second batch (the 40-group), weighing 40a, with the 20-group after it.
  auto second = SearchProgress::BeginBatch(root, {40.0}, {20.0});
  EXPECT_NEAR(progress.Fraction(), 100.0 / total, kEps);
  SearchProgress::Complete(second[0]);
  SearchProgress::EndBatch(root, {20.0});
  EXPECT_NEAR(progress.Fraction(), (100.0 + 40.0 * a) / total, kEps);

  // The best line improved enough to prune everything else.
  SearchProgress::SetRemaining(root, {});
  EXPECT_NEAR(progress.Fraction(), 1.0, kEps);
}

TEST(SearchProgressTest, SearchProgressNestsTrickWinnerSubSearchesByDeals) {
  SearchProgress progress;
  auto tricks = SearchProgress::BeginBatch(progress.root(), {100.0}, {});
  // The trick leads to sub-searches for 60 and 40 deals.
  auto winners = SearchProgress::AddChildren(tricks[0], {60.0, 40.0});
  ASSERT_TRUE(progress.Fraction() == 0.0);

  // The 40-deal sub-search runs one batch of two tricks and finishes one.
  auto inner = SearchProgress::BeginBatch(winners[1], {5.0, 5.0}, {});
  SearchProgress::Complete(inner[0]);
  EXPECT_NEAR(progress.Fraction(), 0.4 * 0.5, kEps);

  SearchProgress::Complete(winners[0]);
  EXPECT_NEAR(progress.Fraction(), 0.6 + 0.4 * 0.5, kEps);

  // Completing a node completes whatever is still running under it.
  SearchProgress::Complete(tricks[0]);
  EXPECT_NEAR(progress.Fraction(), 1.0, kEps);
}

TEST(SearchProgressTest, SearchProgressNeverReportsADecrease) {
  SearchProgress progress;
  auto tricks = SearchProgress::BeginBatch(progress.root(), {50.0, 50.0}, {});
  SearchProgress::Complete(tricks[0]);
  EXPECT_NEAR(progress.Fraction(), 0.5, kEps);

  // Extra work discovered late (a good-card split) would lower the raw
  // estimate; the reported value holds.
  SearchProgress::AddChildren(progress.root(), {10.0});
  EXPECT_NEAR(progress.Fraction(), 0.5, kEps);
}
