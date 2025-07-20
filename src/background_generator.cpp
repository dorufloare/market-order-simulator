#include "background_generator.hpp"
#include "benchmark.hpp"
#include "order.hpp"
#include <chrono>

BackgroundGenerator::BackgroundGenerator(OrderBook& orderBook_) 
    : orderBook(orderBook_),
      running(false),
      rng(std::random_device{}()),
      priceDist(70.0, 120.0), 
      qtyDist(1.0, 10.0),
      userIdDist(1001, 9999),
      sideDist(0, 1),
      typeDist(0, 2), // 0-1: LIMIT, 2: MARKET (more LIMIT orders)
      orderId(10000) {
}

BackgroundGenerator::~BackgroundGenerator() {
    stop();
}

void BackgroundGenerator::start() {
    if (!running.load()) {
        running.store(true);
        bgThread = std::thread(&BackgroundGenerator::tradeLoop, this);
    }
}

void BackgroundGenerator::stop() {
    if (running.load()) {
        running.store(false);
        if (bgThread.joinable()) {
            bgThread.join();
        }
    }
}

void BackgroundGenerator::tradeLoop() {
    while (running.load()) {
        {
            BENCHMARK_TIMER("Background_Order_Generation");
            
            Order order = generateRandomOrder();
            orderBook.match(order);
            Benchmark::getInstance().incrementCounter("Background_Orders_Generated");
        }

        // High frequency single generator - 2ms sleep = ~500 orders/second
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

Order BackgroundGenerator::generateRandomOrder() {
    Order order;
    order.id = orderId++;
    order.userId = userIdDist(rng);
    
    // Create mix of LIMIT and MARKET orders (2/3 LIMIT, 1/3 MARKET)
    int typeRoll = typeDist(rng);
    order.type = (typeRoll <= 1) ? OrderType::LIMIT : OrderType::MARKET;
    
    order.side = static_cast<Side>(sideDist(rng));
    order.price = priceDist(rng);
    order.quantity = qtyDist(rng);
    order.timestamp = std::chrono::high_resolution_clock::now();
    
    return order;
}
