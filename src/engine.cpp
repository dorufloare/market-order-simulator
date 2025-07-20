#include "engine.hpp"
#include "benchmark.hpp"

Engine::Engine(size_t workerCount) : running(false), pool(workerCount), orderBook(workerCount) {}

Engine::~Engine() {
    stop();
}

void Engine::start() {
    running = true;
    dispatcherThread = std::thread(&Engine::dispatchOrders, this);
}

void Engine::stop() {
    running = false;
    cv.notify_all();

    if (dispatcherThread.joinable())
        dispatcherThread.join();
}

void Engine::submitOrder(const Order& order) {
    BENCHMARK_TIMER("Order_Submission");
    Benchmark::getInstance().incrementCounter("Orders_Submitted");
    
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        orderQueue.push(order);
    }

    cv.notify_one();
}

OrderBook& Engine::getOrderBook() {
    return orderBook;
}

void Engine::dispatchOrders() {
    while (running) {
        Order order;

        {
            BENCHMARK_TIMER("Order_Queue_Wait");
            std::unique_lock<std::mutex> lock(queueMutex);

            // Wait until there's an order or the engine stops
            cv.wait(lock, [&]() { return !orderQueue.empty() || !running; });

            if (!running && orderQueue.empty())
                break;

            order = orderQueue.front();
            orderQueue.pop();
        }

        Benchmark::getInstance().incrementCounter("Orders_Dispatched");
        
        pool.enqueue([this, order]() {
            BENCHMARK_TIMER("Order_Processing");
            orderBook.match(order);
        });
    }
}
