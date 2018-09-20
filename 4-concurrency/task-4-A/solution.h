#pragma once

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <forward_list>
#include <functional>
#include <mutex>
#include <vector>


class Semaphore {
 public:
  Semaphore(const std::size_t initial_state = 0) : count_(initial_state) {}

  void Wait() {
    std::unique_lock<std::mutex> lock(m_);

    cv_.wait(lock, [&]{ return count_ != 0; });
    count_--;
  }

  void Signal() {
    std::unique_lock<std::mutex> lock(m_);

    count_++;
    cv_.notify_one();
  }

  private:
    std::size_t count_;

    std::condition_variable cv_;
    std::mutex m_;
};


class ReaderWriterMutex {
 public:
  ReaderWriterMutex() : locked_(1), readers_(0) {}

  void WriteLock() {
    std::lock_guard<std::mutex> guard(mutex_);
    locked_.Wait();
  }

  void WriteUnlock() {
    locked_.Signal();
  }

  void ReadLock() {
    std::lock_guard<std::mutex> guard(mutex_);
    if (readers_.fetch_add(1) == 0)
      locked_.Wait();
  }

  void ReadUnlock() {
    if (readers_.fetch_sub(1) == 1)
      locked_.Signal();
  }

 private:
  Semaphore locked_;
  std::mutex mutex_;
  std::atomic<std::size_t> readers_;
};


template <typename T, class Hash = std::hash<T> >
class StripedHashSet {
 public:
  explicit StripedHashSet(const std::size_t concurrency_level,
                          const std::size_t growth_factor = 3,
                          const double load_factor = 0.75)
      : locks_(concurrency_level),
        table_(concurrency_level * 10),
        table_capacity_(concurrency_level * 10),
        num_elements_(0),
        max_load_factor_(load_factor),
        growth_factor_(growth_factor) {}

  explicit StripedHashSet (const StripedHashSet&) = delete;
  StripedHashSet& operator=(const StripedHashSet&) = delete;

  ~StripedHashSet() = default;

  bool Insert(const T& element_to_insert);

  bool Remove(const T& element_to_remove);

  bool Contains(const T& element_to_find) {
    std::size_t hash_value = Hash()(element_to_find);
    std::size_t lock_index = GetStripeIndex(hash_value);

    locks_[lock_index].ReadLock();

    std::size_t bucket_index = GetBucketIndex(hash_value);

    bool element_found = false;
    auto it = std::find(table_[bucket_index].begin(), table_[bucket_index].end(),
                        element_to_find);
    if (it != table_[bucket_index].end())
      element_found = true;

    locks_[lock_index].ReadUnlock();

    return element_found;
  }

  std::size_t Size() {
    return num_elements_.load();
  }

 private:
  void Resize();

  std::size_t GetBucketIndex(const size_t hash_value) const {
    return hash_value % table_capacity_.load();
  }

  std::size_t GetStripeIndex(const size_t hash_value) const {
    return hash_value % locks_.size();
  }

 private:
  std::vector<ReaderWriterMutex> locks_;

  std::vector< std::forward_list<T> > table_;
  std::atomic<std::size_t> table_capacity_;

  std::atomic<std::size_t> num_elements_;

  const double max_load_factor_;
  const std::size_t growth_factor_;
};

template <typename T, class Hash>
bool StripedHashSet<T, Hash>::Insert(const T& element_to_insert) {
  const bool element_exist = false;
  const bool successful_insert = true;

  bool element_found = Contains(element_to_insert);

  if (!element_found) {
    std::size_t hash_value = Hash()(element_to_insert);
    std::size_t lock_index = GetStripeIndex(hash_value);

    locks_[lock_index].WriteLock();

    std::size_t bucket_index = GetBucketIndex(hash_value);

    auto it = std::find(table_[bucket_index].begin(), table_[bucket_index].end(),
                        element_to_insert);

    if (it != table_[bucket_index].end()) {
      locks_[lock_index].WriteUnlock();
      return element_exist;
    } else {
      table_[bucket_index].push_front(element_to_insert);
      num_elements_.fetch_add(1);
    }

    if ((double)num_elements_.load() / table_capacity_.load() > max_load_factor_) {
      locks_[lock_index].WriteUnlock();
      Resize();
    } else {
      locks_[lock_index].WriteUnlock();
    }

    return successful_insert;
  } else {
    return element_exist;
  }
}

template <typename T, class Hash>
bool StripedHashSet<T, Hash>::Remove(const T& element_to_remove) {
  const bool element_not_exist = false;
  const bool successful_remove = true;

  bool element_found = Contains(element_to_remove);

  if (element_found) {
    std::size_t hash_value = Hash()(element_to_remove);
    std::size_t lock_index = GetStripeIndex(hash_value);

    locks_[lock_index].WriteLock();

    std::size_t bucket_index = GetBucketIndex(hash_value);

    bool element_found = false;
    auto it = std::find(table_[bucket_index].begin(), table_[bucket_index].end(),
                        element_to_remove);
    if (it != table_[bucket_index].end())
      element_found = true;

    if (element_found) {
      table_[bucket_index].remove(element_to_remove);
      num_elements_.fetch_sub(1);
    }

    locks_[lock_index].WriteUnlock();

    if (!element_found)
      return element_not_exist;

    return successful_remove;
  } else {
    return element_not_exist;
  }
}

template <typename T, class Hash>
void StripedHashSet<T, Hash>::Resize() {
  locks_[0].WriteLock();

  if ((double)num_elements_.load() / table_capacity_.load() > max_load_factor_) {
    for (auto it = locks_.begin() + 1; it != locks_.end(); it++)
      it->WriteLock();

    std::size_t new_table_capacity = table_capacity_.load() * growth_factor_;
    table_capacity_.store(new_table_capacity);

    std::vector< std::forward_list<T> > new_table(new_table_capacity);

    for (auto& forw_list : table_)
      for (auto&& element : forw_list) {
        std::size_t hash_value = Hash()(element);
        std::size_t bucket_index = GetBucketIndex(hash_value);
        new_table[bucket_index].emplace_front(element);
      }

    table_.swap(new_table);

    for (auto it = locks_.rbegin(); it != locks_.rend(); it++)
      it->WriteUnlock();
  } else {
      locks_[0].WriteUnlock();
  }
}

template <typename T> using ConcurrentSet = StripedHashSet<T>;
