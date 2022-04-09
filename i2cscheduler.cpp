#include "i2cscheduler.h"

I2CScheduler::I2CScheduler() : yield_running_{}
{
}

void I2CScheduler::defer(void* owner, unsigned ticks, std::function<void()> function)
{
    if (ticks == 0) {
        function();
        return;
    }

    events_.push_back({
        .owner = owner,
        .ticks = ticks,
        .function = std::move(function),
        .cancelled = false,
    });
}

void I2CScheduler::cancel(void* owner)
{
    if (yield_running_) {
        return;
    }

    for (auto& event : events_) {
        if (event.owner == owner) {
            event.cancelled = true;
        }
    }
}

void I2CScheduler::yield()
{
    if (yield_running_) {
        return;
    }

    yield_running_ = true;

    // Build ready list first to avoid callbacks from mutating scheduler state.
    std::deque<Event> ready;

    for (auto it = events_.begin(); it != events_.end();) {
        if (it->cancelled) {
            it = events_.erase(it);
            continue;
        }

        if (--it->ticks == 0) {
            ready.push_back(std::move(*it));
            it = events_.erase(it);
        } else {
            ++it;
        }
    }

    for (auto& event : ready) {
        event.function();
    }

    yield_running_ = false;
}
