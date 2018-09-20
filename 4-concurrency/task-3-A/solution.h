#include <iostream>
#include <condition_variable>
#include <queue>
#include <mutex>
#include <stdexcept>

class BlockingQueueException : public std::exception {
};

class QueueShutdowned : public BlockingQueueException {
};

template <class T, class Container = std::deque<T>>
class BlockingQueue {
 public:
  explicit BlockingQueue(const size_t& capacity)
      : capacity_(capacity),
        turned_off_(false) {}

  void Put(T&& new_element) {
    std::unique_lock<std::mutex> lock(mutex_);

    if (!turned_off_) {
      queue_state_cv_.wait(lock, [&] { return (items_.size() < capacity_ || turned_off_); });

      if (!turned_off_) {
        items_.push_back(std::move(new_element));
        queue_state_cv_.notify_all();
      } else {
        throw QueueShutdowned();
      }
    } else {
      throw QueueShutdowned();
    }
  }

  bool Get(T& result) {
    std::unique_lock<std::mutex> lock(mutex_);

    if (turned_off_) {
      return false;
    } else {
      queue_state_cv_.wait(lock, [&] { return (!items_.empty() || turned_off_); });
      result = std::move(items_.front());
      items_.pop_front();
      queue_state_cv_.notify_all();

      return true;
    }
  }

  void Shutdown() {
    std::lock_guard<std::mutex> guard(mutex_);

    turned_off_ = true;
    queue_state_cv_.notify_all();
  }


 private:
  Container items_;
  const std::size_t capacity_;

  bool turned_off_;
  std::condition_variable queue_state_cv_;
  std::mutex mutex_;
};
