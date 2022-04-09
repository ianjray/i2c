#include "targetbase.hpp"

#include "bus.hpp"
#include "log.hpp"
#include "node.hpp"

class TargetBase::Impl : public Node
{
    /// Bus address (7-bit).
    uint8_t address_;

public:
    Impl(const std::string & name, uint8_t address, Bus * bus) : Node{name, bus}, address_{address}
    {
    }

    uint8_t address() const
    {
        return address_;
    }

    bool address_match(uint8_t octet) const
    {
        auto address_bits = octet >> 1;
        return address_bits == address();
    }

    std::tuple<Result, uint8_t> read()
    {
        LOG_DEBUG << "read";

        uint8_t octet{};

        for (int shift = 7; shift >= 0; shift--) {
            // SCL ▁/▔
            while (scl() == Line::Level::Low) {
            }

            auto level = sda();

            octet <<= 1;
            if (level == Line::Level::High) {
                octet |= 1;
            }

            while (scl() == Line::Level::High) {
                if (level == Line::Level::Low && sda() == Line::Level::High) {
                    // SCL ▁/▔▔▔
                    // SDA ▁▁▁/▔
                    LOG_DEBUG << "read=STOP";
                    return {Result::Stop, {}};
                } else if (level == Line::Level::High && sda() == Line::Level::Low) {
                    // SCL ▁/▔▔▔
                    // SDA ▔▔▔\▁
                    LOG_DEBUG << "read=START";
                    return {Result::Start, {}};
                }
            }
        }

        LOG_DEBUG << "read:" << Log::octet(octet);
        return {Result::Octet, octet};
    }

    void ack()
    {
        // Drive SDA low to acknowledge.
        sda(Line::Level::Low);
        // Wait for controller to sample SDA.
        wait_for_clock_pulse();
        // Release SDA.
        sda(Line::Level::High);
    }

    void write(uint8_t octet)
    {
        LOG_DEBUG << "write:" << Log::octet(octet);

        for (int shift = 7; shift >= 0; shift--) {
            auto level = (octet & 0x80) != 0 ? Line::Level::High : Line::Level::Low;
            sda(level);
            octet <<= 1;

            wait_for_clock_pulse();
        }

        sda(Line::Level::High);

        LOG_DEBUG << "written";
    }

    void wait_for_clock_pulse()
    {
        while (scl() == Line::Level::Low) {
        }

        while (scl() == Line::Level::High) {
        }
    }

    TargetBase::Condition wait_for_condition(TargetBase::WaitFlag flags)
    {
        LOG_DEBUG << "wait_for_condition";

        for (;;) {
            auto level = sda();

            while (scl() == Line::Level::Low) {
                level = sda();
            }

            while (scl() == Line::Level::High) {
                if (level == Line::Level::Low && sda() == Line::Level::High) {
                    // SCL ▁/▔▔▔
                    // SDA ▁▁▁/▔
                    LOG_DEBUG << "wait_for_condition=STOP";
                    return TargetBase::Condition::STOP;

                } else if (flags & TargetBase::WaitFlag::START) {
                    if (level == Line::Level::High && sda() == Line::Level::Low) {
                        // SCL ▁/▔▔▔
                        // SDA ▔▔▔\▁
                        LOG_DEBUG << "wait_for_condition=START";
                        return TargetBase::Condition::START;
                    }
                }
            }
        }
    }
};

TargetBase::TargetBase(const std::string & name, uint8_t address, Bus * bus) : pimpl{std::make_unique<Impl>(name, address, bus)}
{
}

TargetBase::~TargetBase() = default;

uint8_t TargetBase::address() const
{
    return pimpl->address();
}

bool TargetBase::address_match(uint8_t octet) const
{
    return pimpl->address_match(octet);
}

bool TargetBase::read_operation(uint8_t octet) const
{
    return (octet & 0x01) != 0;
}

void TargetBase::ack()
{
    pimpl->ack();
}

std::tuple<TargetBase::Result, uint8_t> TargetBase::read()
{
    return pimpl->read();
}

void TargetBase::write(uint8_t octet)
{
    pimpl->write(octet);
}

void TargetBase::wait_for_clock_pulse()
{
    pimpl->wait_for_clock_pulse();
}

TargetBase::Condition TargetBase::wait_for_condition(TargetBase::WaitFlag flags)
{
    return pimpl->wait_for_condition(flags);
}

Line::Level TargetBase::sda()
{
    return pimpl->sda();
}

void TargetBase::sda(Line::Level level)
{
    pimpl->sda(level);
}

Line::Level TargetBase::scl()
{
    return pimpl->scl();
}

void TargetBase::scl(Line::Level level)
{
    pimpl->scl(level);
}

