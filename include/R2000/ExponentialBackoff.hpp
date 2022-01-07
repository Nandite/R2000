//! Copyright B2A Technology
//! This file and its content are the property of B2A Technology.
//! Any reproduction or use outside B2A Technology is prohibited.
//!
//! @file ExponentialBackoff.hpp

#ifndef EXPONENTIAL_BACKOFF_HPP
#define EXPONENTIAL_BACKOFF_HPP

#include <algorithm>
#include <type_traits>
#include <utility>
namespace Retry
{
/**
 * Implementation of the exponential backoff algorithm. It is useful when we want to modulate the rate
 * of some process in time in a multiplicative fashion. Here is an example of use :
 *
 * using namespace chrono_literals;
 *
 * bool isRetryable() {...};
 *
 * Retry(10, 50ms, 1500ms,
 *      [this](const auto& duration) {std::this_thread::sleep_for(duration);};
 *      Retry::ForwardStatus,
 *      [this]() -> bool {
 *      return isRetryable(...);
 *      });
 *
 * The above code will block until the max number of retry count has been reached or the isRetryable(...)
 * function return false.
 *
 */
const auto ForwardStatus = [](const auto& status) -> bool { return status; };
template <typename DurationType, typename HowToSleep, typename Predicate, typename Callable, typename... Args,
          // figure out what the callable returns
          typename R = std::decay_t<typename std::result_of<Callable(Args...)>::type>,
          // require that Predicate is actually a Predicate
          std::enable_if_t<std::is_convertible<typename std::result_of<Predicate(R)>::type, bool>::value, int> = 0>
R ExponentialBackoff(const unsigned int maxRetryCount, DurationType initialSleepingTime, DurationType maxSleepingTime,
                     HowToSleep&& sleepAction, Predicate&& isRetryable, Callable&& callable, Args&&... args)
{
    auto retryCount{0u};
    while (true)
    {
        const auto status{callable(std::forward<Args>(args)...)};
        if (!isRetryable(status))
            return status;
        if (retryCount >= maxRetryCount)
            return status;
        DurationType delay{
            (initialSleepingTime.count() > 0)
                ? DurationType{std::min(initialSleepingTime.count() << retryCount, maxSleepingTime.count())}
                : DurationType{0}};
        sleepAction(delay);
        ++retryCount;
    }
}
} // namespace Retry

#endif // EXPONENTIAL_BACKOFF_HPP
