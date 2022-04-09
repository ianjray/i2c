#pragma once

#include <cstdint>

// I2C protocol helpers and constants.
constexpr uint8_t I2C_ADDRESS_SHIFT = 1;
constexpr uint8_t I2C_READ_OPERATION = 1;

inline constexpr uint8_t make_address_rw(uint8_t address, bool read)
{
    return static_cast<uint8_t>((address << I2C_ADDRESS_SHIFT) | (read ? I2C_READ_OPERATION : 0));
}
