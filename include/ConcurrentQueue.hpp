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
    
    // Pushes a whole batch of items under a single lock acquisition, then
    // wakes the consumer once. Amortizes lock/notify overhead across
    // `items.size()` elements instead of paying it once per element.
    void pushBatch(std::vector<T> items){
        std::unique_lock<std::mutex> lock(mutex_);
        for (auto& item : items) {
            queue_.push_back(std::move(item));
        }
        lock.unlock();
        cv_.notify_one();
    }

    // Blocks until at least one item is available, then pops up to
    // maxBatch items in one lock acquisition. Returns fewer than
    // maxBatch items if that's all that's currently available - this
    // does not wait to fill the batch completely.
    std::vector<T> popBatch(size_t maxBatch){
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]{ return !queue_.empty(); });

        std::vector<T> batch;
        size_t n = std::min(maxBatch, queue_.size());
        batch.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            batch.push_back(std::move(queue_.front()));
            queue_.pop_front();
        }
        return batch;
    }

private:
    std::deque<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

