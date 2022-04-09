#pragma once

#include "line.h"


/// @brief Node interface class.
/// @discussion Models a node connected to a I²C bus.
/// This is a base class used to implement controller and target nodes.
class NodeInterface
{
public:
    /// @brief Destructor.
    virtual ~NodeInterface()
    {
    }

    /// @brief Get SDA.
    /// @return Line::Level Data line level.
    virtual Line::Level sda() = 0;

    /// @brief Set SDA.
    /// @discussion Set data line to @c level.
    virtual void sda(Line::Level level) = 0;

    /// @brief Get SCL.
    /// @return Line::Level Clock line level.
    virtual Line::Level scl() = 0;

    /// @brief Set SCL.
    /// @discussion Set clock line to @c level.
    virtual void scl(Line::Level level) = 0;
};
