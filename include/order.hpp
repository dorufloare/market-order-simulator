#pragma once

#include <chrono> 
#include <iostream>
#include <string>

enum class OrderType {
    LIMIT,
    MARKET,
    STOP_LIMIT,
    STOP_MARKET
};

enum class Side {
    BUY,
    SELL
};

struct Order {
    int id;
    int userId;
    OrderType type;
    Side side;
    double price;
    double quantity;
    double triggerPrice = 0.0;
    std::chrono::high_resolution_clock::time_point timestamp;

    friend std::ostream& operator<<(std::ostream& os, const Order& order) {
        os << "Order{id: " << order.id
           << ", userId: " << order.userId
           << ", type: ";
        
        switch(order.type) {
            case OrderType::LIMIT: os << "LIMIT"; break;
            case OrderType::MARKET: os << "MARKET"; break;
            case OrderType::STOP_LIMIT: os << "STOP_LIMIT"; break;
            case OrderType::STOP_MARKET: os << "STOP_MARKET"; break;
        }
        
        os << ", side: " << (order.side == Side::BUY ? "BUY" : "SELL")
           << ", price: " << order.price
           << ", quantity: " << order.quantity;
           
        if (order.type == OrderType::STOP_LIMIT || order.type == OrderType::STOP_MARKET) {
            os << ", triggerPrice: " << order.triggerPrice;
        }
        
        os << ", timestamp: " << std::chrono::duration_cast<std::chrono::milliseconds>(order.timestamp.time_since_epoch()).count()
           << '}';
        return os;
    }    
};

