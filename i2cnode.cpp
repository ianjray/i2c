#include "i2cnode.h"

I2CNode::I2CNode(int id, I2CBus& bus) : bus_{bus}, id_{id}
{
}

I2CNode::~I2CNode()
{
    bus_.detach(*this);
}

void I2CNode::on_bus_changed(I2CBus::State, I2CBus::State)
{
    // To be overridden in sub-class.
}

int I2CNode::id() const
{
    return id_;
}

bool I2CNode::scl() const
{
    return bus_.scl();
}

bool I2CNode::sda() const
{
    return bus_.sda();
}

void I2CNode::scl_low()
{
    bus_.drive(*this, I2CBus::Line::Scl, I2CBus::OpenDrainState::Low);
}

void I2CNode::scl_release()
{
    bus_.drive(*this, I2CBus::Line::Scl, I2CBus::OpenDrainState::Release);
}

void I2CNode::sda_low()
{
    bus_.drive(*this, I2CBus::Line::Sda, I2CBus::OpenDrainState::Low);
}

void I2CNode::sda_release()
{
    bus_.drive(*this, I2CBus::Line::Sda, I2CBus::OpenDrainState::Release);
}

void I2CNode::defer(unsigned ticks, std::function<void()> function)
{
    bus_.defer(*this, ticks, std::move(function));
}

bool I2CNode::wait_scl_high(unsigned timeout)
{
    return bus_.wait_for(timeout, I2CBus::Line::Scl, true);
}
