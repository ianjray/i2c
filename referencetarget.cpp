#include "referencetarget.h"

#include "i2cprotocol.h"

#include <algorithm>

static constexpr unsigned CLOCK_STRETCH_TICKS = 5;

ReferenceTarget::ReferenceTarget(int id, I2CBus& bus, uint8_t address) : I2CTarget{id, bus},
      address_{address},
      state_{State::Idle},
      read_{},
      shift_{},
      bit_count_{},
      tx_octet_{},
      tx_bit_{LAST_BIT},
      register_address_{}
{
    std::fill(std::begin(registers_), std::end(registers_), DEFAULT_REGISTER_VALUE);

    switch (address) {
    case 0x40:
        registers_[0x00] = 0x12;
        registers_[0x01] = 0x34;
        break;
    case 0x50:
        registers_[0x00] = 0xBA;
        registers_[0x10] = 0xAB;
        registers_[0x11] = 0xCD;
        registers_[0x12] = 0xEF;
        registers_[0xFF] = 0x55;
        break;
    case 0x60:
        registers_[0x00] = 0x11;
        registers_[0x01] = 0x22;
        break;
    }
}

void ReferenceTarget::on_start_condition()
{
    sda_release();

    state_ = State::Address;
    read_ = false;
    shift_ = 0;
    bit_count_ = 0;
}

void ReferenceTarget::on_stop_condition()
{
    sda_release();

    state_ = State::Idle;
}

/// Prepare SDA for the next rising edge.
void ReferenceTarget::on_scl_falling()
{
    switch (state_) {
    case State::AddressAckSetup:
        // SCL is now LOW. It is legal to change SDA.
        // If the target can prepare its SDA immediately, it doesn't need to hold SCL beyond the normal low period.
        state_ = State::AddressAck;

        // Drive SCL low (clock stretching).
        scl_low();

        defer(CLOCK_STRETCH_TICKS, [this](){
            // Prepare SDA when we are ready.
            sda_low();

            // End clock stretching.
            scl_release();
        });
        break;

    case State::AddressAck:
        // This is the falling edge after the ninth clock.
        sda_release();

        if (read_) {
            prepare_transmit();
        } else {
            state_ = State::Register;
        }
        break;

    case State::RegisterAckSetup:
        // Assert acknowledge.
        sda_low();
        state_ = State::RegisterAck;
        break;

    case State::RegisterAck:
        sda_release();
        state_ = State::Register;
        break;

    case State::Transmit:
        transmit_bit();
        break;

    case State::TransmitAck:
        // The eighth data bit has been sampled.
        // Release SDA for the controller's ACK/NACK.
        sda_release();
        break;

    default:
        break;
    }
}

/// Sample SDA on rising edge.
void ReferenceTarget::on_scl_rising(bool sda)
{
    switch (state_) {
    case State::Address:
        receive_bit(sda);
        break;

    case State::AddressAck:
        // Controller samples our ACK here.
        // Nothing needs to happen yet; SDA remains LOW.
        break;

    case State::Register:
        receive_bit(sda);
        break;

    case State::Transmit:
        // The controller has sampled our data.
        if (tx_bit_ == 0) {
            state_ = State::TransmitAck;
        } else {
            --tx_bit_;
        }
        break;

    case State::TransmitAck:
        receive_transmit_ack(sda);
        break;

    default:
        break;
    }
}

void ReferenceTarget::receive_bit(bool bit)
{
    shift_ <<= 1;
    if (bit) {
        shift_ |= 1;
    }

    ++bit_count_;
    if (bit_count_ != BITS_PER_OCTET) {
        return;
    }

    bit_count_ = 0;

    switch (state_) {
    case State::Address:
        receive_address();
        break;

    case State::Register:
        receive_register_address();
        break;

    default:
        break; //UNREACHABLE
    }
}

void ReferenceTarget::receive_address()
{
    const uint8_t address = shift_ >> I2C_ADDRESS_SHIFT;

    read_ = (shift_ & I2C_READ_OPERATION) != 0;
    shift_ = 0;

    if (address != address_) {
        state_ = State::Idle;
        return;
    }

    state_ = State::AddressAckSetup;
}

void ReferenceTarget::receive_register_address()
{
    register_address_ = shift_;
    shift_ = 0;

    // Wait for SCL to fall before asserting ACK.
    state_ = State::RegisterAckSetup;
}

void ReferenceTarget::prepare_transmit()
{
    tx_octet_ = registers_[register_address_];
    tx_bit_ = LAST_BIT;
    state_ = State::Transmit;

    // SDA must be valid during the LOW period before the first transmitted bit is sampled.
    transmit_bit();
}

/// @note A transmitted bit is not complete when SDA is prepared; it is complete when the subsequent SCL rising edge samples it.
void ReferenceTarget::transmit_bit()
{
    if ((tx_octet_ & (1u << tx_bit_)) != 0) {
        sda_release();
    } else {
        sda_low();
    }
}

void ReferenceTarget::receive_transmit_ack(bool sda)
{
    if (sda) {
        // NACK: controller finished.
        state_ = State::Idle;
        sda_release();
        return;
    }

    // Select the next register (with wrap-around).
    register_address_++;

    // SDA will be prepared on the next SCL falling edge.
    tx_octet_ = registers_[register_address_];
    tx_bit_ = LAST_BIT;
    state_ = State::Transmit;
}
