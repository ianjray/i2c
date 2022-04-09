#pragma once

#include "bitmask_operators.hpp"
#include "line.hpp"

#include <cstdint>
#include <memory>
#include <string>

class Bus;

/// ControllerBase class.
/// @discussion Models an I²C controller connected to a I²C bus.
/// Methods are provided to read and write octets with flags to allow control of start/stop conditions and acknowledgements.
class ControllerBase
{
    class Impl;
    std::unique_ptr<Impl> pimpl;

public:
    /// Constructor.
    ControllerBase(const std::string & name, Bus * bus);

    /// Destructor.
    ~ControllerBase();

    enum class ReadFlag : unsigned
    {
        NONE,
        /// Do not acknowledge the read octet.
        NACK  = 1 << 0,
        /// Send stop condition.
        STOP  = 1 << 1
    };

    /// Read octet.
    /// @param flags Flags that control behaviour.
    /// @return uint8_t The octet.
    uint8_t read(ReadFlag flags = ReadFlag::NONE);

    enum class WriteFlag : unsigned
    {
        NONE,
        /// Send start condition.
        START = 1 << 0,
        /// Send stop condition.
        STOP  = 1 << 1
    };

    /// Write octet.
    /// @param octet The octet to send.
    /// @param flags Flags that control behaviour.
    /// @return bool True if the octet was not acknowledged by the target.
    bool write(uint8_t octet, WriteFlag flags = WriteFlag::NONE);

    /// Recover bus.
    /// @discussion SDA may be stuck low due to an interrupted transaction.
    /// Pulse SCL in order to complete transaction and release SDA.
    int recover();
};

BITMASK_OPERATORS(ControllerBase::WriteFlag)

BITMASK_OPERATORS(ControllerBase::ReadFlag)
