#include <catch2/catch_all.hpp>
#include <cmath>

#include "search_progress.h"

using Catch::Matchers::WithinAbs;

namespace {
constexpr double kEps = 1e-9;
}  // namespace

TEST_CASE("SearchProgress starts at zero and finishes at one") {
  SearchProgress progress;
  REQUIRE(progress.Fraction() == 0.0);
  SearchProgress::Complete(progress.root());
  REQUIRE(progress.Fraction() == 1.0);
}

TEST_CASE("SearchProgress null nodes are ignored") {
  auto kids = SearchProgress::AddChildren(nullptr, {1.0, 2.0});
  REQUIRE(kids.size() == 2);
  REQUIRE(kids[0] == nullptr);
  REQUIRE(kids[1] == nullptr);
  auto batch = SearchProgress::BeginBatch(nullptr, {3.0}, {5.0});
  REQUIRE(batch.size() == 1);
  REQUIRE(batch[0] == nullptr);
  SearchProgress::EndBatch(nullptr, {});
  SearchProgress::SetRemaining(nullptr, {});
  SearchProgress::Complete(nullptr);
  { SearchProgress::Completer completer(nullptr); }
}

TEST_CASE("SearchProgress splits a batch by trick weight") {
  SearchProgress progress;
  // Two candidate first tricks weighing 70 and 95, with no further groups
  // able to beat them.
  auto tricks = SearchProgress::BeginBatch(progress.root(), {70.0, 95.0}, {});
  REQUIRE(tricks.size() == 2);
  REQUIRE(progress.Fraction() == 0.0);

  SearchProgress::Complete(tricks[0]);
  REQUIRE_THAT(progress.Fraction(), WithinAbs(70.0 / 165.0, kEps));

  SearchProgress::Complete(tricks[1]);
  REQUIRE_THAT(progress.Fraction(), WithinAbs(1.0, kEps));
  SearchProgress::EndBatch(progress.root(), {});
  REQUIRE_THAT(progress.Fraction(), WithinAbs(1.0, kEps));
}

TEST_CASE("SearchProgress discounts later batches by size and position") {
  const double a = SearchProgress::kBatchDecay;
  SearchProgress progress;
  SearchProgress::Node *root = progress.root();

  // First batch of size 100 while groups of size 40 and 20 could still need
  // evaluating: expected work is 100 + 40a + 20a^2.
  auto first = SearchProgress::BeginBatch(root, {60.0, 40.0}, {40.0, 20.0});
  double total = 100.0 + 40.0 * a + 20.0 * a * a;
  SearchProgress::Complete(first[0]);
  REQUIRE_THAT(progress.Fraction(), WithinAbs(60.0 / total, kEps));
  SearchProgress::Complete(first[1]);
  SearchProgress::EndBatch(root, {40.0, 20.0});
  REQUIRE_THAT(progress.Fraction(), WithinAbs(100.0 / total, kEps));

  // Second batch (the 40-group), weighing 40a, with the 20-group after it.
  auto second = SearchProgress::BeginBatch(root, {40.0}, {20.0});
  REQUIRE_THAT(progress.Fraction(), WithinAbs(100.0 / total, kEps));
  SearchProgress::Complete(second[0]);
  SearchProgress::EndBatch(root, {20.0});
  REQUIRE_THAT(progress.Fraction(), WithinAbs((100.0 + 40.0 * a) / total,
                                               kEps));

  // The best line improved enough to prune everything else.
  SearchProgress::SetRemaining(root, {});
  REQUIRE_THAT(progress.Fraction(), WithinAbs(1.0, kEps));
}

TEST_CASE("SearchProgress nests trick-winner sub-searches by deals") {
  SearchProgress progress;
  auto tricks = SearchProgress::BeginBatch(progress.root(), {100.0}, {});
  // The trick leads to sub-searches for 60 and 40 deals.
  auto winners = SearchProgress::AddChildren(tricks[0], {60.0, 40.0});
  REQUIRE(progress.Fraction() == 0.0);

  // The 40-deal sub-search runs one batch of two tricks and finishes one.
  auto inner = SearchProgress::BeginBatch(winners[1], {5.0, 5.0}, {});
  SearchProgress::Complete(inner[0]);
  REQUIRE_THAT(progress.Fraction(), WithinAbs(0.4 * 0.5, kEps));

  SearchProgress::Complete(winners[0]);
  REQUIRE_THAT(progress.Fraction(), WithinAbs(0.6 + 0.4 * 0.5, kEps));

  // Completing a node completes whatever is still running under it.
  SearchProgress::Complete(tricks[0]);
  REQUIRE_THAT(progress.Fraction(), WithinAbs(1.0, kEps));
}

TEST_CASE("SearchProgress never reports a decrease") {
  SearchProgress progress;
  auto tricks = SearchProgress::BeginBatch(progress.root(), {50.0, 50.0}, {});
  SearchProgress::Complete(tricks[0]);
  REQUIRE_THAT(progress.Fraction(), WithinAbs(0.5, kEps));

  // Extra work discovered late (a good-card split) would lower the raw
  // estimate; the reported value holds.
  SearchProgress::AddChildren(progress.root(), {10.0});
  REQUIRE_THAT(progress.Fraction(), WithinAbs(0.5, kEps));
}
