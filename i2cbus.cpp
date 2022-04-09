#include "i2cbus.h"

#include "i2cnode.h"
#include "i2cscheduler.h"

#include <utility>

I2CBus::I2CBus(I2CScheduler& scheduler) : state_{true, true}, dispatching_{}, scheduler_{scheduler}
{
}

bool I2CBus::scl() const
{
    return state_.scl;
}

bool I2CBus::sda() const
{
    return state_.sda;
}

void I2CBus::attach(I2CNode& node)
{
    nodes_.emplace(
        &node,
        DriverState{
            .scl = OpenDrainState::Release,
            .sda = OpenDrainState::Release,
        });
}

bool I2CBus::detach(I2CNode& node)
{
    const auto it = nodes_.find(&node);
    if (it == nodes_.end()) {
        return false;
    }

    nodes_.erase(it);

    scheduler_.cancel(&node);

    // Removing a node can release SCL or SDA.
    resolve();
    return true;
}

bool I2CBus::drive(I2CNode& node, Line line, OpenDrainState state)
{
    const auto it = nodes_.find(&node);
    if (it == nodes_.end()) {
        return false;
    }

    it->second.set(line, state);

    resolve();
    return true;
}

void I2CBus::defer(I2CNode& node, unsigned ticks, std::function<void()> function)
{
    scheduler_.defer(&node, ticks, function);
}

bool I2CBus::wait_for(unsigned ticks, Line line, bool level)
{
    while (state_.get(line) != level) {
        if (ticks == 0) {
            return false;
        }

        scheduler_.yield();
        ticks--;
    }

    return true;
}

void I2CBus::resolve()
{
    const State old_state{state_};

    State new_state{
        .scl = true,
        .sda = true,
    };

    // Open-drain resolution: if any node is driving low, then the signal is low.
    for (const auto& [node, driver] : nodes_) {
        if (driver.scl == OpenDrainState::Low) {
            new_state.scl = false;
        }

        if (driver.sda == OpenDrainState::Low) {
            new_state.sda = false;
        }
    }

    if (old_state == new_state) {
        // No change.
        return;
    }

    Transition transition{
        .old_state = old_state,
        .new_state = new_state,
    };

    state_ = new_state;

    // Capture the nodes which are connected at the moment the physical transition occurs.
    transition.nodes.reserve(nodes_.size());

    for (const auto& [node, driver] : nodes_) {
        transition.nodes.push_back(node);
    }

    pending_.push_back(std::move(transition));

    // Avoid recursion.
    if (dispatching_) {
        return;
    }

    dispatching_ = true;

    while (!pending_.empty()) {
        auto change = std::move(pending_.front());
        pending_.pop_front();

        for (auto node : change.nodes) {
            node->on_bus_changed(change.old_state, change.new_state);
        }
    }

    dispatching_ = false;
}
