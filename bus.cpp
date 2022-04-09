#include "bus.hpp"

#include "line.hpp"
#include "log.hpp"
#include "node.hpp"

#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

class Bus::Impl
{
    /// Data line.
    Line sda_;

    /// Clock line.
    Line scl_;

    /// This mutex protects the following member variables.
    std::mutex sync_mutex_;

    /// Sequence number incremented on every event.
    uint64_t sequence_;

    struct ClientState
    {
        /// Observed sequence number.
        uint64_t sequence;

        /// Pending flag is true if this client is blocked attempting to publish an event.
        bool pending;
    };

    /// Tracks attached client nodes.
    std::map<const Node *, ClientState> clients_;

    /// Tracks the in-flight event publisher.
    const Node * publisher_;

    struct Transaction
    {
        /// The node that wants to publish.
        const Node * node;

        /// The event.
        Event event;
    };

    /// Tracks the set of events that are waiting to be published.
    std::vector<Transaction> queue_;

    /// Used to detect that client threads have observed an event.
    std::condition_variable sync_condition_;

    /// Used to wake up pending clients after an on-going transaction completes.
    std::condition_variable pending_condition_;

    /// Process an event by updating the bus state.
    void process(const Transaction & transaction)
    {
        switch (transaction.event) {
            case Event::DataLow:
                sda_.set(transaction.node, Line::Level::Low);
                break;
            case Event::DataHigh:
                sda_.set(transaction.node, Line::Level::High);
                break;
            case Event::ClockLow:
                scl_.set(transaction.node, Line::Level::Low);
                break;
            case Event::ClockHigh:
                scl_.set(transaction.node, Line::Level::High);
                break;
            case Event::Delay:
                break;
        }
    }

    /// @discussion Called by client threads to synchronize with current state.
    /// @return Line::Level SDA level
    /// @return Line::Level SCL level
    std::tuple<Line::Level, Line::Level> sync(const Node * node)
    {
        std::unique_lock<std::mutex> lock(sync_mutex_);
        locked_sync(node);

        return {sda_.get(), scl_.get()};
    }

    void locked_sync(const Node * node)
    {
        if (clients_[node].sequence < sequence_) {
            // Advance.
            clients_[node].sequence++;

            // Notify waiting publisher.
            sync_condition_.notify_one();
        }
    }

    /// @discussion Called when a thread is awoken by one of the two condition variables.
    /// @return bool True if all client threads are synchronized.
    bool all_clients_synchronized() const
    {
        for (auto const & client : clients_) {
            if (client.second.sequence != sequence_) {
                return false;
            }
        }
        return true;
    }

    /// @discussion Publish a state change and wait for all other clients to observe that change.
    void publish(const Node * node, Event event)
    {
        std::unique_lock<std::mutex> lock(sync_mutex_);
        queue_.push_back({node, event});

        if (publisher_) {
            // This thread may gain the lock -- but find that another thread is busy publishing.
            // This happens when (a) two threads race to publish and (b) when this thread wants
            // to publish in response to an event currently being published by another thread.

            // We are pending: we have something to publish.
            clients_[node].pending = true;

            // Note that if multiple publishers are blocked, then once it is possible to publish,
            // the first publisher that gains the lock will handle *all* queued requests.
            // This behaviour keeps all client threads in sync.

            while (publisher_) {
                pending_condition_.wait(lock, [&]{
                    // If our state change was processed, then return to normal processing.
                    if (!clients_[node].pending) {
                        return true;
                    }

                    // Call sync() (with lock held) to allow other publisher to succeed.
                    locked_sync(node);

                    // Proceed when not busy.
                    return !publisher_;
                });

                if (!clients_[node].pending) {
                    // Our state change was published by another thread.  Nothing more to do.
                    return;
                }
            }

            if (queue_.empty()) {
                // Queue emptied by another thread.  Nothing more to do.
                return;
            }
        }

        // Transaction begins.
        publisher_ = node;

        auto snapshot = std::move(queue_);
        for (const auto & transaction : snapshot) {
            // Update state.
            process(transaction);

            // The client state has been acted upon, and is no longer pending.
            clients_[transaction.node].pending = false;
        }

        // Wait for threads to synchronize *twice*.
        // After the first time we know that threads have *observed* the new state by calling sync().
        // After the second time we know that threads have *acted* on that new state and called sync() again.

        for (auto i = 1; i <= 2; ++i) {
            // Advance, since we have updated the state.
            sequence_++;

            // Synchronize sequence number manually since node is blocked in publish().
            clients_[node].sequence = sequence_;

            // Pending publishers implicitly see the new state.
            pending_condition_.notify_all();

            // Wait for threads to observe the new state via a call to sync().
            sync_condition_.wait(lock, [&]{
                // Proceed when all clients are synchronized.
                return all_clients_synchronized();
            });
        }

        // Transaction complete.
        publisher_ = nullptr;

        // Notify pending publisher threads.
        pending_condition_.notify_all();
    }

public:
    Impl() : sda_{}, scl_{}, sync_mutex_{}, sequence_{}, clients_{}, publisher_{}, queue_{}, sync_condition_{}, pending_condition_{}
    {
    }

    void attach(const Node * node)
    {
        std::unique_lock<std::mutex> lock(sync_mutex_);
        clients_[node] = {};
    }

    void detach(const Node * node)
    {
        std::unique_lock<std::mutex> lock(sync_mutex_);
        clients_.erase(node);
    }

    std::tuple<Line::Level, Line::Level> get(const Node * node)
    {
        std::this_thread::yield();
        return sync(node);
    }

    void set(const Node * node, Event event)
    {
        publish(node, event);
    }
};

Bus::Bus() : pimpl{std::make_unique<Impl>()}
{
}

Bus::~Bus() = default;

void Bus::attach(const Node * node)
{
    pimpl->attach(node);
}

void Bus::detach(const Node * node)
{
    pimpl->detach(node);
}

std::tuple<Line::Level, Line::Level> Bus::get(const Node * node)
{
    return pimpl->get(node);
}

void Bus::set(const Node * node, Event event)
{
    pimpl->set(node, event);
}
