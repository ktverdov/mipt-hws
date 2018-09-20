#pragma once

#include <atomic>
#include <thread>

///////////////////////////////////////////////////////////////////////

template <typename T>
class LockFreeStack {
 struct Node {
  T element_;
  std::atomic<Node*> next_;

  Node(T element, Node* next = nullptr)
      : element_(std::move(element)),
        next_(next) {}
 };

 public:
  explicit LockFreeStack() {}
  LockFreeStack& operator=(const LockFreeStack&) = delete;
  LockFreeStack(const LockFreeStack&) = delete;

  ~LockFreeStack() {
    StackFree(garbage_top_);
    StackFree(top_);
  }

  void Push(T element_to_push) {
    Node* curr_top = top_.load();
    Node* new_node = new Node(element_to_push, curr_top);

    while (!top_.compare_exchange_weak(curr_top, new_node)) {
      new_node->next_ = curr_top;
    }
  }

  bool Pop(T& popped_element) {
    Node* curr_top = top_.load();

    while (true) {
      if (!curr_top) {
        return false;
      }
      if (top_.compare_exchange_strong(curr_top, curr_top->next_.load())) {
        popped_element = curr_top->element_;

        NodeToDelete(curr_top);

        return true;
      }
    }
  }

 private:
  void NodeToDelete(Node* node_to_delete) {
    Node* curr_garbage_top = garbage_top_.load();
    node_to_delete->next_.store(curr_garbage_top);
    while (!garbage_top_.compare_exchange_strong(curr_garbage_top, node_to_delete)) {
      node_to_delete->next_.store(curr_garbage_top);
    }
  }

  void StackFree(Node* top) {
    while (top) {
      Node* temp = top->next_;
      delete top;
      top = temp;
    }
  }

  std::atomic<Node*> garbage_top_{nullptr};
  std::atomic<Node*> top_{nullptr};
};

///////////////////////////////////////////////////////////////////////

template <typename T>
using ConcurrentStack = LockFreeStack<T>;

///////////////////////////////////////////////////////////////////////
