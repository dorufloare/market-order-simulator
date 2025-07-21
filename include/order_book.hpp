#pragma once

#include "order.hpp"
#include <map>
#include <mutex>
#include <deque>
#include <vector>
#include <atomic>
#include <functional>

class OrderBookShard {
public:
    void match(const Order& order, const std::function<void(double)>& onMatchPrice);

private:
    std::map<double, std::deque<Order>> asks;
    std::map<double, std::deque<Order>> bids;
    
    // STOP order books - separate from regular orders
    std::map<double, std::deque<Order>> stopAsks;  // STOP SELL orders
    std::map<double, std::deque<Order>> stopBids;  // STOP BUY orders

    std::mutex shardMutex;
    
    void addToStopBook(const Order& order);
    void checkStopTriggers(double lastTradePrice, const std::function<void(double)>& onMatchPrice);
    void processTriggeredOrder(const Order& order, const std::function<void(double)>& onMatchPrice);
};

class OrderBook {
public:
    explicit OrderBook(size_t shardCount = 4);

    void match(const Order& order);

    double getLastTradedPrice() const;
    void setLastTradedPrice(double price);

private:
    std::vector<OrderBookShard> shards;
    size_t shardCount;

    std::atomic<double> last_traded_price {100.0}; 

    size_t getShardIndex(double price);
};
