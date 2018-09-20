#pragma once

#include <cstddef>
#include <condition_variable>
#include <mutex>

template <typename ConditionVariable = std::condition_variable>
class CyclicBarrier
{
    public:
        CyclicBarrier(size_t num_threads): num_threads_ (num_threads),
            num_not_finished_(num_threads), level_(false){}

        void Pass();

    private:
        std::size_t num_threads_;
        std::size_t num_not_finished_;
        //describes on which phase (even / odd) barrier is
        bool level_;

        ConditionVariable all_finished_cv_;
        std::mutex m_;
};

template <typename ConditionVariable>
void CyclicBarrier<ConditionVariable>::Pass(){
    std::unique_lock<std::mutex> lock(m_);

    std::size_t current_position = num_not_finished_;
    num_not_finished_--;

    bool current_level = level_;

    if (current_position == 1){
        level_ = !level_;
        num_not_finished_ = num_threads_;
        all_finished_cv_.notify_all();
    }
    else
        all_finished_cv_.wait(lock, [&]{ return (current_level != level_); });
}
