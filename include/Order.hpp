#pragma once

#include <chrono>
#include <cstdint>

// A single order sitting in (or arriving at) the book.
struct Order {
    enum class Side { Buy, Sell };
    enum class Type { Limit, Market };

    uint64_t id;
    Side side;
    Type type;
    double price;      // ignored for Market orders
    int quantity;       // remaining quantity (decrements on partial fill)
    std::chrono::steady_clock::time_point timestamp;

    Order(uint64_t id_, Side side_, Type type_, double price_, int quantity_)
        : id(id_), side(side_), type(type_), price(price_), quantity(quantity_),
          timestamp(std::chrono::steady_clock::now()) {}
};

// A completed match between two orders.
struct Trade {
    uint64_t buyOrderId;
    uint64_t sellOrderId;
    double price;
    int quantity;
    std::chrono::steady_clock::time_point timestamp;
};
