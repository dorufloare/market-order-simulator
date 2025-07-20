#pragma once
#include "order.hpp"
#include "order_book.hpp"
#include "thread_pool.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

class Engine {
public:
    explicit Engine(size_t workerCount = 4);

    ~Engine();

    void start();
    void stop();
    void submitOrder(const Order& order);
    
    OrderBook& getOrderBook();

private:
    void dispatchOrders();

    std::queue<Order> orderQueue;             
    std::mutex queueMutex;                    
    std::condition_variable cv;               
    std::atomic<bool> running;                

    ThreadPool pool;                            
    OrderBook orderBook;                        

    std::thread dispatcherThread;               
};
