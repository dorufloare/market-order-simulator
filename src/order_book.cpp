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

    double remainingQty = workingOrder.quantity;
    double matchedPrice = 0.0;
    bool matched = false;

    auto processQueue = [&](auto& queue, bool isBuy) {
        while (!queue.empty() && remainingQty > 0) {
            Order& restingOrder = queue.front();
            double tradeQty = std::min(remainingQty, restingOrder.quantity);
            matchedPrice = restingOrder.price;
            matched = true;
            Logger::getInstance().logMatch(workingOrder, restingOrder, matchedPrice, tradeQty);
            Benchmark::getInstance().incrementCounter("Orders_Matched");
            Benchmark::getInstance().addToCounter("Volume_Traded", static_cast<long>(tradeQty * 100));
            if (workingOrder.userId == 0) {
                std::cout << (isBuy ? "[MATCH] You bought " : "[MATCH] You sold ") << tradeQty << " units @ $" << matchedPrice << "\n";
            } else if (restingOrder.userId == 0) {
                std::cout << (isBuy ? "[MATCH] Your resting BUY order executed: " : "[MATCH] Your resting SELL order executed: ") << tradeQty << " units @ $" << matchedPrice << "\n";
            }
            remainingQty -= tradeQty;
            restingOrder.quantity -= tradeQty;
            if (restingOrder.quantity <= 0) {
                Order fullyExecutedOrder = restingOrder;
                queue.pop_front();
                refillIcebergOrder(fullyExecutedOrder, tradeQty, onMatchPrice);
            }
        }
    };

    auto matchLoop = [&](auto& book, bool reverse, bool isBuy) {
        if (reverse) {
            for (auto it = book.rbegin(); it != book.rend() && remainingQty > 0; ) {
                double bookPrice = it->first;
                bool priceMatches = (workingOrder.type == OrderType::MARKET) ? true : (workingOrder.price <= bookPrice);
                if (!priceMatches) break;
                auto& queue = it->second;
                processQueue(queue, isBuy);
                if (queue.empty()) {
                    auto eraseIt = it.base();
                    --eraseIt;
                    it = std::reverse_iterator<decltype(book.erase(eraseIt))>(book.erase(eraseIt));
                } else {
                    ++it;
                }
            }
        } else {
            for (auto it = book.begin(); it != book.end() && remainingQty > 0; ) {
                double bookPrice = it->first;
                bool priceMatches = (workingOrder.type == OrderType::MARKET) ? true : (workingOrder.price >= bookPrice);
                if (!priceMatches) break;
                auto& queue = it->second;
                processQueue(queue, isBuy);
                if (queue.empty()) {
                    it = book.erase(it);
                } else {
                    ++it;
                }
            }
        }
    };

    if (workingOrder.side == Side::BUY) {
        matchLoop(asks, false, true);
    } else {
        matchLoop(bids, true, false);
    }

    if (matched) {
        onMatchPrice(matchedPrice);
        checkStopTriggers(matchedPrice, onMatchPrice);
    }

    if (remainingQty > 0) {
        if (order.type == OrderType::ICEBERG) {
            Order remainingOrder = order;
            remainingOrder.quantity = std::min(remainingQty, order.displayQuantity);
            remainingOrder.type = OrderType::LIMIT;
            if (order.side == Side::BUY) {
                bids[order.price].push_back(remainingOrder);
            } else {
                asks[order.price].push_back(remainingOrder);
            }
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
            if (order.side == Side::BUY) {
                bids[order.price].push_back(remainingOrder);
            } else {
                asks[order.price].push_back(remainingOrder);
            }
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
