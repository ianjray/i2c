#pragma once

#include <memory>


/// @brief Line class.
/// @discussion Models an I²C bus line.
/// Lines are by default high (pull-up).
/// Lines are low while one or more nodes (controllers or targets) drive the line low.
class Line
{
    class Impl;
    std::unique_ptr<Impl> pimpl;

public:
    enum class Level
    {
        Low,
        High
    };

    /// @brief Constructor.
    /// @discussion The line is constructed with initially high level.
    Line();

    /// @brief Destructor.
    ~Line();

    /// @return Level Line level.
    Level get() const;

    /// @brief Set line level.
    void set(const void * connection, Level level);
};
