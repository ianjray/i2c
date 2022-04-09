#include "i2ctarget.h"

I2CTarget::I2CTarget(int id, I2CBus& bus) : I2CNode{id, bus}
{
}

void I2CTarget::on_bus_changed(I2CBus::State old_state, I2CBus::State new_state)
{
    // Call superclass.
    I2CNode::on_bus_changed(old_state, new_state);

    if (old_state.sda && !new_state.sda && new_state.scl) {
        // START = SDA falling while SCL high.
        on_start_condition();
        return;
    }

    if (!old_state.sda && new_state.sda && new_state.scl) {
        // STOP = SDA rising while SCL high.
        on_stop_condition();
        return;
    }

    if (!old_state.scl && new_state.scl) {
        // SDA is sampled on SCL rising edges.
        on_scl_rising(new_state.sda);
        return;
    }

    if (old_state.scl && !new_state.scl) {
        // A transmitter changes SDA while SCL is low.
        on_scl_falling();
        return;
    }
}
