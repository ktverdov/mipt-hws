#pragma once
#include <thread>
#include <vector>

template <class Task>
void hello_world(std::function<void(const Task&)> func, const std::vector<Task>& tasks)
{
    std::vector<std::thread> threads;

    for (const auto& task : tasks)
        threads.emplace_back(func, std::ref(task));

    for (auto& thr : threads)
        thr.join();
}
