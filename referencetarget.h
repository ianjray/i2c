#pragma once

#include "i2ctarget.h"

class ReferenceTarget : public I2CTarget
{
public:
    ReferenceTarget(int id, I2CBus& bus, uint8_t address);

protected:
    /// @see I2CTarget::on_start_condition.
    void on_start_condition() override;

    /// @see I2CTarget::on_stop_condition.
    void on_stop_condition() override;

    /// @see I2CTarget::on_scl_falling.
    void on_scl_falling() override;

    /// @see I2CTarget::on_scl_rising.
    void on_scl_rising(bool sda) override;

private:
    enum class State {
        Idle,
        Address,
        AddressAckSetup,
        AddressAck,
        Register,
        RegisterAckSetup,
        RegisterAck,
        Transmit,
        TransmitAck
    };

    void receive_bit(bool bit);

    void receive_address();

    void receive_register_address();

    void prepare_transmit();

    void transmit_bit();

    void receive_transmit_ack(bool sda);

    static constexpr unsigned BITS_PER_OCTET = 8;
    static constexpr unsigned LAST_BIT = BITS_PER_OCTET - 1;
    static constexpr size_t REGISTER_COUNT = UINT8_MAX + 1u;
    static constexpr uint8_t DEFAULT_REGISTER_VALUE = UINT8_MAX;

    uint8_t address_;

    State state_;

    bool read_;
    uint8_t shift_;
    unsigned bit_count_;

    uint8_t tx_octet_;
    unsigned tx_bit_;

    uint8_t register_address_;
    uint8_t registers_[REGISTER_COUNT];
};
