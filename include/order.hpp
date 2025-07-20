#pragma once

#include <chrono> 
#include <iostream>
#include <string>

enum class OrderType {
    LIMIT,
    MARKET
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
    std::chrono::high_resolution_clock::time_point timestamp;

    friend std::ostream& operator<<(std::ostream& os, const Order& order) {
        os << "Order{id: " << order.id
           << ", userId: " << order.userId
           << ", type: " << (order.type == OrderType::LIMIT ? "LIMIT" : "MARKET")
           << ", side: " << (order.side == Side::BUY ? "BUY" : "SELL")
           << ", price: " << order.price
           << ", quantity: " << order.quantity
           << ", timestamp: " << std::chrono::duration_cast<std::chrono::milliseconds>(order.timestamp.time_since_epoch()).count()
           << '}';
        return os;
    }    
};

