#ifndef BRIDGE_OMP_COMPAT_H_
#define BRIDGE_OMP_COMPAT_H_

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

class OmpCompatPool {
 public:
  struct TaskBatch {
    void (*run)(void *ctx, size_t idx) = nullptr;
    void *ctx = nullptr;
    size_t count = 0;
    size_t next_index = 0;
    size_t remaining = 0;
  };

  static OmpCompatPool &Instance() {
    static OmpCompatPool *pool = new OmpCompatPool();
    return *pool;
  }

  static int ThreadNum() { return tl_thread_num_; }
  static int MaxThreads() { return tl_max_threads_; }
  static int InParallel() { return tl_in_parallel_; }

  template <typename Fn>
  void RunRegion(int num_threads, Fn &&fn) {
    if (num_threads <= 1 || tl_in_parallel_) {
      int prev_max = tl_max_threads_;
      if (!tl_in_parallel_) tl_max_threads_ = 1;
      fn();
      tl_max_threads_ = prev_max;
      return;
    }

    {
      std::unique_lock<std::mutex> lock(mu_);
      EnsureWorkersLocked(num_threads);
      while (active_workers_busy_ > 0) {
        cv_.wait(lock);
      }
      active_num_threads_ = num_threads;
    }

    tl_thread_num_ = 0;
    tl_in_parallel_ = 1;
    tl_max_threads_ = num_threads;
    fn();
    tl_in_parallel_ = 0;
    tl_max_threads_ = 1;

    {
      std::unique_lock<std::mutex> lock(mu_);
      active_num_threads_ = 1;
      while (active_workers_busy_ > 0) {
        cv_.wait(lock);
      }
    }
  }

  template <typename Fn>
  void RunTaskBatch(size_t count, Fn &&fn) {
    if (count == 0) return;
    if (count == 1 || !tl_in_parallel_) {
      for (size_t i = 0; i < count; ++i) {
        fn(i);
      }
      return;
    }

    auto runner = [](void *ctx, size_t idx) {
      (*static_cast<std::remove_reference_t<Fn> *>(ctx))(idx);
    };
    TaskBatch batch{runner, &fn, count, 0, count};

    std::unique_lock<std::mutex> lock(mu_);
    active_batches_.push_back(&batch);
    cv_.notify_all();

    while (batch.remaining > 0) {
      TaskBatch *work = FindBatchAtOrAfterLocked(&batch);
      if (work != nullptr) {
        size_t idx = work->next_index++;
        lock.unlock();
        work->run(work->ctx, idx);
        lock.lock();
        FinishOneItemLocked(work);
      } else {
        cv_.wait(lock);
      }
    }
  }

 private:
  OmpCompatPool() = default;

  void EnsureWorkersLocked(int num_threads) {
    size_t needed = static_cast<size_t>(std::max(0, num_threads - 1));
    while (workers_.size() < needed) {
      int worker_id = static_cast<int>(workers_.size() + 1);
      workers_.emplace_back([this, worker_id]() { WorkerLoop(worker_id); });
    }
  }

  TaskBatch *FindLatestClaimableLocked() {
    for (auto it = active_batches_.rbegin(); it != active_batches_.rend();
         ++it) {
      if ((*it)->next_index < (*it)->count) {
        return *it;
      }
    }
    return nullptr;
  }

  TaskBatch *FindBatchAtOrAfterLocked(TaskBatch *target) {
    if (target->next_index < target->count) {
      return target;
    }
    bool seen_target = false;
    TaskBatch *best = nullptr;
    for (TaskBatch *b : active_batches_) {
      if (b == target) {
        seen_target = true;
      }
      if (seen_target && b->next_index < b->count) {
        best = b;
      }
    }
    return best;
  }

  void FinishOneItemLocked(TaskBatch *work) {
    if (--work->remaining == 0) {
      auto it = std::find(active_batches_.begin(), active_batches_.end(), work);
      if (it != active_batches_.end()) {
        active_batches_.erase(it);
      }
      cv_.notify_all();
    }
  }

  void WorkerLoop(int worker_id) {
    tl_thread_num_ = worker_id;
    std::unique_lock<std::mutex> lock(mu_);
    while (true) {
      TaskBatch *work = (worker_id < active_num_threads_)
                            ? FindLatestClaimableLocked()
                            : nullptr;
      if (work != nullptr) {
        size_t idx = work->next_index++;
        ++active_workers_busy_;
        tl_in_parallel_ = 1;
        tl_max_threads_ = active_num_threads_;
        lock.unlock();
        work->run(work->ctx, idx);
        lock.lock();
        tl_in_parallel_ = 0;
        tl_max_threads_ = 1;
        --active_workers_busy_;
        FinishOneItemLocked(work);
        if (active_workers_busy_ == 0 && active_num_threads_ == 1) {
          cv_.notify_all();
        }
      } else {
        cv_.wait(lock);
      }
    }
  }

  inline static thread_local int tl_thread_num_ = 0;
  inline static thread_local int tl_max_threads_ = 1;
  inline static thread_local int tl_in_parallel_ = 0;

  std::mutex mu_;
  std::condition_variable cv_;
  std::vector<std::thread> workers_;
  std::vector<TaskBatch *> active_batches_;
  int active_num_threads_ = 1;
  int active_workers_busy_ = 0;
};

#if defined(_OPENMP) && !defined(BRIDGE_FORCE_STD_THREADS)
#include <omp.h>

template <typename Fn>
inline void ParallelForDynamic(size_t count, int num_threads, Fn &&fn) {
  if (count == 0) return;
  if (num_threads <= 1 || count == 1) {
    for (size_t i = 0; i < count; ++i) {
      fn(i);
    }
    return;
  }
  #pragma omp parallel for num_threads(num_threads) schedule(dynamic)
  for (size_t i = 0; i < count; ++i) {
    fn(i);
  }
}

template <typename Fn>
inline void ParallelRegion(int num_threads, Fn &&fn) {
  if (num_threads <= 1) {
    fn();
    return;
  }
  #pragma omp parallel num_threads(num_threads)
  {
    #pragma omp single
    {
      fn();
    }
  }
}

template <typename Fn>
inline void ParallelTaskBatch(size_t count, Fn &&fn) {
  if (count == 0) return;
  if (!omp_in_parallel() || count == 1) {
    for (size_t i = 0; i < count; ++i) {
      fn(i);
    }
    return;
  }
  for (size_t i = 0; i < count; ++i) {
    #pragma omp task firstprivate(i) shared(fn)
    {
      fn(i);
    }
  }
  #pragma omp taskwait
}

#else  // !defined(_OPENMP) || defined(BRIDGE_FORCE_STD_THREADS)

inline int omp_get_thread_num() { return OmpCompatPool::ThreadNum(); }
inline int omp_get_max_threads() { return OmpCompatPool::MaxThreads(); }
inline void omp_set_num_threads(int) {}
inline int omp_in_parallel() { return OmpCompatPool::InParallel(); }

template <typename Fn>
inline void ParallelTaskBatch(size_t count, Fn &&fn) {
  OmpCompatPool::Instance().RunTaskBatch(count, std::forward<Fn>(fn));
}

template <typename Fn>
inline void ParallelRegion(int num_threads, Fn &&fn) {
  OmpCompatPool::Instance().RunRegion(num_threads, std::forward<Fn>(fn));
}

template <typename Fn>
inline void ParallelForDynamic(size_t count, int num_threads, Fn &&fn) {
  if (count == 0) return;
  if (num_threads <= 1 || count == 1 || omp_in_parallel()) {
    for (size_t i = 0; i < count; ++i) {
      fn(i);
    }
    return;
  }
  int n = std::min<int>(num_threads, static_cast<int>(count));
  ParallelRegion(n, [&]() { ParallelTaskBatch(count, fn); });
}

#endif  // defined(_OPENMP) && !defined(BRIDGE_FORCE_STD_THREADS)

#endif  // BRIDGE_OMP_COMPAT_H_
