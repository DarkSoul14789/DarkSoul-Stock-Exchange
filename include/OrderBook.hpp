#pragma once

#include "Order.hpp"
#include <map>
#include <deque>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <optional>

// Single-sided-access order book
class OrderBook {
public:
    // Adds a new order to the book, attempting to match it immediately
    // against the opposite side using price-time priority.
    // Returns the list of trades generated (Its empty if no match occurred).
    std::vector<Trade> addOrder(Order order);

    // Cancels a resting order by id. Returns true if found and removed.
    bool cancelOrder(uint64_t orderId);

    // Returns the current best bid / best ask, if they exist.
    std::optional<double> bestBid() const;
    std::optional<double> bestAsk() const;

    // Total number of resting orders currently in the book.
    size_t size() const;

    // Returns qty of the first/best level.
    int quantityAtBestBid() const;
    int quantityAtBestAsk() const;

private:
    // Buy side: highest price first  -> std::greater comparator
    std::map<double, std::deque<Order>, std::greater<double>> buyLevels_;
    // Sell side: lowest price first -> default std::less comparator
    std::map<double, std::deque<Order>> sellLevels_;


    mutable std::mutex mutex_;  // guards the maps above if accessed concurrently
};
