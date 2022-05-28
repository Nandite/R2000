// Copyright (c) 2022 Papa Libasse Sow.
// https://github.com/Nandite/R2000
// Distributed under the MIT Software License (X11 license).
//
// SPDX-License-Identifier: MIT
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of
// the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

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
            jobs.push_back(std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...));
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
