#pragma once

#include "nodeinterface.h"

#include <memory>
#include <string>


class Bus;

/// @brief Node class.
/// @discussion Models a node connected to a I²C bus.
/// This is a base class used to implement controller and target nodes.
/// The I²C bus is specified at https://www.nxp.com/docs/en/user-guide/UM10204.pdf
class Node : public NodeInterface
{
    class Impl;
    std::unique_ptr<Impl> pimpl;

public:
    /// @brief Constructor
    /// @param name The name of the target.
    /// @param bus The bus to connect to.
    Node(const std::string & name, Bus * bus);

    /// @brief Destructor
    ~Node() override;

    /// @return std::string Node name.
    std::string name() const;

    /// @brief Get SDA.
    /// @return Line::Level Data line level.
    Line::Level sda() override;

    /// @brief Set SDA.
    /// @discussion Set data line to @c level.
    void sda(Line::Level level) override;

    /// @brief Get SCL.
    /// @return Line::Level Clock line level.
    Line::Level scl() override;

    /// @brief Set SCL.
    /// @discussion Set clock line to @c level.
    void scl(Line::Level level) override;

    /// @brief Delay.
    /// @discussion Delay to allow changes to SDA and SCL to propogate to other nodes.
    void delay();
};
