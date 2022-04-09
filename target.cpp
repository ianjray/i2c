#include "target.hpp"

#include "bus.hpp"
#include "log.hpp"
#include "node.hpp"
#include "targetbase.hpp"

#include <atomic>

class Target::Impl : public TargetBase
{
    std::atomic_bool running_;

public:
    Impl(const std::string & name, uint8_t address, Bus * bus) : TargetBase{name, address, bus}, running_{}
    {
    }

    void stop()
    {
        running_ = false;
    }

    void run()
    {
        running_ = true;
        for (;;) {
            while (sda() == Line::Level::High) {
                if (!running_) {
                    return;
                }
            }
            // Falling edge SDA ▔\▁
            isr();
        }
    }

    void isr()
    {
        if (scl() == Line::Level::Low) {
            return;
        }

        // SCL ▔\▁
        while (scl() == Line::Level::High) {
        }

        LOG_DEBUG << "START";

        auto [result, octet] = read();
        switch (result) {
            case TargetBase::Result::Octet:
                break;
            case TargetBase::Result::Stop:
            case TargetBase::Result::Start:
                return;
        }

        LOG_DEBUG << "rx address=" << Log::octet(octet);

        if (!address_match(octet)) {
            wait_for_condition(WaitFlag::STOP);
            return;
        }

        ack();

        if (read_operation(octet)) {
            handle_controller_read();
        } else {
            handle_controller_write();
        }
    }

    /// Write data in response to a controller read operation.
    /// @discussion First octet is based on our address, and auto increments.
    /// There is no limit to how much data may be read.
    void handle_controller_read()
    {
        for (uint8_t data = static_cast<uint8_t>(address() << 4); ; data++) {
            LOG_INFO << "tx:" << Log::octet(data);
            write(data);

            // SCL ▁/▔
            while (scl() == Line::Level::Low) {
            }

            if (address() == 0xA6) {
                // Drive SCL low for clock stretching *before* sampling SDA.
                // A target might implement this in order to reserve time to prepare the next octet.
                LOG_DEBUG << "tx clock stretch";
                scl(Line::Level::Low);
            }

            auto nack = sda();

            if (address() == 0xA6) {
                scl(Line::Level::Low);
                scl(Line::Level::Low);
                scl(Line::Level::Low);
                LOG_DEBUG << "tx clock stretch end";
                scl(Line::Level::High);
            }

            // SCL ▔\▁
            while (scl() == Line::Level::High) {
            }

            LOG_DEBUG << "nack=" << static_cast<int>(nack);

            if (nack == Line::Level::High) {
                wait_for_condition(WaitFlag::START|WaitFlag::STOP);
                return;
            }
        }
    }

    /// Read data in response to a controller write operation.
    /// @discussion The data is logged and discarded.
    void handle_controller_write()
    {
        for (;;) {
            auto [result, octet] = read();
            switch (result) {
                case TargetBase::Result::Octet:
                    break;
                case TargetBase::Result::Stop:
                case TargetBase::Result::Start:
                    return;
            }

            if (address() == 0xA6) {
                // Drive SCL low for clock stretching *before* driving SDA low for the ACK.
                // (SDA must be valid before controller sees SCL go high.)
                // A target might implement this in order to reserve time to process the request.
                LOG_DEBUG << "rx clock stretch";
                scl(Line::Level::Low);
            }

            // Drive SDA low to acknowledge.
            sda(Line::Level::Low);

            if (address() == 0xA6) {
                scl(Line::Level::Low);
                scl(Line::Level::Low);
                scl(Line::Level::Low);
                LOG_DEBUG << "rx clock stretch end";
                scl(Line::Level::High);
            }

            // Wait for controller to sample SDA.
            wait_for_clock_pulse();

            // Release SDA.
            sda(Line::Level::High);

            LOG_INFO << "rx=" << Log::octet(octet);
        }
    }
};

Target::Target(const std::string & name, uint8_t address, Bus * bus) : pimpl{std::make_unique<Impl>(name, address, bus)}
{
}

Target::~Target() = default;

void Target::run()
{
    pimpl->run();
}

void Target::stop()
{
    pimpl->stop();
}
