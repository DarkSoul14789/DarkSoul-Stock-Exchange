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
    void push(T item){
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push_back(std::move(item));
        lock.unlock();
        cv_.notify_one();
    }

    // Blocks until an item is available, then pops and returns it.
    T pop(){
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]{return !queue_.empty(); });

        T ret = queue_.front();
        queue_.pop_front();
        return ret;
    }

    // Non-blocking pop. Returns std::nullopt if empty.
    std::optional<T> tryPop(){
        std::unique_lock<std::mutex> lock(mutex_);
        if(queue_.empty()) return std::nullopt;
        else{
            T ret = queue_.front();
            queue_.pop_front();
            return ret;
        }
    }

    size_t size() const{
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    std::deque<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

