#pragma once

#include <mutex>
#include <condition_variable>
#include <deque>
#include <optional>

// Thread-safe bounded/unbounded queue for handing orders from a producer
// thread (order generator) to a consumer thread (the matching engine).

template <typename T>
class ConcurrentQueue {
public:
    // Pushes an item and wakes up one waiting consumer.
    void push(T item);

    // Blocks until an item is available, then pops and returns it.
    T pop();

    // Non-blocking pop. Returns std::nullopt if empty.
    std::optional<T> tryPop();

    size_t size() const;

private:
    std::deque<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

