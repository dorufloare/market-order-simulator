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
      typeDist(0, 19),
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
        //std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

Order BackgroundGenerator::generateRandomOrder() {
    Order order;
    order.id = orderId++;
    order.userId = userIdDist(rng);
    
    int typeRoll = typeDist(rng);
    if (typeRoll <= 9) {
        order.type = OrderType::LIMIT;
    } else if (typeRoll <= 12) {
        order.type = OrderType::MARKET;
    } else if (typeRoll <= 15) {
        order.type = OrderType::STOP_LIMIT;
    } else if (typeRoll <= 17) {
        order.type = OrderType::STOP_MARKET;
    } else {
        order.type = OrderType::ICEBERG;
    }
    
    order.side = static_cast<Side>(sideDist(rng));
    order.quantity = qtyDist(rng);
    
    if (order.type == OrderType::STOP_LIMIT || order.type == OrderType::STOP_MARKET) {
        double currentMarketPrice = orderBook.getLastTradedPrice();
        if (currentMarketPrice <= 0) {
            currentMarketPrice = 95.0;  
        }
        
        if (order.side == Side::SELL) {
            order.triggerPrice = currentMarketPrice * (0.85 + 0.10 * std::uniform_real_distribution<double>(0.0, 1.0)(rng));
            if (order.type == OrderType::STOP_LIMIT) {
                order.price = order.triggerPrice * (0.95 + 0.09 * std::uniform_real_distribution<double>(0.0, 1.0)(rng));
            } else {
                order.price = 0.0;  
            }
        } else {
            order.triggerPrice = currentMarketPrice * (1.05 + 0.10 * std::uniform_real_distribution<double>(0.0, 1.0)(rng));
            if (order.type == OrderType::STOP_LIMIT) {
                order.price = order.triggerPrice * (1.01 + 0.04 * std::uniform_real_distribution<double>(0.0, 1.0)(rng));
            } else {
                order.price = 0.0;  
            }
        }
    } else if (order.type == OrderType::ICEBERG) {
        order.price = priceDist(rng);
        order.totalQuantity = order.quantity * (3.0 + 5.0 * std::uniform_real_distribution<double>(0.0, 1.0)(rng));  
        order.displayQuantity = order.quantity; 
        order.quantity = order.totalQuantity;  
    } else {
        order.price = priceDist(rng);
    }
    
    order.timestamp = std::chrono::high_resolution_clock::now();
    
    return order;
}
