#pragma once

#include "line.h"

#include <memory>
#include <tuple>


class Node;

/// @brief Bus class.
/// @discussion Models an I²C bus to which nodes are attached.
/// The bus has two lines, data (SDA) and clock (SCL) which are used for communication.
/// Methods are provided to both get and set the current state of those lines.
class Bus
{
    class Impl;
    std::unique_ptr<Impl> pimpl;

public:
    /// @brief Constructor
    Bus();

    /// @brief Destructor
    ~Bus();

    /// @brief Attach a bus node.
    void attach(const Node * node);

    /// @brief Detach a bus node.
    void detach(const Node * node);

    /// @brief Get current bus state.
    /// @return int SCL status
    /// @return int SDA status
    std::tuple<Line::Level, Line::Level> get(const Node * node);

    enum class Event
    {
        DataLow,
        DataHigh,
        ClockLow,
        ClockHigh,
        Delay
    };

    /// @brief Set new bus state.
    void set(const Node * node, Event event);
};
