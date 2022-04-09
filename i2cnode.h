#pragma once

#include "i2cbus.h"

/// Base class for a node attached to an I2C bus.
///
/// A node may drive SCL and SDA low or release them, and observes resolved bus transitions through @c on_bus_changed().
/// Controllers and targets derive from this class to implement their respective protocol behavior.
///
class I2CNode
{
public:
    /// Constructor.
    I2CNode(int id, I2CBus& bus);

    /// Destructor.
    virtual ~I2CNode();

    /// Called when the resolved bus state changes.
    ///
    /// @param old_state State before the transition.
    /// @param new_state State produced by the transition.
    ///
    /// @note The arguments describe this transition and are not necessarily the current bus state when the callback executes.
    ///        A callback may cause subsequent transitions before other callbacks are invoked.
    virtual void on_bus_changed(I2CBus::State old_state, I2CBus::State new_state);

    /// @return Caller-defined identity.
    int id() const;

    /// Get observed electrical state of SCL.
    /// @return True if HIGH.
    bool scl() const;

    /// Get observed electrical state of SDA.
    /// @return True if HIGH.
    bool sda() const;

    /// Drive SCL low.
    void scl_low();

    /// Release SCL.
    void scl_release();

    /// Drive SDA low.
    void sda_low();

    /// Release SDA.
    void sda_release();

    /// Wait for SCL to go high.
    /// @param ticks Maximum number of scheduler ticks to wait.
    /// @return True if SCL goes high within @c ticks ticks.
    bool wait_scl_high(unsigned ticks);

protected:
    /// Defer @c function by @c ticks.
    /// @param ticks Number of ticks to defer by; zero means run immediately.
    void defer(unsigned ticks, std::function<void()> function);

    /// Reference to the bus.
    I2CBus& bus_;

private:
    /// Identity for logging.
    int id_;
};
