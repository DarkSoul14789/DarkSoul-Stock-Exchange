#include <catch2/catch_test_macros.hpp>
#include "OrderBook.hpp"


TEST_CASE("empty book has no best bid or ask", "[orderbook][basic]") {
    OrderBook book;
    REQUIRE(book.size() == 0);
    REQUIRE_FALSE(book.bestBid().has_value());
    REQUIRE_FALSE(book.bestAsk().has_value());
}

TEST_CASE("a single resting limit order becomes the best price", "[orderbook][basic]") {
    OrderBook book;
    Order buy(1, Order::Side::Buy, Order::Type::Limit, 100.0, 10);
    auto trades = book.addOrder(buy);

    REQUIRE(trades.empty());              // nothing to match against yet
    REQUIRE(book.bestBid().value() == 100.0);
    REQUIRE(book.size() == 1);
}

