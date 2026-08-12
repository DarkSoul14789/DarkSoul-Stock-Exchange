#include "OrderBook.hpp"
#include <stdexcept>

template <typename OppositeLevels, typename OwnLevels>
std::vector<Trade> matchAndRest(Order order, OppositeLevels& opposite, OwnLevels& own) {
    std::vector<Trade> trades;

    while (order.quantity != 0 && !opposite.empty()) {
        auto levelIt = opposite.begin(); //gives best price,deque

        if(order.type == Order::Type::Limit){
            if(order.side == Order::Side::Buy && order.price < levelIt->first){
                break;
            }
            if(order.side == Order::Side::Sell && order.price > levelIt->first){
                break;
            }
        }

        auto &restingOrder = levelIt->second.front();
        int matchedQty = std::min(order.quantity, restingOrder.quantity);
        
        Trade trade;

        if(order.side == Order::Side::Buy){
            trade = Trade{order.id, restingOrder.id, restingOrder.price, matchedQty, std::chrono::steady_clock::now()};
        }
        else{
            trade = Trade{restingOrder.id, order.id, restingOrder.price, matchedQty, std::chrono::steady_clock::now()};
        }

        trades.push_back(trade);

        order.quantity -= matchedQty;
        restingOrder.quantity -= matchedQty;

        if(restingOrder.quantity == 0){
            //Pop resting order from deque
            levelIt->second.pop_front();

            if(levelIt->second.empty()){
                //Erase the level from the map
                opposite.erase(levelIt);
            }
        }
    }
    if (order.quantity != 0 && order.type == Order::Type::Limit) {
        own[order.price].push_back(order);
    }
    return trades;
}

std::vector<Trade> OrderBook::addOrder(Order order) {
    if(order.side == Order::Side::Buy){
        return matchAndRest(order, sellLevels_, buyLevels_);
    }
    else{
        return matchAndRest(order, buyLevels_, sellLevels_);
    }
}

bool OrderBook::cancelOrder(uint64_t orderId) {
    (void)orderId;
    // TODO
    throw std::logic_error("OrderBook::cancelOrder not implemented");
}

std::optional<double> OrderBook::bestBid() const {
    if(buyLevels_.empty()){
        return std::nullopt;
    }
    return buyLevels_.begin()->first;
}

std::optional<double> OrderBook::bestAsk() const {
    if(sellLevels_.empty()){
        return std::nullopt;
    }
    return sellLevels_.begin()->first;
}

size_t OrderBook::size() const {
    size_t size = 0;
    for(const auto &[price, orders] : buyLevels_){
        size += orders.size();
    }
    for(const auto &[price, orders] : sellLevels_){
        size += orders.size();
    }
    return size;
}
