#include "OrderBook.hpp"
#include <stdexcept>

std::vector<Trade> OrderBook::addOrder(Order order) {
    (void)order;
    // TODO
    throw std::logic_error("OrderBook::addOrder not implemented");
}

bool OrderBook::cancelOrder(uint64_t orderId) {
    (void)orderId;
    // TODO
    throw std::logic_error("OrderBook::cancelOrder not implemented");
}

std::optional<double> OrderBook::bestBid() const {
    // TODO
    throw std::logic_error("OrderBook::bestBid not implemented");
}

std::optional<double> OrderBook::bestAsk() const {
    // TODO
    throw std::logic_error("OrderBook::bestAsk not implemented");
}

size_t OrderBook::size() const {
    // TODO
    throw std::logic_error("OrderBook::size not implemented");
}
