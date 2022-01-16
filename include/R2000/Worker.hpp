//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file Worker.hpp

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <future>
#include <iostream>

class Worker {
public:
    /**
     *
     * @param run
     * @param maxJob
     */
    explicit Worker(const unsigned int maxJob = std::numeric_limits<unsigned int>::max())
            : maxJob(maxJob) {
        workingTask = std::async(std::launch::async, [&] {
            for (; !interruptFlag.load(std::memory_order_acquire);) {
                decltype(jobs) localJobQueue{};
                {
                    std::unique_lock<std::mutex> guard(jobLock);
                    jobQueueCondition.wait(guard, [this] {
                        return !jobs.empty() || interruptFlag.load(std::memory_order_acquire);
                    });
                    if (interruptFlag.load(std::memory_order_acquire))
                        return;
                    std::swap(jobs, localJobQueue);
                }
                for (auto &job : localJobQueue) {
                    std::invoke(job);
                }
            }
        });
    }

    ~Worker() {
        if (!interruptFlag.load(std::memory_order_acquire)) {
            {
                std::unique_lock<std::mutex> guard(jobLock, std::adopt_lock);
                interruptFlag.store(true, std::memory_order_release);
                jobQueueCondition.notify_one();
            }
            workingTask.wait();
            jobs.clear();
        }
    }

    template<typename... Args>
    inline bool pushJob(Args &&... args) {
        std::lock_guard<std::mutex> guard(jobLock);
        if (jobs.size() <= maxJob) {
            jobs.push_back(std::bind(std::forward<Args>(args)...));
            jobQueueCondition.notify_one();
            return true;
        }
        return false;
    }

private:
    std::condition_variable jobQueueCondition{};
    std::list<std::function<void()>> jobs{};
    std::mutex jobLock{};
    std::future<void> workingTask{};
    std::atomic_bool interruptFlag{false};
    const unsigned int maxJob{};
};
