//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file Worker.hpp

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <list>
#include <mutex>

class Worker
{
public:
    /**
     * Construct a new worker that can handle task asynchronously.
     * @param maxNumberOfJob The maximum number of job to queue for execution.
     */
    explicit Worker(const unsigned int maxNumberOfJob = std::numeric_limits<unsigned int>::max())
        : maxJob(maxNumberOfJob)
    {
        workingTask = std::async(
            std::launch::async,
            [&]
            {
                for (; !interruptFlag.load(std::memory_order_acquire);)
                {
                    decltype(jobs) localJobQueue{};
                    {
                        std::unique_lock<std::mutex> guard{jobLock, std::adopt_lock};
                        jobQueueCondition.wait(
                            guard, [&] { return !jobs.empty() || interruptFlag.load(std::memory_order_acquire); });
                        if (interruptFlag.load(std::memory_order_acquire))
                        {
                            return;
                        }
                        std::swap(jobs, localJobQueue);
                    }
                    for (auto& job : localJobQueue)
                    {
                        std::invoke(job);
                    }
                }
            });
    }

    /**
     * Interrupt and wait for the worker to finish.
     */
    virtual ~Worker()
    {
        if (!interruptFlag.load(std::memory_order_acquire))
        {
            {
                std::unique_lock<std::mutex> guard{jobLock, std::adopt_lock};
                interruptFlag.store(true, std::memory_order_release);
                jobQueueCondition.notify_one();
            }
            workingTask.wait();
            jobs.clear();
        }
    }

    /**
     * Queue a new job to execute.
     * @tparam Args The arguments type of the job to execute.
     * @param args The arguments of the job to execute.
     * @return True if the job has been queued for execution, False if the
     * maximum number of waiting job has been reached.
     */
    template <typename Fn, typename... Args, typename = std::enable_if_t<std::is_invocable_v<Fn, Args...>>,
              typename = std::enable_if_t<std::is_same_v<std::decay_t<std::invoke_result_t<Fn, Args...>>, void>>>
    inline bool pushJob(Fn&& fn, Args&&... args) noexcept
    {
        std::lock_guard<std::mutex> guard{jobLock, std::adopt_lock};
        if (jobs.size() <= maxJob)
        {
            // jobs.push_back(std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...));
            jobs.push_back([&]() { std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...); });
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
