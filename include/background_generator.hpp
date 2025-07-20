#pragma once

#include "order_book.hpp"
#include <thread>
#include <atomic>
#include <random>

class BackgroundGenerator {
public:
    explicit BackgroundGenerator(OrderBook& orderBook);
    ~BackgroundGenerator();

    void start();
    void stop();

private:
    void tradeLoop();
    Order generateRandomOrder();

    OrderBook& orderBook;
    std::atomic<bool> running;
    std::thread bgThread;
    
    // Random number generators
    std::mt19937 rng;
    std::uniform_real_distribution<double> priceDist;
    std::uniform_real_distribution<double> qtyDist;
    std::uniform_int_distribution<int> userIdDist;
    std::uniform_int_distribution<int> sideDist;
    std::uniform_int_distribution<int> typeDist;
    
    int orderId;
};
