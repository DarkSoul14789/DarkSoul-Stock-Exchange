#include "OrderBook.hpp"
#include <stdexcept>

template <typename OppositeLevels, typename OwnLevels>
std::vector<Trade> OrderBook::matchAndRest(Order order, OppositeLevels& opposite, OwnLevels& own) {
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
        orderLocations_.insert({order.id, {order.price, order.side}});
    }
    return trades;
}

std::vector<Trade> OrderBook::addOrder(Order order) {
    std::lock_guard<std::mutex> lock(mutex_);
    if(order.side == Order::Side::Buy){
        return matchAndRest(order, sellLevels_, buyLevels_);
    }
    else{
        return matchAndRest(order, buyLevels_, sellLevels_);
    }
}

template <typename Levels>
bool OrderBook::cancel(Levels& levels, double price, uint64_t orderId){
    if(levels.size() == 0) return false;

    auto levelIt = levels.find(price);

    if(levelIt == levels.end()) return false;

    auto &level = levelIt->second;

    auto it = std::find_if(level.begin(), level.end(), [orderId](const Order &order){
        return orderId == order.id;
    });

    if(it == level.end()) return false;

    level.erase(it);

    if(level.empty()){
        levels.erase(levelIt);
    }

    return true;
}

bool OrderBook::cancelOrder(uint64_t orderId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto location = orderLocations_.find(orderId);
    if(location == orderLocations_.end()){
        return false; //Order not found
    }
    bool ret;
    if(location->second.second == Order::Side::Buy){
        ret = cancel(buyLevels_, location->second.first, orderId);
    }
    else{
        ret = cancel(sellLevels_, location->second.first, orderId);
    }

    if(ret == true){
        orderLocations_.erase(orderId);
    }
    return ret;
}

std::optional<double> OrderBook::bestBid() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if(buyLevels_.empty()){
        return std::nullopt;
    }
    return buyLevels_.begin()->first;
}

std::optional<double> OrderBook::bestAsk() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if(sellLevels_.empty()){
        return std::nullopt;
    }
    return sellLevels_.begin()->first;
}

size_t OrderBook::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t size = 0;
    for(const auto &[price, orders] : buyLevels_){
        size += orders.size();
    }
    for(const auto &[price, orders] : sellLevels_){
        size += orders.size();
    }
    return size;
}

int OrderBook::quantityAtBestBid() const {
    std::lock_guard<std::mutex> lock(mutex_);
    int qty = 0;
    if(buyLevels_.empty()) return qty;
    for(const auto& order:buyLevels_.begin()->second){
        qty += order.quantity;
    }
    return qty;
}

int OrderBook::quantityAtBestAsk() const {
    std::lock_guard<std::mutex> lock(mutex_);
    int qty = 0;
    if(sellLevels_.empty()) return qty;
    for(const auto& order:sellLevels_.begin()->second){
        qty += order.quantity;
    }
    return qty;
}