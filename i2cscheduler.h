#pragma once

#include <deque>
#include <functional>

/// A cooperative, single-threaded scheduler.
///
class I2CScheduler
{
public:
    /// Constructor.
    I2CScheduler();

    /// Defer work @c function for @c ticks.
    /// The caller of this function returns control to the main thread, and then simulation code must call @c yield.
    /// @param owner Work is queued according to this parameter.
    /// @param ticks Number of ticks to defer for, zero means run immediately.
    /// @see yield.
    void defer(void* owner, unsigned ticks, std::function<void()> function);

    /// Cancel all pending tasks associated with @c owner.
    /// @see defer.
    /// @note Ignored if called during @c yield.
    void cancel(void* owner);

    /// Advance the simulation clock by one tick.
    /// Executes all work that becomes ready.
    /// Work deferred by a callback is not executed until a subsequent tick.
    /// @note Recursive calls are ignored.
    void yield();

private:
    struct Event {
        void* owner;
        unsigned ticks;
        std::function<void()> function;
        bool cancelled;
    };

    std::deque<Event> events_;

    bool yield_running_;
};
