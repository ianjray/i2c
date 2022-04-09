#include "bus.hpp"
#include "controllerbase.hpp"
#include "log.hpp"
#include "target.hpp"

#include "xassert.hpp"

#include <thread>
#include <vector>

namespace
{

constexpr int ADDRESS_SHIFT = 1;
constexpr int READ_OPERATION = 1;

void test_register_read(ControllerBase & controller, uint8_t address)
{
    constexpr int REGISTER = 0xAD;

    LOG_INFO << "[ read address " << Log::octet(address) << " register " << Log::octet(REGISTER) << " (write address, register, restart, read) ]";

    address <<= ADDRESS_SHIFT;

    auto nack = controller.write(address, ControllerBase::WriteFlag::START);
    xassert(!nack);

    nack = controller.write(REGISTER);
    xassert(!nack);

    address |= READ_OPERATION;

    nack = controller.write(address, ControllerBase::WriteFlag::START);
    xassert(!nack);
    auto octet = controller.read();
    xassert(octet == 0x00);
    octet = controller.read();
    xassert(octet == 0x01);
    octet = controller.read();
    xassert(octet == 0x02);
    octet = controller.read(ControllerBase::ReadFlag::NACK|ControllerBase::ReadFlag::STOP);
    xassert(octet == 0x03);
}

void test_write_simple(ControllerBase & controller, uint8_t address)
{
    LOG_INFO << "[ write value to address " << Log::octet(address) << " ]";

    address <<= ADDRESS_SHIFT;

    auto nack = controller.write(address, ControllerBase::WriteFlag::START);
    xassert(!nack);

    nack = controller.write(0x42, ControllerBase::WriteFlag::STOP);
    xassert(!nack);
}

void test_write_multi(ControllerBase & controller, uint8_t address)
{
    LOG_INFO << "[ write multiple values to address " << Log::octet(address) << " ]";

    address <<= ADDRESS_SHIFT;

    auto nack = controller.write(address, ControllerBase::WriteFlag::START);
    xassert(!nack);

    nack = controller.write(0x01);
    xassert(!nack);
    nack = controller.write(0x02);
    xassert(!nack);
    nack = controller.write(0x03, ControllerBase::WriteFlag::STOP);
    xassert(!nack);
}

void test_read_interrupted(ControllerBase & controller, uint8_t address)
{
    LOG_INFO << "[ read address " << Log::octet(address) << " (read, recover) ]";

    address <<= ADDRESS_SHIFT;
    address |= READ_OPERATION;

    auto nack = controller.write(address, ControllerBase::WriteFlag::START);
    xassert(!nack);

    // Read octet is ACK'd and not stopped.  Target prepares to send next octet.
    auto octet = controller.read();
    xassert(octet == 0x20);

    // Recover from interrupted transaction.
    controller.recover();
}

void test_read_with_restart(ControllerBase & controller, uint8_t address)
{
    LOG_INFO << "[ read address " << Log::octet(address) << " (read, nack, restart, read, nack, stop) ]";

    address <<= ADDRESS_SHIFT;
    address |= READ_OPERATION;

    auto nack = controller.write(address, ControllerBase::WriteFlag::START);
    xassert(!nack);

    auto octet = controller.read(ControllerBase::ReadFlag::NACK);
    xassert(octet == 0x10);

    nack = controller.write(address, ControllerBase::WriteFlag::START);
    xassert(!nack);
    octet = controller.read(ControllerBase::ReadFlag::NACK|ControllerBase::ReadFlag::STOP);
    xassert(octet == 0x10);
}

void test_read_nonexistent_target(ControllerBase & controller, uint8_t address)
{
    LOG_INFO << "[ read non-existent address " << Log::octet(address) << " (read, nack, stop) ]";

    address <<= ADDRESS_SHIFT;
    address |= READ_OPERATION;

    auto nack = controller.write(address, ControllerBase::WriteFlag::START);
    xassert(!!nack);

    auto octet = controller.read(ControllerBase::ReadFlag::NACK|ControllerBase::ReadFlag::STOP);
    xassert(octet == 0xFF);
}

void test_read(ControllerBase & controller, uint8_t address, uint8_t expected)
{
    LOG_INFO << "[ read address " << Log::octet(address) << " (read, nask, stop) ]";

    address <<= ADDRESS_SHIFT;
    address |= READ_OPERATION;

    auto nack = controller.write(address, ControllerBase::WriteFlag::START);
    xassert(!nack);

    auto octet = controller.read(ControllerBase::ReadFlag::NACK|ControllerBase::ReadFlag::STOP);
    xassert(octet == expected);
}

void test_write(ControllerBase & controller, uint8_t address)
{
    LOG_INFO << "[ write value to address " << Log::octet(address) << " ]";

    address <<= ADDRESS_SHIFT;

    auto nack = controller.write(address, ControllerBase::WriteFlag::START);
    xassert(!nack);

    nack = controller.write(0x42, ControllerBase::WriteFlag::STOP);
    xassert(!nack);
}

} // namespace

int main()
{
    Log::set_level(Log::Level::Info);

    Bus bus;

    std::vector<std::unique_ptr<Target>> targets{};
    std::vector<std::thread> threads{};

#define N_TARGETS 4

    for (auto i = 0; i < N_TARGETS; ++i) {
        auto address = static_cast<uint8_t>(0x50 + i);
        auto name = "T" + Log::octet(address);

        auto t = std::make_unique<Target>(name, address, &bus);
        auto n = targets.size();
        targets.push_back(std::move(t));

        auto thr = std::thread([&targets, n, name]
        {
            Log::set_prefix(name);

            targets[n]->run();
        });

        threads.push_back(std::move(thr));
    }

    auto name = "C00";
    Log::set_prefix(name);
    ControllerBase controller(name, &bus);

    test_register_read(controller, 0x50);
    test_write_simple(controller, 0x51);
    test_write_multi(controller, 0x52);
    test_read_interrupted(controller, 0x52);
    test_read_with_restart(controller, 0x51);
    test_read_nonexistent_target(controller, 0x20);
    test_read(controller, 0x52, 0x20);
    // Clock stretching.
    test_write(controller, 0x53);
    test_read(controller, 0x53, 0x30);

    for (auto & target : targets) {
        target->stop();
    }

    for (auto & thread : threads) {
        thread.join();
    }
}
