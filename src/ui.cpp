#include "ui.hpp"
#include "benchmark.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

UI::UI(Engine& engine_) 
    : engine(engine_),
      running(false),
      orderId(1000000) {
}

UI::~UI() {
    stop();
}

void UI::start() {
    if (!running.load()) {
        running.store(true);
        interactiveInput();
        running.store(false);
    }
}

void UI::stop() {
    if (running.load()) {
        running.store(false);
    }
}

void UI::interactiveInput() {
    std::string sideStr;
    std::string typeStr;
    double price;
    double qty;

    std::cout << "\n=== Market Order Simulator ===\n";
    std::cout << "Commands:\n";
    std::cout << "- <BUY/SELL> <LIMIT/MARKET> <price> <quantity> : Place an order\n";
    std::cout << "- price : Show current last traded price\n";
    std::cout << "- help : Show detailed help\n";
    std::cout << "- stats : Show performance statistics\n";
    std::cout << "- quit : Exit the simulator\n";
    std::cout << "Example: BUY LIMIT 100.5 10 or SELL MARKET 0 5\n\n";

    while (running.load()) {
        std::cout << "> ";

        if (!(std::cin >> sideStr)) {
            break;
        }

        if (sideStr == "quit" || sideStr == "QUIT" || sideStr == "exit" || sideStr == "EXIT") {
            break;
        }

        if (sideStr == "price" || sideStr == "PRICE") {
            double currentPrice = engine.getOrderBook().getLastTradedPrice();
            std::cout << "💰 Last Traded Price: $" << std::fixed << std::setprecision(2) << currentPrice << "\n";
            continue;
        }

        if (sideStr == "help" || sideStr == "HELP") {
            std::cout << "\n=== Order Book Trading Help ===\n";
            std::cout << "Order Types:\n";
            std::cout << "• LIMIT orders: Execute only at your specified price or better\n";
            std::cout << "  - If no match found immediately, your order waits in the book\n";
            std::cout << "  - You'll see [RESTING] when placed, [MATCH] when executed\n";
            std::cout << "• MARKET orders: Execute immediately at best available price\n";
            std::cout << "  - Price parameter ignored (use 0)\n";
            std::cout << "  - Always execute if there are opposite orders\n\n";
            std::cout << "Examples:\n";
            std::cout << "  BUY LIMIT 95.50 10   - Buy 10 units at $95.50 or better\n";
            std::cout << "  SELL MARKET 0 5      - Sell 5 units at best available price\n\n";
            continue;
        }

        if (sideStr == "stats" || sideStr == "STATS") {
            Benchmark::getInstance().displayRealTimeStats();
            continue;
        }

        if (!(std::cin >> typeStr >> price >> qty)) {
            std::cout << "Invalid format. Use: <BUY/SELL> <LIMIT/MARKET> <price> <quantity>\n";
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }

        if (sideStr != "BUY" && sideStr != "SELL") {
            std::cout << "Invalid side. Use 'BUY' or 'SELL'.\n";
            continue;
        }

        if (typeStr != "LIMIT" && typeStr != "MARKET") {
            std::cout << "Invalid order type. Use 'LIMIT' or 'MARKET'.\n";
            continue;
        }

        if (qty <= 0) {
            std::cout << "Quantity must be positive.\n";
            continue;
        }

        if (typeStr == "LIMIT" && price <= 0) {
            std::cout << "Price must be positive for LIMIT orders.\n";
            continue;
        }

        Side side = (sideStr == "BUY") ? Side::BUY : Side::SELL;
        OrderType orderType = (typeStr == "LIMIT") ? OrderType::LIMIT : OrderType::MARKET;

        Order order {
            orderId++,
            0, // userId 0 for manual orders
            orderType,
            side,
            price,
            qty,
            std::chrono::high_resolution_clock::now()
        };

        engine.submitOrder(order);
        Benchmark::getInstance().incrementCounter("User_Orders_Submitted");
        std::cout << "✓ Order submitted: " << sideStr << " " << typeStr << " " << qty << " @ $" << std::fixed << std::setprecision(2) << price << "\n";
    }

    std::cout << "\nExiting...\n";
    running.store(false);
}
