#include "order_book.hpp"
#include "logger.hpp"
#include "benchmark.hpp"
#include <iostream>

void OrderBook::addToStopBook(const Order& order) {
    if (order.side == Side::SELL) {
        stopAsks[order.triggerPrice].push_back(order);
    } else {
        stopBids[order.triggerPrice].push_back(order);
    }
}

void OrderBook::checkStopTriggers(double lastTradePrice, const std::function<void(double)>& onMatchPrice) {
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
                
                match(triggeredOrder, onMatchPrice);
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
                
                match(triggeredOrder, onMatchPrice);
            }
            
            auto forwardIt = std::next(it).base();
            stopBids.erase(forwardIt);
            break;
        } else {
            ++it;
        }
    }
}

