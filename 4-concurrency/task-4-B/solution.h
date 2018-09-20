#pragma once

#include "arena_allocator.h"

#include <atomic>
#include <limits>

///////////////////////////////////////////////////////////////////////

template <typename T>
struct ElementTraits {
  static T Min() {
    return std::numeric_limits<T>::min();
  }
  static T Max() {
    return std::numeric_limits<T>::max();
  }
};

///////////////////////////////////////////////////////////////////////

//use Ticket SpinLock
class SpinLock {
 public:
  explicit SpinLock()
      : owner_ticket(0),
        next_free_ticket(0) {}

  void Lock() {
    std::size_t curr_thread_ticket = next_free_ticket.fetch_add(1);

    while (curr_thread_ticket != owner_ticket.load())
      {}
  }

  void Unlock() {
    owner_ticket.store(owner_ticket.load() + 1);
  }

  // adapters for BasicLockable concept
  void lock() {
    Lock();
  }

  void unlock() {
    Unlock();
  }
 private:
  std::atomic<size_t> owner_ticket;
  std::atomic<size_t> next_free_ticket;
};

///////////////////////////////////////////////////////////////////////

template <typename T>
class OptimisticLinkedSet {
 private:
  struct Node {
    T element_;
    std::atomic<Node*> next_;
    std::atomic<bool> marked_{false};

    SpinLock lock_{};

    Node(const T& element, Node* next = nullptr)
        : element_(element),
          next_(next) {}
  };

  struct Edge {
    Node* pred_;
    Node* curr_;

    Edge(Node* pred, Node* curr)
        : pred_(pred),
          curr_(curr) {}
  };

 public:
  explicit OptimisticLinkedSet(ArenaAllocator& allocator)
      : allocator_(allocator) {
        CreateEmptyList();
  }

  bool Insert(const T& element);

  bool Remove(const T& element);

  bool Contains(const T& element) const {
    Node* curr = head_;
    while (curr->element_ < element)
      curr = curr->next_.load();

    return (curr->element_ == element && !curr->marked_.load());
  }

  size_t Size() const {
    size_t size = 0;
    Node* curr = head_;

    while (curr->next_.load()) {
      curr = curr->next_.load();
      size++;
    }
    //subtract to account for last sentinel
    size--;

    return size;
  }

 private:
  void CreateEmptyList() {
    //add sentinel nodes
    head_ = allocator_.New<Node>(ElementTraits<T>::Min());
    head_->next_ = allocator_.New<Node>(ElementTraits<T>::Max());

  }

  Edge Locate(const T& element) const {
    Edge edge(head_, head_->next_);
    while (edge.curr_->element_ < element) {
      edge.pred_ = edge.curr_;
      edge.curr_ = edge.curr_->next_.load();
    }

    return edge;
  }

  bool Validate(const Edge& edge) const {
    return (edge.pred_->next_ == edge.curr_ &&
            !edge.pred_->marked_ && !edge.curr_->marked_);
  }

 private:
  ArenaAllocator& allocator_;
  Node* head_{nullptr};

};


template<typename T>
bool OptimisticLinkedSet<T>::Insert(const T& element) {
  bool element_exist = false;
  bool successful_insert = false;

  while(true) {
    Edge edge = Locate(element);

    edge.pred_->lock_.lock();
    edge.curr_->lock_.lock();

    if (Validate(edge)) {
      if (edge.curr_->element_ != element) {
        Node* added_node = allocator_.New<Node>(element);
        added_node->next_ = edge.curr_;
        edge.pred_->next_.store(added_node);

        successful_insert = true;
      }
      else {
        element_exist = true;
      }
    }

    edge.curr_->lock_.unlock();
    edge.pred_->lock_.unlock();

    if (element_exist)
      return false;
    if (successful_insert)
      return true;
  }
}

template<typename T>
bool OptimisticLinkedSet<T>::Remove(const T& element) {
  bool element_not_exist = false;
  bool successful_remove = false;

  while(true) {
    Edge edge = Locate(element);

    edge.pred_->lock_.lock();
    edge.curr_->lock_.lock();

    if (Validate(edge)) {
      if (edge.curr_->element_ == element) {
        edge.curr_->marked_.store(true);
        edge.pred_->next_.store(edge.curr_->next_);

        successful_remove = true;
      }
      else {
        element_not_exist = true;
      }
    }

    edge.curr_->lock_.unlock();
    edge.pred_->lock_.unlock();

    if (element_not_exist)
      return false;
    if (successful_remove)
      return true;
  }
}

template <typename T> using ConcurrentSet = OptimisticLinkedSet<T>;

///////////////////////////////////////////////////////////////////////
