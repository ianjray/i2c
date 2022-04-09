#pragma once

#include "i2cbus.h"
#include "i2cnode.h"

/// Models an I2C target node.
///
/// Provides protocol-level notifications for SCL edges and START/STOP conditions.
/// Derived classes implement the target's protocol behavior.
///
class I2CTarget : public I2CNode
{
public:
    /// Constructor.
    I2CTarget(int id, I2CBus& bus);

    /// Notification of a resolved bus state change.
    /// @see I2CBus.
    ///
    /// Detects SCL edges and START/STOP conditions and dispatches them to the corresponding protocol callbacks.
    /// @see on_scl_rising.
    /// @see on_scl_falling.
    /// @see on_start_condition.
    /// @see on_stop_condition.
    virtual void on_bus_changed(I2CBus::State old_state, I2CBus::State new_state);

protected:
    /// Notification of an SCL rising edge, with the resolved SDA level.
    virtual void on_scl_rising(bool sda) = 0;

    /// Notification of an SCL falling edge.
    /// A target changes SDA only during the SCL low period, in preparation for the next rising edge.
    /// START and STOP conditions are exceptions, as they change SDA while SCL is high.
    virtual void on_scl_falling() = 0;

    /// Notification of an I2C START condition.
    virtual void on_start_condition() = 0;

    /// Notification of an I2C STOP condition.
    virtual void on_stop_condition() = 0;
};
