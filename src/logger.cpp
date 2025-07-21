#include "logger.hpp"
#include <iostream>
#include <filesystem>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    ensureLogsDirectory();
    
    ordersFile.open("logs/orders.log", std::ios::app);
    matchesFile.open("logs/matches.log", std::ios::app);
    
    if (!ordersFile.is_open()) {
        std::cerr << "Warning: Could not open orders.log file\n";
    }
    
    if (!matchesFile.is_open()) {
        std::cerr << "Warning: Could not open matches.log file\n";
    }

    ordersFile.seekp(0, std::ios::end);
    if (ordersFile.tellp() == 0) {
        ordersFile << "Timestamp,OrderID,UserID,Type,Side,Price,Quantity,TriggerPrice,Status\n";
    }
    
    matchesFile.seekp(0, std::ios::end);
    if (matchesFile.tellp() == 0) {
        matchesFile << "Timestamp,IncomingOrderID,RestingOrderID,MatchPrice,MatchQuantity,IncomingSide,RestingSide\n";
    }
}

Logger::~Logger() {
    if (ordersFile.is_open()) {
        ordersFile.close();
    }
    if (matchesFile.is_open()) {
        matchesFile.close();
    }
}

void Logger::ensureLogsDirectory() {
    try {
        std::filesystem::create_directories("logs");
    } catch (const std::exception& e) {
        std::cerr << "Warning: Could not create logs directory: " << e.what() << "\n";
    }
}

std::string Logger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

void Logger::logOrder(const Order& order) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    if (ordersFile.is_open()) {
        std::string orderTypeStr;
        switch(order.type) {
            case OrderType::LIMIT: orderTypeStr = "LIMIT"; break;
            case OrderType::MARKET: orderTypeStr = "MARKET"; break;
            case OrderType::STOP_LIMIT: orderTypeStr = "STOP_LIMIT"; break;
            case OrderType::STOP_MARKET: orderTypeStr = "STOP_MARKET"; break;
        }
        
        ordersFile << getCurrentTimestamp() << ","
                  << order.id << ","
                  << order.userId << ","
                  << orderTypeStr << ","
                  << (order.side == Side::BUY ? "BUY" : "SELL") << ","
                  << std::fixed << std::setprecision(2) << order.price << ","
                  << std::fixed << std::setprecision(2) << order.quantity << ",";
        
        if (order.type == OrderType::STOP_LIMIT || order.type == OrderType::STOP_MARKET) {
            ordersFile << std::fixed << std::setprecision(2) << order.triggerPrice << ",";
        } else {
            ordersFile << "0.00,";
        }
        
        ordersFile << "SUBMITTED" << "\n";
        ordersFile.flush();
    }
}

void Logger::logMatch(const Order& incomingOrder, const Order& restingOrder, 
                     double matchPrice, double matchQuantity) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    if (matchesFile.is_open()) {
        matchesFile << getCurrentTimestamp() << ","
                   << incomingOrder.id << ","
                   << restingOrder.id << ","
                   << std::fixed << std::setprecision(2) << matchPrice << ","
                   << std::fixed << std::setprecision(2) << matchQuantity << ","
                   << (incomingOrder.side == Side::BUY ? "BUY" : "SELL") << ","
                   << (restingOrder.side == Side::BUY ? "BUY" : "SELL") << "\n";
        matchesFile.flush();
    }
}

void Logger::logRestingOrder(const Order& order) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    if (ordersFile.is_open()) {
        std::string orderTypeStr;
        switch(order.type) {
            case OrderType::LIMIT: orderTypeStr = "LIMIT"; break;
            case OrderType::MARKET: orderTypeStr = "MARKET"; break;
            case OrderType::STOP_LIMIT: orderTypeStr = "STOP_LIMIT"; break;
            case OrderType::STOP_MARKET: orderTypeStr = "STOP_MARKET"; break;
        }
        
        ordersFile << getCurrentTimestamp() << ","
                  << order.id << ","
                  << order.userId << ","
                  << orderTypeStr << ","
                  << (order.side == Side::BUY ? "BUY" : "SELL") << ","
                  << std::fixed << std::setprecision(2) << order.price << ","
                  << std::fixed << std::setprecision(2) << order.quantity << ",";
        
        if (order.type == OrderType::STOP_LIMIT || order.type == OrderType::STOP_MARKET) {
            ordersFile << std::fixed << std::setprecision(2) << order.triggerPrice << ",";
        } else {
            ordersFile << "0.00,";
        }
        
        ordersFile << "RESTING" << "\n";
        ordersFile.flush();
    }
}
