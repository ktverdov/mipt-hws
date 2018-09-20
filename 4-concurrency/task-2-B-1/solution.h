#pragma once

#include <cstddef>
#include <condition_variable>
#include <iostream>
#include <mutex>

class Robot
{
    public:
        //first leg is left
        Robot(): leg_(0) {}

        void StepLeft();
        void StepRight();

    private:
        //leg to go now (left - 0, right - 1)
        bool leg_;

        std::condition_variable left_turn_cv_, right_turn_cv_;
        std::mutex m_;
};

void Robot::StepLeft(){
    std::unique_lock<std::mutex> lock(m_);

    left_turn_cv_.wait(lock, [&]{ return (leg_ == 0); });

    std::cout << "left" << std::endl;

    leg_ = !leg_;
    right_turn_cv_.notify_one();
}

void Robot::StepRight(){
    std::unique_lock<std::mutex> lock(m_);

    right_turn_cv_.wait(lock, [&]{ return (leg_ == 1); });

    std::cout << "right" << std::endl;

    leg_ = !leg_;
    left_turn_cv_.notify_one();
}
