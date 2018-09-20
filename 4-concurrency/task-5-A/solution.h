#pragma once

#include "spinlock_pause.h"

#include <iostream>
#include <atomic>
#include <thread>

//////////////////////////////////////////////////////////////////////

/* scalable and fair MCS (Mellor-Crummey, Scott) spinlock implementation
 *
 * usage:
 * {
 *   MCSSpinLock::Guard guard(spinlock); // spinlock acquired
 *   ... // in critical section
 * } // spinlock released
 */

template <template <typename T> class Atomic = std::atomic>
class MCSSpinLock {
  public:
    class Guard {
      public:
        explicit Guard(MCSSpinLock& spinlock)
            : spinlock_(spinlock) {
          Acquire();
        }

        ~Guard() {
          Release();
        }

      private:
        void Acquire() {
          // add self to spinlock queue and wait for ownership
          Guard* prev_tail = spinlock_.wait_queue_tail_.exchange(this);
          if (prev_tail != nullptr)
            prev_tail->next_.store(this);
          else
            is_owner_ = true;

          while (!is_owner_.load()) {
            //wait
          }
        }

        void Release() {
          /* transfer ownership to the next guard node in spinlock wait queue
          * or reset tail pointer if there are no other contenders
          */
          if (next_.load() == nullptr) {
            Guard* temp_this = this;
            if (spinlock_.wait_queue_tail_.compare_exchange_strong(temp_this, nullptr))
              return;

            while (next_.load() == nullptr) {
              //wait
            }
          }
          next_.load()->is_owner_ .store(true);
        }

      private:
        MCSSpinLock& spinlock_;

        Atomic<bool> is_owner_{false};
        Atomic<Guard*> next_{nullptr};
    };

  private:
    Atomic<Guard*> wait_queue_tail_{nullptr};
};

/////////////////////////////////////////////////////////////////////

// alias for checker
template <template <typename T> class Atomic = std::atomic>
using SpinLock = MCSSpinLock<Atomic>;

/////////////////////////////////////////////////////////////////////
