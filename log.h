#pragma once

#include <sstream>
#include <string>


/// @brief Log class.
/// @discussion Provides logging services.
class Log
{
public:
    /// @brief Log level.
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
    /// @brief Set global logging level.
    static void set_level(Level level);

    /// @brief Set per-thread prefix.
    static void set_prefix(const std::string & prefix);

    /// @brief Constructor.
    /// @param level Log level.
    Log(Level level);

    Log(const Log &) = delete;

    auto operator=(const Log &) -> Log & = delete;

    Log(Log&& other) = delete;

    auto operator=(Log &&) -> Log & = delete;

    /// @brief Destructor.
    ~Log();

    template<class T>
    Log & operator<<(const T & msg)
    {
        ss_ << msg;
        return *this;
    }

    /// @brief Format an octet as ASCIIHEX.
    /// @return std::string The formatted value.
    static std::string octet(int value);
};
