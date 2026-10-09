#pragma once

#include <memory>
#include <mutex>
#include <vector>

// Estimates how far LineFinder's search has progressed.
//
// The search is a tree. A "groups" node (GetLinesFromGroups) evaluates
// batches of candidate tricks one after another, and a "trick" node
// (GetLineFromTrick) runs one sub-search per possible trick winner. Each node
// owns a share of its parent's work and splits it among its children:
//   * a trick node splits its share among the winner sub-searches in
//     proportion to the deals each one handles;
//   * a batch splits its share among its tricks in proportion to their
//     weights (deals the trick applies to, scaled up for tricks that leave
//     every deal in play);
//   * a groups node weighs each batch by its size, discounted geometrically
//     by its position, since later groups can win fewer deals and reuse more
//     cached work. The work still expected is the discounted size of the
//     groups that can still beat the best line found so far.
// A node's progress is the completed share of its subtree; Fraction() reports
// the root's progress and never decreases.
//
// Every static method accepts a null node (and then returns null children),
// so untracked searches need no special casing at the call sites.
class SearchProgress {
 public:
  class Node;

  // Each batch of a groups node is expected to cost this fraction, per unit
  // of size, of the batch before it. Measured solves showed a dominant first
  // batch followed by a long tail of cheaper ones; smaller values front-load
  // the estimate, larger ones hold back too much for tails that never come.
  static constexpr double kBatchDecay = 0.6;

  SearchProgress();
  ~SearchProgress();
  SearchProgress(const SearchProgress &) = delete;
  SearchProgress &operator=(const SearchProgress &) = delete;

  Node *root() const { return root_; }

  // Estimated fraction of the whole search that is finished, in [0, 1].
  double Fraction();

  // Adds children of `parent` with the given relative weights.
  static std::vector<Node *> AddChildren(Node *parent,
                                         const std::vector<double> &weights);

  // Starts the next batch under a groups node and returns one child per
  // entry of `weights` (the weight of each trick in the batch). `remaining`
  // holds the estimated size of each batch the node may still run after this
  // one, in order.
  static std::vector<Node *> BeginBatch(Node *groups,
                                        const std::vector<double> &weights,
                                        const std::vector<double> &remaining);

  // Marks the batch started by the last BeginBatch finished and updates the
  // estimate of the batches still to come.
  static void EndBatch(Node *groups, const std::vector<double> &remaining);

  // Updates the estimate of the batches still to come, e.g. after a cached
  // group raised the best coverage; empty means no more batches.
  static void SetRemaining(Node *groups, const std::vector<double> &remaining);

  // Marks a node, and anything still running under it, complete.
  static void Complete(Node *node);

  // Completes a node when the scope ends.
  class Completer {
   public:
    explicit Completer(Node *node) : node_(node) {}
    ~Completer() { Complete(node_); }
    Completer(const Completer &) = delete;
    Completer &operator=(const Completer &) = delete;

   private:
    Node *node_;
  };

 private:
  static double FractionLocked(const Node *node);
  static void CompleteLocked(Node *node);
  static void DeleteSubtree(Node *node);
  static std::vector<Node *> AddChildrenLocked(
      Node *parent, const std::vector<double> &weights);
  static void SetRemainingLocked(Node *groups,
                                 const std::vector<double> &remaining);

  std::mutex mutex_;
  Node *root_;
  double reported_ = 0.0;
};
