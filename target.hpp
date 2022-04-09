#pragma once

#include <cstdint>
#include <memory>
#include <string>

class Bus;

/// Target class.
/// @discussion Models a generic I²C target having a 7-bit address on the I²C bus.
/// The private implementation inherits from class @c Node which provides methods to interact with the bus.
/// This example target accepts read and write operations.
class Target
{
    class Impl;
    std::unique_ptr<Impl> pimpl;

public:
    /// Constructor.
    /// @param name The name of the target.
    /// @param address The 7-bit bus address of the target.
    /// @param bus The bus to connect to.
    Target(const std::string & name, uint8_t address, Bus * bus);

    /// Destructor.
    ~Target();

    /// Runs the "main loop".
    /// @discussion This method must be called from a unique thread.
    void run();

    /// Stop the "main loop".
    void stop();
};
