#include "order_book.hpp"
#include "logger.hpp"
#include "benchmark.hpp"
#include <iostream>
#include <algorithm>
#include <functional>

void OrderBookShard::match(const Order& order, const std::function<void(double)>& onMatchPrice) {
    BENCHMARK_TIMER("OrderBook_Match");
    
    std::lock_guard<std::mutex> lock(shardMutex);
    
    // Log the incoming order
    Logger::getInstance().logOrder(order);
    Benchmark::getInstance().incrementCounter("Orders_Processed");

    auto& oppositeBook = (order.side == Side::BUY) ? asks : bids;
    auto& sameSideBook = (order.side == Side::BUY) ? bids : asks;

    double remainingQty = order.quantity;
    double matchedPrice = 0.0;
    bool matched = false;

    // Try to match with opposite orders
    for (auto it = oppositeBook.begin(); it != oppositeBook.end() && remainingQty > 0;) {
        double bookPrice = it->first;

        bool priceMatches = false;
        if (order.type == OrderType::MARKET) {
            priceMatches = true;
        } else if (order.side == Side::BUY) {
            priceMatches = order.price >= bookPrice;
        } else {
            priceMatches = order.price <= bookPrice;
        }

        if (!priceMatches)
            break;

        auto& queue = it->second;
        while (!queue.empty() && remainingQty > 0) {
            Order& restingOrder = queue.front();

            double tradeQty = std::min(remainingQty, restingOrder.quantity);
            matchedPrice = restingOrder.price;
            matched = true;

            // Log the match
            Logger::getInstance().logMatch(order, restingOrder, matchedPrice, tradeQty);
            Benchmark::getInstance().incrementCounter("Orders_Matched");
            Benchmark::getInstance().addToCounter("Volume_Traded", static_cast<long>(tradeQty * 100)); // Track volume in cents

            // Notify if either the incoming order OR the resting order belongs to the user
            if (order.userId == 0) {
                std::cout << "[MATCH] You "
                          << ((order.side == Side::BUY) ? "bought" : "sold")
                          << " " << tradeQty << " units @ $" << matchedPrice << "\n";
            } else if (restingOrder.userId == 0) {
                std::cout << "[MATCH] Your resting " 
                          << ((restingOrder.side == Side::BUY) ? "BUY" : "SELL")
                          << " order executed: " << tradeQty << " units @ $" << matchedPrice << "\n";
            }

            remainingQty -= tradeQty;
            restingOrder.quantity -= tradeQty;

            if (restingOrder.quantity <= 0) {
                queue.pop_front();
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
    }

    // If not fully filled and it's a LIMIT order, add remaining to same side
    if (remainingQty > 0 && order.type == OrderType::LIMIT) {
        Order remainingOrder = order;
        remainingOrder.quantity = remainingQty;

        sameSideBook[order.price].push_back(remainingOrder);
        
        Logger::getInstance().logRestingOrder(remainingOrder);
        Benchmark::getInstance().incrementCounter("Orders_Resting");
        
        // Notify user if their order is resting in the book
        if (order.userId == 0) {
            std::cout << "[RESTING] Your " << ((order.side == Side::BUY) ? "BUY" : "SELL")
                      << " order for " << remainingQty << " units @ $" << order.price 
                      << " is now in the order book waiting for a match.\n";
        }
    }
}

OrderBook::OrderBook(size_t shardCount_) : shardCount(shardCount_), shards(shardCount_) {}

size_t OrderBook::getShardIndex(double price) {
    return static_cast<size_t>(price) % shardCount;
}

void OrderBook::match(const Order& order) {
    size_t shardIdx = getShardIndex(order.price);

    auto updatePrice = [this](double price) {
        setLastTradedPrice(price);
    };

    shards[shardIdx].match(order, updatePrice);
}

double OrderBook::getLastTradedPrice() const {
    return last_traded_price.load();
}

void OrderBook::setLastTradedPrice(double price) {
    last_traded_price.store(price);
}
