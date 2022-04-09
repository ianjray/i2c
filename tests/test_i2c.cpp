#include "i2cbus.h"
#include "i2ccontroller.h"
#include "i2cnode.h"
#include "i2cprotocol.h"
#include "i2cscheduler.h"
#include "i2ctarget.h"
#include "referencetarget.h"

#include <cassert>
#include <iostream>

void test_open_drain_independence()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    I2CNode n{1, bus};
    bus.attach(n);

    n.scl_low();
    assert(!bus.scl());
    assert(bus.sda());

    n.sda_low();
    assert(!bus.scl());
    assert(!bus.sda());

    n.scl_release();
    assert(bus.scl());
    assert(!bus.sda());

    n.sda_release();
    assert(bus.scl());
    assert(bus.sda());
}

class RecordingNode : public I2CNode
{
public:
    RecordingNode(int id, I2CBus& bus) : I2CNode{id, bus} {}

    struct Event {
        I2CBus::State old_state;
        I2CBus::State new_state;
    };

    std::vector<Event> events;

    void on_bus_changed(I2CBus::State old_state, I2CBus::State new_state) override
    {
        assert(old_state != new_state);
        events.push_back({old_state, new_state});
    }
};

void assert_event(const RecordingNode& node, size_t index, I2CBus::State old_state, I2CBus::State new_state)
{
    assert(node.events.size() > index);
    assert(node.events[index].old_state == old_state);
    assert(node.events[index].new_state == new_state);
}

void test_transition_notification()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    RecordingNode a{1, bus};
    bus.attach(a);
    RecordingNode b{2, bus};
    bus.attach(b);

    a.scl_low();
    assert(a.events.size() == 1);
    assert_event(a, 0, {true, true}, {false, true});
    assert(b.events.size() == 1);
    assert_event(b, 0, {true, true}, {false, true});

    b.scl_low();
    assert(a.events.size() == 1);
    assert(b.events.size() == 1);

    a.scl_release();
    assert(a.events.size() == 1);
    assert(b.events.size() == 1);

    b.scl_release();
    assert(a.events.size() == 2);
    assert_event(a, 1, {false, true}, {true, true});
    assert(b.events.size() == 2);
    assert_event(b, 1, {false, true}, {true, true});
}

class CascadingNode : public RecordingNode
{
    bool cascaded_{};

public:
    CascadingNode(int id, I2CBus& bus) : RecordingNode{id, bus}
    {
    }

    void on_bus_changed(I2CBus::State old_state, I2CBus::State new_state) override
    {
        RecordingNode::on_bus_changed(old_state, new_state);

        if (new_state.scl == false && !cascaded_) {
            cascaded_ = true;
            sda_low();
        }
    }
};

void test_transition_cascade()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    RecordingNode a{1, bus};
    bus.attach(a);
    CascadingNode b{2, bus};
    bus.attach(b);

    a.scl_low();

    assert(a.events.size() == 2);
    assert_event(a, 0, {true, true}, {false, true});
    assert_event(a, 1, {false, true}, {false, false});
    assert(b.events.size() == 2);
    assert_event(b, 0, {true, true}, {false, true});
    assert_event(b, 1, {false, true}, {false, false});
}

class MultiCascadingNode : public RecordingNode
{
    bool cascaded_{};

public:
    MultiCascadingNode(int id, I2CBus& bus) : RecordingNode{id, bus}
    {
    }

    void on_bus_changed(I2CBus::State old_state, I2CBus::State new_state) override
    {
        RecordingNode::on_bus_changed(old_state, new_state);

        if (new_state.scl == false && !cascaded_) {
            cascaded_ = true;
            scl_low();
            sda_low();
            scl_release();
            sda_release();
        }
    }
};

void test_transition_cascade_multi()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    RecordingNode a{1, bus};
    bus.attach(a);
    MultiCascadingNode b{2, bus};
    bus.attach(b);

    a.scl_low();
    // Cascade here.
    a.scl_release();

    assert(a.events.size() == 4);
    assert_event(a, 0, {true, true}, {false, true});
    assert_event(a, 1, {false, true}, {false, false});
    assert_event(a, 2, {false, false}, {false, true});
    assert_event(a, 3, {false, true}, {true, true});
    assert(b.events.size() == 4);
    assert_event(b, 0, {true, true}, {false, true});
    assert_event(b, 1, {false, true}, {false, false});
    assert_event(b, 2, {false, false}, {false, true});
    assert_event(b, 3, {false, true}, {true, true});
}

class DetachingNode : public RecordingNode
{
    bool detached_{};
    I2CNode& victim_;

public:
    DetachingNode(int id, I2CBus& bus, I2CNode& victim) : RecordingNode{id, bus}, victim_{victim} {}

    void on_bus_changed(I2CBus::State old_state, I2CBus::State new_state) override
    {
        RecordingNode::on_bus_changed(old_state, new_state);
        if (!detached_) {
            detached_ = true;
            bus_.detach(victim_);
        }
    }
};

class AttachingNode : public RecordingNode
{
    bool attached_{};
    I2CNode& node_;

public:
    AttachingNode(int id, I2CBus& bus, I2CNode& node) : RecordingNode{id, bus}, node_{node} {}

    void on_bus_changed(I2CBus::State old_state, I2CBus::State new_state) override
    {
        RecordingNode::on_bus_changed(old_state, new_state);
        if (!attached_) {
            attached_ = true;
            bus_.attach(node_);
        }
    }
};

void test_node_destruction_detaches()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};

    {
        I2CNode n{1, bus};
        bus.attach(n);
        n.scl_low();
        n.sda_low();
        assert(!bus.scl());
        assert(!bus.sda());
    }

    // Destruction detached n.
    assert(bus.scl());
    assert(bus.sda());
}

class DeferringNode : public I2CNode
{
public:
    DeferringNode(int id, I2CBus& bus) : I2CNode{id, bus} {}

    void defer_test(int ticks, std::function<void()> function)
    {
        defer(ticks, std::move(function));
    }
};

void test_node_destruction_cancels_deferred()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    bool run{};
    constexpr int ticks = 3;

    {
        DeferringNode n{1, bus};
        bus.attach(n);
        n.defer_test(ticks, [&] {
            run = true;
        });
    }

    for (int i = 0; i < ticks; ++i) {
        scheduler.yield();
    }

    assert(!run);
}

void test_attach_during_callback()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    RecordingNode a{1, bus};
    bus.attach(a);
    RecordingNode c{3, bus};
    AttachingNode b{2, bus, c};
    bus.attach(b);

    a.scl_low();

    // C must not receive the transition which caused it to be attached.
    assert(!bus.scl());
    assert(bus.sda());
    assert(c.events.empty());

    // A, B saw the original transition.
    assert(a.events.size() == 1);
    assert(b.events.size() == 1);

    // The newly attached node must participate in subsequent transitions.
    a.scl_release();

    assert(c.events.size() == 1);
    assert_event(c, 0, {false, true}, {true, true});

    assert(b.events.size() == 2);
    assert_event(b, 0, {true, true}, {false, true});
    assert_event(b, 1, {false, true}, {true, true});

    assert(a.events.size() == 2);
    assert_event(a, 0, {true, true}, {false, true});
    assert_event(a, 1, {false, true}, {true, true});
}

void test_detach_during_callback()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    RecordingNode a{1, bus};
    bus.attach(a);
    DetachingNode b{1, bus, a};
    bus.attach(b);

    a.scl_low();

    // Note: a is detached by b, and this releases SCL.
    assert(a.events.size() == 1);
    assert_event(a, 0, {true, true}, {false, true});
    assert(b.events.size() == 2);
    assert_event(b, 0, {true, true}, {false, true});
    assert_event(b, 1, {false, true}, {true, true});
}

class SelfDetachingNode : public RecordingNode
{
    bool detached_{};

public:
    SelfDetachingNode(int id, I2CBus& bus) : RecordingNode{id, bus} {}

    void on_bus_changed(I2CBus::State old_state, I2CBus::State new_state) override
    {
        RecordingNode::on_bus_changed(old_state, new_state);
        if (!detached_) {
            detached_ = true;
            bus_.detach(*this);
        }
    }
};

void test_self_detach_during_callback()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    RecordingNode a{1, bus};
    bus.attach(a);
    SelfDetachingNode b{2, bus};
    bus.attach(b);

    b.scl_low();

    assert(a.events.size() == 2);
    assert_event(a, 0, {true, true}, {false, true});
    assert_event(a, 1, {false, true}, {true, true});

    assert(b.events.size() == 1);
    assert_event(b, 0, {true, true}, {false, true});
}

void test_hot_plug()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    I2CController c{0, bus};
    bus.attach(c);

    c.start_condition();
    assert(!c.write_octet(make_address_rw(0x60, false)));
    c.stop_condition();

    // Plug.
    ReferenceTarget t{3, bus, 0x60};
    bus.attach(t);

    assert( bus.scl());
    assert( bus.sda());

    t.scl_low();
    t.sda_low();

    assert(!bus.scl());
    assert(!bus.sda());
    assert(!c.scl());
    assert(!c.sda());
    assert(!t.scl());
    assert(!t.sda());

    // Unplug.
    bus.detach(t);

    // Unplug removes the driven lines.
    assert( bus.scl());
    assert( bus.sda());
    assert( c.scl());
    assert( c.sda());

    // Plug.
    bus.attach(t);

    assert( bus.scl());
    assert( bus.sda());

    c.start_condition();
    assert(c.write_octet(make_address_rw(0x60, false)));
    c.stop_condition();

    // Unplug.
    bus.detach(t);

    c.start_condition();
    assert(!c.write_octet(make_address_rw(0x60, false)));
    c.stop_condition();
}

class StretchNode : public I2CNode
{
public:
    StretchNode(int id, I2CBus& bus) : I2CNode{id, bus} {}

    void stretch(int ticks)
    {
        scl_low();
        defer(ticks, [this]() {
            scl_release();
        });
    }
};

void test_wait_for()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    I2CController c{0, bus};
    bus.attach(c);
    StretchNode a{1, bus};
    bus.attach(a);

    assert(bus.scl());
    constexpr int ticks = 3;
    a.stretch(ticks);
    assert(!bus.scl());
    c.wait_scl_high(ticks);
    assert(bus.scl());
}

int main()
{
    test_open_drain_independence();
    test_transition_notification();
    test_transition_cascade();
    test_transition_cascade_multi();
    test_node_destruction_detaches();
    test_node_destruction_cancels_deferred();
    test_attach_during_callback();
    test_detach_during_callback();
    test_self_detach_during_callback();
    test_hot_plug();
    test_wait_for();
}
