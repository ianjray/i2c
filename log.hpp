#pragma once

#include <sstream>
#include <string>

/// Log class.
/// @discussion Provides logging services.
class Log
{
public:
    /// Log level.
    enum class Level
    {
        Debug,
        Info
    };

#define LOG_DEBUG  Log(Log::Level::Debug)
#define LOG_INFO   Log(Log::Level::Info)

private:
    /// Level of this log message.
    Level level_;

    /// Output stream.
    std::stringstream ss_;

public:
    /// Set global logging level.
    static void set_level(Level level);

    /// Set per-thread prefix.
    static void set_prefix(const std::string & prefix);

    /// Constructor.
    /// @param level Log level.
    Log(Level level);

    Log(const Log &) = delete;

    auto operator=(const Log &) -> Log & = delete;

    Log(Log&& other) = delete;

    auto operator=(Log &&) -> Log & = delete;

    /// Destructor.
    ~Log();

    template<class T>
    Log & operator<<(const T & msg)
    {
        ss_ << msg;
        return *this;
    }

    /// Format an octet as ASCIIHEX.
    /// @return std::string The formatted value.
    static std::string octet(int value);
};
