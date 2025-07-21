#include "order_book.hpp"

double OrderBook::getLastTradedPrice() const {
    return last_traded_price.load();
}

void OrderBook::setLastTradedPrice(double price) {
    last_traded_price.store(price);
}
