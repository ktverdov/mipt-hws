#pragma once

#include <cstddef>
#include <condition_variable>
#include <deque>
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
        Robot(const std::size_t num_foots);
        void Step(const std::size_t foot);

    private:
        std::size_t num_foots_;
        //replaced vector such as vector can't be used with
        //nor copy-constructible and nor copy-assignable objects (atomic, mutex, ...)
        std::deque<Semaphore> foots_order_;
};

Robot::Robot(const std::size_t num_foots)
{
    num_foots_ = num_foots;

    if (num_foots > 0)
        foots_order_.emplace_back(1);

    for (std::size_t i = 1; i < num_foots_; i++)
        foots_order_.emplace_back(0);
}

void Robot::Step(const std::size_t foot){
    foots_order_[foot].Wait();

    std::cout << "foot " << foot << std::endl;

    std::size_t next_foot = foot == (num_foots_ - 1) ? 0 : foot + 1;
    foots_order_[next_foot].Signal();
}
