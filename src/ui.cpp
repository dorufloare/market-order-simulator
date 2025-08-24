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
    double triggerPrice = 0.0;  // For STOP orders

    std::cout << "\n=== Market Order Simulator ===\n";
    std::cout << "Commands:\n";
    std::cout << "- <BUY/SELL> <LIMIT/MARKET> <price> <quantity> : Place an order\n";
    std::cout << "- <BUY/SELL> <STOP_LIMIT/STOP_MARKET> <trigger_price> <limit_price> <quantity> : Place a STOP order\n";
    std::cout << "- <BUY/SELL> <ICEBERG> <price> <total_quantity> <display_quantity> : Place an ICEBERG order\n";
    std::cout << "- price : Show current last traded price\n";
    std::cout << "- help : Show detailed help\n";
    std::cout << "- stats : Show performance statistics\n";
    std::cout << "- quit : Exit the simulator\n";
    std::cout << "Examples:\n";
    std::cout << "  BUY LIMIT 100.5 10        - Buy at $100.50 or better\n";
    std::cout << "  SELL MARKET 0 5           - Sell immediately at market price\n";
    std::cout << "  SELL STOP_LIMIT 95.0 94.5 10 - If price drops to $95, sell at $94.50\n";
    std::cout << "  BUY ICEBERG 100.0 1000 100   - Buy 1000 shares, showing only 100 at a time\n";
    std::cout << "  BUY STOP_MARKET 105.0 0 5     - If price rises to $105, buy at market\n\n";

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
            std::cout << "  - Format: BUY/SELL LIMIT <price> <quantity>\n";
            std::cout << "• MARKET orders: Execute immediately at best available price\n";
            std::cout << "  - Price parameter ignored (use 0)\n";
            std::cout << "  - Format: BUY/SELL MARKET 0 <quantity>\n";
            std::cout << "• STOP_LIMIT orders: Trigger when price hits trigger, then become LIMIT orders\n";
            std::cout << "  - STOP BUY: Triggers when price RISES to trigger price\n";
            std::cout << "  - STOP SELL: Triggers when price DROPS to trigger price\n";
            std::cout << "  - ⚠️  Orders with unrealistic limits get REJECTED (like real exchanges)\n";
            std::cout << "  - Format: BUY/SELL STOP_LIMIT <trigger_price> <limit_price> <quantity>\n";
            std::cout << "• STOP_MARKET orders: Trigger when price hits trigger, then become MARKET orders\n";
            std::cout << "  - Always executes at current market price when triggered\n";
            std::cout << "  - Format: BUY/SELL STOP_MARKET <trigger_price> 0 <quantity>\n";
            std::cout << "• ICEBERG orders: Hide large order size by showing only small portions\n";
            std::cout << "  - Only display_quantity is visible in the order book\n";
            std::cout << "  - When visible portion fills, more shares automatically appear\n";
            std::cout << "  - Format: BUY/SELL ICEBERG <price> <total_quantity> <display_quantity>\n\n";
            std::cout << "Examples:\n";
            std::cout << "  BUY LIMIT 95.50 10              - Buy 10 units at $95.50 or better\n";
            std::cout << "  SELL MARKET 0 5                 - Sell 5 units at best available price\n";
            std::cout << "  SELL STOP_LIMIT 90.0 89.5 10    - If price drops to $90, try to sell at $89.50\n";
            std::cout << "  BUY STOP_MARKET 110.0 0 5       - If price rises to $110, buy at market\n";
            std::cout << "  BUY ICEBERG 100.0 1000 100      - Buy 1000 units, showing only 100 at a time\n\n";
            std::cout << "Strategy Tips:\n";
            std::cout << "• Use STOP SELL orders below current price as stop-losses\n";
            std::cout << "• Use STOP BUY orders above current price for breakout trades\n";
            std::cout << "• Use ICEBERG orders to hide large positions from other traders\n";
            std::cout << "• STOP_MARKET is safer - guarantees execution at market price\n";
            std::cout << "• Exchange enforces price collars - unrealistic limits get rejected\n\n";
            continue;
        }

        if (sideStr == "stats" || sideStr == "STATS") {
            Benchmark::getInstance().displayRealTimeStats();
            continue;
        }

        // Parse order type first to determine parameter count
        if (!(std::cin >> typeStr)) {
            std::cout << "Invalid format. Type 'help' for command examples.\n";
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }

        if (sideStr != "BUY" && sideStr != "SELL") {
            std::cout << "Invalid side. Use 'BUY' or 'SELL'.\n";
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }

        Side side = (sideStr == "BUY") ? Side::BUY : Side::SELL;

        bool isStopOrder = (typeStr == "STOP_LIMIT" || typeStr == "STOP_MARKET");
        bool isIcebergOrder = (typeStr == "ICEBERG");
        
        if (!isStopOrder && !isIcebergOrder && typeStr != "LIMIT" && typeStr != "MARKET") {
            std::cout << "Invalid order type. Use 'LIMIT', 'MARKET', 'STOP_LIMIT', 'STOP_MARKET', or 'ICEBERG'.\n";
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }

        double totalQty = 0.0;
        double displayQty = 0.0;

        if (isStopOrder) {
            if (!(std::cin >> triggerPrice >> price >> qty)) {
                std::cout << "Invalid format for STOP order. Use: " << sideStr << " " << typeStr << " <trigger_price> <limit_price> <quantity>\n";
                std::cout << "For STOP_MARKET orders, use 0 for limit_price.\n";
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                continue;
            }
        } else if (isIcebergOrder) {
            if (!(std::cin >> price >> totalQty >> displayQty)) {
                std::cout << "Invalid format for ICEBERG order. Use: " << sideStr << " " << typeStr << " <price> <total_quantity> <display_quantity>\n";
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                continue;
            }
        } else {
            if (!(std::cin >> price >> qty)) {
                std::cout << "Invalid format. Use: " << sideStr << " " << typeStr << " <price> <quantity>\n";
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                continue;
            }
        }

        // Validation
        if (isIcebergOrder) {
            if (totalQty <= 0 || displayQty <= 0) {
                std::cout << "Both total quantity and display quantity must be positive.\n";
                continue;
            }
            if (displayQty >= totalQty) {
                std::cout << "Display quantity must be less than total quantity.\n";
                continue;
            }
            if (price <= 0) {
                std::cout << "Price must be positive for ICEBERG orders.\n";
                continue;
            }
            qty = totalQty;  // Set qty for the order creation
        } else if (qty <= 0) {
            std::cout << "Quantity must be positive.\n";
            continue;
        }

        if (isStopOrder) {
            if (triggerPrice <= 0) {
                std::cout << "Trigger price must be positive for STOP orders.\n";
                continue;
            }
            if (typeStr == "STOP_LIMIT" && price <= 0) {
                std::cout << "Limit price must be positive for STOP_LIMIT orders.\n";
                continue;
            }
            
            if (typeStr == "STOP_LIMIT") {
                double currentPrice = engine.getOrderBook().getLastTradedPrice();
                if (side == Side::SELL && price > currentPrice * 1.10) {
                    std::cout << "⚠️  WARNING: Your SELL limit $" << price << " is " 
                              << std::fixed << std::setprecision(1) << ((price/currentPrice - 1) * 100) 
                              << "% above current price ($" << std::setprecision(2) << currentPrice 
                              << "). This will be REJECTED if triggered (exchange price collar).\n";
                } else if (side == Side::BUY && price < currentPrice * 0.90) {
                    std::cout << "⚠️  WARNING: Your BUY limit $" << price << " is " 
                              << std::fixed << std::setprecision(1) << ((1 - price/currentPrice) * 100) 
                              << "% below current price ($" << std::setprecision(2) << currentPrice 
                              << "). This will be REJECTED if triggered (exchange price collar).\n";
                }
            }
        } else {
            if (typeStr == "LIMIT" && price <= 0) {
                std::cout << "Price must be positive for LIMIT orders.\n";
                continue;
            }
        }

        // Create order
        OrderType orderType;
        
        if (typeStr == "LIMIT") {
            orderType = OrderType::LIMIT;
        } else if (typeStr == "MARKET") {
            orderType = OrderType::MARKET;
        } else if (typeStr == "STOP_LIMIT") {
            orderType = OrderType::STOP_LIMIT;
        } else if (typeStr == "STOP_MARKET") {
            orderType = OrderType::STOP_MARKET;
        } else {
            orderType = OrderType::ICEBERG;
        }

        Order order {
            orderId++,
            0,
            orderType,
            side,
            price,
            qty,
            triggerPrice,
            isIcebergOrder ? totalQty : 0.0,
            isIcebergOrder ? displayQty : 0.0,
            std::chrono::high_resolution_clock::now()
        };

        engine.submitOrder(order);
        Benchmark::getInstance().incrementCounter("User_Orders_Submitted");
        
        if (isStopOrder) {
            std::cout << "✓ STOP Order submitted: " << sideStr << " " << typeStr 
                      << " - Trigger: $" << std::fixed << std::setprecision(2) << triggerPrice;
            if (typeStr == "STOP_LIMIT") {
                std::cout << " → Limit: $" << price;
            }
            std::cout << " - Qty: " << qty << "\n";
            std::cout << "🎯 Your STOP order is now monitoring price movements...\n";
        } else if (isIcebergOrder) {
            std::cout << "✓ ICEBERG Order submitted: " << sideStr << " " << typeStr 
                      << " - Total: " << std::fixed << std::setprecision(0) << totalQty 
                      << " - Display: " << displayQty << " @ $" << std::setprecision(2) << price << "\n";
            std::cout << "🧊 Your ICEBERG order is hiding " << (totalQty - displayQty) << " shares behind the scenes...\n";
        } else {
            if (orderType == OrderType::MARKET) {
                double currentPrice = engine.getOrderBook().getLastTradedPrice();
                std::cout << "✓ Order submitted: " << sideStr << " " << typeStr << " " << qty << " @ Market ($" << std::fixed << std::setprecision(2) << currentPrice << ")\n";
            } else {
                std::cout << "✓ Order submitted: " << sideStr << " " << typeStr << " " << qty << " @ $" << std::fixed << std::setprecision(2) << price << "\n";
            }
        }
    }

    std::cout << "\nExiting...\n";
    running.store(false);
}
