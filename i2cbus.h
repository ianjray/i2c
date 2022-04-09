#pragma once

#include <cstddef>
#include <deque>
#include <unordered_map>
#include <vector>

class I2CNode;
class I2CScheduler;

/// Models an I2C bus with clock (SCL) and data (SDA) lines.
///
/// Both lines use open-drain signaling: any attached node may drive a line low, and the line is high only when all attached nodes release it.
/// The electrical state may be observed with getters @c scl() and @c sda().
///
/// The bus is a discrete-event model with no notion of physical time.
/// It processes a sequence of atomic resolved-state transitions, and each transition is observed by every attached node.
///
/// Changes made by nodes via @c control_scl() and @c control_sda() may cause a transition in the resolved bus state.
/// If the resolved line state changes, the transition is delivered to all nodes attached when that transition occurred via their @c I2CNode::on_bus_changed() callback.
/// A callback may cause further bus transitions.
/// Such transitions are queued and are dispatched only after all observers of the current transition have been notified.
///
/// @note The bus state may have advanced beyond the transition being delivered when a callback executes.
/// In particular, @c scl() and @c sda() report the current resolved bus state, not necessarily @c new_state from the callback.
///
/// @note I2CBus is not thread-safe; all bus operations occur on the calling thread.
///
class I2CBus
{
public:
    /// Line selection.
    enum class Line {
        Scl,
        Sda,
    };

    enum class OpenDrainState {
        /// Do not drive the line.
        /// The line may be pulled high by the bus pull-up resistor unless another node is pulling it low.
        Release,
        /// Actively pull the line low.
        Low
    };

    /// Models the electrical bus state.
    struct State {
        /// Clock.
        /// True if HIGH.
        bool scl;
        /// Data.
        /// True if HIGH.
        bool sda;

        /// @return @c line state.
        bool get(Line line) const
        {
            if (line == Line::Scl) {
                return scl;
            } else {
                return sda;
            }
        }

        bool operator==(const State& other) const
        {
            return scl == other.scl && sda == other.sda;
        }

        bool operator!=(const State& other) const
        {
            return !(*this == other);
        }
    };

    /// Constructor.
    I2CBus(I2CScheduler&);

    /// Get electrical state of clock line.
    bool scl() const;

    /// Get electrical state of data line.
    bool sda() const;

    /// Attach @c node to the bus.
    void attach(I2CNode& node);

    /// Detach @c node from the bus.
    /// @return True on success.
    /// @return False if @c node is unknown.
    bool detach(I2CNode& node);

    /// Drive an open-drain bus line low or release it.
    /// The node's requested drive state is updated and the resulting bus state is resolved.
    /// If the resolved line state changes, the change is dispatched to the connected nodes.
    /// @return True on success.
    /// @return False if @c node is unknown.
    bool drive(I2CNode&, Line, OpenDrainState);

    /// Defer @c function by @c ticks.
    /// @param ticks Number of ticks to defer by; zero means run immediately.
    /// @note @c wait_for must be called in order to advance the simulation clock.
    void defer(I2CNode&, unsigned ticks, std::function<void()> function);

    /// Wait for @c line to reach @c level.
    /// @param ticks Maximum number of scheduler ticks to wait.
    /// @return True if @c line reaches @c level within @c ticks ticks.
    bool wait_for(unsigned ticks, Line line, bool level);

private:
    /// Models the state of the node's two drivers.
    struct DriverState {
        /// Clock.
        OpenDrainState scl;
        /// Data.
        OpenDrainState sda;

        void set(Line line, OpenDrainState state)
        {
            if (line == Line::Scl) {
                scl = state;
            } else {
                sda = state;
            }
        }
    };

    /// Models a bus state transition, and the nodes affected by it.
    struct Transition {
        State old_state;
        State new_state;
        std::vector<I2CNode*> nodes;
    };

    /// Resolve and dispatch pending bus transitions.
    /// A callback may modify the bus and thereby enqueue another transition.
    /// A transition caused by a callback is not dispatched recursively; it is appended to pending_ and processed after the current transition has been delivered to all of its observers.
    void resolve();

    /// The electrical state of the bus.
    State state_;

    std::unordered_map<I2CNode*, DriverState> nodes_;

    std::deque<Transition> pending_;

    /// True while dispatch() is draining pending_.
    /// Prevents bus callbacks from recursively entering dispatch().
    bool dispatching_;

    I2CScheduler& scheduler_;
};
