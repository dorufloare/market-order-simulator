#pragma once

#include "order.hpp"
#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

class Logger {
public:
    static Logger& getInstance();
    
    void logOrder(const Order& order);
    void logMatch(const Order& incomingOrder, const Order& restingOrder, double matchPrice, double matchQuantity);
    void logRestingOrder(const Order& order);
    
private:
    Logger();
    ~Logger();
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void ensureLogsDirectory();
    std::string getCurrentTimestamp();
    
    std::ofstream ordersFile;
    std::ofstream matchesFile;
    std::mutex logMutex;
};
