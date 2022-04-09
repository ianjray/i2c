#pragma once

#include "line.hpp"

/// Node interface class.
/// @discussion Models a node connected to a I²C bus.
/// This is a base class used to implement controller and target nodes.
class NodeInterface
{
public:
    /// Destructor.
    virtual ~NodeInterface() = default;

    /// Get SDA.
    /// @return Line::Level Data line level.
    virtual Line::Level sda() = 0;

    /// Set SDA.
    /// @discussion Set data line to @c level.
    virtual void sda(Line::Level level) = 0;

    /// Get SCL.
    /// @return Line::Level Clock line level.
    virtual Line::Level scl() = 0;

    /// Set SCL.
    /// @discussion Set clock line to @c level.
    virtual void scl(Line::Level level) = 0;
};
