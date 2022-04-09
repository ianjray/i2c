#include "log.hpp"

#include <iomanip>
#include <iostream>
#include <mutex>

namespace
{

/// Logging level.
Log::Level log_level_;

/// Per-thread prefix.
thread_local std::string prefix_;

/// Mutex to prevent multiple threads writing to std::cout simultaneously.
std::mutex log_mutex_;

} // namespace

void Log::set_level(Level level)
{
    log_level_ = level;
}

void Log::set_prefix(const std::string & prefix)
{
    prefix_ = prefix;
}

std::string Log::octet(int value)
{
    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << std::uppercase << std::hex << value;
    return ss.str();
}

Log::Log(Log::Level level) : level_{level}, ss_{}
{
    if (level >= log_level_) {
        operator<<(prefix_);
        operator<<('\t');
    }
}

Log::~Log()
{
    if (level_ >= log_level_) {
        std::lock_guard<std::mutex> lock(log_mutex_);
        std::cout << ss_.str() << std::endl;
    }
}
