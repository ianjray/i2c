#pragma once

#include "i2cnode.h"

/// Models an I2C controller node.
///
/// Provides protocol functions.
///
class I2CController : public I2CNode
{
public:
    /// Constructor.
    I2CController(int id, I2CBus& bus);

    /// Get and clear the error flag.
    /// The flag is set when a controller operation fails due to a timeout.
    bool error();

    /// Get and clear the arbitration lost flag.
    /// The flag is set when a controller transmits '1' but observes '0'.
    bool lost();

    /// Get timeout.
    unsigned timeout() const;

    /// Set timeout.
    void set_timeout(unsigned ticks);

    /// Create START condition on the bus.
    /// @return True if condition was successfully created.
    bool start_condition();

    /// Create STOP condition on the bus.
    /// @return True if condition was successfully created.
    bool stop_condition();

    /// Write @c octet.
    /// @return True on success.
    bool write_octet(uint8_t octet);

    /// Read octet and acknowledge according to @c ack.
    /// @return Read octet.
    uint8_t read_octet(bool ack);

    /// Attempt to recover the bus from a target holding SDA low.
    /// Generates up to nine SCL clock pulses and then generates a STOP condition.
    /// @return True if SDA was released and the bus was recovered.
    bool recover_bus();

protected:
    /// Number of ticks to wait for a line state.
    /// @see set_timeout.
    unsigned timeout_ticks_;

    /// Indicates that a controller operation failed.
    bool error_;

    /// Indicates that arbitration was lost.
    bool lost_;
};
