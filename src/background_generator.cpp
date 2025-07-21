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
      typeDist(0, 4),
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

        // 2ms sleep = ~500 orders/sec
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

Order BackgroundGenerator::generateRandomOrder() {
    Order order;
    order.id = orderId++;
    order.userId = userIdDist(rng);
    
    int typeRoll = typeDist(rng);
    if (typeRoll <= 1) {
        order.type = OrderType::LIMIT;
    } else if (typeRoll == 2) {
        order.type = OrderType::MARKET;
    } else if (typeRoll == 3) {
        order.type = OrderType::STOP_LIMIT;
    } else {
        order.type = OrderType::STOP_MARKET;
    }
    
    order.side = static_cast<Side>(sideDist(rng));
    order.quantity = qtyDist(rng);
    
    if (order.type == OrderType::STOP_LIMIT || order.type == OrderType::STOP_MARKET) {
        // Get current market price for realistic STOP order generation
        double currentMarketPrice = orderBook.getLastTradedPrice();
        if (currentMarketPrice <= 0) {
            currentMarketPrice = 95.0;  // Default fallback price
        }
        
        if (order.side == Side::SELL) {
            // STOP SELL: trigger below current price (stop-loss)
            order.triggerPrice = currentMarketPrice * (0.85 + 0.10 * std::uniform_real_distribution<double>(0.0, 1.0)(rng));
            if (order.type == OrderType::STOP_LIMIT) {
                // Set limit price within collar range when triggered
                order.price = order.triggerPrice * (0.95 + 0.09 * std::uniform_real_distribution<double>(0.0, 1.0)(rng));
            } else {
                order.price = 0.0;  // STOP_MARKET doesn't need limit price
            }
        } else {
            // STOP BUY: trigger above current price (breakout)
            order.triggerPrice = currentMarketPrice * (1.05 + 0.10 * std::uniform_real_distribution<double>(0.0, 1.0)(rng));
            if (order.type == OrderType::STOP_LIMIT) {
                // Set limit price within collar range when triggered
                order.price = order.triggerPrice * (1.01 + 0.04 * std::uniform_real_distribution<double>(0.0, 1.0)(rng));
            } else {
                order.price = 0.0;  // STOP_MARKET doesn't need limit price
            }
        }
    } else {
        // Regular LIMIT/MARKET orders
        order.price = priceDist(rng);
    }
    
    order.timestamp = std::chrono::high_resolution_clock::now();
    
    return order;
}
