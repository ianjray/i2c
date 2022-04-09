#pragma once

#include "bitmask_operators.h"
#include "nodeinterface.h"

#include <memory>
#include <string>


class Bus;

/// @brief Target base class.
/// @discussion Models an I²C target at an address on the I²C bus.
class TargetBase : public NodeInterface
{
    class Impl;
    std::unique_ptr<Impl> pimpl;

public:
    /// @brief Constructor.
    /// @param name The name of the target.
    /// @param address The 7-bit bus address of the target.
    /// @param bus The bus to connect to.
    TargetBase(const std::string & name, uint8_t address, Bus * bus);

    /// @brief Destructor.
    ~TargetBase() override;

    /// @return uint8_t I²C bus address of node.
    uint8_t address() const;

    /// @return bool True if the address portion of the first octet matches.
    bool address_match(uint8_t octet) const;

    /// @return bool True if the R/W' bit in the first octet indicates a read operation.
    bool read_operation(uint8_t octet) const;

    enum class Result
    {
        Octet,
        Stop,
        Start
    };

    /// @brief Read octet.
    /// @discussion For each bit (from MSB to LSB) this function samples SDA when the
    /// controller has driven SCL high.
    /// This function samples SDA while SCL is high, in order to detect a STOP or RESTART condition.
    /// @return Result Result.
    /// @return uint8_t Octet.
    std::tuple<Result, uint8_t> read();

    /// @brief Acknowledge.
    /// @discussion Drive SDA low to acknowledge an octet written by the controller.
    /// Wait for controller to sample SDA (by detecting a clock pulse), then drive SDA high again.
    void ack();

    /// @brief Write octet.
    /// @param octet The octet to send.
    /// @discussion For each bit (from MSB to LSB) this function drives SDA appropriately,
    /// and awaits a clock pulse from controller (which indicates that the bit was read).
    void write(uint8_t octet);

    /// @brief Await clock pulse.
    /// @discussion Wait for SCL low->high->low ▁/▔\▁ pulse.
    void wait_for_clock_pulse();

    enum class WaitFlag : unsigned
    {
        STOP,
        /// Wait for start condition.
        START = 1 << 0
    };

    enum class Condition
    {
        STOP,
        START
    };

    /// @brief Detect STOP condition.
    /// @discussion The STOP condition is defined as SCL HIGH then SDA going HIGH.
    /// SDA only changes when SCL is high for START and STOP conditions.
    Condition wait_for_condition(WaitFlag flags);

    /// @brief Get SDA.
    /// @return int Data line level.
    Line::Level sda() override;

    /// @brief Set SDA.
    /// @discussion Set data line to @c level.
    void sda(Line::Level level) override;

    /// @brief Get SCL.
    /// @return int Clock line level.
    Line::Level scl() override;

    /// @brief Set SCL.
    /// @discussion Set clock line to @c level.
    void scl(Line::Level level) override;
};

BITMASK_OPERATORS(TargetBase::WaitFlag)
