#include "node.hpp"

#include "bus.hpp"
#include "log.hpp"

class Node::Impl
{
    /// Back-pointer to parent.
    /// @discussion Objects of type @c Node are passed across the bus interface in order to identify the node making the request.
    Node * parent_;

    /// Node name.
    std::string name_;

    /// Bus that the node is connected to.
    Bus * bus_;

public:
    Impl(Node * node, const std::string & name, Bus * bus) : parent_{node}, name_{name}, bus_{bus}
    {
        bus_->attach(parent_);
    }

    virtual ~Impl()
    {
        bus_->detach(parent_);
    }

    std::string name() const
    {
        return name_;
    }

    Line::Level sda()
    {
        auto [sda, _] = bus_->get(parent_);
        return sda;
    }

    void sda(Line::Level level)
    {
        switch (level) {
            case Line::Level::Low:
                bus_->set(parent_, Bus::Event::DataLow);
                break;
            case Line::Level::High:
                bus_->set(parent_, Bus::Event::DataHigh);
                break;
        }
    }

    Line::Level scl()
    {
        auto [_, scl] = bus_->get(parent_);
        return scl;
    }

    void scl(Line::Level level)
    {
        switch (level) {
            case Line::Level::Low:
                bus_->set(parent_, Bus::Event::ClockLow);
                break;
            case Line::Level::High:
                bus_->set(parent_, Bus::Event::ClockHigh);
                break;
        }
    }

    void delay()
    {
        bus_->set(parent_, Bus::Event::Delay);
    }
};

Node::Node(const std::string & name, Bus * bus) : pimpl{std::make_unique<Impl>(this, name, bus)}
{
}

Node::~Node() = default;

std::string Node::name() const
{
    return pimpl->name();
}

Line::Level Node::sda()
{
    return pimpl->sda();
}

void Node::sda(Line::Level level)
{
    pimpl->sda(level);
}

Line::Level Node::scl()
{
    return pimpl->scl();
}

void Node::scl(Line::Level level)
{
    pimpl->scl(level);
}

void Node::delay()
{
    pimpl->delay();
}
