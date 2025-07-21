#include "order_book.hpp"
#include "logger.hpp"
#include "benchmark.hpp"
#include <iostream>
#include <algorithm>

void OrderBook::addToIcebergBook(const Order& order) {
    Order visibleOrder = order;
    visibleOrder.quantity = order.displayQuantity;
    visibleOrder.type = OrderType::LIMIT;
    
    if (order.side == Side::SELL) {
        asks[order.price].push_back(visibleOrder);
        icebergAsks[order.price].push_back(order);
    } else {
        bids[order.price].push_back(visibleOrder);
        icebergBids[order.price].push_back(order);
    }
    
    Logger::getInstance().logRestingOrder(visibleOrder);
    Benchmark::getInstance().incrementCounter("Orders_Resting");
}

void OrderBook::addToIcebergTrackingOnly(const Order& order) {
    if (order.side == Side::SELL) {
        icebergAsks[order.price].push_back(order);
    } else {
        icebergBids[order.price].push_back(order);
    }
}

void OrderBook::refillIcebergOrder(const Order& fullyExecutedOrder, double tradedQty, const std::function<void(double)>&) {
    double price = fullyExecutedOrder.price;
    
    auto& icebergBook = (fullyExecutedOrder.side == Side::SELL) ? icebergAsks : icebergBids;
    auto& normalBook = (fullyExecutedOrder.side == Side::SELL) ? asks : bids;
    
    auto icebergIt = icebergBook.find(price);
    if (icebergIt == icebergBook.end() || icebergIt->second.empty()) {
        return;
    }
    
    auto& icebergQueue = icebergIt->second;
    
    for (auto it = icebergQueue.begin(); it != icebergQueue.end(); ++it) {
        if (it->id == fullyExecutedOrder.id) {
            it->totalQuantity -= tradedQty;
            
            if (it->totalQuantity <= 0) {
                if (fullyExecutedOrder.userId == 0) {
                    std::cout << "[ICEBERG COMPLETE] Your ICEBERG order fully executed!\n";
                }
                icebergQueue.erase(it);
                if (icebergQueue.empty()) {
                    icebergBook.erase(icebergIt);
                }
                return;
            }
            
            double newVisibleQty = std::min(it->displayQuantity, it->totalQuantity);
            if (newVisibleQty > 0) {
                Order newVisibleOrder = *it;
                newVisibleOrder.quantity = newVisibleQty;
                newVisibleOrder.type = OrderType::LIMIT;
                
                normalBook[price].push_back(newVisibleOrder);
                
                Logger::getInstance().logRestingOrder(newVisibleOrder);
                Benchmark::getInstance().incrementCounter("Orders_Resting");
                Benchmark::getInstance().incrementCounter("Iceberg_Orders_Refilled");
                
                if (fullyExecutedOrder.userId == 0) {
                    std::cout << "[ICEBERG REFILL] " << newVisibleQty << " more shares now visible @ $" << price << " (remaining: " << it->totalQuantity << ")\n";
                }
            }
            return;
        }
    }
}
