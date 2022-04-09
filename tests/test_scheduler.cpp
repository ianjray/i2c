#include "i2cscheduler.h"

#include <cassert>
#include <iostream>

void test_defer_immediate()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    bool run{};

    scheduler.defer(nullptr, 0, [&] {
        run = true;
    });

    assert(run);
}

void test_defer_yield()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    bool run{};
    constexpr unsigned ticks = 3;

    scheduler.defer(nullptr, ticks, [&]() {
        run = true;
    });

    for (unsigned i = 0; i < ticks; ++i) {
        assert(!run);
        scheduler.yield();
    }

    assert(run);
}

void test_yield_non_reentrant()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    unsigned count{};

    scheduler.defer(nullptr, 1, [&] {
        ++count;
        scheduler.yield();
        ++count;
    });

    scheduler.yield();

    assert(count == 2);
}

void test_cancel()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    bool run{};
    constexpr unsigned ticks = 3;

    scheduler.defer(nullptr, ticks, [&]() {
        run = true;
    });

    scheduler.cancel(nullptr);

    for (unsigned i = 0; i < ticks; ++i) {
        scheduler.yield();
    }

    assert(!run);
}

void test_cancel_during_yield_ignored_now()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    void* a = reinterpret_cast<void*>(1);
    void* b = reinterpret_cast<void*>(2);
    unsigned a_count{};
    unsigned b_count{};

    scheduler.defer(a, 1, [&] {
        a_count++;
        scheduler.cancel(b);
    });

    scheduler.defer(b, 1, [&] {
        b_count++;
    });

    scheduler.yield();
    assert(a_count == 1);
    assert(b_count == 1);
}

void test_cancel_during_yield_ignored_later()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    void* a = reinterpret_cast<void*>(1);
    void* b = reinterpret_cast<void*>(2);
    unsigned a_count{};
    unsigned b_count{};

    scheduler.defer(a, 1, [&] {
        a_count++;
        scheduler.cancel(b);
    });

    scheduler.defer(b, 2, [&] {
        b_count++;
    });

    scheduler.yield();
    assert(a_count == 1);
    assert(b_count == 0);

    scheduler.yield();
    assert(a_count == 1);
    assert(b_count == 1);

    scheduler.yield();
    assert(a_count == 1);
    assert(b_count == 1);
}

int main()
{
    test_defer_immediate();
    test_defer_yield();
    test_yield_non_reentrant();
    test_cancel();
    test_cancel_during_yield_ignored_now();
    test_cancel_during_yield_ignored_later();
}
