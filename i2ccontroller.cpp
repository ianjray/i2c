#include "i2ccontroller.h"

static constexpr unsigned DEFAULT_TIMEOUT_TICKS = 10;
static constexpr int BITS_PER_OCTET = 8;
static constexpr int BUS_RECOVERY_CLOCKS = 9;

I2CController::I2CController(int id, I2CBus& bus) : I2CNode{id, bus}, timeout_ticks_{DEFAULT_TIMEOUT_TICKS}, error_{}, lost_{}
{
}

bool I2CController::error()
{
    const bool error = error_;
    error_ = false;
    return error;
}

bool I2CController::lost()
{
    const bool lost = lost_;
    lost_ = false;
    return lost;
}

unsigned I2CController::timeout() const
{
    return timeout_ticks_;
}

void I2CController::set_timeout(unsigned ticks)
{
    timeout_ticks_ = ticks;
}

bool I2CController::start_condition()
{
    // SDA HIGH -> LOW while SCL HIGH.
    // SCL:  ‾‾‾‾‾‾‾\__
    // SDA:  ‾‾‾‾\_____
    sda_release();
    scl_release();
    if (!wait_scl_high(timeout_ticks_)) {
        error_ = true;
        return false;
    }
    sda_low();
    scl_low();
    return true;
}

bool I2CController::stop_condition()
{
    // SDA LOW -> HIGH while SCL HIGH.
    // SCL:  __/‾‾‾‾‾‾‾
    // SDA:  _____/‾‾‾‾
    sda_low();
    scl_release();
    const bool ready = wait_scl_high(timeout_ticks_);
    if (!ready) {
        error_ = true;
    }
    sda_release();
    return ready;
}

bool I2CController::write_octet(uint8_t octet)
{
    for (auto bit = BITS_PER_OCTET - 1; bit >= 0; --bit) {
        scl_low();

        const bool high = octet & (1u << bit);
        if (high) {
            sda_release();
        } else {
            sda_low();
        }

        scl_release();

        // Sample SDA (for detecting arbitration lost) on rising edge of clock.
        if (!wait_scl_high(timeout_ticks_)) {
            error_ = true;
            scl_low();
            return false;
        }

        if (high && !sda()) {
            // Lost arbitration: wanted SDA high, some other controller drove SDA low.
            lost_ = true;
            sda_release();
            scl_release();
            return false;
        }
    }

    scl_low();
    sda_release();
    scl_release();

    // Sample SDA on rising edge of clock.
    if (!wait_scl_high(timeout_ticks_)) {
        error_ = true;
        scl_low();
        return false;
    }

    const bool ack = !sda();
    scl_low();
    return ack;
}

uint8_t I2CController::read_octet(bool ack)
{
    uint8_t octet = 0;

    sda_release();

    for (auto bit = BITS_PER_OCTET - 1; bit >= 0; --bit) {
        scl_low();
        scl_release();

        // Sample SDA on rising edge of clock.
        if (!wait_scl_high(timeout_ticks_)) {
            error_ = true;
            scl_low();
            return 0xFF;
        }

        if (sda()) {
            octet |= 1u << bit;
        }
    }

    scl_low();
    if (ack) {
        sda_low();
    } else {
        sda_release();
    }

    // Target samples acknowledge on rising edge of clock.
    scl_release();

    scl_low();
    sda_release();

    return octet;
}

bool I2CController::recover_bus()
{
    sda_release();

    for (unsigned i = 0; i < BUS_RECOVERY_CLOCKS; ++i) {
        scl_low();
        scl_release();
        if (!wait_scl_high(timeout_ticks_)) {
            return false;
        }

        if (sda()) {
            stop_condition();
            return true;
        }
    }

    return false;
}
