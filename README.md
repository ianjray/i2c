# i2c

C++ I²C Bus Simulation

Models an I²C bus an I2C bus as a discrete-event, single-threaded simulation.

This project supports a useful sub-set of the full [I²C bus specification](https://www.nxp.com/docs/en/user-guide/UM10204.pdf).

Intended for testing I2C protocol behavior, bus arbitration, clock stretching, event ordering, and node lifecycle without relying on real hardware, kernel facilities, or real-time delays.

## Design Notes

The simulation deliberately avoids real-time behavior and external concurrency.

There are:

- No real-time delays.
- No threads.
- No OS scheduler.
- No wall-clock dependency.
- No hardware access.

Instead, behavior is driven entirely by:

1. Resolved SCL/SDA transitions.
2. Protocol callbacks.
3. Explicit scheduler ticks.

This makes protocol behavior and event ordering deterministic and easy to test.

## Implementation

### Overview

1. A node drives a line.
2. The bus resolves the open-drain state across all attached nodes.
3. If the resolved state changed, a Transition is queued.
4. Queued transitions are dispatched in order; each attached node receives `on_bus_changed()` for the transition.
5. A callback may drive lines (causing new transitions, queued behind the current one) or schedule deferred work via defer().
6. Deferred work executes when the scheduler advances (yield()), which models the passage of simulation time.

### Open-Drain Bus

The bus uses open-drain signaling:

- Each attached node can either release a line or drive it low.
- A line is high only when all attached nodes release it.
- If any attached node drives a line low, the resolved bus value is low.

This is implemented by `I2CBus` and provides the electrical semantics of SCL and SDA.

### Discrete Transitions

`I2CBus` resolves the electrical state whenever a node changes its drive state.

If the resolved state changes, the bus creates a transition containing the old and new states and delivers it to the nodes that were attached when the transition occurred.

Callbacks are invoked after the new bus state has been committed.
A callback may cause another bus transition.
Such transitions are queued and delivered after the current callback returns, preventing recursive callback chains while preserving deterministic event ordering.
This provides deterministic, re-entrant-safe event delivery without requiring multiple threads.

The `old_state` and `new_state` passed to a callback always describe the transition being reported.
The bus may have undergone subsequent transitions by the time the callback runs, so `scl()` and `sda()` may no longer correspond to `new_state`.

### Scheduler

The scheduler is cooperative and single-threaded.

- `defer(owner, ticks, fn)` schedules work after `ticks` simulation ticks; zero delays execute immediately.
- `yield()` advances the simulation by one tick and executes all work that becomes ready on that tick.
- `cancel(owner)` cancels all pending work owned by the given node.

A callback scheduled while `yield()` is running is not executed recursively; it waits until a subsequent call to `yield()`.

The scheduler is intentionally not a general-purpose runtime scheduler.
It exists to model delayed protocol behavior such as clock stretching and other time-dependent bus activity.

## Node Lifecycle

`I2CNode` represents a participant on the bus.

A node can:

- Read the resolved bus state using `scl()` and `sda()`.
- Drive SCL or SDA low, or release either line.
- Defer work through the node API.
- Wait for SCL to become high.
- Participate in bus transition callbacks.

Nodes are attached explicitly with `I2CBus::attach()`.

When an `I2CNode` is destroyed, it automatically detaches itself from the bus.
Pending scheduler work owned by the node is also cancelled as part of node cleanup.

## Protocol Behaviour

The implementation includes:

- `I2CController`: controller-side operations such as START, STOP, reading, and writing octets.
- `I2CTarget`: base class for I2C target devices, including hooks for START, STOP, and SCL transitions.
- `ReferenceTarget`: a simple target implementation used to exercise register-addressed I2C transactions.

The protocol model includes support for:

- START and STOP conditions.
- Repeated START conditions.
- Address ACK/NACK.
- Data ACK/NACK.
- Clock stretching.
- Bus recovery after SDA stuck low.
- Controller timeout on SCL stuck low.
- Multi-octet reads.
- Register-addressed target devices.
- Bus transitions caused from within callbacks.

## Building

```bash
./configure
make test
```

## Requirements

- C++17 or later
- POSIX-compatible system

## Thread Safety

This library is **not thread-safe**.

The bus, nodes, and scheduler are intended to be used from a single thread.
