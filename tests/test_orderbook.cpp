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

TEST_CASE("a marketable limit order fully matches against the resting order", "[orderbook][matching]") {
    OrderBook book;
    Order sell(1, Order::Side::Sell, Order::Type::Limit, 100.0, 10);
    auto trades = book.addOrder(sell);
    
    Order buy(2, Order::Side::Buy, Order::Type::Limit, 100.0, 10);
    auto trades1 = book.addOrder(buy);

    REQUIRE(trades1.size() == 1);
    REQUIRE(trades1[0].price == 100.0);
    REQUIRE(trades1[0].quantity == 10);
    REQUIRE(book.size() == 0);
}

TEST_CASE("a limit order partially fills and the remainder rests", "[orderbook][matching]") {
    OrderBook book;
    Order sell(1, Order::Side::Sell, Order::Type::Limit, 100.0, 10);
    auto trades = book.addOrder(sell);
    
    Order buy(2, Order::Side::Buy, Order::Type::Limit, 100.0, 4);
    auto trades1 = book.addOrder(buy);

    REQUIRE(trades1.size() == 1);
    REQUIRE(trades1[0].quantity == 4);
    REQUIRE(book.size() == 1);
    REQUIRE(book.quantityAtBestAsk() == 6);
}

TEST_CASE("price-time priority: earlier order at the same price fills first", "[orderbook][matching]") {
    OrderBook book;
    Order sell(1, Order::Side::Sell, Order::Type::Limit, 100.0, 5);
    auto trades = book.addOrder(sell);
    
    Order sell1(2, Order::Side::Sell, Order::Type::Limit, 100.0, 5);
    auto trades1 = book.addOrder(sell1);
    REQUIRE(book.quantityAtBestAsk() == 10);

    Order buy(3, Order::Side::Buy, Order::Type::Limit, 100.0, 5);
    auto trades2 = book.addOrder(buy);

    REQUIRE(trades2.size() == 1);
    REQUIRE(trades2[0].buyOrderId == 3);
    REQUIRE(trades2[0].sellOrderId == 1);
    REQUIRE(book.quantityAtBestAsk() == 5);
}

TEST_CASE("a market order matches at the best available price", "[orderbook][matching]") {
    OrderBook book;
    Order sell(1, Order::Side::Sell, Order::Type::Limit, 101.0, 5);
    book.addOrder(sell);

    Order buy(2, Order::Side::Buy, Order::Type::Market, -1, 5);
    auto trades = book.addOrder(buy);

    REQUIRE(trades.size() == 1);
    REQUIRE(trades[0].price == 101.0);
}

TEST_CASE("a market order with insufficient liquidity does not rest", "[orderbook][matching]") {
    OrderBook book;
    Order buy(1, Order::Side::Buy, Order::Type::Market, -1, 5);
    auto trades = book.addOrder(buy);
    REQUIRE(trades.size() == 0);
    REQUIRE(book.size() == 0);
}

TEST_CASE("cancel removes a resting order and it can no longer be matched", "[orderbook][cancel]") {
    OrderBook book;
    Order sell(1, Order::Side::Sell, Order::Type::Limit, 100.0, 5);
    book.addOrder(sell);
    bool success = book.cancelOrder(1);
    REQUIRE(success == true);
    REQUIRE(book.size() == 0);

    Order buy(2, Order::Side::Buy, Order::Type::Limit, 100.0, 5);
    auto trades = book.addOrder(buy);
    REQUIRE(trades.empty());
}

TEST_CASE("cancelling a non-existent order id returns false", "[orderbook][cancel]") {
    OrderBook book;
    bool success = book.cancelOrder(999);
    REQUIRE(success == false);
}