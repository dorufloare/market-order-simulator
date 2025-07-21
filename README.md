# 🚀 Market Order Simulator

A fast, multi-threaded order book simulator built in C++17. I built this to learn how financial markets really work and practice building systems where every millisecond counts. Features real-time performance monitoring, detailed logging, and concurrent order processing.

## ✨ Key Features

### 🔥 **Fast Architecture**
- Multi-threaded engine that detects your CPU cores automatically
- Order book split into sections for better performance
- Background trading that generates 500+ orders per second

### 📊 **Performance Monitoring**
- Real-time timing measurements down to microseconds
- Tracks order processing speed and throughput
- Saves performance data to CSV files

### 🏗️ **Logging System**
- Records every order and trade to CSV files with timestamps
- Thread-safe logging that works across all components
- Creates log directories automatically

### 🎯 **Market Simulation**
- Supports LIMIT, MARKET, STOP_LIMIT, and STOP_MARKET order types
- Advanced STOP order functionality with realistic price collar validation
- Orders match based on price and time priority  
- Partial order fills and resting orders in the book
- Real-time price updates from completed trades
- Background generator creates realistic mix of all order types

---

## 🛠️ How to Build & Run

### What You Need
- C++17 compiler (GCC 7+, Clang 5+, or Visual Studio 2017+)
- CMake 3.16 or newer
- A system that supports threads

### Build Steps

```bash
# Get the code
git clone https://github.com/yourusername/market-order-simulator.git
cd market-order-simulator

# Build it
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run it
./OrderBookSimulator
```

### What You'll See
```
🚀 Starting Market Order Simulator with Performance Benchmarking...
🖥️  Hardware threads detected: 8
⚙️  Using 8 worker threads
🔥 Starting high-volume background trading simulation...
📈 Background generator started

=== Market Order Simulator ===
Commands:
- <BUY/SELL> <LIMIT/MARKET> <price> <quantity> : Place an order
- <BUY/SELL> <STOP_LIMIT/STOP_MARKET> <trigger_price> <limit_price> <quantity> : Place a STOP order
- price : Show current last traded price
- help : Show detailed help
- stats : Show performance statistics
- quit : Exit the simulator

> 
```

---

## 🎮 How to Use

### Basic Trading
```bash
# Buy 100 shares at $99.50 or better
> BUY LIMIT 99.50 100
✓ Order submitted: BUY LIMIT 100 @ $99.50

# Sell 50 shares at current market price
> SELL MARKET 0 50
✓ Order submitted: SELL MARKET 50 @ $0.00

# STOP order: Sell if price drops to $95 (stop-loss)
> SELL STOP_LIMIT 95.0 94.5 100
[STOP] Your STOP-LIMIT SELL order placed. Will trigger when price <= $95

# STOP order: Buy if price breaks above $105 (breakout)
> BUY STOP_MARKET 105.0 0 50
[STOP] Your STOP-MARKET BUY order placed. Will trigger when price >= $105

# Check what the last trade price was
> price
💰 Last Traded Price: $98.75
```

### Performance Stats
```bash
# See how fast everything is running
> stats
📊 === REAL-TIME PERFORMANCE STATS ===
📊 === REAL-TIME PERFORMANCE STATS ===
Program Runtime: 12.19s

📈 Counters:
  Stop Orders Triggered: 719 (59.0/sec)
  Volume Traded: 785487 (64420.0/sec)
  Orders Matched: 2753 (225.8/sec)
  Stop Orders Placed: 1864 (152.9/sec)
  Background Orders Generated  : 4670 (383.0/sec)
  Orders Resting: 1641 (134.6/sec)
  Stop Orders Rejected: 361 (29.6/sec)
  Orders Processed: 2806 (230.1/sec)

⏱️  Timing Statistics:
Operation                          Count       Avg(ms)     Min(ms)     Max(ms)     Throughput(ops/sec)
-----------------------------------------------------------------------------------------------
Stop Trigger Check                 1363        0.024       0.000       2.896       41281.1
Background Order Generation        4670        0.090       0.007       3.106       11158.6
OrderBook Match                    4670        0.087       0.007       3.102       11434.5

=======================================

> quit
Exiting...
🛑 Stopping background generator...

🏁 === FINAL PERFORMANCE REPORT ===
📊 === REAL-TIME PERFORMANCE STATS ===
Program Runtime: 12.19s

📈 Counters:
  Stop Orders Triggered: 719 (59.0/sec)
  Volume Traded: 785487 (64420.0/sec)
  Orders Matched: 2753 (225.8/sec)
  Stop Orders Placed: 1864 (152.9/sec)
  Background Orders Generated  : 4670 (383.0/sec)
  Orders Resting: 1641 (134.6/sec)
  Stop Orders Rejected: 361 (29.6/sec)
  Orders Processed: 2806 (230.1/sec)

⏱️  Timing Statistics:
Operation                          Count       Avg(ms)     Min(ms)     Max(ms)     Throughput(ops/sec)
-----------------------------------------------------------------------------------------------
Stop Trigger Check                 1363        0.024       0.000       2.896       41281.1
Background Order Generation        4670        0.090       0.007       3.106       11158.6
OrderBook Match                    4670        0.087       0.007       3.102       11434.5

=======================================
```

### Help
```bash
> help
=== Order Book Trading Help ===
Order Types:
• LIMIT orders: Execute only at your specified price or better
• MARKET orders: Execute immediately at best available price
• STOP_LIMIT orders: When triggered, become LIMIT orders (require trigger + limit price)
• STOP_MARKET orders: When triggered, become MARKET orders (require only trigger price)

STOP Order Examples:
• SELL STOP_LIMIT 95.0 94.5 100   - If price drops to $95, sell at $94.50 limit
• BUY STOP_MARKET 105.0 0 50      - If price rises to $105, buy at market
• Exchange enforces price collars - unrealistic limits get rejected

Examples:
• BUY LIMIT 100.25 500            - Buy 500 shares at $100.25 or better
• SELL MARKET 0 200               - Sell 200 shares at current market price
• SELL STOP_LIMIT 95.0 94.5 100   - Stop-loss: sell if price drops to $95
• BUY STOP_MARKET 105.0 0 50      - Breakout: buy if price rises to $105
• price                           - Show last traded price
• stats                  - Display performance metrics
• quit                   - Exit simulator
```

---

## 📈 Performance Results

### Actual Numbers
Real performance from testing:

```
📊 Performance Results:
┌────────────────────────────────────────────────────────┐
│ Metric                   │ Value                       │
├────────────────────────────────────────────────────────┤
│ Order Processing Rate    │ 383+ orders/sec             │
│ Average Latency          │ 0.087ms per order           │
│ Stop Trigger Checks      │ 41,281+ ops/sec             │
│ Volume Processing        │ 64,420+ trades/sec          │
│ Order Matching Rate      │ 225+ matches/sec            │
│ Stop Orders Triggered    │ 59+ triggers/sec            │
│ Background Orders/sec    │ 383 (2ms sleep)             │
└────────────────────────────────────────────────────────┘
```

### Key Numbers
```
- OrderBook Match:         11,434 ops/sec  
- Background Generation:   11,158 ops/sec
- Stop Trigger Checks:     41,281 ops/sec
- Volume Traded:          64,420 trades/sec
- Average latency:        0.087ms
- STOP Orders Triggered:  59 triggers/sec
```

---

## 🏗️ How It Works

The system runs multiple threads:
- Main thread handles user input
- Engine thread manages the order queue
- Worker threads process orders in parallel
- Background thread generates fake market activity

Orders flow like this:
```
User Input → Engine Queue → Worker Threads → Order Book
Background Generator → Order Book (direct)
All activity gets logged to CSV files
```

---

## 📊 Log Files

The program creates these log files in the `build/logs/` directory:

### orders.log
```csv
Timestamp,OrderID,UserID,Type,Side,Price,Quantity,TriggerPrice,Status
2025-07-21 12:37:15.708,10000,8438,LIMIT,SELL,119.21,6.13,0.00,SUBMITTED
2025-07-21 12:37:15.710,10000,8438,LIMIT,SELL,119.21,6.13,0.00,RESTING
2025-07-21 12:37:15.762,10001,2967,STOP_LIMIT,BUY,106.08,1.73,105.00,SUBMITTED
2025-07-21 12:37:15.763,10001,2967,STOP_LIMIT,BUY,106.08,1.73,105.00,STOP_PLACED
```

### matches.log
```csv
Timestamp,IncomingOrderID,RestingOrderID,MatchPrice,MatchQuantity,IncomingSide,RestingSide
2025-07-21 12:37:16.125,10002,10000,119.21,2.50,BUY,SELL
2025-07-21 12:37:16.847,10008,10003,106.08,1.73,SELL,BUY
```

### benchmarks.log
```csv
Operation,AvgTime(ms),MinTime(ms),MaxTime(ms),Count,Throughput(ops/sec)
Order_Queue_Wait,12194.129,12194.129,12194.129,1,0.1
Stop_Trigger_Check,0.024,0.000,2.896,1363,41281.1
Background_Order_Generation,0.090,0.007,3.106,4670,11158.6
OrderBook_Match,0.087,0.007,3.102,4670,11434.5
```

---

## 📄 License

This project is open source and available under the [MIT License](LICENSE).