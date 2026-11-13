#pragma once

#include <deque>
#include <functional>
#include <mutex>

namespace sp {
// Shared by off-thread context events and skill awards. Only the consumer
// executes callbacks, keeping mutable progression on the recorded main thread.
class DeferredQueue {
 public:
  bool post(std::function<void()> event) {
    std::lock_guard lock(mutex_);
    if (queue_.size() >= 4096) return false;
    queue_.push_back(std::move(event));
    return true;
  }
  void drain() {
    // Execute a finite batch outside the lock: callbacks may enqueue again.
    // Running under the lock deadlocks; draining until empty can starve Update.
    std::deque<std::function<void()>> batch;
    {
      std::lock_guard lock(mutex_);
      batch.swap(queue_);
    }
    for (auto& event : batch) event();
  }
  void clear() {
    // A load/revert must not apply queued actions from the previous world.
    std::lock_guard lock(mutex_);
    queue_.clear();
  }

 private:
  std::mutex mutex_;
  std::deque<std::function<void()>> queue_;
};
}  // namespace sp
