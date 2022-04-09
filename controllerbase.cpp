#include "controllerbase.hpp"

#include "bus.hpp"
#include "log.hpp"
#include "node.hpp"

class ControllerBase::Impl : public Node
{
    bool started_;

    void clock_stretching()
    {
        while (scl() == Line::Level::Low) {
            //TODO: timeout
            LOG_DEBUG << "clock stretched";
        }
    }

    /// Write I²C START condition.
    /// @discussion A start condition is signalled by SDA being pulled low while SCL stays high.
    void write_start_condition()
    {
        if (started_) {
            LOG_DEBUG << "restart";

            sda(Line::Level::High);
            delay();
            scl(Line::Level::High);
            clock_stretching();
            delay();
        }

        LOG_DEBUG << "start";

        sda(Line::Level::Low);
        delay();
        scl(Line::Level::Low);
        started_ = true;

        LOG_DEBUG << "started";
    }

    /// Write I²C STOP condition.
    /// @discussion A stop condition is signalled when SCL goes high, then SDA goes high.
    void write_stop_condition()
    {
        LOG_DEBUG << "stop";

        sda(Line::Level::Low);
        delay();
        scl(Line::Level::High);
        clock_stretching();
        delay();
        sda(Line::Level::High);
        delay();
        started_ = false;

        LOG_DEBUG << "stopped";
    }

    /// Write a bit.
    /// @discussion Drive SDA, then pulse SCL.
    /// Other bus nodes sample SDA while SCL is high.
    void write_bit(Line::Level bit)
    {
        LOG_DEBUG << "write bit:" << static_cast<int>(bit);

        sda(bit);
        delay();
        scl(Line::Level::High);
        delay();
        clock_stretching();
        scl(Line::Level::Low);

        LOG_DEBUG << "written";
    }

    /// Read a bit.
    /// @discussion Pulse SCL, sampling SDA while SCL is high.
    Line::Level read_bit()
    {
        LOG_DEBUG << "read bit";

        sda(Line::Level::High);
        delay();
        scl(Line::Level::High);
        clock_stretching();
        delay();
        auto bit = sda();
        scl(Line::Level::Low);

        LOG_DEBUG << "read bit=" << static_cast<int>(bit);
        return bit;
    }

public:
    Impl(const std::string & name, Bus * bus) : Node{name, bus}, started_{}
    {
    }

    uint8_t read(ReadFlag flags)
    {
        LOG_DEBUG << "read";

        uint8_t octet{};
        for (int bit = 0; bit < 8; ++bit) {
            octet <<= 1;
            if (read_bit() == Line::Level::High) {
                octet |= 1;
            }
        }

        // Default to acknowledge.
        auto nack = Line::Level::Low;

        if (flags & ReadFlag::NACK) {
            nack = Line::Level::High;
        }

        LOG_DEBUG << "nack:" << static_cast<int>(nack);
        write_bit(nack);

        if (flags & ReadFlag::STOP) {
            write_stop_condition();
        }

        LOG_DEBUG << "read=" << Log::octet(octet);
        return octet;
    }

    bool write(uint8_t octet, WriteFlag flags)
    {
        LOG_DEBUG << "write octet:" << Log::octet(octet);

        if (flags & WriteFlag::START) {
            write_start_condition();
        }

        for (auto bit = 0; bit < 8; ++bit) {
            auto level = (octet & 0x80) != 0 ? Line::Level::High : Line::Level::Low;
            write_bit(level);
            octet <<= 1;
        }

        auto nack_bit = read_bit();
        LOG_DEBUG << "nack=" << static_cast<int>(nack_bit);

        if (flags & WriteFlag::STOP) {
            write_stop_condition();
        }

        LOG_DEBUG << "written";
        return nack_bit == Line::Level::High;
    }

    int recover()
    {
        LOG_DEBUG << "recover";

        scl(Line::Level::Low);
        delay();

        int counter{};
        for (;;) {
            // Pulse SCL until we get 'NUM_SAMPLES' of SDA HIGH samples.
            constexpr auto NUM_SAMPLES = 9;
            auto level = read_bit();

            if (level == Line::Level::High) {
                if (++counter == NUM_SAMPLES) {
                    write_stop_condition();
                    break;
                }
            } else {
                // Non-HIGH sample.
                counter = 0;
            }

            LOG_DEBUG << "recover=" << counter;
        }

        LOG_DEBUG << "recovered";
        return 0;
    }
};

ControllerBase::ControllerBase(const std::string & name, Bus * bus) : pimpl{std::make_unique<Impl>(name, bus)}
{
}

ControllerBase::~ControllerBase() = default;

uint8_t ControllerBase::read(ReadFlag flags)
{
    return pimpl->read(flags);
}

bool ControllerBase::write(uint8_t octet, WriteFlag flags)
{
    return pimpl->write(octet, flags);
}

int ControllerBase::recover()
{
    return pimpl->recover();
}
