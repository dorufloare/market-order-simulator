#pragma once

#include "order.hpp"
#include <map>
#include <mutex>
#include <deque>
#include <atomic>
#include <functional>

class OrderBook {
public:
    void match(const Order& order);
    void match(const Order& order, const std::function<void(double)>& onMatchPrice);
    
    double getLastTradedPrice() const;
    void setLastTradedPrice(double price);

private:
    std::map<double, std::deque<Order>> asks;
    std::map<double, std::deque<Order>> bids;
    
    // STOP order books - separate from regular orders
    std::map<double, std::deque<Order>> stopAsks;  // STOP SELL orders
    std::map<double, std::deque<Order>> stopBids;  // STOP BUY orders
    
    // ICEBERG order books - track hidden quantities
    std::map<double, std::deque<Order>> icebergAsks;  // ICEBERG SELL orders
    std::map<double, std::deque<Order>> icebergBids;  // ICEBERG BUY orders

    std::mutex orderBookMutex;
    std::atomic<double> last_traded_price {100.0}; 
    
    void addToStopBook(const Order& order);
    void addToIcebergBook(const Order& order);
    void addToIcebergTrackingOnly(const Order& order);
    void checkStopTriggers(double lastTradePrice, const std::function<void(double)>& onMatchPrice);
    void processTriggeredOrder(const Order& order, const std::function<void(double)>& onMatchPrice);
    void refillIcebergOrder(const Order& fullyExecutedOrder, double tradedQty, const std::function<void(double)>& onMatchPrice);
};
