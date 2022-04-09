#include "line.hpp"

#include <set>

class Line::Impl
{
    /// Set of connections which are driving the line low.
    std::set<const void *> low_;

public:
    Impl() : low_{}
    {
    }

    Line::Level get() const
    {
        // If *any* node drives the line low it will be low.
        return !low_.empty() ? Line::Level::Low : Line::Level::High;
    }

    void set(const void * connection, Line::Level level)
    {
        switch (level) {
            case Line::Level::Low:
                low_.insert(connection);
                break;
            case Line::Level::High:
                low_.erase(connection);
                break;
        }
    }
};

Line::Line() : pimpl{std::make_unique<Impl>()}
{
}

Line::~Line() = default;

Line::Level Line::get() const
{
    return pimpl->get();
}

void Line::set(const void * connection, Line::Level level)
{
    pimpl->set(connection, level);
}
