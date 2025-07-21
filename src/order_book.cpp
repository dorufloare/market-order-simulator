#include "order_book.hpp"
#include "logger.hpp"
#include "benchmark.hpp"
#include <iostream>
#include <algorithm>
#include <functional>

void OrderBookShard::match(const Order& order, const std::function<void(double)>& onMatchPrice) {
    BENCHMARK_TIMER("OrderBook_Match");
    
    std::lock_guard<std::mutex> lock(shardMutex);
    
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
    
    Logger::getInstance().logOrder(order);
    Benchmark::getInstance().incrementCounter("Orders_Processed");

    auto& oppositeBook = (order.side == Side::BUY) ? asks : bids;
    auto& sameSideBook = (order.side == Side::BUY) ? bids : asks;

    double remainingQty = order.quantity;
    double matchedPrice = 0.0;
    bool matched = false;

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

            Logger::getInstance().logMatch(order, restingOrder, matchedPrice, tradeQty);
            Benchmark::getInstance().incrementCounter("Orders_Matched");
            Benchmark::getInstance().addToCounter("Volume_Traded", static_cast<long>(tradeQty * 100));

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
        checkStopTriggers(matchedPrice, onMatchPrice);
    }

    if (remainingQty > 0 && order.type == OrderType::LIMIT) {
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

void OrderBookShard::addToStopBook(const Order& order) {
    if (order.side == Side::SELL) {
        stopAsks[order.triggerPrice].push_back(order);
    } else {
        stopBids[order.triggerPrice].push_back(order);
    }
}

void OrderBookShard::checkStopTriggers(double lastTradePrice, const std::function<void(double)>& onMatchPrice) {
    BENCHMARK_TIMER("Stop_Trigger_Check");
    
    for (auto it = stopAsks.begin(); it != stopAsks.end();) {
        double triggerPrice = it->first;
        
        if (lastTradePrice <= triggerPrice) {
            auto& stopOrders = it->second;
            
            while (!stopOrders.empty()) {
                Order triggeredOrder = stopOrders.front();
                stopOrders.pop_front();
                
                if (triggeredOrder.type == OrderType::STOP_MARKET) {
                    triggeredOrder.type = OrderType::MARKET;
                    triggeredOrder.price = 0.0;  // Market orders don't need price
                } else {
                    // Price collar check
                    if (triggeredOrder.side == Side::SELL) {
                        if (triggeredOrder.price > lastTradePrice * 1.05) {
                            if (triggeredOrder.userId == 0) {
                                std::cout << "[ORDER REJECTED] Your STOP SELL limit $" << triggeredOrder.price 
                                          << " exceeds maximum allowed deviation from market price $" << lastTradePrice 
                                          << " (exchange price collar violation)\n";
                            }
                            Benchmark::getInstance().incrementCounter("Stop_Orders_Rejected");
                            continue;  
                        } else {
                            triggeredOrder.type = OrderType::LIMIT;
                        }
                    } else {
                        if (triggeredOrder.price < lastTradePrice * 0.95) {
                            if (triggeredOrder.userId == 0) {
                                std::cout << "[ORDER REJECTED] Your STOP BUY limit $" << triggeredOrder.price 
                                          << " exceeds maximum allowed deviation from market price $" << lastTradePrice 
                                          << " (exchange price collar violation)\n";
                            }
                            Benchmark::getInstance().incrementCounter("Stop_Orders_Rejected");
                            continue; 
                        } else {
                            triggeredOrder.type = OrderType::LIMIT;
                        }
                    }
                }
                
                Benchmark::getInstance().incrementCounter("Stop_Orders_Triggered");
                
                if (triggeredOrder.userId == 0) {
                    std::cout << "[STOP TRIGGERED] Your STOP " 
                              << ((triggeredOrder.side == Side::BUY) ? "BUY" : "SELL")
                              << " triggered at $" << lastTradePrice 
                              << " -> executing " << ((triggeredOrder.type == OrderType::MARKET) ? "MARKET" : "LIMIT")
                              << " order\n";
                }
                
                processTriggeredOrder(triggeredOrder, onMatchPrice);
            }
            
            it = stopAsks.erase(it);  
        } else {
            ++it;
        }
    }
    
    for (auto it = stopBids.rbegin(); it != stopBids.rend();) {
        double triggerPrice = it->first;
        
        if (lastTradePrice >= triggerPrice) {
            auto& stopOrders = it->second;
            
            while (!stopOrders.empty()) {
                Order triggeredOrder = stopOrders.front();
                stopOrders.pop_front();
                
                if (triggeredOrder.type == OrderType::STOP_MARKET) {
                    triggeredOrder.type = OrderType::MARKET;
                    triggeredOrder.price = 0.0;  // Market orders don't need price
                } else {
                    // Price collar check
                    if (triggeredOrder.side == Side::SELL) {
                        if (triggeredOrder.price > lastTradePrice * 1.05) {
                            if (triggeredOrder.userId == 0) {
                                std::cout << "[ORDER REJECTED] Your STOP SELL limit $" << triggeredOrder.price 
                                          << " exceeds maximum allowed deviation from market price $" << lastTradePrice 
                                          << " (exchange price collar violation)\n";
                            }
                            Benchmark::getInstance().incrementCounter("Stop_Orders_Rejected");
                            continue;  
                        } else {
                            triggeredOrder.type = OrderType::LIMIT;
                        }
                    } else {
                        if (triggeredOrder.price < lastTradePrice * 0.95) {
                            if (triggeredOrder.userId == 0) {
                                std::cout << "[ORDER REJECTED] Your STOP BUY limit $" << triggeredOrder.price 
                                          << " exceeds maximum allowed deviation from market price $" << lastTradePrice 
                                          << " (exchange price collar violation)\n";
                            }
                            Benchmark::getInstance().incrementCounter("Stop_Orders_Rejected");
                            continue; 
                        } else {
                            triggeredOrder.type = OrderType::LIMIT;
                        }
                    }
                }
                
                Benchmark::getInstance().incrementCounter("Stop_Orders_Triggered");
                
                if (triggeredOrder.userId == 0) {
                    std::cout << "[STOP TRIGGERED] Your STOP " 
                              << ((triggeredOrder.side == Side::BUY) ? "BUY" : "SELL")
                              << " triggered at $" << lastTradePrice 
                              << " -> executing " << ((triggeredOrder.type == OrderType::MARKET) ? "MARKET" : "LIMIT")
                              << " order\n";
                }
                
                processTriggeredOrder(triggeredOrder, onMatchPrice);
            }
            
            auto forwardIt = std::next(it).base();
            stopBids.erase(forwardIt);
            break;
        } else {
            ++it;
        }
    }
}

void OrderBookShard::processTriggeredOrder(const Order& order, const std::function<void(double)>& onMatchPrice) {    
    auto& oppositeBook = (order.side == Side::BUY) ? asks : bids;
    auto& sameSideBook = (order.side == Side::BUY) ? bids : asks;

    double remainingQty = order.quantity;
    double matchedPrice = 0.0;
    bool matched = false;

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

            Logger::getInstance().logMatch(order, restingOrder, matchedPrice, tradeQty);
            Benchmark::getInstance().incrementCounter("Orders_Matched");
            Benchmark::getInstance().addToCounter("Volume_Traded", static_cast<long>(tradeQty * 100));

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

    if (remainingQty > 0 && order.type == OrderType::LIMIT) {
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
