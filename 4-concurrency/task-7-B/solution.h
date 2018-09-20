#pragma once

#include <atomic>
#include <thread>

///////////////////////////////////////////////////////////////////////

template <typename T, template <typename U> class Atomic = std::atomic>
class LockFreeQueue {
 struct Node {
  T element_{};
  Atomic<Node*> next_{nullptr};

  explicit Node(T element, Node* next = nullptr)
      : element_(std::move(element)),
        next_(next) {}

  explicit Node() {}
 };

 public:
  explicit LockFreeQueue() {
    Node* dummy = new Node{};
    head_ = dummy;
    tail_ = dummy;
    garbage_head_ = dummy;
  }

  LockFreeQueue& operator=(const LockFreeQueue&) = delete;
  LockFreeQueue(const LockFreeQueue&) = delete;

  ~LockFreeQueue() {
    FreeQueue();
  }

  void Enqueue(T element_to_push) {
    Node* new_node = new Node(element_to_push);
    Node* curr_tail;

    num_working_thrs.fetch_add(1);

    while (true) {
      Node* null_pointer = nullptr;
      Node* curr_tail = tail_.load();
      Node* curr_tail_next = curr_tail->next_.load();

      if (!curr_tail_next) {
        if (curr_tail->next_.compare_exchange_strong(null_pointer, new_node)) {
          break;
        }
      } else {
        tail_.compare_exchange_strong(curr_tail, curr_tail_next);
      }
    }

    tail_.compare_exchange_strong(curr_tail, new_node);

    num_working_thrs.fetch_sub(1);
  }

  bool Dequeue(T& popped_element) {
    num_working_thrs.fetch_add(1);

    while (true) {
      Node* curr_head = head_.load();
      Node* curr_head_next = curr_head->next_.load();
      Node* curr_tail = tail_.load();


      if (curr_head == curr_tail) {
        if (!curr_head_next) {
          num_working_thrs.fetch_sub(1);
          return false;
        }
        else {
          tail_.compare_exchange_strong(curr_head, curr_head_next);
        }
      } else {
        if (head_.compare_exchange_strong(curr_head, curr_head_next)) {
          popped_element = std::move(curr_head_next->element_);

          if (num_working_thrs.load() == 1) {
            FreeQueue(curr_head_next);
          }

          num_working_thrs.fetch_sub(1);

          return true;
        }
      }
    }
  }

 private:
  void FreeQueue(Node* bound = nullptr) {
    Node* head = garbage_head_.load();

    while (head != bound) {
      Node* temp = head->next_.load();
      delete head;
      head = temp;
    }

    garbage_head_.store(head);
  }

  Atomic<Node*> garbage_head_{nullptr};
  Atomic<Node*> head_{nullptr};
  Atomic<Node*> tail_{nullptr};
  Atomic<size_t> num_working_thrs{0};
};

///////////////////////////////////////////////////////////////////////
