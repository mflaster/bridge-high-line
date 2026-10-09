#include "search_progress.h"

#include <algorithm>
#include <cmath>
#include <numeric>

class SearchProgress::Node {
 public:
  Node(SearchProgress *owner, Node *parent, double weight)
      : owner(owner), parent(parent), weight(weight) {}

  SearchProgress *owner;
  Node *parent;
  // This node's weight relative to its siblings.
  double weight;
  // Total weight of completed children.
  double done = 0.0;
  // Estimated weight of children not yet started.
  double pending = 0.0;
  // Children that have started and not completed.
  std::vector<Node *> active;
  // For groups nodes: the batch currently running and the number started.
  Node *batch = nullptr;
  int batches = 0;
  // Set on the root once the whole search has finished.
  bool complete = false;
};

SearchProgress::SearchProgress() : root_(new Node(this, nullptr, 1.0)) {}

SearchProgress::~SearchProgress() {
  std::lock_guard<std::mutex> lock(mutex_);
  DeleteSubtree(root_);
}

double SearchProgress::Fraction() {
  std::lock_guard<std::mutex> lock(mutex_);
  double f = std::clamp(FractionLocked(root_), 0.0, 1.0);
  reported_ = std::max(reported_, f);
  return reported_;
}

double SearchProgress::FractionLocked(const Node *node) {
  if (node->complete) return 1.0;
  double total = node->done + node->pending;
  double finished = node->done;
  for (const Node *child : node->active) {
    total += child->weight;
    finished += child->weight * FractionLocked(child);
  }
  if (total <= 0.0) return 0.0;
  return finished / total;
}

std::vector<SearchProgress::Node *> SearchProgress::AddChildren(
    Node *parent, const std::vector<double> &weights) {
  if (parent == nullptr) {
    return std::vector<Node *>(weights.size(), nullptr);
  }
  std::lock_guard<std::mutex> lock(parent->owner->mutex_);
  return AddChildrenLocked(parent, weights);
}

std::vector<SearchProgress::Node *> SearchProgress::AddChildrenLocked(
    Node *parent, const std::vector<double> &weights) {
  std::vector<Node *> children;
  children.reserve(weights.size());
  for (double w : weights) {
    Node *child = new Node(parent->owner, parent, std::max(w, 0.0));
    parent->active.push_back(child);
    children.push_back(child);
  }
  return children;
}

std::vector<SearchProgress::Node *> SearchProgress::BeginBatch(
    Node *groups, const std::vector<double> &weights,
    const std::vector<double> &remaining) {
  if (groups == nullptr) {
    return std::vector<Node *>(weights.size(), nullptr);
  }
  std::lock_guard<std::mutex> lock(groups->owner->mutex_);
  if (groups->batch != nullptr) {
    CompleteLocked(groups->batch);
  }
  double size = std::accumulate(weights.begin(), weights.end(), 0.0);
  int index = groups->batches++;
  Node *batch = new Node(groups->owner, groups,
                         size * std::pow(kBatchDecay, index));
  groups->active.push_back(batch);
  groups->batch = batch;
  SetRemainingLocked(groups, remaining);
  return AddChildrenLocked(batch, weights);
}

void SearchProgress::EndBatch(Node *groups,
                              const std::vector<double> &remaining) {
  if (groups == nullptr) return;
  std::lock_guard<std::mutex> lock(groups->owner->mutex_);
  if (groups->batch != nullptr) {
    CompleteLocked(groups->batch);
  }
  SetRemainingLocked(groups, remaining);
}

void SearchProgress::SetRemaining(Node *groups,
                                  const std::vector<double> &remaining) {
  if (groups == nullptr) return;
  std::lock_guard<std::mutex> lock(groups->owner->mutex_);
  SetRemainingLocked(groups, remaining);
}

void SearchProgress::SetRemainingLocked(Node *groups,
                                        const std::vector<double> &remaining) {
  // The m-th batch still to come would be batch number batches + m.
  double pending = 0.0;
  double discount = std::pow(kBatchDecay, groups->batches);
  for (double size : remaining) {
    pending += std::max(size, 0.0) * discount;
    discount *= kBatchDecay;
  }
  groups->pending = pending;
}

void SearchProgress::Complete(Node *node) {
  if (node == nullptr) return;
  std::lock_guard<std::mutex> lock(node->owner->mutex_);
  CompleteLocked(node);
}

void SearchProgress::CompleteLocked(Node *node) {
  Node *parent = node->parent;
  if (parent == nullptr) {
    // The root stays allocated so Fraction() can keep reporting 1.0.
    for (Node *child : node->active) {
      DeleteSubtree(child);
    }
    node->active.clear();
    node->batch = nullptr;
    node->complete = true;
    return;
  }
  if (parent->batch == node) {
    parent->batch = nullptr;
  }
  auto it = std::find(parent->active.begin(), parent->active.end(), node);
  if (it != parent->active.end()) {
    parent->active.erase(it);
    parent->done += node->weight;
  }
  DeleteSubtree(node);
}

void SearchProgress::DeleteSubtree(Node *node) {
  for (Node *child : node->active) {
    DeleteSubtree(child);
  }
  delete node;
}
