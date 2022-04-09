#include "i2cbus.h"
#include "i2ccontroller.h"
#include "i2cnode.h"
#include "i2cprotocol.h"
#include "i2cscheduler.h"
#include "i2ctarget.h"
#include "referencetarget.h"

#include <cassert>
#include <iostream>

void test_bus_recovery_failure()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    I2CController c{0, bus};
    bus.attach(c);
    I2CNode target{1, bus};
    bus.attach(target);

    target.sda_low();

    assert(!c.recover_bus());

    assert(bus.scl());
    assert(!bus.sda());
}

class ResettingController : public I2CController
{
public:
    ResettingController(int id, I2CBus& bus) : I2CController{id, bus}
    {
    }

    /// Simulate a controller reset during a read transaction.
    void partial_read()
    {
        sda_release();

        // Generate one read clock.
        scl_low();
        scl_release();

        // Wait for the rising edge to be observed.
        wait_scl_high(timeout());
    }
};

void test_bus_recovery()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    ResettingController c{0, bus};
    bus.attach(c);
    ReferenceTarget t{1, bus, 0x40};
    bus.attach(t);

    // Begin a register read.
    c.start_condition();
    assert(c.write_octet(make_address_rw(0x40, true)));
    // Controller initiates read.
    c.partial_read();
    // ReferenceTarget register 0x00 contains 0x12 (00010010).
    // After the first sampled bit, it prepares the second bit, which is 0.
    assert(bus.scl());
    assert(!bus.sda());

    // Recover the bus by generating clock pulses.
    assert(c.recover_bus());

    // Recovery must leave an idle bus.
    assert(bus.scl());
    assert(bus.sda());

    // A regular read proves that the target state machine has recovered.
    c.start_condition();
    assert(c.write_octet(make_address_rw(0x40, true)));
    assert(c.read_octet(false) == 0x12);
    c.stop_condition();

    assert(bus.scl());
    assert(bus.sda());
}

void test_start_timeout_immediate()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    I2CController c{0, bus};
    bus.attach(c);
    I2CNode n{1, bus};
    bus.attach(n);

    c.set_timeout(0);

    // SCL not held low, timeout of zero irrelevant.
    assert(c.start_condition());
    assert(!c.error());

    n.scl_low();

    assert(!c.start_condition());
    assert(c.error());
    assert(!c.error());
}

void test_start_timeout()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    I2CController c{0, bus};
    bus.attach(c);
    I2CNode n{1, bus};
    bus.attach(n);

    n.scl_low();

    scheduler.defer(nullptr, c.timeout(), [&] {
        // Release SCL before the controller's timeout expires.
        n.scl_release();
    });

    assert(c.start_condition());
    assert(!c.error());
    assert(c.stop_condition());
    assert(!c.error());

    // Hold SCL low until controller times-out.
    n.scl_low();

    assert(!c.start_condition());
    assert(c.error());
    assert(!c.error());

    n.scl_release();

    assert(c.start_condition());
    assert(!c.error());
    assert(!bus.scl());
    assert(!bus.sda());
    assert(c.stop_condition());
    assert(bus.scl());
    assert(bus.sda());
}

void test_stop_timeout()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    I2CController c{0, bus};
    bus.attach(c);
    I2CNode n{1, bus};
    bus.attach(n);

    n.scl_low();

    // Drive low to test the behaviour of STOP.
    c.sda_low();

    assert(!c.stop_condition());
    assert(c.error());
    assert(!c.error());

    // STOP must release SDA.
    assert(bus.sda());
}

void test_write_octet_timeout()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    I2CController c{0, bus};
    bus.attach(c);
    I2CNode n{1, bus};
    bus.attach(n);
    ReferenceTarget t{2, bus, 0x50};
    bus.attach(t);

    n.scl_low();

    assert(!c.write_octet(make_address_rw(0x50, true)));
    assert(c.error());
    assert(!c.error());

    n.scl_release();

    // Missing START.
    assert(!c.write_octet(make_address_rw(0x50, true)));
    assert(!c.error());

    c.start_condition();
    assert(c.write_octet(make_address_rw(0x50, true)));
    assert(!c.error());
    c.stop_condition();
}

class StretchingNode : public I2CNode
{
    unsigned rising_edges_{};
    unsigned target_rising_edge_{};

public:
    StretchingNode(int id, I2CBus& bus) : I2CNode{id, bus}
    {
    }

    void drive_scl_low_on_rising_edge(unsigned rising_edge)
    {
        assert(rising_edge > 0);
        rising_edges_ = 0;
        target_rising_edge_ = rising_edge;
    }

    void on_bus_changed(I2CBus::State old_state, I2CBus::State new_state) override
    {
        if (!old_state.scl && new_state.scl) {
            ++rising_edges_;

            if (rising_edges_ == target_rising_edge_) {
                scl_low();
            }
        }
    }
};

void test_read_octet_timeout()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    I2CController c{0, bus};
    bus.attach(c);
    StretchingNode n{1, bus};
    ReferenceTarget t{2, bus, 0x50};
    bus.attach(t);

    assert(c.start_condition());
    assert(c.write_octet(make_address_rw(0x50, true)));

    // Attach the fault injector after the address has been written so that it does not interfere with the address phase.
    bus.attach(n);

    // Allow four bits to be read, then hold SCL low.
    n.drive_scl_low_on_rising_edge(4);
    assert(0xFF == c.read_octet(false));
    assert(c.error());
    assert(!c.error());

    // Ensure bus is released by the controller.
    assert(bus.sda());

    // Release the stuck clock.
    n.scl_release();

    // The controller should still be able to terminate the transaction.
    assert(c.stop_condition());
    assert(!c.error());

    assert(c.start_condition());
    assert(c.write_octet(make_address_rw(0x50, true)));
    assert(0xBA == c.read_octet(false));
    assert(!c.error());
    assert(c.stop_condition());
}

void test_address_nack()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    I2CController c{0, bus};
    bus.attach(c);
    ReferenceTarget t{2, bus, 0x50};
    bus.attach(t);

    c.start_condition();
    assert(!c.write_octet(make_address_rw(0x30, true)));
    assert(0xFF == c.read_octet(true));
    c.stop_condition();

    // Ensure bus is released.
    assert(bus.scl());
    assert(bus.sda());

    // The target must have returned to Idle.
    c.start_condition();
    assert(c.write_octet(make_address_rw(0x50, true)));
    assert(c.read_octet(false) == 0xBA);
    c.stop_condition();

    assert(bus.scl());
    assert(bus.sda());
}

void test_target_selection()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    I2CController c{0, bus};
    bus.attach(c);
    ReferenceTarget t1{1, bus, 0x40};
    bus.attach(t1);
    ReferenceTarget t2{2, bus, 0x50};
    bus.attach(t2);

    c.start_condition();
    assert(c.write_octet(make_address_rw(0x40, true)));
    assert(0x12 == c.read_octet(false));
    c.stop_condition();

    c.start_condition();
    assert(c.write_octet(make_address_rw(0x50, true)));
    assert(0xBA == c.read_octet(false));
    c.stop_condition();
}

void test_repeated_start_without_stop()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    I2CController c{0, bus};
    bus.attach(c);
    ReferenceTarget t{2, bus, 0x50};
    bus.attach(t);

    c.start_condition();
    assert(c.write_octet(make_address_rw(0x50, false)));
    c.start_condition();
    assert(c.write_octet(make_address_rw(0x50, true)));
    assert(0xBA == c.read_octet(false));
    c.stop_condition();
}

void test_stop_resets_transaction()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    I2CController c{0, bus};
    bus.attach(c);
    ReferenceTarget t{2, bus, 0x50};
    bus.attach(t);

    c.start_condition();
    assert(c.write_octet(make_address_rw(0x50, false)));
    assert(c.write_octet(0x10));
    c.stop_condition();

    c.start_condition();
    assert(c.write_octet(make_address_rw(0x50, true)));
    assert(0xAB == c.read_octet(false));
    c.stop_condition();
}

class ArbitrationFaultNode : public I2CNode
{
    unsigned rising_edges_{};
    unsigned target_rising_edge_{};

public:
    ArbitrationFaultNode(int id, I2CBus& bus) : I2CNode{id, bus}
    {
    }

    unsigned rising_edges() const
    {
        return rising_edges_;
    }

    void drive_sda_low_on_rising_edge(unsigned rising_edge)
    {
        target_rising_edge_ = rising_edge;
    }

    void on_bus_changed(I2CBus::State old_state, I2CBus::State new_state) override
    {
        if (!old_state.scl && new_state.scl) {
            ++rising_edges_;

            if (rising_edges_ == target_rising_edge_) {
                sda_low();
            }
        }
    }
};

void test_arbitration_loss()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    I2CController c{0, bus};
    bus.attach(c);
    ArbitrationFaultNode n{1, bus};
    bus.attach(n);
    ReferenceTarget t1{1, bus, 0x40};
    bus.attach(t1);

    // 0x40 = 0100 0000.
    // Force SDA low on the seventh rising edge.
    // The controller is transmitting a one at that point and should lose arbitration.
    n.drive_sda_low_on_rising_edge(7);

    c.start_condition();
    assert(!c.write_octet(make_address_rw(0x40, true)));
    assert(!c.error());
    assert(c.lost());
    assert(!c.lost());

    // Release SDA.
    n.sda_release();

    // The controller must stop transmitting after losing arbitration.
    assert(8 == n.rising_edges());
    assert(bus.scl());
    assert(bus.sda());

    // A subsequent operation works normally.
    c.start_condition();
    assert(c.write_octet(make_address_rw(0x40, true)));
    assert(0x12 == c.read_octet(false));
    c.stop_condition();
}

/// @param nack_at The index to NACK.
void verify_register_read(I2CController& c, uint8_t address, uint8_t offset, const std::vector<uint8_t>& expected, size_t nack_at)
{
    c.start_condition();
    assert(c.write_octet(make_address_rw(address, false)));
    assert(c.write_octet(offset));
    c.start_condition();
    assert(c.write_octet(make_address_rw(address, true)));
    for (size_t i = 0; i < expected.size(); ++i) {
        assert(expected[i] == c.read_octet(i != nack_at));
    }
    c.stop_condition();
}

void test_read()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    I2CController c{0, bus};
    bus.attach(c);
    ReferenceTarget t1{1, bus, 0x40};
    bus.attach(t1);
    ReferenceTarget t2{2, bus, 0x50};
    bus.attach(t2);

    // Repeated-start read with no data bytes.
    verify_register_read(c, 0x50, 0x10, {}, 0);

    // NACK early, target does not provide second octet.
    verify_register_read(c, 0x50, 0x10, {0xAB, 0xFF}, 0);

    // Multi-register reads.
    verify_register_read(c, 0x50, 0x10, {0xAB, 0xCD}, 1);
    verify_register_read(c, 0x50, 0x10, {0xAB, 0xCD, 0xEF}, 2);

    // Different offset.
    verify_register_read(c, 0x50, 0x11, {0xCD}, 1);

    // Wraparound.
    verify_register_read(c, 0x50, 0xff, {0x55, 0xBA}, 1);
}

void test_stress()
{
    std::cout << __func__ << std::endl;

    I2CScheduler scheduler;
    I2CBus bus{scheduler};
    I2CController c{0, bus};
    bus.attach(c);
    ReferenceTarget t1{1, bus, 0x40};
    bus.attach(t1);
    ReferenceTarget t2{2, bus, 0x50};
    bus.attach(t2);

    for (auto i = 0; i < 10; ++i) {
        verify_register_read(c, 0x40, 0x00, {0x12, 0x34}, 1);
        verify_register_read(c, 0x50, 0x10, {0xAB}, 0);
    }
}

int main()
{
    test_bus_recovery_failure();
    test_bus_recovery();
    test_start_timeout_immediate();
    test_start_timeout();
    test_stop_timeout();
    test_write_octet_timeout();
    test_read_octet_timeout();
    test_address_nack();
    test_target_selection();
    test_repeated_start_without_stop();
    test_stop_resets_transaction();
    test_arbitration_loss();
    test_read();
    test_stress();
}
