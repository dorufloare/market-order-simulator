#include "order_book.hpp"
#include "logger.hpp"
#include "benchmark.hpp"
#include <iostream>
#include <algorithm>
#include <functional>

void OrderBook::match(const Order& order) {
    auto updatePrice = [this](double price) {
        setLastTradedPrice(price);
    };
    
    match(order, updatePrice);
}

void OrderBook::match(const Order& order, const std::function<void(double)>& onMatchPrice) {
    BENCHMARK_TIMER("OrderBook_Match");
    
    std::lock_guard<std::mutex> lock(orderBookMutex);
    
    if (order.type == OrderType::STOP_LIMIT || order.type == OrderType::STOP_MARKET) {
        addToStopBook(order);
        Logger::getInstance().logOrder(order);
        Benchmark::getInstance().incrementCounter("Stop_Orders_Placed");
        
        if (order.userId == 0) {
            std::cout << "[STOP] Your " << ((order.type == OrderType::STOP_LIMIT) ? "STOP-LIMIT" : "STOP-MARKET")
                      << " " << ((order.side == Side::BUY) ? "BUY" : "SELL")
                      << " order placed. Will trigger when price " 
                      << ((order.side == Side::BUY) ? ">=" : "<=") << " $" << order.triggerPrice << "\n";
        }
        return;
    }
    
    Order workingOrder = order;
    if (order.type == OrderType::ICEBERG) {
        // For ICEBERG orders, process only the display quantity as a LIMIT order
        workingOrder.quantity = order.displayQuantity;
        workingOrder.type = OrderType::LIMIT;
        
        Logger::getInstance().logOrder(order);
        Benchmark::getInstance().incrementCounter("Iceberg_Orders_Placed");
    } else {
        Logger::getInstance().logOrder(order);
        Benchmark::getInstance().incrementCounter("Orders_Processed");
    }

    auto& oppositeBook = (workingOrder.side == Side::BUY) ? asks : bids;
    auto& sameSideBook = (workingOrder.side == Side::BUY) ? bids : asks;

    double remainingQty = workingOrder.quantity;
    double matchedPrice = 0.0;
    bool matched = false;

    for (auto it = oppositeBook.begin(); it != oppositeBook.end() && remainingQty > 0;) {
        double bookPrice = it->first;

        bool priceMatches = false;
        if (workingOrder.type == OrderType::MARKET) {
            priceMatches = true;
        } else if (workingOrder.side == Side::BUY) {
            priceMatches = workingOrder.price >= bookPrice;
        } else {
            priceMatches = workingOrder.price <= bookPrice;
        }

        if (!priceMatches)
            break;

        auto& queue = it->second;
        while (!queue.empty() && remainingQty > 0) {
            Order& restingOrder = queue.front();

            double tradeQty = std::min(remainingQty, restingOrder.quantity);
            matchedPrice = restingOrder.price;
            matched = true;

            Logger::getInstance().logMatch(workingOrder, restingOrder, matchedPrice, tradeQty);
            Benchmark::getInstance().incrementCounter("Orders_Matched");
            Benchmark::getInstance().addToCounter("Volume_Traded", static_cast<long>(tradeQty * 100));

            if (workingOrder.userId == 0) {
                std::cout << "[MATCH] You "
                          << ((workingOrder.side == Side::BUY) ? "bought" : "sold")
                          << " " << tradeQty << " units @ $" << matchedPrice << "\n";
            } else if (restingOrder.userId == 0) {
                std::cout << "[MATCH] Your resting " 
                          << ((restingOrder.side == Side::BUY) ? "BUY" : "SELL")
                          << " order executed: " << tradeQty << " units @ $" << matchedPrice << "\n";
            }

            remainingQty -= tradeQty;
            restingOrder.quantity -= tradeQty;

            if (restingOrder.quantity <= 0) {
                Order fullyExecutedOrder = restingOrder;
                queue.pop_front();
                refillIcebergOrder(fullyExecutedOrder, tradeQty, onMatchPrice);
            }
        }

        if (queue.empty()) {
            it = oppositeBook.erase(it);
        } else {
            ++it;
        }
    }

    if (matched) {
        onMatchPrice(matchedPrice);
        checkStopTriggers(matchedPrice, onMatchPrice);
    }

    if (remainingQty > 0) {
        if (order.type == OrderType::ICEBERG) {
            // For ICEBERG orders, add the full order to ICEBERG book
            // but only show the display quantity in the normal book
            Order remainingOrder = order;  // Use original order, not workingOrder
            remainingOrder.quantity = std::min(remainingQty, order.displayQuantity);
            remainingOrder.type = OrderType::LIMIT;
            
            sameSideBook[order.price].push_back(remainingOrder);
            
            // Add full ICEBERG order to tracking
            addToIcebergTrackingOnly(order);
            
            Logger::getInstance().logRestingOrder(remainingOrder);
            Benchmark::getInstance().incrementCounter("Orders_Resting");
            
            if (order.userId == 0) {
                std::cout << "[ICEBERG] Your ICEBERG " << ((order.side == Side::BUY) ? "BUY" : "SELL")
                          << " order placed. Showing " << remainingOrder.quantity 
                          << " of " << order.totalQuantity << " shares @ $" << order.price << "\n";
            }
        } else if (order.type == OrderType::LIMIT) {
            Order remainingOrder = order;
            remainingOrder.quantity = remainingQty;

            sameSideBook[order.price].push_back(remainingOrder);
            
            Logger::getInstance().logRestingOrder(remainingOrder);
            Benchmark::getInstance().incrementCounter("Orders_Resting");
            
            if (order.userId == 0) {
                std::cout << "[RESTING] Your " << ((order.side == Side::BUY) ? "BUY" : "SELL")
                          << " order for " << remainingQty << " units @ $" << order.price 
                          << " is now in the order book waiting for a match.\n";
            }
        }
    }
}
