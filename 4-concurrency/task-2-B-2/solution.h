#pragma once

#include <cstddef>
#include <condition_variable>
#include <iostream>
#include <mutex>

class Semaphore
{
    public:
        Semaphore(std::size_t initial_state):
            count_(initial_state) {}

        void Wait(){
            std::unique_lock<std::mutex> lock(m_);

            cv_.wait(lock, [&]{ return (count_ != 0); });
            count_--;
        }

        void Signal(){
            std::unique_lock<std::mutex> lock(m_);

            count_++;
            cv_.notify_one();
        }

    private:
        std::size_t count_;

        std::condition_variable cv_;
        std::mutex m_;
};

class Robot
{
    public:
        Robot(): left_turn_(1), right_turn_(0) {}

        void StepLeft();
        void StepRight();

    private:
        Semaphore left_turn_, right_turn_;
};

void Robot::StepLeft(){
    left_turn_.Wait();

    std::cout << "left" << std::endl;

    right_turn_.Signal();
}

void Robot::StepRight(){
    right_turn_.Wait();

    std::cout << "right" << std::endl;

    left_turn_.Signal();
}
